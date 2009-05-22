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

/** @file tpcc_env.cpp
 *
 *  @brief Declaration of the TPC-C database tables and indexes
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#include "workload/tpcc/tpcc_env.h"
#include "workload/tpcc/tpcc_filenames.h"
#include "workload/tpcc/tpcc_compare.h"
#include "workload/tpcc/bdb_tbl_parsers.h"
#include "workload/tpcc/tpcc_tbl_readers.h"


using namespace tpcc;


/* exported data structures */

#define TABLE(name) { TBL_FILENAME_##name, \
                      TABLE_ID_##name,     \
                      NULL, \
                      tpcc_bt_compare_fn_##name, \
                      tpcc_parse_tbl_##name, \
                      tpcc_read_tbl_##name }

bdb_table_s tpcc::tpcc_tables[_TPCC_TABLE_COUNT_] = {
    TABLE(CUSTOMER),
    TABLE(DISTRICT),
    TABLE(HISTORY),
    TABLE(ITEM),
    TABLE(NEW_ORDER),
    TABLE(ORDER),
    TABLE(ORDERLINE),
    TABLE(STOCK),
    TABLE(WAREHOUSE)
};
