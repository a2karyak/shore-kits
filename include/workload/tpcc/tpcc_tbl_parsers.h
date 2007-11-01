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
#include <utility>
#include "stages/tpcc/common/tpcc_struct.h"


ENTER_NAMESPACE(tpcc);

/* exported functions */
#define DECLARE_TPCC_PARSER(tname, key, body, key_desc, body_flag) \
    struct parse_tpcc_##tname { \
	    typedef std::pair<key, body> record_t;	\
	    record_t parse_row(char* linebuffer) const; \
	    char const* describe_key() const { return key_desc; } \
	    char const* table_name() const { return #tname; } \
	    bool has_body() const { return body_flag; } \
    }

#define DEFINE_TPCC_PARSER(tname) \
    parse_tpcc_##tname::record_t parse_tpcc_##tname::parse_row(char* linebuffer) const

DECLARE_TPCC_PARSER(ITEM, tpcc_item_tuple_key, tpcc_item_tuple, "i4", true);
DECLARE_TPCC_PARSER(NEW_ORDER, tpcc_new_order_tuple, int, "i4i4i4", false);
DECLARE_TPCC_PARSER(ORDER, tpcc_order_tuple_key, tpcc_order_tuple, "i4i4i4i4", true);
DECLARE_TPCC_PARSER(DISTRICT, tpcc_district_tuple_key, tpcc_district_tuple, "i4i4", true);
DECLARE_TPCC_PARSER(WAREHOUSE, tpcc_warehouse_tuple_key, tpcc_warehouse_tuple, "i4", true);

//DECLARE_TPCC_PARSER(HISTORY, tpcc_history_tuple_key, tpcc_history_tuple, "i4i4i4i4i4i4", true);
DECLARE_TPCC_PARSER(HISTORY, tpcc_history_tuple_key, tpcc_history_tuple_body, "i4i4i4i4i4i4", true);

DECLARE_TPCC_PARSER(CUSTOMER, tpcc_customer_tuple_key, tpcc_customer_tuple_body, "i4i4i4", true);

#undef DECLARE_TPCC_PARSER

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
