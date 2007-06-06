/* -*- mode:C++; c-basic-offset:4 -*- */
#include "stages.h"
#include "scheduler.h"

#include "workload/tpch/drivers/tpch_q1.h"
#include "workload/tpch/drivers/tpch_q1_types.h"
#include "workload/tpch/tpch_env.h"
#include "workload/common.h"

using namespace q1;
using namespace tpch;
using namespace workload;


ENTER_NAMESPACE(workload);


using namespace qpipe;


void tpch_q1_driver::submit(void* disp) {

    scheduler::policy_t* dp = (scheduler::policy_t*)disp;
  
    // TSCAN PACKET
    tuple_fifo* tscan_out_buffer =
        new tuple_fifo(sizeof(projected_lineitem_tuple));
    tscan_packet_t* q1_tscan_packet =
        new tscan_packet_t("lineitem TSCAN",
                           tscan_out_buffer,
                           new q1_tscan_filter_t(),
                           tpch_tables[TPCH_TABLE_LINEITEM].db);

    // AGG PACKET CREATION
    tuple_fifo* agg_output_buffer =
        new tuple_fifo(sizeof(aggregate_tuple));
    packet_t* q1_agg_packet;
    q1_agg_packet = 
        new partial_aggregate_packet_t("Q1_AGG_PACKET",
                                       agg_output_buffer,
                                       new trivial_filter_t(agg_output_buffer->tuple_size()),
                                       q1_tscan_packet,
                                       new q1_count_aggregate_t(),
                                       new q1_key_extract_t(),
                                       new int_key_compare_t());

    qpipe::query_state_t* qs = dp->query_state_create();
    q1_agg_packet->assign_query_state(qs);
    q1_tscan_packet->assign_query_state(qs);
        

    tpch_q1_process_tuple_t pt;
    process_query(q1_agg_packet, pt);


    dp->query_state_destroy(qs);
}


EXIT_NAMESPACE(workload);
