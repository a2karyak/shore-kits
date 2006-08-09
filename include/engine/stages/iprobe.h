/* -*- mode:C++; c-basic-offset:4 -*- */
#ifndef _IPROBE_H
#define _IPROBE_H

#include "db_cxx.h"
#include "engine/core/tuple.h"
#include "engine/core/packet.h"
#include "engine/core/stage.h"

using namespace qpipe;



/**
 *  @brief Packet definition for the IPROBE stage.
 */

struct iprobe_packet_t : public packet_t {

public:

    static const c_str PACKET_TYPE;

    Db* _db;
    guard<packet_t> _input;
    guard<tuple_fifo> _input_buffer;
   
    /**
     *  @brief iprobe_packet_t constructor.
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
     *  @param db A handle to the table we are going to scan. Since
     *  Db*'s can be shared among threads, we will not take ownership
     *  of this object.
     */
    iprobe_packet_t(const c_str    &packet_id,
                    tuple_fifo* output_buffer,
                    tuple_filter_t* output_filter,
                    packet_t* input, Db* db)
	: packet_t(packet_id, PACKET_TYPE, output_buffer, output_filter, false),
	  _db(db), _input(input), _input_buffer(input->output_buffer())
    {
        assert(db != NULL);
    }
    
    virtual bool is_compatible(packet_t*) {
        // TODO: detect overlapping/dominating ranges
        return false;
    }

};



/**
 *  @brief Table scan stage that reads tuples from the storage manager.
 */

class iprobe_stage_t : public stage_t {

public:

    typedef iprobe_packet_t stage_packet_t;
    static const c_str DEFAULT_STAGE_NAME;

    iprobe_stage_t() { }

    virtual ~iprobe_stage_t() { }

protected:

    virtual void process_packet();
};



#endif
