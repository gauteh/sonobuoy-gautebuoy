/* Author:  Gaute Hope <eg@gaute.vetsj.com>
 * Date:    2012-10-10
 *
 * MATLAB interface to libmseed for writing miniSEED files. All processing
 * should be done in MATLAB before passing 'em to this function. This function
 * should be fairly dump so that errors or time problems are as obvious as
 * possible - as little as possible processing should be done here.
 *
 * Written for libmseed 2.7
 *
 * Memory: Is not handled very well, this function might be a source of a
 *         memory leak.
 *
 * Warning: Fix byte order issues.
 *
 */

# include <mex.h>
# include <libmseed/libmseed.h>

# include <time.h>

/* Prototypes */
void record_handler (char *record, int reclen, void *f);

/* This function is designed to work with one source at the time, it takes
 * a series of batches as input, per batch parameters are:
 *
 *  - Start time
 *  - End time (in case of e.g. fixing a time skew..)
 *  - Number of samples
 *  - Data quality (time and position status)
 *  - Data series
 *
 * This allows for time skews between the batches. The batches should follow
 * each other chronologically.
 *
 * The batches are formatted in one long data series, batches are fixed size,
 * the per batch parameters are passed along in a matrix with a row per batch.
 *
 * Common parameters for all batches:
 *
 *  - Source information (network, station, location, channel)
 *  - Sample rate
 *
 * Other parameters:
 *
 *  - Time tolerance to group batches
 *  - Sample rate tolerance
 */

void mexFunction (int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {

  /* Help and argument checking */
  if (nrhs < 5 || nrhs > 9) {
    mexPrintf ("MsWriteMseed - Gaute Hope <eg@gaute.vetsj.com> / 2012-10-10\n\n");
    mexPrintf ("   Create miniSEED files.\n\n");
    mexPrintf ("Usage: \n");
    mexPrintf ("  [fname] = MsWriteMseed ( batches, dataseries, network, station, location, [channel], [sample rate], [timetol], [sampletol] )\n\n");

    mexPrintf ("  batches is a matrix with a row for each batch with a column for:\n");
    mexPrintf ("    - Start time (hptime_t)\n");
    mexPrintf ("    - Number of samples\n");
    mexPrintf ("    - Data quality\n");
    mexPrintf ("  dataseries is a vector with data values in same order as batches\n");
    mexPrintf ("  network, station, location and channel (default BNR) specify source\n");
    mexPrintf ("  sample rate, default: 250 Hz\n");
    mexPrintf ("  timetol, tolerance between batches before splitting them in several traces, default: 1.0\n");
    mexPrintf ("  sampletol, tolerance for sample rate before splitting them in several traces, default: 250\n\n");

    mexPrintf ("  Return value: Output file name.\n\n");

    mexErrMsgTxt ("Incorrect number of arguments.");
  }

  /* Steps:
   *
   * 0. Get data.
   * 1. Set up trace group, configure common parameters.
   * 2. Add each batch as record.
   * 3. Write out to file with name defined by source info (SEISAN compatible).
   *
   */

  /* Batches matrix */
  const mxArray * mbatches = prhs[0];
  double * batches = mxGetPr (mbatches);
  int batches_n = mxGetN (mbatches);
  int batches_m = mxGetM (mbatches);

  if (batches_n != 3) {
    mexErrMsgTxt ("Wrong number of columns in batches.");
  }

  if (batches_m < 1) {
    mexErrMsgTxt ("Empty batches.");
  }

  /* Data series */
  const mxArray * dataseries = prhs[1];
  double * values     = mxGetPr (dataseries);
  int numberofsamples = mxGetN (dataseries);

  if (numberofsamples < 1) {
    mexErrMsgTxt ("No values.");
  }

  /* Network, station, location, channel */
  const mxArray * s = prhs[2]; /* network */
  int s_len         = mxGetN (s) + 1;
  char * network = mxCalloc (s_len, sizeof (char));
  mxGetString (s, network, s_len);
  for (int i = 0; i < s_len; i++) network[i] = toupper(network[i]);

  s     = prhs[3]; /* station */
  s_len = mxGetN (s) + 1;
  char * station = mxCalloc (s_len, sizeof (char));
  mxGetString (s, station, s_len);
  for (int i = 0; i < s_len; i++) station[i] = toupper(station[i]);

  s     = prhs[4]; /* location */
  s_len = mxGetN (s) + 1;
  char * location = mxCalloc (s_len, sizeof (char));
  mxGetString (s, location, s_len);
  for (int i = 0; i < s_len; i++) location[i] = toupper(location[i]);

  char * channel;
  if (nrhs > 5) {
    s     = prhs[5];
    s_len = mxGetN (s) + 1;
    channel = mxCalloc (s_len, sizeof (char));
    mxGetString (s, channel, s_len);
  } else {
    channel = mxCalloc (strlen("BNR0"), sizeof (char));
    strncpy (channel, "BNR", 3); /* default */
  }
  for (int i = 0; i < s_len; i++) channel[i] = toupper(channel[i]);

  /* sample rate */
  double samplerate = 250.0;
  if (nrhs > 6) {
    const mxArray * m = prhs[6];
    samplerate = mxGetScalar (m);
  }

  /* timetol */
  double timetol = 1.0;
  if (nrhs > 7) {
    const mxArray * m = prhs[7];
    timetol = mxGetScalar (m);
  }

  /* sampletol */
  double sampletol = 250.0;
  if (nrhs > 8) {
    const mxArray * m = prhs[8];
    sampletol = mxGetScalar (m);
  }

  mexPrintf ("Writing MiniSEED file for: %s_%s_%s_%s..\n", network, station,
      location, channel);

  /*
  mexPrintf ("=> Batches:         %4d\n", batches_m);
  mexPrintf ("=> Total samples:   %4d\n", numberofsamples);
  mexPrintf ("=> Sample rate:     %6.1f Hz\n", samplerate);
  mexPrintf ("=> Time tol.:       %6.1f s\n", timetol);
  mexPrintf ("=> Samplerate tol:  %6.1f Hz\n", sampletol);
  mexPrintf ("\n");
  */

  /* Ensure big-endianess */
  MS_PACKHEADERBYTEORDER(1);
  MS_PACKDATABYTEORDER(1);

  /* Set up miniSEED volume */
  ms_loginit ((void *)&mexPrintf, NULL, (void *)&mexWarnMsgTxt, NULL);

  /* Set up trace group */
  MSTraceGroup * mstg = mst_initgroup (NULL);

  /* Generate file name
   *
   * Format:
   * YYYY-MM-DD-HHMM-SS.NET_STA_LOC_CHA.mseed (date is start)
   */
  char * fname   = mxCalloc (1024, sizeof(char));
  char * timestr = mxCalloc (80, sizeof (char));
  hptime_t start = (hptime_t) batches[0];
  time_t   _start = (time_t) ((int64_t) start / 10e5);
  struct tm * ptm = gmtime (&_start);

  strftime (timestr, 80, "%Y-%m-%d-%H%M-%S", ptm);
  sprintf (fname, "%s.%s_%s_%s_%s.mseed", timestr, network, station, location,
      channel);

  /* Add batches as traces to tracegroup */
  //mexPrintf ("Loading batches..\n");
  double  *curdata   = values;
  int64_t cursample  = 0;

  for (int i = 0; i < batches_m; i++) {
    MSRecord *msr = msr_init (NULL);

    strcpy (msr->network, network);
    strcpy (msr->station, station);
    strcpy (msr->location, location);
    strcpy (msr->channel, channel);

    msr->samprate   = samplerate;
    msr->starttime  = (hptime_t) batches[i];
    msr->sampletype = 'i';
    msr->encoding   = DE_INT32;

    // TODO:  Byteorder should be little endian (0), seisan 9.0 interpreted
    //        this reversly. New seisan 9.1 needs correctly
    //        encoded <-> specified byte order.
    msr->byteorder  = 1; // should be 0 on my machine with this reading

    msr->numsamples = (int64_t) batches[i + 1 * batches_m];
    msr->samplecnt  = msr->numsamples;
    msr->dataquality = 0;

    /*
    mexPrintf ("Batch %d, start: %lu, numsamples: %d, quality: %d\n",
        i, msr->starttime, msr->numsamples, msr->dataquality);
    */

    if ((cursample + msr->numsamples) > numberofsamples) {
      mexErrMsgTxt ("Number of samples specified in batches does not match with avilable samples in dataseries.");
    }

    msr->datasamples = (int32_t*) malloc (msr->numsamples * sizeof(int32_t));

    for (int j = 0; j < msr->numsamples; j++) {
      ((int32_t*)msr->datasamples)[j] = (int32_t) values[cursample + j];
    }

    cursample += msr->numsamples;

    mst_addmsrtogroup (mstg, msr, 1, timetol, sampletol);
  }

  mst_printtracelist (mstg, 0, 1, 0);

  /* Heal group */
  //mexPrintf ("Healing group (joining adjacent trace segments)..\n");
  mst_groupheal (mstg, timetol, sampletol);

  /* Open file */
  FILE * f = fopen (fname, "w");

  /* Packing */
  //mexPrintf ("Packing traces..\n");
  //mexPrintf ("Writing to file: %s..\n", fname);
  # define DATABLOCK  2*4096
  # define ENCODING   DE_INT32
  # define BYTEORDER  1 // Big endian
  # define FLUSH      1
  # define VERBOSE    0
  int64_t psamples, precords;
  precords = mst_packgroup (mstg, &(record_handler), (void*) f,
                            DATABLOCK, ENCODING, BYTEORDER, &psamples,
                            FLUSH, VERBOSE, NULL);

  mexPrintf ("=> Packed %d samples in %d records to file %s.\n", psamples, precords, fname);

  /* Return file name */
  nlhs = 1;
  mxArray * fname_r = mxCreateString (fname);
  plhs[0] = fname_r;

  fclose (f);
}

void record_handler (char *record, int reclen, void *f) {
  fwrite (record, sizeof (char), reclen, (FILE *) f);
}

