/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file shore_payment_begin.cpp
 *
 *  @brief Implementation of the tpc-c shore_payment_begin stage
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#include "stages/tpcc/shore/staged/shore_payment_begin.h"
#include "workload/common.h"



const c_str shore_payment_begin_packet_t::PACKET_TYPE = "SHORE_PAYMENT_BEGIN";

const c_str shore_payment_begin_stage_t::DEFAULT_STAGE_NAME = "SHORE_PAYMENT_BEGIN_STAGE";


int shore_payment_begin_stage_t::_trx_counter = 0;
tatas_lock shore_payment_begin_stage_t::_trx_counter_lock;


/**
 *  @brief Class for processing the results of the shore_payment_begin
 */

class shore_payment_begin_process_tuple_t : public process_tuple_t {

public:

    void begin() {
        TRACE( TRACE_DEBUG, "*** SHORE_PAYMENT_BEGIN RESULTS\n" );
    }

    void process(const tuple_t& output) {
        trx_result_tuple_t* r = aligned_cast<trx_result_tuple_t>(output.data);
        TRACE(TRACE_QUERY_RESULTS, "*** SHORE_PAYMENT_BEGIN TRX=%d RESULT=%s\n",
              r->get_id(),
              (r->get_state() == COMMITTED) ? "OK" : "ABORT");
        
    }        
};


/**
 *  @brief shore_payment_begin_stage_t constructor
 */

shore_payment_begin_stage_t::shore_payment_begin_stage_t() {    
    //    TRACE(TRACE_DEBUG, "SHORE_PAYMENT_BEGIN constructor\n");
}


/**
 *  @brief Initialize transaction specified by shore_payment_begin_packet_t p.
 *
 *  @return void
 *
 *  @throw May throw exceptions on error.
 */

void shore_payment_begin_stage_t::process_packet() {

    adaptor_t* adaptor = _adaptor;

    shore_payment_begin_packet_t* packet = 
	(shore_payment_begin_packet_t*)adaptor->get_packet();

    /* STEP 1: Assigns a unique id */
    int my_trx_id;
    my_trx_id = get_next_counter();
    packet->set_trx_id(my_trx_id);

    /* Prints out the packet info */
    //    packet->describe_trx();

    /* STEP 2: Submits appropriate shore_payment_* packets */
    
    /** FIXME: (ip) Scheduler policy?
     *  by default is set to OS  */
    scheduler::policy_t* dp = new scheduler::policy_os_t();

    /** @note The output buffers of all the non-finalize stages, which is
     *  the input to the finalize stage, are simple integers 
     */

    // 2a. SHORE_PAYMENT_UPD_WH
    tuple_fifo* buffer = new tuple_fifo(sizeof(int));
    tuple_filter_t* filter = new trivial_filter_t(sizeof(int));

    trx_packet_t* upd_wh_packet = 
        create_shore_payment_upd_wh_packet( packet->_packet_id,
                                            buffer,
                                            filter,
                                            packet->get_trx_id(),
                                            &packet->_p_in);
    

    // 2b. SHORE_PAYMENT_UPD_DISTR
    buffer = new tuple_fifo(sizeof(int));
    filter = new trivial_filter_t(sizeof(int));

    trx_packet_t* upd_distr_packet = 
        create_shore_payment_upd_distr_packet( packet->_packet_id,
                                               buffer,
                                               filter,
                                               packet->get_trx_id(),
                                               &packet->_p_in);
    

    // 2c. SHORE_PAYMENT_UPD_CUST
    buffer = new tuple_fifo(sizeof(int));
    filter = new trivial_filter_t(sizeof(int));

    trx_packet_t* upd_cust_packet = 
        create_shore_payment_upd_cust_packet( packet->_packet_id,
                                              buffer,
                                              filter,
                                              packet->get_trx_id(),
                                              &packet->_p_in);


    // 2d. SHORE_PAYMENT_INS_HIST
    /** FIXME: (ip) I am not considering the two different options 
     *  I assume that C_H_WH_ID = _WH_ID */
    buffer = new tuple_fifo(sizeof(int));
    filter = new trivial_filter_t(sizeof(int));

    trx_packet_t* ins_hist_packet = 
        create_shore_payment_ins_hist_packet( packet->_packet_id,
                                              buffer,
                                              filter,
                                              packet->get_trx_id(),
                                              &packet->_p_in);
    
    // 2e. SHORE_PAYMENT_FINALIZE
    buffer = new tuple_fifo(sizeof(int));
    filter = new trivial_filter_t(sizeof(int));

    trx_packet_t* finalize_packet = 
        create_shore_payment_finalize_packet( packet->_packet_id,
                                              buffer,
                                              filter,
                                              packet->get_trx_id(),
                                              upd_wh_packet,
                                              upd_distr_packet,
                                              upd_cust_packet,
                                              ins_hist_packet);
    
    // go!
    qpipe::query_state_t* qs = dp->query_state_create();
    finalize_packet->assign_query_state(qs);
    upd_wh_packet->assign_query_state(qs);
    upd_distr_packet->assign_query_state(qs);
    upd_cust_packet->assign_query_state(qs);
    ins_hist_packet->assign_query_state(qs);


    // start processing packets
    trx_result_process_tuple_t rpt(finalize_packet);
    process_query(finalize_packet, rpt);

    dp->query_state_destroy(qs);
    
    //    TRACE( TRACE_ALWAYS, "DONE. NOTIFYING CLIENT\n" );
    
    // writing output 

    // create output tuple
    // "I" own tup, so allocate space for it in the stack
    size_t dest_size = packet->output_buffer()->tuple_size();
    
    array_guard_t<char> dest_data = new char[dest_size];
    tuple_t dest(dest_data, dest_size);
    
    
    trx_result_tuple_t aTrxResultTuple(COMMITTED, my_trx_id);
    
    trx_result_tuple_t* dest_result_tuple;
    dest_result_tuple = aligned_cast<trx_result_tuple_t>(dest.data);
    
    *dest_result_tuple = aTrxResultTuple;
    
    adaptor->output(dest);


} // process_packet




/** @fn get_next_counter
 *  @brief Using the mutex, increases the internal counter by 1 and returns it.
 */

int shore_payment_begin_stage_t::get_next_counter() {

    CRITICAL_SECTION( ccs, _trx_counter_lock);

    //    TRACE( TRACE_ALWAYS, "counter: %d\n", _trx_counter);

    return (++_trx_counter);
}


/** SHORE_PAYMENT_BEGIN packet creation functions */


/** @fn  create_shore_payment_upd_wh_packet_t
 *  @brief Retuns a shore_payment_upd_wh_packet_t 
 */

trx_packet_t* 
shore_payment_begin_stage_t::create_shore_payment_upd_wh_packet(const c_str& client_prefix,
                                                                tuple_fifo* uwh_buffer,
                                                                tuple_filter_t* uwh_filter,
                                                                int a_trx_id,
                                                                payment_input_t* p_pin)
{
    c_str packet_name("%s_shore_payment_upd_wh", client_prefix.data());

    shore_payment_upd_wh_packet_t* shore_payment_upd_wh_packet =
        new shore_payment_upd_wh_packet_t(packet_name,
                                          uwh_buffer,
                                          uwh_filter,
                                          a_trx_id,
                                          p_pin);

    return (shore_payment_upd_wh_packet);
}


/** @fn  create_shore_payment_upd_distr_packet_t
 *  @brief Returns a shore_payment_upd_distr_packet_t 
 */
 
trx_packet_t* 
shore_payment_begin_stage_t::create_shore_payment_upd_distr_packet(const c_str& client_prefix,
                                                                   tuple_fifo* ud_buffer,
                                                                   tuple_filter_t* ud_filter,
                                                                   int a_trx_id,
                                                                   payment_input_t* p_pin)
{
    c_str packet_name("%s_shore_payment_upd_distr", client_prefix.data());

    shore_payment_upd_distr_packet_t* shore_payment_upd_distr_packet =
        new shore_payment_upd_distr_packet_t(packet_name,
                                             ud_buffer,
                                             ud_filter,
                                             a_trx_id,
                                             p_pin);
    
    return (shore_payment_upd_distr_packet);
}



/** @fn  create_shore_payment_upd_cust_packet_t
 *  @brief Returns a shore_payment_upd_cust_packet_t 
 */
 
trx_packet_t* 
shore_payment_begin_stage_t::create_shore_payment_upd_cust_packet(const c_str& client_prefix,
                                                                  tuple_fifo* uc_buffer,
                                                                  tuple_filter_t* uc_filter,
                                                                  int a_trx_id,
                                                                  payment_input_t* p_pin)
{
    c_str packet_name("%s_shore_payment_upd_cust", client_prefix.data());

    shore_payment_upd_cust_packet_t* shore_payment_upd_cust_packet =
        new shore_payment_upd_cust_packet_t(packet_name,
                                            uc_buffer,
                                            uc_filter,
                                            a_trx_id,
                                            p_pin);
    
    return (shore_payment_upd_cust_packet);
}


/** @fn  create_shore_payment_ins_hist_packet_t
 *  @brief Returns a shore_payment_ins_hist_packet_t
 */

trx_packet_t* 
shore_payment_begin_stage_t::create_shore_payment_ins_hist_packet(const c_str& client_prefix,
                                                                  tuple_fifo* ih_buffer,
                                                                  tuple_filter_t* ih_filter,
                                                                  int a_trx_id,
                                                                  payment_input_t* p_pin)
{
    c_str packet_name("%s_shore_payment_ins_hist", client_prefix.data());
    
    shore_payment_ins_hist_packet_t* shore_payment_ins_hist_packet =
        new shore_payment_ins_hist_packet_t(packet_name,
                                            ih_buffer,
                                            ih_filter,
                                            a_trx_id,
                                            p_pin,
                                            "warehouse", /* (ip) TODO!! */
                                            "district"); 

    return (shore_payment_ins_hist_packet);
}


/** @fn  create_shore_payment_finalize_packet_t
 *  @brief Returns a shore_payment_finalize_packet_t
 */

trx_packet_t* 
shore_payment_begin_stage_t::create_shore_payment_finalize_packet(const c_str& client_prefix,
                                                                  tuple_fifo* fin_buffer,
                                                                  tuple_filter_t* fin_filter,
                                                                  int a_trx_id,
                                                                  trx_packet_t* upd_wh,
                                                                  trx_packet_t* upd_distr,
                                                                  trx_packet_t* upd_cust,
                                                                  trx_packet_t* ins_hist)
{
    c_str packet_name("%s_shore_payment_finalize", client_prefix.data());

    shore_payment_finalize_packet_t* shore_payment_finalize_packet =
        new shore_payment_finalize_packet_t(packet_name,
                                            fin_buffer,
                                            fin_filter,
                                            a_trx_id,
                                            upd_wh,
                                            upd_distr,
                                            upd_cust,
                                            ins_hist);
    
    return (shore_payment_finalize_packet);
}

