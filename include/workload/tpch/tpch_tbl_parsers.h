/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _TPCH_TBL_PARSERS_H
#define _TPCH_TBL_PARSERS_H

#include <db_cxx.h>
#include <cstdio>


/* exported functions */

void tpch_parse_tbl_CUSTOMER(Db* db, FILE* fd);
void tpch_parse_tbl_LINEITEM(Db* db, FILE* fd);
void tpch_parse_tbl_NATION  (Db* db, FILE* fd);
void tpch_parse_tbl_ORDERS  (Db* db, FILE* fd);
void tpch_parse_tbl_PART    (Db* db, FILE* fd);
void tpch_parse_tbl_PARTSUPP(Db* db, FILE* fd);
void tpch_parse_tbl_REGION  (Db* db, FILE* fd);
void tpch_parse_tbl_SUPPLIER(Db* db, FILE* fd);


#endif
