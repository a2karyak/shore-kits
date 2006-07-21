/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _TPCH_COMPARE_H
#define _TPCH_COMPARE_H

#include <db_cxx.h>

// tables
int tpch_lineitem_bt_compare_fcn(Db*, const Dbt* k1, const Dbt* k2);
int tpch_orders_bt_compare_fcn(Db*, const Dbt* k1, const Dbt* k2);
int tpch_part_bt_compare_fcn(Db*, const Dbt* k1, const Dbt* k2);
int tpch_partsupp_bt_compare_fcn(Db*, const Dbt* k1, const Dbt* k2);
int tpch_region_bt_compare_fcn(Db*, const Dbt* k1, const Dbt* k2);
int tpch_supplier_bt_compare_fcn(Db*, const Dbt* k1, const Dbt* k2);
int tpch_customer_bt_compare_fcn(Db*, const Dbt* k1, const Dbt* k2);
int tpch_nation_bt_compare_fcn(Db*, const Dbt* k1, const Dbt* k2);

// indexes
int tpch_lineitem_shipdate_compare_fcn(Db*, const Dbt* k1, const Dbt* k2);
int tpch_lineitem_shipdate_key_fcn(Db*, const Dbt* pk, const Dbt* pd, Dbt* sk);



#endif
