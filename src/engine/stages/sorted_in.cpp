// -*- mode:C++; c-basic-offset:4 -*-

#include "engine/stages/sorted_in.h"
#include "engine/functors.h"
#include "engine/dispatcher.h"

const char* sorted_in_stage_t::DEFAULT_STAGE_NAME = "Sorted IN";
const char* sorted_in_packet_t::PACKET_TYPE = "Sorted IN";

/**
 * @brief implements "where left.key [not] in right". Assumes both
 * relations are sorted and that the right relation is distinct.
 */
stage_t::result_t sorted_in_stage_t::process_packet() {
    // setup
    sorted_in_packet_t* packet;
    packet = (sorted_in_packet_t*) _adaptor->get_packet();
    tuple_buffer_t* left_input = packet->_left_input;
    tuple_buffer_t* right_input = packet->_right_input;
    key_extractor_t* left_extractor = packet->_left_extractor;
    key_extractor_t* right_extractor = packet->_right_extractor;
    key_compare_t* compare = packet->_compare;
    bool reject_matches = packet->_reject_matches;

    // fire things up
    dispatcher_t::dispatch_packet(packet->_left);
    dispatcher_t::dispatch_packet(packet->_right);

    tuple_t left;
    const char* left_key;
    int left_hint;

    tuple_t right;
    const char* right_key;
    int right_hint;
    
    // seed the process
    if(left_input->get_tuple(left))
        return stage_t::RESULT_STOP;

    left_key = left_extractor->extract_key(left);
    left_hint = left_extractor->extract_hint(left_key);

    // loop until there are no more tuples left to read
    while(1) {
        // process (ie discard) right side tuples that come before the
        // current left tuple
        int diff;
        do {
            // advance the right side
            if(right_input->get_tuple(right)) {
                // no possible matches remain
                if(reject_matches) {
                    while(!left_input->get_tuple(left)) {
                        result_t result = _adaptor->output(left);
                        if(result)
                            return result;
                    }
                }
            
                // done
                return stage_t::RESULT_STOP;
            }

            right_key = right_extractor->extract_key(right);
            right_hint = right_extractor->extract_hint(right_key);

            diff = left_hint - right_hint;
            if(!diff && left_extractor->key_size() > sizeof(int))
                diff = (*compare)(left_key, right_key);

        } while(diff > 0);

        // now, process left side tuples until we pass the current
        // right side tuple
        while(diff <= 0) {
            // output?
            if((diff == 0) != reject_matches) {
                result_t result = _adaptor->output(left);
                if(result)
                    return result;
            }
            
            // advance the left side
            if(left_input->get_tuple(left))
                return stage_t::RESULT_STOP;

            left_key = left_extractor->extract_key(left);
            left_hint = left_extractor->extract_hint(left_key);
            diff = left_hint - right_hint;
            if(!diff && left_extractor->key_size() > sizeof(int))
                diff = (*compare)(left_key, right_key);
        } 
    }
    // unreachable
}
