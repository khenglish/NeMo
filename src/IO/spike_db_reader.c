//
// Created by Mark Plagge on 11/28/17.
//

#include "spike_db_reader.h"

/*
** This function is used to load the contents of a database file on disk
** into the "main" database of open database connection pInMemory, or
** to save the current contents of the database opened by pInMemory into
** a database file on disk. pInMemory is probably an in-memory database,
** but this function will also work fine if it is not.
**
** Parameter zFilename points to a nul-terminated string containing the
** name of the database file on disk to load from or save to. If parameter
** isSave is non-zero, then the contents of the file zFilename are
** overwritten with the contents of the database opened by pInMemory. If
** parameter isSave is zero, then the contents of the database opened by
** pInMemory are replaced by data loaded from the file zFilename.
*/
int loadOrSaveDb(const char *zFilename, int isSave) {
  int rc;                   /* Function return code */

  MPI_File fileheader;
  MPI_Status status;

  int max_spikes = 2097152;								// initially allocate 2M entries, or 24MB to store spike input data. Can expand like C++ vector if necessary
  spikedb = malloc(sizeof(spikedb_struct), max_spikes);

  int read_size = 4096;										// read this many bytes at a time. should be set equal to the IO system block size
  char* read_buf = malloc(sizeof(char), read_size);
  int bytes_read;
  char* token, line;
  int num_spikes = 0;

  MPI_File_open(MPI_COMM_WORLD, zFilename, MPI_MODE_RONLY,MPI_INFO_NULL, &fileheader);

  do
  {
    rc = MPI_File_read_all (fileheader, read_buf, read_size, MPI_Char, &status);
    MPI_Get_count(&status, MPI_CHAR, &bytes_read);

    while ((line = strsep(&read_buf,'\n')) != NULL)
    {
      spikedb[num_spikes].time = atoi(strsep(&line,','));
      spikedb[num_spikes].core = atoi(strsep(&line,','));
      spikedb[num_spikes].axon = atoi(line);					// I could add checks to make sure the neuron was written with non-null data, but that needs branches and will slow things down a ton

      num_spikes++;
      if (num_spikes == max_spikes)
      {
        max_spikes *= 2;
        spikedb = realloc(spikedb, sizeof(spikedb_struct) * max_spikes);
      }
    }
  } while (bytes_read == read_size && rc != -1)

  spikedb[num_spikes].core = -1;						// used to flag that this is the end of the array

  MPI_File_close(&fileheader);
  free(read_buf);
  return rc;
}

int testImport() {
  return 2;
}

/**
 * queries the sqlite db for a spike for axom.core. MessageData is populated with valid
 * info.
 * @param M - list pointer. Will be initialized in this function with n elements, where n is the number of elements found for this axon.
 *
 * @param coreID The core id.
 * @param axonID The local axon ID (core,axon notation)
 * @return 0 if spike not found, otherwise the number of elements found.
 */
/*int getSpikesFromAxonSQL(void *M, id_type coreID, id_type axonID) {

//    char * query = genSelectQuery(axonID, coreID);
//    char * countQuery = genCountQuery(axonID, coreID);

  list_t *spikelist = (list_t *) M;

  char *err_msg;
  sqlite3_stmt *res;
  int rc = 0;

  char *query = "SELECT   input_spikes.time\n"
      "FROM     input_spikes\n"
      "WHERE    ( input_spikes.axon = ?  ) AND ( input_spikes.core = ?  );";
  rc = sqlite3_prepare_v2(spikedb, query, -1, &res, 0);
  if (rc==SQLITE_OK) {
    sqlite3_bind_int64(res, 1, axonID);
    sqlite3_bind_int64(res, 2, coreID);
  } else {
    tw_error(TW_LOC, "SQL Error - unable to execute statement: Details: %s\n", sqlite3_errmsg(spikedb));

  }
  int cntr = 0;
  int step = sqlite3_step(res);
  if (step==SQLITE_ROW) {

    list_attributes_copy(spikelist, list_meter_int64_t, 1);

    while (step==SQLITE_ROW) {
      long sptime = sqlite3_column_int64(res, 0);
      list_append(spikelist, &sptime);
      cntr++;
      step = sqlite3_step(res);
    }
  }
  sqlite3_finalize(res);
  return cntr;

}*/

/**
 * queries the sqlite db for a spike for axon,core. returns 0 if no spike found, otherwise returns 1
 * @param coreID
 * @param axonID
 * @return
 */
/*int checkIfSpikesExistForAxon(id_type coreID, id_type axonID) {

}*/

/** @defgroup spin Spike Input Functions @{ */
/**
 * Override - generic function to get spike list from Axon ID. This function uses SQLITE.
 * @param timeList A null array - initalized when spikes are found by this function.
 * @param core Axon's core ID
 * @param axonID  Axon's local ID
 * @return The number of spikes found for this axon.
 */
/*int getSpikesFromAxon(void *timeList, id_type core, id_type axonID) {
  return getSpikesFromAxonSQL(timeList, core, axonID);
}*/

