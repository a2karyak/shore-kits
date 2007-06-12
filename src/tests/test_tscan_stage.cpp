// -*- mode:C++; c-basic-offset:4 -*-

/** @file    : test_tscan_stage.cpp
 *  @brief   : Unittest for the tscan stage
 *  @author  : Ippokratis Pandis
 */ 

#include "stages.h"
#include "tests/common.h"
#include "workload/common.h"

#include "workload/tpch/q6_packet.h"
#include "workload/tpch/tpch_db.h"
#include "workload/tpch/tpch_env.h"


using namespace tpch;
using namespace workload;
using namespace qpipe;



class test_tscan_stage_process_tuple_t : public process_tuple_t {
public:

    virtual void process(const tuple_t& output) {
	decimal* d = aligned_cast<decimal>(output.data);
	TRACE(TRACE_ALWAYS, "Read ID: EXT=%lf - DISC=%lf\n",
	      d[0].to_double(), d[1].to_double());
    }
    
};


/** @fn    : main
 *  @brief : Calls a table scan and outputs a specific projection
 */

int main() {

    thread_init();
    db_open_guard_t db_open;

    register_stage<tscan_stage_t>(1);


    // TSCAN PACKET
    // the output consists of 2 decimals
    tuple_fifo* tscan_out_buffer = new tuple_fifo(2*sizeof(decimal));
    tuple_filter_t* tscan_filter = new q6_tscan_filter_t(true);


    tscan_packet_t* q6_tscan_packet =
        new tscan_packet_t("TSCAN_PACKET_1",
                           tscan_out_buffer,
                           tscan_filter,
                           tpch_tables[TPCH_TABLE_LINEITEM].db);

    test_tscan_stage_process_tuple_t pt;
    process_query(q6_tscan_packet, pt);

    return 0;
}
