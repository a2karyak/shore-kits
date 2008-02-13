// -*- mode:C++; c-basic-offset:4 -*-

#include "tests/common/int_tuple_writer.h"
#include <vector>
#include <algorithm>

using std::vector;
using namespace qpipe;




void int_tuple_writer_fc(void*, void* itw_info) {

    struct int_tuple_writer_info_s* info =
        (struct int_tuple_writer_info_s*)itw_info;
    
    tuple_fifo* int_buffer = info->int_buffer;
    int num_tuples = info->num_tuples;
    
    for (int i = info->start_tuple; i < num_tuples; i++) {
	tuple_t in_tuple((char*)&i, sizeof(int));
	int_buffer->append(in_tuple);
    }
}



void int_tuple_writer_main(void* itw_info) {

    int_tuple_writer_fc(NULL, itw_info);
    
    struct int_tuple_writer_info_s* info =
        (struct int_tuple_writer_info_s*)itw_info;
    info->int_buffer->send_eof();
}



void shuffled_triangle_int_tuple_writer_fc(void*, void* itw_info) {
  
    struct int_tuple_writer_info_s* info =
        (struct int_tuple_writer_info_s*)itw_info;

    tuple_fifo* int_buffer = info->int_buffer;
    int num_tuples = info->num_tuples;
    
    vector<int> tuples;


    // create X copies of X
    for (int i = info->start_tuple; i < num_tuples; i++) {
        for(int j=0; j < i; j++)
            tuples.push_back(i);
    }
 

    // shuffle inputs
    std::random_shuffle(tuples.begin(), tuples.end());
    
    
    for (unsigned i=0; i < tuples.size(); i++) {
        TRACE(TRACE_ALWAYS, "Writing %d\n", tuples[i]);
        tuple_t in_tuple((char*)&tuples[i], sizeof(int));
        int_buffer->append(in_tuple);
    }
}



void increasing_int_tuple_writer_fc(void*, void* itw_info)
{

    struct int_tuple_writer_info_s* info =
        (struct int_tuple_writer_info_s*)itw_info;

    tuple_fifo* int_buffer = info->int_buffer;
    int num_tuples = info->num_tuples;


    for (int i = info->start_tuple; i < num_tuples; i++) {
	tuple_t in_tuple((char*)&i, sizeof(int));
	int_buffer->append(in_tuple);
    }
}
