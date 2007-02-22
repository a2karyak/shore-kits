/* -*- mode:C++; c-basic-offset:4 -*- */

#include "util.h"
#include "workload/tpch/tpch_db.h"
#include "workload/tpch/tpch_filenames.h"




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
void db_open(uint32_t, uint32_t db_cache_size_gb, uint32_t db_cache_size_bytes) {

    assert(db_cache_size_gb + db_cache_size_bytes > 0);
    
    // open tables
    open_db_table(tpch_customer, CDB_FILENAME_CUSTOMER);
    open_db_table(tpch_lineitem, CDB_FILENAME_LINEITEM);
    open_db_table(tpch_nation,   CDB_FILENAME_NATION);
    open_db_table(tpch_orders,   CDB_FILENAME_ORDERS);
    open_db_table(tpch_part,     CDB_FILENAME_PART);
    open_db_table(tpch_partsupp, CDB_FILENAME_PARTSUPP);
    open_db_table(tpch_region,   CDB_FILENAME_REGION);
    open_db_table(tpch_supplier, CDB_FILENAME_SUPPLIER);

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
    close_db_table(tpch_customer, CDB_FILENAME_CUSTOMER);
    close_db_table(tpch_lineitem, CDB_FILENAME_LINEITEM);
    close_db_table(tpch_nation,   CDB_FILENAME_NATION);
    close_db_table(tpch_orders,   CDB_FILENAME_ORDERS);
    close_db_table(tpch_part,     CDB_FILENAME_PART);
    close_db_table(tpch_partsupp, CDB_FILENAME_PARTSUPP);
    close_db_table(tpch_region,   CDB_FILENAME_REGION);
    close_db_table(tpch_supplier, CDB_FILENAME_SUPPLIER);

    TRACE(TRACE_ALWAYS, "TPCH database closed\n");
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
        THROW2(FileException, "Unable to open %s\n", fname);
    
    qpipe::page* p = qpipe::page::alloc(1);
    while(p->fread_full_page(f)) {
        if (0 && !strcmp(table_name, "LINEITEM.cdb")) {
            TRACE(TRACE_ALWAYS, "Loaded %s with another page\n", table_name);
        }
        table->push_back(p);
        p = qpipe::page::alloc(1);
    }
    TRACE(TRACE_ALWAYS, "%s loaded with %d pages\n", table_name, table->size());

    
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

