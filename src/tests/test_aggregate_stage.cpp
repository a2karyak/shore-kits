// -*- mode:C++; c-basic-offset:4 -*-

#include "engine/thread.h"
#include "engine/core/stage_container.h"
#include "engine/stages/func_call.h"
#include "engine/stages/aggregate.h"
#include "engine/dispatcher/dispatcher_policy_os.h"
#include "engine/dispatcher/dispatcher_policy_rr_cpu.h"
#include "trace.h"
#include "qpipe_panic.h"

#include "tests/common.h"



using namespace qpipe;



int main(int argc, char* argv[]) {

    thread_init();
    dispatcher_policy_t* dp = new dispatcher_policy_rr_cpu_t();

    // parse output filename
    if ( argc < 2 ) {
	TRACE(TRACE_ALWAYS, "Usage: %s <tuple count>\n", argv[0]);
	exit(-1);
    }
    int num_tuples = atoi(argv[1]);
    if ( num_tuples == 0 ) {
	TRACE(TRACE_ALWAYS, "Invalid tuple count %s\n", argv[1]);
	exit(-1);
    }


    // create a FUNC_CALL stage to feed the AGGREGATE stage
    register_stage<func_call_stage_t>(1);
    register_stage<aggregate_stage_t>(1);


    tuple_buffer_t* int_buffer = new tuple_buffer_t(sizeof(int));
    char* func_call_packet_id;
    int func_call_packet_id_ret =
        asprintf( &func_call_packet_id, "FUNC_CALL_PACKET_1" );
    assert( func_call_packet_id_ret != -1 );



    struct int_tuple_writer_info_s writer_info(int_buffer, num_tuples);
    func_call_packet_t* fc_packet = 
	new func_call_packet_t(func_call_packet_id,
                               int_buffer, 
                               new trivial_filter_t(sizeof(int)), // unused, cannot be NULL
                               increasing_int_tuple_writer_fc,
                               &writer_info);
    
    
    tuple_buffer_t* count_buffer = new tuple_buffer_t(sizeof(int));


    char* aggregate_packet_id;
    int aggregate_packet_id_ret =
        asprintf( &aggregate_packet_id, "AGGREGATE_PACKET_1" );
    assert( aggregate_packet_id_ret != -1 );
 
    aggregate_packet_t* agg_packet =
        new aggregate_packet_t( aggregate_packet_id,
                                count_buffer,
                                new trivial_filter_t(sizeof(int)),
                                new count_aggregate_t(),
                                new default_key_extractor_t(),
                                fc_packet );

    // send packet tree
    dispatcher_policy_t::query_state_t* qs = dp->query_state_create();
    dp->assign_packet_to_cpu(fc_packet, qs);
    dp->assign_packet_to_cpu(agg_packet, qs);
    dispatcher_t::dispatch_packet(agg_packet);
    dp->query_state_destroy(qs);

    
    tuple_t output;
    while(!count_buffer->get_tuple(output))
        TRACE(TRACE_ALWAYS, "Count: %d\n", *(int*)output.data);
    TRACE(TRACE_ALWAYS, "TEST DONE\n");

    return 0;
}
