/* -*- mode:C++; c-basic-offset:4 -*- */

#include "engine/stages/fdump.h"
#include "engine/core/tuple.h"
#include "engine/util/guard.h"
#include "engine/core/exception.h"
#include "qpipe_panic.h"
#include "trace.h"



const c_str fdump_packet_t::PACKET_TYPE = "FDUMP";



const c_str fdump_stage_t::DEFAULT_STAGE_NAME = "FDUMP_STAGE";



/**
 *  @brief Write the file specified by fdump_packet_t p.
 *
 *  @return 0 on success. Non-zero on unrecoverable error. The stage
 *  should terminate all queries it is processing.
 */

void fdump_stage_t::process_packet() {

    adaptor_t* adaptor = _adaptor;
    fdump_packet_t* packet = (fdump_packet_t*)adaptor->get_packet();


    const c_str &filename = packet->_filename;
    // make sure the file gets closed when we're done
    file_guard_t file = fopen(filename, "w+");
    if ( file == NULL )
        throw EXCEPTION(FileException, (string("fopen() failed on ") + filename).c_str());

    
    tuple_buffer_t* input_buffer = packet->_input_buffer;
    
    
    // read the file; make sure the buffer pages get deleted
    while (1) {
    
        page_guard_t tuple_page = input_buffer->get_page();
        if(tuple_page == NULL) {
            // no more pages
            TRACE(TRACE_DEBUG, "Finished dump to file %s\n", filename.data());
            return;
        }
        TRACE(TRACE_ALWAYS, "Wrote page\n");
        
        tuple_page->fwrite_full_page(file);
    }
}
