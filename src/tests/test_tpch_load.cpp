// -*- mode:C++; c-basic-offset:4 -*-

#include "util.h"
#include "tests/common.h"
#include "workload/tpch/tpch_db.h"
#include "workload/bdb_config.h"

#include <unistd.h>
#include <sys/time.h>
#include <cmath>



/**
 *  @brief : Calls a table scan and outputs a specific projection
 */

int main() {

    thread_init();
    
    /* NGM: Removed DB_TRUNCATE flag since it was causing an exception
       to be thrown in table open. As a temporary fix, we will simply
       delete the old database file before rebuilding. */
    db_open(DB_CREATE|DB_TRUNCATE|DB_THREAD);
    db_load(BDB_TBL_DIRECTORY);
    db_close();
    return 0;
}
