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


    TRACE( TRACE_ALWAYS, "!! FINALIZING TRX=%d !!\n", packet->trx_id());

} // process_packet

