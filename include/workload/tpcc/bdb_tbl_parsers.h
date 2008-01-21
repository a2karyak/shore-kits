/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file bdb_tbl_parsers.h
 *
 *  @brief Interface for the TPC-C table parsing functions for BDB
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __BDB_TBL_PARSERS_H
#define __BDB_TBL_PARSERS_H

#include <db_cxx.h>
#include <cstdio>

#include "util/namespace.h"
#include "stages/tpcc/common/tpcc_struct.h"


ENTER_NAMESPACE(tpcc);


//////////////////////////////////////////////////////
// BDB TABLE LOADING
//
// (ip) To be replaced with the per-table row parsers

void tpcc_parse_tbl_CUSTOMER  (Db* db, FILE* fd);
void tpcc_parse_tbl_DISTRICT  (Db* db, FILE* fd);
void tpcc_parse_tbl_HISTORY   (Db* db, FILE* fd);
void tpcc_parse_tbl_ITEM      (Db* db, FILE* fd);
void tpcc_parse_tbl_NEW_ORDER (Db* db, FILE* fd);
void tpcc_parse_tbl_ORDER     (Db* db, FILE* fd);
void tpcc_parse_tbl_ORDERLINE (Db* db, FILE* fd);
void tpcc_parse_tbl_STOCK     (Db* db, FILE* fd);
void tpcc_parse_tbl_WAREHOUSE (Db* db, FILE* fd);


// EOF BDB TABLE LOADING
//////////////////////////////////////////////////////


EXIT_NAMESPACE(tpcc);

#endif
