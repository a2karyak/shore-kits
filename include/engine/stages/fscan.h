/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _FSCAN_H
#define _FSCAN_H

#include <cstdio>

#include "tuple.h"
#include "packet.h"
#include "stage.h"
#include "dispatcher.h"
#include "trace.h"
#include "qpipe_panic.h"



using namespace qpipe;



/* exported datatypes */


class fscan_packet_t : public packet_t {
  
public:

    static const char* PACKET_TYPE;
    

    char* _filename;


    /**
     *  @brief fscan_packet_t constructor.
     *
     *  @param packet_id The ID of this packet. This should point to a
     *  block of bytes allocated with malloc(). This packet will take
     *  ownership of this block and invoke free() when it is
     *  destroyed.
     *
     *  @param output_buffer The buffer where this packet should send
     *  its data. A packet DOES NOT own its output buffer (we will not
     *  invoke delete or free() on this field in our packet
     *  destructor).
     *
     *  @param output_filter The filter that will be applied to any
     *  tuple sent to output_buffer. The packet OWNS this filter. It
     *  will be deleted in the packet destructor.
     *
     *  @param filename The name of the file to scan. This should be a
     *  block of bytes allocated with malloc(). This packet will take
     *  ownership of this block and invoke free() when it is
     *  destroyed.
     *
     *  Raw FSCANS should not be mergeable based on not
     *  mergeable. They should be merged at the meta-stage.
     */
    fscan_packet_t(char*           packet_id,
		   tuple_buffer_t* output_buffer,
		   tuple_filter_t* output_filter,
		   char*           filename)
	: packet_t(packet_id, PACKET_TYPE, output_buffer, output_filter, false),
          _filename(filename)
    {
        // error checking
        assert(filename != NULL);
    }

    
    virtual ~fscan_packet_t() {
        free(_filename);
    }
    
    
    virtual void destroy_subpackets() {
        // No input buffers or subpackets ... do nothing.
    }

    virtual void terminate_inputs() {
        // No input buffers ... do nothing.
    }

};



/**
 *  @brief FSCAN stage. Reads tuples from a file on the local file
 *  system.
 */
class fscan_stage_t : public stage_t {

public:

    static const char* DEFAULT_STAGE_NAME;

    fscan_stage_t() { }
    
    virtual ~fscan_stage_t() { }
    
protected:

    virtual result_t process_packet();
    
    result_t read_file(adaptor_t* adaptor, FILE* file, tuple_page_t* tuple_page);
};



#endif
