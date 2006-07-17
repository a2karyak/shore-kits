/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef __PARTIAL_AGGREGATE_H
#define __PARTIAL_AGGREGATE_H

#include "engine/thread.h"
#include "engine/core/stage.h"
#include <set>

using namespace qpipe;

struct partial_aggregate_packet_t : public packet_t {
    static const c_str PACKET_TYPE;
    pointer_guard_t<packet_t> _input;
    pointer_guard_t<tuple_buffer_t> _input_buffer;
    pointer_guard_t<tuple_aggregate_t> _aggregate;
    pointer_guard_t<key_extractor_t> _extractor;
    pointer_guard_t<key_compare_t> _compare;

    partial_aggregate_packet_t(const c_str    &packet_id,
                               tuple_buffer_t* out_buffer,
                               tuple_filter_t* out_filter,
                               packet_t* input,
                               tuple_aggregate_t *aggregate,
                               key_extractor_t* extractor,
                               key_compare_t* compare)
        : packet_t(packet_id, PACKET_TYPE, out_buffer, out_filter),
          _input(input), _input_buffer(input->_output_buffer),
          _aggregate(aggregate), _extractor(extractor), _compare(compare)
    {
    }

};

class partial_aggregate_stage_t : public stage_t {
    typedef std::set<hint_tuple_pair_t, tuple_less_t> tuple_set_t;

    page_list_guard_t _page_list;
    size_t _page_count;
    tuple_aggregate_t* _aggregate;
    tuple_page_t* _agg_page;
public:
    static const c_str DEFAULT_STAGE_NAME;
    typedef partial_aggregate_packet_t stage_packet_t;

protected:
    virtual void process_packet();
    int alloc_agg(hint_tuple_pair_t &agg, const char* key);
};

#endif
