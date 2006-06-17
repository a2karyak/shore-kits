// -*- mode:C++ c-basic-offset:4 -*-

/** @file    : test_tscan_stage.cpp
 *  @brief   : Unittest for the tscan stage
 *  @author  : Ippokratis Pandis
 *  @version : 0.1
 *  @history :
 8/6/2006 : Updated to work with the new class definitions
 25/5/2006: Initial version
*/ 

#include "engine/thread.h"
#include "engine/core/stage_container.h"
#include "engine/stages/tscan.h"
#include "engine/dispatcher.h"
#include "trace.h"
#include "qpipe_panic.h"
#include "tests/common.h"

#include <unistd.h>
#include <sys/time.h>
#include <math.h>

using namespace qpipe;


// Q6 SPECIFIC UTILS
/* Declaration of some constants */

#define DATABASE_HOME	 "."
#define CONFIG_DATA_DIR "./database"
#define TMP_DIR "./temp"

#define TABLE_LINEITEM_NAME "LINEITEM"
#define TABLE_LINEITEM_ID   "TBL_LITEM"

/* Set Bufferpool equal to 450 MB -- Maximum is 4GB in 32-bit platforms */
size_t TPCH_BUFFER_POOL_SIZE_GB = 0; /* 0 GB */
size_t TPCH_BUFFER_POOL_SIZE_BYTES = 450 * 1024 * 1024; /* 450 MB */

// END OF: Q6 SPECIFIC UTILS


// Q6 TSCAN FILTER 

/* Specific filter for this client */

class q6_tscan_filter_t : public tuple_filter_t {

private:
    /* Our predicate is represented by these fields. The predicate stays
       constant throughout the execution of the query. */

    time_t t1;
    time_t t2;

    struct timeval tv;
    uint mn;

    /* Random predicates */
    /* TPC-H specification 2.3.0 */

    /* DATE is 1st Jan. of year [1993 .. 1997] */
    int DATE;

    /* DISCOUNT is random [0.02 .. 0.09] */
    double DISCOUNT;

    /* QUANTITY is randon [24 .. 25] */
    double QUANTITY;

public:

    /* Initialize the predicates */
    q6_tscan_filter_t() : tuple_filter_t(sizeof(tpch_lineitem_tuple)) {
	t1 = datestr_to_timet("1997-01-01");
	t2 = datestr_to_timet("1998-01-01");

	/* Calculate random predicates */
	gettimeofday(&tv, 0);
	mn = tv.tv_usec * getpid();
	DATE = 1993 + abs((int)(5*(float)(rand_r(&mn))/(float)(RAND_MAX+1)));

	gettimeofday(&tv, 0);
	mn = tv.tv_usec * getpid();
	DISCOUNT = 0.02 + (float)(fabs((float)(rand_r(&mn))/(float)(RAND_MAX+1)))/(float)14.2857142857143;

	gettimeofday(&tv, 0);
	mn = tv.tv_usec * getpid();
	QUANTITY = 24 + fabs((float)(rand_r(&mn))/(float)(RAND_MAX+1));

	TRACE(TRACE_DEBUG, "Q6 - DISCOUNT = %.2f. QUANTITY = %.2f\n", DISCOUNT, QUANTITY);
    }


    /* Predication */
    virtual bool select(const tuple_t &input) {

	/* Predicate:
	   L_SHIPDATE >= DATE AND
	   L_SHIPDATE < DATE + 1 YEAR AND
	   L_DISCOUNT BETWEEN DISCOUNT - 0.01 AND DISCOUNT + 0.01 AND
	   L_QUANTITY < QUANTITY
	*/

	tpch_lineitem_tuple *tuple = (tpch_lineitem_tuple*)input.data;

	/*
	  printf("%d - %d\t", (int)tuple->L_SHIPDATE, (int)t1);
	  printf("%d - %d\t", (int)tuple->L_SHIPDATE, (int)t2);
	  printf("%.2f - %.2f\t", tuple->L_DISCOUNT, DISCOUNT - 0.01);
	  printf("%.2f - %.2f\t", tuple->L_DISCOUNT, DISCOUNT + 0.01);
	  printf("%.2f - %.2f\n", tuple->L_QUANTITY, QUANTITY);
	*/

	if  ( ( tuple->L_SHIPDATE >= t1 ) &&
	    ( tuple->L_SHIPDATE < t2 ) &&
	    ( tuple->L_DISCOUNT >= (DISCOUNT - 0.01)) &&
	    ( tuple->L_DISCOUNT <= (DISCOUNT + 0.01)) &&
	    ( tuple->L_QUANTITY < (QUANTITY)) )
	    {
		printf("+");
		return (true);
	    }
	else {
	    printf(".");
	    return (false);
	}

	/*
	// TODO: Should ask the Catalog
	double* d_discount = (double*)(input.data + 4*sizeof(int)+3*sizeof(double));

	// all the lineitems with discount > 0.04 pass the filter
        if (*d_discount > 0.04) {
	    //	    TRACE(TRACE_DEBUG, "Passed Filtering:\t %.2f\n", *d_discount);
	    return (true);
	}
	*/
    }
    
    /* Projection */
    virtual void project(tuple_t &dest, const tuple_t &src) {

	/* Should project L_EXTENDEDPRICE & L_DISCOUNT */

	// Calculate L_EXTENDEDPRICE
	tpch_lineitem_tuple *at = (tpch_lineitem_tuple*)(src.data);

	memcpy(dest.data,& at->L_EXTENDEDPRICE, sizeof(double));
	memcpy(dest.data + sizeof(double), &at->L_DISCOUNT, sizeof(double));
    }
};


// END OF: Q6 TSCAN FILTER





/** @fn    : main
 *  @brief : Calls a table scan and outputs a specific projection
 */

int main() {

    thread_init();

    stage_container_t* tscan_sc = 
	new stage_container_t("TSCAN_CONTAINER", new stage_factory<tscan_stage_t>);

    dispatcher_t::register_stage_container(tscan_packet_t::PACKET_TYPE, tscan_sc);

    tester_thread_t* tscan_thread = new tester_thread_t(drive_stage, tscan_sc, "TSCAN THREAD");

    if ( thread_create(NULL, tscan_thread) ) {
	TRACE(TRACE_ALWAYS, "thread_create failed\n");
	QPIPE_PANIC();
    }

    // OPENS THE LINEITEM TABLE
    Db* tpch_lineitem = NULL;
    DbEnv* dbenv = NULL;

    try {
	// the tscan packet does not have an input buffer but a table

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
    }
    catch ( DbException &e) {
	TRACE(TRACE_ALWAYS, "DbException: %s\n", e.what());
    }
    catch ( std::exception &en) {
	TRACE(TRACE_ALWAYS, "std::exception\n");
    }

    // TSCAN PACKET
    // the output consists of 2 doubles
    tuple_buffer_t* tscan_out_buffer = new tuple_buffer_t(2*sizeof(double));
    tuple_filter_t* tscan_filter = new q6_tscan_filter_t();


    char* tscan_packet_id;
    int tscan_packet_id_ret =
        asprintf( &tscan_packet_id, "TSCAN_PACKET_1" );
    assert( tscan_packet_id_ret != -1 );

    tscan_packet_t* q6_tscan_packet =
        new tscan_packet_t(tscan_packet_id, tscan_out_buffer, tscan_filter, tpch_lineitem);

    // Dispatch packet
    dispatcher_t::dispatch_packet(q6_tscan_packet);
    
    tuple_t output;
    double* d = NULL;
    while(!tscan_out_buffer->get_tuple(output)) {
	d = (double*)output.data;
	TRACE(TRACE_ALWAYS, "Read ID: EXT=%lf - DISC=%lf\n", d[0], d[1]);
    }

    try {    
	// closes file and environment
	TRACE(TRACE_DEBUG, "Closing Storage Manager...\n");

	// Close tables
	tpch_lineitem->close(0);
    
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
