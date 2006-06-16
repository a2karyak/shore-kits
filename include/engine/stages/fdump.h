/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _FDUMP_H
#define _FDUMP_H

#include <cstdio>

#include "db_cxx.h"
#include "tuple.h"
#include "packet.h"
#include "stage.h"
#include "trace.h"
#include "qpipe_panic.h"



using namespace qpipe;



/* exported functions */


/**
 *  @brief
 */
struct fdump_packet_t : public packet_t {

public:
    
    static const char* PACKET_TYPE;

    tuple_buffer_t* _input_buffer;
    
    char* _filename;

    notify_t* _notifier;


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
     *  FDUMP doesn't really need a tuple filter. All filtering for
     *  the query can be moved to the stage feeding the FDUMP. The
     *  caller should be able to get away with creating an instance of
     *  the tuple_filter_t base class.
     *
     *  @param filename The name of the file to scan. This should be a
     *  block of bytes allocated with malloc(). This packet will take
     *  ownership of this block and invoke free() when it is
     *  destroyed.
     *
     *  @param notifier An optional notify_t instance that the worker
     *  thread will invoke notify() on when the FDUMP operation is
     *  complete. Pass NULL to avoid this behavior. Even if a non-NULL
     *  value is passed, this packet WILL NOT take ownership of this
     *  object. We will however, invoke notify() on this parameter in
     *  our destructor.
     */    
    fdump_packet_t(char*           packet_id,
		   tuple_buffer_t* output_buffer,
                   tuple_filter_t* output_filter,
		   tuple_buffer_t* input_buffer,
		   char*           filename,
                   notify_t*       notifier=NULL)
	: packet_t(packet_id, PACKET_TYPE, output_buffer, output_filter, false),
	  _input_buffer(input_buffer),
          _filename(filename),
          _notifier(notifier)
    {
    }


    virtual ~fdump_packet_t() {

        if ( _notifier != NULL )
            _notifier->notify();

        // input buffer should have been set to NULL by either
        // destroy_subpackets() or terminate_inputs()
        assert(_input_buffer == NULL);

        free(_filename);
    }


    virtual void destroy_subpackets() {
        // FDUMP has no subpackets. This should never be invoked
        // anyway since an FDUMP is inherently non-mergeable.
        TRACE(TRACE_ALWAYS, "FDUMP is non-mergeable!\n");
        QPIPE_PANIC();
    }    


    virtual void terminate_inputs() {	

        // input buffer
        if ( !_input_buffer->terminate() ) {
            // Producer has already terminated this buffer! We are now
            // responsible for deleting it.
            delete _input_buffer;
        }
        _input_buffer = NULL;

        // We don't have an input packet(s) that we need to worry
        // about. Nothing left to do.
    }
};



/**
 *  @brief FDUMP stage. Creates a (hopefully) temporary file on the
 *  local file system.
 */
class fdump_stage_t : public stage_t {

public:

    static const char* DEFAULT_STAGE_NAME;

    fdump_stage_t() { }

    virtual ~fdump_stage_t() { }

protected:

    virtual result_t process_packet();
};



#endif
