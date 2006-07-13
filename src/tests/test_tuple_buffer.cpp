/* -*- mode:C++; c-basic-offset:4 -*- */

#include "engine/thread.h"
#include "engine/core/tuple.h"
#include "qpipe_panic.h"

#include "tests/common/tester_thread.h"

#include <stdlib.h>



using namespace qpipe;



int num_tuples = 100;



void usage(const char* program_name) {
    TRACE(TRACE_ALWAYS, "Usage: %s TERMINATE|SEND_EOF index\n",
          program_name);
    exit(-1);
}

void do_terminate(tuple_buffer_t &buf, int i) {
    bool ret = buf.terminate();
    TRACE(TRACE_ALWAYS, "i = %d: terminate() returned %s\n",
          i,
          ret ? "TRUE" : "FALSE");
}

void do_send_eof(tuple_buffer_t &buf, int i) {
    bool ret = buf.send_eof();
    TRACE(TRACE_ALWAYS, "i = %d: send_eof() returned %s\n",
          i,
          ret ? "TRUE" : "FALSE");
}



int main(int argc, char* argv[]) {

    thread_init();


    // command-line args
    if ( argc < 3 )
        usage(argv[0]);
        
    // parse action
    const char* opt = argv[1];
    bool terminate = false, send_eof = false;
    if ( !strcmp(opt, "TERMINATE") )
        terminate = true;
    if ( !strcmp(opt, "SEND_EOF") )
        send_eof = true;
    if ( !terminate && !send_eof )
        usage(argv[0]);

    // parse index
    int index = 0;
    char* index_string = argv[2];
    char* end_ptr;
    index = strtol(index_string, &end_ptr, 10);
    if ( (index == 0) && (end_ptr == index_string) ) {
        // invalid number
        TRACE(TRACE_ALWAYS, "%s is not a valid number\n", index_string);
        usage(argv[0]);
    }



    tuple_buffer_t int_buffer(sizeof(int));

    // can only send EOF once, although we can terminate multiple
    // times
    bool sent_eof = false;


    for (int i = 0; i < num_tuples; i++) {

        if (i <= index) {
            tuple_t tuple((char*)&i, sizeof(int));
            int_buffer.put_tuple(tuple);
        }

        if ((i == index) && terminate)
            do_terminate(int_buffer, i);
        if ((i == index) && send_eof && !sent_eof) {
            do_send_eof(int_buffer, i);
            sent_eof = true;
        }
        
        if (i >= index) {
            do_terminate(int_buffer, i);
            if ( !sent_eof ) {
                do_send_eof(int_buffer, i);
                sent_eof = true;
            }
        }
    }
    
    
    return 0;
}
