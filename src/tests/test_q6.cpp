// -*- mode:C++ c-basic-offset:4 -*-

/** @file    : test_q6.cpp
 *  @brief   : Unittest for Q6
 *  @version : 0.1
 *  @history :
 6/8/2006 : Updated to work with the new class definitions
 5/25/2006: Initial version
*/ 

#include "thread.h"
#include "stage_container.h"
#include "stages/tscan.h"
#include "stages/aggregate.h"
#include "trace.h"
#include "qpipe_panic.h"
#include "dispatcher.h"
#include "tester_thread.h"
#include "util/stopwatch.h"

#include "db_cxx.h"

#include <unistd.h>
#include <sys/time.h>
#include <math.h>



using namespace qpipe;



extern uint32_t trace_current_setting;




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


/* tpch_l_shipmode.
   TODO: Unnecessary */
enum tpch_l_shipmode {
    REG_AIR,
    AIR,
    RAIL,
    TRUCK,
    MAIL,
    FOB,
    SHIP
};


/* Lineitem tuple.
   TODO: Catalog will provide this metadata info */
struct tpch_lineitem_tuple {
    int L_ORDERKEY;
    int L_PARTKEY;
    int L_SUPPKEY;
    int L_LINENUMBER;
    double L_QUANTITY;
    double L_EXTENDEDPRICE;
    double L_DISCOUNT;
    double L_TAX;
    char L_RETURNFLAG;
    char L_LINESTATUS;
    time_t L_SHIPDATE;
    time_t L_COMMITDATE;
    time_t L_RECEIPTDATE;
    char L_SHIPINSTRUCT[25];
    tpch_l_shipmode L_SHIPMODE;
    // char L_COMMENT[44];
};


int tpch_lineitem_bt_compare_fcn(Db*, const Dbt* k1, const Dbt* k2) {
    // LINEITEM key has 3 integers
    int u1[3];
    int u2[3];
    memcpy(u1, k1->get_data(), 3 * sizeof(int));
    memcpy(u2, k2->get_data(), 3 * sizeof(int));

    if ((u1[0] < u2[0]) || ((u1[0] == u2[0]) && (u1[1] < u2[1])) || ((u1[0] == u2[0]) && (u1[1] == u2[1]) && (u1[2] < u2[2])))
	return -1;
    else if ((u1[0] == u2[0]) && (u1[1] == u2[1]) && (u1[2] == u2[2]))
	return 0;
    else
	return 1;

}


/** @fn    : datestr_to_timet(char*)
 *  @brief : Converts a string to corresponding time_t
 */

time_t datestr_to_timet(char* str) {
    char buf[100];
    strcpy(buf, str);

    // str in yyyy-mm-dd format
    char* year = buf;
    char* month = buf + 5;
    // char* day = buf + 8;

    buf[4] = '\0';
    buf[7] = '\0';

    tm time_str;
    time_str.tm_year = atoi(year) - 1900;
    time_str.tm_mon = atoi(month) - 1;
    time_str.tm_mday = 4;
    time_str.tm_hour = 0;
    time_str.tm_min = 0;
    time_str.tm_sec = 1;
    time_str.tm_isdst = -1;

    return mktime(&time_str);
}


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
		//printf("+");
		return (true);
	    }
	else {
	    //printf(".");
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

	memcpy(dest.data, &at->L_EXTENDEDPRICE, sizeof(double));
	memcpy(dest.data + sizeof(double), &at->L_DISCOUNT, sizeof(double));
    }

};


// END OF: Q6 TSCAN FILTER



// Q6 AGG

class count_aggregate_t : public tuple_aggregate_t {

private:
    int count;
    double sum;
    
public:
  
    count_aggregate_t() {
	count = 0;
	sum = 0.0;
    }
  
    bool aggregate(tuple_t &, const tuple_t & src) {

	// update COUNT and SUM
	count++;
	double * d = (double*)src.data;
	sum += d[0] * d[1];
    
	if(count % 10 == 0) {
	    TRACE(TRACE_DEBUG, "%d - %lf\n", count, sum);
	    fflush(stdout);
	}

	return false;
    }

    bool eof(tuple_t &dest) {
        double *output = (double*)dest.data;
        output[0] = sum;
        output[1] = count;
	return true;
    }
};

// END OF: Q6 AGG



/** @fn    : void * drive_stage(void *)
 *  @brief : Simulates a worker thread on the specified stage.
 *  @param : arg A stage_t* to work on.
 */

void *drive_stage(void *arg) {

    stage_container_t* sc = (stage_container_t*)arg;
    sc->run();
    
    return NULL;
}


/** @fn    : main
 *  @brief : TPC-H Q6
 */

int main() {
    trace_current_setting = TRACE_ALWAYS;
    thread_init();

    // creates a TSCAN stage
    stage_container_t* tscan_sc = 
	new stage_container_t("TSCAN_CONTAINER", new stage_factory<tscan_stage_t>);

    dispatcher_t::register_stage_container(tscan_packet_t::PACKET_TYPE, tscan_sc);

    tester_thread_t* tscan_thread = new tester_thread_t(drive_stage, tscan_sc, "TSCAN THREAD");

    if ( thread_create(NULL, tscan_thread) ) {
	TRACE(TRACE_ALWAYS, "thread_create failed\n");
	QPIPE_PANIC();
    }

    // creates a AGG stage
    stage_container_t* agg_sc = 
	new stage_container_t("AGGREGATE_CONTAINER", new stage_factory<aggregate_stage_t>);

    dispatcher_t::register_stage_container(aggregate_packet_t::PACKET_TYPE, agg_sc);

    tester_thread_t* agg_thread = new tester_thread_t(drive_stage, agg_sc, "AGG THREAD");

    if ( thread_create(NULL, agg_thread) ) {
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
    

    for(int i=0; i < 10; i++) {
        stopwatch_t timer;
        
        // TSCAN PACKET
        // the output consists of 2 doubles
        tuple_buffer_t* tscan_out_buffer = new tuple_buffer_t(2*sizeof(double));
        tuple_filter_t* tscan_filter = new q6_tscan_filter_t();


        char* tscan_packet_id;
        int tscan_packet_id_ret = asprintf(&tscan_packet_id, "Q6_TSCAN_PACKET");
        assert( tscan_packet_id_ret != -1 );
        tscan_packet_t *q6_tscan_packet = new tscan_packet_t(tscan_packet_id,
                                                             tscan_out_buffer,
                                                             tscan_filter,
                                                             tpch_lineitem);

        // AGG PACKET CREATION
        // the output consists of 2 int
        tuple_buffer_t* agg_output_buffer = new tuple_buffer_t(2*sizeof(double));
        tuple_filter_t* agg_filter = new tuple_filter_t(agg_output_buffer->tuple_size);
        count_aggregate_t*  q6_aggregator = new count_aggregate_t();
    

        char* agg_packet_id;
        int agg_packet_id_ret = asprintf(&agg_packet_id, "Q6_AGGREGATE_PACKET");
        assert( agg_packet_id_ret != -1 );
        aggregate_packet_t* q6_agg_packet = new aggregate_packet_t(agg_packet_id,
                                                                   agg_output_buffer,
                                                                   agg_filter,
                                                                   q6_aggregator,
                                                                   q6_tscan_packet);


        // Dispatch packet
        dispatcher_t::dispatch_packet(q6_agg_packet);
    
        tuple_t output;
        double * r = NULL;
        while(!agg_output_buffer->get_tuple(output)) {
            r = (double*)output.data;
            TRACE(TRACE_ALWAYS, "*** Q6 Count: %lf. Sum: %lf.  ***\n", r[0], r[1]);
        }
        
        printf("Query executed in %lf ms\n", timer.time_ms());
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
