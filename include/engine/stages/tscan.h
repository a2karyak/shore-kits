/* -*- mode:C++; c-basic-offset:4 -*- */
#ifndef __TSCAN_H
#define __TSCAN_H

#include "db_cxx.h"
#include "engine/core/tuple.h"
#include "engine/core/packet.h"
#include "engine/core/stage.h"



using namespace qpipe;



/**
 *@brief Packet definition for the Tscan stage
 */

struct tscan_packet_t : public packet_t {

public:

    static const char* PACKET_TYPE;


    Db* _db;

   
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
     *  @param db A handle to the table we are going to scan. Since
     *  Db*'s can be shared among threads, we will not take ownership
     *  of this object.
     */
    tscan_packet_t(char*           packet_id,
		   tuple_buffer_t* output_buffer,
		   tuple_filter_t* output_filter,
		   Db* db)
	: packet_t(packet_id, PACKET_TYPE, output_buffer, output_filter, true),
	  _db(db)
    {
        assert(db != NULL);
    }
    
    virtual ~tscan_packet_t() {
        // we don't own _db
    }

    virtual void destroy_subpackets() {
        // No input buffers or subpackets ... do nothing.
    }

    virtual void terminate_inputs() {
        // No input buffers ... do nothing.
    }

    virtual bool is_compatible(packet_t* other) {
        // two TSCAN packets are compatible if they want to scan the
        // same table
        tscan_packet_t* packet = dynamic_cast<tscan_packet_t*>(other);
        return _db == packet->_db;
    }

};



/**
 *  @brief Table scan stage that reads tuples from the storage manager.
 */

class tscan_stage_t : public stage_t {

public:

    typedef tscan_packet_t stage_packet_t;
    static const char* DEFAULT_STAGE_NAME;

    static const size_t TSCAN_BULK_READ_BUFFER_SIZE;

    tscan_stage_t() { }

    virtual ~tscan_stage_t() { }

protected:

    virtual result_t process_packet();
};



#endif
