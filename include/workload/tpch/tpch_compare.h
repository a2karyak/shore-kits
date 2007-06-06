/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef __TPCH_COMPARE_H
#define __TPCH_COMPARE_H

#include <db_cxx.h>
#include <cstdio>

#include "util/namespace.h"

ENTER_NAMESPACE(tpch);

// BerkeleyDB comparators for B-tree table organization

int tpch_bt_compare_fn_CUSTOMER(Db* idx, 
                                const Dbt* k1, const Dbt* k2);

int tpch_bt_compare_fn_LINEITEM(Db* idx, 
                                const Dbt* k1, const Dbt* k2);

int tpch_bt_compare_fn_NATION  (Db* idx, 
                                const Dbt* k1, const Dbt* k2);

int tpch_bt_compare_fn_ORDERS  (Db* idx, 
                                const Dbt* k1, const Dbt* k2);

int tpch_bt_compare_fn_PART    (Db* idx, 
                                const Dbt* k1, const Dbt* k2);

int tpch_bt_compare_fn_PARTSUPP(Db* idx, 
                                const Dbt* k1, const Dbt* k2);

int tpch_bt_compare_fn_REGION  (Db* idx, 
                                const Dbt* k1, const Dbt* k2);

int tpch_bt_compare_fn_SUPPLIER(Db* idx, 
                                const Dbt* k1, const Dbt* k2);

// indexes
int tpch_lineitem_shipdate_compare_fcn(Db* idx, 
                                       const Dbt* k1, const Dbt* k2);
int tpch_lineitem_shipdate_key_fcn(Db*idx, 
                                   const Dbt* pk, const Dbt* pd, Dbt* sk);

EXIT_NAMESPACE(tpch);


#endif
