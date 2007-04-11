/* -*- mode:C++; c-basic-offset:4 -*- */

# include "stages/tpcc/payment_begin.h"
# include "util.h"


const c_str payment_begin_packet_t::PACKET_TYPE = "PAYMENT_BEGIN";

const c_str payment_begin_stage_t::DEFAULT_STAGE_NAME = "PAYMENT_BEGIN_STAGE";


int payment_begin_stage_t::_trx_counter = 0;
pthread_mutex_t payment_begin_stage_t::_trx_counter_mutex = PTHREAD_MUTEX_INITIALIZER;



/**
 *  @brief Constructor: Initializes the counter
 */

payment_begin_stage_t::payment_begin_stage_t() {
    
    TRACE(TRACE_ALWAYS, "PAYMENT_BEGIN constructor\n");
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

    int my_trx_id;
    my_trx_id = get_next_counter();
    packet->set_trx_id(my_trx_id);

    TRACE(TRACE_ALWAYS, 
	  "Processing PAYMENT_BEGIN with id: %d\n", 
	  packet->trx_id());

    packet->describe_trx();

    /** FIXME: (ip) Below is a test */

    // create output tuple
    // "I" own tup, so allocate space for it in the stack
     size_t dest_size = packet->output_buffer()->tuple_size();
     array_guard_t<char> dest_data = new char[dest_size];
     tuple_t dest(dest_data, dest_size);

     int* dest_tmp;
     dest_tmp = aligned_cast<int>(dest.data);

     *dest_tmp = my_trx_id;

     adaptor->output(dest);

} // process_packet


/**
 *  @brief Using the mutex, increases the internal counter by 1 and returns it.
 */

int payment_begin_stage_t::get_next_counter() {

    critical_section_t cs( _trx_counter_mutex );

    int tmp_counter = ++_trx_counter;

    TRACE( TRACE_ALWAYS, "counter: %d\n", _trx_counter);

    cs.exit(); 

    return (tmp_counter);
}
