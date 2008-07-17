// -*- mode:C++; c-basic-offset:4 -*-

#include "stages.h"
#include "workload/tpch/tpch_db.h"
#include "workload/common.h"
#include "tests/common.h"

#include <utility>
#include <vector>
#include <algorithm>

using namespace qpipe;
using namespace workload;
using std::vector;
using std::pair;


typedef vector<int> input_list_t;
typedef pair<tuple_fifo*, input_list_t> write_info_t;


class test_merge_stage_process_tuple_t : public process_tuple_t {
private:
    bool _do_echo;

public:

    test_merge_stage_process_tuple_t(bool do_echo)
        : _do_echo(do_echo)
    {
    }

    virtual void process(const tuple_t& output) {
        if(_do_echo)
            TRACE(TRACE_ALWAYS, "Value: %d\n", *aligned_cast<int>(output.data));
    }

    virtual void end() {
        TRACE(TRACE_ALWAYS, "No more tuples...\n");
        TRACE(TRACE_ALWAYS, "TEST DONE\n");
    }

};


void write_tuples(void*, void* winfo)
{
    write_info_t *info = (write_info_t *)winfo;
    tuple_fifo* buffer = info->first;
    input_list_t &inputs = info->second;

    int value;
    tuple_t input((char *)&value, sizeof(int));
    for(input_list_t::iterator it=inputs.begin(); it != inputs.end(); ++it) {
        value = *it;
        buffer->append(input);
    }

    TRACE(TRACE_ALWAYS, "Done inserting tuples\n");
}


int main(int argc, char* argv[]) {

    thread_init();
    db_open_guard_t db_open;
    
    TRACE(TRACE_ALWAYS,
          "This unit test is currently broken for the following reasons:\n"
          " - it does not acquire and release workers like a meta-stage should"
          " - I have disabled the dispatching of the FUNC_CALL packets that feed the merge\n");
    assert(0);
    
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
    register_stage<func_call_stage_t>(1);
    register_stage<merge_stage_t>(merge_factor);


    array_guard_t<write_info_t> merge_info = new write_info_t[merge_factor];

    
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

        tuple_fifo *input_buffer = new tuple_fifo(sizeof(int));
        input_buffers.push_back(input_buffer);
        merge_info[i].first = input_buffer;

        c_str writer_thread_name("WRITER_THREAD_%d", i);
        
        c_str fc_packet_id("FUNC_CALL_PACKET_%d", i);
        func_call_packet_t* fc_packet = 
            new func_call_packet_t(fc_packet_id.data(),
                                   merge_info[i].first, 
                                   new trivial_filter_t(sizeof(int)), // unused, cannot be NULL
                                   write_tuples,
                                   &merge_info[i]);

        // Need to dispatch the FUNC_CALL packet ourselves. MERGE
        // assumes that the meta-stage performs this dispatch.
        fc_packet = fc_packet;
#if 0
        reserve_query_workers(fc_packet);
        dispatcher_t::dispatch_packet(fc_packet);
#endif
    }
    
    
    // fire up the merge stage now
    guard<tuple_fifo> output_buffer = new tuple_fifo(sizeof(int));
    merge_packet_t* packet = new merge_packet_t("MERGE_PACKET_1",
                                                output_buffer,
                                                new trivial_filter_t(input_buffers[0]->tuple_size()),
                                                input_buffers,
                                                new int_key_extractor_t(),
                                                new int_key_compare_t());

    test_merge_stage_process_tuple_t pt(do_echo);
    process_query(packet, pt);
    
    return 0;
}