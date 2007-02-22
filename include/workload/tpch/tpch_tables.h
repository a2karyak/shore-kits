/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _TPCH_TABLES_H
#define _TPCH_TABLES_H


// BerekeleyDB table names

#define TABLE_CUSTOMER_ID           "TBL_CUST"
#define TBL_FILENAME_CUSTOMER       "customer.tbl"
#define BDB_FILENAME_CUSTOMER       "CUSTOMER.bdb"

#define TABLE_LINEITEM_ID           "TBL_LITEM"
#define TBL_FILENAME_LINEITEM       "lineitem.tbl"
#define BDB_FILENAME_LINEITEM       "LINEITEM.bdb"

#define TABLE_NATION_ID             "TBL_NAT"
#define TBL_FILENAME_NATION         "nation.tbl"
#define BDB_FILENAME_NATION         "NATION.bdb"

#define TABLE_ORDERS_ID             "TBL_ORD"
#define TBL_FILENAME_ORDERS         "orders.tbl"
#define BDB_FILENAME_ORDERS         "ORDERS.bdb"

#define TABLE_PART_ID               "TBL_PRT"
#define TBL_FILENAME_PART           "part.tbl"
#define BDB_FILENAME_PART           "PART.bdb"

#define TABLE_PARTSUPP_ID           "TBL_PRTSUP"
#define TBL_FILENAME_PARTSUPP       "partsupp.tbl"
#define BDB_FILENAME_PARTSUPP       "PARTSUPP.bdb"

#define TABLE_REGION_ID             "TBL_REG"
#define TBL_FILENAME_REGION         "region.tbl"
#define BDB_FILENAME_REGION         "REGION.bdb"

#define TABLE_SUPPLIER_ID           "TBL_SUP"
#define TBL_FILENAME_SUPPLIER       "supplier.tbl"
#define BDB_FILENAME_SUPPLIER       "SUPPLIER.bdb"

#define INDEX_LINEITEM_SHIPDATE_NAME "LINEITEM.SHIPDATE"
#define INDEX_LINEITEM_SHIPDATE_ID   "IDX_LI_SD"
#define INDEX_LINEITEM_SHIPDATE_FILENAME "lineitem-shipdate.tbl"

#endif
