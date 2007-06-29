/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file bdb_config.h
 *
 *  @brief Basic configuration for the BDB environment.
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __BDB_CONFIG_H
#define __BDB_CONFIG_H



// buffer pool: set size equal to 450 MB -- Maximum is 4GB in 32-bit
// platforms
#define BDB_BUFFER_POOL_SIZE_GB    1             /* 1 GB */
#define BDB_BUFFER_POOL_SIZE_BYTES 600*1024*1024 /* 600MB */

#define BDB_ERROR_PREFIX "BDB_ERROR"

// General
#define BDB_HOME_DIRECTORY  "."
#define BDB_TEMP_DIRECTORY  "temp"


/** @note Define the page size of the database tables */
#define QUERY_ENV_PAGESIZE 4096
#define TRX_ENV_PAGESIZE   4096
#define BDB_PAGESIZE       TRX_ENV_PAGESIZE

/** @note Define the maximum number of lockers, locks and locked objects. 
 *  BDB's default value is 1000 for each of them. This value low and may 
 *  result to ENOMEM errors at run-time, especially when the number of 
 *  clients is high.
 */
#define BDB_MAX_LOCKERS 40000
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
#define BDB_TRX_TIMEOUT 0 * BDB_SEC



/** @note Define the logging method
 *  There are two types of logging, regural on disk logging and in-memory logging.
 *  The latter is dangerous since there is are no transaction durability guarantees.
 */

#define BDB_IN_MEMORY_LOGGING  1
#define BDB_REGULAR_LOGGING    0



#endif
