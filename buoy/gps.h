/* Author:  Gaute Hope <eg@gaute.vetsj.com>
 * Date:    2012-01-31
 *
 * Interface to standard NMEA GPS.
 *
 *
 */

# pragma once

# ifndef ONLY_SPEC

# include <stdint.h>
# include "types.h"

namespace Buoy {
  class GPS {
# define TELEGRAM_LEN 80

# define GPS_BAUDRATE 4800
# define GPS_Serial Serial1

# define GPS_SYNC_PIN 27 // Should be 5V tolerant

    private:
      char gps_buf [TELEGRAM_LEN + 2];
      int  gps_buf_pos;

      /* Keep track of last sync pulse */
      volatile uint32_t lastsync;
      volatile uint8_t  referencerolled; // Pass debug message from main loop

      void parse ();

    public:
      RF      * rf;
      ADS1282 * ad;

      /* Telegram and data structures {{{ */
      typedef enum _GPS_TELEGRAM {
        UNSPECIFIED = 0,
        UNKNOWN,
        GPRMC,
        GPGGA,
        GPGLL,
        GPGSA,
        GPGSV,
        GPVTG,
      } GPS_TELEGRAM;

      typedef struct _GPS_DATA {
        GPS_TELEGRAM lasttype;
        char    lasttelegram[TELEGRAM_LEN];
        int     received; /* Received telegrams */
        bool    valid;
        int     fixtype;

        int     satellites;
        int     satellites_used[12];
        uint8_t    mode1;
        uint8_t    mode2;

        char    latitude[12];
        bool    north;    /* true = Latitude is north aligned, false = south */
        char    longitude[12];
        bool    east;     /* true = Longitude is east aligned, false = south */

        uint32_t   time;
        int     hour;
        int     minute;
        int     second;
        int     seconds_part;
        int     day;
        int     month;
        int     year;

        char    speedoverground[6];
        char    courseoverground[6]; /* True north */
      } GPS_DATA; // }}}

      GPS_DATA gps_data;

      GPS ();
      void setup (BuoyMaster *);
      void loop ();
      static void sync_pulse_int ();
      void sync_pulse ();
      void roll_reference ();
      void update_second ();

      /* Timing */
# define LEAP_SECONDS 19 // as of 2011
      bool HAS_LEAP_SECONDS;             // Has received leap seconds inf.
      bool HAS_TIME;                     // Has valid time from GPS
      volatile bool HAS_SYNC;            // Has PPS synced
      volatile bool HAS_SYNC_REFERENCE;  // Reference is set using PPS
      volatile bool IN_OVERFLOW;         // micros () is overflowed

      /* Current basis to calculate microsecond timestamp from
       *
       * Needs to be logged as reference for microsecond resolution time stamps in
       * data.
       *
       * ROLL_REFERENCE specifies how often the reference should be updated .
       */
      volatile uint64_t referencesecond;

# endif

# define ROLL_REFERENCE 40 // [s]

# ifndef ONLY_SPEC

      /* For Store and RF to know reference has been changed at given
       * queue position.
       *
       * previous_reference contains the original reference. referencesecond
       * contains the reference for all samples after update_reference_position.
       *
       * Is reset by ads1282.cpp when entering the batch interval again.
       */

      volatile uint64_t previous_reference;
      volatile bool     update_reference;
      volatile uint32_t update_reference_position;

      /* Last second received from GPS , the next pulse should indicate +1 second.
       * UTC (HAS_LEAP_SECONDS indicate wether this includes leap seconds)
       *
       * This is updated even without valid data and can only be trusted if
       * data.valid is set and gps_update_second has been run.
       *
       */
      volatile uint64_t lastsecond;

      /* The time in microseconds between Arduino micros() clock and reference second
       *
       * Synchronized with pulse from GPS.
       *
       */
      volatile uint64_t microdelta;

      /*
       * For detecting overflow: lastmicros is result of last micros ()
       *                         call in ad_drdy ()
       *
       * TODO: Check if we count the first step (0) correctly when in overflow.
       *
       */
      volatile uint64_t lastmicros;

      /* Get time related to reference
       *
       * ACCURACY:
       *
       * Accurate to (max(ulong) 2^32 - 1 ) % 4 for Arduino internal clock, micros()
       * has steps of 4. And to accuracy of GPS sync pulse (if available), about 1
       * us + delay for handling pulse.
       */

# define ULONG_MAX ((2^32) - 1)
# define TIME_FROM_REFERENCE(x) (!x->IN_OVERFLOW ? (micros() - x->microdelta) : (micros () + (ULONG_MAX - x->microdelta)))
# define CHECK_FOR_OVERFLOW(x) (x->IN_OVERFLOW = (micros () < x->lastmicros))

/* Overflow handling, the math.. {{{
 *
 * m   = micros
 * d   = delta
 *
 * at t0:
 * ref = const.
 * d   = m0
 * t   = ref + (m0 - d)
 *
 * at t1:
 * t   = ref + (m1 - d)
 *
 * at t2:
 * m > ULONG_MAX and is clocked around
 *
 * g is modulated m
 *
 * g = m - ULONG_MAX, we _will_ catch overrun/modulation before a second
 *                    modulation would have time to occur.
 * t = ref + (m - d)
 *   = ref + (g + ULONG_MAX - d)
 *   = ref + (g + (ULONG_MAX - d))
 *
 * for d > g, this means we will have to recalculate ref before g >= d.
 *
 * }}} */
  };
}

# endif

/* vim: set filetype=arduino :  */

