/* -*- mode:C++; c-basic-offset:4 -*- */

#include "util.h"
#include "workload/tpch/tpch_db.h"
#include "workload/tpch/tpch_tables.h"




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
        THROW2(FileException, "Unable to open %s\n", fname);
    
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

