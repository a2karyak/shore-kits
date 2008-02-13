/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file table_load.h
 *
 *  @brief Interface for the database load functions
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __TABLE_DB_LOAD_H
#define __TABLE_DB_LOAD_H

#include "util/namespace.h"
#include "workload/common/bdb_config.h"

#include "workload/tpch/tpch_db.h"
#include "workload/tpcc/tpcc_db.h"

using namespace tpch;
using namespace tpcc;


ENTER_NAMESPACE(workload);

typedef void (*tbl_loader) (Db*, FILE*);


// database load wrappers
void db_tpch_load(const char* tbl_path=BDB_TPCH_TBL_DIRECTORY);
void db_tpcc_load(const char* tbl_path=BDB_TPCC_TBL_DIRECTORY);


/** for verification purposes */
void db_tpch_read();
void db_tpcc_read();


/** experimental: multi-threaded loading */

void db_tpcc_load_mt(const char* tbl_path=BDB_TPCC_TBL_DIRECTORY);

class loader_thread_t : public thread_t {

private:

    c_str _name;
    tbl_loader _loader;
    Db* _db;
    FILE* _file;

public:

    loader_thread_t(const c_str &name,
                    tbl_loader a_loader,
                    Db* db,
                    FILE* file)
        : thread_t(name),
          _name(name),
          _loader(a_loader),
          _db(db),
          _file(file)
    {
        TRACE( TRACE_DEBUG, 
               "Loader thread (%s) constructed...\n", 
               _name.data());
    }


    virtual ~loader_thread_t() { 
        if (_name) 
            delete (_name);
    }

    virtual void run() {
        assert (_name);
        assert (_loader);
        TRACE( TRACE_DEBUG, 
               "Starting loading (%s)...\n", 
               _name.data());
        _loader(_db, _file);
    }    

}; // EOF loader_thread_t


/** EOF experimental multi-threaded loading */


EXIT_NAMESPACE(workload);

#endif
