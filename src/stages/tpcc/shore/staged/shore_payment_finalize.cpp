/* -*- mode:C++; c-basic-offset:4 -*- */

#include "stages/tpcc/shore/staged/shore_payment_finalize.h"
#include "util.h"


const c_str shore_payment_finalize_packet_t::PACKET_TYPE = "SHORE_PAYMENT_FINALIZE";

const c_str shore_payment_finalize_stage_t::DEFAULT_STAGE_NAME = "SHORE_PAYMENT_FINALIZE_STAGE";


/**
 *  @brief shore_payment_finalize constructor
 */

shore_payment_finalize_stage_t::shore_payment_finalize_stage_t() {    
    TRACE(TRACE_DEBUG, "SHORE_PAYMENT_FINALIZE constructor\n");
}


/** FIXME: (ip) THIS STAGE Should be generic and NOT specific to SHORE_PAYMENT trx. */

/**
 *  @brief Commits if every sub-transaction is ok, otherwise broadcasts a message
 *  to Rollback if someone wants to abort.
 *
 *  @return void
 *
 *  @throw May throw exceptions on error.
 */

void shore_payment_finalize_stage_t::process_packet() 
{
    adaptor_t* adaptor = _adaptor;

    shore_payment_finalize_packet_t* packet = 
	(shore_payment_finalize_packet_t*)adaptor->get_packet();

    //    packet->describe_trx();

    /* First dispatch all the shore_payment packets */
    dispatcher_t::dispatch_packet(packet->_upd_wh);
    dispatcher_t::dispatch_packet(packet->_upd_distr);
    dispatcher_t::dispatch_packet(packet->_upd_cust);
    dispatcher_t::dispatch_packet(packet->_ins_hist);

    /* Variables used to determine the transaction status.
       All stages return a single integer indicating success/failure */
    int trx_status = 0;
    int* trx_data;
    tuple_t trx_result;

    /* Get the results of each sub-transaction */
    trx_status = packet->_upd_wh_buffer->get_tuple(trx_result);    
    
    if (!trx_status && !(trx_data = aligned_cast<int>(trx_result.data))) {
        // should abort
        //TRACE( TRACE_ALWAYS, "ST=(%d) D=(%d)\n", trx_status, *trx_data);
        packet->set_trx_state(POISSONED);
        packet->rollback();
    }

    trx_status = packet->_upd_distr_buffer->get_tuple(trx_result);
    
    if (!trx_status && !(trx_data = aligned_cast<int>(trx_result.data))) {
        // should abort
        //TRACE( TRACE_ALWAYS, "ST=(%d) D=(%d)\n", trx_status, *trx_data);
        packet->set_trx_state(POISSONED);
        packet->rollback();
    }

    trx_status = packet->_upd_cust_buffer->get_tuple(trx_result);
    
    if (!trx_status && !(trx_data = aligned_cast<int>(trx_result.data))) {
        // should abort
        //TRACE( TRACE_ALWAYS, "ST=(%d) D=(%d)\n", trx_status, *trx_data);
        packet->set_trx_state(POISSONED);
        packet->rollback();
    }

    trx_status = packet->_ins_hist_buffer->get_tuple(trx_result);
    
    if (!trx_status && !(trx_data = aligned_cast<int>(trx_result.data))) {
        // should abort
        //TRACE( TRACE_ALWAYS, "ST=(%d) D=(%d)\n", trx_status, *trx_data);
        packet->set_trx_state(POISSONED);
        packet->rollback();
    }


    // if reached this point should commit
    packet->commit();
    packet->set_trx_state(COMMITTED);

} // process_packet


