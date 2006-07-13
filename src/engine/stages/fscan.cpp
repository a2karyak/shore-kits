/* -*- mode:C++; c-basic-offset:4 -*- */

#include "engine/stages/fscan.h"
#include "engine/core/tuple.h"
#include "qpipe_panic.h"
#include "trace.h"
#include "exceptions.h"



const char* fscan_packet_t::PACKET_TYPE = "FSCAN";



const char* fscan_stage_t::DEFAULT_STAGE_NAME = "FSCAN_STAGE";



/**
 *  @brief Read the file specified by fscan_packet_t p.
 *
 *  @return 0 on success. Non-zero on unrecoverable error. The stage
 *  should terminate all queries it is processing.
 */

void fscan_stage_t::process_packet() {

    adaptor_t* adaptor = _adaptor;
    fscan_packet_t* packet = (fscan_packet_t*)adaptor->get_packet();


    char* filename = packet->_filename;
    file_guard_t file = fopen(filename, "r");
    if ( file == NULL )
        throw syscall_exception(string("fopen() failed on ") + filename);

    page_guard_t tuple_page =
	tuple_page_t::alloc(packet->_output_buffer->tuple_size);


    // If we have FSCAN working sharing enabled, we could be accepting
    // packets now. We would like to avoid doing so when we start
    // sending tuples to output().
    bool accepting_packets = true;

    while (1)
    {
	// read the next page of tuples
	if(tuple_page->fread_full_page(file))
            return;
        
	// We must stop accepting packets as soon as we output() any
	// tuples. Any packet accepted after this point will miss some
	// of the data we are reading.
	if (accepting_packets) {
	    adaptor->stop_accepting_packets();
	    accepting_packets = false;
	}
	   

	// output() each tuple in the page
	tuple_page_t::iterator it = tuple_page->begin();
        while(it != tuple_page->end())
	    adaptor->output(*it++);

	// continue to next page
    }
}
