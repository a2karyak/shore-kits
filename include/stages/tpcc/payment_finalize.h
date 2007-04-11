/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _PAYMENT_FINALIZE_H
#define _PAYMENT_FINALIZE_H

#include <cstdio>

#include "stages/tpcc/trx_packet.h"
#include "core.h"
#include "util.h"


using namespace qpipe;



/* exported datatypes */

class payment_finalize_packet_t : public trx_packet_t {
  
public:

    static const c_str PACKET_TYPE;


    /**
     *  @brief payment_finalize_packet_t constructor.
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
     */

    payment_finalize_packet_t(const c_str    &packet_id,
                              tuple_fifo*     output_buffer,
                              tuple_filter_t* output_filter,
                              const int a_trx_id)
	: trx_packet_t(packet_id, PACKET_TYPE, output_buffer, output_filter,
                       create_plan(a_trx_id),
                       true, /* merging allowed */
                       true,  /* unreserve worker on completion */
                       a_trx_id
                       )
    {
    }

    
    // Destructor
    ~payment_finalize_packet_t() { }


    void describe_trx() {

        TRACE( TRACE_ALWAYS,
               "\nFINALIZE - TRX_ID=%d\n",
               _trx_id);
    }

    // FIXME: (ip) Correct the plan creation
    static query_plan* create_plan( const int a_trx_id) 
    {
        c_str action("%s:%d", PACKET_TYPE.data(), a_trx_id);

        return new query_plan(action, "none", NULL, 0);
    }


    virtual void declare_worker_needs(resource_declare_t* declare) {
        declare->declare(_packet_type, 1);
        /* no inputs */
    }

}; // END OF CLASS: payment_finalize_packet_t



/**
 *  @brief PAYMENT_FINALIZE stage. 
 *
 *  The stage at the end of each PAYMENT transaction. It is the one which
 *  coordinates the commit/rollback between the parallel executing parts.
 */

class payment_finalize_stage_t : public stage_t {

protected:

    virtual void process_packet();

public:

    static const c_str DEFAULT_STAGE_NAME;
    typedef payment_finalize_packet_t stage_packet_t;

    payment_finalize_stage_t();
    
    virtual ~payment_finalize_stage_t() { 

	TRACE(TRACE_ALWAYS, "PAYMENT_FINALIZE destructor\n");	
    }
    
}; // END OF CLASS: payment_finalize_stage_t



#endif
