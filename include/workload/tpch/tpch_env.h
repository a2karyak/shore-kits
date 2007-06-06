/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _TPCH_ENV_H
#define _TPCH_ENV_H

#include <cstdio>

#include "workload/common/bdb_env.h"

// bdb_table_s: Class the represents a table/file
class bdb_table_s {
public:
    const char* tbl_filename;
    const char* bdb_filename;
    const char* table_id;

    Db* db;

    bt_compare_fn_t bt_compare_fn;
    parse_tbl_t     parse_tbl;
};


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

extern bdb_table_s tpch_tables[_TPCH_TABLE_COUNT_];

/* BerkeleyDB indexes */
extern Db* tpch_lineitem_shipdate;
extern Db* tpch_lineitem_shipdate_idx;



#endif
