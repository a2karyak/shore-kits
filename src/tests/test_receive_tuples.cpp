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

    util_init();

    // command-line args
    if ( argc < 2 )
        usage(argv[0]);
        
    // parse index
    char* numtuples_ptr = argv[1];
    char* numtuples_endptr;
    int numtuples = strtol(numtuples_ptr, &numtuples_endptr, 10);
    if ((numtuples_endptr == numtuples_ptr) || (numtuples < 0)) {
        // invalid number
        TRACE(TRACE_ALWAYS, "%s must be a non-negative integer\n", numtuples_ptr);
        usage(argv[0]);
    }


    tuple_fifo buf(sizeof(int));

    
    for (int i = 0; i < numtuples; i++) {
        
        tuple_t srctup((char*)&i, (size_t)sizeof(int));
        tuple_t outtup = buf.allocate();
        outtup.assign(srctup);
    }
    bool ret = buf.send_eof();
    TRACE(TRACE_ALWAYS, "send_eof() returned %s\n",
          ret ? "TRUE" : "FALSE");


    /* Receive the tuples which were sent to me. */
    tuple_t tup;
    while (buf.get_tuple(tup)) {
        int i;
        memcpy(&i, tup.data, sizeof(int));
        TRACE(TRACE_ALWAYS, "Got %d\n", i);        
    }


    return 0;
}
