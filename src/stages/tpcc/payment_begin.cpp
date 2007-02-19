/* -*- mode:C++; c-basic-offset:4 -*- */

# include "stages/tpcc/payment_begin.h"
# include "util.h"


const c_str payment_begin_packet_t::PACKET_TYPE = "PAYMENT_BEGIN";

const c_str payment_begin_stage_t::DEFAULT_STAGE_NAME = "PAYMENT_BEGIN_STAGE";


/**
 *  @brief Initialize transaction specified by payment_begin_packet_t p.
 *
 *  @return void
 *
 *  @throw May throw exceptions on error.
 */

void payment_begin_stage_t::process_packet() {

    adaptor_t* adaptor = _adaptor;
    payment_begin_packet_t* packet = (payment_begin_packet_t*)adaptor->get_packet();

    const int my_trx_id = packet->trx_id();

    TRACE(TRACE_ALWAYS, "Processing PAYMENT_BEGIN with id: %d\n", my_trx_id);

} // process_packet
