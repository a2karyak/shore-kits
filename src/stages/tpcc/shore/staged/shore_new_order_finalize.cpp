/* -*- mode:C++; c-basic-offset:4 -*- */

#include "stages/tpcc/shore/staged/shore_new_order_finalize.h"
#include "util.h"


const c_str shore_new_order_finalize_packet_t::PACKET_TYPE = "SHORE_NEW_ORDER_FINALIZE";

const c_str shore_new_order_finalize_stage_t::DEFAULT_STAGE_NAME = "SHORE_NEW_ORDER_FINALIZE_STAGE";


/**
 *  @brief shore_new_order_finalize constructor
 */

shore_new_order_finalize_stage_t::shore_new_order_finalize_stage_t() 
{    
    TRACE(TRACE_DEBUG, "SHORE_NEW_ORDER_FINALIZE constructor\n");
}


/** FIXME: (ip) THIS STAGE Should be generic and NOT specific to SHORE_NEW_ORDER trx. */

/**
 *  @brief Commits if every sub-transaction is ok, otherwise broadcasts a message
 *  to Rollback if someone wants to abort.
 *
 *  @return void
 *
 *  @throw May throw exceptions on error.
 */

void shore_new_order_finalize_stage_t::process_packet() 
{
    adaptor_t* adaptor = _adaptor;

    shore_new_order_finalize_packet_t* packet = 
	(shore_new_order_finalize_packet_t*)adaptor->get_packet();

    //    packet->describe_trx();

    /* First dispatch all the shore_new_order packets */
    dispatcher_t::dispatch_packet(packet->_outside);

    for (int i=0; i<packet->_ol_cnt; i++)
        dispatcher_t::dispatch_packet(packet->_list_one_ol[i]);


    /* Variables used to determine the transaction status.
       All stages return a single integer indicating success/failure */
    int trx_status = 0;
    int* trx_data;
    tuple_t trx_result;

    /* Get the results of each sub-transaction */
    trx_status = packet->_outside_buffer->get_tuple(trx_result);    
    
    if (!trx_status && !(trx_data = aligned_cast<int>(trx_result.data))) {
        // should abort
        //TRACE( TRACE_ALWAYS, "ST=(%d) D=(%d)\n", trx_status, *trx_data);
        packet->set_trx_state(POISSONED);
        packet->rollback();
    }

    for (int i=0; i<packet->_ol_cnt; i++) {
        trx_status = packet->_list_one_ol_buffer[i]->get_tuple(trx_result);
    
        if (!trx_status && !(trx_data = aligned_cast<int>(trx_result.data))) {
            // should abort
            //TRACE( TRACE_ALWAYS, "ST=(%d) D=(%d)\n", trx_status, *trx_data);
            packet->set_trx_state(POISSONED);
            packet->rollback();
        }
    }

    // if reached this point should commit
    packet->commit();
    packet->set_trx_state(COMMITTED);

} // process_packet


