// -*- mode:C++; c-basic-offset:4 -*-

#include "engine/thread.h"
#include "engine/core/stage_container.h"
#include "engine/stages/func_call.h"
#include "engine/stages/hash_join.h"
#include "trace.h"
#include "qpipe_panic.h"

#include "workload/common.h"
#include "tests/common.h"

#include <vector>
#include <algorithm>

using std::vector;

using namespace qpipe;




int main(int argc, char* argv[]) {

    thread_init();


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


    register_stage<func_call_stage_t>(2);
    register_stage<hash_join_stage_t>(1);



    single_int_join_t *join = new single_int_join_t();
    tuple_buffer_t* left_int_buffer = new tuple_buffer_t(join->left_tuple_size());
    struct int_tuple_writer_info_s left_writer_info(left_int_buffer, num_tuples);

    func_call_packet_t* left_packet = 
	new func_call_packet_t("LEFT_PACKET",
                               left_int_buffer, 
                               new trivial_filter_t(sizeof(int)), // unused, cannot be NULL
                               shuffled_triangle_int_tuple_writer_fc,
                               &left_writer_info);
    
    tuple_buffer_t* right_int_buffer = new tuple_buffer_t(join->right_tuple_size());
    struct int_tuple_writer_info_s right_writer_info(right_int_buffer, num_tuples);
    func_call_packet_t* right_packet = 
	new func_call_packet_t("RIGHT_PACKET",
                               right_int_buffer, 
                               new trivial_filter_t(sizeof(int)), // unused, cannot be NULL
                               shuffled_triangle_int_tuple_writer_fc,
                               &right_writer_info);
    
    
    tuple_buffer_t* join_buffer = new tuple_buffer_t(join->out_tuple_size());

    
    hash_join_packet_t* join_packet =
        new hash_join_packet_t( "HASH_JOIN_PACKET_1",
                                join_buffer,
                                new trivial_filter_t(sizeof(int)),
                                left_packet,
                                right_packet,
                                join
                                );
    dispatcher_t::dispatch_packet(join_packet);
    
    
    tuple_t output;
    while(!join_buffer->get_tuple(output))
        TRACE(TRACE_ALWAYS, "Value: %d\n", *(int*)output.data);
    TRACE(TRACE_ALWAYS, "TEST DONE\n");

    return 0;
}
