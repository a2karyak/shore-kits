// -*- mode:C++; c-basic-offset:4 -*-

#include "stages/echo.h"



const c_str echo_packet_t::PACKET_TYPE = "ECHO";



const c_str echo_stage_t::DEFAULT_STAGE_NAME = "ECHO_STAGE";



void echo_stage_t::process_packet() {


    adaptor_t* adaptor = _adaptor;
    echo_packet_t* packet = (echo_packet_t*)adaptor->get_packet();
    
    
    tuple_fifo* input_buffer = packet->_input_buffer;
    dispatcher_t::dispatch_packet(packet->_input);

    
    guard<qpipe::page> next_page = page::alloc(input_buffer->tuple_size());
    while (1) {
        if (!input_buffer->copy_page(next_page))
            break;
        adaptor->output(next_page);
    }
}
