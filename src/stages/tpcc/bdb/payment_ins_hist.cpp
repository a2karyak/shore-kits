/* -*- mode:C++; c-basic-offset:4 -*- */

# include "stages/tpcc/bdb/payment_ins_hist.h"
# include "util.h"


const c_str payment_ins_hist_packet_t::PACKET_TYPE = "PAYMENT_INS_HIST";

const c_str payment_ins_hist_stage_t::DEFAULT_STAGE_NAME = "PAYMENT_INS_HIST_STAGE";



/**
 *  @brief payment_ins_hist constructor
 */

payment_ins_hist_stage_t::payment_ins_hist_stage_t() {
    
    TRACE(TRACE_ALWAYS, "PAYMENT_INS_HIST constructor\n");
}


/**
 *  @brief Insert a row at the history table according to $2.5.2.2
 *
 *  @return void
 *
 *  @throw May throw exceptions on error.
 */

void payment_ins_hist_stage_t::process_packet() {

    adaptor_t* adaptor = _adaptor;

    payment_ins_hist_packet_t* packet = 
	(payment_ins_hist_packet_t*)adaptor->get_packet();

    packet->describe_trx();


    TRACE( TRACE_ALWAYS, "!! INSERTING HISTORY !!\n");

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

