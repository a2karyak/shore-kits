/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file tpcc_db.h
 *
 *  @brief Interface for the functionality that creates and configures
 *  the transaction processing database. The current implemetnation uses 
 *  BerkeleyDB as the underlying storage manager, logging, and locking 
 *  engine.
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __TPCC_DB_H
#define __TPCC_DB_H

#include <inttypes.h>

#include "workload/common/bdb_config.h"

ENTER_NAMESPACE(tpcc);


/** Transaction processing engine parameters */


/** @note Define tpcc data directories */
#define BDB_TPCC_TBL_DIRECTORY   "tbl_tpcc"
#define BDB_TPCC_DIRECTORY       "database_bdb_tpcc"


/** @note Define the dbopen flags. This flag defines, among others,
 *  the possible isolation level. BDB's default isolation level is
 *  SERIALIZABLE (level 3). Other possible values are:
 *  READ UNCOMMITTED (level 1) - Allows reads of dirtied (uncommitted) data.
 *  READ COMMITTED (level 2)   - Releases read locks before trx ends.
 *  as well as 
 *  SNAPSHOT ISOLATION - Which is a form a multiversion concurrency control.
 *
 *  @note In order to enable READ UNCOMMITTED isolation we must pass this
 *  parameter in the dbopen function.
 *
 *  @note In order to open a table with transactional support the environmental
 *  handle should be passed, and the database must open within a transaction.
 *  For this reason DB_AUTO_COMMIT is used in the database open command.
 */

#define BDB_TPCC_DB_OPEN_FLAGS DB_CREATE | DB_AUTO_COMMIT
//#define BDB_TPCC_DB_OPEN_FLAGS DB_CREATE | DB_READ_UNCOMMITTED


/** @note Define the logging method
 *  There are two types of logging, regural on disk logging and in-memory logging.
 *  The latter is dangerous since there is are no transaction durability guarantees.
 */

#define BDB_TPCC_LOGGING_METHOD BDB_IN_MEMORY_LOGGING
//#define BDB_TPCC_LOGGING_METHOD BDB_REGULAR_LOGGING



/** @note Define the env flags.
 *
 *  @note In order to enable SNAPSHOT ISOLATION the environment should be 
 *  opened with multiversion support.
 */

//#define BDB_TPCC_ENV_OPEN_FLAGS DB_MULTIVERSION


/** Exported functions */

void db_open(int in_memory,
             uint32_t flags,
             uint32_t db_cache_size_gb=1,
             uint32_t db_cache_size_bytes=0);

void db_close();

void db_checkpoint();

void db_read(); /* for debugging purposes */



EXIT_NAMESPACE(tpcc);

#endif
