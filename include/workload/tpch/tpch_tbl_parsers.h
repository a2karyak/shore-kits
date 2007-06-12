/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file tpch_tbl_parsers.h
 *
 *  @brief Declaration of the TPC-H table parsing functions
 */

#ifndef __TPCH_TBL_PARSERS_H
#define __TPCH_TBL_PARSERS_H

#include <db_cxx.h>
#include <cstdio>

#include "util/namespace.h"

ENTER_NAMESPACE(tpch);

/* exported functions */

void tpch_parse_tbl_CUSTOMER(Db* db, FILE* fd);
void tpch_parse_tbl_LINEITEM(Db* db, FILE* fd);
void tpch_parse_tbl_NATION  (Db* db, FILE* fd);
void tpch_parse_tbl_ORDERS  (Db* db, FILE* fd);
void tpch_parse_tbl_PART    (Db* db, FILE* fd);
void tpch_parse_tbl_PARTSUPP(Db* db, FILE* fd);
void tpch_parse_tbl_REGION  (Db* db, FILE* fd);
void tpch_parse_tbl_SUPPLIER(Db* db, FILE* fd);

EXIT_NAMESPACE(tpch);

#endif
