// -*- mode:C++; c-basic-offset:4 -*-

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
#include "engine/util/stopwatch.h"
#include "trace.h"
#include "qpipe_panic.h"
#include "tests/common.h"

#include "db_cxx.h"

using namespace qpipe;

extern uint32_t trace_current_setting;





/** @fn    : main
 *  @brief : TPC-H Q6
 */

int main(int argc, char* argv[]) {

    trace_current_setting = TRACE_ALWAYS;
    thread_init();


    
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


    if ( !db_open() ) {
        TRACE(TRACE_ALWAYS, "db_open() failed\n");
        QPIPE_PANIC();
    }        

    

    register_stage<tscan_stage_t>(1);
    register_stage<aggregate_stage_t>(1);
    

    for(int i=0; i < num_iterations; i++) {
        stopwatch_t timer;
        
        
        packet_t* q6_packet = create_q6_packet("Q6");

        tuple_buffer_t* output_buffer = q6_packet->_output_buffer;
        dispatcher_t::dispatch_packet(q6_packet);
    
        tuple_t output;
        while(!output_buffer->get_tuple(output)) {
            double* r = (double*)output.data;
            TRACE(TRACE_ALWAYS, "*** Q6 Count: %lf. Sum: %lf.  ***\n", r[0], r[1]);
        }
        
        TRACE(TRACE_ALWAYS, "Query executed in %.3lf s\n", timer.time());
    }
    
 
    if ( !db_close() )
        TRACE(TRACE_ALWAYS, "db_close() failed\n");
    return 0;
}
