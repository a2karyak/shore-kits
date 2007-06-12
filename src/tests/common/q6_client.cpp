// -*- mode:C++; c-basic-offset:4 -*-

#include "tests/common/q6_client.h"

#include "stages.h"

#include "workload/common/process_query.h"

#include "workload/tpch/tpch_db.h"
#include "workload/tpch/q6_packet.h"


using namespace qpipe;
using namespace workload;


void* q6_client_main(void* arg) {
    
    
    struct q6_client_info_s* info = (struct q6_client_info_s*)arg;

    
    for (int i = 0; i < info->_num_iterations; i++) {
        
        c_str prefix("Q6_CLIENT%d_%d_", info->_client_id, i);
        packet_t* q6 = create_q6_packet( prefix, info->_policy );

        /* Store query state since we can't get it after packet
           dispatch. */
        qpipe::query_state_t* qs = q6->get_query_state();

        q6_process_tuple_t pt;
        process_query(q6, pt);
        
        /* done with query state */
        info->_policy->query_state_destroy(qs);
        
    } // endof i-loop

    
    return NULL;
}



