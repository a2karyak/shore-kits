/* -*- mode:C++; c-basic-offset:4 -*- */

#include "tests/common/tester_query.h"

#include "scheduler.h"


////////////////////////////
///// Staged execution /////


query_info_t query_init(int argc, char* argv[], int env) {

    thread_init();

    // parse command line args
    if ( argc < 2 ) {
	TRACE(TRACE_ALWAYS, "Usage: %s <num iterations>\n", argv[0]);
	exit(-1);
    }

    int num_iterations = atoi(argv[1]);
    if ( num_iterations <= 0 ) {
	TRACE(TRACE_ALWAYS, "Invalid iterations per client %s\n", argv[1]);
	exit(-1);
    }

    
    // open database
    switch (env) {

    case (TRX_MEM_ENV):
        break;

    default:
        break;
    }


    query_info_t info;
    info.num_iterations = num_iterations;
    info._policy = new scheduler::policy_os_t();

    return info;
}



void query_main(query_info_t& info, workload::driver_t* driver, int env) {

    TRACE_SET(TESTER_TRACE_MASK);

    for(int i=0; i < info.num_iterations; i++) {
        stopwatch_t timer;
        driver->submit(info._policy);
        TRACE(TRACE_STATISTICS, "Query executed in %.3lf s\n", timer.time());
    }


    // open database
    switch (env) {

    case (TRX_MEM_ENV):
        break;

    default:
        break;
    }

}



//////////////////////////////////////////////////////////////
///// Conventional (single-thread per request) execution /////



query_info_t query_single_thr_init(int argc, char* argv[], int env) {

    thread_init();

    // parse command line args
    if ( argc < 4 ) {
	TRACE(TRACE_ALWAYS, "Usage: %s <num clients> <num iterations> <scaling factor>\n", argv[0]);
	exit(-1);
    }

    int num_clients = atoi(argv[1]);
    if ( num_clients <= 0 ) {
	TRACE(TRACE_ALWAYS, "Invalid number of clients = %s\n", argv[1]);
	exit(-1);
    }

    int num_iterations = atoi(argv[2]);
    if ( num_iterations <= 0 ) {
	TRACE(TRACE_ALWAYS, "Invalid iterations per client = %s\n", argv[2]);
	exit(-1);
    }

    int num_sf = atoi(argv[3]);
    if ( num_sf <= 0 ) {
	TRACE(TRACE_ALWAYS, "Invalid scaling factor = %s\n", argv[3]);
	exit(-1);
    }


    
    // open database
    switch (env) {

    case (TRX_MEM_ENV):
        break;

    default:
        break;
    }


    query_info_t info;
    info.num_iterations = num_iterations;
    info._policy = new scheduler::policy_os_t();
    info.num_clients = num_clients;
    info.scaling_factor = num_sf;

    return info;
}

