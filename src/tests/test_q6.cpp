/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file    : test_q6.cpp
 *  @brief   : Unittest for Q6
 *  @version : 0.1
 *  @history :
 6/8/2006 : Updated to work with the new class definitions
 5/25/2006: Initial version
*/ 

#include "engine/thread.h"
#include "engine/dispatcher.h"
#include "engine/stages/tscan.h"
#include "engine/stages/aggregate.h"
#include "engine/dispatcher/dispatcher_policy_os.h"
#include "engine/util/stopwatch.h"
#include "trace.h"
#include "qpipe_panic.h"

#include "workload/common.h"
#include "tests/common.h"
#include "workload/tpch/tpch_db.h"

#include "db_cxx.h"

using namespace qpipe;



/** @fn    : main
 *  @brief : TPC-H Q6
 */

int main(int argc, char* argv[]) {

    thread_init();
    db_open();
    dispatcher_policy_t* dp = new dispatcher_policy_os_t();
    TRACE_SET(TRACE_ALWAYS);

    
    // parse command line args
    if ( argc < 2 ) {
	TRACE(TRACE_ALWAYS, "Usage: %s <num iterations>\n", argv[0]);
	exit(-1);
    }
    int num_iterations = atoi(argv[1]);
    if ( num_iterations == 0 ) {
	TRACE(TRACE_ALWAYS, "Invalid num iterations %s\n", argv[1]);
	exit(-1);
    }


    register_stage<tscan_stage_t>(1);
    register_stage<aggregate_stage_t>(1);
    

    for(int i=0; i < num_iterations; i++) {
        stopwatch_t timer;
        
        
        packet_t* q6_packet = create_q6_packet("Q6", dp);

        tuple_buffer_t* output_buffer = q6_packet->_output_buffer;
        dispatcher_t::dispatch_packet(q6_packet);
    
        tuple_t output;
        while(output_buffer->get_tuple(output)) {
            double* r = (double*)output.data;
            TRACE(TRACE_ALWAYS, "*** Q6 Count: %u. Sum: %lf.  ***\n", (unsigned)r[0], r[1]);
        }
        
        TRACE(TRACE_ALWAYS, "Query executed in %.3lf s\n", timer.time());
    }
    
 
    db_close();
    return 0;
}
