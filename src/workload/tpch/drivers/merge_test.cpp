/* -*- mode:C++; c-basic-offset:4 -*- */
#include "stages.h"
#include "scheduler.h"

#include "workload/tpch/drivers/merge_test.h"
#include "workload/process_query.h"

using namespace qpipe;
using namespace workload;


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



class merge_test_process_tuple_t : public process_tuple_t {

private:
    int*    v;
    int     old_v;
    bool    first;

public:
    merge_test_process_tuple_t()
        : v(NULL)
        , old_v(-1)
        , first(true)
    {
    }
        
    virtual void begin() {
        TRACE(TRACE_ALWAYS, "*** ANSWER ...\n");
    }
    
    virtual void process(const tuple_t& output) {

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

    virtual void end() {
        TRACE(TRACE_ALWAYS, "*** END old_v: %d, v: %d\n", old_v, *v);
    }
};



void merge_test_driver::submit(void* disp) {

    scheduler::policy_t* dp = (scheduler::policy_t*)disp;

    // FUNC_CALL PACKET with merging allowed
    tuple_fifo* fifo = new tuple_fifo(sizeof(int));
    func_call_packet_t* packet =
        new func_call_packet_t("FUNC_CALL_PACKET",
                               fifo,
                               new trivial_filter_t(fifo->tuple_size()),
                               merge_test_fc,
                               NULL,
                               NULL,
                               true);

    qpipe::query_state_t* qs = dp->query_state_create();
    packet->assign_query_state(qs);
        

    /* sleep before dispatch so we get merged in later */
    thread_t* self = thread_get_self();
    sleep(self->rand(20));
    

    merge_test_process_tuple_t pt;
    process_query(packet, pt);


    dp->query_state_destroy(qs);
}


EXIT_NAMESPACE(workload);
