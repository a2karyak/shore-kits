/* -*- mode:C++; c-basic-offset:4 -*- */

#include "util.h"
#include "core.h"
#include "workload/tpch/tpch_db.h"

#include "tests/common/tester_thread.h"

#include <cstdlib>



using namespace qpipe;



void usage(const char* program_name) {
    TRACE(TRACE_ALWAYS, "Usage: %s <numtuples>\n",
          program_name);
    exit(-1);
}



int main(int argc, char* argv[]) {

    thread_init();

    // command-line args
    if ( argc < 2 )
        usage(argv[0]);
        
    // parse index
    char* numtuples_ptr = argv[2];
    char* numtuples_endptr;
    int numtuples = strtol(numtuples_ptr, &numtuples_endptr, 10);
    if ((numtuples_endptr == numtuples_ptr) || (numtuples < 0)) {
        // invalid number
        TRACE(TRACE_ALWAYS, "%s must be a non-negative integer\n", numtuples);
        usage(argv[0]);
    }


    tuple_fifo int_buffer(sizeof(int));


    for (int i = 0; i < numtuples; i++) {
        
        tuple_t srctup(&i, sizeof(i));
        tuple_t outtup = int_buffer.allocate();
        outtup.assign(srctup);
    }
    bool ret = buf.send_eof();
    TRACE(TRACE_ALWAYS, "i = %d: send_eof() returned %s\n",
          i,
          ret ? "TRUE" : "FALSE");



    return 0;
}
