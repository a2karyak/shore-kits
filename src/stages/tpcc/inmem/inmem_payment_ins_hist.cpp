/* -*- mode:C++; c-basic-offset:4 -*- */

# include "stages/tpcc/inmem/inmem_payment_ins_hist.h"
# include "util.h"

using namespace tpcc_payment;


const c_str inmem_payment_ins_hist_packet_t::PACKET_TYPE = "INMEM_PAYMENT_INS_HIST";

const c_str inmem_payment_ins_hist_stage_t::DEFAULT_STAGE_NAME = "INMEM_PAYMENT_INS_HIST_STAGE";



/**
 *  @brief inmem_payment_ins_hist constructor
 */

inmem_payment_ins_hist_stage_t::inmem_payment_ins_hist_stage_t() {
    
    TRACE(TRACE_TRX_FLOW, "INMEM_PAYMENT_INS_HIST constructor\n");
}


/**
 *  @brief Insert a row at the history table according to $2.5.2.2
 *
 *  @return void
 *
 *  @throw May throw exceptions on error.
 */

void inmem_payment_ins_hist_stage_t::process_packet() {

    adaptor_t* adaptor = _adaptor;

    inmem_payment_ins_hist_packet_t* packet = 
	(inmem_payment_ins_hist_packet_t*)adaptor->get_packet();

    //    packet->describe_trx();

    if (staged_insertInMemHistory(&packet->_pin,inmem_env)) {
        TRACE( TRACE_TRX_FLOW, 
               "Error in History Insertion.\n");
        
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

