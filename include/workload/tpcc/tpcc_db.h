/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _TPCC_DB_H
#define _TPCC_DB_H

#include <inttypes.h>

//#include "workload/tpch/tpch_env.h"
//#include "workload/tpch/tpch_db_load.h"

ENTER_NAMESPACE(tpcc);


/** Transaction processing engine parameters */

/** @note Define the maximum number of locks. BDB's default
 *  value is 1000, which is low and may result to ENOMEM
 *  errors at run-time
 */
#define BDB_MAX_LOCKS   40000
#define BDB_MAX_OBJECTS 40000


/** @note Define the maximum number of possible in-flight
 *  transactions. BDB's default value is 10.
 */
#define BDB_MAX_TRX     100


/** @note Define the timeout value for the transactions.
 *  BDB's default value is 0, which means that there is 
 *  no timeout.
 */
#define BDB_SEC         1000000
#define BDB_TRX_TIMEOUT 0 * SEC


/** Exported functions */

void db_open(uint32_t flags=0,
             uint32_t db_cache_size_gb=1,
             uint32_t db_cache_size_bytes=0);

void db_close();

void db_read();

EXIT_NAMESPACE(tpcc);

#endif
