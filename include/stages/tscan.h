/* -*- mode:C++; c-basic-offset:4 -*- */
#ifndef _TSCAN_H
#define _TSCAN_H

#include "core.h"
#include "workload/tpch/tpch_db.h"



using namespace qpipe;



/**
 *  @brief Packet definition for the TSCAN stage.
 */

struct tscan_packet_t : public packet_t {

public:

    static const c_str PACKET_TYPE;


    Db* _db;

   
    /**
     *  @brief tscan_packet_t constructor.
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
    tscan_packet_t(const c_str    &packet_id,
		   tuple_fifo* output_buffer,
		   tuple_filter_t* output_filter,
		   Db* db)
	: packet_t(packet_id, PACKET_TYPE, output_buffer, output_filter,
                   create_plan(output_filter, db)),
	  _db(db)
    {
        assert(db != NULL);
    }

    static query_plan* create_plan(tuple_filter_t* filter, Db* db) {
        c_str action("%s:%p", PACKET_TYPE.data(), db);
        return new query_plan(action, filter->to_string(), NULL, 0);
    }
    
    virtual bool is_compatible(packet_t* other) {
        // enforce the OSP_SCAN policy (attempt to merge compatible
        // packets unless OSP_NONE prevents this from being called in
        // the first place)
        return packet_t::is_compatible(plan(), other->plan());
    }
};



/**
 *  @brief Table scan stage that reads tuples from the storage manager.
 */

class tscan_stage_t : public stage_t {

public:

    typedef tscan_packet_t stage_packet_t;
    static const c_str DEFAULT_STAGE_NAME;

    static const size_t TSCAN_BULK_READ_BUFFER_SIZE;

    tscan_stage_t() { }

    virtual ~tscan_stage_t() { }

protected:

    virtual void process_packet();
};



#endif
