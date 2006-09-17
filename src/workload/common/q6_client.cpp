// -*- mode:C++; c-basic-offset:4 -*-

#include "workload/common/q6_client.h"

#include "stages.h"
#include "workload/common/q6_packet.h"
#include "workload/common/q6_aggregate.h"
#include "workload/common/q6_tscan_filter.h"
#include "workload/tpch/tpch_db.h"


using namespace qpipe;


void* q6_client_main(void* arg) {
    
    
    struct q6_client_info_s* info = (struct q6_client_info_s*)arg;

    
    for (int i = 0; i < info->_num_iterations; i++) {
        
        c_str prefix("Q6_CLIENT%d_%d_", info->_client_id, i);
        packet_t* q6 = create_q6_packet( prefix, info->_policy );
        tuple_fifo* out = q6->output_buffer();
        
        dispatcher_t::dispatch_packet(q6);

        tuple_t output;
        int count;
        if(0) {
            for(count=0; out->get_tuple(output); count++);
            printf("Count: %d\n", count);
        }
        if(1)
        while( out->get_tuple(output) ) {
            double* r = (double*)output.data;
            TRACE(TRACE_QUERY_RESULTS, "*** Q6 Count: %u. Sum: %lf.  ***\n", (unsigned)r[0], r[1]);
        }
        
    } // endof i-loop

    
    return NULL;
}



