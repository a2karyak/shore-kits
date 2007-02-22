/* -*- mode:C++; c-basic-offset:4 -*- */

#include "util.h"
#include "workload/tpch/tpch_db.h"
#include "workload/tpch/tpch_filenames.h"
#include "workload/tpch/tpch_compare.h"
#include "workload/bdb_config.h"




static void open_db_table(Db*& table, u_int32_t flags,
                          bt_compare_func_t cmp,
                          const char* table_name);
static void open_db_index(Db* table, Db* &assoc, Db*& index, u_int32_t flags,
                          bt_compare_func_t cmp,
                          idx_key_create_func_t key_create,
                          const char* assoc_name, const char* index_name);
static void close_db_table(Db*& table, const char* table_name);



/**
 *  @brief Open TPC-H tables.
 *
 *  @return void
 *
 *  @throw BdbException on error.
 */
void db_open(u_int32_t flags, u_int32_t db_cache_size_gb, u_int32_t db_cache_size_bytes) {

    // create environment
    try {
        dbenv = new DbEnv(0);
        dbenv->set_errpfx(BDB_ERROR_PREFIX);
    }
    catch ( DbException &e) {
        TRACE(TRACE_ALWAYS, "Caught DbException creating new DbEnv object: %s\n", e.what());
        THROW1(BdbException, "Could not create new DbEnv");
    }
  
    // specify buffer pool size
    try {
        if (dbenv->set_cachesize(db_cache_size_gb, db_cache_size_bytes, 0)) {
            TRACE(TRACE_ALWAYS, "dbenv->set_cachesize() failed!\n");
            THROW1(BdbException, "dbenv->set_cachesize() failed!\n");
        }
    }
    catch ( DbException &e) {
        TRACE(TRACE_ALWAYS, "Caught DbException setting buffer pool size to %d GB, %d B: %s\n",
              db_cache_size_gb,
              db_cache_size_bytes,
              e.what());
        THROW1(BdbException, "dbenv->set_cachesize() threw DbException");
    }
  
  
    // set data directory
    try {

        const char* desc = "BDB_TPCH_DIRECTORY";
        const char* dir  = BDB_TPCH_DIRECTORY;

        if (fileops_check_directory_accessible(dir))
            THROW3(BdbException, "%s %s not accessible.\n",
                   desc,
                   dir);

        // data directory stores table files
        dbenv->set_data_dir(BDB_TPCH_DIRECTORY);
    }
    catch ( DbException &e) {
        TRACE(TRACE_ALWAYS, "Caught DbException setting data directory to \"%s\". Make sure directory exists\n",
              BDB_TPCH_DIRECTORY);
        TRACE(TRACE_ALWAYS, "DbException: %s\n", e.what());
        THROW1(BdbException, "dbenv->set_data_dir() threw DbException");
    }
  
  
    // set temp directory
    try {

        const char* desc = "BDB_TEMP_DIRECTORY";
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
              "Caught DbException setting temp directory to \"%s\". Make sure directory exists\n",
              BDB_TEMP_DIRECTORY);
        TRACE(TRACE_ALWAYS, "DbException: %s\n", e.what());
        THROW1(BdbException, "dbenv->set_tmp_dir() threw DbException");
    }

    dbenv->set_msgfile(stderr);
    dbenv->set_errfile(stderr);
    // TODO: DB_NOMMAP?
    // dbenv->set_flags(DB_NOMMAP, true);
    dbenv->set_verbose(DB_VERB_REGISTER, true);
    

    // open home directory
    try {
        
        const char* desc = "BDB_HOME_DIRECTORY";
        const char* dir  = BDB_HOME_DIRECTORY;

        if (fileops_check_directory_accessible(dir))
            THROW3(BdbException, "%s %s not accessible.\n",
                   desc,
                   dir);

        if (fileops_check_file_writeable(dir))
            THROW3(BdbException, "%s %s not writeable.\n",
                   desc,
                   dir);

        // open environment with no transactional support.
        dbenv->open(BDB_HOME_DIRECTORY,
                    DB_CREATE | DB_PRIVATE | DB_THREAD | DB_INIT_CDB | DB_INIT_MPOOL,
                    0);
    }
    catch ( DbException &e) {
        TRACE(TRACE_ALWAYS,
              "Caught DbException opening home directory \"%s\". Make sure directory exists\n",
              BDB_HOME_DIRECTORY);
        TRACE(TRACE_ALWAYS, "DbException: %s\n", e.what());
        THROW1(BdbException, "dbenv->open() threw DbException");
    }
  

    // open tables
    open_db_table(tpch_customer, flags, tpch_customer_bt_compare_fcn, BDB_FILENAME_CUSTOMER);
    open_db_table(tpch_lineitem, flags, tpch_lineitem_bt_compare_fcn, BDB_FILENAME_LINEITEM);
    open_db_table(tpch_nation,   flags, tpch_nation_bt_compare_fcn,   BDB_FILENAME_NATION);
    open_db_table(tpch_orders,   flags, tpch_orders_bt_compare_fcn,   BDB_FILENAME_ORDERS);
    open_db_table(tpch_part,     flags, tpch_part_bt_compare_fcn,     BDB_FILENAME_PART);
    open_db_table(tpch_partsupp, flags, tpch_partsupp_bt_compare_fcn, BDB_FILENAME_PARTSUPP);
    open_db_table(tpch_region,   flags, tpch_region_bt_compare_fcn,   BDB_FILENAME_REGION);
    open_db_table(tpch_supplier, flags, tpch_supplier_bt_compare_fcn, BDB_FILENAME_SUPPLIER);

    // open indexes
    open_db_index(tpch_lineitem, tpch_lineitem_shipdate,
                  tpch_lineitem_shipdate_idx,
                  flags,
                  tpch_lineitem_shipdate_compare_fcn,
                  tpch_lineitem_shipdate_key_fcn,
                  INDEX_LINEITEM_SHIPDATE_NAME,
                  INDEX_LINEITEM_SHIPDATE_NAME "_IDX");

    TRACE(TRACE_ALWAYS, "BerkeleyDB buffer pool set to %d GB, %d B\n",
          db_cache_size_gb,
          db_cache_size_bytes);
    TRACE(TRACE_ALWAYS, "TPCH database open\n");
}



/**
 *  @brief Close TPC-H tables.
 *
 *  @return void
 *
 *  @throw BdbException on error.
 */
void db_close() {


    // close indexes
    close_db_table(tpch_lineitem_shipdate_idx, INDEX_LINEITEM_SHIPDATE_NAME "_IDX");
    close_db_table(tpch_lineitem_shipdate, INDEX_LINEITEM_SHIPDATE_NAME);

    // close tables
    close_db_table(tpch_customer, BDB_FILENAME_CUSTOMER);
    close_db_table(tpch_lineitem, BDB_FILENAME_LINEITEM);
    close_db_table(tpch_nation,   BDB_FILENAME_NATION);
    close_db_table(tpch_orders,   BDB_FILENAME_ORDERS);
    close_db_table(tpch_part,     BDB_FILENAME_PART);
    close_db_table(tpch_partsupp, BDB_FILENAME_PARTSUPP);
    close_db_table(tpch_region,   BDB_FILENAME_REGION);
    close_db_table(tpch_supplier, BDB_FILENAME_SUPPLIER);

    
    // close environment
    try {    
 	dbenv->close(0);
    }
    catch ( DbException &e) {
	TRACE(TRACE_ALWAYS, "Caught DbException closing environment\n");
        TRACE(TRACE_ALWAYS, "DbException: %s\n", e.what());
        THROW1(BdbException, "dbenv->close() threw DbException");
    }

}



/**
 *  @brief Open the specified table.
 *
 *  @return void
 *
 *  @throw BdbException on error.
 */
static void open_db_table(Db*& table, u_int32_t flags,
                          bt_compare_func_t cmp, const char* table_name) {
    try {
        table = new Db(dbenv, 0);
        table->set_bt_compare(cmp);
        table->set_pagesize(4096);
        table->open(NULL, table_name, NULL, DB_BTREE, flags, 0644);
    }
    catch ( DbException &e) {
        TRACE(TRACE_ALWAYS,
              "Caught DbException opening table \"%s\". Make sure database is set up properly.\n",
              table_name);
        TRACE(TRACE_ALWAYS, "DbException: %s\n", e.what());
        THROW1(BdbException, "table->open() failed");
    }
}



/**
 *  @brief Close the specified table.
 *
 *  @return void
 *
 *  @throw BdbException on error.
 */
static void close_db_table(Db* &table, const char* table_name) {

    try {
        table->close(0);
    }
    catch ( DbException &e) {
	TRACE(TRACE_ALWAYS, "Caught DbException closing table \"%s\"\n",
              table_name);
        TRACE(TRACE_ALWAYS, "DbException: %s\n", e.what());
        THROW1(BdbException, "table->close() failed");
    }
}



/**
 *  @brief Open the specified table index.
 *
 *  @return void
 *
 *  @throw BdbException on error.
 */
static void open_db_index(Db* table, Db*& assoc, Db*& index, u_int32_t,
                          bt_compare_func_t cmp,
                          idx_key_create_func_t key_create,
                          const char* , const char* index_name) {

    try {
        assoc = new Db(dbenv, 0);
        assoc->set_bt_compare(cmp);
        // not necessarily unique...
        assoc->set_flags(DB_DUP);
        assoc->open(NULL, index_name, NULL, DB_BTREE, DB_THREAD | DB_CREATE, 0644);
        table->associate(NULL, assoc, key_create, DB_CREATE);

        index = new Db(dbenv, 0);
        index->set_bt_compare(cmp);
        index->set_flags(DB_DUP);
        index->open(NULL, index_name, NULL, DB_BTREE, DB_THREAD, 0644);
        
    }
    catch ( DbException &e) {
        TRACE(TRACE_ALWAYS, "Caught DbException opening index \"%s\". Make sure database is set up properly\n",
              index_name);
        TRACE(TRACE_ALWAYS, "DbException: %s\n", e.what());
        THROW1(BdbException, "index->open() failed");
    }
}

