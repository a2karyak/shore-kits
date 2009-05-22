/* -*- mode:C++; c-basic-offset:4 -*-
     Shore-kits -- Benchmark implementations for Shore-MT
   
                       Copyright (c) 2007-2009
      Data Intensive Applications and Systems Labaratory (DIAS)
               Ecole Polytechnique Federale de Lausanne
   
                         All Rights Reserved.
   
   Permission to use, copy, modify and distribute this software and
   its documentation is hereby granted, provided that both the
   copyright notice and this permission notice appear in all copies of
   the software, derivative works or modified versions, and any
   portions thereof, and that both notices appear in supporting
   documentation.
   
   This code is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. THE AUTHORS
   DISCLAIM ANY LIABILITY OF ANY KIND FOR ANY DAMAGES WHATSOEVER
   RESULTING FROM THE USE OF THIS SOFTWARE.
*/

/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file tpcc_tbl_parsers.h
 *
 *  @brief Interface for the TPC-C table parsing functions
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __TPCC_TBL_PARSERS_H
#define __TPCC_TBL_PARSERS_H

#include <cstdio>

#include "util/namespace.h"
#include <utility>
#include "workload/tpcc/common/tpcc_struct.h"


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

/** NOTE: For the tables that the key is large (>3) we are using only the 
 *        remaining part of the tuple as the body. Otherwise, we are using
 *         the whole tuple.
 */

// The whole tuple as body
DECLARE_TPCC_PARSER(ITEM, tpcc_item_tuple_key, tpcc_item_tuple, "i4", true);
DECLARE_TPCC_PARSER(NEW_ORDER, tpcc_new_order_tuple, int, "i4i4i4", false);
DECLARE_TPCC_PARSER(DISTRICT, tpcc_district_tuple_key, tpcc_district_tuple, "i4i4", true);
DECLARE_TPCC_PARSER(WAREHOUSE, tpcc_warehouse_tuple_key, tpcc_warehouse_tuple, "i4", true);
DECLARE_TPCC_PARSER(STOCK, tpcc_stock_tuple_key, tpcc_stock_tuple, "i4i4", true);


// Only the remaining (non-key) as body
DECLARE_TPCC_PARSER(HISTORY, tpcc_history_tuple_key, tpcc_history_tuple_body, "i4i4i4i4i4i4", true);
DECLARE_TPCC_PARSER(ORDERLINE, tpcc_orderline_tuple_key, tpcc_orderline_tuple_body, "i4i4i4i4", true);
DECLARE_TPCC_PARSER(ORDER, tpcc_order_tuple_key, tpcc_order_tuple_body, "i4i4i4i4", true);
DECLARE_TPCC_PARSER(CUSTOMER, tpcc_customer_tuple_key, tpcc_customer_tuple_body, "i4i4i4", true);

#undef DECLARE_TPCC_PARSER


EXIT_NAMESPACE(tpcc);

#endif
