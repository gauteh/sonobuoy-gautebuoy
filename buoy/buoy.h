/* Author:  Gaute Hope <eg@gaute.vetsj.com>
 * Date:    2012-01-18
 *
 * Buoy controller.
 *
 */

# pragma once

/* Version and settings */
# define VERSION GIT_DESC

/* ID for this Buoy */
# define BUOY_ID    1
# define BUOY_ID_S  "1"
# define BUOY_NAME  "One"

# ifndef ONLY_SPEC

# include "wirish.h"
# include "types.h"

/* Define to have debug messages sent to USB serial */
# define DIRECT_SERIAL 1

namespace Buoy {
  class BuoyMaster {
    public:
      GPS       *gps;
      ADS1282   *ad;
      RF        *rf;
      Store     *store;

      BuoyMaster ();
      void main ();
      void send_greeting ();

    private:
      void setup ();
  };

  /* Globally available instance of BuoyMaster */
  extern BuoyMaster *bu;
}

# endif

/* vim: set filetype=arduino :  */

