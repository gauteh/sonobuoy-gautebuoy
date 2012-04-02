/* Author:  Gaute Hope <eg@gaute.vetsj.com>
 * Date:    2012-01-31
 *
 * Communication protocol over Synapse RF Wireless.
 *
 */

# include <stdio.h>
# include <string.h>
# include "wirish.h"

# include "buoy.h"
# include "rf.h"
# include "ads1282.h"
# include "gps.h"
# include "store.h"

using namespace std;

namespace Buoy {
  RF::RF () {
    laststatus = 0;
    lastbatch  = 0;
    continuous_transfer = false;
    rf = this;

    isactive = false;
    stayactive = false;
    activated = 0;

    rf_buf_pos = 0;
  }

  void RF::setup (BuoyMaster *b) {
    ad  = b->ad;
    gps = b->gps;
    store = b->store;

    RF_Serial.begin (RF_BAUDRATE);

    b->send_greeting ();
    send_debug ("[RF] RF subsystem initiated.");
  }

  void RF::loop () {
    /* Handle incoming RF telegrams on serial line (non-blocking) {{{
     *
     *
     * States:
     * 0 = Waiting for start of telegram $
     * 1 = Receiving (between $ and *)
     * 2 = Waiting for Checksum digit 1
     * 3 = Waiting for Checksum digit 2
     */
    static int state = 0;

    /* Time out activeness.. */
    if (isactive) {
      if ((millis () - activated) >
         ((stayactive ? STAYACTIVE_TIMEOUT : ACTIVE_TIMEOUT) * 1000)) {
        isactive = false;
        stayactive = false;
      }
    }

    int ca = RF_Serial.available ();

    while (ca > 0) {
      char c = (char)RF_Serial.read ();

      if (rf_buf_pos >= RF_SERIAL_BUFLEN) {
        state = 0;
        rf_buf_pos = 0;
      }

      switch (state)
      {
        case 0:
          if (c == '$') {
            rf_buf[0] = '$';
            rf_buf_pos = 1;
            state = 1;
          }
          break;

        case 2:
          state = 3;
        case 1:
          if (c == '*') state = 2;

          rf_buf[rf_buf_pos] = c;
          rf_buf_pos++;
          break;

        case 3:
          rf_buf[rf_buf_pos] = c;
          rf_buf_pos++;
          rf_buf[rf_buf_pos] = 0;

          parse (); // Complete telegram received
          state = 0;
          break;

        /* Should not be reached. */
        default:
          state = 0;
          break;
      }

      ca--;
    }

    /* }}} Done telegram handler */
  }

  void RF::parse ()
  {
    /* RF parser (non-blocking) {{{
     *
     *
     */

    RF_TELEGRAM type = UNSPECIFIED;
    int tokeni = 0;
    int len    = rf_buf_pos; // Excluding NULL terminator
    int i      = 0;

    /* Test checksum before parsing */
    if (!test_checksum (rf_buf)) goto cmderror;

    /* Parse */
    while (i < len)
    {
      /*
      uint32_t ltmp = 0;
      uint32_t remainder = 0;
      */

      char token[80]; // Max length of token
      int j = 0;
      /* Get next token */
      while ((rf_buf[i] != ',' && rf_buf[i] != '*') && i < len) {
        token[j] = rf_buf[i];

        i++;
        j++;
      }
      i++; /* Skip delimiter */

      token[j] = 0;

      if (i < len) {
        if (tokeni == 0) {
          /* Determine telegram type */
          if (strcmp(token, "$A") == 0)
            type = ACTIVATE;
          else if (strcmp(token, "$DA") == 0)
            type = DEACTIVATE;
          else if (strcmp(token, "$GS") == 0)
            type = GETSTATUS;
          else if (strcmp(token, "$SA") == 0)
            type = STAYACTIVE;
          else if (strcmp(token, "$GIDS") == 0)
            type = GETIDS;
          else if (strcmp(token, "$GID") == 0)
            type = GETID;
          else if (strcmp(token, "$GB") == 0)
            type = GETBATCH;
          else {
            /* Cancel parsing */
            type = UNKNOWN;
            send_error (E_UNKNOWNCOMMAND);
            return;
          }
        } else {
          /* Must be activated first */
          if (isactive || (type == ACTIVATE) || (type == STAYACTIVE)) {
            switch (type)
            {
              // ACTIVATE {{{
              case ACTIVATE:
                isactive = true;
                stayactive = false;
                activated = millis ();
                break;
              // }}}

              // DEACTIVATE {{{
              case DEACTIVATE:
                isactive = false;
                stayactive = false;
                break;
              // }}}

              // STAYACTIVE {{{
              case STAYACTIVE:
                isactive = true;
                stayactive = true;
                activated = millis ();
                break;
              // }}}

              // GETSTATUS {{{
              case GETSTATUS:
                send_status ();
                break;
              // }}}

              // GETIDS {{{
              case GETIDS:
                switch (tokeni)
                {
                  /* first token specifies starting id to send */
                  case 1:
                    {
                    int r = sscanf (token, "%lu", &(id));
                    if (r != 1) goto cmderror;

                    store->send_indexes (id, GET_IDS_N);
                    }
                    break;
                }
                break;
                // }}}

              // GETID {{{
              case GETID:
                switch (tokeni)
                {
                  /* first token specifies starting id to send */
                  case 1:
                    {
                    int r = sscanf (token, "%lu", &(id));
                    if (r != 1) goto cmderror;

                    store->send_index (id);
                    }
                    break;
                }
                break;
                // }}}

              // GETLASTID {{{
              case GETLASTID:
                store->send_lastid ();
                break;
                // }}}

              // GETBATCH {{{
              case GETBATCH:
                switch (tokeni)
                {
                  case 1:
                    {
                    int r = sscanf (token, "%lu", &(id));
                    if (r != 1) goto cmderror;
                    }
                    break;

                  case 2:
                    {
                    int r = sscanf (token, "%lu", &(ref));
                    if (r != 1) goto cmderror;
                    }
                    break;

                  case 3:
                    {
                    int r = sscanf (token, "%lu", &(sample));
                    if (r != 1) goto cmderror;
                    }
                    break;

                  case 4:
                    {
                    int r = sscanf (token, "%lu", &(length));
                    if (r != 1) goto cmderror;

                    store->send_batch (id, ref, sample, length);
                    }
                    break;
                }
                break;
                // }}}

              default:
                /* Having reached here on an unknown or unspecified telegram
                 * parsing is cancelled. */
                return;
            }
          }
        }
      } else {
        /* Last token: Check sum */
      }
      tokeni++;
    }

    return;
cmderror:
    if (isactive) send_error (E_BADCOMMAND);
    return;

    /* Done parser }}} */
  }

  /* Status, debug and error messages {{{ */
  void RF::send_status () {
    static int sid;

    ad_message (AD_STATUS);
    gps_message (GPS_STATUS);

    /* Every 10 status */
    if (sid % 10 == 0) {
      rf_send_debug_f ("Uptime micros %u", micros ());
    }

    sid++;
    laststatus = millis ();
  }

  void RF::send_error (RF_ERROR code) {
    sprintf (buf, "$ERR,%d*", code);
    APPEND_CSUM (buf);
    RF_Serial.println (buf);
  }

  void RF::send_debug (const char * msg)
  {
    /* Format:
     * $DBG,[msg]*CS
     *
     */

    byte cs = gen_checksum ("DBG,", false);
    cs ^= gen_checksum (msg, false);

    RF_Serial.print   ("$DBG,");
    RF_Serial.print   (msg);
    RF_Serial.print   ("*");
    RF_Serial.print   (cs>>4, HEX);
    RF_Serial.println (cs&0xf, HEX);
  } // }}}

  void RF::ad_message (RF_AD_MESSAGE messagetype) // {{{
  {
    switch (messagetype)
    {
      case AD_STATUS:
        // $AD,S,[queue position], [queue fill time],[value],[config]*CS
        sprintf (buf, "$AD,S,%lu,%lu,0x%08lX,0x%08hX*", ad->position, ad->batchfilltime, ad->value, ad->reg.raw[1]);
        APPEND_CSUM (buf);
        RF_Serial.println (buf);

        break;

      case AD_DATA_BATCH:
        /* Send BATCH_LENGTH samples */

        /* Format and protocol:

         * 1. Initiate binary data stream:

         $AD,D,[k = number of samples],[reference],[reference_status]*CC

         * 2. Send one $ to indicate start of data

         * 3. Send k number of samples: 4 bytes * k

         * 4. Send end of data with checksum

         */
        {
          rf_send_debug_f ("On batch %d sending batch %d", ad->batch, lastbatch);
          uint32_t start    = (lastbatch * BATCH_LENGTH);
          uint32_t length   = BATCH_LENGTH;
          uint64_t ref      = ad->references[lastbatch];
          uint32_t refstat  = ad->reference_status[lastbatch];

          sprintf (buf, "$AD,D,%lu,%llu,%lu*", length, ref, refstat);
          APPEND_CSUM (buf);
          RF_Serial.println (buf);

          delayMicroseconds (100);

          byte csum = 0;

          /* Write '$' to signal start of binary data */
          RF_Serial.write ('$');

          //uint32_t lasts = 0;
          uint32_t s;

          for (uint32_t i = 0; i < length; i++)
          {
            s = ad->values[start + i];
            /* MSB first (big endian), means concatenating bytes on RX will
             * result in LSB first; little endian. */
            RF_Serial.write ((byte*)(&s), 4);

            csum = csum ^ ((byte*)&s)[0];
            csum = csum ^ ((byte*)&s)[1];
            csum = csum ^ ((byte*)&s)[2];
            csum = csum ^ ((byte*)&s)[3];

            //lasts = s;

            delayMicroseconds (100);
          }

          /* Send end of data with Checksum */
          sprintf (buf, "$AD,DE," F_CSUM "*", csum);
          APPEND_CSUM (buf);
          RF_Serial.println (buf);
          delayMicroseconds (100);

          /*
          SerialUSB.print ("[RF] Last sample: 0x");
          SerialUSB.println (lasts, HEX);
          rf_send_debug_f ("[RF] Last sample: 0x%lX", lasts);
          */

          lastbatch =  (lastbatch + 1) % BATCHES;
          if (lastbatch != ad->batch) {
            send_debug ("[RF] [Error] Did not finish sending batch before it was swapped.");
# if DIRECT_SERIAL
            SerialUSB.println ("[RF] [Error] Did not finish sending batch before it was swapped.");
# endif
          }
        }
        break;

      default:
        return;
    }
  } // }}}

  void RF::gps_message (RF_GPS_MESSAGE messagetype)
  { // Send GPS related message {{{
    switch (messagetype)
    {
      case GPS_STATUS:
        // $GPS,S,[lasttype],[telegrams received],[lasttelegram],Lat,Lon,unixtime,time,date,Valid,HAS_TIME,HAS_SYNC,HAS_SYNC_REFERENCE*CS
        // Valid: Y = Yes, N = No
        sprintf (buf, "$GPS,S,%d,%d,%s,%c,%s,%c,%lu,%lu,%02d%02d%02d,%c,%c,%c,%c*", gps->gps_data.lasttype, gps->gps_data.received, gps->gps_data.latitude, (gps->gps_data.north ? 'N' : 'S'), gps->gps_data.longitude, (gps->gps_data.east ? 'E' : 'W'), (uint32_t) gps->lastsecond, gps->gps_data.time, gps->gps_data.day, gps->gps_data.month, gps->gps_data.year, (gps->gps_data.valid ? 'Y' : 'N'), (gps->HAS_TIME ? 'Y' : 'N'), (gps->HAS_SYNC ? 'Y' : 'N'), (gps->HAS_SYNC_REFERENCE ? 'Y' : 'N'));

        break;

      default:
        return;
    }

    APPEND_CSUM (buf);
    RF_Serial.println (buf);
  } // }}}

  /* Checksum {{{ */
  byte RF::gen_checksum (const char *buf, bool skip)
  {
  /* Generate checksum for NULL terminated string
   * (skipping first and last char if skip (default)) */

    byte csum = 0;
    int len = strlen(buf);

    int i = 0;
    if (skip) {
      i = 1;
      len--;
    }

    for (; i < len; i++)
      csum = csum ^ ((byte)buf[i]);

    return csum;
  }

  bool RF::test_checksum (const char *buf)
  {
    /* Input: String including $ and * with HEX decimal checksum
     *        to test. NULL terminated.
     */
    int len = strlen(buf);

    uint16_t csum = 0;
    if (sscanf (&(buf[len-2]), F_CSUM, &csum) != 1) return false;

    uint32_t tsum = 0;
    for (int i = 1; i < (len - 3); i++)
      tsum = tsum ^ (uint8_t)buf[i];

    return tsum == csum;
  } // }}}

  void RF::start_continuous_transfer () {
    continuous_transfer = true;
  }

  void RF::stop_continuous_transfer () {
    continuous_transfer = false;
  }
}

/* vim: set filetype=arduino :  */

