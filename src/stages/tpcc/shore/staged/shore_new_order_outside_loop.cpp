/* -*- mode:C++; c-basic-offset:4 -*- */

#include "stages/tpcc/shore/staged/shore_new_order_outside_loop.h"
#include "util.h"

const c_str shore_new_order_outside_loop_packet_t::PACKET_TYPE = "SHORE_NEW_ORDER_OUTSIDE_LOOP";

const c_str shore_new_order_outside_loop_stage_t::DEFAULT_STAGE_NAME = "SHORE_NEW_ORDER_OUTSIDE_LOOP_STAGE";



/**
 *  @brief shore_new_order_outside_loop constructor
 */

shore_new_order_outside_loop_stage_t::shore_new_order_outside_loop_stage_t() {
    
    TRACE(TRACE_DEBUG, "SHORE_NEW_ORDER_OUTSIDE_LOOP constructor\n");
}


/**
 *  @brief Update warehouse table according to $2.5.2.2
 *
 *  @return void
 *
 *  @throw May throw exceptions on error.
 */

void shore_new_order_outside_loop_stage_t::process_packet() {

    adaptor_t* adaptor = _adaptor;

    shore_new_order_outside_loop_packet_t* packet = 
	(shore_new_order_outside_loop_packet_t*)adaptor->get_packet();

    //    packet->describe_trx();

    w_rc_t e = _g_shore_env->db()->begin_xct();
    if (e.is_error()) {
        TRACE( TRACE_ALWAYS,
               "Problem in beginning TRX...\n");
        assert (false); // Error handling not implemented yet
    }
    
    trx_result_tuple_t atrt;
    assert(0);//_g_shore_env->staged_no_outside_loop(&packet->_noin, 
//                                       packet->_tstamp,
//                                       packet->get_trx_id(),
//                                       atrt);
        
    if (atrt.get_state() == POISSONED) {
        TRACE( TRACE_ALWAYS, 
               "Error in Warehouse Update...\n");
        e = _g_shore_env->db()->abort_xct();

        if (e.is_error()) {
            TRACE( TRACE_ALWAYS,
                   "Error in Abort...\n");
            assert (false); // Error handling not implemented yet
        }
    }

    e = _g_shore_env->db()->commit_xct();
    if (e.is_error()) {
        TRACE( TRACE_ALWAYS, 
               "Error in Commit...\n");
        assert (false); // Error handling not implemented yet
    }
    
    assert (false); // TODO


    // create output tuple
    // "I" own tup, so allocate space for it in the stack
    //    size_t dest_size = packet->output_buffer()->tuple_size();
    //    char* dest_data = new char[dest_size];
    //    tuple_t* dest = new tuple_t(dest_data, dest_size);

    //    int* dest_tmp;
    //    dest_tmp = aligned_cast<int>(dest.data);

    //    *dest_tmp = my_trx_id;

    //    adaptor->output(dest);
     
} // process_packet

