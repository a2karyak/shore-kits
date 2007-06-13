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

#include "util/trace.h"

#include "workload/tpcc/tpcc_tbl_readers.h"
#include "workload/tpcc/tpcc_struct.h"


ENTER_NAMESPACE(tpcc);


/* definitions of exported functions */


void tpcc_read_tbl_CUSTOMER  (Db* db) {

    TRACE( TRACE_ALWAYS, "Should be multi-threaded!"); 

    TRACE(TRACE_DEBUG, "Reading TPC-C CUSTOMER...\n");

    tpcc_customer_tuple tup;


}


void tpcc_read_tbl_DISTRICT  (Db* db) { TRACE( TRACE_ALWAYS, "Not Implemented yet!\n"); }


void tpcc_read_tbl_HISTORY   (Db* db) { TRACE( TRACE_ALWAYS, "Not Implemented yet!\n"); }

void tpcc_read_tbl_ITEM      (Db* db) { TRACE( TRACE_ALWAYS, "Not Implemented yet!\n"); } 


void tpcc_read_tbl_NEW_ORDER (Db* db) { TRACE( TRACE_ALWAYS, "Not Implemented yet!\n"); }

void tpcc_read_tbl_ORDER     (Db* db) { TRACE( TRACE_ALWAYS, "Not Implemented yet!\n"); }

void tpcc_read_tbl_ORDERLINE (Db* db) { TRACE( TRACE_ALWAYS, "Not Implemented yet!\n"); }

void tpcc_read_tbl_STOCK     (Db* db) { TRACE( TRACE_ALWAYS, "Not Implemented yet!\n"); }

void tpcc_read_tbl_WAREHOUSE (Db* db) { TRACE( TRACE_ALWAYS, "Not Implemented yet!\n"); }







EXIT_NAMESPACE(tpcc);

