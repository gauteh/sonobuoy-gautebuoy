/* Author:  Gaute Hope <eg@gaute.vetsj.com>
 * Date:    2011-10-16
 *
 * Reads index and data files stored on memory card
 *
 * TODO: DAT files are little endian, this program only works on little endian systems.
 */

# include <stdint.h>
# include <fstream>
# include <iostream>
# include <iomanip>
# include <stdlib.h>
# include <getopt.h>
# include <math.h>
# include <time.h>
# include <string.h>

using namespace std;


namespace Zero {
  namespace DatReader {
# define ONLY_SPEC
    typedef char byte;
# include "store.h"

    Index open_index (string);
    void  print_index (Index);
    void  usage (string);
    void  header ();

    bool verbose = false;
    bool sparse = false;

    int main (int argc, char **argv) {
      string self = argv[0];

      /* Option parsing */
      if (argc < 2) {
        cerr << "[ERROR] You have to specify some options, see usage." << endl;
        usage (self);
        exit (1);
      }

      int opt;
      bool opt_only_index = false;
      bool twos = false;
      bool references = false;
      bool formatset = false;
      bool converttime = false;
      bool dtt = false; // Output file in .dtt format

# define DEFAULT_FORMAT_TIME "[%09lu]"
# define DEFAULT_FORMAT_TIME_S "[%Y-%m-%d %H:%M:%S."
# define DEFAULT_FORMAT_TIME_REST "%06lu]"
# define DEFAULT_FORMAT_UNSIGNED "%10lu\n"
# define DEFAULT_FORMAT_SIGNED "%10d\n"
# define DEFAULT_FORMAT DEFAULT_FORMAT_TIME " " DEFAULT_FORMAT_UNSIGNED
//# define DEFAULT_FORMAT "[%09lX] %08lu\n"
      string format;

      while ((opt = getopt(argc, argv, "isdcruhtHvf:")) != -1) {
        switch (opt)
        {
          case 's':
            sparse = true;
            break;
          case 'v':
            verbose = true;
            break;
          case 'h':
          case 'H':
          case 'u':
            usage (self);
            exit (0);
            break;
          case 'i':
            opt_only_index = true;
            break;
          case 'c':
            converttime = true;
            break;
          case 'f':
            formatset = true;
            format = optarg;
            format += '\n';
            break;
          case 't':
            twos = true;
            break;
          case 'r':
            references = true;
            break;
          case 'd':
            dtt = true;
            break;
          case '?':
            usage (self);
            exit (1);
        }
      }

      if (!formatset) {
        if (!converttime) {
          format = DEFAULT_FORMAT_TIME;
        }
        format += " ";
        format += (twos ? DEFAULT_FORMAT_SIGNED : DEFAULT_FORMAT_UNSIGNED);
      }

      if (verbose) header ();

      if (sparse && (references || verbose || opt_only_index)) {
          cerr << "-s can not be combined with -r, -v or -i." << endl;
        }

      if (optind >= argc) {
        cerr << "[ERROR] You must specify an id." << endl;
        exit (1);
      }

      if (references && opt_only_index) {
        cerr << "[ERROR] You can only specify either the -i or -r flag." << endl;
        exit (1);
      }

      if (converttime && formatset) {
        cerr << "[ERROR] You can only specify either the -c or -f flag." << endl;
        exit (1);
      }

      if (dtt) {
        if (converttime || formatset || opt_only_index || twos || references) {
          cerr << "[ERROR] You cannot combine the -d flag with any other flag." << endl;
          exit (1);
        }
      }

      string indexfn (argv[optind]);
      string datafn (indexfn);
      datafn.replace (indexfn.rfind (".IND"), 4, ".DAT");

      Index i = open_index (indexfn);
      if (!sparse)
        print_index (i);

      if (i.samples > 0 && !opt_only_index) {
        cerr << "=> Reading " << datafn << "..";
        if (!sparse) cerr << endl;
        else cerr << flush;

        /* Opening DATA */
        ifstream fd (datafn.c_str (), ios::binary);
        if (!fd.is_open ()) {
          cerr << endl << "[ERROR] Could not open data file: " << datafn << endl;
          exit (1);
        }

        bool corrupt = false;

        int ref = 0;
        int sam = 0;
        int sam_ref = 0; // sample since current reference

        bool failref        = false;
        uint32_t refid      = 0;
        uint64_t reft       = 0;
        uint32_t refstatus  = 0;
        char latitude[12];
        char longitude[12];
        uint32_t crc        = 0;


        /* Next file position expecting a reference */
        uint32_t nextrefpos = 0;
        uint32_t testcsum   = 0;

        while (!fd.eof ())
        {
          char timebuf[400];
          string out;

          if (ref < i.nrefs) {
            if (fd.tellg() == nextrefpos) {
              /* On reference, reading.. */
              failref = false;

              /* Checking checksum for previous ref/batch.. */
              if (ref > 0) {
                if (crc != testcsum) {
                  failref = true;
                  corrupt = true;
                  cerr << "=> [ERROR] [Checksum mismatch] For reference: " << (ref - 1) << ", from ref: " << ios::hex << crc << ", samples: " << ios::hex << testcsum << endl;
                }

                testcsum = 0;
              }



              for (int k = 0; k < (3 * (SAMPLE_LENGTH)); k++) {
                int r = fd.get ();
                if (r != 0) {
                  failref = true;
                }
              }

              fd.read (reinterpret_cast<char*>(&refid), sizeof(uint32_t));
              fd.read (reinterpret_cast<char*>(&reft), sizeof(uint64_t));
              fd.read (reinterpret_cast<char*>(&refstatus), sizeof(uint32_t));
              fd.read (reinterpret_cast<char*>(&latitude), sizeof(char) * 12);
              fd.read (reinterpret_cast<char*>(&longitude), sizeof(char) * 12);
              fd.read (reinterpret_cast<char*>(&crc), sizeof(uint32_t));

              for (int k = 0; k < (3 * (SAMPLE_LENGTH)); k++) {
                int r = fd.get ();
                if (r != 0) {
                  failref = true;
                  corrupt = true;
                }
              }

              if (fd.eof ()) {
                corrupt = true;
                failref = true;
                cerr << "=> [ERROR] [Unexpected EOF] While reading reference." << endl;
                break;
              }

              if (ref != refid) {
                corrupt = true;
                failref = true;
                cerr << "=> [ERROR] Reference id does not match reference number." << endl;
              }

              if (!sparse)
                cerr << "=> Reference [" << refid << "]: "
                     << reft << " (status: " << refstatus << "), "
                     << "Lat: " << latitude << ", Lon: " << longitude << endl;

              if (reft == 0) {
                cerr << "=> [WARNING] Reference is 0, store has no time reference." << endl;
                failref = true;
                corrupt = true;
              }

              if (strlen(latitude) < 3) {
                strcpy (latitude, "0S");
              }

              if (strlen(longitude) < 3) {
                strcpy (longitude, "0W");
              }

              /* Output DTT format */
              if (dtt) {
                cout << "R," << i.samples_per_reference << "," << refid << "," << reft << "," << refstatus << "," << latitude << "," << longitude
                     << "," << crc << endl;
              }

              ref++;
              sam_ref = 0; // reset sample on reference count

              /* Calculate next refpos */
              nextrefpos += BATCH_LENGTH * SAMPLE_LENGTH + SD_REFERENCE_LENGTH;
            }
          }

          /* On sample */
          uint64_t timestamp;
          sample ss;
          uint32_t s = 0;

          fd.read (reinterpret_cast<char*>(&ss), sizeof(sample));

          if (fd.eof ()) {
            if (verbose) cerr << "=> [EOF] File finished." << endl;
            break;
          }

          s = ss;
          testcsum ^= s;

          // TODO: Endianness probs?
          // __builtin_bswap32 (tt);

# define MICROS_PER_SAMPLE (1e6 / FREQUENCY)
          timestamp = reft + (sam_ref * MICROS_PER_SAMPLE);

          if (!references && !dtt) {
            out = "";
            if (converttime) {
              uint64_t ttime = timestamp / exp10(6);
              struct tm *t = gmtime ((const long int32_t *) (&ttime));
              strftime (timebuf, 400, DEFAULT_FORMAT_TIME_S, t);
              out += timebuf;

              ttime = timestamp - ttime * exp10(6);
              sprintf (timebuf, DEFAULT_FORMAT_TIME_REST, ttime);
              out += timebuf;

              out += format;

              printf (out.c_str (), s);
            } else {
              printf (format.c_str (), timestamp, s);
            }

          } else if (dtt) {
            /* Output DTT format */
            cout << s << endl;
          }

          sam++;
          sam_ref++;

          /* Used as a crappy test for a while since getting exactly 0 is fairly
           * unlikely :D
          if (s == 0) {
            cerr << "=> [ERROR] Sample == 0" << endl;
            corrupt = true;
            break;
          }
          */
        }

        if (!sparse || corrupt) cerr << "=> Read ";
        else cerr << "done, read ";
        cerr << sam << " samples (of " << i.samples << " expected)." << endl;

        if (sam != i.samples) {
          corrupt = true;
          cerr << "=> [ERROR] Number of samples not matching index." << endl;
        }

        if (corrupt)
        {
          cerr << "=> Warning: Datafile seems to be corrupt, please see any error messages." << endl;
          return 1;
        }
      }


      return 0;
    }

    Index open_index (string fn) {
      if (verbose and !sparse)
        cerr << "Opening index " << fn << "..";

      Index i;

      ifstream fi (fn.c_str (), ios::binary);

      if (!fi.is_open ()) {
        cerr << endl << "[ERROR] Could not open index." << endl;
        exit(1);
      }

      /* Reading index, member by member to avoid struct padding issues */
      fi.read (reinterpret_cast<char*>(&i.version), sizeof(i.version));
      fi.read (reinterpret_cast<char*>(&i.id), sizeof(i.id));
      fi.read (reinterpret_cast<char*>(&i.sample_l), sizeof(i.sample_l));
      fi.read (reinterpret_cast<char*>(&i.samples), sizeof(i.samples));
      fi.read (reinterpret_cast<char*>(&i.samples_per_reference), sizeof(i.samples_per_reference));
      fi.read (reinterpret_cast<char*>(&i.nrefs), sizeof(i.nrefs));

      if (i.version > 8) {
        fi.read (reinterpret_cast<char*>(&i.e_sdlag), sizeof(i.e_sdlag));
      } else {
        i.e_sdlag = false;
      }

      if (verbose)
        cerr << "done." << endl;

      return i;
    }

    void print_index (Index i) {
      cerr << "=> Index:             " << i.id << endl;
      cerr << "=> Version:           " << i.version << endl;
      cerr << "=> Sample length:     " << i.sample_l << endl;
      cerr << "=> Samples:           " << i.samples << endl;
      cerr << "=> Samples per ref:   " << i.samples_per_reference << endl;
      cerr << "=> References:        " << i.nrefs << endl;
      cerr << "=> SD lag:            " << (i.e_sdlag ? "Yes" : "No") << endl;
    }

    void usage (string argv) {
      header ();

      cerr << endl << "Usage: " << argv << " [-u|-h|-H] [-v] [-f format] [-i|-r] [-t] INDEX_FILE.IND" << endl;
      cerr << endl;
      cerr << " -u, -h or -H  Print this help text." << endl;
      cerr << " -v            Be verbose." << endl;
      cerr << " -s            Be sparse. May not be combined with -v, -r or -i." << endl;
      cerr << " -i            Only print index." << endl;
      cerr << " -r            Only print references." << endl;
      cerr << "               (only one of -r or -i may be used at the same time)" << endl;
      cerr << endl;
      cerr << " -t            Take twos complement to value." << endl;
      cerr << " -c            Convert unix time (cannot be used with -f)." << endl;
      cerr << endl;
      cerr << " -f format     Format specifies output format of data file, " \
              "follows" << endl;
      cerr << "               printf syntax, where the first argument will be " << endl;
      cerr << "               the time stamp and second the sample. A newline" << endl;
      cerr << "               will be added." << endl;
      cerr << "               Default: " << DEFAULT_FORMAT; // Has newline at end
      cerr << endl;
      cerr << " -d            Output in DTT format to stdout (may not be used with other flags)." << endl;
      cerr << endl;
      cerr << "  A datafile (.DAT) with the same name as the INDEX_FILE is " \
              "expected." << endl;
    }

    void header () {
      cerr << "Store reader for Gautebøye ( rev " << VERSION << " )" << endl;
      cerr << endl;
      cerr << "Store version .........: " << STORE_VERSION << endl;
      cerr << "Max samples ...........: " << MAX_SAMPLES_PER_FILE << endl;
      cerr << "Max references ........: " << MAX_REFERENCES << endl;
      cerr << "Max file size [B] .....: " << SD_DATA_FILE_SIZE << endl;
      cerr << endl;
    }
  }
}

int main (int argc, char **argv) {
  return Zero::DatReader::main (argc, argv);
}

