/* -*- mode:C++; c-basic-offset:4 -*- */

#include "engine/stages/fscan.h"
#include "engine/core/tuple.h"
#include "qpipe_panic.h"
#include "trace.h"



const char* fscan_packet_t::PACKET_TYPE = "FSCAN";



const char* fscan_stage_t::DEFAULT_STAGE_NAME = "FSCAN_STAGE";



/**
 *  @brief Read the file specified by fscan_packet_t p.
 *
 *  @return 0 on success. Non-zero on unrecoverable error. The stage
 *  should terminate all queries it is processing.
 */

stage_t::result_t fscan_stage_t::process_packet() {

    adaptor_t* adaptor = _adaptor;
    fscan_packet_t* packet = (fscan_packet_t*)adaptor->get_packet();


    char* filename = packet->_filename;
    file_guard_t file = fopen(filename, "r");
    if ( file == NULL ) {
	TRACE(TRACE_ALWAYS, "fopen() failed on %s\n", filename);
	return stage_t::RESULT_ERROR;
    }
    

    page_guard_t tuple_page =
	tuple_page_t::alloc(packet->_output_buffer->tuple_size);
    if ( tuple_page == NULL ) {
	TRACE(TRACE_ALWAYS, "tuple_page_t::alloc() failed\n");
	return stage_t::RESULT_ERROR;
    }

    return read_file(adaptor, file, tuple_page);
}



stage_t::result_t fscan_stage_t::read_file(adaptor_t* adaptor, FILE* file, tuple_page_t* tuple_page) {


   // If we have FSCAN working sharing enabled, we could be accepting
    // packets now. We would like to avoid doing so when we start
    // sending tuples to output().
    bool accepting_packets = true;

    while (1)
    {
	// read the next page of tuples
	int read_ret = tuple_page->fread_full_page(file);
	if ( read_ret ==  1 ) {
	    // done reading the file
	    return stage_t::RESULT_STOP;
	}

	if ( read_ret == -1 ) {
	    // short read! treat this as an error!
	    TRACE(TRACE_ALWAYS, "tuple_page_t::fread() failed\n");
	    return stage_t::RESULT_ERROR;
	}


	// We must stop accepting packets as soon as we output() any
	// tuples. Any packet accepted after this point will miss some
	// of the data we are reading.
	if (accepting_packets) {
	    adaptor->stop_accepting_packets();
	    accepting_packets = false;
	}
	   

	// output() each tuple in the page
	tuple_page_t::iterator it;
	for (it = tuple_page->begin(); it != tuple_page->end(); ++it) {
	    tuple_t current_tuple = *it;
	    stage_t::result_t output_ret = adaptor->output(current_tuple);
            if(output_ret)
                return output_ret;
	}

	// continue to next page
    }
}
