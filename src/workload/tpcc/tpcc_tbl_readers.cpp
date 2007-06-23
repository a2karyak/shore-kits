/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file tpcc_tbl_readers.cpp
 *
 *  @brief Implementation of the TPC-C table reading functions, used only
 *  for debugging purposes
 *
 *  @author Ippokratis Pandis (ipandis)
 */

/*
#include "workload/common/bdb_env.h"
//#include "workload/tpcc/tpcc_type_convert.h"
*/


#include "stages.h"
#include "scheduler.h"

#include "util/trace.h"

#include "stages/tpcc/common/tpcc_struct.h"

#include "workload/common.h"

#include "workload/tpcc/tpcc_env.h"
#include "workload/tpcc/tpcc_tbl_readers.h"
#include "workload/tpcc/tpcc_filenames.h"



ENTER_NAMESPACE(tpcc);



/** Definitions of internal helper class and functions */

static const int interval = 1000;
static int row_cnt = 0;


class tpcc_customer_process_tuple_t : public process_tuple_t {

public:
  
  virtual void process(const tuple_t& output) {

    tpcc_customer_tuple* c = aligned_cast<tpcc_customer_tuple>(output.data);

    //    if ( (row_cnt++ % interval) == 0) {
      TRACE(TRACE_ALWAYS, 
            "*** CUSTOMER: %d %d %d %s %s\n",
            c->C_C_ID,
            c->C_D_ID,
            c->C_W_ID,
            c->C_LAST,
            c->C_FIRST);
      
      //    }
  }

};



/** Definitions of exported functions */


/** tpc-c CUSTOMER table scan */

void tpcc_read_tbl_CUSTOMER  (Db* db) {

    TRACE(TRACE_DEBUG, "Reading TPC-C CUSTOMER...\n");

    scheduler::policy_t* dp = new scheduler::policy_os_t();
    qpipe::query_state_t* qs = dp->query_state_create();
    

    TRACE( TRACE_ALWAYS, "%d\n", sizeof(tpcc_customer_tuple));

    // CUSTOMER scan
    tuple_filter_t* filter = new trivial_filter_t(sizeof(tpcc_customer_tuple));
    tuple_fifo* output = new tuple_fifo(sizeof(tpcc_customer_tuple));
    packet_t* tscan_customer_packet;

    tscan_customer_packet = new tscan_packet_t("TPCC-CUSTOMER-TSCAN",
                                               output,
                                               filter,
                                               tpcc_tables[TPCC_TABLE_CUSTOMER].db);
    tscan_customer_packet->assign_query_state(qs);


    tpcc_customer_process_tuple_t pt;
    process_query(tscan_customer_packet, pt);

    
    dp->query_state_destroy(qs);
};



void tpcc_read_tbl_DISTRICT  (Db*) { TRACE( TRACE_ALWAYS, "Not Implemented yet!\n"); }


void tpcc_read_tbl_HISTORY   (Db*) { TRACE( TRACE_ALWAYS, "Not Implemented yet!\n"); }

void tpcc_read_tbl_ITEM      (Db*) { TRACE( TRACE_ALWAYS, "Not Implemented yet!\n"); } 


void tpcc_read_tbl_NEW_ORDER (Db*) { TRACE( TRACE_ALWAYS, "Not Implemented yet!\n"); }

void tpcc_read_tbl_ORDER     (Db*) { TRACE( TRACE_ALWAYS, "Not Implemented yet!\n"); }

void tpcc_read_tbl_ORDERLINE (Db*) { TRACE( TRACE_ALWAYS, "Not Implemented yet!\n"); }

void tpcc_read_tbl_STOCK     (Db*) { TRACE( TRACE_ALWAYS, "Not Implemented yet!\n"); }

void tpcc_read_tbl_WAREHOUSE (Db*) { TRACE( TRACE_ALWAYS, "Not Implemented yet!\n"); }





EXIT_NAMESPACE(tpcc);

