/* -*- mode:C++; c-basic-offset:4 -*- */
#include "stages.h"
#include "scheduler.h"

#include "workload/tpch/drivers/merge_test.h"



ENTER_NAMESPACE(merge_test);

void sleep_interval() {
    sleep(1);
}

void merge_test_fc(void* sadaptor, void*) {

    TRACE(TRACE_ALWAYS, "Starting...\n");

    stage_t::adaptor_t* adaptor = (stage_t::adaptor_t*)sadaptor;
    
    for (int i = 0; i < 10; i++) {
        sleep_interval();
	tuple_t tup((char*)&i, sizeof(int));
	adaptor->output(tup);
    }
}

EXIT_NAMESPACE(merge_test);



using namespace merge_test;

ENTER_NAMESPACE(workload);


void merge_test_driver::submit(void* disp) {

    scheduler::policy_t* dp = (scheduler::policy_t*)disp;

    // FUNC_CALL PACKET with merging allowed
    tuple_fifo* out_buffer = new tuple_fifo(sizeof(int));
    func_call_packet_t* packet =
        new func_call_packet_t("FUNC_CALL_PACKET",
                               out_buffer,
                               new trivial_filter_t(out_buffer->tuple_size()),
                               merge_test_fc,
                               NULL,
                               NULL,
                               true);

    qpipe::query_state_t* qs = dp->query_state_create();
    packet->assign_query_state(qs);
        
    // Dispatch packet
    dispatcher_t::dispatch_packet(packet);
    
    tuple_t output;
    TRACE(TRACE_ALWAYS, "*** ANSWER ...\n");
    while(out_buffer->get_tuple(output)) {
        int* v;
        v = aligned_cast<int>(output.data);
        TRACE(TRACE_ALWAYS, "*** %d\n", *v);
    }

    dp->query_state_destroy(qs);
}

EXIT_NAMESPACE(workload);
