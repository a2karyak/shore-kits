/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file tpch_tbl_readers.cpp
 *
 *  @brief Implementation of the TPC-H table reading functions, used only
 *  for debugging purposes
 *
 *  @author Ippokratis Pandis (ipandis)
 */

/*
#include "workload/common/bdb_env.h"
//#include "workload/tpcc/tpcc_type_convert.h"
*/

#include "util/trace.h"

#include "workload/tpch/tpch_tbl_readers.h"
#include "workload/tpch/tpch_struct.h"


ENTER_NAMESPACE(tpch);


/* definitions of exported functions */


void tpch_read_tbl_CUSTOMER (Db* db) { TRACE( TRACE_ALWAYS, "Not Implemented yet!\n"); }
void tpch_read_tbl_LINEITEM (Db* db) { TRACE( TRACE_ALWAYS, "Not Implemented yet!\n"); }
void tpch_read_tbl_NATION   (Db* db) { TRACE( TRACE_ALWAYS, "Not Implemented yet!\n"); }
void tpch_read_tbl_ORDERS   (Db* db) { TRACE( TRACE_ALWAYS, "Not Implemented yet!\n"); }
void tpch_read_tbl_PART     (Db* db) { TRACE( TRACE_ALWAYS, "Not Implemented yet!\n"); }
void tpch_read_tbl_PARTSUPP (Db* db) { TRACE( TRACE_ALWAYS, "Not Implemented yet!\n"); }
void tpch_read_tbl_REGION   (Db* db) { TRACE( TRACE_ALWAYS, "Not Implemented yet!\n"); }
void tpch_read_tbl_SUPPLIER (Db* db) { TRACE( TRACE_ALWAYS, "Not Implemented yet!\n"); }



EXIT_NAMESPACE(tpch);

