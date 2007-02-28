/* -*- mode:C++; c-basic-offset:4 -*- */

#include "workload/tpch/tpch_db_load.h"
#include "workload/tpch/tpch_env.h"
#include "workload/tpch/tpch_struct.h"
#include "workload/tpch/tpch_type_convert.h"

#include "core.h"

#include <cstdlib>
#include <cmath>
#include <sys/types.h>
#include <sys/stat.h>

using namespace qpipe;



void progress_reset();
void progress_update();
void progress_done();

#define MAX_LINE_LENGTH 1024
#define MAX_FILENAME_SIZE 1024



/* definitions of exported functions */


// dest is guaranteed to be zeroed out and have "enough" alignment for casting purposes


void parse_table(char const* fin_name, char const* fout_name, tuple_parser_t parse, size_t tuple_size)
{
    char linebuffer[MAX_LINE_LENGTH];
    

    c_str fin_path("tbl/%s", fin_name);
    c_str fout_path("database/%s", fout_name);
    

    // force 8-byte alignment
    array_guard_t<char> tuple_data = new char[tuple_size+7];
    union {
	size_t i;
	char* p;
    } u;
    u.p = tuple_data;
    u.i = (u.i+7) & -8;
    tuple_t tuple(u.p, tuple_size);

    TRACE(TRACE_DEBUG, "Populating %s...\n", fout_name);
    progress_reset();

    FILE* fin = fopen(fin_path.data(), "r");
    if (fin == NULL) {
        TRACE(TRACE_ALWAYS, "fopen() failed on %s\n", fin_path.data());
        THROW1(BdbException, "fopen() failed");
    }
    FILE* fout = fopen(fout_path.data(), "w");
    if (fout == NULL) {
        TRACE(TRACE_ALWAYS, "fopen() failed on %s\n", fout_path.data());
        THROW1(BdbException, "fopen() failed");
    }

    qpipe::page *p = qpipe::page::alloc(tuple_size);
    while (fgets(linebuffer, MAX_LINE_LENGTH, fin)) {
        // flush to file?
        if(p->full()) {
            p->fwrite_full_page(fout);
            p->clear();
        }
        
        memset(tuple_data, 0, tuple_size);
        parse(tuple_data, linebuffer);
        p->append_tuple(tuple);
        progress_update();
    }

    if(!p->empty())
        p->fwrite_full_page(fout);
    p->free();
    
    if ( fclose(fout) ) {
        TRACE(TRACE_ALWAYS, "fclose() failed on %s\n", fout_path.data());
        THROW1(BdbException, "fclose() failed");
    }
    if ( fclose(fin) ) {
        TRACE(TRACE_ALWAYS, "fclose() failed on %s\n", fin_path.data());
        THROW1(BdbException, "fclose() failed");
    }
    
    progress_done();
}

/**
 *  @brief Load TPC-H tables.
 *
 *  @return void
 *
 *  @throw BdbException on error.
 */
void db_load(const char*) {

    // load tables
    for (int i = 0; i < _TPCH_TABLE_COUNT_; i++)
        parse_table(tpch_tables[i].tbl_filename,
                    tpch_tables[i].cdb_filename,
                    tpch_tables[i].tuple_parser,
                    tpch_tables[i].tuple_size);
}



/* definitions of internal helper functions */

// progress reporting

#define PROGRESS_INTERVAL 100000

static unsigned long progress = 0;

void progress_reset() {
    progress = 0;
}

void progress_update() {
    if ( (progress++ % PROGRESS_INTERVAL) == 0 ) {
        printf(".");
        fflush(stdout);
        progress = 1; // prevent overflow
    }
}

void progress_done() {
    printf("done\n");
    fflush(stdout);
}
