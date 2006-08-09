// -*- mode:C++; c-basic-offset:4 -*-

#include "engine/stages/partial_aggregate.h"
#include "engine/functors.h"
#include "engine/dispatcher.h"



const c_str partial_aggregate_packet_t::PACKET_TYPE = "PARTIAL_AGGREGATE";



const c_str partial_aggregate_stage_t::DEFAULT_STAGE_NAME = "PARTIAL_AGGREGATE_STAGE";



// the maximum number of pages allowed in a single run
static const size_t MAX_RUN_PAGES = 10000;



int partial_aggregate_stage_t::alloc_agg(hint_tuple_pair_t &agg, const char* key) {
    // out of space?
    if(!_agg_page || _agg_page->full()) {
        if(_page_count >= MAX_RUN_PAGES)
            return 1;

        _agg_page = tuple_page_t::alloc(_aggregate->tuple_size());
        _page_list.append(_agg_page);
        _page_count++;
    }

    // allocate the tuple
    agg.data = _agg_page->allocate_tuple();

    // initialize the data
    _aggregate->init(agg.data);
    
    // copy over the key
    key_extractor_t* extract = _aggregate->key_extractor();
    char* agg_key = extract->extract_key(agg.data);
    memcpy(agg_key, key, extract->key_size());
    return 0;
}

void partial_aggregate_stage_t::process_packet() {

    partial_aggregate_packet_t* packet;
    packet = (partial_aggregate_packet_t*) _adaptor->get_packet();
    tuple_fifo* input_buffer = packet->_input_buffer;
    dispatcher_t::dispatch_packet(packet->_input);
    _aggregate = packet->_aggregate;
    key_extractor_t* agg_key = _aggregate->key_extractor();
    key_extractor_t* tup_key = packet->_extractor;
    key_compare_t* compare = packet->_compare;
    
    
    // create a set to hold the sorted run
    tuple_less_t less(agg_key, compare);
    tuple_set_t run(less);

    // read in the tuples and aggregate them in the set
    while(!input_buffer->eof()) {
        _page_list.done();
        _page_count = 0;
        _agg_page = NULL;
        while(_page_count < MAX_RUN_PAGES && !input_buffer->eof()) {
            page_guard_t page = NULL;
            tuple_t in;
            while(1) {
                // out of pages?
                if(!input_buffer->get_tuple(in))
                   break;

                int hint = tup_key->extract_hint(in);

                // fool the aggregate's key extractor into thinking
                // the tuple is an aggregate. Use pointer math to put
                // the tuple's key bits where the aggregate's key bits
                // are supposed to go. (the search only touches the
                // key bits anyway, which are guaranteed to be the
                // same for both...)
                size_t offset = agg_key->key_offset();
                char* key_data = tup_key->extract_key(in);
                hint_tuple_pair_t key(hint, key_data - offset);
                
                // find the lowest aggregate such that candidate >=
                // key. This is either the aggregate we want or a good
                // hint for the insertion that otherwise follows
                tuple_set_t::iterator candidate = run.lower_bound(key);
                if(candidate == run.end() || less(key, *candidate)) {
                    // initialize a blank aggregate tuple
                    hint_tuple_pair_t agg(hint, NULL);
                    if(alloc_agg(agg, key_data))
                        break;
                    
                    // insert the new aggregate tuple
                    candidate = run.insert(candidate, agg);
                }
                else {
                    TRACE(TRACE_DEBUG, "Merging a tuple\n");
                }

                // update an existing aggregate tuple (which may have
                // just barely been inserted)
                _aggregate->aggregate(candidate->data, in);
            }
        }

        // TODO: handle cases where the run doesn't fit in memory
        assert(input_buffer->eof());

        // write out the result
        size_t out_size = packet->_output_filter->input_tuple_size();
        char out_data[out_size];
        tuple_t out(out_data, out_size);
        for(tuple_set_t::iterator it=run.begin(); it != run.end(); ++it) {
            // convert the aggregate tuple to an output tuple
            _aggregate->finish(out, it->data);
            _adaptor->output(out);
        }
    }
}
