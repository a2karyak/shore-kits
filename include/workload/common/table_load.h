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
#include "workload/common/bdb_config.h"

#include "workload/tpch/tpch_db.h"
#include "workload/tpcc/tpcc_db.h"

using namespace tpch;
using namespace tpcc;


ENTER_NAMESPACE(workload);

void db_tpch_load(const char* tbl_path=BDB_TPCH_TBL_DIRECTORY);
void db_tpcc_load(const char* tbl_path=BDB_TPCC_TBL_DIRECTORY);


/** for verification purposes */
void db_tpch_read();
void db_tpcc_read();


EXIT_NAMESPACE(workload);

#endif
