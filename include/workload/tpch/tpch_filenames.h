/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file tpch_filenames.h
 *
 *  @brief Definition of TPC-H related filenames
 */

#ifndef _TPCH_FILENAMES_H
#define _TPCH_FILENAMES_H

#include "util/namespace.h"

ENTER_NAMESPACE(tpch);

#define TABLE_ID_CUSTOMER           "TBL_CUST"
#define TBL_FILENAME_CUSTOMER       "customer.tbl"
#define BDB_FILENAME_CUSTOMER       "CUSTOMER.bdb"

#define TABLE_ID_LINEITEM           "TBL_LITEM"
#define TBL_FILENAME_LINEITEM       "lineitem.tbl"
#define BDB_FILENAME_LINEITEM       "LINEITEM.bdb"

#define TABLE_ID_NATION             "TBL_NAT"
#define TBL_FILENAME_NATION         "nation.tbl"
#define BDB_FILENAME_NATION         "NATION.bdb"

#define TABLE_ID_ORDERS             "TBL_ORD"
#define TBL_FILENAME_ORDERS         "orders.tbl"
#define BDB_FILENAME_ORDERS         "ORDERS.bdb"

#define TABLE_ID_PART               "TBL_PRT"
#define TBL_FILENAME_PART           "part.tbl"
#define BDB_FILENAME_PART           "PART.bdb"

#define TABLE_ID_PARTSUPP           "TBL_PRTSUP"
#define TBL_FILENAME_PARTSUPP       "partsupp.tbl"
#define BDB_FILENAME_PARTSUPP       "PARTSUPP.bdb"

#define TABLE_ID_REGION             "TBL_REG"
#define TBL_FILENAME_REGION         "region.tbl"
#define BDB_FILENAME_REGION         "REGION.bdb"

#define TABLE_ID_SUPPLIER           "TBL_SUP"
#define TBL_FILENAME_SUPPLIER       "supplier.tbl"
#define BDB_FILENAME_SUPPLIER       "SUPPLIER.bdb"

#define INDEX_LINEITEM_SHIPDATE_NAME "LINEITEM.SHIPDATE"
#define INDEX_LINEITEM_SHIPDATE_ID   "IDX_LI_SD"
#define INDEX_LINEITEM_SHIPDATE_FILENAME "lineitem-shipdate.tbl"

EXIT_NAMESPACE(tpch);

#endif
