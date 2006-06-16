/* -*- mode:C++; c-basic-offset:4 -*- */

#include "engine/thread.h"
#include "engine/core/stage_container.h"
#include "engine/stages/fdump.h"
#include "trace.h"
#include "qpipe_panic.h"

#include "engine/dispatcher.h"
#include "tests/tester_thread.h"



using namespace qpipe;



int num_fdump_tuples = -1;



/**
 *  @brief Simulates a worker thread on the specified stage.
 *
 *  @param arg A stage_t* to work on.
 */
void* drive_stage(void* arg)
{
  stage_container_t* sc = (stage_container_t*)arg;
  sc->run();

  return NULL;
}



void* write_tuples(void* arg)
{
    tuple_buffer_t* int_buffer = (tuple_buffer_t*)arg;

    for (int i = 0; i < num_fdump_tuples; i++) {
	tuple_t in_tuple((char*)&i, sizeof(int));
	if( int_buffer->put_tuple(in_tuple) ) {
	    TRACE(TRACE_ALWAYS, "tuple_page->append_init() returned non-zero!\n");
	    TRACE(TRACE_ALWAYS, "Terminating loop...\n");
	    break;
	}
    }
    
    if ( !int_buffer->send_eof() ) {
        // Consumer has already terminated this buffer! We are now
        // responsible for deleting it.
        TRACE(TRACE_ALWAYS, "Detected buffer termination\n");
        delete int_buffer;
    }

    return NULL;
}



int main(int argc, char* argv[]) {

    thread_init();
    
    
    // parse output filename
    if ( argc < 3 ) {
	TRACE(TRACE_ALWAYS, "Usage: %s <output file> <tuple count>\n", argv[0]);
	exit(-1);
    }
    const char* output_filename = argv[1];
    num_fdump_tuples = atoi(argv[2]);
    if ( num_fdump_tuples == 0 ) {
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


    tester_thread_t* writer_thread =
	new tester_thread_t(write_tuples, int_buffer, "WRITER_THREAD");
    
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
