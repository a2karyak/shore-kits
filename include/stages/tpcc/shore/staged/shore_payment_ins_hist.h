/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file inmem_payment_ins_hist.h
 *
 *  @brief Interface for the inmem_payment_insert_history stage,
 *  part of the payment_staged transaction.
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __INMEM_TPCC_PAYMENT_INS_HIST_H
#define __INMEM_TPCC_PAYMENT_INS_HIST_H

#include <cstdio>

#include "core.h"
#include "util.h"

#include "stages/tpcc/common/trx_packet.h"
#include "stages/tpcc/inmem/inmem_payment_functions.h"


using namespace qpipe;


/* exported datatypes */

class inmem_payment_ins_hist_packet_t : public trx_packet_t {
  
public:

    static const c_str PACKET_TYPE;

    /**
     *  @brief PAYMENT_UPD_WH transaction inputs:
     *  
     *  1) WH_ID int: warehouse id
     *  2) DISTR_ID int: district id
     *  3) CUST_ID int: customer id
     *  4) CUST_LAST char*: customer lastname
     *  5) H_AMOUNT long: payment amount
     */

    payment_input_t _pin;


    /**
     *  @brief inmem_payment_ins_hist_packet_t constructor.
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
     *  @param distr_id The district id
     *  @param cust_id The customer id
     *  @param cust_wh_id The customer warehouse id
     *  @param cust_distr_id The customer district id
     */

    inmem_payment_ins_hist_packet_t(const c_str    &packet_id,
                                    tuple_fifo*     output_buffer,
                                    tuple_filter_t* output_filter,
                                    const int a_trx_id,
                                    const payment_input_t* p_pin)
        : trx_packet_t(packet_id, PACKET_TYPE, output_buffer, output_filter,
                       create_plan(a_trx_id, p_pin->_home_wh_id, p_pin->_home_d_id, 
                                   p_pin->_c_id, p_pin->_home_wh_id, p_pin->_home_d_id),
                       false, /* merging not allowed */
                       true,  /* unreserve worker on completion */
                       a_trx_id
                       ),
          _pin(*p_pin)
    {
    }


    virtual ~inmem_payment_ins_hist_packet_t()  { }

        
    void describe_trx() {
        TRACE( TRACE_DEBUG,
               "\nINS_HIST - TRX_ID=%d\n" \
               "WH_ID=%d\tD_ID=%d\tC_ID=%d\tC_WH_ID=%d\tC_D_ID=%d\n",
               _trx_id, 
               _pin._home_wh_id, _pin._home_d_id, _pin._c_id, 
               _pin._home_wh_id , _pin._home_d_id);
    }

    
    // FIXME: (ip) Correct the plan creation
    static query_plan* create_plan( const int a_trx_id, 
                                    const int a_wh_id,
                                    const int a_distr_id,
                                    const int a_cust_id,
                                    const int a_cust_wh_id,
                                    const int a_cust_d_id)
      {
        c_str action("%s:%d:%d:%d:%d:%d", PACKET_TYPE.data(), 
		     a_trx_id, a_wh_id, a_distr_id, a_cust_id, 
                     a_cust_wh_id, a_cust_d_id );
        
        return new query_plan(action, "none", NULL, 0);
      }


    virtual void declare_worker_needs(resource_declare_t* declare) {
        declare->declare(_packet_type, 1);
        /* no inputs */
    }

}; // END OF CLASS: inmem_payment_ins_hist_packet_t



/**
 *  @brief INMEM_PAYMENT_INS_HIST stage. 
 *
 *  A new row is inserted into the HISTORY table as
 *  (_C_ID, _C_D_ID, _C_WH_ID, _WH_ID, _D_ID)
 */

class inmem_payment_ins_hist_stage_t : public stage_t {

protected:

    virtual void process_packet();

public:

    static const c_str DEFAULT_STAGE_NAME;
    typedef inmem_payment_ins_hist_packet_t stage_packet_t;

    inmem_payment_ins_hist_stage_t();
    
    virtual ~inmem_payment_ins_hist_stage_t() { 
	TRACE(TRACE_DEBUG, "INMEM_PAYMENT_INS_HIST destructor\n");
    }
    
}; // END OF CLASS: inmem_payment_ins_hist_stage_t



#endif
