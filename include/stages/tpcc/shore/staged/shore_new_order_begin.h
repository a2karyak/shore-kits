/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file shore_new_order_begin.h
 *
 *  @brief Interface for the Shore TPC-C Begin stage
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __SHORE_TPCC_NEW_ORDER_BEGIN_H
#define __SHORE_TPCC_NEW_ORDER_BEGIN_H

#include <cstdio>

#include "core.h"
#include "util.h"
#include "scheduler.h"

#include "stages/tpcc/common/trx_packet.h"
#include "stages/tpcc/shore/shore_tpcc_env.h"

#include "stages/tpcc/shore/staged/shore_new_order_outside_loop.h"
#include "stages/tpcc/shore/staged/shore_new_order_one_ol.h"
#include "stages/tpcc/shore/staged/shore_new_order_finalize.h"

using namespace qpipe;
using namespace shore;
using namespace tpcc;


/* exported datatypes */

class shore_new_order_begin_packet_t : public trx_packet_t {
  
public:

    static const c_str PACKET_TYPE;

    // structure that contains the required input
    new_order_input_t _no_in;


    /**
     *  @brief shore_new_order_begin_packet_t constructor.
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
     *  @param All the NEW_ORDER transaction input variables
     */

    shore_new_order_begin_packet_t(const c_str    &packet_id,
                                   tuple_fifo*     output_buffer,
                                   tuple_filter_t* output_filter,
                                   const new_order_input_t a_no_input)
        : trx_packet_t(packet_id, PACKET_TYPE, output_buffer, output_filter,
                       create_plan(a_no_input._wh_id, a_no_input._d_id, 
                                   a_no_input._ol_cnt),
                       false, /* merging disallowed */
                       true  /* unreserve worker on completion */
                       ),
          _no_in(a_no_input)
    {
        _trx_state = UNDEF;
    }


    virtual ~shore_new_order_begin_packet_t() { }
    

    // FIXME: (ip) Correct the plan creation
    static query_plan* create_plan( const int a_wh_id,
                                    const int a_d_id,
                                    const int a_ol_cnt) 
    {
        c_str action("%s:%d:%d:%d", PACKET_TYPE.data(), 
		     a_wh_id, a_d_id, a_ol_cnt);

        return new query_plan(action, "none", NULL, 0);
    }


    virtual void declare_worker_needs(resource_declare_t* declare) {
        declare->declare(_packet_type, 1);
        /* no inputs */
    }


    /** Helper Functions */

    void describe_trx() {        
        _no_in.describe(_trx_id);
    }


}; // EOF new_order_begin_packet_t



/**
 *  @brief SHORE_NEW_ORDER_BEGIN stage. 
 *
 *  1) Assigns a unique id to the submitted NEW_ORDER transaction
 *
 *  2) Submits the appropriate packets to their stages (NEW_ORDER_UPD_CUST, 
 *  NEW_ORDER_UPD_WH, NEW_ORDER_UPD_DISTR and NEW_ORDER_INS_HIST) and a transaction
 *  finalization packet (NEW_ORDER_FINALIZE)
 */

class shore_new_order_begin_stage_t : public stage_t 
{
protected:

    virtual void process_packet();

    static int _trx_counter;
    static tatas_lock _trx_counter_lock;
    
public:

    static const c_str DEFAULT_STAGE_NAME;
    typedef shore_new_order_begin_packet_t stage_packet_t;

    shore_new_order_begin_stage_t();
 
    ~shore_new_order_begin_stage_t() { 
	TRACE(TRACE_DEBUG, "SHORE_NEW_ORDER_BEGIN destructor\n");	
    }

    /** @brief Returns the next trx_id */
    int get_next_counter();
    

    /** @brief Returns an shore_new_order_outside_loop_packet_t */
    trx_packet_t* 
    create_shore_new_order_outside_loop_packet(const c_str& client_prefix,
                                               tuple_fifo* iout_buffer,
                                               tuple_filter_t* iout_filter,
                                               int a_trx_id,
                                               new_order_input_t* p_noin);

    /** @brief Returns an shore_new_order_one_ol_packet_t */
    trx_packet_t* 
    create_shore_new_order_one_ol_packet(const c_str& client_prefix,
                                         tuple_fifo* iol_buffer,
                                         tuple_filter_t* iol_filter,
                                         int a_trx_id,
                                         ol_item_info* p_nolin);
    

    /** @brief Returns an shore_new_order_finalize_packet_t */
    trx_packet_t* 
    create_shore_new_order_finalize_packet(const c_str& client_prefix,
                                           tuple_fifo* fin_buffer,
                                           tuple_filter_t* fin_filter,
                                           int a_trx_id,
                                           trx_packet_t* outside_loop,
                                           trx_packet_t** list_ol_packets,
                                           int ol_cnt);
    
    
    
}; // EOF shore_new_order_begin_stage_t



#endif
