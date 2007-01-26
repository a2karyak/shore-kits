// -*- mode:C++; c-basic-offset:4 -*-

#include "stages.h"

#include "workload/common.h"
#include "tests/common.h"
#include "workload/tpch/tpch_db.h"
#include "workload/common/register_stage.h"
#include <vector>
#include <algorithm>

using std::vector;

using namespace qpipe;


int num_values;
int num_copies;



void write_ints(void* arg)
{
    tuple_fifo* buffer = (tuple_fifo *)arg;

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
        buffer->append(input);
    }

    TRACE(TRACE_ALWAYS, "Finished inserting '%d' tuples\n", value);
}



int main(int argc, char* argv[]) {

    thread_init();
    db_open();
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


    register_stage<fdump_stage_t>(THREAD_POOL_SIZE);
    register_stage<fscan_stage_t>(THREAD_POOL_SIZE);
    register_stage<merge_stage_t>(THREAD_POOL_SIZE);
    register_stage<sort_stage_t>(THREAD_POOL_SIZE);
    register_stage<func_call_stage_t>(1);
    

    tuple_fifo* int_buffer = new tuple_fifo(sizeof(int));
    func_call_packet_t* fc_packet = 
	new func_call_packet_t("FUNC_CALL_PACKET_1",
                               int_buffer, 
                               new trivial_filter_t(sizeof(int)), // unused, cannot be NULL
                               write_ints,
                               int_buffer);


    tuple_fifo* output_buffer = new tuple_fifo(sizeof(int));
    tuple_filter_t* output_filter = new trivial_filter_t(int_buffer->tuple_size());
    sort_packet_t* packet = new sort_packet_t("SORT_PACKET_1",
                                              output_buffer,
                                              output_filter,
                                              new int_key_extractor_t(),
                                              new int_key_compare_t(),
                                              fc_packet);
    dispatcher_t::dispatch_packet(packet);

    
    tuple_t output;


    while (output_buffer->get_tuple(output)) {
        TRACE(TRACE_ALWAYS, "Value: %d\n", *aligned_cast<int>(output.data));
    }
}
