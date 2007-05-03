/* -*- mode:C++; c-basic-offset:4 -*- */

# include "stages/tpcc/payment_finalize.h"
# include "util.h"


const c_str payment_finalize_packet_t::PACKET_TYPE = "PAYMENT_FINALIZE";

const c_str payment_finalize_stage_t::DEFAULT_STAGE_NAME = "PAYMENT_FINALIZE_STAGE";


/**
 *  @brief payment_finalize constructor
 */

payment_finalize_stage_t::payment_finalize_stage_t() {
    
    TRACE(TRACE_ALWAYS, "PAYMENT_FINALIZE constructor\n");
}


/** FIXME: (ip) THIS STAGE Should be generic and NOT specific to PAYMENT trx. */

/**
 *  @brief Commits if every sub-transaction is ok, otherwise broadcasts a message
 *  to Rollback if someone wants to abort.
 *
 *  @return void
 *
 *  @throw May throw exceptions on error.
 */

void payment_finalize_stage_t::process_packet() {

    adaptor_t* adaptor = _adaptor;

    payment_finalize_packet_t* packet = 
	(payment_finalize_packet_t*)adaptor->get_packet();

    packet->describe_trx();


    /* First dispatch all the payment packets */
    dispatcher_t::dispatch_packet(packet->_upd_wh);
    dispatcher_t::dispatch_packet(packet->_upd_distr);
    dispatcher_t::dispatch_packet(packet->_upd_cust);
    dispatcher_t::dispatch_packet(packet->_ins_hist);


    /* Variables used to determine the transaction status.
       All stages return a single integer indicating success/failure */
    int trx_status = 0;
    tuple_t trx_result;

    /* Get the results of each sub-transaction */
    trx_status = packet->_upd_wh_buffer->get_tuple(trx_result);
    
    if (!trx_status && !(aligned_cast<int>(trx_result.data))) {
        // should abort
        packet->set_trx_state(POISSONED);
        packet->rollback();
    }

    trx_status = packet->_upd_distr_buffer->get_tuple(trx_result);
    
    if (!trx_status && !(aligned_cast<int>(trx_result.data))) {
        // should abort
        packet->set_trx_state(POISSONED);
        packet->rollback();
    }

    trx_status = packet->_upd_cust_buffer->get_tuple(trx_result);
    
    if (!trx_status && !(aligned_cast<int>(trx_result.data))) {
        // should abort
        packet->set_trx_state(POISSONED);
        packet->rollback();
    }

    trx_status = packet->_ins_hist_buffer->get_tuple(trx_result);
    
    if (!trx_status && !(aligned_cast<int>(trx_result.data))) {
        // should abort
        packet->set_trx_state(POISSONED);
        packet->rollback();
    }


    // if reached this point should commit
    packet->commit();
    packet->set_trx_state(COMMITTED);

} // process_packet


