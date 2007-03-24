/* -*- mode:C++; c-basic-offset:4 -*- */

#include "util.h"
#include "core.h"
#include "workload/tpch/tpch_db.h"
#include "tests/common.h"

#include <cstdlib>



using namespace qpipe;



void usage(const char* program_name) {
    TRACE(TRACE_ALWAYS, "Usage: %s num_tuples\n",
          program_name);
    exit(-1);
}



int main(int argc, char* argv[]) {

    util_init();
    db_open_guard_t db_open;


    // command-line args
    if ( argc < 2 )
        usage(argv[0]);
        
    // parse index
    char* num_tuples_string = argv[1];
    char* end_ptr;
    int num_tuples = strtol(num_tuples_string, &end_ptr, 10);
    if ( (num_tuples == 0) && (end_ptr == num_tuples_string) ) {
        // invalid number
        TRACE(TRACE_ALWAYS, "%s is not a valid number\n", num_tuples);
        usage(argv[0]);
    }



    tuple_fifo int_buffer(sizeof(int), 1, 1);


    for (int i = 0; i < num_tuples; i++) {
        tuple_t tuple((char*)&i, sizeof(int));
        int_buffer.append(tuple);
    }
    int_buffer.send_eof();
    TRACE(TRACE_ALWAYS, "Added %d tuples into the tuple_fifo\n", num_tuples);
    tuple_t tuple;
    while (int_buffer.get_tuple(tuple)) {
        int* tuple_int = aligned_cast<int>(tuple.data);
        TRACE(TRACE_ALWAYS, "Read %d\n", *tuple_int);
    }


    return 0;
}
