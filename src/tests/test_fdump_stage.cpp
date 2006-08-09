/* -*- mode:C++; c-basic-offset:4 -*- */

#include "engine/thread.h"
#include "engine/core/stage_container.h"
#include "engine/stages/fdump.h"
#include "engine/dispatcher.h"
#include "trace.h"
#include "qpipe_panic.h"
#include "workload/tpch/tpch_db.h"
#include "tests/common.h"



using namespace qpipe;



int main(int argc, char* argv[]) {

    thread_init();
    db_open();
    
    
    // parse output filename
    if ( argc < 3 ) {
	TRACE(TRACE_ALWAYS, "Usage: %s <output file> <tuple count>\n", argv[0]);
	exit(-1);
    }
    const char* output_filename = argv[1];
    int num_tuples = atoi(argv[2]);
    if ( num_tuples == 0 ) {
	TRACE(TRACE_ALWAYS, "Invalid tuple count %s\n", argv[2]);
	exit(-1);
    }


    register_stage<fdump_stage_t>(1);
    
    
    // just need to pass one int at a time to the counter
    tuple_fifo* int_buffer = new tuple_fifo(sizeof(int), dbenv);
    tuple_fifo* signal_buffer = new tuple_fifo(sizeof(int), dbenv);


    struct int_tuple_writer_info_s info(int_buffer, num_tuples);
    tester_thread_t* writer_thread =
	new tester_thread_t(int_tuple_writer_main, &info, "WRITER_THREAD");
    
    if ( thread_create( NULL, writer_thread ) ) {
	TRACE(TRACE_ALWAYS, "thread_create() failed\n");
	QPIPE_PANIC();
    }
    
    // aggregate single count result (single int)
    fdump_packet_t* packet = 
	new fdump_packet_t("FDUMP_PACKET_1",
			   signal_buffer, 
                           new trivial_filter_t(int_buffer->tuple_size()),
			   int_buffer, 
			   output_filename);
    
    
    dispatcher_t::dispatch_packet(packet);
  
  
    tuple_t output;
    TRACE(TRACE_ALWAYS, "get_tuple() returned %d\n",
	  signal_buffer->get_tuple(output) );
    TRACE(TRACE_ALWAYS, "TEST DONE\n");
    exit(0);
    
    return 0;
}
