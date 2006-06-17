
#ifndef _INT_TUPLE_WRITER_H
#define _INT_TUPLE_WRITER_H

#include "engine/core/tuple.h"



/* exported datatypes */

struct int_tuple_writer_info_s {
  
  // output tuple size should be sizeof(int)
  qpipe::tuple_buffer_t* int_buffer;

  // the number of tuples to write;
  int num_tuples;
};



/* exported functions */

void* int_tuple_writer_main(void* arg);



#endif
