/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file bdb_env.cpp
 *
 *  @brief Common operations on the storage manager between the 
 *  two engines (tpch and tpcc)
 *
 *  @author Ippokratis Pandis (ipandis)
 */


#include "util.h"
#include "workload/common/bdb_env.h"

ENTER_NAMESPACE(workload);

/* exported data structures */

/* BDB environment */
DbEnv* dbenv = NULL;

/* exported functions */


/**
 *  @brief Open the specified table.
 *
 *  @return void
 *
 *  @throw BdbException on error.
 */

void open_db_table(Db*& table, u_int32_t flags,
                   bt_compare_func_t cmp,
                   const char* table_name) 
{
    c_str path("%s", table_name);
    
    try {
        assert (dbenv != NULL); /* for transaction-support dbenv should not be NULL */

        table = new Db(dbenv, 0);
        table->set_bt_compare(cmp);
        table->set_pagesize(BDB_PAGESIZE);

        /* Opens the table with support for duplicated records.
        int ret = table->set_flags(DB_DUPSORT);
        if (ret != 0) {
            table->err(ret, "Attempt to set DUPSORT flag failed.");
            TRACE (TRACE_ALWAYS, "Attempt to set DUPSORT flag failed");
        }
        */

        table->open(NULL, 
                    path.data(), 
                    NULL, 
                    DB_BTREE, 
                    flags, 
                    0644);

        TRACE(TRACE_DEBUG, "Table (%s) opened...\n", table_name);
    }
    catch ( DbException &e) {
        TRACE(TRACE_ALWAYS,
              "Caught DbException opening table \"%s\". "
              "Make sure database is set up properly.\n",
              path.data());
        TRACE(TRACE_ALWAYS, "DbException: %s\n", e.what());
        THROW1(BdbException, "table->open() failed");
    }
}



/**
 *  @brief Open the specified table index.
 *
 *  @return void
 *
 *  @throw BdbException on error.
 */

void open_db_index(Db* table, Db* &assoc, 
                   Db*& index, 
                   u_int32_t,
                   bt_compare_func_t cmp,
                   idx_key_create_func_t key_create,
                   const char*, 
                   const char* index_name) 
{
    c_str path("%s", index_name);
    
    try {
        assoc = new Db(dbenv, 0);
        assoc->set_bt_compare(cmp);
        // not necessarily unique...
        //        assoc->set_flags(DB_DUP);
        assoc->open(NULL, 
                    path.data(), 
                    NULL, 
                    DB_BTREE, 
                    DB_THREAD | DB_CREATE, 
                    0644);
        table->associate(NULL, assoc, key_create, DB_CREATE);

        index = new Db(dbenv, 0);
        index->set_bt_compare(cmp);
        index->set_flags(DB_DUP);
        index->open(NULL, 
                    path.data(), 
                    NULL, 
                    DB_BTREE, 
                    DB_THREAD, 
                    0644);
        
    }
    catch ( DbException &e) {
        TRACE(TRACE_ALWAYS,
              "Caught DbException opening index \"%s\". "
              "Make sure database is set up properly\n",
              path.data());
        TRACE(TRACE_ALWAYS, "DbException: %s\n", e.what());
        THROW1(BdbException, "index->open() failed");
    }
}



/** @fn close_db_table
 *  @brief Close the specified table.
 *
 *  @throw BdbException on error.
 */

void close_db_table(Db* &table, 
                    const char* dir_name,
                    const char* table_name,
                    const int is_trx)
{    
    c_str path("%s/%s", dir_name, table_name);

    try {
        table->close(0);
        TRACE(TRACE_DEBUG, "Table (%s) closed...\n", table_name);

        if (is_trx) {

            /** @note Once we close the database table successfully and we
             *  have applied transactions (it is a trx table) we also reset
             *  the corresponding LSN number
             */
            dbenv->lsn_reset(path, 0);
        }
    }
    catch ( DbException &e) {
	TRACE(TRACE_ALWAYS, "Caught DbException closing table \"%s\"\n",
              path.data());
        TRACE(TRACE_ALWAYS, "DbException: %s\n", e.what());
        THROW1(BdbException, "table->close() failed");
    }
}




EXIT_NAMESPACE(workload);



