// -*- mode:C++; c-basic-offset:4 -*-

#include "tests/common/int_tuple_writer.h"
#include <vector>
#include <algorithm>

using std::vector;
using namespace qpipe;



void* int_tuple_writer_main(void* arg)
{

    struct int_tuple_writer_info_s* info =
        (struct int_tuple_writer_info_s*)arg;

    tuple_buffer_t* int_buffer = info->int_buffer;
    int num_tuples = info->num_tuples;
    
    for (int i = info->start_tuple; i < num_tuples; i++) {
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



stage_t::result_t shuffled_triangle_int_tuple_writer_fc(void* arg) {
  
    struct int_tuple_writer_info_s* info =
        (struct int_tuple_writer_info_s*)arg;

    tuple_buffer_t* int_buffer = info->int_buffer;
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
        tuple_t in_tuple((char*)&tuples[i], sizeof(int));
        if( int_buffer->put_tuple(in_tuple) ) {
            TRACE(TRACE_ALWAYS, "tuple_page->append_init() returned non-zero!\n");
            TRACE(TRACE_ALWAYS, "Terminating loop...\n");
            break;
        }
    }
   
    return stage_t::RESULT_STOP;
}



stage_t::result_t increasing_int_tuple_writer_fc(void* arg)
{

    struct int_tuple_writer_info_s* info =
        (struct int_tuple_writer_info_s*)arg;

    tuple_buffer_t* int_buffer = info->int_buffer;
    int num_tuples = info->num_tuples;


    for (int i = info->start_tuple; i < num_tuples; i++) {
	tuple_t in_tuple((char*)&i, sizeof(int));
	if( int_buffer->put_tuple(in_tuple) ) {
	    TRACE(TRACE_ALWAYS, "tuple_page->append_init() returned non-zero!\n");
	    TRACE(TRACE_ALWAYS, "Terminating loop...\n");
	    break;
	}
    }
    
    return stage_t::RESULT_STOP;
}
