/* -*- mode:C++; c-basic-offset:4 -*- */

#include "engine/stages/merge.h"
#include <list>

using std::list;



const char *merge_packet_t::PACKET_TYPE = "MERGE";



const char *merge_stage_t::DEFAULT_STAGE_NAME = "MERGE_STAGE";



/**
 * This merge stage maintains a linked list of input buffers, sorted
 * by the next data item in each buffer, in order to choose the next
 * buffer to read from.
 *
 * If merges of more than 8-10 runs are common the code should be
 * extended to use a modified heap (see note below) as well as -- or
 * instead of -- the list.
 *
 * A 7-way merge using a list requires 7/2 = 3.5 and < 1 comparisons
 * and swaps, respectively; a 15-way merge requires 7.5 and < 1
 * comparisons and swaps on average.
 *
 * Using a heap for a 7-way merge one can expect to make 2*(1*1 +
 * 2*6)/7 = 3.7 comparisons and (0*1 + 1*2 + 2*4)/7 = 1.4 swaps per
 * tuple read; a 15-way merge requires 2*(1*1 + 2*2 + 3*12)/15 = ~5.5
 * comparisons and (0*1 + 1*2 + 2*4 + 3*8)/15 = ~2.3 swaps, on
 * average.
 *
 * NOTE: merging is a special case operation where we mutate the 'min'
 * element rather than removing it. Most generic data structures would
 * require removing and reinserting the element to maintain proper
 * ordering. This is fine lists where the remove operation trivial,
 * but would increase the number of comparisons and swaps required for
 * the heap by up to 2x and make it even less attractive than shown
 * above.
 */
bool merge_stage_t::buffer_head_t::init(tuple_buffer_t *buf,
                                        key_extractor_t* extract)
{
    buffer = buf;
    _extract = extract;
    int size = buffer->tuple_size;
    data = new char[size];
    tuple = tuple_t(data, size);
    item.data = data;
    return has_tuple();
}



/**
 *  @return 0 if a tuple is removed from the buffer. 1 if the buffer
 *  is empty and the producer has sent EOF. -1 if the buffer has been
 *  terminated.
 */
int merge_stage_t::buffer_head_t::has_tuple() {

    tuple_t input;

    int get_ret = buffer->get_tuple(input);
    if (get_ret)
        return get_ret;

    // otherwise, we got a tuple
    
    // copy the tuple to safe memory
    tuple.assign(input);
    
    // update the head key
    item.hint = _extract->extract_hint(tuple);
    return 0;
}


int merge_stage_t::compare(const hint_tuple_pair_t &a, const hint_tuple_pair_t &b) {
    return tuple_comparator_t(_extract, _compare)(a, b);
}

/**
 *  @brief Inserts an item into the list in ascending order.
 */
void merge_stage_t::insert_sorted(buffer_head_t *head)
{
    // beginning? (if so we have to change the list base pointer)
    if(!_head_list || compare(head->item, _head_list->item) <= 0) {
        head->next = _head_list;
        _head_list = head;
        return;
    }

    // find the position, then
    buffer_head_t *prev = _head_list;
    while(prev->next && compare(prev->next->item, head->item) < 0)
        prev = prev->next;
    
    // insert
    head->next = prev->next;
    prev->next = head;
}



stage_t::result_t merge_stage_t::process_packet() {

    typedef merge_packet_t::buffer_list_t buffer_list_t;


    merge_packet_t *packet = (merge_packet_t *)_adaptor->get_packet();

    _compare = packet->_compare;
    _extract = packet->_extract;
    buffer_list_t &inputs = packet->_input_buffers;

    
    // allocate an array of buffer heads
    int merge_factor = inputs.size();
    buffer_head_t head_array[merge_factor];
    TRACE(TRACE_DEBUG, "Processing %d-way merge\n", merge_factor);

                             
    // get the input buffers and perform the initial sort
    for(int i=0; i < merge_factor; i++) {
        buffer_head_t &head = head_array[i];
        switch (head.init(inputs[i], _extract)) {
        case 0:
            insert_sorted(&head);
            continue;
        case 1:
            continue;
        case -1:
            // error!
            return stage_t::RESULT_ERROR;
        default:
            // unrecognized value
            QPIPE_PANIC();
        }
    }


    // always output the smallest tuple
    for(int i=0; _head_list; i++) {


        // pop it off
        buffer_head_t *head = _head_list;
        _head_list = head->next;


        // output it
        result_t result = _adaptor->output(head->tuple);
	if (result)
	    return result;
        
        
        // put it back?
        switch (head->has_tuple()) {
        case 0:
            // run has a tuple
            insert_sorted(head);
            continue;
        case 1:
            // run has no more tuples... do nothing
            continue;
        case -1:
            // error!
            return stage_t::RESULT_ERROR;
        default:
            // unrecognized value
            QPIPE_PANIC();
        }
    }
    
    
    // done!
    return stage_t::RESULT_STOP;
}
