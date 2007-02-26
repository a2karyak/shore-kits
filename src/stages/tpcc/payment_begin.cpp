/* -*- mode:C++; c-basic-offset:4 -*- */

# include "stages/tpcc/payment_begin.h"
# include "util.h"


const c_str payment_begin_packet_t::PACKET_TYPE = "PAYMENT_BEGIN";

const c_str payment_begin_stage_t::DEFAULT_STAGE_NAME = "PAYMENT_BEGIN_STAGE";



/**
 *  @brief Constructor: Initializes the counter
 */

payment_begin_stage_t::payment_begin_stage_t()
    : _trx_counter_mutex(thread_mutex_create())
{
    
    TRACE(TRACE_ALWAYS, "PAYMENT_BEGIN constructor");

    _trx_counter = 0;    
}

/**
 *  @brief Initialize transaction specified by payment_begin_packet_t p.
 *
 *  @return void
 *
 *  @throw May throw exceptions on error.
 */

void payment_begin_stage_t::process_packet() {

    adaptor_t* adaptor = _adaptor;

    payment_begin_packet_t* packet = 
	(payment_begin_packet_t*)adaptor->get_packet();

    const int my_trx_id = packet->trx_id();

    TRACE(TRACE_ALWAYS, 
	  "Processing PAYMENT_BEGIN with id: %d. Counter: &d\n", 
	  my_trx_id, get_next_counter());

} // process_packet


/**
 *  @brief Using the mutex, increases the internal counter by 1 and returns it.
 */

int payment_begin_stage_t::get_next_counter() {

    critical_section_t cs( _trx_counter_mutex );

    int tmp_counter = ++_trx_counter;

    cs.exit(); 

    return (tmp_counter);
}
