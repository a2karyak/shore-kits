/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file inmem_payment_upd_wh.h
 *
 *  @brief Interface for the inmem_payment_update_warehouse stage,
 *  part of the payment_staged transaction.
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __INMEM_TPCC_PAYMENT_UPD_WH_H
#define __INMEM_TPCC_PAYMENT_UPD_WH_H

#include <cstdio>

#include "core.h"
#include "util.h"

#include "stages/tpcc/common/trx_packet.h"

using namespace qpipe;


/* exported datatypes */

class inmem_payment_upd_wh_packet_t : public trx_packet_t {
  
public:

    static const c_str PACKET_TYPE;

    /**
     *  @brief INMEM_PAYMENT_UPD_WH transaction inputs:
     *  
     *  1) WH_ID int: warehouse id
     *  2) H_AMOUNT long: payment amount
     */

    int _wh_id;
    double _amount;



    /**
     *  @brief inmem_payment_upd_wh_packet_t constructor.
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
     *  @param h_amount The payment amount
     */

    inmem_payment_upd_wh_packet_t(const c_str    &packet_id,
                                  tuple_fifo*     output_buffer,
                                  tuple_filter_t* output_filter,
                                  const int a_trx_id,
                                  const int a_wh_id,   
                                  const double a_amount)
        : trx_packet_t(packet_id, PACKET_TYPE, output_buffer, output_filter,
                       create_plan(a_trx_id, a_wh_id, a_amount),
                       false, /* merging not allowed */
                       true,  /* unreserve worker on completion */
                       a_trx_id
                       ),
          _wh_id(a_wh_id),
          _amount(a_amount)
    {
    }
    

    virtual ~inmem_payment_upd_wh_packet_t() { } 


    void describe_trx() {

        TRACE( TRACE_DEBUG,
               "\nUPD_WH - TRX_ID=%d\n" \
               "WH_ID=%d\t\tAMOYNT=%.2f\n", 
               _trx_id, 
               _wh_id, _amount);
    }

    
    // FIXME: (ip) Correct the plan creation
    static query_plan* create_plan( const int a_trx_id, 
                                    const int a_wh_id,
                                    const double a_amount)
      {
        c_str action("%s:%d:%d:%f", PACKET_TYPE.data(), 
		     a_trx_id, a_wh_id, a_amount);
        
        return new query_plan(action, "none", NULL, 0);
      }


    virtual void declare_worker_needs(resource_declare_t* declare) {
        declare->declare(_packet_type, 1);
        /* no inputs */
    }

}; // END OF CLASS: inmem_payment_upd_wh_packet_t



/**
 *  @brief INMEM_PAYMENT_UPD_WH stage. 
 *
 *  The row in the WAREHOUSE table with matching WH_ID is selected. 
 *  W_NAME, W_STREET_1, W_STREET_2, W_CITY, W_STATE and W_ZIP are retrieved,
 *  and W_YTD, the warehouse's year-to-date balance, is increased by H_AMOUNT.
 */

class inmem_payment_upd_wh_stage_t : public stage_t {

protected:

    virtual void process_packet();

public:

    static const c_str DEFAULT_STAGE_NAME;
    typedef inmem_payment_upd_wh_packet_t stage_packet_t;

    inmem_payment_upd_wh_stage_t();
    
    virtual ~inmem_payment_upd_wh_stage_t() { 
	TRACE(TRACE_DEBUG, "INMEM_PAYMENT_UPD_WH destructor\n");
    }
    
}; // END OF CLASS: inmem_payment_upd_wh_stage_t



#endif
