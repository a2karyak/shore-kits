/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _TPCH_ENV_H
#define _TPCH_ENV_H

#include <db_cxx.h>

typedef int (*bt_compare_func_t)(Db*, const Dbt*, const Dbt*);
typedef int (*idx_key_create_func_t)(Db*, const Dbt*, const Dbt*, Dbt*);

// environment
extern DbEnv* dbenv;


// tables
extern Db* tpch_customer;
extern Db* tpch_lineitem;
extern Db* tpch_nation;
extern Db* tpch_orders;
extern Db* tpch_part;
extern Db* tpch_partsupp;
extern Db* tpch_region;
extern Db* tpch_supplier;


// indexes
extern Db* tpch_lineitem_shipdate;
extern Db* tpch_lineitem_shipdate_idx;
#endif
