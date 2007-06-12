/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _TPCC_DB_H
#define _TPCC_DB_H

#include <inttypes.h>

//#include "workload/tpch/tpch_env.h"
//#include "workload/tpch/tpch_db_load.h"

ENTER_NAMESPACE(tpcc);

void db_open(uint32_t flags=0,
             uint32_t db_cache_size_gb=1,
             uint32_t db_cache_size_bytes=0);

void db_close();

void db_read();

EXIT_NAMESPACE(tpcc);

#endif
