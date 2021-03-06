/* Author:  Gaute Hope <eg@gaute.vetsj.com>
 * Date:    2012-08-15
 *
 * ms.h: Interface to libmseed designed for buoy
 *
 */

# pragma once

# include <stdint.h>
# include <string>

# include <libmseed/libmseed.h>

# include "bdata.h"

using namespace std;

/* Structure:
 *
 * Trace list -> Sequence of IDs (DTT or DAT files)
 *   |..Trace -> ID (one DTT or DAT file)
 *      |..... Record -> Batch (reference with 1024 samples)
 *
 */

/* Time tolerance between MS records to join in trace */
# define TIMETOLERANCE 2.0
# define SAMPLERATETOLERANCE 250

namespace Zero {
  class Ms {
    public:
      char network[11];
      char station[11];
      char location[11];
      char channel[11];

      /* Main trace group */
      MSTraceGroup *mstg;

      Ms (const char*, const char*, const char*, const char*);
      ~Ms ();

      void add_bdata (Bdata *);
      bool pack_group ();
      static void record_handler (char *, int, void *);

  };
}

