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

    // seed the process with the first left tuple
    tuple_t left;
    if(left_input->get_tuple(left))
        return stage_t::RESULT_STOP;
    
    while(1) {
        // try to match against a right tuple
        tuple_t right;
        const char* right_key;
        int right_hint;
        while(1) {
            if(right_input->get_tuple(right)) {
                // no possible matches remain
                if(reject_matches) {
                    while(!left_input->get_tuple(left)) {
                        result_t result = _adaptor->output(left);
                        if(result)
                            return result;
                    }
                }
                else {
                    while(!left_input->get_tuple(left)) {
                        int left_hint = left_extractor->extract_hint(left);
                        printf("Rejected suppkey %d\n", left_hint);
                    }
                }
            
                // done
                return stage_t::RESULT_STOP;
            }

            right_key = right_extractor->extract_key(right);
            right_hint = right_extractor->extract_hint(right_key);

            // match as many left tuples against this right as possible
            while(1) {
                const char* left_key = left_extractor->extract_key(left);
                int left_hint = left_extractor->extract_hint(left_key);
                int diff = left_hint - right_hint;
                // break a tie?
                if(!diff && left_extractor->key_size() > sizeof(int)) 
                    diff = (*compare)(left_key, right_key);

                if(diff < 0) {
                    // no match (advance the left side)
                    if(reject_matches) {
                        result_t result = _adaptor->output(left);
                        if(result)
                            return result;
                    }
                    else {
                        printf("Rejected suppkey %d\n", left_hint);
                    }
                }
                else if(diff > 0) {
                    // need to advance the right side
                    break;
                }
                else {
                    // match! (advance the left side)
                    if(!reject_matches) {
                        result_t result = _adaptor->output(left);
                        if(result)
                            return result;
                    }
                    else {
                        printf("Rejected suppkey %d\n", left_hint);
                    }
                }

                // advance the left side
                if(left_input->get_tuple(left))
                    return stage_t::RESULT_STOP;
            }
        }
    }

    // unreachable
}
