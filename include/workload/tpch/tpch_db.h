/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef __TPCH_DB_H
#define __TPCH_DB_H

#include <inttypes.h>

ENTER_NAMESPACE(tpch);

void db_open(uint32_t flags=0,
             uint32_t db_cache_size_gb=1,
             uint32_t db_cache_size_bytes=0);

void db_close();

EXIT_NAMESPACE(tpch);

#endif
