/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file tpcc_tbl_parsers.cpp
 *
 *  @brief Implementation of the TPC-C table parsing functions
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#include "util/trace.h"
#include "util/time_util.h"
#include "util/progress.h"

#include "workload/common/bdb_env.h"
#include "workload/tpcc/tpcc_tbl_parsers.h"
#include "workload/tpcc/tpcc_struct.h"
//#include "workload/tpcc/tpcc_type_convert.h"

using namespace workload;

ENTER_NAMESPACE(tpcc);


/* definitions of exported functions */


void tpcc_parse_tbl_CUSTOMER  (Db* db, FILE* fd) { TRACE( TRACE_ALWAYS, "Doing Nothing!"); }

void tpcc_parse_tbl_DISTRICT  (Db* db, FILE* fd) { TRACE( TRACE_ALWAYS, "Doing Nothing!"); }

void tpcc_parse_tbl_HISTORY   (Db* db, FILE* fd) { TRACE( TRACE_ALWAYS, "Doing Nothing!"); }

void tpcc_parse_tbl_ITEM      (Db* db, FILE* fd) { TRACE( TRACE_ALWAYS, "Doing Nothing!"); }

void tpcc_parse_tbl_NEW_ORDER (Db* db, FILE* fd) { TRACE( TRACE_ALWAYS, "Doing Nothing!"); }

void tpcc_parse_tbl_ORDER     (Db* db, FILE* fd) { TRACE( TRACE_ALWAYS, "Doing Nothing!"); }

void tpcc_parse_tbl_ORDERLINE (Db* db, FILE* fd) { TRACE( TRACE_ALWAYS, "Doing Nothing!"); }

void tpcc_parse_tbl_STOCK     (Db* db, FILE* fd) { TRACE( TRACE_ALWAYS, "Doing Nothing!"); }

void tpcc_parse_tbl_WAREHOUSE (Db* db, FILE* fd) { TRACE( TRACE_ALWAYS, "Doing Nothing!"); }



EXIT_NAMESPACE(tpcc);
