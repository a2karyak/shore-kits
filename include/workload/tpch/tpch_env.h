/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef __TPCH_ENV_H
#define __TPCH_ENV_H

#include <cstdio>

#include "util/namespace.h"
#include "workload/common/bdb_env.h"

using namespace workload;

ENTER_NAMESPACE(tpch);

/* exported constants */

enum tpch_table_list_t {
    TPCH_TABLE_CUSTOMER = 0,
    TPCH_TABLE_LINEITEM = 1,
    TPCH_TABLE_NATION   = 2,
    TPCH_TABLE_ORDERS   = 3,
    TPCH_TABLE_PART     = 4,
    TPCH_TABLE_PARTSUPP = 5,
    TPCH_TABLE_REGION   = 6,
    TPCH_TABLE_SUPPLIER = 7,
    _TPCH_TABLE_COUNT_  = 8
};

/* pointers to tpch_tables */
extern bdb_table_s tpch_tables[_TPCH_TABLE_COUNT_];

/* BerkeleyDB indexes */
extern Db* tpch_lineitem_shipdate;
extern Db* tpch_lineitem_shipdate_idx;

EXIT_NAMESPACE(tpch);


#endif
