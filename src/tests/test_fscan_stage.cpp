/* -*- mode:C++; c-basic-offset:4 -*- */

#include "stages.h"
#include "workload/tpch/tpch_db.h"
#include "tests/common.h"
#include "workload/common/register_stage.h"

#include <cstdlib>



using namespace qpipe;


int main(int argc, char* argv[]) {

    thread_init();
    db_open();

    // parse input filename
    if ( argc < 2 ) {
	TRACE(TRACE_ALWAYS, "Usage: %s <input file>\n", argv[0]);
	exit(-1);
    }
    const char* input_filename = argv[1];

    
    register_stage<fscan_stage_t>(1);
    
    
    // aggregate single count result (single int)
    tuple_fifo* int_buffer = new tuple_fifo(sizeof(int));
    fscan_packet_t* packet = 
	new fscan_packet_t("FSCAN_PACKET_1",
                           int_buffer,
                           new trivial_filter_t(int_buffer->tuple_size()),
                           input_filename);
    
    
    dispatcher_t::dispatch_packet(packet);
    
  
    tuple_t output;
    while ( int_buffer->get_tuple(output) ) {
	TRACE(TRACE_ALWAYS, "Read %d\n", *aligned_cast<int>(output.data));
    }
    TRACE(TRACE_ALWAYS, "TEST DONE\n");

    
    return 0;
}
