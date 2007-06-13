/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file tpch_tbl_readers.h
 *
 *  @brief Interface for the TPC-C table reading functions, used only
 *  for debugging purposes
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __TPCH_TBL_READERS_H
#define __TPCH_TBL_READERS_H

#include <db_cxx.h>
#include <cstdio>

#include "util/namespace.h"

ENTER_NAMESPACE(tpch);

/* exported functions */

void tpch_read_tbl_CUSTOMER (Db* db);
void tpch_read_tbl_LINEITEM (Db* db);
void tpch_read_tbl_NATION   (Db* db);
void tpch_read_tbl_ORDERS   (Db* db);
void tpch_read_tbl_PART     (Db* db);
void tpch_read_tbl_PARTSUPP (Db* db);
void tpch_read_tbl_REGION   (Db* db);
void tpch_read_tbl_SUPPLIER (Db* db);


EXIT_NAMESPACE(tpch);

#endif
