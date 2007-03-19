/* -*- mode:C++; c-basic-offset:4 -*- */

#include "stages/delay_writer.h"
#include "util.h"
#include <unistd.h>



const c_str delay_writer_packet_t::PACKET_TYPE = "DELAY_WRITER";



const c_str delay_writer_stage_t::DEFAULT_STAGE_NAME = "DELAY_WRITER_STAGE";



/**
 *  @brief Write a fixed number of tuples at a specified rate.
 */
void delay_writer_stage_t::process_packet() {

    adaptor_t* adaptor = _adaptor;
    delay_writer_packet_t* packet = (delay_writer_packet_t*)adaptor->get_packet();
    
    /* create blank tuple */
    size_t tuple_size = packet->_output_tuple_size;
    char   tuple_data[tuple_size];
    memset(tuple_data, 0, sizeof(tuple_data));
    tuple_t tuple(tuple_data, tuple_size);
    

    int delay_ms = packet->_delay_ms;
    int num_tuples = packet->_num_tuples;
    for (int i = 0; i < num_tuples; i++) {
        busy_delay(delay_ms);
        adaptor->output(tuple);
    }
}
