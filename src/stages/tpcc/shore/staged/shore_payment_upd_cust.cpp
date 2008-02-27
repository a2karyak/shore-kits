/* -*- mode:C++; c-basic-offset:4 -*- */

# include "stages/tpcc/inmem/inmem_payment_upd_cust.h"
# include "util.h"


using namespace tpcc_payment;


const c_str inmem_payment_upd_cust_packet_t::PACKET_TYPE = "INMEM_PAYMENT_UPD_CUST";

const c_str inmem_payment_upd_cust_stage_t::DEFAULT_STAGE_NAME = "INMEM_PAYMENT_UPD_CUST_STAGE";



/**
 *  @brief inmem_payment_upd_cust constructor
 */

inmem_payment_upd_cust_stage_t::inmem_payment_upd_cust_stage_t() {
    
    TRACE(TRACE_TRX_FLOW, "INMEM_PAYMENT_UPD_CUST constructor\n");
}


/**
 *  @brief Update customer table according to $2.5.2.2
 *
 *  @return void
 *
 *  @throw May throw exceptions on error.
 */

void inmem_payment_upd_cust_stage_t::process_packet() {

    adaptor_t* adaptor = _adaptor;

    inmem_payment_upd_cust_packet_t* packet = 
	(inmem_payment_upd_cust_packet_t*)adaptor->get_packet();

    //    packet->describe_trx();

    if (staged_updateInMemCustomer(&packet->_pin, inmem_env)) {
        TRACE( TRACE_TRX_FLOW, 
               "Error in Customer Update.\n");
        
        assert (1==0); // Not implemented yet
    }



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

