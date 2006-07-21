/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _TPCH_TABLES_H
#define _TPCH_TABLES_H


// BerekeleyDB table names

#define TABLE_CUSTOMER_NAME         "CUSTOMER"
#define TABLE_CUSTOMER_ID           "TBL_CUST"
#define TABLE_CUSTOMER_TBL_FILENAME "customer.tbl"

#define TABLE_LINEITEM_NAME         "LINEITEM"
#define TABLE_LINEITEM_ID           "TBL_LITEM"
#define TABLE_LINEITEM_TBL_FILENAME "lineitem.tbl"

#define TABLE_NATION_NAME           "NATION"
#define TABLE_NATION_ID             "TBL_NAT"
#define TABLE_NATION_TBL_FILENAME   "nation.tbl"

#define TABLE_ORDERS_NAME           "ORDERS"
#define TABLE_ORDERS_ID             "TBL_ORD"
#define TABLE_ORDERS_TBL_FILENAME   "orders.tbl"

#define TABLE_PART_NAME             "PART"
#define TABLE_PART_ID               "TBL_PRT"
#define TABLE_PART_TBL_FILENAME     "part.tbl"

#define TABLE_PARTSUPP_NAME         "PARTSUPP"
#define TABLE_PARTSUPP_ID           "TBL_PRTSUP"
#define TABLE_PARTSUPP_TBL_FILENAME "partsupp.tbl"

#define TABLE_REGION_NAME           "REGION"
#define TABLE_REGION_ID             "TBL_REG"
#define TABLE_REGION_TBL_FILENAME   "region.tbl"

#define TABLE_SUPPLIER_NAME         "SUPPLIER"
#define TABLE_SUPPLIER_ID           "TBL_SUP"
#define TABLE_SUPPLIER_TBL_FILENAME "supplier.tbl"

#define INDEX_LINEITEM_SHIPDATE_NAME "LINEITEM.SHIPDATE"
#define INDEX_LINEITEM_SHIPDATE_ID   "IDX_LI_SD"
#define INDEX_LINEITEM_SHIPDATE_FILENAME "lineitem-shipdate.tbl"

#endif
