/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef __MERGE_H
#define __MERGE_H

#include "engine/core/stage.h"
#include "engine/dispatcher.h"

#include <vector>

using std::vector;



using namespace qpipe;



/**
 *  @brief Packet definition for an N-way merge stage
 */
struct merge_packet_t : public packet_t {

    static const c_str PACKET_TYPE;
    typedef vector<tuple_fifo*> buffer_list_t;
    
    buffer_list_t _input_buffers;
    guard<key_extractor_t> _extract;
    guard<key_compare_t>   _compare;


    /**
     *  @brief aggregate_packet_t constructor.
     *
     *  @param packet_id The ID of this packet. This should point to a
     *  block of bytes allocated with malloc(). This packet will take
     *  ownership of this block and invoke free() when it is
     *  destroyed.
     *
     *  @param output_buffer The buffer where this packet should send
     *  its data. A packet DOES NOT own its output buffer (we will not
     *  invoke delete or free() on this field in our packet
     *  destructor).
     *
     *  @param output_filter The filter that will be applied to any
     *  tuple sent to output_buffer. The packet OWNS this filter. It
     *  will be deleted in the packet destructor.
     *
     *  @param output_filter The filter that will be applied to any
     *  tuple sent to output_buffer. The packet OWNS this filter. It
     *  will be deleted in the packet destructor.
     *
     *  @param input_buffers A list of tuple_fifo pointers. This
     *  is the set of inputs that we are merging. This list should be
     *  set up by the meta-stage that creates this merge_packet_t. We
     *  will take ownership of the tuple_fifo's, but not the list
     *  itself. We will copy the list.
     *
     *  @param comparator A comparator for the tuples in our input
     *  buffers. This packet owns this comparator.
     */
    merge_packet_t(const c_str         &packet_id,
                   tuple_fifo*      output_buffer,
                   tuple_filter_t*      output_filter,
                   const buffer_list_t& input_buffers,
                   key_extractor_t* extract,
                   key_compare_t*  compare)
	: packet_t(packet_id, PACKET_TYPE, output_buffer, output_filter, false),
          _input_buffers(input_buffers),
          _extract(extract),
          _compare(compare)
    {
    }
    
    ~merge_packet_t() {
        buffer_list_t::iterator it=_input_buffers.begin();
        while(it != _input_buffers.end())
            delete *it++;
    }

};



/**
 *  @brief Merge stage that merges N sorted inputs into one sorted
 *  output run.
 */
class merge_stage_t : public stage_t {

private:

    struct buffer_head_t {
        // for the linked list
        buffer_head_t*   next;
        tuple_fifo*  buffer;
        key_extractor_t* _extract;
        array_guard_t<char> data;
        tuple_t tuple;
        hint_tuple_pair_t item;
        buffer_head_t() { }
        bool init(tuple_fifo* buf, key_extractor_t* c);
        bool has_tuple();
    };
    
    buffer_head_t*   _head_list;
    key_compare_t*   _compare;
    key_extractor_t* _extract;
    
public:

    static const c_str DEFAULT_STAGE_NAME;
    typedef merge_packet_t stage_packet_t;

    merge_stage_t()
        : _head_list(NULL)
    {
    }
    
protected:

    virtual void process_packet();
    
private:

    void insert_sorted(buffer_head_t* head);
    int compare(const hint_tuple_pair_t &a, const hint_tuple_pair_t &b);
};



#endif
