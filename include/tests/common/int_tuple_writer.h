// -*- mode:C++; c-basic-offset:4 -*-

#ifndef _INT_TUPLE_WRITER_H
#define _INT_TUPLE_WRITER_H

#include "engine/core/tuple.h"
#include "engine/core/stage.h"

using namespace qpipe;



/* exported datatypes */

struct int_tuple_writer_info_s {
  
    // output tuple size should be sizeof(int)
    tuple_buffer_t* int_buffer;

    // the number of tuples to write;
    int num_tuples;

    int start_tuple;
    
    int_tuple_writer_info_s(tuple_buffer_t* i_buffer, int n_tuples, int s_tuple=0)
        : int_buffer(i_buffer),
          num_tuples(n_tuples),
          start_tuple(s_tuple)
    {
    }
    
    ~int_tuple_writer_info_s()
    {
    }
};



/* exported functions */

void* int_tuple_writer_main(void* arg);
void shuffled_triangle_int_tuple_writer_fc(void* arg);
void increasing_int_tuple_writer_fc(void* arg);



#endif
