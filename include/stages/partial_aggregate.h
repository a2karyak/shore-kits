/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef __PARTIAL_AGGREGATE_H
#define __PARTIAL_AGGREGATE_H

#include "core.h"

#include <set>

using namespace qpipe;

struct partial_aggregate_packet_t : public packet_t {
    static const c_str PACKET_TYPE;
    guard<packet_t> _input;
    guard<tuple_fifo> _input_buffer;
    guard<tuple_aggregate_t> _aggregate;
    guard<key_extractor_t> _extractor;
    guard<key_compare_t> _compare;

    partial_aggregate_packet_t(const c_str    &packet_id,
                               tuple_fifo* out_buffer,
                               tuple_filter_t* out_filter,
                               packet_t* input,
                               tuple_aggregate_t *aggregate,
                               key_extractor_t* extractor,
                               key_compare_t* compare)
        : packet_t(packet_id, PACKET_TYPE, out_buffer, out_filter,
                   create_plan(out_filter, aggregate, extractor, input->plan())),
          _input(input), _input_buffer(input->output_buffer()),
          _aggregate(aggregate), _extractor(extractor), _compare(compare)
    {
    }

    // TODO: consider the key comparator as well
    static query_plan* create_plan(tuple_filter_t* filter, tuple_aggregate_t* agg,
                                   key_extractor_t* key, query_plan const* child)
    {
        c_str action("%s:%s:%s", PACKET_TYPE.data(),
                     agg->to_string().data(), key->to_string().data());

        query_plan const** children = new query_plan const*[1];
        children[0] = child;
        return new query_plan(action, filter->to_string(), children, 1);
    }
    
};

class partial_aggregate_stage_t : public stage_t {
    typedef std::set<hint_tuple_pair_t, tuple_less_t> tuple_set_t;

    page_trash_stack _page_list;
    size_t _page_count;
    tuple_aggregate_t* _aggregate;
    page* _agg_page;
    size_t _tuple_align;
public:
    static const c_str DEFAULT_STAGE_NAME;
    typedef partial_aggregate_packet_t stage_packet_t;

protected:
    virtual void process_packet();
    int alloc_agg(hint_tuple_pair_t &agg, const char* key);
};

#endif
