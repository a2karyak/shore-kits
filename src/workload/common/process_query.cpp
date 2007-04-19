/* -*- mode:C++; c-basic-offset:4 -*- */

#include "core/tuple_fifo.h"
#include "core/dispatcher.h"
#include "workload/common/process_query.h"
#include "util.h"

using namespace qpipe;


ENTER_NAMESPACE(workload);



bool RESERVE_WORKERS_BEFORE_DISPATCH = true;



void process_query(packet_t* root, process_tuple_t& pt)
{
    guard<tuple_fifo> out = root->output_buffer();
    
    dispatcher_t::worker_reserver_t* wr;
    if (RESERVE_WORKERS_BEFORE_DISPATCH) {
        /* reserve worker threads */
        wr = dispatcher_t::reserver_acquire();
        root->declare_worker_needs(wr);
        wr->acquire_resources();
    }

    /* dispatch... */
    dispatcher_t::dispatch_packet(root);
    
    /* process query results */
    tuple_t output;
    pt.begin();
    while(out->get_tuple(output))
        pt.process(output);
    pt.end();

    if (RESERVE_WORKERS_BEFORE_DISPATCH) {
        dispatcher_t::reserver_release(wr);
    }
}



EXIT_NAMESPACE(workload);
