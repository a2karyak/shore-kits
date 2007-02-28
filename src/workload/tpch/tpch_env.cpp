/* -*- mode:C++; c-basic-offset:4 -*- */

#include "workload/tpch/tpch_env.h"
#include "workload/tpch/tpch_filenames.h"
#include "workload/tpch/tpch_tuple_parsers.h"
#include "workload/tpch/tpch_struct.h"


/* internal macros */

#define TABLE(name, lname) { TBL_FILENAME_##name, \
                             CDB_FILENAME_##name, \
                             TABLE_ID_##name,     \
                             new page_list(), \
                             tpch_tuple_parser_##name, \
                             sizeof(tpch_##lname##_tuple) }


/* exported data structures */

cdb_table_s tpch_tables[_TPCH_TABLE_COUNT_] = {
    TABLE(CUSTOMER, customer),
    TABLE(LINEITEM, lineitem),
    TABLE(NATION,   nation),
    TABLE(ORDERS,   orders),
    TABLE(PART,     part),
    TABLE(PARTSUPP, partsupp),
    TABLE(REGION,   region),
    TABLE(SUPPLIER, supplier)
};
