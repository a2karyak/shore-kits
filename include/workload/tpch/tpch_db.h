/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file tpch_db.h
 *
 *  @brief Interface for the functionality that creates and configures
 *  the query execution database. The current implemetnation uses 
 *  BerkeleyDB as the underlying storage manager, logging, and locking 
 *  engine.
 */

#ifndef __TPCH_DB_H
#define __TPCH_DB_H

#include <inttypes.h>

#include "workload/common/bdb_config.h"

ENTER_NAMESPACE(tpch);


/** Query engine parameters */

/** @note Define tpch data directories */
#define BDB_TPCH_TBL_DIRECTORY   "tbl_tpch"
#define BDB_TPCH_DIRECTORY       "database_bdb_tpch"


/** Exported functions */

void db_open(uint32_t flags=0,
             uint32_t db_cache_size_gb=1,
             uint32_t db_cache_size_bytes=0);

void db_close();

EXIT_NAMESPACE(tpch);

#endif
