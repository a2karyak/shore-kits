/* -*- mode:C++; c-basic-offset:4 -*- */

#include "engine/thread.h"
#include "engine/core/stage_container.h"
#include "engine/stages/fdump.h"
#include "engine/dispatcher.h"
#include "trace.h"
#include "qpipe_panic.h"

#include "tests/common.h"



using namespace qpipe;



int main(int argc, char* argv[]) {

    thread_init();
    
    
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


    stage_container_t* sc = new stage_container_t("FDUMP_CONTAINER", new stage_factory<fdump_stage_t>);
    dispatcher_t::register_stage_container(fdump_packet_t::PACKET_TYPE, sc);

    tester_thread_t* fdump_thread = 
	new tester_thread_t(drive_stage, sc, "FDUMP_THREAD");
    
    if ( thread_create( NULL, fdump_thread ) ) {
	TRACE(TRACE_ALWAYS, "thread_create() failed\n");
	QPIPE_PANIC();
    }
    
    
    
    // just need to pass one int at a time to the counter
    tuple_buffer_t* int_buffer = new tuple_buffer_t(sizeof(int));
    tuple_buffer_t* signal_buffer = new tuple_buffer_t(sizeof(int));


    struct int_tuple_writer_info_s info(int_buffer, num_tuples);
    tester_thread_t* writer_thread =
	new tester_thread_t(int_tuple_writer_main, &info, "WRITER_THREAD");
    
    if ( thread_create( NULL, writer_thread ) ) {
	TRACE(TRACE_ALWAYS, "thread_create() failed\n");
	QPIPE_PANIC();
    }
    
    // aggregate single count result (single int)
    char* fdump_packet_id;
    int fdump_packet_id_ret =
        asprintf( &fdump_packet_id, "FDUMP_PACKET_1" );
    assert( fdump_packet_id_ret != -1 );
    
    char* fdump_packet_filename;
    int fdump_packet_filename_ret =
        asprintf( &fdump_packet_filename, "%s", output_filename );
    assert( fdump_packet_filename_ret != -1 );

    fdump_packet_t* packet = 
	new fdump_packet_t(fdump_packet_id,
			   signal_buffer, 
                           new tuple_filter_t(int_buffer->tuple_size),
			   int_buffer, 
			   fdump_packet_filename);
    
    
    dispatcher_t::dispatch_packet(packet);
  
  
    tuple_t output;
    TRACE(TRACE_ALWAYS, "get_tuple() returned %d\n",
	  signal_buffer->get_tuple(output) );
    TRACE(TRACE_ALWAYS, "TEST DONE\n");
    exit(0);
    
    return 0;
}
