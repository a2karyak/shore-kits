// -*- mode:C++; c-basic-offset:4 -*-

#include "stages/aggregate.h"



const c_str aggregate_packet_t::PACKET_TYPE = "AGGREGATE";



const c_str aggregate_stage_t::DEFAULT_STAGE_NAME = "AGGREGATE_STAGE";



void aggregate_stage_t::process_packet() {


    adaptor_t* adaptor = _adaptor;
    aggregate_packet_t* packet = (aggregate_packet_t*)adaptor->get_packet();

    
    tuple_aggregate_t* aggregate = packet->_aggregator;
    key_extractor_t* extract = packet->_extract;
    tuple_fifo* input_buffer = packet->_input_buffer;
    dispatcher_t::dispatch_packet(packet->_input);


    // "I" own dest, so allocate space for it on the stack
    size_t dest_size = packet->output_buffer()->tuple_size();
    array_guard_t<char> dest_data = new char[dest_size];
    tuple_t dest(dest_data, dest_size);


    size_t agg_size = aggregate->tuple_size();
    array_guard_t<char> agg_data = new char[agg_size];
    tuple_t agg(agg_data, agg_size);
    
    size_t key_size = extract->key_size();
    char* last_key = aggregate->key_extractor()->extract_key(agg_data);

    int i = 0;
    bool first = true;
    while (1) {

        // No more tuples?
        tuple_t src;
        if(!input_buffer->get_tuple(src))
            // Exit from loop, but can't return quite yet since we may
            // still have one more aggregation to perform.
            break;
            
        // got another tuple
        const char* key = extract->extract_key(src);

        // break group?
        if(first || /* allow init() call if first tuple */
           (key_size && memcmp(last_key, key, key_size))) {

            if(!first) {
                aggregate->finish(dest, agg.data);
                adaptor->output(dest);
            }
 
            aggregate->init(agg.data);
            memcpy(last_key, key, key_size);
            first = false;
        }
        
        aggregate->aggregate(agg.data, src);
        i++;
    }

    // output the last group, if any
    if(!first) {
        aggregate->finish(dest, agg.data);
        adaptor->output(dest);
    }
}
