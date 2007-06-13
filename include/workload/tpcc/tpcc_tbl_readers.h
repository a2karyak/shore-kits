/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file tpcc_tbl_readers.h
 *
 *  @brief Interface for the TPC-C table reading functions, used only
 *  for debugging purposes
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __TPCC_TBL_READERS_H
#define __TPCC_TBL_READERS_H

#include <db_cxx.h>
#include <cstdio>

#include "util/namespace.h"

ENTER_NAMESPACE(tpcc);

/* exported functions */

void tpcc_read_tbl_CUSTOMER  (Db* db);
void tpcc_read_tbl_DISTRICT  (Db* db);
void tpcc_read_tbl_HISTORY   (Db* db);
void tpcc_read_tbl_ITEM      (Db* db);
void tpcc_read_tbl_NEW_ORDER (Db* db);
void tpcc_read_tbl_ORDER     (Db* db);
void tpcc_read_tbl_ORDERLINE (Db* db);
void tpcc_read_tbl_STOCK     (Db* db);
void tpcc_read_tbl_WAREHOUSE (Db* db);

EXIT_NAMESPACE(tpcc);

#endif
