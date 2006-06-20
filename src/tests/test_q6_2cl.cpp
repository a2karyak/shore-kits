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

    thread_init();
    trace_current_setting = TRACE_ALWAYS;

    if ( !db_open() ) {
        TRACE(TRACE_ALWAYS, "db_open() failed\n");
        QPIPE_PANIC();
    }        


    register_stage<tscan_stage_t>(1);
    register_stage<aggregate_stage_t>(1);
   

    for(int i=0; i < 10; i++) {
        stopwatch_t timer;


        // 1-st CLIENT
        // TSCAN PACKET
        // the output consists of 2 doubles
        tuple_buffer_t* cl_1_tscan_out_buffer = new tuple_buffer_t(2*sizeof(double));
        tuple_filter_t *cl_1_tscan_filter = new q6_tscan_filter_t();


        char* cl_1_tscan_packet_id;
        int cl_1_tscan_packet_id_ret = asprintf(&cl_1_tscan_packet_id, "CL_1_Q6_TSCAN_PACKET");
        assert( cl_1_tscan_packet_id_ret != -1 );
        tscan_packet_t *cl_1_q6_tscan_packet = new tscan_packet_t(cl_1_tscan_packet_id,
                                                                  cl_1_tscan_out_buffer,
                                                                  cl_1_tscan_filter,
                                                                  tpch_lineitem);

        // AGG PACKET CREATION
        // the output consists of 2 int
        tuple_buffer_t* cl_1_agg_output_buffer = new tuple_buffer_t(2*sizeof(double));
        tuple_filter_t* cl_1_agg_filter = new tuple_filter_t(cl_1_agg_output_buffer->tuple_size);
        tuple_aggregate_t* cl_1_q6_aggregator = new q6_count_aggregate_t();
    

        char* cl_1_agg_packet_id;
        int cl_1_agg_packet_id_ret = asprintf(&cl_1_agg_packet_id, "CL_1_Q6_AGGREGATE_PACKET");
        assert( cl_1_agg_packet_id_ret != -1 );
        aggregate_packet_t* cl_1_q6_agg_packet = new aggregate_packet_t(cl_1_agg_packet_id,
                                                                        cl_1_agg_output_buffer,
                                                                        cl_1_agg_filter,
                                                                        cl_1_q6_aggregator,
                                                                        cl_1_q6_tscan_packet);


        // Dispatch 1-st CLIENT packet
        dispatcher_t::dispatch_packet(cl_1_q6_agg_packet);
        

        
        // 2-nd CLIENT
        // TSCAN PACKET
        // the output consists of 2 doubles
        tuple_buffer_t* cl_2_tscan_out_buffer = new tuple_buffer_t(2*sizeof(double));
        tuple_filter_t *cl_2_tscan_filter = new q6_tscan_filter_t();


        char* cl_2_tscan_packet_id;
        int cl_2_tscan_packet_id_ret = asprintf(&cl_2_tscan_packet_id, "CL_2_Q6_TSCAN_PACKET");
        assert( cl_2_tscan_packet_id_ret != -1 );
        tscan_packet_t *cl_2_q6_tscan_packet = new tscan_packet_t(cl_2_tscan_packet_id,
                                                                  cl_2_tscan_out_buffer,
                                                                  cl_2_tscan_filter,
                                                                  tpch_lineitem);

        // AGG PACKET CREATION
        // the output consists of 2 int
        tuple_buffer_t* cl_2_agg_output_buffer = new tuple_buffer_t(2*sizeof(double));
        tuple_filter_t* cl_2_agg_filter = new tuple_filter_t(cl_2_agg_output_buffer->tuple_size);
        count_aggregate_t*  cl_2_q6_aggregator = new count_aggregate_t();
    

        char* cl_2_agg_packet_id;
        int cl_2_agg_packet_id_ret = asprintf(&cl_2_agg_packet_id, "CL_2_Q6_AGGREGATE_PACKET");
        assert( cl_2_agg_packet_id_ret != -1 );
        aggregate_packet_t* cl_2_q6_agg_packet = new aggregate_packet_t(cl_2_agg_packet_id,
                                                                        cl_2_agg_output_buffer,
                                                                        cl_2_agg_filter,
                                                                        cl_2_q6_aggregator,
                                                                        cl_2_q6_tscan_packet);


        // Dispatch 1-st CLIENT packet
        dispatcher_t::dispatch_packet(cl_2_q6_agg_packet);
        
        
        tuple_t output;
        double * r = NULL;

        //cl_1_agg_output_buffer->get_tuple(output);
        //cl_2_agg_output_buffer->get_tuple(output);
        
        while(cl_1_agg_output_buffer->get_tuple(output)) {
            r = (double*)output.data;
            TRACE(TRACE_ALWAYS, "*** CL1: Q6 Count: %lf. Sum: %lf.  ***\n", r[0], r[1]);
        }

        while(cl_2_agg_output_buffer->get_tuple(output)) {
            r = (double*)output.data;
            TRACE(TRACE_ALWAYS, "*** CL2: Q6 Count: %lf. Sum: %lf.  ***\n", r[0], r[1]);
        }
        
                
        
        TRACE(TRACE_ALWAYS, "Query executed in %lf ms\n", timer.time_ms());
    }


    if ( !db_close() )
        TRACE(TRACE_ALWAYS, "db_close() failed\n");
    return 0;
}
