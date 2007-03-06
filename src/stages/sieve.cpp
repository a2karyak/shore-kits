// -*- mode:C++; c-basic-offset:4 -*-

#include "stages/sieve.h"



const c_str sieve_packet_t::PACKET_TYPE = "SIEVE";



const c_str sieve_stage_t::DEFAULT_STAGE_NAME = "SIEVE_STAGE";



void sieve_stage_t::process_packet() {


    adaptor_t* adaptor = _adaptor;
    sieve_packet_t* packet = (sieve_packet_t*)adaptor->get_packet();

    
    tuple_sieve_t* sieve = packet->_sieve;
    tuple_fifo* input_buffer = packet->_input_buffer;
    dispatcher_t::dispatch_packet(packet->_input);


    // "I" own dest, so allocate space for it on the stack
    size_t dest_size = packet->output_buffer()->tuple_size();
    array_guard_t<char> dest_data = new char[dest_size];
    tuple_t dest(dest_data, dest_size);


    while (1) {

        tuple_t src;
        if (!input_buffer->get_tuple(src))
            break;

        if (sieve->pass(dest, src))
            adaptor->output(dest);
    }

    if (sieve->flush(dest))
        adaptor->output(dest);
}
