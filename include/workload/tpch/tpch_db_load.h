/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _TPCH_DB_LOAD_H
#define _TPCH_DB_LOAD_H

#include "workload/tpch/tpch_filenames.h"
#include "workload/bdb_config.h"

void db_load(const char* tbl_path=BDB_TBL_DIRECTORY);

#endif
