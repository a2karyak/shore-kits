/* -*- mode:C++; c-basic-offset:4 -*- */

#include "stages/fdump.h"



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
    guard<FILE> file = fopen(filename.data(), "w+");
    if (file == NULL)
        THROW3(FileException,
                        "Caught %s opening '%s'",
                        errno_to_str().data(), filename.data());

    
    tuple_fifo* input_buffer = packet->_input_buffer;
    
    
    // read the file; make sure the buffer pages get deleted
    while (1) {
    
        guard<qpipe::page> tuple_page = input_buffer->get_page();
        if(tuple_page == NULL) {
            // no more pages
            TRACE(TRACE_DEBUG, "Finished dump to file %s\n", filename.data());
            return;
        }
        TRACE(TRACE_ALWAYS, "Wrote page\n");
        
        tuple_page->fwrite_full_page(file);
    }
}
