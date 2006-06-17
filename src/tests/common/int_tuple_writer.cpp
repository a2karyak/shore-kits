
#include "tests/common/int_tuple_writer.h"


using namespace qpipe;


void* int_tuple_writer_main(void* arg)
{

    struct int_tuple_writer_info_s* info =
        (struct int_tuple_writer_info_s*)arg;

    tuple_buffer_t* int_buffer = info->int_buffer;
    int num_tuples = info->num_tuples;
    
    for (int i = 0; i < num_tuples; i++) {
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
