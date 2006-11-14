/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _TPCH_ENV_H
#define _TPCH_ENV_H

#include <db_cxx.h>
#include <vector>
#include "core.h"

typedef int (*bt_compare_func_t)(Db*, const Dbt*, const Dbt*);
typedef int (*idx_key_create_func_t)(Db*, const Dbt*, const Dbt*, Dbt*);

// environment
extern DbEnv* dbenv;

typedef std::vector<qpipe::page*> page_list;

// tables
extern page_list* tpch_customer;
extern page_list* tpch_lineitem;
extern page_list* tpch_nation;
extern page_list* tpch_orders;
extern page_list* tpch_part;
extern page_list* tpch_partsupp;
extern page_list* tpch_region;
extern page_list* tpch_supplier;

// indexes (*_idx are unassociated and only return primary keys)
extern Db* tpch_lineitem_shipdate;
extern Db* tpch_lineitem_shipdate_idx;

#endif
