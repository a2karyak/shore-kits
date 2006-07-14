/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef __SORTED_IN_H
#define __SORTED_IN_H

#include "engine/thread.h"
#include "engine/core/stage.h"

using namespace qpipe;

struct sorted_in_packet_t : public packet_t {
    static const c_str PACKET_TYPE;
    pointer_guard_t<packet_t> _left;
    pointer_guard_t<packet_t> _right;
    buffer_guard_t _left_input;
    buffer_guard_t _right_input;
    pointer_guard_t<key_extractor_t> _left_extractor;
    pointer_guard_t<key_extractor_t> _right_extractor;
    pointer_guard_t<key_compare_t> _compare;
    bool _reject_matches;

    sorted_in_packet_t(const char* packet_id,
                       tuple_buffer_t* out_buffer,
                       tuple_filter_t* out_filter,
                       packet_t* left, packet_t* right,
                       key_extractor_t* left_extractor,
                       key_extractor_t* right_extractor,
                       key_compare_t* compare, bool reject_matches)
        : packet_t(packet_id, PACKET_TYPE, out_buffer, out_filter),
          _left(left), _right(right),
          _left_input(left->_output_buffer),
          _right_input(right->_output_buffer),
          _left_extractor(left_extractor),
          _right_extractor(right_extractor),
          _compare(compare), _reject_matches(reject_matches)
    {
    }

};

struct sorted_in_stage_t : public stage_t {
    static const c_str DEFAULT_STAGE_NAME;
    typedef sorted_in_packet_t stage_packet_t;
protected:
    virtual void process_packet();
};

#endif
