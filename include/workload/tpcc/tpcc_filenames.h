/* -*- mode:C++; c-basic-offset:4 -*-
     Shore-MT -- Multi-threaded port of the SHORE storage manager
   
                       Copyright (c) 2007-2009
      Data Intensive Applications and Systems Labaratory (DIAS)
               Ecole Polytechnique Federale de Lausanne
   
                         All Rights Reserved.
   
   Permission to use, copy, modify and distribute this software and
   its documentation is hereby granted, provided that both the
   copyright notice and this permission notice appear in all copies of
   the software, derivative works or modified versions, and any
   portions thereof, and that both notices appear in supporting
   documentation.
   
   This code is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. THE AUTHORS
   DISCLAIM ANY LIABILITY OF ANY KIND FOR ANY DAMAGES WHATSOEVER
   RESULTING FROM THE USE OF THIS SOFTWARE.
*/

/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file tpcc_filenames.h
 *
 *  @brief Definition of TPC-C related filenames
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef _TPCC_FILENAMES_H
#define _TPCC_FILENAMES_H

#include "util/namespace.h"

ENTER_NAMESPACE(tpcc);

#define TABLE_ID_CUSTOMER            "TPCC_TBL_CUSTOMER"
#define TBL_FILENAME_CUSTOMER        "CUSTOMER.dat"
#define BDB_FILENAME_CUSTOMER        "TPCC_CUSTOMER.bdb"


#define TABLE_ID_DISTRICT            "TPCC_TBL_DISTRICT"
#define TBL_FILENAME_DISTRICT        "DISTRICT.dat"
#define BDB_FILENAME_DISTRICT        "TPCC_DISTRICT.bdb"


#define TABLE_ID_HISTORY             "TPCC_TBL_HISTORY"
#define TBL_FILENAME_HISTORY         "HISTORY.dat"
#define BDB_FILENAME_HISTORY         "TPCC_HISTORY.bdb"


#define TABLE_ID_ITEM                "TPCC_TBL_ITEM"
#define TBL_FILENAME_ITEM            "ITEM.dat"
#define BDB_FILENAME_ITEM            "TPCC_ITEM.bdb"


#define TABLE_ID_NEW_ORDER           "TPCC_TBL_NEW_ORDER"
#define TBL_FILENAME_NEW_ORDER       "NEW_ORDER.dat"
#define BDB_FILENAME_NEW_ORDER       "TPCC_NEW_ORDER.bdb"


#define TABLE_ID_ORDER               "TPCC_TBL_ORDER"
#define TBL_FILENAME_ORDER           "ORDER.dat"
#define BDB_FILENAME_ORDER           "TPCC_ORDER.bdb"


#define TABLE_ID_ORDERLINE           "TPCC_TBL_ORDERLINE"
#define TBL_FILENAME_ORDERLINE       "ORDERLINE.dat"
#define BDB_FILENAME_ORDERLINE       "TPCC_ORDERLINE.bdb"


#define TABLE_ID_STOCK               "TPCC_TBL_STOCK"
#define TBL_FILENAME_STOCK           "STOCK.dat"
#define BDB_FILENAME_STOCK           "TPCC_STOCK.bdb"


#define TABLE_ID_WAREHOUSE           "TPCC_TBL_WAREHOUSE"
#define TBL_FILENAME_WAREHOUSE       "WAREHOUSE.dat"
#define BDB_FILENAME_WAREHOUSE       "TPCC_WAREHOUSE.bdb"


/*
#define INDEX_LINEITEM_SHIPDATE_NAME "LINEITEM.SHIPDATE"
#define INDEX_LINEITEM_SHIPDATE_ID   "IDX_LI_SD"
#define INDEX_LINEITEM_SHIPDATE_FILENAME "lineitem-shipdate.tbl"
*/

EXIT_NAMESPACE(tpcc);

#endif
