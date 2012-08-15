/* Author:  Gaute Hope <eg@gaute.vetsj.com>
 * Date:    2012-08-15
 *
 * dtttomseed.cpp: Converts a range of DTT's to a miniSEED store
 *
 * Requires: libmseed (developed for v2.5.1)
 *
 * All the ids supplied as an argument will be packed into one contiuous trace list,
 * you want more than one trace list make numerous invocations of this program with
 * each group that should be packed.
 *
 * Files are expected to be in current directory. Output will be written to files in current directory.
 *
 */

# include <stdint.h>
# include <iostream>
# include <fstream>
# include <vector>

# include "dtt.h"
# include "ms.h"

# define VERSION GIT_DESC

/* Configuration */
# define NETWORK  "GBU"
# define LOCATION "GAK1"
# define CHANNEL  "HYD"

using namespace std;

namespace Zero {
  namespace DttToMseed {

    void usage ();

    vector<int> ids;

    int main (int argc, char ** argv) {
      cout << "DttToMseed ( rev " << VERSION << " )" << endl;
      cout << "Author: Gaute Hope <eg@gaute.vetsj.com>, 2012-08-15" << endl << endl;

      cout << "Parsing id list.."; // {{{
      if (argc < 2) {
        cerr << "[E] No ids specified." << endl;
        usage ();
        return 1;
      }

      for (int i = 1; i < argc; i++) {
        int start, end;
        int n = sscanf (argv[i], "%d-%d", &start, &end);
        if (n == 2) {
          for (; start <= end; start++) {
            ids.push_back (start);
          }
        } else {
          int id;
          n = sscanf (argv[i], "%d", &id);
          if (n == 1) {
            ids.push_back (id);
          } else {
            cerr << "[e] Invalid argument: " << argv[i] << endl;
            return 1;
          }
        }
      }
      cout << "done." << endl;
      // }}}

      /* Set up miniSeed record (template) and trace list */
      Ms ms;

      /* Work through ids */
      vector<int>::iterator id = ids.begin ();
      while (id != ids.end ()) {

        /* Load DTT */
        Dtt dtt (*id);

        id++;
      }



      return 0;
    }

    void usage () {
      cout << endl << "Usage: dtttomseed sequence.." << endl;
      cout << "Specify ids as space separated sequence of single ids or range" << endl
           << "with a range of the format STARTID-ENDID (no spaces, start and" << endl
           << "end separated with a -)." << endl;

    }
  }
}

int main (int argc, char ** argv) {
  return Zero::DttToMseed::main (argc, argv);
}

