/* -*- mode:C++; c-basic-offset:4 -*- */

#include "core/tuple_fifo.h"
#include "core/dispatcher.h"
#include "stages/common/process_query.h"
#include "util.h"


ENTER_NAMESPACE(qpipe);


void process_query(packet_t* root, process_tuple_t& pt)
{
    guard<tuple_fifo> out = root->output_buffer();
    
    dispatcher_t::worker_reserver_t* wr = dispatcher_t::reserver_acquire();

    /* reserve worker threads and dispatch... */
    root->declare_worker_needs(wr);
    wr->acquire_resources();
    dispatcher_t::dispatch_packet(root);
    
    /* process query results */
    tuple_t output;
    pt.begin();
    while(out->get_tuple(output))
        pt.process(output);
    pt.end();

    dispatcher_t::reserver_release(wr);
}


EXIT_NAMESPACE(qpipe);
