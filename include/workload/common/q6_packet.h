// -*- mode:C++; c-basic-offset:4 -*-

#ifndef _Q6_PACKET_H
#define _Q6_PACKET_H


#include "core.h"
#include "scheduler.h"
#include "workload/common/process_tuple.h"
#include "workload/common/q6_aggregate.h"

using namespace qpipe;
using namespace scheduler;
using namespace workload;


class q6_process_tuple_t : public process_tuple_t {
public:
    virtual void process(const tuple_t& output) {
        q6_count_aggregate_t::agg_t* r =
            aligned_cast<q6_count_aggregate_t::agg_t>(output.data);
        TRACE(TRACE_QUERY_RESULTS,
              "*** Q6 Count: %u. Sum: %.2f.  ***\n", r->count, (float)r->sum.to_double());
    }
};


packet_t* create_q6_packet(const c_str &client_prefix, policy_t* dp);


#endif
