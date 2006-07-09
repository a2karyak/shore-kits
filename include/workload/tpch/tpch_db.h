
#ifndef _TPCH_DB_H
#define _TPCH_DB_H

#include "tests/common/tpch_env.h"
#include "tests/common/tpch_db_load.h"
#include "engine/bdb_config.h"
#include <db_cxx.h>


bool db_open(u_int32_t flags=DB_RDONLY|DB_THREAD,
             u_int32_t db_cache_size_gb=BDB_BUFFER_POOL_SIZE_GB,
             u_int32_t db_cache_size_bytes=BDB_BUFFER_POOL_SIZE_BYTES);
bool db_close();


#endif
