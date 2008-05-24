/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file shore_payment_upd_distr.h
 *
 *  @brief Interface for the shore_payment_update_district stage,
 *  part of the payment_staged transaction.
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __SHORE_TPCC_PAYMENT_UPD_DISTR_H
#define __SHORE_TPCC_PAYMENT_UPD_DISTR_H

#include <cstdio>

#include "core.h"
#include "util.h"

#include "core/trx_packet.h"

#include "stages/tpcc/shore/shore_tpcc_env.h"

using namespace qpipe;
using namespace shore;
using namespace tpcc;


/* exported datatypes */

class shore_payment_upd_distr_packet_t : public trx_packet_t {
  
public:

    static const c_str PACKET_TYPE;

    /**
     *  @brief SHORE_PAYMENT_UPD_WH transaction inputs:
     *  
     *  1) WH_ID int: warehouse id
     *  2) DISTR_ID int: district id
     *  3) H_AMOUNT long: payment amount
     */

    payment_input_t _pin;



    /**
     *  @brief shore_payment_upd_distr_packet_t constructor.
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
     *  @param distr_id The district id
     *  @param wh_id The warehouse id
     *  
     *  @param h_amount The payment amount
     */

    shore_payment_upd_distr_packet_t(const c_str    &packet_id,
                                     tuple_fifo*     output_buffer,
                                     tuple_filter_t* output_filter,
                                     const int a_trx_id,
                                     const payment_input_t* p_pin)
        : trx_packet_t(packet_id, PACKET_TYPE, output_buffer, output_filter,
                       create_plan(a_trx_id, p_pin->_home_wh_id, 
                                   p_pin->_home_d_id, p_pin->_h_amount),
                       false, /* merging not allowed */
                       true,  /* unreserve worker on completion */
                       a_trx_id
                       ),
          _pin(*p_pin)
    {
    }


    virtual ~shore_payment_upd_distr_packet_t() { }


    void describe_trx() {
        TRACE( TRACE_DEBUG,
               "\nUPD_DISTR - TRX_ID=%d\n" \
               "WH_ID=%d\t\tDISTRICT=%d\tAMOYNT=%.2f\n", 
               _trx_id, 
               _pin._home_wh_id, _pin._home_d_id, _pin._h_amount);
    }

    
    // FIXME: (ip) Correct the plan creation
    static query_plan* create_plan( const int a_trx_id, 
                                    const int a_wh_id,
                                    const int a_distr_id,
                                    const double a_amount)
      {
        c_str action("%s:%d:%d:%d:%f", PACKET_TYPE.data(), 
		     a_trx_id, a_wh_id, a_distr_id, a_amount);
        
        return new query_plan(action, "none", NULL, 0);
      }


    virtual void declare_worker_needs(resource_declare_t* declare) {
        declare->declare(_packet_type, 1);
        /* no inputs */
    }

}; // END OF CLASS: shore_payment_upd_distr_packet_t



/**
 *  @brief SHORE_PAYMENT_UPD_DISTR stage. 
 *
 *  The row in the DISTRICT table with matching D_W_ID and D_ID is selected.
 *  D_NAME, D_STREET_1, D_STREET_2, D_CITY, D_STATE, and D_ZIP are retrieved,
 *  and D_YTD, the district's year-to-date balance, is increased by H_AMOUNT
 */

class shore_payment_upd_distr_stage_t : public stage_t {

protected:

    virtual void process_packet();

public:

    static const c_str DEFAULT_STAGE_NAME;
    typedef shore_payment_upd_distr_packet_t stage_packet_t;

    shore_payment_upd_distr_stage_t();
    
    virtual ~shore_payment_upd_distr_stage_t() { 
	TRACE(TRACE_DEBUG, "SHORE_PAYMENT_UPD_DISTR destructor\n");
    }
    
}; // END OF CLASS: shore_payment_upd_distr_stage_t



#endif
