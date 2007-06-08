/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file table_load.h
 *
 *  @brief Interface for the database load functions
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __TABLE_DB_LOAD_H
#define __TABLE_DB_LOAD_H

#include "util/namespace.h"
#include "workload/bdb_config.h"

ENTER_NAMESPACE(workload);

void db_tpch_load(const char* tbl_path=BDB_TPCH_TBL_DIRECTORY);
void db_tpcc_load(const char* tbl_path=BDB_TPCC_TBL_DIRECTORY);

EXIT_NAMESPACE(workload);

#endif
