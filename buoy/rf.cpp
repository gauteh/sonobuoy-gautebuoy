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

using namespace std;

namespace Buoy {
  RF::RF () {
    laststatus = 0;
    lastbatch  = 0;
    continuous_transfer = false;
    rf = this;
  }

  void RF::setup (BuoyMaster *b) {
    ad  = b->ad;
    gps = b->gps;

    RF_Serial.begin (RF_BAUDRATE);

    b->send_greeting ();
    send_debug ("[RF] RF subsystem initiated.");
  }

  void RF::loop () {
    /* Status is sent every second */
    if (millis () - laststatus > 1000) send_status ();

    /* Loop must run at least 2x speed (Nyquist) of batchfilltime */
    if (continuous_transfer) {
      if (ad->batch != lastbatch) {
       ad_message (AD_DATA_BATCH);
      }
    }
  }

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
  }

  void RF::ad_message (RF_AD_MESSAGE messagetype)
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
  }

  void RF::gps_message (RF_GPS_MESSAGE messagetype)
  {
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
  }

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
  }

  void RF::start_continuous_transfer () {
    continuous_transfer = true;
  }

  void RF::stop_continuous_transfer () {
    continuous_transfer = false;
  }
}

/* vim: set filetype=arduino :  */

