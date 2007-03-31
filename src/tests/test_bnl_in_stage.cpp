// -*- mode:C++; c-basic-offset:4 -*-

#include "stages.h"
#include "tests/common.h"
#include "workload/tpch/tpch_db.h"
#include "workload/common.h"

#include <vector>
#include <algorithm>

using std::vector;

using namespace qpipe;
using namespace workload;


class test_bnl_in_stage_process_tuple_t : public process_tuple_t {
public:

    virtual void process(const tuple_t& output) {
        TRACE(TRACE_ALWAYS, "Value: %d\n", *aligned_cast<int>(output.data));
    }

    virtual void end() {
        TRACE(TRACE_ALWAYS, "TEST DONE\n");
    }    
};


int main(int argc, char* argv[]) {

    util_init();
    db_open_guard_t db_open;

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
    register_stage<bnl_in_stage_t>(1);


    
    tuple_fifo* left_int_buffer = new tuple_fifo(sizeof(int));
    struct int_tuple_writer_info_s left_writer_info(left_int_buffer, num_tuples, 0);

    func_call_packet_t* left_packet = 
	new func_call_packet_t("LEFT_PACKET",
                               left_int_buffer, 
                               new trivial_filter_t(sizeof(int)), // unused, cannot be NULL
                               shuffled_triangle_int_tuple_writer_fc,
                               &left_writer_info);
    
    tuple_fifo* right_int_buffer = new tuple_fifo(sizeof(int));
    struct int_tuple_writer_info_s right_writer_info(right_int_buffer, num_tuples, num_tuples/2);
    func_call_packet_t* right_packet = 
	new func_call_packet_t("RIGHT_PACKET",
                               right_int_buffer, 
                               new trivial_filter_t(sizeof(int)), // unused, cannot be NULL
                               shuffled_triangle_int_tuple_writer_fc,
                               &right_writer_info);
    
    
    tuple_fifo* output_buffer = new tuple_fifo(sizeof(int));

    bnl_in_packet_t* in_packet =
        new bnl_in_packet_t( "BNL_IN_PACKET_1",
                             output_buffer,
                             new trivial_filter_t(sizeof(int)),
                             left_packet,
                             new workload::tuple_source_once_t(right_packet, sizeof(int)),
                             new int_key_extractor_t(),
                             new int_key_compare_t(),
                             false );

    test_bnl_in_stage_process_tuple_t pt;
    process_query(in_packet, pt);

    return 0;
}
