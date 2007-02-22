// -*- mode:C++; c-basic-offset:4 -*-

/** @file    : test_tscan_stage.cpp
 *  @brief   : Unittest for the tscan stage
 *  @author  : Ippokratis Pandis
 *  @version : 0.1
 *  @history :
 8/6/2006 : Updated to work with the new class definitions
 25/5/2006: Initial version
*/ 

#include "stages.h"
#include "tests/common.h"
#include "workload/common.h"
#include "workload/tpch/tpch_db.h"
#include "workload/common/register_stage.h"


using namespace qpipe;




/** @fn    : main
 *  @brief : Calls a table scan and outputs a specific projection
 */

int main() {

    thread_init();
    db_open();

    register_stage<tscan_stage_t>(1);


    // TSCAN PACKET
    // the output consists of 2 decimals
    tuple_fifo* tscan_out_buffer = new tuple_fifo(2*sizeof(decimal));
    tuple_filter_t* tscan_filter = new q6_tscan_filter_t(true);


    tscan_packet_t* q6_tscan_packet =
        new tscan_packet_t("TSCAN_PACKET_1" , tscan_out_buffer, tscan_filter,
                           tpch_tables[TPCH_TABLE_LINEITEM].db);

    // Dispatch packet
    reserve_query_workers(q6_tscan_packet);
    dispatcher_t::dispatch_packet(q6_tscan_packet);
    
    tuple_t output;
    decimal* d = NULL;
    while(tscan_out_buffer->get_tuple(output)) {
	d = aligned_cast<decimal>(output.data);
	TRACE(TRACE_ALWAYS, "Read ID: EXT=%lf - DISC=%lf\n",
	      d[0].to_double(), d[1].to_double());
    }


    db_close();
    return 0;
}
