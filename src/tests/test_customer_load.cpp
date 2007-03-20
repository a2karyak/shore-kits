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
#include "workload/tpch/tpch_struct.h"
#include "workload/common/register_stage.h"


using namespace qpipe;
using namespace workload;


class test_customer_load_process_tuple_t : public process_tuple_t {
public:
    
    virtual void begin() {
        TRACE(TRACE_ALWAYS, "sizeof(tpch_customer_tuple) = %d\n",
              sizeof(tpch_customer_tuple));
    }

    virtual void process(const tuple_t& output) {
	struct tpch_customer_tuple* t =
            aligned_cast<struct tpch_customer_tuple>(output.data);
	TRACE(TRACE_ALWAYS, "C_CUSTKEY %d\n", t->C_CUSTKEY);
    }
    
};


int main() {

    thread_init();
    db_open_guard_t db_open;

    register_stage<tscan_stage_t>(1);

    tuple_fifo* tscan_out_buffer =
        new tuple_fifo(sizeof(struct tpch_customer_tuple));
    tuple_filter_t* tscan_filter =
        new trivial_filter_t(sizeof(struct tpch_customer_tuple));
    

    tscan_packet_t* tscan_packet =
        new tscan_packet_t("TSCAN_PACKET_1" , tscan_out_buffer, tscan_filter,
                           tpch_tables[TPCH_TABLE_CUSTOMER].db);

    test_customer_load_process_tuple_t pt;
    process_query(tscan_packet, pt);

    return 0;
}
