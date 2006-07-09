// -*- mode:C++; c-basic-offset:4 -*-

#include "engine/thread.h"
#include "engine/stages/sort.h"
#include "engine/stages/fscan.h"
#include "engine/stages/fdump.h"
#include "engine/stages/merge.h"
#include "engine/stages/func_call.h"
#include "engine/core/stage_container.h"
#include "trace.h"
#include "qpipe_panic.h"

#include "workload/common.h"
#include "tests/common.h"

#include <vector>
#include <algorithm>

using std::vector;

using namespace qpipe;


int num_values;
int num_copies;



stage_t::result_t write_ints(void* arg)
{
    tuple_buffer_t *buffer = (tuple_buffer_t *)arg;

    // produce a set of inputs, with duplicated values
    vector<int> inputs;
    inputs.reserve(num_values * num_copies);
    for(int i=0; i < num_values; i++)
        for(int j=0; j < num_copies; j++)
            inputs.push_back(i);

    // shuffle the input
    std::random_shuffle(inputs.begin(), inputs.end());

    int value;
    tuple_t input((char *)&value, sizeof(int));
    for(unsigned  i=0; i < inputs.size(); i++) {
        value = inputs[i];
        if(buffer->put_tuple(input))
        {
            TRACE(TRACE_ALWAYS, "buffer->put_tuple() returned non-zero!\n");
            TRACE(TRACE_ALWAYS, "Terminating loop...\n");
            break;
        }
    }

    TRACE(TRACE_ALWAYS, "Finished inserting tuples\n", value);
    return stage_t::RESULT_STOP;
}



int main(int argc, char* argv[]) {

    thread_init();
    int THREAD_POOL_SIZE = 20;

    bool do_echo = true;
    

    // parse command line args
    if ( argc < 3 ) {
	TRACE(TRACE_ALWAYS, "Usage: %s <num values> <num copies> [off]\n", argv[0]);
	exit(-1);
    }
    num_values = atoi(argv[1]);
    if ( num_values == 0 ) {
	TRACE(TRACE_ALWAYS, "Invalid num values %s\n", argv[1]);
	exit(-1);
    }
    num_copies = atoi(argv[2]);
    if ( num_copies == 0 ) {
	TRACE(TRACE_ALWAYS, "Invalid num copies %s\n", argv[2]);
	exit(-1);
    }
    if ( (argc > 3) && !strcmp(argv[3], "off") )
        do_echo = false;



    stage_container_t* sc;

    sc = new stage_container_t("FDUMP_CONTAINER", new stage_factory<fdump_stage_t>);
    dispatcher_t::register_stage_container(fdump_packet_t::PACKET_TYPE, sc);
    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
	tester_thread_t* fdump_thread = new tester_thread_t(drive_stage, sc, "FDUMP THREAD");
	if ( thread_create( NULL, fdump_thread ) ) {
	    TRACE(TRACE_ALWAYS, "thread_create failed\n");
	    QPIPE_PANIC();
	}
    }
    
    sc = new stage_container_t("FSCAN_CONTAINER", new stage_factory<fscan_stage_t>);
    dispatcher_t::register_stage_container(fscan_packet_t::PACKET_TYPE, sc);
    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
	tester_thread_t* fscan_thread = new tester_thread_t(drive_stage, sc, "FSCAN THREAD");
	if ( thread_create( NULL, fscan_thread ) ) {
	    TRACE(TRACE_ALWAYS, "thread_create failed\n");
	    QPIPE_PANIC();
	}
    }
    
    sc = new stage_container_t("MERGE_CONTAINER", new stage_factory<merge_stage_t>);
    dispatcher_t::register_stage_container(merge_packet_t::PACKET_TYPE, sc);
    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
	tester_thread_t* merge_thread = new tester_thread_t(drive_stage, sc, "MERGE THREAD");
	if ( thread_create( NULL, merge_thread ) ) {
	    TRACE(TRACE_ALWAYS, "thread_create failed\n");
	    QPIPE_PANIC();
	}
    }

    sc = new stage_container_t("SORT_CONTAINER", new stage_factory<sort_stage_t>);
    dispatcher_t::register_stage_container(sort_packet_t::PACKET_TYPE, sc);
    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
	tester_thread_t* sort_thread = new tester_thread_t(drive_stage, sc, "SORT THREAD");
	if ( thread_create( NULL, sort_thread ) ) {
	    TRACE(TRACE_ALWAYS, "thread_create failed\n");
	    QPIPE_PANIC();
	}
    }


    // for now just sort ints
    sc = new stage_container_t("FUNC_CALL_CONTAINER", new stage_factory<func_call_stage_t>);
    dispatcher_t::register_stage_container(func_call_packet_t::PACKET_TYPE, sc);
    tester_thread_t* func_call_thread =  new tester_thread_t(drive_stage, sc, "FUNC_CALL_THREAD");
    if ( thread_create( NULL, func_call_thread ) ) {
	TRACE(TRACE_ALWAYS, "thread_create() failed\n");
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
                               new trivial_filter_t(sizeof(int)), // unused, cannot be NULL
                               write_ints,
                               int_buffer);


    char* sort_packet_id;
    int sort_packet_id_ret = asprintf( &sort_packet_id, "SORT_PACKET_1" );
    assert( sort_packet_id_ret != -1 );

    tuple_buffer_t* output_buffer = new tuple_buffer_t(sizeof(int));
    tuple_filter_t* output_filter = new trivial_filter_t(int_buffer->tuple_size);
    sort_packet_t* packet = new sort_packet_t(sort_packet_id,
                                              output_buffer,
                                              output_filter,
                                              new int_key_extractor_t(),
                                              new int_key_compare_t(),
                                              fc_packet);
    dispatcher_t::dispatch_packet(packet);

    
    tuple_t output;


    while(1) {
        int get_ret = output_buffer->get_tuple(output);
        switch (get_ret) {

        case 0:
            if(do_echo)
                TRACE(TRACE_ALWAYS, "Value: %d\n", *(int*)output.data);
            continue;

        case 1:
            TRACE(TRACE_ALWAYS, "No more tuples...\n");
            TRACE(TRACE_ALWAYS, "TEST DONE\n");
            return 0;
            
        case -1:
            TRACE(TRACE_ALWAYS, "get_tuple() error!\n");
            QPIPE_PANIC();

        default:
            // unrecognized return value
            QPIPE_PANIC();
        }
    }
}
