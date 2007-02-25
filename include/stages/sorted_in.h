/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef __SORTED_IN_H
#define __SORTED_IN_H

#include "core.h"

using namespace qpipe;

struct sorted_in_packet_t : public packet_t {
    static const c_str PACKET_TYPE;
    guard<packet_t> _left;
    guard<packet_t> _right;
    guard<tuple_fifo> _left_input;
    guard<tuple_fifo> _right_input;
    guard<key_extractor_t> _left_extractor;
    guard<key_extractor_t> _right_extractor;
    guard<key_compare_t> _compare;
    bool _reject_matches;

    sorted_in_packet_t(const c_str &packet_id,
                       tuple_fifo* out_buffer,
                       tuple_filter_t* out_filter,
                       packet_t* left, packet_t* right,
                       key_extractor_t* left_extractor,
                       key_extractor_t* right_extractor,
                       key_compare_t* compare, bool reject_matches)
        : packet_t(packet_id, PACKET_TYPE, out_buffer, out_filter, NULL,
                   false, /* merging not allowed */
                   true   /* unreserve worker on completion */
                   ),
          _left(left), _right(right),
          _left_input(left->output_buffer()),
          _right_input(right->output_buffer()),
          _left_extractor(left_extractor),
          _right_extractor(right_extractor),
          _compare(compare), _reject_matches(reject_matches)
    {
    }

    virtual void declare_worker_needs(resource_declare_t* declare) {
        declare->declare(_packet_type, 1);
        _left->declare_worker_needs(declare);
        _right->declare_worker_needs(declare);
    }
};

struct sorted_in_stage_t : public stage_t {
    static const c_str DEFAULT_STAGE_NAME;
    typedef sorted_in_packet_t stage_packet_t;
protected:
    virtual void process_packet();
};

#endif
