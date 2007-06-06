/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef __BDB_ENV_H
#define __BDB_ENV_H

#include <inttypes.h>

#include <db_cxx.h>

#include "util.h"
#include "workload/bdb_config.h"

ENTER_NAMESPACE(workload);

/* exported datatypes */

typedef int (*bt_compare_fn_t) (Db*, const Dbt*, const Dbt*);
typedef void (*parse_tbl_t) (Db*, FILE*);

typedef int (*bt_compare_func_t)(Db*, const Dbt*, const Dbt*);
typedef int (*idx_key_create_func_t)(Db*, const Dbt*, 
                                     const Dbt*, Dbt*);



// bdb_table_s: Class the represents a table/file
class bdb_table_s {
public:
    const char* tbl_filename;
    const char* bdb_filename;
    const char* table_id;

    Db* db;

    bt_compare_fn_t bt_compare_fn;
    parse_tbl_t     parse_tbl;
};


/* BerkeleyDB environment */

extern DbEnv* dbenv;

/* exported functions */


/**
 *  @brief Open the specified table.
 *
 *  @return void
 *
 *  @throw BdbException on error.
 */
static void open_db_table(Db*& table, u_int32_t flags,
                          bt_compare_func_t cmp,
                          const char* table_name) 
{
    c_str path("%s", table_name);
    
    try {
        table = new Db(dbenv, 0);
        table->set_bt_compare(cmp);
        table->set_pagesize(4096);
        table->open(NULL, path.data(), NULL, DB_BTREE, flags, 0644);
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

static void open_db_index(Db* table, Db* &assoc, 
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
        assoc->set_flags(DB_DUP);
        assoc->open(NULL, path.data(), NULL, DB_BTREE, DB_THREAD | DB_CREATE, 0644);
        table->associate(NULL, assoc, key_create, DB_CREATE);

        index = new Db(dbenv, 0);
        index->set_bt_compare(cmp);
        index->set_flags(DB_DUP);
        index->open(NULL, path.data(), NULL, DB_BTREE, DB_THREAD, 0644);
        
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



/**
 *  @brief Close the specified table.
 *
 *  @return void
 *
 *  @throw BdbException on error.
 */
static void close_db_table(Db* &table, 
                           const char* table_name) {

    c_str path("%s/%s", BDB_TPCH_DIRECTORY, table_name);

    try {
        table->close(0);
    }
    catch ( DbException &e) {
	TRACE(TRACE_ALWAYS, "Caught DbException closing table \"%s\"\n",
              path.data());
        TRACE(TRACE_ALWAYS, "DbException: %s\n", e.what());
        THROW1(BdbException, "table->close() failed");
    }
}


EXIT_NAMESPACE(workload);

#endif
