/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file tpcc_tbl_parsers.h
 *
 *  @brief Interface for the TPC-C table parsing functions
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __TPCC_TBL_PARSERS_H
#define __TPCC_TBL_PARSERS_H

#include <db_cxx.h>
#include <cstdio>

#include "util/namespace.h"

ENTER_NAMESPACE(tpcc);

/* exported functions */

void tpcc_parse_tbl_CUSTOMER  (Db* db, FILE* fd);
void tpcc_parse_tbl_DISTRICT  (Db* db, FILE* fd);
void tpcc_parse_tbl_HISTORY   (Db* db, FILE* fd);
void tpcc_parse_tbl_ITEM      (Db* db, FILE* fd);
void tpcc_parse_tbl_NEW_ORDER (Db* db, FILE* fd);
void tpcc_parse_tbl_ORDER     (Db* db, FILE* fd);
void tpcc_parse_tbl_ORDERLINE (Db* db, FILE* fd);
void tpcc_parse_tbl_STOCK     (Db* db, FILE* fd);
void tpcc_parse_tbl_WAREHOUSE (Db* db, FILE* fd);

EXIT_NAMESPACE(tpcc);

#endif
