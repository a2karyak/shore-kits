// -*- mode:C++; c-basic-offset:4 -*-

/** @file    : test_q6.cpp
 *  @brief   : Unittest for Q6
 *  @version : 0.1
 *  @history :
 6/8/2006 : Updated to work with the new class definitions
 5/25/2006: Initial version
*/ 

#include "engine/thread.h"
#include "engine/core/stage_container.h"
#include "engine/stages/tscan.h"
#include "engine/stages/aggregate.h"
#include "engine/dispatcher.h"
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

int main() {
    trace_current_setting = TRACE_ALWAYS;
    thread_init();

    if ( !db_open() ) {
        TRACE(TRACE_ALWAYS, "db_open() failed\n");
        QPIPE_PANIC();
    }        


    register_stage<tscan_stage_t>(1);
    register_stage<aggregate_stage_t>(1);
    

    for(int i=0; i < 10; i++) {
        stopwatch_t timer;
        
        // TSCAN PACKET
        // the output consists of 2 doubles
        tuple_buffer_t* tscan_out_buffer = new tuple_buffer_t(2*sizeof(double));
        tuple_filter_t* tscan_filter = new q6_tscan_filter_t();


        char* tscan_packet_id;
        int tscan_packet_id_ret = asprintf(&tscan_packet_id, "Q6_TSCAN_PACKET");
        assert( tscan_packet_id_ret != -1 );
        tscan_packet_t *q6_tscan_packet = new tscan_packet_t(tscan_packet_id,
                                                             tscan_out_buffer,
                                                             tscan_filter,
                                                             tpch_lineitem);

        // AGG PACKET CREATION
        // the output consists of 2 int
        tuple_buffer_t* agg_output_buffer = new tuple_buffer_t(2*sizeof(double));
        tuple_filter_t* agg_filter = new tuple_filter_t(agg_output_buffer->tuple_size);
        tuple_aggregate_t*  q6_aggregator = new q6_count_aggregate_t();
    

        char* agg_packet_id;
        int agg_packet_id_ret = asprintf(&agg_packet_id, "Q6_AGGREGATE_PACKET");
        assert( agg_packet_id_ret != -1 );
        aggregate_packet_t* q6_agg_packet = new aggregate_packet_t(agg_packet_id,
                                                                   agg_output_buffer,
                                                                   agg_filter,
                                                                   q6_aggregator,
                                                                   q6_tscan_packet);


        // Dispatch packet
        dispatcher_t::dispatch_packet(q6_agg_packet);
    
        tuple_t output;
        double * r = NULL;
        while(!agg_output_buffer->get_tuple(output)) {
            r = (double*)output.data;
            TRACE(TRACE_ALWAYS, "*** Q6 Count: %lf. Sum: %lf.  ***\n", r[0], r[1]);
        }
        
        printf("Query executed in %lf ms\n", timer.time_ms());
    }


    if ( !db_close() )
        TRACE(TRACE_ALWAYS, "db_close() failed\n");
    return 0;
}
