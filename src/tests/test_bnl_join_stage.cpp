// -*- mode:C++; c-basic-offset:4 -*-

#include "engine/thread.h"
#include "engine/core/stage_container.h"
#include "engine/stages/func_call.h"
#include "engine/stages/bnl_join.h"
#include "trace.h"
#include "qpipe_panic.h"

#include "tests/common.h"

#include <vector>
#include <algorithm>

using std::vector;

using namespace qpipe;



void destroy_writer_info(void* arg) {
    struct int_tuple_writer_info_s* info =
        (struct int_tuple_writer_info_s*)arg;
    // No inputs to destroy. The container takes care of our
    // int_buffer field.
    delete info;
}



class write_ints_tuple_source_t : public tuple_source_t {

private:

    int _num_tuples;

public:

    write_ints_tuple_source_t(int num_tuples)
        : _num_tuples(num_tuples)
    {
    }
    
    packet_t* reset() {
        tuple_buffer_t* right_int_buffer = new tuple_buffer_t(sizeof(int));
        char* right_packet_id = copy_string("RIGHT_PACKET");
        struct int_tuple_writer_info_s* info =
            new int_tuple_writer_info_s(right_int_buffer, _num_tuples);
        return
            new func_call_packet_t(right_packet_id,
                                   right_int_buffer,
                                   new tuple_filter_t(sizeof(int)),
                                   shuffled_triangle_int_tuple_writer_fc,
                                   info,
                                   destroy_writer_info);
    }
};



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
    register_stage<bnl_join_stage_t>(1);



    single_int_join_t *join = new single_int_join_t();
    tuple_buffer_t* left_int_buffer = new tuple_buffer_t(join->left_tuple_size());
    char* left_packet_id = copy_string("LEFT_PACKET");
    struct int_tuple_writer_info_s left_writer_info(left_int_buffer, num_tuples);

    func_call_packet_t* left_packet = 
	new func_call_packet_t(left_packet_id,
                               left_int_buffer, 
                               new tuple_filter_t(sizeof(int)), // unused, cannot be NULL
                               shuffled_triangle_int_tuple_writer_fc,
                               &left_writer_info);
    
    tuple_buffer_t* join_buffer = new tuple_buffer_t(join->out_tuple_size());

    char* join_packet_id = copy_string("BNL_JOIN_PACKET_1");
    bnl_join_packet_t* join_packet =
        new bnl_join_packet_t( join_packet_id,
                               join_buffer,
                               new tuple_filter_t(sizeof(int)),
                               left_packet,
                               new write_ints_tuple_source_t(num_tuples),
                               //new tuple_source_once_t(right_packet),
                               join);
    dispatcher_t::dispatch_packet(join_packet);
    
    
    tuple_t output;
    while(!join_buffer->get_tuple(output))
        TRACE(TRACE_ALWAYS, "Value: %d\n", *(int*)output.data);
    TRACE(TRACE_ALWAYS, "TEST DONE\n");
    
    return 0;
}
