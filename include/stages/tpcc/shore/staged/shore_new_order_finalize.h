/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file shore_new_order_finalize.h
 *
 *  @brief Interface for the Shore TPC-C Finalize stage
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __SHORE_TPCC_NEW_ORDER_FINALIZE_H
#define __SHORE_TPCC_NEW_ORDER_FINALIZE_H

#include <cstdio>

#include "core.h"
#include "util.h"
#include "scheduler.h"

#include "stages/tpcc/common/trx_packet.h"

#include "stages/tpcc/shore/staged/shore_new_order_outside_loop.h"
#include "stages/tpcc/shore/staged/shore_new_order_one_ol.h"


using namespace qpipe;



/* exported datatypes */

class shore_new_order_finalize_packet_t : public trx_packet_t {
  
public:

    static const c_str PACKET_TYPE;


    // Removed the guards

    // number of ol
    const int _ol_cnt;

    // The input packets
    shore_new_order_outside_loop_packet_t* _outside;
    shore_new_order_one_ol_packet_t* _list_one_ol[MAX_OL_PER_ORDER];

    // The input buffers
    tuple_fifo* _outside_buffer;
    tuple_fifo* _list_one_ol_buffer[MAX_OL_PER_ORDER];
    

    /**
     *  @brief shore_new_order_finalize_packet_t constructor.
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
     *  @param trx_id The transaction id
     *
     *  @param The four packets that make the new_order transaction
     */

    shore_new_order_finalize_packet_t(const c_str    &packet_id,
                                      tuple_fifo*     output_buffer,
                                      tuple_filter_t* output_filter,
                                      const int a_trx_id,
                                      trx_packet_t* outside_loop,
                                      trx_packet_t** ol_packets,
                                      int ol_cnt)
	: trx_packet_t(packet_id, PACKET_TYPE, output_buffer, output_filter,
                       create_plan(a_trx_id),
                       false, /* merging disallowed */
                       true,  /* unreserve worker on completion */
                       a_trx_id
                       ),
          _outside((shore_new_order_outside_loop_packet_t*)outside_loop),
          _outside_buffer(outside_loop->output_buffer()),
          _ol_cnt(ol_cnt)
    {
        /* asserts on uninitiliazed input packets */
        assert(_outside != NULL);
        /* asserts on NULL input buffers */
        assert(_outside_buffer != NULL);

        // read-in the ol packets
        assert (ol_packets);
        assert ((_ol_cnt>=0) && (_ol_cnt<= MAX_OL_PER_ORDER));
        for (int i=0; i<_ol_cnt; i++) {
            _list_one_ol[i] = (shore_new_order_one_ol_packet_t*)ol_packets[i];
            _list_one_ol_buffer[i] = ol_packets[i]->output_buffer();
            assert (_list_one_ol[i]);
            assert (_list_one_ol_buffer[i]);
        }
    }


    virtual ~shore_new_order_finalize_packet_t() { }

    void describe_trx() {
        TRACE( TRACE_DEBUG,
               "\nFINALIZE - TRX_ID=%d\n",
               _trx_id);
    }

    // FIXME: (ip) Correct the plan creation
    static query_plan* create_plan( const int a_trx_id) 
    {
        c_str action("%s:%d", PACKET_TYPE.data(), a_trx_id);

        return new query_plan(action, "none", NULL, 0);
    }


    virtual void declare_worker_needs(resource_declare_t* declare) 
    {        
        /* declares inputs */
        _outside->declare_worker_needs(declare);
        for (int i=0; i<_ol_cnt; i++)
            _list_one_ol[i]->declare_worker_needs(declare);

        /* declare own needs */
        declare->declare(_packet_type, 1);
    }


    /** @brief Calls for this transaction to rollback */
    
    void rollback() {
        
        TRACE( TRACE_TRX_FLOW, "~~~ Should Rollback TRX=%d ~~~\n", _trx_id);
    }


    /** @brief Calls for this transaction to commit */

    void commit() {

        TRACE( TRACE_TRX_FLOW, "~~~ Should Commit: TRX=%d ~~~~\n", _trx_id);
    }

}; // END OF CLASS: shore_new_order_finalize_packet_t



/**
 *  @brief SHORE_NEW_ORDER_FINALIZE stage. 
 *
 *  The stage at the end of each SHORE_NEW_ORDER transaction. It is the one which
 *  coordinates the commit/rollback between the parallel executing parts.
 */

class shore_new_order_finalize_stage_t : public stage_t 
{
protected:

    virtual void process_packet();

public:

    static const c_str DEFAULT_STAGE_NAME;
    typedef shore_new_order_finalize_packet_t stage_packet_t;

    shore_new_order_finalize_stage_t();
    
    virtual ~shore_new_order_finalize_stage_t() { 
	TRACE(TRACE_DEBUG, "SHORE_NEW_ORDER_FINALIZE destructor\n");	
    }
    
}; // END OF CLASS: shore_new_order_finalize_stage_t



#endif
