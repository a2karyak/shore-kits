// -*- mode:C++; c-basic-offset:4 -*-

#ifndef _INT_TUPLE_WRITER_H
#define _INT_TUPLE_WRITER_H

#include "core.h"

using namespace qpipe;



/* exported datatypes */

struct int_tuple_writer_info_s {
  
    // output tuple size should be sizeof(int)
    tuple_fifo* int_buffer;

    // the number of tuples to write;
    int num_tuples;

    int start_tuple;
    
    int_tuple_writer_info_s(tuple_fifo* i_buffer, int n_tuples, int s_tuple=0)
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

void int_tuple_writer_main(void* itw_info);
void int_tuple_writer_fc(void* unused, void* itw_info);
void shuffled_triangle_int_tuple_writer_fc(void* unused, void* itw_info);
void increasing_int_tuple_writer_fc(void* unused, void* itw_info);



#endif
