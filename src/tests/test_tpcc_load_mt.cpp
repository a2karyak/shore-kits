/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file test_tpcc_load_mt.cpp
 *
 *  @brief Test loading TPC-C tables. Testing multi-threaded version
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#include <unistd.h>
#include <sys/time.h>
#include <cmath>

#include "util.h"
#include "tests/common.h"
#include "workload/measurements.h" /* for execution_time_t */

#include "workload/tpcc/tpcc_db.h"
#include "workload/common/table_load.h"


using namespace workload;
using namespace tpcc;


int main() {

    TRACE_SET( LOAD_TRACE_MASK | TRACE_DEBUG );

    thread_init();

    execution_time_t _etime;
    
    /* NGM: Removed DB_TRUNCATE flag since it was causing an exception
       to be thrown in table open. As a temporary fix, we will simply
       delete the old database file before rebuilding. */

    _etime.start();
    tpcc::db_open(BDB_REGULAR_LOGGING, DB_CREATE|DB_THREAD); 
    db_tpcc_load_mt("tpcc_sf");
    tpcc::db_close();
    _etime.stop();

    TRACE (TRACE_STATISTICS, 
           "TPC-C Loading completed in %.2f secs\n", 
           _etime.time());

    return 0;
}

