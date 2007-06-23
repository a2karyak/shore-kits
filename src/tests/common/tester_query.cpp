/* -*- mode:C++; c-basic-offset:4 -*- */

#include "tests/common/tester_query.h"

#include "scheduler.h"

#include "workload/tpch/tpch_db.h"
#include "workload/tpcc/tpcc_db.h"


using namespace tpcc;
using namespace tpch;

query_info_t query_init(int argc, char* argv[], int env) {

    thread_init();

    // parse command line args
    if ( argc < 2 ) {
	TRACE(TRACE_ALWAYS, "Usage: %s <num iterations>\n", argv[0]);
	exit(-1);
    }

    int num_iterations = atoi(argv[1]);
    if ( num_iterations == 0 ) {
	TRACE(TRACE_ALWAYS, "Invalid iterations per client %s\n", argv[1]);
	exit(-1);
    }

    
    // open database
    if (env == QUERY_ENV) {
        tpch::db_open();
    }
    else {
        tpcc::db_open(BDB_TPCC_DB_OPEN_FLAGS);
    }



    query_info_t info;
    info.num_iterations = num_iterations;
    info._policy = new scheduler::policy_os_t();

    return info;
}



void query_main(query_info_t& info, workload::driver_t* driver, int env) {

    TRACE_SET(TRACE_ALWAYS | TRACE_STATISTICS | TRACE_QUERY_RESULTS);

    for(int i=0; i < info.num_iterations; i++) {
        stopwatch_t timer;
        driver->submit(info._policy);
        TRACE(TRACE_STATISTICS, "Query executed in %.3lf s\n", timer.time());
    }

    // close database
    if (env == QUERY_ENV) {
        tpch::db_close();
    }
    else {
        tpcc::db_close();
    }
}
