/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef __TPCC_DB_LOAD_H
#define __TPCC_DB_LOAD_H

#include "util/namespace.h"
#include "workload/bdb_config.h"

ENTER_NAMESPACE(tpcc);

void db_tpcc_load(const char* tbl_path=BDB_TPCC_TBL_DIRECTORY);

EXIT_NAMESPACE(tpcc);

#endif
