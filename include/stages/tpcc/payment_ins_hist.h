/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file payment_ins_hist.h
 *
 *  @brief Interface for the payment_insert_history stage,
 *  part of the payment_staged transaction.
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __TPCC_PAYMENT_INS_HIST_H
#define __TPCC_PAYMENT_INS_HIST_H

#include <cstdio>

#include "core.h"
#include "util.h"

#include "stages/tpcc/common/trx_packet.h"

using namespace qpipe;



/* exported datatypes */

class payment_ins_hist_packet_t : public trx_packet_t {
  
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

    int _wh_id;
    int _distr_id;
    int _cust_id;
    int _cust_wh_id;
    int _cust_distr_id;



    /**
     *  @brief payment_ins_hist_packet_t constructor.
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

    payment_ins_hist_packet_t(const c_str    &packet_id,
                              tuple_fifo*     output_buffer,
                              tuple_filter_t* output_filter,
                              const int a_trx_id,
                              const int a_wh_id,   
                              const int a_distr_id,
                              const int a_cust_id,
                              const int a_cust_wh_id,
                              const int a_cust_distr_id)
      : trx_packet_t(packet_id, PACKET_TYPE, output_buffer, output_filter,
                     create_plan(a_trx_id, a_wh_id, a_distr_id, 
                                 a_cust_id, a_cust_wh_id, a_cust_distr_id),
                     false, /* merging not allowed */
                     true,  /* unreserve worker on completion */
                     a_trx_id
                     ),
      _wh_id(a_wh_id),
      _distr_id(a_distr_id),
      _cust_id(a_cust_id),
      _cust_wh_id(a_cust_wh_id),
      _cust_distr_id(a_cust_distr_id)
      {
      }

        
    void describe_trx() {

        TRACE( TRACE_ALWAYS,
               "\nINS_HIST - TRX_ID=%d\n" \
               "WH_ID=%d\tD_ID=%d\tC_ID=%d\tC_WH_ID=%d\tC_D_ID=%d\n",
               _trx_id, 
               _wh_id, _distr_id, _cust_id, _cust_wh_id , _cust_distr_id);
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

}; // END OF CLASS: payment_ins_hist_packet_t



/**
 *  @brief PAYMENT_INS_HIST stage. 
 *
 *  A new row is inserted into the HISTORY table as
 *  (_C_ID, _C_D_ID, _C_WH_ID, _WH_ID, _D_ID)
 */

class payment_ins_hist_stage_t : public stage_t {

protected:

    virtual void process_packet();

public:

    static const c_str DEFAULT_STAGE_NAME;
    typedef payment_ins_hist_packet_t stage_packet_t;

    payment_ins_hist_stage_t();
    
    virtual ~payment_ins_hist_stage_t() { 

	TRACE(TRACE_ALWAYS, "PAYMENT_INS_HIST destructor\n");
    }
    
}; // END OF CLASS: payment_ins_hist_stage_t



#endif
