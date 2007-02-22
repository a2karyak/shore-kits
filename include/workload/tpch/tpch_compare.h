/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _TPCH_COMPARE_H
#define _TPCH_COMPARE_H

#include <db_cxx.h>


/* BerkelyDB comparators for B-tree table organization */
int tpch_bt_compare_fn_CUSTOMER(Db*, const Dbt* k1, const Dbt* k2);
int tpch_bt_compare_fn_LINEITEM(Db*, const Dbt* k1, const Dbt* k2);
int tpch_bt_compare_fn_NATION  (Db*, const Dbt* k1, const Dbt* k2);
int tpch_bt_compare_fn_ORDERS  (Db*, const Dbt* k1, const Dbt* k2);
int tpch_bt_compare_fn_PART    (Db*, const Dbt* k1, const Dbt* k2);
int tpch_bt_compare_fn_PARTSUPP(Db*, const Dbt* k1, const Dbt* k2);
int tpch_bt_compare_fn_REGION  (Db*, const Dbt* k1, const Dbt* k2);
int tpch_bt_compare_fn_SUPPLIER(Db*, const Dbt* k1, const Dbt* k2);

// indexes
int tpch_lineitem_shipdate_compare_fcn(Db*, const Dbt* k1, const Dbt* k2);
int tpch_lineitem_shipdate_key_fcn(Db*, const Dbt* pk, const Dbt* pd, Dbt* sk);



#endif
