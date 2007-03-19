/* -*- mode:C++; c-basic-offset:4 -*- */

#include "util.h"
#include "workload/tpch/tpch_db.h"
#include "workload/tpch/tpch_filenames.h"

using namespace qpipe;



/* helper functions */

static void open_db_table(page_list* table, const char* table_name);
static void close_db_table(page_list* table, const char* table_name);



/* definitions of exported functions */

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
    for (int i = 0; i < _TPCH_TABLE_COUNT_; i++)
        open_db_table(tpch_tables[i].db, tpch_tables[i].cdb_filename);

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
    for (int i = 0; i < _TPCH_TABLE_COUNT_; i++)
        close_db_table(tpch_tables[i].db, tpch_tables[i].cdb_filename);

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


    /* TODO Move this into a configuration variable */
    const char* DATABASE_DIRECTORY = "database/";


    /* check directory */
    const char* dir = DATABASE_DIRECTORY;
    if (fileops_check_file_directory(dir))
        THROW2(FileException, "Database directory %s does not exist\n", dir);

    /* check table file */
    c_str table_path("%s/%s", DATABASE_DIRECTORY, table_name);
    if (fileops_check_file_exists(table_path.data()))
        THROW2(FileException, "Table file %s does not exist\n", table_path.data());
    if (fileops_check_file_readable(table_path.data()))
        THROW2(FileException, "Table file %s not readable\n", table_path.data());

    /* open table file */
    FILE* f = fopen(table_path.data(), "r");
    if(f == NULL) 
        THROW2(FileException, "Unable to open %s\n", table_path.data());
    
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
