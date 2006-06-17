// -*- mode:C++; c-basic-offset:4 -*-

#include "engine/thread.h"
#include "engine/stages/func_call.h"
#include "engine/stages/merge.h"
#include "trace.h"
#include "qpipe_panic.h"
#include "engine/core/stage_container.h"

#include "tests/common/tester_thread.h"

#include <utility>
#include <vector>
#include <algorithm>

using namespace qpipe;
using std::vector;
using std::pair;



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


typedef vector<int> input_list_t;
typedef pair<tuple_buffer_t*, input_list_t> write_info_t;


stage_t::result_t write_tuples(void* arg)
{
    write_info_t *info = (write_info_t *)arg;
    tuple_buffer_t *buffer = info->first;
    input_list_t &inputs = info->second;

    int value;
    tuple_t input((char *)&value, sizeof(int));
    for(input_list_t::iterator it=inputs.begin(); it != inputs.end(); ++it) {
        value = *it;
        if(buffer->put_tuple(input)) {
            TRACE(TRACE_ALWAYS, "buffer->put_tuple() returned non-zero!\n");
            TRACE(TRACE_ALWAYS, "Terminating loop...\n");
            break;
        }
    }

    TRACE(TRACE_ALWAYS, "Done inserting tuples\n");
    return stage_t::RESULT_STOP;
}


struct int_tuple_comparator_t : public tuple_comparator_t {
    virtual int make_key(const tuple_t &tuple) {
        return *(int*)tuple.data;
    }
};



int main(int argc, char* argv[]) {

    thread_init();

    int merge_factor;
    int count;
    bool do_echo = true;



    // parse command line args
    if ( argc < 3 ) {
	TRACE(TRACE_ALWAYS, "Usage: %s <merge factor> <tuples per run> [off]\n", argv[0]);
	exit(-1);
    }
    merge_factor = atoi(argv[1]);
    if ( merge_factor == 0 ) {
	TRACE(TRACE_ALWAYS, "Invalid merge factor %s\n", argv[1]);
	exit(-1);
    }
    count = atoi(argv[2]);
    if ( count == 0 ) {
	TRACE(TRACE_ALWAYS, "Invalid count %s\n", argv[2]);
	exit(-1);
    }
    if ( (argc > 3) && !strcmp(argv[3], "off") )
        do_echo = false;



    // create a FUNC_CALL stage to feed the MERGE stage
    stage_container_t* fc_sc = new stage_container_t("FUNC_CALL_CONTAINER", new stage_factory<func_call_stage_t>);
    dispatcher_t::register_stage_container(func_call_packet_t::PACKET_TYPE, fc_sc);
    

    stage_container_t* merge_sc = new stage_container_t("MERGE_CONTAINER", new stage_factory<merge_stage_t>);
    dispatcher_t::register_stage_container(merge_packet_t::PACKET_TYPE, merge_sc);

    tester_thread_t* merge_thread = new tester_thread_t(drive_stage, merge_sc, "MERGE THREAD");
    if ( thread_create( NULL, merge_thread ) ) {
        TRACE(TRACE_ALWAYS, "thread_create failed\n");
        QPIPE_PANIC();
    }



    write_info_t merge_info[merge_factor];


    
    // Generate a list of count unique tuples with merge_factor
    // duplicates of each
    input_list_t main_inputs;
    for(int i=0; i < count; i++)
        for(int j=0; j < merge_factor; j++)
            main_inputs.push_back(i);
    
    
    
    // shuffle the list and partition it
    std::random_shuffle(main_inputs.begin(), main_inputs.end());

    

    int i=0;
    input_list_t::iterator it=main_inputs.begin();
    while(it != main_inputs.end()) {
        input_list_t &input_list = merge_info[i].second;
        std::sort(it, it+count);
        input_list.insert(input_list.end(), it, it+count);
        i++;
        it += count;
    }
    


    // send the partitions to merge input generation threads
    merge_packet_t::buffer_list_t input_buffers;
    for(int i=0; i < merge_factor; i++) {

        tuple_buffer_t *input_buffer = new tuple_buffer_t(sizeof(int));
        input_buffers.push_back(input_buffer);
        merge_info[i].first = input_buffer;

        char* writer_thread_name;
        asprintf(&writer_thread_name, "WRITER_THREAD_%d", i);
        
        // We want to feed MERGE in parallel, so create a new
        // FUNC_CALL worker thread as well as a FUNC_CALL packet.
        tester_thread_t* writer_thread = new tester_thread_t(drive_stage, fc_sc, writer_thread_name);
        if ( thread_create( NULL, writer_thread ) ) {
            TRACE(TRACE_ALWAYS, "thread_create failed\n");
            QPIPE_PANIC();
        }

        char* fc_packet_id;
        asprintf(&fc_packet_id, "FUNC_CALL_PACKET_%d", i);
        func_call_packet_t* fc_packet = 
            new func_call_packet_t(fc_packet_id,
                                   merge_info[i].first, 
                                   new tuple_filter_t(sizeof(int)), // unused, cannot be NULL
                                   write_tuples,
                                   &merge_info[i]);

        // Need to dispatch the FUNC_CALL packet ourselves. MERGE
        // assumes that the meta-stage performs this dispatch.
        dispatcher_t::dispatch_packet(fc_packet);
    }
    
    
    
    // fire up the merge stage now
    tuple_buffer_t* output_buffer = new tuple_buffer_t(sizeof(int));
    int_tuple_comparator_t comparator;

    char* merge_packet_id;
    int merge_packet_id_ret =
        asprintf( &merge_packet_id, "MERGE_PACKET_1" );
    assert( merge_packet_id_ret != -1 );

    
    merge_packet_t* packet = new merge_packet_t(merge_packet_id,
                                                output_buffer,
                                                new tuple_filter_t(input_buffers[0]->tuple_size),
                                                input_buffers,
                                                &comparator);
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
