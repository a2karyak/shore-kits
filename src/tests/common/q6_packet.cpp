// -*- mode:C++; c-basic-offset:4 -*-

#include "tests/common/q6_packet.h"

#include "engine/stages/aggregate.h"
#include "engine/stages/tscan.h"
#include "tests/common/q6_aggregate.h"
#include "tests/common/q6_tscan_filter.h"
#include "tests/common/copy_string.h"
#include "tests/common/tpch_db.h"


using namespace qpipe;
using std::string;


packet_t* create_q6_packet(const char* client_prefix) {
  
    string prefix(client_prefix);

    // TSCAN
    string tscan_packet_name = prefix + "_TSCAN_PACKET";
    tuple_buffer_t* tscan_output = new tuple_buffer_t(2*sizeof(double));
    tscan_packet_t *q6_tscan_packet = new tscan_packet_t(copy_string(tscan_packet_name.c_str()),
                                                         tscan_output,
                                                         new q6_tscan_filter_t(),
                                                         tpch_lineitem);
    
    // AGGREGATE
    string agg_packet_name = prefix + "_AGGREGATE_PACKET";
    aggregate_packet_t* q6_agg_packet = new aggregate_packet_t(copy_string(agg_packet_name.c_str()),
                                                               new tuple_buffer_t(2*sizeof(double)),
                                                               new trivial_filter_t(tscan_output->tuple_size),
                                                               new q6_count_aggregate_t(),
                                                               q6_tscan_packet);

    return q6_agg_packet;
}
