/* -*- mode:C++; c-basic-offset:4 -*- */

#include "engine/stages/bnl_join.h"
#include "engine/core/tuple.h"
#include "qpipe_panic.h"
#include "trace.h"

#include <cstring>



const char* bnl_join_packet_t::PACKET_TYPE = "BNL_JOIN";



const char* bnl_join_stage_t::DEFAULT_STAGE_NAME = "BNL_JOIN_STAGE";



/**
 *  @brief Join the left and right relations using a nested
 *  loop. Instead of reading the inner relation for every outer
 *  relation tuple, we do page-sized reads.
 */

stage_t::result_t bnl_join_stage_t::process_packet() {

    adaptor_t* adaptor = _adaptor;
    bnl_join_packet_t* packet = (bnl_join_packet_t*)adaptor->get_packet();
    
    tuple_buffer_t* left_buffer  = packet->_left_buffer;
    tuple_source_t* right_source = packet->_right_source;
    tuple_join_t*   join = packet->_join;
    
    
    // dispatch outer relation so we can stream it
    dispatcher_t::dispatch_packet(packet->_left);


    // get ready...
    size_t key_size = join->key_size();
    char outer_key[key_size];
    char inner_key[key_size];
    char output_data[join->out_tuple_size()];
    tuple_t output_tuple(output_data, sizeof(output_data));
    
    
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
        

        // re-read the inner relation
        packet_t* right_packet = right_source->reset();
        tuple_buffer_t* right_buffer = right_packet->_output_buffer;
        dispatcher_t::dispatch_packet(right_packet);
        
        
        while (1) {
            
            
            // get another page of tuples from the inner relation
            tuple_page_t* ipage;
            int inner_get_ret = right_buffer->get_page(ipage);
            assert( inner_get_ret != -1 );
            if ( inner_get_ret )
                // done with inner relation... continue to next page
                // of outer relation
                break;
            
            
            // free inner relation page when done
            page_guard_t inner_tuple_page = ipage;
            
            
            // join each tuple on the outer relation page with each
            // tuple on the inner relation page
            tuple_page_t::iterator o_it;
            for (o_it = outer_tuple_page->begin(); o_it != outer_tuple_page->end(); ++o_it) {
                
                tuple_t outer_tuple = *o_it;
                join->get_left_key(outer_key, outer_tuple);

                tuple_page_t::iterator i_it;
                for (i_it = inner_tuple_page->begin(); i_it != inner_tuple_page->end(); ++i_it) {

                    tuple_t inner_tuple = *i_it;
                    join->get_right_key(inner_key, inner_tuple);
                    if ( memcmp( outer_key, inner_key, key_size ) == 0 ) {
                        // keys match!
                        join->join(output_tuple, outer_tuple, inner_tuple);
                        result_t result = _adaptor->output(output_tuple);
                        if (result)
                            return result;
                    }
                } // endof loop over inner page
            } // endof loop over outer page

        } // endof loop over inner relation
    } // endof loop over outer relation


    return stage_t::RESULT_STOP;
}
