// -*- mode:C++; c-basic-offset:4 -*-

#include "workload/common/q6_packet.h"

#include "stages.h"
#include "workload/common/q6_aggregate.h"
#include "workload/common/q6_tscan_filter.h"
#include "workload/tpch/tpch_db.h"


using namespace qpipe;


packet_t* create_q6_packet(const c_str &client_prefix, scheduler::policy_t* dp) {

    // TSCAN
    tuple_fifo* tscan_output = new tuple_fifo(2*sizeof(double));
    tscan_packet_t *q6_tscan_packet;
    q6_tscan_packet = new tscan_packet_t("lineitem TSCAN",
                                         tscan_output,
                                         new q6_tscan_filter_t(),
                                         tpch_lineitem);
    
    // AGGREGATE
    aggregate_packet_t* q6_agg_packet;
    q6_agg_packet = new aggregate_packet_t(c_str("%s_AGGREGATE_PACKET", client_prefix.data()),
                                           new tuple_fifo(2*sizeof(double)),
                                           new trivial_filter_t(tscan_output->tuple_size()),
                                           new q6_count_aggregate_t(),
                                           new default_key_extractor_t(0),
                                           q6_tscan_packet);
    
    qpipe::query_state_t* qs = dp->query_state_create();
    q6_agg_packet->assign_query_state(qs);
    q6_tscan_packet->assign_query_state(qs);

    if(1)
        return q6_agg_packet;
    else
        return q6_tscan_packet;
}
