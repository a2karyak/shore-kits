// -*- mode:C++; c-basic-offset:4 -*-

#include "workload/common/q6_packet.h"

#include "engine/stages/aggregate.h"
#include "engine/stages/tscan.h"
#include "workload/common/q6_aggregate.h"
#include "workload/common/q6_tscan_filter.h"
#include "workload/tpch/tpch_db.h"
#include "workload/common/copy_string.h"


using namespace qpipe;
using std::string;


packet_t* create_q6_packet(const c_str &client_prefix, dispatcher_policy_t* dp) {

    // TSCAN
    tuple_buffer_t* tscan_output = new tuple_buffer_t(2*sizeof(double));
    tscan_packet_t *q6_tscan_packet = new tscan_packet_t(c_str("%s_TSCAN_PACKET", client_prefix.data()).data(),
                                                         tscan_output,
                                                         new q6_tscan_filter_t(),
                                                         tpch_lineitem);
    
    // AGGREGATE
    aggregate_packet_t* q6_agg_packet = new aggregate_packet_t(copy_string(c_str("%s_AGGREGATE_PACKET", client_prefix.data()).data()),
                                                               new tuple_buffer_t(2*sizeof(double)),
                                                               new trivial_filter_t(tscan_output->tuple_size),
                                                               new q6_count_aggregate_t(),
                                                               new default_key_extractor_t(0),
                                                               q6_tscan_packet);
    
    dispatcher_policy_t::query_state_t* qs = dp->query_state_create();
    dp->assign_packet_to_cpu(q6_agg_packet, qs);
    dp->assign_packet_to_cpu(q6_tscan_packet, qs);
    dp->query_state_destroy(qs);

    //    return q6_agg_packet;
    return q6_tscan_packet;
}
