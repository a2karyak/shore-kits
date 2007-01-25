// -*- mode:C++; c-basic-offset:4 -*-

#include "stages.h"
#include "scheduler.h"
#include "workload/tpch/tpch_db.h"
#include "workload/common.h"
#include "tests/common.h"
#include "workload/common/register_stage.h"



using namespace qpipe;



int main(int argc, char* argv[]) {

    thread_init();
    db_open();
    scheduler::policy_t* dp = new scheduler::policy_rr_cpu_t();

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


    tuple_fifo* int_buffer = new tuple_fifo(sizeof(int));

    struct int_tuple_writer_info_s writer_info(int_buffer, num_tuples);
    func_call_packet_t* fc_packet = 
	new func_call_packet_t("FUNC_CALL_PACKET_1",
                               int_buffer, 
                               new trivial_filter_t(sizeof(int)), // unused, cannot be NULL
                               increasing_int_tuple_writer_fc,
                               &writer_info);
    
    
    tuple_fifo* count_buffer = new tuple_fifo(sizeof(int));


    aggregate_packet_t* agg_packet =
        new aggregate_packet_t(  "AGGREGATE_PACKET_1",
                                count_buffer,
                                new trivial_filter_t(sizeof(int)),
                                new count_aggregate_t(),
                                new default_key_extractor_t(),
                                fc_packet );

    // send packet tree
    qpipe::query_state_t* qs = dp->query_state_create();
    fc_packet->assign_query_state(qs);
    agg_packet->assign_query_state(qs);


    dispatcher_t::dispatch_packet(agg_packet);

 
    tuple_t output;
    while(count_buffer->get_tuple(output))
        TRACE(TRACE_ALWAYS, "Count: %d\n", *safe_cast<int>(output.data));

    dp->query_state_destroy(qs);
    TRACE(TRACE_ALWAYS, "TEST DONE\n");

    return 0;
}
