/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _TPCH_ENV_H
#define _TPCH_ENV_H

#include <cstdlib>
#include <vector>
#include "core.h"



/* exported datatypes and structures */

typedef std::vector<qpipe::page*> page_list;
typedef void (*tuple_parser_t) (char*, char*);

struct cdb_table_s
{
    const char* tbl_filename;
    const char* cdb_filename;
    const char* table_id;
    
    page_list*  db;
    
    tuple_parser_t  tuple_parser;

    size_t tuple_size;
};



/* exported constants */

enum tpch_table_list_t {
    TPCH_TABLE_CUSTOMER = 0,
    TPCH_TABLE_LINEITEM = 1,
    TPCH_TABLE_NATION   = 2,
    TPCH_TABLE_ORDERS   = 3,
    TPCH_TABLE_PART     = 4,
    TPCH_TABLE_PARTSUPP = 5,
    TPCH_TABLE_REGION   = 6,
    TPCH_TABLE_SUPPLIER = 7,
    _TPCH_TABLE_COUNT_  = 8
};



/* exported data structures */

/* in memory tables */
extern cdb_table_s tpch_tables[_TPCH_TABLE_COUNT_];



#endif
