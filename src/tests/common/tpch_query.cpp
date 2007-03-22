/* -*- mode:C++; c-basic-offset:4 -*- */

#include "tests/common/tpch_query.h"

#include "core/tuple_fifo_directory.h"
#include "scheduler.h"
#include "workload/tpch/tpch_db.h"
#include "workload/common.h"



query_info_t query_init(int argc, char* argv[]) {

    util_init();

    /* check command line arguments */
    if ( argc < 2 ) {
	TRACE(TRACE_ALWAYS, "Usage: %s <num iterations>\n", argv[0]);
	exit(-1);
    }

    db_open(); /* destroy this below in query_main */

    /* parse command line arguments */
    int num_iterations = atoi(argv[1]);
    if ( num_iterations == 0 ) {
	TRACE(TRACE_ALWAYS, "Invalid iterations per client %s\n", argv[1]);
	exit(-1);
    }

    query_info_t info;
    info.num_iterations = num_iterations;
    info._policy = new scheduler::policy_os_t();

    return info;
}



void query_main(query_info_t& info, workload::driver_t* driver) {

    TRACE_SET(TRACE_ALWAYS | TRACE_STATISTICS | TRACE_QUERY_RESULTS);

    for(int i=0; i < info.num_iterations; i++) {
        stopwatch_t timer;
        driver->submit(info._policy);
        TRACE(TRACE_STATISTICS, "Query executed in %.3lf s\n", timer.time());
    }

    db_close();
}
