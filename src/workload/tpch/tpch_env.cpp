/* -*- mode:C++; c-basic-offset:4 -*- */

#include "workload/tpch/tpch_env.h"
#include "workload/tpch/tpch_filenames.h"
#include "workload/tpch/tpch_compare.h"
#include "workload/tpch/tpch_tbl_parsers.h"


/* exported data structures */

/* BerkeleyDB environment */
DbEnv* dbenv = NULL;

/* BerkeleyDB indexes */
Db* tpch_lineitem_shipdate = NULL;
Db* tpch_lineitem_shipdate_idx = NULL;


#define TABLE(name) { TBL_FILENAME_##name, \
                      BDB_FILENAME_##name, \
                      TABLE_ID_##name,     \
                      NULL, \
                      tpch_bt_compare_fn_##name, \
                      tpch_parse_tbl_##name }

bdb_table_s tpch_tables[_TPCH_TABLE_COUNT_] = {
    TABLE(CUSTOMER),
    TABLE(LINEITEM),
    TABLE(NATION),
    TABLE(ORDERS),
    TABLE(PART),
    TABLE(PARTSUPP),
    TABLE(REGION),
    TABLE(SUPPLIER)
};
