// -*- mode:C++; c-basic-offset:4 -*-

#include "stages/echo.h"



const c_str echo_packet_t::PACKET_TYPE = "ECHO";



const c_str echo_stage_t::DEFAULT_STAGE_NAME = "ECHO_STAGE";



void echo_stage_t::process_packet() {


    adaptor_t* adaptor = _adaptor;
    echo_packet_t* packet = (echo_packet_t*)adaptor->get_packet();
    
    
    tuple_fifo* input_buffer = packet->_input_buffer;
    dispatcher_t::dispatch_packet(packet->_input);

    
    while (1) {
        guard<qpipe::page> p = input_buffer->get_page();
        if (p == NULL)
            break;
        adaptor->output(p);
    }
}
