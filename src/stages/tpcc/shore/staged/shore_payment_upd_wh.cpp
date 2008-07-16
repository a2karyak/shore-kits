/* -*- mode:C++; c-basic-offset:4 -*- */

#include "stages/tpcc/shore/staged/shore_payment_upd_wh.h"

using namespace qpipe;
using namespace shore;

const c_str shore_payment_upd_wh_packet_t::PACKET_TYPE = "SHORE_PAYMENT_UPD_WH";

const c_str shore_payment_upd_wh_stage_t::DEFAULT_STAGE_NAME = "SHORE_PAYMENT_UPD_WH_STAGE";



/**
 *  @brief shore_payment_upd_wh constructor
 */

shore_payment_upd_wh_stage_t::shore_payment_upd_wh_stage_t() 
{    
    TRACE(TRACE_TRX_FLOW, "SHORE_PAYMENT_UPD_WH constructor\n");
}

/**
 *  @brief Update warehouse table according to $2.5.2.2
 *
 *  @return void
 *
 *  @throw May throw exceptions on error.
 */

void shore_payment_upd_wh_stage_t::process_packet() {

    adaptor_t* adaptor = _adaptor;

    shore_payment_upd_wh_packet_t* packet = 
	(shore_payment_upd_wh_packet_t*)adaptor->get_packet();

    //    packet->describe_trx();

    w_rc_t e = _g_shore_env->db()->begin_xct();
    if (e != RCOK) {
        TRACE( TRACE_ALWAYS,
               "Problem in beginning TRX...\n");
        assert (false); // Error handling not implemented yet
    }
    
    trx_result_tuple_t atrt;
    e = _g_shore_env->staged_pay_updateShoreWarehouse(&packet->_pin, 
                                                   packet->get_trx_id(),
                                                   atrt);

    if (atrt.get_state() == POISSONED) {
        TRACE( TRACE_ALWAYS, 
               "Error in Warehouse Update...\n");
        e = _g_shore_env->db()->abort_xct();

        if (e != RCOK) {
            TRACE( TRACE_ALWAYS,
                   "Error in Abort...\n");
            assert (false); // Error handling not implemented yet
        }
    }

    e = _g_shore_env->db()->commit_xct();
    if (e != RCOK) {
        TRACE( TRACE_ALWAYS, 
               "Error in Commit...\n");
        assert (false); // Error handling not implemented yet
    }

    // create output tuple
    // "I" own tup, so allocate space for it in the stack
    size_t dest_size = packet->output_buffer()->tuple_size();    
    alloc_guard dest_data = dest_size;
    tuple_t dest(dest_data, dest_size);

    int* dest_result_int;
    dest_result_int = aligned_cast<int>(dest.data);    
    *dest_result_int = atrt.get_state();    
    adaptor->output(dest);
     
} // process_packet

