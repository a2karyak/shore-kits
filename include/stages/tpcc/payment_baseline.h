/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file payment_baseline.h
 *
 *  @brief Interface for the Baseline TPC-C Payment transaction.
 *  The Baseline implementation uses a single thread for the entire
 *  transaction. We wrap the code to a stage in order to use the
 *  same subsystem
 *
 *  @author Ippokratis Pandis (ipandis)
 */


#ifndef __TPCC_PAYMENT_BASELINE_H
#define __TPCC_PAYMENT_BASELINE_H


#include <cstdio>

#include "core.h"
#include "util.h"
#include "scheduler.h"

#include "stages/tpcc/common/trx_packet.h"
#include "stages/tpcc/common/payment_functions.h"


using namespace qpipe;
using namespace tpcc_payment;



/* exported datatypes */

class payment_baseline_packet_t : public trx_packet_t {

public:

    static const c_str PACKET_TYPE;

    // structure that contains the required input
    payment_input_t _p_in;

    // pointer to placeholders for the Dbts
    s_payment_dbt_t* _p_dbts;


    /**
     *  @brief payment_baseline_packet_t constructor.
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
     *  @param All the PAYMENT transaction input variables
     */

    payment_baseline_packet_t(const c_str    &packet_id,
                              tuple_fifo*     output_buffer,
                              tuple_filter_t* output_filter,
                              const int a_home_wh_id,   
                              const int a_home_d_id,
                              const int a_v_cust_wh_selection,
                              const int a_remote_wh_id,
                              const int a_remote_d_id,
                              const int a_v_cust_ident_selection,
                              const int a_c_id,
                              const char a_c_last[16],
                              const double a_h_amount,
                              const int a_h_date,
                              s_payment_dbt_t* a_p_dbts)
      : trx_packet_t(packet_id, PACKET_TYPE, output_buffer, output_filter,
                     create_plan(a_c_id, a_h_amount, a_h_date),
                     true, /* merging allowed */
                     true  /* unreserve worker on completion */
                     )
    {
        // take pointer to allocated Dbts
        assert (a_p_dbts);
        _p_dbts = a_p_dbts;
            

        // copy input
        _p_in._home_wh_id = a_home_wh_id;
        _p_in._home_d_id = a_home_d_id;
        _p_in._v_cust_wh_selection = a_v_cust_wh_selection;
        _p_in._remote_wh_id = a_remote_wh_id;
        _p_in._remote_d_id = a_remote_d_id;
        _p_in._v_cust_ident_selection = a_v_cust_ident_selection;
        _p_in._c_id = a_c_id;
        _p_in._h_amount = a_h_amount;

        if (a_c_last) {
        
            strncpy(_p_in._c_last, a_c_last, 14);
            _p_in._c_last[15] = '\0';
        }

        _trx_state = UNDEF;
    }

    
    // FIXME: (ip) Correct the plan creation
    static query_plan* create_plan( const int a_c_id,
                                    const double a_h_amount,
                                    const int a_h_date) 
      {
        c_str action("%s:%d:%f:%d", 
                     PACKET_TYPE.data(), 
		     a_c_id, 
                     a_h_amount, 
                     a_h_date);
        
        return new query_plan(action, "none", NULL, 0);
      }

    
    virtual void declare_worker_needs(resource_declare_t* declare) {
      declare->declare(_packet_type, 1);
      /* no inputs */
    }


    /** Helper Functions */

    void describe_trx() {
        
        describe(_p_in, _trx_id);
    }


}; // EOF payment_baseline_packet_t



/** 
 *  @brief PAYMENT_BASELINE stage. 
 *
 *  @desc Executes the entire tpcc payment transaction in a conventional,
 *  single-threaded fashion.
 */

class payment_baseline_stage_t : public stage_t {

protected:

    virtual void process_packet();

public:

    static const c_str DEFAULT_STAGE_NAME;
    typedef payment_baseline_packet_t stage_packet_t;

    payment_baseline_stage_t();
 
    ~payment_baseline_stage_t() { 
	TRACE(TRACE_DEBUG, "PAYMENT_BASELINE destructor\n");	
    }        

private:

    trx_result_tuple_t executePaymentBaseline(payment_input_t* pin, 
                                              DbTxn* txn, 
                                              const int id, 
                                              s_payment_dbt_t* a_p_dbts);
    
    
}; // EOF: payment_baseline_stage_t



#endif
