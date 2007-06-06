/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef __TPCH_DB_LOAD_H
#define __TPCH_DB_LOAD_H

#include "util/namespace.h"
#include "workload/bdb_config.h"

ENTER_NAMESPACE(tpch);

void db_load(const char* tbl_path=BDB_TPCH_TBL_DIRECTORY);

EXIT_NAMESPACE(tpch);

#endif
