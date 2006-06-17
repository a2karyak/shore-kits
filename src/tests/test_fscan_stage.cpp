/* -*- mode:C++; c-basic-offset:4 -*- */

#include "engine/thread.h"
#include "engine/core/stage_container.h"
#include "engine/stages/fscan.h"
#include "trace.h"
#include "qpipe_panic.h"

#include "tests/common/tester_thread.h"

#include <stdlib.h>



using namespace qpipe;



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



int main(int argc, char* argv[]) {

    thread_init();

    // parse input filename
    if ( argc < 2 ) {
	TRACE(TRACE_ALWAYS, "Usage: %s <input file>\n", argv[0]);
	exit(-1);
    }
    const char* input_filename = argv[1];

    
    stage_container_t* sc = new stage_container_t("FSCAN_CONTAINER", new stage_factory<fscan_stage_t>);
    dispatcher_t::register_stage_container(fscan_packet_t::PACKET_TYPE, sc);
    
    tester_thread_t* fscan_thread =
	new tester_thread_t(drive_stage, sc, "FSCAN_THREAD");
    
    if ( thread_create( NULL, fscan_thread ) ) {
	TRACE(TRACE_ALWAYS, "thread_create() failed\n");
	QPIPE_PANIC();
    }
    
    
    
    // aggregate single count result (single int)
    tuple_buffer_t* int_buffer = new tuple_buffer_t(sizeof(int));

    char* fscan_packet_id;
    int fscan_packet_id_ret =
        asprintf( &fscan_packet_id, "FSCAN_PACKET_1" );
    assert( fscan_packet_id_ret != -1 );

    char* fscan_packet_filename;
    int fscan_packet_filename_ret =
        asprintf( &fscan_packet_filename, "%s", input_filename );
    assert( fscan_packet_filename_ret != -1 );
    
    fscan_packet_t* packet = 
	new fscan_packet_t(fscan_packet_id,
                           int_buffer,
                           new tuple_filter_t(int_buffer->tuple_size),
                           fscan_packet_filename);
    
    
    dispatcher_t::dispatch_packet(packet);
    
  
    tuple_t output;
    while ( !int_buffer->get_tuple(output) ) {
	TRACE(TRACE_ALWAYS, "Read %d\n", *(int*)output.data);
    }
    TRACE(TRACE_ALWAYS, "TEST DONE\n");

    
    return 0;
}
