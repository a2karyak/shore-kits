/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file tpcc_env.h
 *
 *  @brief Definitions useful for the TPC-C database
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __TPCC_ENV_H
#define __TPCC_ENV_H

#include <cstdio>

#include "util/namespace.h"
#include "workload/common/bdb_env.h"

using namespace workload;

ENTER_NAMESPACE(tpcc);

/* exported constants */

enum tpcc_table_list_t {
    TPCC_TABLE_CUSTOMER   = 0,
    TPCC_TABLE_DISTRICT   = 1,
    TPCC_TABLE_HISTORY    = 2,
    TPCC_TABLE_ITEM       = 3,
    TPCC_TABLE_NEW_ORDER  = 4,
    TPCC_TABLE_ORDER      = 5,
    TPCC_TABLE_ORDERLINE  = 6,
    TPCC_TABLE_STOCK      = 7,
    TPCC_TABLE_WAREHOUSE  = 8,
    _TPCC_TABLE_COUNT_    = 9
};

/* pointers to tpcc_tables */
extern bdb_table_s tpcc_tables[_TPCC_TABLE_COUNT_];

/*
// BDB indexes
extern Db* tpch_lineitem_shipdate;
extern Db* tpch_lineitem_shipdate_idx;
*/

EXIT_NAMESPACE(tpcc);


#endif
