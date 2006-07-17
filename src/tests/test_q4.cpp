/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file    : test_q4.cpp
 *  @brief   : Unittest for Q4
 *  @version : 0.1
 *  @history :
 6/16/2006: Initial version
*/ 

#include "engine/stages/tscan.h"
#include "engine/stages/aggregate.h"
#include "engine/stages/sort.h"
#include "engine/stages/hash_join.h"
#include "engine/stages/fdump.h"
#include "engine/stages/fscan.h"
#include "engine/stages/hash_join.h"
#include "engine/stages/partial_aggregate.h"
#include "engine/dispatcher.h"
#include "engine/util/stopwatch.h"
#include "trace.h"
#include "qpipe_panic.h"
#include "workload/tpch/tpch_db.h"
#include "engine/dispatcher/dispatcher_policy_os.h"

#include "workload/common.h"
#include "tests/common.h"

using namespace qpipe;







/** @fn    : main
 *  @brief : TPC-H Q6
 */

int main(int argc, char* argv[]) {

    thread_init();
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


    if ( !db_open() ) {
        TRACE(TRACE_ALWAYS, "db_open() failed\n");
        QPIPE_PANIC();
    }        


    // line up the stages...
    register_stage<tscan_stage_t>(2);
    register_stage<hash_join_stage_t>(1);
    register_stage<partial_aggregate_stage_t>(1);

    dispatcher_policy_t* dp = new dispatcher_policy_os_t();

    for(int i=0; i < iterations; i++) {
        stopwatch_t timer;
        TPCH_Q4::run(dp);
        TRACE(TRACE_ALWAYS, "Query executed in %.3lf s\n", timer.time());
    }

    delete dp;
    if ( !db_close() )
        TRACE(TRACE_ALWAYS, "db_close() failed\n");
    return 0;
}
