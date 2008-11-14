/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file shore_delivery_baseline.h
 *
 *  @brief Interface for the Shore Baseline TPC-C Delivery transaction.
 *  The Baseline implementation uses a single thread for the entire
 *  transaction. We wrap the code to a stage in order to use the
 *  same subsystem
 *
 *  @author Ippokratis Pandis (ipandis)
 */


#ifndef __SHORE_TPCC_DELIVERY_BASELINE_H
#define __SHORE_TPCC_DELIVERY_BASELINE_H

#include <cstdio>

#include "core.h"
#include "util.h"
#include "scheduler.h"

#include "core/trx_packet.h"

#include "stages/tpcc/common/tpcc_input.h"
#include "workload/tpcc/shore_tpcc_env.h"


using namespace qpipe;
using namespace tpcc;


/* exported datatypes */

class shore_delivery_baseline_packet_t : public trx_packet_t {

public:

    static const c_str PACKET_TYPE;

    // structure that contains the required input
    delivery_input_t _d_in;


    /**
     *  @brief shore_delivery_baseline_packet_t constructor.
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
     *  @param All the DELIVERY transaction input variables
     */

    shore_delivery_baseline_packet_t(const c_str    &packet_id,
                                     tuple_fifo*     output_buffer,
                                     tuple_filter_t* output_filter,
                                     const delivery_input_t a_d_input)
        : trx_packet_t(packet_id, PACKET_TYPE, output_buffer, output_filter,
                       create_plan(a_d_input._wh_id, a_d_input._carrier_id),
                       false, /* merging allowed */
                       true  /* unreserve worker on completion */
                       ),
          _d_in(a_d_input) /* copy input */          
    {
        _trx_state = UNDEF;
    }


    virtual ~shore_delivery_baseline_packet_t() { }

    
    // FIXME: (ip) Correct the plan creation
    static query_plan* create_plan(const short a_wh_id,
                                   const short a_carrier) 
      {
        c_str action("%s:%d:%d", 
                     PACKET_TYPE.data(), 
		     a_wh_id, 
                     a_carrier);
        
        return new query_plan(action, "none", NULL, 0);
      }

    
    virtual void declare_worker_needs(resource_declare_t* declare) {
      declare->declare(_packet_type, 1);
      /* no inputs */
    }


    /** Helper Functions */

    void describe_trx() {        
        _d_in.describe(_trx_id);
    }

}; // EOF shore_delivery_baseline_packet_t



/** 
 *  @brief SHORE_DELIVERY_BASELINE stage. 
 *
 *  @desc Executes the entire tpcc delivery transaction in a conventional,
 *  single-threaded fashion, using the Shoreory data structures.
 */

class shore_delivery_baseline_stage_t : public stage_t {

protected:

    virtual void process_packet();

public:

    static const c_str DEFAULT_STAGE_NAME;
    typedef shore_delivery_baseline_packet_t stage_packet_t;

    shore_delivery_baseline_stage_t();
 
    ~shore_delivery_baseline_stage_t() { 
	TRACE(TRACE_DEBUG, "SHORE_DELIVERY_BASELINE destructor\n");	
    }        
    
}; // EOF: shore_delivery_baseline_stage_t


#endif
