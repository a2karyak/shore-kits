/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file shore_new_order_outside_loop.h
 *
 *  @brief Interface for the shore_new_order_update_warehouse stage,
 *  part of the new_order_staged transaction.
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __SHORE_TPCC_NEW_ORDER_OUTSIDE_LOOP_H
#define __SHORE_TPCC_NEW_ORDER_OUTSIDE_LOOP_H

#include <cstdio>

#include "core.h"
#include "util.h"

#include "core/trx_packet.h"
#include "stages/tpcc/shore/shore_tpcc_env.h"

using namespace qpipe;
using namespace shore;
using namespace tpcc;


/* exported datatypes */

class shore_new_order_outside_loop_packet_t : public trx_packet_t {
  
public:

    static const c_str PACKET_TYPE;

    /**
     *  @brief SHORE_NEW_ORDER_OUTSIDE_LOOP transaction inputs:
     *  
     *  1) WH_ID int: warehouse id
     *  2) H_AMOUNT long: new_order amount
     */
    
    new_order_input_t _noin;
    time_t _tstamp;


    /**
     *  @brief shore_new_order_outside_loop_packet_t constructor.
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
     *  @param wh_id The warehouse id
     *  
     *  @param h_amount The new_order amount
     */

    shore_new_order_outside_loop_packet_t(const c_str    &packet_id,
                                          tuple_fifo*     output_buffer,
                                          tuple_filter_t* output_filter,
                                          const int a_trx_id,
                                          const new_order_input_t* p_noin,
                                          const time_t a_tstamp)
        : trx_packet_t(packet_id, PACKET_TYPE, output_buffer, output_filter,
                       create_plan(a_trx_id, p_noin->_wh_id, p_noin->_d_id, 
                                   p_noin->_c_id, p_noin->_ol_cnt),
                       false, /* merging not allowed */
                       true,  /* unreserve worker on completion */
                       a_trx_id
                       ),
          _noin(*p_noin), _tstamp(a_tstamp)
    {
    }
    

    virtual ~shore_new_order_outside_loop_packet_t() { } 


    void describe_trx() {

        TRACE( TRACE_DEBUG,
               "\nOUTSIDE_LOOP - TRX_ID=%d\n" \
               "WH_ID=%d\t\tD_ID=%d\tC_ID=%d\tOL_CNT=%d\n", 
               _trx_id, 
               _noin._wh_id, _noin._d_id, _noin._c_id, _noin._ol_cnt);
    }

    
    // FIXME: (ip) Correct the plan creation
    static query_plan* create_plan( const int a_trx_id, 
                                    const int a_wh_id,
                                    const int a_d_id,
                                    const int a_c_id,
                                    const int a_ol_cnt)
      {
        c_str action("%s:%d:%d:%d:%d:%d", PACKET_TYPE.data(), 
		     a_trx_id, a_wh_id, a_d_id, a_c_id, a_ol_cnt);
        
        return new query_plan(action, "none", NULL, 0);
      }


    virtual void declare_worker_needs(resource_declare_t* declare) {
        declare->declare(_packet_type, 1);
        /* no inputs */
    }

}; // END OF CLASS: shore_new_order_outside_loop_packet_t



/**
 *  @brief SHORE_NEW_ORDER_OUTSIDE_LOOP stage. 
 *
 *  The row in the WAREHOUSE table with matching WH_ID is selected. 
 *  W_NAME, W_STREET_1, W_STREET_2, W_CITY, W_STATE and W_ZIP are retrieved,
 *  and W_YTD, the warehouse's year-to-date balance, is increased by H_AMOUNT.
 */

class shore_new_order_outside_loop_stage_t : public stage_t {

protected:

    virtual void process_packet();

public:

    static const c_str DEFAULT_STAGE_NAME;
    typedef shore_new_order_outside_loop_packet_t stage_packet_t;

    shore_new_order_outside_loop_stage_t();
    
    virtual ~shore_new_order_outside_loop_stage_t() { 
	TRACE(TRACE_DEBUG, "SHORE_NEW_ORDER_OUTSIDE_LOOP destructor\n");
    }
    
}; // END OF CLASS: shore_new_order_outside_loop_stage_t



#endif
