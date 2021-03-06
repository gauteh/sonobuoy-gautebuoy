/* Author:  Gaute Hope <eg@gaute.vetsj.com>
 * Date:    2011-10-06
 *
 * Handle local SD card storage
 *
 */

# ifndef STORE_H
# define STORE_H

# include "buoy.h"
# include "gps.h"
# include "ads1282.h"

# ifndef ONLY_SPEC
# include <SdFat.h>

# define SD_SS    10
# define SD_MOSI  11
# define SD_MISO  12
# define SD_SCK   13

extern SdFat  sd;
extern bool   SD_AVAILABLE;

extern ulong  sd_status;

# endif /* ONLY_SPEC */

enum SD_STATUS {
  SD_VALID_GPS    = 0b1,
  SD_HAS_TIME     = 0b10,
  SD_HAS_SYNC     = 0b100,
  SD_HAS_SYNC_REF = 0b1000,
};

# ifndef ONLY_SPEC
/* Should be set when new reference has become available, will be written
 * outside of interrupt since it might otherwise come in the middle of a
 * writing operation */
extern volatile bool update_reference;
extern volatile uint update_reference_qposition;

void sd_setup ();
void sd_loop ();
void sd_init ();

void sd_open_index ();
void sd_write_index ();
void sd_next_index (ulong);
void sd_roll_data_file ();
void sd_open_data ();

void sd_write_batch ();
void sd_write_reference (ulong);

# endif /* ONLY_SPEC */

/* Data format */
# define STORE_VERSION 1uL
# define SAMPLE_LENGTH 3uL
# define TIMESTAMP_LENGTH 4uL

/* Maximum number of timestamp, sample pairs for each datafile */
# define EST_MINUTES_PER_DATAFILE 1uL
# define MAX_SAMPLES_PER_FILE (FREQUENCY * 60uL * EST_MINUTES_PER_DATAFILE)
# define MAX_REFERENCES ((MAX_SAMPLES_PER_FILE / ( FREQUENCY * ROLL_REFERENCE)) + 20uL)

# define _SD_DATA_FILE_SIZE (MAX_SAMPLES_PER_FILE * (SAMPLE_LENGTH + TIMESTAMP_LENGTH) + MAX_REFERENCES * 50uL)
# define SD_DATA_FILE_SIZE (_SD_DATA_FILE_SIZE + (_SD_DATA_FILE_SIZE % 512uL))

/* Data file format {{{
 *
 * Reference:
 *  - 3 * (SAMPLE_LENGTH + TIMESTAMP_LENGTH) with 0
 *  - Reference id: ulong
 *  - Reference:    ulong referencesecond [unix time]
 *  - Status bit:   ulong status
 *  - 3 * (SAMPLE_LENGTH + TIMESTAMP_LENGTH) with 0
 *  Total length: 54 bytes.
 *
 * Entry:
 *  - TIMESTAMP (4 bytes)
 *  - SAMPLE    (3 bytes)
 *  Total length: 7 bytes.
 *
 * }}} */

# define SD_REFERENCE_LENGTH 54uL

/* Last ID is one unsigned long */
typedef ulong LASTID;

typedef struct _Index {
  uint16_t version;     // Version of data (as defined in STORE_VERSION)
  uint32_t id;          // Id of index (limited by MAXID)

  uint16_t sample_l;    // Length of sample (bytes)
  uint16_t timestamp_l; // Length of time stamp (bytes)

  uint32_t samples;     // Can maximum reach MAX_SAMPLES_PER_FILE
  uint32_t nrefs;       // Current number of references
  uint32_t refs[MAX_REFERENCES]; // List with position of reference points.
} Index;


/* Using 8.3 file names limits the ID */
# define MAXID (10^8uL -1uL)

/* Files:
 * LASTID.LON     - file with current index id (not to be trusted..)
 *
 * [id].IND       - Index of data files, highest id is current.
 *
 * A new Index will be created after a number of data files or
 * if index cannot be parsed.
 *
 * [id].DAT       -  Data related to INDEX with same id
 *
 * A new data file will be created on start up, if file is corrupt, on
 * new index or if file is corrupt.
 */



# endif

/* vim: set filetype=arduino :  */


