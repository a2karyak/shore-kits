/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file tpcc_db.cpp
 *
 *  @brief Creates and configures the transaction processing database
 *  uses the BerkeleyDB as the underlying storage manager.
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#include "util.h"

#include "workload/common.h"
#include "workload/common/bdb_env.h"
#include "workload/common/bdb_config.h"

#include "workload/tpcc/tpcc_env.h"
#include "workload/tpcc/tpcc_db.h"
#include "workload/tpcc/tpcc_filenames.h"



using namespace qpipe;
using namespace workload;


ENTER_NAMESPACE(tpcc);



/** @fn db_open
 *
 *  @brief Open TPC-C tables. Initializes a transactional 
 *  subsystem.
 *
 *  @return void
 *
 *  @throw BdbException on error.
 */

void db_open(u_int32_t flags, u_int32_t db_cache_size_gb, 
             u_int32_t db_cache_size_bytes) 
{
    TRACE(TRACE_ALWAYS,
          "TPC-C DB_OPEN called\n");


    // create environment
    try {
        dbenv = new DbEnv(0);
        dbenv->set_errpfx(BDB_ERROR_PREFIX);
    }
    catch (DbException &e) {
        TRACE(TRACE_ALWAYS, "Caught DbException creating new DbEnv object: %s\n", 
              e.what());
        THROW1(BdbException, "Could not create new DbEnv");
    }
  
    // specify buffer pool size
    try {
        if (dbenv->set_cachesize(db_cache_size_gb, db_cache_size_bytes, 0)) {
            TRACE(TRACE_ALWAYS, "dbenv->set_cachesize() failed!\n");
            THROW1(BdbException, "dbenv->set_cachesize() failed!\n");
        }
    }
    catch (DbException &e) {
        TRACE(TRACE_ALWAYS, 
              "Caught DbException setting buffer pool size to %d GB, %d B: %s\n",
              db_cache_size_gb,
              db_cache_size_bytes,
              e.what());
        THROW1(BdbException, "dbenv->set_cachesize() threw DbException");
    }
  
  
    // set data directory
    try {

        const char* desc = "BDB_TPCC_DIRECTORY (BDB data)";
        const char* dir  = BDB_TPCC_DIRECTORY;

        if (fileops_check_directory_accessible(dir))
            THROW3(BdbException, "%s %s not accessible.\n",
                   desc,
                   dir);

        // data directory stores table files
        dbenv->set_data_dir(BDB_TPCC_DIRECTORY);
    }
    catch (DbException &e) {
        TRACE(TRACE_ALWAYS,
              "Caught DbException setting data directory to \"%s\"."
              " Make sure directory exists\n",
              BDB_TPCC_DIRECTORY);
        TRACE(TRACE_ALWAYS, "DbException: %s\n", e.what());
        THROW1(BdbException, "dbenv->set_data_dir() threw DbException");
    }
  
  
    // set temp directory
    try {

        const char* desc = "BDB_TEMP_DIRECTORY (BDB temp)";
        const char* dir  = BDB_TEMP_DIRECTORY;

        if (fileops_check_directory_accessible(dir))
            THROW3(BdbException, "%s %s not accessible.\n",
                   desc,
                   dir);

        if (fileops_check_file_writeable(dir))
            THROW3(BdbException, "%s %s not writeable.\n",
                   desc,
                   dir);

        dbenv->set_tmp_dir(BDB_TEMP_DIRECTORY);
    }
    catch ( DbException &e) {
        TRACE(TRACE_ALWAYS,
              "Caught DbException setting temp directory to \"%s\"."
              " Make sure directory exists\n",
              BDB_TEMP_DIRECTORY);
        TRACE(TRACE_ALWAYS, "DbException: %s\n", e.what());
        THROW1(BdbException, "dbenv->set_tmp_dir() threw DbException");
    }

    dbenv->set_msgfile(stderr);
    dbenv->set_errfile(stderr);
    // TODO: DB_NOMMAP?
    // dbenv->set_flags(DB_NOMMAP, true);
    dbenv->set_verbose(DB_VERB_REGISTER, true);
    

    // Open TPC-C Database Environment

    try {        
        const char* desc = "BDB_HOME_DIRECTORY (BDB home)";
        const char* dir  = BDB_HOME_DIRECTORY;

        if (fileops_check_directory_accessible(dir))
            THROW3(BdbException, "%s %s not accessible.\n",
                   desc,
                   dir);

        if (fileops_check_file_writeable(dir))
            THROW3(BdbException, "%s %s not writeable.\n",
                   desc,
                   dir);


        // initialize the transactional subsystem
        u_int32_t env_flags = 
            DB_CREATE     | // If the environment does not exist, create it
            DB_PRIVATE    | // The env will be accessed by a single process
            DB_INIT_LOCK  | // Initialize locking
            DB_INIT_LOG   | // Initialize logging
            DB_INIT_MPOOL | // Initialize the cache
            DB_THREAD     | // Free-thread the env handle
            DB_INIT_TXN;    // Initialize transactions  



        // open environment with no transactional support.
        dbenv->open(BDB_HOME_DIRECTORY,
                    env_flags,
                    0);
    }
    catch ( DbException &e) {
        TRACE(TRACE_ALWAYS,
              "Caught DbException opening home directory \"%s\"."
              " Make sure directory exists\n",
              BDB_HOME_DIRECTORY);
        TRACE(TRACE_ALWAYS, "DbException: %s\n", e.what());
        THROW1(BdbException, "dbenv->open() threw DbException");
    }
  
    
    // open tables
    for (int i = 0; i < _TPCC_TABLE_COUNT_; i++)
        open_db_table(tpcc_tables[i].db,
                      flags,
                      tpcc_tables[i].bt_compare_fn,
                      tpcc_tables[i].bdb_filename);
    

    /*
    // open indexes
    open_db_index(tpch_tables[TPCH_TABLE_LINEITEM].db,
                  tpch_lineitem_shipdate,
                  tpch_lineitem_shipdate_idx,
                  flags,
                  tpch_lineitem_shipdate_compare_fcn,
                  tpch_lineitem_shipdate_key_fcn,
                  INDEX_LINEITEM_SHIPDATE_NAME,
                  INDEX_LINEITEM_SHIPDATE_NAME "_IDX");
    */                  

    TRACE(TRACE_ALWAYS, "BerkeleyDB buffer pool set to %d GB, %d B\n",
          db_cache_size_gb,
          db_cache_size_bytes);

    TRACE(TRACE_ALWAYS, "TPC-C database open\n");
}




/** @fn db_close
 *  @brief Close TPC-C tables.
 *
 *  @return void
 *
 *  @throw BdbException on error.
 */

void db_close() {

    // close tables
    for (int i = 0; i < _TPCC_TABLE_COUNT_; i++)
        close_db_table(tpcc_tables[i].db, 
                       BDB_TPCC_DIRECTORY,
                       tpcc_tables[i].bdb_filename);
        

    /*
    // close indexes
    close_db_table(tpch_lineitem_shipdate_idx, INDEX_LINEITEM_SHIPDATE_NAME "_IDX");
    close_db_table(tpch_lineitem_shipdate, INDEX_LINEITEM_SHIPDATE_NAME);
    */


    // close environment
    try {    
 	dbenv->close(0);
    }
    catch ( DbException &e) {
	TRACE(TRACE_ALWAYS, "Caught DbException closing environment\n");
        TRACE(TRACE_ALWAYS, "DbException: %s\n", e.what());
        THROW1(BdbException, "dbenv->close() threw DbException");
    }

    TRACE(TRACE_ALWAYS, "TPC-C database closed\n");
}



/** @fn db_read
 *  @brief Reads the TPC-C tables. For test purposes
 *
 *  @throw BdbException on error.
 */

void db_read() {

    TRACE( TRACE_ALWAYS, "For Debug!!\n");


    // close tables
    for (int i = 0; i < _TPCC_TABLE_COUNT_; i++) {

        TRACE( TRACE_ALWAYS, "To be done!\n");
    }   

    TRACE(TRACE_ALWAYS, "TPC-C database read\n");
}





EXIT_NAMESPACE(tpcc);
