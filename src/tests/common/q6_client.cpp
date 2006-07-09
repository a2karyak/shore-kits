// -*- mode:C++; c-basic-offset:4 -*-

#include "tests/common/q6_client.h"

#include "engine/dispatcher.h"
#include "tests/common/q6_packet.h"
#include "engine/stages/aggregate.h"
#include "engine/stages/tscan.h"
#include "tests/common/q6_aggregate.h"
#include "tests/common/q6_tscan_filter.h"
#include "tests/common/copy_string.h"
#include "tests/common/tpch_db.h"


using namespace qpipe;
using std::string;


void* q6_client_main(void* arg) {
    
    
    struct q6_client_info_s* info = (struct q6_client_info_s*)arg;

    
    for (int i = 0; i < info->_num_iterations; i++) {

        char index_str[16];
        sprintf(index_str, "%d_%d_", info->_client_id, i);
    
        packet_t* q6 = create_q6_packet( (string("Q6_CLIENT") + index_str).c_str(), info->_policy );
        tuple_buffer_t* out = q6->_output_buffer;
        
        dispatcher_t::dispatch_packet(q6);

        tuple_t output;
        while( !out->get_tuple(output) ) {
            double* r = (double*)output.data;
            TRACE(TRACE_ALWAYS, "*** Q6 Count: %u. Sum: %lf.  ***\n", (unsigned)r[0], r[1]);
        }
        
    } // endof i-loop

    
    return NULL;
}



