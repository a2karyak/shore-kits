// -*- mode:C++; c-basic-offset:4 -*-

#include "engine/thread.h"
#include "engine/core/stage_container.h"
#include "tests/tester_thread.h"
#include "engine/stages/func_call.h"
#include "engine/stages/aggregate.h"
#include "trace.h"
#include "qpipe_panic.h"



using namespace qpipe;



int num_tuples = -1;



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



class count_aggregate_t : public tuple_aggregate_t {

private:
    int count;
    
public:
  
    count_aggregate_t() {
        count = 0;
    }
  
    bool aggregate(tuple_t &, const tuple_t &) {
        count++;
        return false;
    }

    bool eof(tuple_t &dest) {
        *(int*)dest.data = count;
        return true;
    }
};



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


    // create a FUNC_CALL stage to feed the AGGREGATE stage
    stage_container_t* sc = new stage_container_t("FUNC_CALL_CONTAINER", new stage_factory<func_call_stage_t>);
    dispatcher_t::register_stage_container(func_call_packet_t::PACKET_TYPE, sc);
    
    tester_thread_t* func_call_thread = 
	new tester_thread_t(drive_stage, sc, "FUNC_CALL_THREAD");
    
    if ( thread_create( NULL, func_call_thread ) ) {
	TRACE(TRACE_ALWAYS, "thread_create() failed\n");
	QPIPE_PANIC();
    }



    stage_container_t* sc2 = new stage_container_t("AGGREGATE_CONTAINER", new stage_factory<aggregate_stage_t>);
    dispatcher_t::register_stage_container(aggregate_packet_t::PACKET_TYPE, sc2);
    tester_thread_t* aggregate_thread = new tester_thread_t(drive_stage, sc2, "AGGREGATE THREAD");
    if ( thread_create( NULL, aggregate_thread ) ) {
        TRACE(TRACE_ALWAYS, "thread_create failed\n");
        QPIPE_PANIC();
    }


    tuple_buffer_t* int_buffer = new tuple_buffer_t(sizeof(int));
    char* func_call_packet_id;
    int func_call_packet_id_ret =
        asprintf( &func_call_packet_id, "FUNC_CALL_PACKET_1" );
    assert( func_call_packet_id_ret != -1 );


    func_call_packet_t* fc_packet = 
	new func_call_packet_t(func_call_packet_id,
                               int_buffer, 
                               new tuple_filter_t(sizeof(int)), // unused, cannot be NULL
                               write_ints,
                               int_buffer);
    
    
    tuple_buffer_t* count_buffer = new tuple_buffer_t(sizeof(int));


    char* aggregate_packet_id;
    int aggregate_packet_id_ret =
        asprintf( &aggregate_packet_id, "AGGREGATE_PACKET_1" );
    assert( aggregate_packet_id_ret != -1 );
 
    aggregate_packet_t* agg_packet =
        new aggregate_packet_t( aggregate_packet_id,
                                count_buffer,
                                new tuple_filter_t(sizeof(int)),
                                new count_aggregate_t(),
                                fc_packet );
    dispatcher_t::dispatch_packet(agg_packet);
    
    tuple_t output;
    while(!count_buffer->get_tuple(output))
        TRACE(TRACE_ALWAYS, "Count: %d\n", *(int*)output.data);
    TRACE(TRACE_ALWAYS, "TEST DONE\n");

    return 0;
}
