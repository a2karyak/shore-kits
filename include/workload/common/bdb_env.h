/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file bdb_env.cpp
 *
 *  @brief Defines datatypes/functions/variables for the BerkeleyDB enviroment
 *
 *  @author Ippokratis Pandis (ipandis)
 */


#ifndef __BDB_ENV_H
#define __BDB_ENV_H

#include <inttypes.h>

#include <db_cxx.h>

#include "util.h"
#include "workload/common/bdb_config.h"

ENTER_NAMESPACE(workload);

/* definition of macros */

// 'typeof' is a gnu extension
#define TYPEOF(TYPE, MEMBER) __typeof__(((TYPE *)0)->MEMBER)


#define SIZEOF(TYPE, MEMBER) sizeof(((TYPE *)0)->MEMBER)

// 'offsetof' is found in <stddef.h>
#define FIELD(SRC, TYPE, MEMBER) (((char*) (SRC)) + offsetof(TYPE, MEMBER))

#define GET_FIELD(DEST, SRC, TYPE, MEMBER) \
    memcpy(DEST, \
           FIELD(SRC, TYPE, MEMBER), \
           SIZEOF(TYPE, MEMBER))


// load max line constant
#define LOAD_MAX_LINE_LENGTH 1024


/* exported datatypes */

typedef int (*bt_compare_fn_t) (Db*, const Dbt*, const Dbt*);
typedef void (*parse_tbl_t) (Db*, FILE*);
typedef void (*read_tbl_t) (Db*);


typedef int (*bt_compare_func_t)(Db*, const Dbt*, const Dbt*);
typedef int (*idx_key_create_func_t)(Db*, const Dbt*, 
                                     const Dbt*, Dbt*);


// bdb_table_s: Class the represents a table/file
struct bdb_table_s {
    const char* tbl_filename;
    const char* bdb_filename;
    const char* table_id;

    Db* db;

    bt_compare_fn_t bt_compare_fn;
    parse_tbl_t     parse_tbl;
    read_tbl_t      read_tbl;
};



/* BerkeleyDB environment */

extern DbEnv* dbenv;


/* exported functions */

extern void open_db_table(Db*& table, u_int32_t flags,
                          bt_compare_func_t cmp,
                          const char* table_name);

extern void open_db_index(Db* table, Db* &assoc, 
                          Db*& index, 
                          u_int32_t,
                          bt_compare_func_t cmp,
                          idx_key_create_func_t key_create,
                          const char*, 
                          const char* index_name);

extern void close_db_table(Db* &table, 
                           const char* dir_name,
                           const char* table_name,
                           const int is_trx = 0);




EXIT_NAMESPACE(workload);

#endif
