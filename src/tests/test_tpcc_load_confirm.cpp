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

#include "workload/common.h"
#include "workload/common/table_load.h"

#include "workload/tpcc/tpcc_db.h"



using namespace workload;
using namespace tpcc;


int main() {

    thread_init();
    
    // line up the stages...
    register_stage<tscan_stage_t>();

    // open db
    tpcc::db_open(DB_JOINENV|DB_THREAD); 
    db_tpcc_read();
    tpcc::db_close();

    return 0;
}

