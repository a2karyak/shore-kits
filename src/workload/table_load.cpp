/* -*- mode:C++; c-basic-offset:4 -*- */

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


using namespace workload;


#define MAX_LINE_LENGTH 1024
#define MAX_FILENAME_SIZE 1024


// Forward declaration of internal helper functions
void progress_reset();
void progress_update();
void progress_done();

static void db_table_load(void (*tbl_loader) (Db*, FILE*),
                          Db* db,
                          const char* tbl_path, 
                          const char* tbl_filename);


/* definitions of exported functions */

/**
 *  @brief Load TPC-H tables.
 *
 *  @return void
 *
 *  @throw BdbException on error.
 */
void tpch::db_load(const char* tbl_path) {

    for (int i = 0; i < _TPCH_TABLE_COUNT_; i++)
        db_table_load(tpch_tables[i].parse_tbl,
                      tpch_tables[i].db,
                      tbl_path,
                      tpch_tables[i].tbl_filename);
}



/* definitions of internal helper functions */

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
