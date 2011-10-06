/* Author:  Gaute Hope <eg@gaute.vetsj.com>
 * Date:    2011-09-03
 *
 * Buoy controller.
 *
 */

/* Must include all external libraries here in main sketch for build system
 * to recgonize them as a dependency. */
# include <SdFat.h>

# include "buoy.h"
# include "ad7710.h"
# include "gps.h"
# include "rf.h"

ulong laststatus = 0;

void setup ()
{

  /* Set up devices */
  ad_setup ();
  gps_setup ();
  rf_setup ();

  /* Let devices settle */
  delay(10);
}

int i = 0;

void loop ()
{

  if ((millis () - laststatus) > 500) {
    /* Send status to RF */
    rf_send_status ();

    laststatus = millis ();
  }

  ad_loop ();
  gps_loop ();
}

/* vim: set filetype=arduino :  */

