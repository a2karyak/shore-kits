/* -*- mode:C++; c-basic-offset:4 -*- */

#include "stages/fdump.h"
#include "qpipe_panic.h"
#include "trace.h"
#include "tuple.h"
#include "guard.h"



const char* fdump_packet_t::PACKET_TYPE = "FDUMP";



const char* fdump_stage_t::DEFAULT_STAGE_NAME = "FDUMP_STAGE";



/**
 *  @brief Write the file specified by fdump_packet_t p.
 *
 *  @return 0 on success. Non-zero on unrecoverable error. The stage
 *  should terminate all queries it is processing.
 */

stage_t::result_t fdump_stage_t::process_packet() {

    adaptor_t* adaptor = _adaptor;
    fdump_packet_t* packet = (fdump_packet_t*)adaptor->get_packet();


    char* filename = packet->_filename;
    // make sure the file gets closed when we're done
    file_guard_t file = fopen(filename, "w+");
    if (file == NULL) {
	TRACE(TRACE_ALWAYS, "fopen() failed on %s\n", filename);
	return stage_t::RESULT_ERROR;
    }
    
    
    tuple_buffer_t* input_buffer = packet->_input_buffer;
    
    
    // read the file; make sure the buffer pages get deleted
    while (1) {
    
        tuple_page_t* tuple_page;
        switch (input_buffer->get_page(tuple_page)) {

        case 0:
            {
                // removed a page
                page_guard_t page_guard = tuple_page;
                if ( page_guard->fwrite_full_page(file) ) {
                    TRACE(TRACE_ALWAYS, "fwrite_full_page() failed\n");
                    return stage_t::RESULT_ERROR;
                }
                continue;
            }
   
        case 1:
            // no more pages
            TRACE(TRACE_DEBUG, "Finished dump to file %s\n", filename);
            return stage_t::RESULT_STOP;
            
        case -1:
            // producer has terminated buffer!
            TRACE(TRACE_DEBUG, "Detected input buffer termination. Halting dump to file %s\n", filename);
            return stage_t::RESULT_ERROR;

        default:
            // unrecognized return value
            QPIPE_PANIC();
        }
    }
}
