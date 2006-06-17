// -*- mode:C++ c-basic-offset:4 -*-

/** @file    : test_tpch_spec.cpp
 *  @brief   : Header follows TPC-H spec
 *  @version : 0.1
 *  @history :
 6/16/2006: Initial version
*/ 

#include "db_cxx.h"

#include <unistd.h>
#include <sys/time.h>
#include <math.h>

#include "tests/common/tpch_struct.h"


using namespace qpipe;


#define DATABASE_HOME	 "."
#define CONFIG_DATA_DIR "./database"
#define TMP_DIR "./temp"

#define TABLE_LINEITEM_NAME "LINEITEM"
#define TABLE_LINEITEM_ID   "TBL_LITEM"

#define TABLE_ORDERS_NAME "ORDERS"
#define TABLE_ORDERS_ID   "TBL_ORDERS"

/* Set Bufferpool equal to 450 MB -- Maximum is 4GB in 32-bit platforms */
size_t TPCH_BUFFER_POOL_SIZE_GB = 0; /* 0 GB */
size_t TPCH_BUFFER_POOL_SIZE_BYTES = 450 * 1024 * 1024; /* 450 MB */



/** @fn    : void open_tables()
 *  @brief : Opens the tpch tables
 */

int open_tables(DbEnv* dbenv, Db* &tpch_lineitem, Db* &tpch_orders) {

    try {
	// Create an environment object and initialize it for error
	// reporting.
	dbenv = new DbEnv(0);
	dbenv->set_errpfx("qpipe");
	
	// We want to specify the shared memory buffer pool cachesize,
	// but everything else is the default.
  
	//dbenv->set_cachesize(0, TPCH_BUFFER_POOL_SIZE, 0);
	if (dbenv->set_cachesize(TPCH_BUFFER_POOL_SIZE_GB, TPCH_BUFFER_POOL_SIZE_BYTES, 0)) {
	    TRACE(TRACE_ALWAYS, "*** Error while trying to set BUFFERPOOL size ***\n");
	}
	else {
	    TRACE(TRACE_ALWAYS, "*** BUFFERPOOL SIZE SET: %d GB + %d B ***\n", TPCH_BUFFER_POOL_SIZE_GB, TPCH_BUFFER_POOL_SIZE_BYTES);
	}

	// Databases are in a subdirectory.
	dbenv->set_data_dir(CONFIG_DATA_DIR);
  
	// set temporary directory
	dbenv->set_tmp_dir(TMP_DIR);

	// Open the environment with no transactional support.
	dbenv->open(DATABASE_HOME, DB_CREATE | DB_PRIVATE | DB_THREAD | DB_INIT_MPOOL, 0);

        tpch_lineitem = new Db(dbenv, 0);

        tpch_lineitem->set_bt_compare(tpch_lineitem_bt_compare_fcn);
        tpch_lineitem->open(NULL, TABLE_LINEITEM_NAME, NULL, DB_BTREE,
                            DB_RDONLY | DB_THREAD, 0644);

        TRACE(TRACE_ALWAYS, "Lineitem table opened...\n");

	tpch_orders = new Db(dbenv, 0);

	tpch_orders->set_bt_compare(tpch_orders_bt_compare_fcn);
	tpch_orders->open(NULL, TABLE_LINEITEM_NAME, NULL, DB_BTREE,
			  DB_RDONLY | DB_THREAD, 0644);

	TRACE(TRACE_ALWAYS, "Orders table opened...\n");
    }
    catch ( DbException &e) {
	TRACE(TRACE_ALWAYS, "DbException: %s\n", e.what());
	return (1);
    }
    catch ( std::exception &en) {
	TRACE(TRACE_ALWAYS, "std::exception\n");
	return (2);
    }

    return (0);
}    



/** @fn    : int close_tables(
 *  @brief : Closes the tpch tables
 */

int close_tables(DbEnv* dbenv, Db* tpch_lineitem, Db* tpch_orders) {

    try {    
	// closes file and environment
	TRACE(TRACE_DEBUG, "Closing Storage Manager...\n");

	// Close tables
	tpch_lineitem->close(0);
	tpch_orders->close(0);

	// Close the handle.
	dbenv->close(0);
    }
    catch ( DbException &e) {
	TRACE(TRACE_ALWAYS, "DbException: %s\n", e.what());
    }
    catch ( std::exception &en) {
	TRACE(TRACE_ALWAYS, "std::exception\n");
    }

    return 0;
}
