/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file test_tpcc_load_confirm.cpp
 *
 *  @brief Confirm loading TPC-C tables
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#include <unistd.h>
#include <sys/time.h>
#include <cmath>

#include "util.h"
#include "tests/common.h"

#include "workload/tpcc/tpcc_db.h"
#include "workload/tpcc/tpcc_struct.h"

#include "workload/common/table_load.h"


using namespace workload;
using namespace tpcc;


int main() {

    thread_init();
    
    /* NGM: Removed DB_TRUNCATE flag since it was causing an exception
       to be thrown in table open. As a temporary fix, we will simply
       delete the old database file before rebuilding. */

    tpcc::db_open(DB_JOINENV|DB_THREAD); 
    tpcc::db_read();
    tpcc::db_close();

    return 0;
}

