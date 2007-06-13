/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file table_load.cpp
 *
 *  @brief Calls the database load functions
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#include <cstdlib>
#include <cmath>
#include <sys/types.h>
#include <sys/stat.h>

#include "util.h"

#include "workload/common/bdb_env.h"
#include "workload/common/table_load.h"


// TPC-H related
#include "workload/tpch/tpch_env.h"
#include "workload/tpch/tpch_compare.h"


// TPC-C related
#include "workload/tpcc/tpcc_env.h"
#include "workload/tpcc/tpcc_compare.h"


using namespace workload;
using namespace tpch;
using namespace tpcc;


#define MAX_LINE_LENGTH 1024
#define MAX_FILENAME_SIZE 1024



/** Declaration of internal helper functions */

static void db_table_load(void (*tbl_loader) (Db*, FILE*),
                          Db* db,
                          const char* tbl_path, 
                          const char* tbl_filename);


static void db_table_read(void (*tbl_reader) (Db*),
                          Db* db);




/** Definitions of exported functions */


/** @fn db_tpch_load
 *
 *  @brief Load TPC-H tables.
 *
 *  @return void
 *
 *  @throw BdbException on error.
 */
void workload::db_tpch_load(const char* tbl_path) {

    for (int i = 0; i < _TPCH_TABLE_COUNT_; i++)
        db_table_load(tpch_tables[i].parse_tbl,
                      tpch_tables[i].db,
                      tbl_path,
                      tpch_tables[i].tbl_filename);
}


/** @fn db_tpcc_load
 *
 *  @brief Load TPC-C tables.
 *
 *  @return void
 *
 *  @throw BdbException on error.
 */
void workload::db_tpcc_load(const char* tbl_path) {

    for (int i = 0; i < _TPCC_TABLE_COUNT_; i++)
        db_table_load(tpcc_tables[i].parse_tbl,
                      tpcc_tables[i].db,
                      tbl_path,
                      tpcc_tables[i].tbl_filename);
}



/** @fn db_tpch_read
 *
 *  @brief Read TPC-H tables, for verification.
 *
 *  @return void
 *
 *  @throw BdbException on error.
 */
void workload::db_tpch_read() {

    for (int i = 0; i < _TPCH_TABLE_COUNT_; i++)
        db_table_read(tpch_tables[i].read_tbl,
                      tpch_tables[i].db);
}



/** @fn db_tpcc_read
 *
 *  @brief Read TPC-C tables, for verification.
 *
 *  @return void
 *
 *  @throw BdbException on error.
 */
void workload::db_tpcc_read() {

    for (int i = 0; i < _TPCC_TABLE_COUNT_; i++)
        db_table_read(tpcc_tables[i].read_tbl,
                      tpcc_tables[i].db);
}



/* definitions of internal helper functions */



/** @fn db_table_load
 *  @brief Static function. Opens the requested file, and if succeeds, it calls the 
 *  tbl_loader function, for the specific file and the database table handle.
 *
 *  @throw BdbException
 */

static void db_table_load(void (*tbl_loader) (Db*, FILE*),
                          Db* db,
                          const char* tbl_path, 
                          const char* tbl_filename) 
{    
    // prepend filename with common path
    c_str path("%s/%s", tbl_path, tbl_filename);

    FILE* fd = fopen(path.data(), "r");
    if (fd == NULL) {
        TRACE(TRACE_ALWAYS, "fopen() failed on %s\n", path.data());
        THROW1(BdbException, "fopen() failed");
    }

    tbl_loader(db, fd);

    if ( fclose(fd) ) {
        TRACE(TRACE_ALWAYS, "fclose() failed on %s\n", path.data());
        THROW1(BdbException, "fclose() failed");
    }
}



/** @fn db_table_read
 *  @brief Static function. It calls the tbl_reader function, for the specific
 *   database table handle.
 *
 *  @throw BdbException
 */

static void db_table_read(void (*tbl_reader) (Db*),
                          Db* db)
{    
    tbl_reader(db);
}
