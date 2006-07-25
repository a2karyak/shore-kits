/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file    : test_q6.cpp
 *  @brief   : Unittest for Q1
 *  @version : 0.1
 *  @history :
 6/9/2006: Initial version
*/ 

#include "trace.h"
#include "qpipe_panic.h"
#include "engine/util/stopwatch.h"
#include "engine/dispatcher/dispatcher_policy_os.h"

#include "tests/common.h"
#include "workload/common.h"
#include "workload/tpch/tpch_db.h"
#include "workload/tpch/drivers/tpch_q1.h"

#include <unistd.h>
#include <sys/time.h>
#include <math.h>

using namespace qpipe;



int main(int argc, char* argv[]) {

    thread_init();
    db_open();
    TRACE_SET(TRACE_ALWAYS | TRACE_QUERY_RESULTS);


    // parse command line args
    if ( argc < 2 ) {
	TRACE(TRACE_ALWAYS, "Usage: %s <iterations>\n", argv[0]);
	exit(-1);
    }
    int iterations = atoi(argv[1]);
    if ( iterations == 0 ) {
	TRACE(TRACE_ALWAYS, "Invalid iterations per client %s\n", argv[1]);
	exit(-1);
    }
   
    
    register_stage<tscan_stage_t>(1);
    register_stage<partial_aggregate_stage_t>(1);

    dispatcher_policy_t* dp = new dispatcher_policy_os_t();

    for(int i=0; i < iterations; i++) {
        tpch_q1_driver driver(c_str("TPC-H Q1"));
        stopwatch_t timer;
        driver.submit(dp);
        TRACE(TRACE_ALWAYS, "Query executed in %.3lf s\n", timer.time());
    }

    delete dp;
    db_close();
    return 0;
}
