/* Author:  Gaute Hope <eg@gaute.vetsj.com>
 * Date:    2012-01-18
 *
 * ADS1282 driver.
 *
 */


# include "buoy.h"


# if 0 // No need to configure PCA9535 for our usage, it is wired up though.
  # include "wirish.h"
  # include "Wire.h"
# endif

# include <string.h>

# include "ads1282.h"

# if HASGPS
  # include "gps.h"
# endif

using namespace std;

namespace Buoy {
  ADS1282::ADS1282 () {
    // Init class {{{
    disabled        = false;
    continuous_read = false;

    /*
    state.ports0 = 0;
    state.ports1 = 0;
    state.polarity0 = 0;
    state.polarity1 = 0;

    state.sync  = false;
    state.reset = false;
    state.pdwn  = false;
    */

    for (int i = 0; i < 11; i++) reg.raw[i] = 0;

    batch       = 0;
    value       = 0;
    //memset ((void*) values, 0, QUEUE_LENGTH * sizeof (uint32_t));
    position      = 0;
    totalsamples  = 0;
    batchstart    = millis ();
    batchfilltime = millis ();

    for (int i = 0; i < BATCHES; i++) {
      references [i] = 0;
# if HASGPS
      reference_status[i] = GPS::NOTHING;
# endif
    }

    return;
    // }}}
  }

  void ADS1282::setup (BuoyMaster *b) {
    // Set up interface and ADS1282 {{{
# if HASGPS
    gps = b->gps;
# endif

    /* Setup AD and get ready for data */
# if DEBUG_VERB
    SerialUSB.println ("[AD] Setting up ADS1282..");
# endif

# if 0
    /* Set up I2C */
    Wire.begin (AD_SDA, AD_SCL);
# endif

    /* Set up SPI */
    pinMode (AD_SCLK, OUTPUT);
    pinMode (AD_DIN, OUTPUT);
    pinMode (AD_DOUT, INPUT);
    pinMode (AD_nDRDY, INPUT);

    digitalWrite (AD_SCLK, LOW);
    digitalWrite (AD_DIN, LOW);

    /* Pick initial reference for batch, counting on GPS to have waited
     * for some initial reference.
     */
# if HASGPS
    references[batch] = (gps->reference * E6) + (micros () - gps->microdelta);
    reference_status[batch] = (gps->HAS_TIME & GPS::TIME) |
                              (gps->HAS_SYNC & GPS::SYNC) |
                              (gps->HAS_SYNC_REFERENCE & GPS::SYNC_REFERENCE);
# endif
    /* Configure AD */
    configure ();
    // }}}
  }

  void ADS1282::loop () {
    /* Run as part of main loop {{{ */
    /*

    static uint32_t lasts;

    if (!disabled)
    {

    }
    */
  } // }}}

# if DEBUG_VERB
  void ADS1282::print_status () {
    SerialUSB.print ("[AD] Queue pos: ");
    SerialUSB.print (position);

    SerialUSB.print (", samples: ");
    SerialUSB.print (totalsamples);

    SerialUSB.print (", value: 0x");
    SerialUSB.println (value, HEX);
  }
# endif

  void ADS1282::configure () {
    // Configure {{{
# if DEBUG_VERB
    SerialUSB.println ("[AD] Configuring ADS1282..");
# endif

    /*
    // Configure I2C (U7)

    // Important: Currently setting all ports to input, but still doing an
    // presumable in-effective RESET which seems to be necessary (!). Possibly
    // due to an time delay. Anyway; it works.

    int n = 0;
    Wire.beginTransmission (AD_I2C_ADDRESS);
    Wire.send (0x06);
    Wire.send (AD_I2C_CONTROL0);
    Wire.send (AD_I2C_CONTROL1);
    n = Wire.endTransmission ();

    if (n != SUCCESS) { error (); return; }
    */

    // Read configuration
    //read_pca9535 (CONTROL0);
    //read_pca9535 (POLARITY0);

    /* Set up outputs: (defined in header file)
     * - SYNC:   HIGH  (active low)
     * - PDWN:   HIGH  (active low)
     * - RESET:  HIGH  (active low)
     * - M0:     LOW
     * - M1:     HIGH
     * - MCLK:   LOW
     * - SUPSOR: LOW   (bipolar mode)
     * - EXTCLK: HIGH  (also hardwired high: _must_ be set HIGH)
     *
     * All other U7 outputs are meanwhile configured as inputs:
     * - PMODE (not available on ADS1282)
     * - MFLAG
     */

    /*
    Wire.beginTransmission (AD_I2C_ADDRESS);
    Wire.send (0x02);
    Wire.send (AD_I2C_OUTPUT0);
    Wire.send (AD_I2C_OUTPUT1);
    n = Wire.endTransmission ();
    if (n != SUCCESS) { error (__LINE__); return; }

    read_pca9535 (OUTPUT0);
    */

    delay (1000); // Allow EVM and AD to power up..
    //reset ();
    //delay (100);

# if DEBUG_VERB
    SerialUSB.println ("[AD] Reset by command and stop read data continuous..");
# endif
    send_command (RESET);
    delay (100);

    send_command (SDATAC);
    delay (100);

    read_registers ();
    configure_registers (); // resets ADC, 63 data cycles are lost..
    delay (100);
    read_registers ();
    delay (400);

# if DEBUG_VERB
    SerialUSB.println ("[AD] Configuration done.");
# endif
    // }}}
  }

  /* Continuous read and write {{{ */
  void ADS1282::start_continuous_read () {
    continuous_read = true;
# if DEBUG_VERB
    SerialUSB.println ("[AD] Sync and start read data continuous..");
# endif
    send_command (SYNC);
    send_command (RDATAC);
    delay (400);

    attachInterrupt (AD_nDRDY,&(ADS1282::drdy), FALLING);
  }

  void ADS1282::stop_continuous_read () {
# if DEBUG_VERB
    SerialUSB.println ("[AD] Reset by command and stop read data continuous..");
# endif
    detachInterrupt (AD_nDRDY);

    send_command (RESET);
    delay (100);

    send_command (SDATAC);
    delay (100);
    continuous_read = false;
  } // }}}

# if 0
  void ADS1282::read_pca9535 (PCA9535REGISTER reg) {
    /* Read registers of PCA9535RGE {{{
     *
     * Select register, if first register: reads both, if second: only last.
     */
    int n = 0;
    Wire.beginTransmission (AD_I2C_ADDRESS);
    Wire.send (reg);
    n = Wire.endTransmission ();
    if (n != SUCCESS) { error (__LINE__); return; }

    // Read outputs
    Wire.beginTransmission (AD_I2C_ADDRESS);
    n = Wire.requestFrom (AD_I2C_ADDRESS, 2);
    if (n == 2) {
      uint8 r = 0;
      switch (reg)
      {
        case OUTPUT0:
          /* Register 1 */
          r = Wire.receive ();
          state.sync = (r & AD_I2C_SYNC);
          state.pdwn = (r & AD_I2C_PDWN);
        case OUTPUT1:
          /* Register 2 */
          r = Wire.receive ();
          //state.pmode = (r & AD_I2C_PMODE);
          state.reset = (r & AD_I2C_RESET);
# if DEBUG_VERB
          SerialUSB.print   ("[AD] Sync: ");
          SerialUSB.print   ((state.sync ? "True " : "False"));
          SerialUSB.print   (", Reset: ");
          SerialUSB.print   ((state.reset ? "True " : "False"));
          SerialUSB.print   (", Power down: ");
          SerialUSB.println ((state.pdwn ? "True " : "False"));
# endif
          break;

        case POLARITY0:
          state.polarity0 = Wire.receive ();
        case POLARITY1:
          state.polarity1 = Wire.receive ();
# if DEBUG_VERB
          SerialUSB.print   ("[AD] PCA9535 polarity: (0)[0b");
          SerialUSB.print   (state.polarity0, BIN);
          SerialUSB.print   ("] (1)[0b");
          SerialUSB.print   (state.polarity1, BIN);
          SerialUSB.println ("]");
# endif
          break;

        case CONTROL0:
          state.ports0 = Wire.receive ();
        case CONTROL1:
          state.ports1 = Wire.receive ();
# if DEBUG_VERB
          SerialUSB.print   ("[AD] PCA9535 control:  (0)[0b");
          SerialUSB.print   (state.ports0, BIN);
          SerialUSB.print   ("] (1)[0b");
          SerialUSB.print   (state.ports1, BIN);
          SerialUSB.println ("]");
# endif
          break;

        /* Skipping inputs and polarity inverts.. */
        default:
          break;
      }
    }

    n = Wire.endTransmission ();
    if (n != SUCCESS) { error (__LINE__); return; }
    // }}}
  }
# endif

# if 0
  void ADS1282::reset_spi () {
    /* Reset SPI interface: Hold SCLK low for 64 nDRDY cycles
     * (warning: may block), is not used in buoy implementation.
     {{{*/
# if DEBUG_VERB
    SerialUSB.println ("[AD] [SPI] Resetting SPI..");
# endif
    digitalWrite (AD_SCLK, LOW);

    /* Make sure data is read continuously and nDRDY interrupt is detached */

    // TODO: Check for time out.
    for (int i = 0; i < 64; i++) {
      while (digitalRead (AD_nDRDY))  delayMicroseconds (2);
      while (!digitalRead (AD_nDRDY)) delayMicroseconds (2);
    }

    // }}}
  }
# endif

# if 0
  void ADS1282::reset () {
    // Reset ADS1282 over I2C / U7 {{{
# if DEBUG_VERB
    SerialUSB.println ("[AD] Resetting..");
# endif

    digitalWrite (AD_SCLK, LOW); // Make sure SPI interface is reset

    /* Sequence:
     *
     * - Set nRESET low
     * - delay min 2/fclk (currently 100 ms)
     * - Set nRESET high
     *
     */

    int n = 0;

    Wire.beginTransmission (AD_I2C_ADDRESS);
    Wire.send (0x02);
    Wire.send (AD_I2C_OUTPUT0);
    Wire.send (AD_I2C_OUTPUT1 & !AD_I2C_RESET);
    n = Wire.endTransmission ();
    if (n != SUCCESS) { error (__LINE__); return; }

    read_pca9535 (OUTPUT0);
    //digitalWrite (BOARD_LED_PIN, !digitalRead (AD_nDRDY));
    delay (1000);
    //digitalWrite (BOARD_LED_PIN, !digitalRead (AD_nDRDY));

    Wire.beginTransmission (AD_I2C_ADDRESS);
    Wire.send (0x02);
    Wire.send (AD_I2C_OUTPUT0);
    Wire.send (AD_I2C_OUTPUT1);
    n = Wire.endTransmission ();
    if (n != SUCCESS) { error (__LINE__); return; }

    read_pca9535 (OUTPUT0);
    //digitalWrite (BOARD_LED_PIN, !digitalRead (AD_nDRDY));
    delay (100);


# if DEBUG_VERB
    SerialUSB.println ("[AD] Reset done.");
# endif
    // }}}
  }
# endif

  void ADS1282::send_command (COMMAND cmd, uint8_t start, uint8_t n) {
    /* Send SPI command to ADS1282 {{{ */
# if DEBUG_VERB
    SerialUSB.print   ("[AD] [SPI] Sending command: [");

    // String representation of command {{{
    switch (cmd) {
      case WAKEUP:
        SerialUSB.print   ("WAKEUP");
        break;
      case STANDBY:
        SerialUSB.print   ("STANDBY");
        break;
      case SYNC:
        SerialUSB.print   ("SYNC");
        break;
      case RDATAC:
        SerialUSB.print   ("RDATAC");
        break;
      case SDATAC:
        SerialUSB.print   ("SDATAC");
        break;
      case RDATA:
        SerialUSB.print   ("RDATA");
        break;
      case OFSCAL:
        SerialUSB.print   ("OFSCAL");
        break;
      case GANCAL:
        SerialUSB.print   ("GANCAL");
        break;
      case RESET:
        SerialUSB.print   ("RESET");
        break;
      case RREG:
        SerialUSB.print   ("RREG");
        break;
      case WREG:
        SerialUSB.print   ("WREG");
        break;
    }; // }}}

    SerialUSB.print   ("] 0b");
    SerialUSB.println ((uint8_t) (cmd + start), BIN);
# endif

    switch (cmd) {
      case WAKEUP:
      case STANDBY:
      case SYNC:
      case RDATAC:
      case SDATAC:
      case RDATA:
      case OFSCAL:
      case GANCAL:
      case RESET:
        shift_out ((uint8_t) cmd);
        break;

      case RREG:
      case WREG:
# if DEBUG_VERB
        SerialUSB.print   ("[AD] [SPI] Sending: 0b");
        SerialUSB.println ((uint8_t) (n), BIN);
# endif
        shift_out ((uint8_t) (cmd + start));
        shift_out (n);
        break;
    };
    // }}}
  }

  void ADS1282::read_registers () {
    /* Read registers of ADS1282, SDATAC must allready have been issued {{{ */
# if DEBUG_VERB
    SerialUSB.println ("[AD] Reading registers..");
# endif
    send_command (RREG, 0, 10);

    shift_in_n (reg.raw, 11);

    for (int i = 0; i < 11; i++) {

# if DEBUG_VERB
      SerialUSB.print   ("[AD] Register [");
      SerialUSB.print   (i);
      SerialUSB.print   ("] 0b");
      SerialUSB.print   (reg.raw[i], BIN);
      SerialUSB.print   (" (0x");
      SerialUSB.print   (reg.raw[i], HEX);
      SerialUSB.println (")");
# endif

      switch (i)
      {
        case 0:
          reg.id = reg.raw[0];
          break;

        case 1:
          reg.sync = reg.raw[1] & (1 << 7);
          reg.mode = reg.raw[1] & (1 << 6);
          reg.datarate = (reg.raw[1] & 0b00111000) >> 3;
          reg.firphase = reg.raw[1] & (1 << 2);
          reg.filterselect = reg.raw[1] & 0b11;
          break;

        case 2:
          reg.muxselect = (reg.raw[2] & 0b01110000) >> 4;
          reg.pgachop = reg.raw[2] & 0b00001000;
          reg.pgagain = reg.raw[2] & 0b00000111;
          break;

        case 3:
          reg.hpf0 = reg.raw[3];
          break;
        case 4:
          reg.hpf1 = reg.raw[4];
          break;

        case 5:
          reg.ofc0 = reg.raw[5];
          break;
        case 6:
          reg.ofc1 = reg.raw[6];
          break;
        case 7:
          reg.ofc2 = reg.raw[7];
          break;

        case 8:
          reg.fsc0 = reg.raw[8];
          break;
        case 9:
          reg.fsc1 = reg.raw[9];
          break;
        case 0xa:
          reg.fsc2 = reg.raw[0xa];
          break;
      };

    }
    // }}}
  }

  void ADS1282::configure_registers () {
    /* Configure ADS1282 registers {{{ */
# if DEBUG_VERB
    SerialUSB.println ("[AD] Configuring registers..");
# endif

    // Config 0, changes from default:
    // - Sample rate: 250
# define AD_CONFIG0 0b01000010
    send_command (WREG, 1, 0);
# if DEBUG_VERB
    SerialUSB.print   ("[AD] [SPI] Sending: 0b");
    SerialUSB.println (AD_CONFIG0, BIN);
# endif
    shift_out (AD_CONFIG0);

    // Config 1, changes from default:
    // - Muxselect: Internal short via 400 Ohm
    /*
# define AD_CONFIG1 0b00101000
    SerialUSB.print   ("[AD] [SPI] Sending: 0b");
    SerialUSB.println (AD_CONFIG1, BIN);
    shift_out (AD_CONFIG1);
    */

    // High pass filter configuration (see Instrument Response in docs)
    // HPF[1:0] = 0x0337 => fHP = 0.5 Hz @ 250 SPS
    // HPF[1:0] = 0x0052 => fHP = 0.05 Hz @ 250 SPS
//# define AD_HPF1 0x03
//# define AD_HPF0 0x37
# define AD_HPF1 0x00
# define AD_HPF0 0x52
    send_command (WREG, 3, 1);
# if DEBUG_VERB
    SerialUSB.print   ("[AD] [SPI] Sending: 0b");
    SerialUSB.println (AD_HPF0, BIN);
# endif
    shift_out (AD_HPF0);
# if DEBUG_VERB
    SerialUSB.print   ("[AD] [SPI] Sending: 0b");
    SerialUSB.println (AD_HPF1, BIN);
# endif
    shift_out (AD_HPF1);

    // }}}
  }

  // Acquire {{{
  void ADS1282::drdy () {
    /* Static wrapper function for interrupt */
    bu->ad->acquire ();
  }

  void ADS1282::acquire () {
    /* In continuous mode: Must complete read operation before four
     *                     DRDY (ADS1282) periods. */

    /* On new batch, pick reference */
    if (position % BATCH_LENGTH == 0) {
      gps->assert_time ();

      /* Pick new reference for batch */
# if HASGPS
      references[batch] = (gps->reference * E6) + (micros () - gps->microdelta);
      reference_status[batch] = 0;
      if (gps->HAS_TIME) reference_status[batch] |= GPS::TIME;
      if (gps->HAS_SYNC) reference_status[batch] |= GPS::SYNC;
      if (gps->HAS_SYNC_REFERENCE) reference_status[batch] |= GPS::SYNC_REFERENCE;
      if (gps->valid) reference_status[batch] |= GPS::POSITION;

      if (!gps->ref_position_lock)
        gps->update_ref_position (); // if locked, use previous position.

      strcpy((char*) (reference_latitudes[batch]), (const char*) (gps->ref_latitude));
      strcpy((char*) (reference_longitudes[batch]), (const char*) (gps->ref_longitude));
# endif

      checksums[batch] = 0;
      togglePin (13);
    }

    uint8_t v[4];
    shift_in_n (v, 4);

    /*
     * Data is formatted in twos complement.
     *
     * In sinc filter mode output is scaled by 1/2.
     */

    value  = 0;
    value |= v[0] << 24;
    value |= v[1] << 16;
    value |= v[2] << 8;
    value |= v[3];

    /* LSB of value is a redundant sign bit unless output is clipped to +/- FS
     * then it is 1 for +FS and 0 for -FS.
     */

    /* Fill batch */
    values[position]  = value;
    checksums[batch] ^= value;

    position++;
    totalsamples++;

    /* Rolled out of batch, new batch */
    if (position % BATCH_LENGTH == 0) {
      /* Stats */
      batchfilltime = millis () - batchstart;
      batchstart    = millis ();

      batch++;
      batch         %= BATCHES;       // Increment batch or roll over
      position      %= QUEUE_LENGTH;  // Roll over queue position
    }
  }

  void ADS1282::acquire_on_command () {
    send_command (RDATA);

    /* Wait for falling nDRDY, time out after 5 secs */
    uint32_t start = millis ();
    while (digitalRead (AD_nDRDY)) if ((millis () - start)  > 5000) return;

    // Shift bits in (should wait min 100 ns)
    acquire ();

    SerialUSB.print   ("[AD] Value: ");
    SerialUSB.println (value, HEX);
  }
  // }}}

  /* SPI clocking operations: in and out {{{ */
# if 0
  uint8_t ADS1282::shift_in () {
    /* Read each bit, MSB first */
    uint8_t v = 0;
    for (int i = 7; i >= 0; i--) {
      digitalWrite (AD_SCLK, HIGH);
      digitalWrite (AD_SCLK, LOW);

      v |= (((uint8_t)digitalRead (AD_DOUT)) << (i-1));
    }

    return v;
  }
# endif

  void ADS1282::shift_in_n (uint8_t *v, int n) {
    /* Shift in n bytes to byte array v */

    /* Read each byte */
    for (int j = 0; j < n; j++) {

      v[j] = 0;

      /* Read each bit, MSB first */
      for (int i = 0; i < 8; ++i) {
        digitalWrite (AD_SCLK, HIGH);

        v[j] |= (((uint8_t)digitalRead (AD_DOUT)) << (7 - i));
        digitalWrite (AD_SCLK, LOW);
      }

      if (j < n) delayMicroseconds (11); // delay, min: 24 / fclk
    }
  }

  void ADS1282::shift_out (uint8_t v, bool delay) {
    /* Write each bit, MSB first */
    for (int i = 7; i >= 0; i--) {
      digitalWrite (AD_DIN, !!(v & (1 << i)));
      digitalWrite (AD_SCLK, HIGH);
      digitalWrite (AD_SCLK, LOW);
    }

    digitalWrite (AD_DIN, LOW);
    if (delay) delayMicroseconds (11); // delay, min: 24 / fclk, required if
                                       // reading or sending more commands.
  }

  // }}}

  void ADS1282::error (uint16_t code) {
    /* Some error on the ADS1282 - disable {{{ */
# if DEBUG_WARN
    SerialUSB.print ("[AD] E: ");
    SerialUSB.println (code);
# endif

    disabled = true;
    detachInterrupt (AD_nDRDY);
    continuous_read = false;
    // }}}
  }
}

/* vim: set filetype=arduino :  */

