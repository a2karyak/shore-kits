/* -*- mode:C++; c-basic-offset:4 -*- */

#include "engine/stages/bnl_in.h"
#include "engine/core/tuple.h"
#include "qpipe_panic.h"
#include "trace.h"

#include <cstring>
#include <bitset>

using std::bitset;



const c_str bnl_in_packet_t::PACKET_TYPE = "BNL_IN";



const c_str bnl_in_stage_t::DEFAULT_STAGE_NAME = "BNL_IN_STAGE";



/**
 *  @brief Implement an IN or NOT IN operator on the left and right
 *  relations using a nested loop. We do page-sized reads of both
 *  relations.
 */

void bnl_in_stage_t::process_packet() {

    adaptor_t* adaptor = _adaptor;
    bnl_in_packet_t* packet = (bnl_in_packet_t*)adaptor->get_packet();
    
    // get ready...
    tuple_fifo* left_buffer  = packet->_left_buffer;
    tuple_source_t* right_source = packet->_right_source;
    key_compare_t* _compare = packet->_compare;
    key_extractor_t* _extract = packet->_extract;
    tuple_comparator_t compare(_extract, _compare);
    bool output_on_match = packet->_output_on_match;
    
    
    // dispatch outer relation so we can stream it
    dispatcher_t::dispatch_packet(packet->_left);

 
    
    while (1) {
        
        
        // get another page of tuples from the outer relation
        guard<page> outer_tuple_page = left_buffer->get_page();
        if ( !outer_tuple_page )
            // done with outer relation... done with join
            return;
    
                
        // Need a bitmap to track matches between outer relation page
        // and inner relation. So far, we've seen no matches.
        bitset<2048> matches;
        assert(outer_tuple_page->tuple_count() <= matches.size());
        
 
        // read the entire inner relation
        packet_t* right_packet = right_source->reset();
        tuple_fifo* right_buffer = right_packet->output_buffer();
        dispatcher_t::dispatch_packet(right_packet);
        

        while (1) {
            
            
            // get another page of tuples from the inner relation
            guard<page> inner_tuple_page = right_buffer->get_page();
            if ( !inner_tuple_page )
                // done with inner relation... time to output some
                // tuples from opage
                break;
            
            
            // compare each tuple on the outer relation page with each
            // tuple on the inner relation page
            page::iterator o_it = outer_tuple_page->begin();
            for (int o_index = 0; o_it != outer_tuple_page->end(); ++o_index) {
                
                
                // grab a tuple from the outer relation page
                tuple_t outer_tuple = o_it.advance();
                hint_tuple_pair_t
                    outer_ktpair(_extract->extract_hint(outer_tuple), outer_tuple.data);

                
                page::iterator i_it = inner_tuple_page->begin();
                while(i_it != inner_tuple_page->end()) {

                    
                    // grab a tuple from the inner relation page
                    tuple_t inner_tuple = i_it.advance();
                    hint_tuple_pair_t
                        inner_ktpair(_extract->extract_hint(inner_tuple), inner_tuple.data);
                    

                    if ( compare(outer_ktpair, inner_ktpair) == 0 )
                        // left and right match!
                        matches.set(o_index);
                    
                    
                } // endof loop over inner page
            } // endof loop over outer page

        } // endof loop over inner relation

        
        // now that we've scanned over the inner relation, we have
        // enough information to process the page of outer relation
        // tuples
        page::iterator o_it = outer_tuple_page->begin();
        for (int o_index = 0; o_it != outer_tuple_page->end(); ++o_index) {
            if ( matches[o_index] != output_on_match ) 
                _adaptor->output(o_it.advance());
        }
        
    } // endof loop over outer relation
    
}
