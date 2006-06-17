/* -*- mode:C++; c-basic-offset:4 -*- */

#include "engine/thread.h"
#include "engine/core/stage_container.h"
#include "engine/stages/func_call.h"
#include "engine/dispatcher.h"
#include "trace.h"
#include "qpipe_panic.h"

#include "tests/common.h"



using namespace qpipe;



int num_tuples = -1;



stage_t::result_t write_ints(void* arg)
{
    tuple_buffer_t* int_buffer = (tuple_buffer_t*)arg;    
    for (int i = 0; i < num_tuples; i++) {
	tuple_t in_tuple((char*)&i, sizeof(int));
	if( int_buffer->put_tuple(in_tuple) ) {
	    TRACE(TRACE_ALWAYS, "tuple_page->append_init() returned non-zero!\n");
	    TRACE(TRACE_ALWAYS, "Terminating loop...\n");
	    break;
	}
    }
    
    return stage_t::RESULT_STOP;
}



int main(int argc, char* argv[]) {

    thread_init();
    
    
    // parse output filename
    if ( argc < 2 ) {
	TRACE(TRACE_ALWAYS, "Usage: %s <tuple count>\n", argv[0]);
	exit(-1);
    }
    num_tuples = atoi(argv[1]);
    if ( num_tuples == 0 ) {
	TRACE(TRACE_ALWAYS, "Invalid tuple count %s\n", argv[1]);
	exit(-1);
    }


    stage_container_t* sc = new stage_container_t("FUNC_CALL_CONTAINER", new stage_factory<func_call_stage_t>);
    dispatcher_t::register_stage_container(func_call_packet_t::PACKET_TYPE, sc);
    
    tester_thread_t* func_call_thread = 
	new tester_thread_t(drive_stage, sc, "FUNC_CALL_THREAD");
    
    if ( thread_create( NULL, func_call_thread ) ) {
	TRACE(TRACE_ALWAYS, "thread_create() failed\n");
	QPIPE_PANIC();
    }
    
    
    // just need to pass one int at a time to the counter
    tuple_buffer_t* int_buffer = new tuple_buffer_t(sizeof(int));
    
    // aggregate single count result (single int)
    char* func_call_packet_id;
    int func_call_packet_id_ret =
        asprintf( &func_call_packet_id, "FUNC_CALL_PACKET_1" );
    assert( func_call_packet_id_ret != -1 );
    
    func_call_packet_t* packet = 
	new func_call_packet_t(func_call_packet_id,
                               int_buffer, 
                               new tuple_filter_t(int_buffer->tuple_size),
                               write_ints,
                               int_buffer);
    
    
    dispatcher_t::dispatch_packet(packet);
  
  
    tuple_t output;
    while ( !int_buffer->get_tuple(output) ) {
	TRACE(TRACE_ALWAYS, "Read %d\n", *(int*)output.data);
    }
    TRACE(TRACE_ALWAYS, "TEST DONE\n");
    
    
    return 0;
}
