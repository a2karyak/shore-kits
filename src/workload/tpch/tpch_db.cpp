/* -*- mode:C++; c-basic-offset:4 -*- */

#include "util.h"
#include "workload/tpch/tpch_db.h"
#include "workload/tpch/tpch_tables.h"
#include "workload/tpch/tpch_compare.h"
#include "workload/tpch/bdb_config.h"




static void open_db_table(page_list* table, const char* table_name);
static void close_db_table(page_list* table, const char* table_name);

using namespace qpipe;
/**
 *  @brief Open TPC-H tables.
 *
 *  @return void
 *
 *  @throw BdbException on error.
 */
void db_open(u_int32_t, u_int32_t db_cache_size_gb, u_int32_t db_cache_size_bytes) {

    // create environment
    try {
        dbenv = new DbEnv(0);
        dbenv->set_errpfx("qpipe");
    }
    catch ( DbException &e) {
        TRACE(TRACE_ALWAYS, "Caught DbException creating new DbEnv object: %s\n",
              e.what());
        throw EXCEPTION(BdbException, "Could not create new DbEnv");
    }

    // specify buffer pool size
    try {
        if (dbenv->set_cachesize(db_cache_size_gb, db_cache_size_bytes, 0)) {
            TRACE(TRACE_ALWAYS, "dbenv->set_cachesize() failed!\n");
            throw EXCEPTION(BdbException, "dbenv->set_cachesize() failed!\n");
        }
    }
    catch ( DbException &e) {
        TRACE(TRACE_ALWAYS, "Caught DbException setting buffer pool size to "
              "%d GB, %d B: %s\n", db_cache_size_gb, db_cache_size_bytes,
              e.what());
        throw EXCEPTION(BdbException, "dbenv->set_cachesize() threw DbException");
    }


    // set data directory
    try {
        // data directory stores table files
        dbenv->set_data_dir(BDB_DATA_DIRECTORY);
    }
    catch ( DbException &e) {
        TRACE(TRACE_ALWAYS, "Caught DbException setting data directory to \"%s\". "
              "Make sure directory exists\n", BDB_DATA_DIRECTORY);
        TRACE(TRACE_ALWAYS, "DbException: %s\n", e.what());
        throw EXCEPTION(BdbException, "dbenv->set_data_dir() threw DbException");
    }


    // set temp directory
    try {
        dbenv->set_tmp_dir(BDB_TEMP_DIRECTORY);
    }
    catch ( DbException &e) {
        TRACE(TRACE_ALWAYS, "Caught DbException setting temp directory to \"%s\". "
              "Make sure directory exists\n", BDB_TEMP_DIRECTORY);
        TRACE(TRACE_ALWAYS, "DbException: %s\n", e.what());
        throw EXCEPTION(BdbException, "dbenv->set_tmp_dir() threw DbException");
    }

    dbenv->set_msgfile(stderr);
    dbenv->set_errfile(stderr);

    // TODO: DB_NOMMAP?
    dbenv->set_flags(DB_NOMMAP|DB_DIRECT_DB, true);
    dbenv->set_verbose(DB_VERB_REGISTER, true);
        
            
    // open home directory
    try {
        // open environment with no transactional support.
        dbenv->open(BDB_HOME_DIRECTORY,
                    DB_CREATE | DB_PRIVATE | DB_THREAD | DB_INIT_CDB | DB_INIT_MPOOL,
                    0);
    }
    catch ( DbException &e) {
        TRACE(TRACE_ALWAYS, "Caught DbException opening home directory \"%s\". Make sure directory exists\n",
              BDB_HOME_DIRECTORY);
        TRACE(TRACE_ALWAYS, "DbException: %s\n", e.what());
        throw EXCEPTION(BdbException, "dbenv->open() threw DbException");
    }
            
    // open tables
    open_db_table(tpch_customer, TABLE_CUSTOMER_NAME);
    open_db_table(tpch_lineitem, TABLE_LINEITEM_NAME);
    open_db_table(tpch_nation, TABLE_NATION_NAME);
    open_db_table(tpch_orders, TABLE_ORDERS_NAME);
    open_db_table(tpch_part, TABLE_PART_NAME);
    open_db_table(tpch_partsupp, TABLE_PARTSUPP_NAME);
    open_db_table(tpch_region, TABLE_REGION_NAME);
    open_db_table(tpch_supplier, TABLE_SUPPLIER_NAME);

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

    // close tables
    close_db_table(tpch_customer, TABLE_CUSTOMER_NAME);
    close_db_table(tpch_lineitem, TABLE_LINEITEM_NAME);
    close_db_table(tpch_nation, TABLE_NATION_NAME);
    close_db_table(tpch_orders, TABLE_ORDERS_NAME);
    close_db_table(tpch_part, TABLE_PART_NAME);
    close_db_table(tpch_partsupp, TABLE_PARTSUPP_NAME);
    close_db_table(tpch_region, TABLE_REGION_NAME);
    close_db_table(tpch_supplier, TABLE_SUPPLIER_NAME);
}



/**
 *  @brief Open the specified table.
 *
 *  @return void
 *
 *  @throw BdbException on error.
 */
static void open_db_table(page_list* table, const char* table_name) {
    char fname[256];
    sprintf(fname, "database/%s", table_name);
    
    FILE* f = fopen(fname, "r");
    if(f == NULL) 
        throw EXCEPTION(FileException, "Unable to open %s\n", fname);
    
    page* p = page::alloc(1);
    while(p->fread_full_page(f)) {
        table->push_back(p);
        p = page::alloc(1);
    }
    
    // this one didn't get used...
    p->free();
    fclose(f);
}



/**
 *  @brief Close the specified table.
 *
 *  @return void
 *
 *  @throw BdbException on error.
 */
static void close_db_table(page_list* table, const char*) {
    for(page_list::iterator it=table->begin(); it != table->end(); ++it)
        (*it)->free();
    table->clear();
}

