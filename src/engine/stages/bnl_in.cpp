/* -*- mode:C++; c-basic-offset:4 -*- */

#include "engine/stages/bnl_in.h"
#include "engine/core/tuple.h"
#include "qpipe_panic.h"
#include "trace.h"

#include <cstring>
#include <vector>

using std::vector;



const char* bnl_in_packet_t::PACKET_TYPE = "BNL_IN";



const char* bnl_in_stage_t::DEFAULT_STAGE_NAME = "BNL_IN_STAGE";



/**
 *  @brief Implement an IN or NOT IN operator on the left and right
 *  relations using a nested loop. We do page-sized reads of both
 *  relations.
 */

stage_t::result_t bnl_in_stage_t::process_packet() {

    adaptor_t* adaptor = _adaptor;
    bnl_in_packet_t* packet = (bnl_in_packet_t*)adaptor->get_packet();
    
    // get ready...
    tuple_buffer_t* left_buffer  = packet->_left_buffer;
    tuple_source_t* right_source = packet->_right_source;
    key_compare_t* _compare = packet->_compare;
    key_extractor_t* _extract = packet->_extract;
    tuple_comparator_t compare(_extract, _compare);
    bool output_on_match = packet->_output_on_match;
    
    
    // dispatch outer relation so we can stream it
    dispatcher_t::dispatch_packet(packet->_left);

 
    
    while (1) {
        
        
        // get another page of tuples from the outer relation
        tuple_page_t* opage;
        int outer_get_ret = left_buffer->get_page(opage);
        assert( outer_get_ret != -1 );
        if ( outer_get_ret )
            // done with outer relation... done with join
            return stage_t::RESULT_STOP;
    
        
        // free outer relation page when done
        page_guard_t outer_tuple_page = opage;
        
        
        // Need a bitmap to track matches between outer relation page
        // and inner relation. So far, we've seen no matches.
        vector<bool> matches(opage->tuple_count(), false);
        
 
        // read the entire inner relation
        packet_t* right_packet = right_source->reset();
        tuple_buffer_t* right_buffer = right_packet->_output_buffer;
        dispatcher_t::dispatch_packet(right_packet);
        

        while (1) {
            
            
            // get another page of tuples from the inner relation
            tuple_page_t* ipage;
            int inner_get_ret = right_buffer->get_page(ipage);
            assert( inner_get_ret != -1 );
            if ( inner_get_ret )
                // done with inner relation... time to output some
                // tuples from opage
                break;
            
            
            // free inner relation page when done
            page_guard_t inner_tuple_page = ipage;
            
            
            // compare each tuple on the outer relation page with each
            // tuple on the inner relation page
            tuple_page_t::iterator o_it;
            int o_index = 0;
            for (o_it = outer_tuple_page->begin(); o_it != outer_tuple_page->end(); ++o_it, ++o_index) {
                
                
                // grab a tuple from the outer relation page
                tuple_t outer_tuple = *o_it;
                hint_tuple_pair_t
                    outer_ktpair(_extract->extract_hint(outer_tuple), outer_tuple.data);

                
                tuple_page_t::iterator i_it;
                for (i_it = inner_tuple_page->begin(); i_it != inner_tuple_page->end(); ++i_it) {

                    
                    // grab a tuple from the inner relation page
                    tuple_t inner_tuple = *i_it;
                    hint_tuple_pair_t
                        inner_ktpair(_extract->extract_hint(inner_tuple), inner_tuple.data);
                    

                    if ( compare(outer_ktpair, inner_ktpair) == 0 )
                        // left and right match!
                        matches[o_index] = true;
                    
                    
                } // endof loop over inner page
            } // endof loop over outer page

        } // endof loop over inner relation

        
        // now that we've scanned over the inner relation, we have
        // enough information to process the page of outer relation
        // tuples


        if ( output_on_match ) {
            // we are processing an IN operator
            tuple_page_t::iterator o_it;
            int o_index = 0;
            for (o_it = outer_tuple_page->begin(); o_it != outer_tuple_page->end(); ++o_it, ++o_index) {
                if ( matches[o_index] ) {
                    result_t result = _adaptor->output(*o_it);
                    if (result)
                        return result;
                }
            }
        }
        else {
            // we are processing a NOT IN operator
            tuple_page_t::iterator o_it;
            int o_index = 0;
            for (o_it = outer_tuple_page->begin(); o_it != outer_tuple_page->end(); ++o_it, ++o_index) {
                if ( !matches[o_index] ) {
                    result_t result = _adaptor->output(*o_it);
                    if (result)
                        return result;
                }
            }
        }
        
        
    } // endof loop over outer relation
    
    
    return stage_t::RESULT_STOP;
}
