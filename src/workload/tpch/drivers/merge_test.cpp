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

    static const int APPROX_TUPLES_PER_PAGE = 1000;
    
    int count = 0;
    for (int i = 0; i < 10; i++) {
        for (int t = 0; t < APPROX_TUPLES_PER_PAGE; t++) {
            tuple_t tup((char*)&count, sizeof(count));
            adaptor->output(tup);
            count++;
        }
        sleep_interval();
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
        

    /* sleep before dispatch so we get merged in later */
    thread_t* self = thread_get_self();
    sleep(self->rand(20));
    
    
    // dispatch root
    reserve_query_workers(packet);
    dispatcher_t::dispatch_packet(packet);
    

    tuple_t output;
    int*    v = NULL;
    int     old_v = -1;
    bool    first = true;
    

    /* Only print boundary values (first, end, and rollover from late
       merging). */
    TRACE(TRACE_ALWAYS, "*** ANSWER ...\n");
    while(out_buffer->get_tuple(output)) {

        if (!first)
            old_v = *v;

        v = aligned_cast<int>(output.data);

        if (first)
            TRACE(TRACE_ALWAYS, "*** first! v: %d\n", *v);

        if (!first && (old_v != ((*v) - 1)))
            /* non-consecutive values! */
            TRACE(TRACE_ALWAYS, "*** old_v: %d, v: %d\n", old_v, *v);
        
        first = false;
    }

    TRACE(TRACE_ALWAYS, "*** END old_v: %d, v: %d\n", old_v, *v);

    dp->query_state_destroy(qs);
}


EXIT_NAMESPACE(workload);
