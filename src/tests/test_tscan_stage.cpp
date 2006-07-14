// -*- mode:C++; c-basic-offset:4 -*-

/** @file    : test_tscan_stage.cpp
 *  @brief   : Unittest for the tscan stage
 *  @author  : Ippokratis Pandis
 *  @version : 0.1
 *  @history :
 8/6/2006 : Updated to work with the new class definitions
 25/5/2006: Initial version
*/ 

#include "engine/thread.h"
#include "engine/core/stage_container.h"
#include "engine/stages/tscan.h"
#include "engine/dispatcher.h"
#include "trace.h"
#include "qpipe_panic.h"
#include "tests/common.h"
#include "workload/common.h"
#include "workload/tpch/tpch_db.h"


using namespace qpipe;




/** @fn    : main
 *  @brief : Calls a table scan and outputs a specific projection
 */

int main() {

    thread_init();

    if ( !db_open() ) {
        TRACE(TRACE_ALWAYS, "db_open() failed\n");
        QPIPE_PANIC();
    }        


    register_stage<tscan_stage_t>(1);


    // TSCAN PACKET
    // the output consists of 2 doubles
    tuple_buffer_t* tscan_out_buffer = new tuple_buffer_t(2*sizeof(double));
    tuple_filter_t* tscan_filter = new q6_tscan_filter_t(true);


    tscan_packet_t* q6_tscan_packet =
        new tscan_packet_t("TSCAN_PACKET_1" , tscan_out_buffer, tscan_filter, tpch_lineitem);

    // Dispatch packet
    dispatcher_t::dispatch_packet(q6_tscan_packet);
    
    tuple_t output;
    double* d = NULL;
    while(!tscan_out_buffer->get_tuple(output)) {
	d = (double*)output.data;
	TRACE(TRACE_ALWAYS, "Read ID: EXT=%lf - DISC=%lf\n", d[0], d[1]);
    }


    if ( !db_close() )
        TRACE(TRACE_ALWAYS, "db_close() failed\n");
    return 0;
}
