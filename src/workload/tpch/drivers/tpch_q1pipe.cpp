/* -*- mode:C++; c-basic-offset:4 -*- */
#include "stages.h"
#include "scheduler.h"

#include "workload/tpch/drivers/tpch_q1pipe.h"
#include "workload/tpch/drivers/tpch_q1_types.h"
#include "workload/tpch/tpch_struct.h"
#include "workload/tpch/tpch_type_convert.h"
#include "workload/tpch/tpch_env.h"
#include "workload/common.h"

using namespace q1;


ENTER_NAMESPACE(q1pipe);


EXIT_NAMESPACE(q1pipe);


using namespace q1pipe;
using namespace workload;
using namespace qpipe;


ENTER_NAMESPACE(workload);


void tpch_q1pipe_driver::submit(void* disp) {

    scheduler::policy_t* dp = (scheduler::policy_t*)disp;
  
    // TSCAN PACKET
    struct cdb_table_s* tpch_lineitem = &tpch_tables[TPCH_TABLE_LINEITEM];
    tuple_fifo* tscan_output = new tuple_fifo(tpch_lineitem->tuple_size);
    tscan_packet_t* q1_tscan_packet =
        new tscan_packet_t("Q1PIPE_TSCAN_PACKET",
                           tscan_output,
                           new trivial_filter_t(tscan_output->tuple_size()),
                           tpch_lineitem->db);
    
    // ECHO (filter)
    tuple_fifo* echo_output = new tuple_fifo(sizeof(projected_lineitem_tuple));
    echo_packet_t* q1_echo_packet =
        new echo_packet_t("Q1PIPE_ECHO_PACKET",
                          echo_output,
                          new q1_tscan_filter_t(),
                          q1_tscan_packet);
    
    // AGG PACKET CREATION
    tuple_fifo* agg_output_buffer = new tuple_fifo(sizeof(aggregate_tuple));
    packet_t* q1_agg_packet =
        new partial_aggregate_packet_t("Q1PIPE_AGG_PACKET",
                                       agg_output_buffer,
                                       new trivial_filter_t(agg_output_buffer->tuple_size()),
                                       q1_echo_packet,
                                       new q1_count_aggregate_t(),
                                       new q1_key_extract_t(),
                                       new int_key_compare_t());

    qpipe::query_state_t* qs = dp->query_state_create();
    q1_agg_packet->assign_query_state(qs);
    q1_echo_packet->assign_query_state(qs);
    q1_tscan_packet->assign_query_state(qs);

    tpch_q1_process_tuple_t pt;
    process_query(q1_agg_packet, pt);
    dp->query_state_destroy(qs);
}


EXIT_NAMESPACE(workload);
