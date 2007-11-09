/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file inmem_payment_begin.h
 *
 *  @brief Interface for the InMemory TPC-C Begin stage
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __INMEM_TPCC_PAYMENT_BEGIN_H
#define __INMEM_TPCC_PAYMENT_BEGIN_H

#include <cstdio>

#include "core.h"
#include "util.h"
#include "scheduler.h"

#include "stages/tpcc/common/trx_packet.h"
#include "stages/tpcc/inmem/inmem_payment_functions.h"

#include "stages/tpcc/inmem/inmem_payment_upd_wh.h"
#include "stages/tpcc/inmem/inmem_payment_upd_distr.h"
#include "stages/tpcc/inmem/inmem_payment_upd_cust.h"
#include "stages/tpcc/inmem/inmem_payment_ins_hist.h"
#include "stages/tpcc/inmem/inmem_payment_finalize.h"

using namespace qpipe;
using namespace tpcc_payment;


/* exported datatypes */

class inmem_payment_begin_packet_t : public trx_packet_t {
  
public:

    static const c_str PACKET_TYPE;

    // structure that contains the required input
    payment_input_t _p_in;


    /**
     *  @brief inmem_payment_begin_packet_t constructor.
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

    inmem_payment_begin_packet_t(const c_str    &packet_id,
                                 tuple_fifo*     output_buffer,
                                 tuple_filter_t* output_filter,
                                 const payment_input_t a_p_input)
        : trx_packet_t(packet_id, PACKET_TYPE, output_buffer, output_filter,
                       create_plan(a_p_input._c_id, a_p_input._h_amount, a_p_input._h_date),
                       false, /* merging disallowed */
                       true  /* unreserve worker on completion */
                       )
    {
        // Copy input
        _p_in = a_p_input;

        // Set transaction state
        _trx_state = UNDEF;
    }


    virtual ~inmem_payment_begin_packet_t() { }
    

    // FIXME: (ip) Correct the plan creation
    static query_plan* create_plan( const int a_c_id,
                                    const double a_h_amount,
                                    const int a_h_date) 
    {
        c_str action("%s:%d:%f:%d", PACKET_TYPE.data(), 
		     a_c_id, a_h_amount, a_h_date);

        return new query_plan(action, "none", NULL, 0);
    }


    virtual void declare_worker_needs(resource_declare_t* declare) {
        declare->declare(_packet_type, 1);
        /* no inputs */
    }


    /** Helper Functions */

    void describe_trx() {        
        _p_in.describe(_trx_id);
    }


}; // EOF payment_begin_packet_t



/**
 *  @brief INMEM_PAYMENT_BEGIN stage. 
 *
 *  1) Assigns a unique id to the submitted PAYMENT transaction
 *
 *  2) Submits the appropriate packets to their stages (PAYMENT_UPD_CUST, 
 *  PAYMENT_UPD_WH, PAYMENT_UPD_DISTR and PAYMENT_INS_HIST) and a transaction
 *  finalization packet (PAYMENT_FINALIZE)
 */

class inmem_payment_begin_stage_t : public stage_t {

protected:

    virtual void process_packet();

    static int _trx_counter;
    static pthread_mutex_t _trx_counter_mutex;
    
public:

    static const c_str DEFAULT_STAGE_NAME;
    typedef inmem_payment_begin_stage_t stage_packet_t;

    inmem_payment_begin_stage_t();
 
    ~inmem_payment_begin_stage_t() { 
	TRACE(TRACE_DEBUG, "INMEM_PAYMENT_BEGIN destructor\n");	
    }

    /** @brief Returns the next trx_id */
    int get_next_counter();

    /** @brief Retuns a inmem_payment_upd_wh_packet_t */
    trx_packet_t* 
    create_inmem_payment_upd_wh_packet(const c_str& client_prefix,
                                       tuple_fifo* uwh_buffer,
                                       tuple_filter_t* uwh_filter,
                                       int a_trx_id,
                                       int a_wh_id,
                                       double a_amount);
    

    /** @brief Returns a inmem_payment_upd_distr_packet_t */
    trx_packet_t* 
    create_inmem_payment_upd_distr_packet(const c_str& client_prefix,
                                          tuple_fifo* ud_buffer,
                                          tuple_filter_t* ud_filter,
                                          int a_trx_id,
                                          int a_wh_id,
                                          int a_distr_id,
                                          double a_amount);
    

    /** @brief Returns a inmem_payment_upd_cust_packet_t */
    trx_packet_t* 
    create_inmem_payment_upd_cust_packet(const c_str& client_prefix,
                                         tuple_fifo* uc_buffer,
                                         tuple_filter_t* uc_filter,
                                         int a_trx_id,
                                         int a_wh_id,
                                         int a_distr_id,
                                         int a_cust_id,
                                         char* a_cust_last,
                                         double a_amount);
    

    /** @brief Returns an inmem_payment_ins_hist_packet_t */
    trx_packet_t* 
    create_inmem_payment_ins_hist_packet(const c_str& client_prefix,
                                         tuple_fifo* ih_buffer,
                                         tuple_filter_t* ih_filter,
                                         int a_trx_id,
                                         int a_wh_id,
                                         int a_distr_id,
                                         int a_cust_id,
                                         int a_cust_wh_id,
                                         int a_cust_distr_id);
    

    /** @brief Returns an inmem_payment_finalize_packet_t */
    trx_packet_t* 
    create_inmem_payment_finalize_packet(const c_str& client_prefix,
                                         tuple_fifo* fin_buffer,
                                         tuple_filter_t* fin_filter,
                                         int a_trx_id,
                                         trx_packet_t* upd_wh,
                                         trx_packet_t* upd_distr,
                                         trx_packet_t* upd_cust,
                                         trx_packet_t* ins_hist);
    
    
    
}; // EOF inmem_payment_begin_stage_t



#endif
