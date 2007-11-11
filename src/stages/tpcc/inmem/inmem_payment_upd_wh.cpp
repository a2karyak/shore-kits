/* -*- mode:C++; c-basic-offset:4 -*- */

# include "stages/tpcc/inmem/inmem_payment_upd_wh.h"
# include "util.h"


using namespace tpcc_payment;


const c_str inmem_payment_upd_wh_packet_t::PACKET_TYPE = "INMEM_PAYMENT_UPD_WH";

const c_str inmem_payment_upd_wh_stage_t::DEFAULT_STAGE_NAME = "INMEM_PAYMENT_UPD_WH_STAGE";



/**
 *  @brief inmem_payment_upd_wh constructor
 */

inmem_payment_upd_wh_stage_t::inmem_payment_upd_wh_stage_t() {
    
    TRACE(TRACE_ALWAYS, "INMEM_PAYMENT_UPD_WH constructor\n");
}


/**
 *  @brief Update warehouse table according to $2.5.2.2
 *
 *  @return void
 *
 *  @throw May throw exceptions on error.
 */

void inmem_payment_upd_wh_stage_t::process_packet() {

    adaptor_t* adaptor = _adaptor;

    inmem_payment_upd_wh_packet_t* packet = 
	(inmem_payment_upd_wh_packet_t*)adaptor->get_packet();

    //    packet->describe_trx();

    staged_updateInMemWarehouse(&packet->_pin, inmem_env);

    // create output tuple
    // "I" own tup, so allocate space for it in the stack
    //     size_t dest_size = packet->output_buffer()->tuple_size();
    //     array_guard_t<char> dest_data = new char[dest_size];
    //     tuple_t dest(dest_data, dest_size);

    //     int* dest_tmp;
    //     dest_tmp = aligned_cast<int>(dest.data);

    //     *dest_tmp = my_trx_id;

    //     adaptor->output(dest);
     
} // process_packet

