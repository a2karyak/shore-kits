/* -*- mode:C++; c-basic-offset:4 -*- */
#ifndef _ISCAN_H
#define _ISCAN_H

#include "db_cxx.h"
#include "engine/core/tuple.h"
#include "engine/core/packet.h"
#include "engine/core/stage.h"

using namespace qpipe;



/**
 *  @brief Packet definition for the ISCAN stage.
 */

struct iscan_packet_t : public packet_t {

public:

    static const c_str PACKET_TYPE;

    typedef int (*bt_compare_func_t)(Db*, const Dbt*, const Dbt*);
    Db* _db;
    
    dbt_guard_t _start_key;
    dbt_guard_t _stop_key;
    bt_compare_func_t _bt_compare;

   
    /**
     *  @brief iscan_packet_t constructor.
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
    iscan_packet_t(const c_str    &packet_id,
		   tuple_buffer_t* output_buffer,
		   tuple_filter_t* output_filter,
		   Db* db,
                   dbt_guard_t start_key,
                   dbt_guard_t stop_key,
                   bt_compare_func_t bt_compare)
	: packet_t(packet_id, PACKET_TYPE, output_buffer, output_filter, false),
	  _db(db), _start_key(start_key), _stop_key(stop_key), _bt_compare(bt_compare)
    {
        assert(db != NULL);
        
        // did the user pass in two guards pointing to the same array?
        assert(start_key->get_data() != stop_key->get_data());
        assert(bt_compare != NULL);
    }
    
    virtual bool is_compatible(packet_t*) {
        // TODO: detect overlapping/dominating ranges
        return false;
    }

};



/**
 *  @brief Table scan stage that reads tuples from the storage manager.
 */

class iscan_stage_t : public stage_t {

public:

    typedef iscan_packet_t stage_packet_t;
    static const c_str DEFAULT_STAGE_NAME;

    static const size_t ISCAN_BULK_READ_BUFFER_SIZE;

    iscan_stage_t() { }

    virtual ~iscan_stage_t() { }

protected:

    virtual void process_packet();
};



#endif
