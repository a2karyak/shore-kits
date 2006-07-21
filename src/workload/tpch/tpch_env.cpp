/* -*- mode:C++; c-basic-offset:4 -*- */

#include "workload/tpch/tpch_env.h"


// environment
DbEnv* dbenv = NULL;


// tables
Db* tpch_customer = NULL;
Db* tpch_lineitem = NULL;
Db* tpch_nation = NULL;
Db* tpch_orders = NULL;
Db* tpch_part = NULL;
Db* tpch_partsupp = NULL;
Db* tpch_region = NULL;
Db* tpch_supplier = NULL;

// indexes
Db* tpch_lineitem_shipdate = NULL;
