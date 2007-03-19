/* -*- mode:C++; c-basic-offset:4 -*- */

#include "core.h"
#include "stages.h"
#include "workload/tpch/tpch_db.h"
#include "workload/common.h"
#include "tests/common.h"

using namespace qpipe;
using namespace workload;


class test_func_call_stage_process_tuple_t : public process_tuple_t {
public:

    virtual void process(const tuple_t& output) {
	TRACE(TRACE_ALWAYS, "Read %d\n", *aligned_cast<int>(output.data));
    }

    virtual void end() {
        TRACE(TRACE_ALWAYS, "TEST DONE\n");
    }

};


int main(int argc, char* argv[]) {

    util_init();
    

    int num_tuples = -1;

    
    // parse number of tuples
    if ( argc < 2 ) {
	TRACE(TRACE_ALWAYS, "Usage: %s <tuple count>\n", argv[0]);
	exit(-1);
    }
    num_tuples = atoi(argv[1]);
    if ( num_tuples == 0 ) {
	TRACE(TRACE_ALWAYS, "Invalid tuple count %s\n", argv[1]);
	exit(-1);
    }


    register_stage<func_call_stage_t>(1);
    
    
    // just need to pass one int at a time to the counter
    tuple_fifo* int_buffer = new tuple_fifo(sizeof(int));
    
    // aggregate single count result (single int)
    int_tuple_writer_info_s info(int_buffer, num_tuples, 0);
    
    func_call_packet_t* packet = 
	new func_call_packet_t("FUNC_CALL_PACKET_1",
                               int_buffer, 
                               new trivial_filter_t(int_buffer->tuple_size()),
                               int_tuple_writer_fc,
                               &info);
    
    test_func_call_stage_process_tuple_t pt;
    process_query(packet, pt);
    
    return 0;
}
