// -*- mode:C++ c-basic-offset:4 -*-

/** @file    : test_q6.cpp
 *  @brief   : Unittest for Q1
 *  @version : 0.1
 *  @history :
 6/9/2006: Initial version
*/ 

#include "trace.h"
#include "qpipe_panic.h"
#include "engine/thread.h"
#include "engine/core/stage_container.h"
#include "engine/dispatcher.h"
#include "engine/util/time_util.h"
#include "engine/util/stopwatch.h"
#include "engine/stages/tscan.h"
#include "engine/stages/aggregate.h"
#include "engine/stages/sort.h"
#include "engine/stages/fscan.h"
#include "engine/stages/fdump.h"
#include "engine/stages/merge.h"

#include "tests/common.h"

#include <unistd.h>
#include <sys/time.h>
#include <math.h>

using namespace qpipe;

extern uint32_t trace_current_setting;


// Q1 SPECIFIC UTILS
/* Declaration of some constants */

#define DATABASE_HOME	 "."
#define CONFIG_DATA_DIR "./database"
#define TMP_DIR "./temp"


#define TABLE_LINEITEM_NAME "LINEITEM"
#define TABLE_LINEITEM_ID   "TBL_LITEM"

/* Set Bufferpool equal to 450 MB -- Maximum is 4GB in 32-bit platforms */
size_t TPCH_BUFFER_POOL_SIZE_GB = 0; /* 0 GB */
size_t TPCH_BUFFER_POOL_SIZE_BYTES = 450 * 1024 * 1024; /* 450 MB */



// the tuples after tablescan projection
struct projected_lineitem_tuple {
    char L_RETURNFLAG;
    char L_LINESTATUS;
    double L_QUANTITY;
    double L_EXTENDEDPRICE;
    double L_DISCOUNT;
    double L_TAX;
};

// the final aggregated tuples
struct aggregate_tuple {
    char L_RETURNFLAG;
    char L_LINESTATUS;
    double L_SUM_QTY;
    double L_SUM_BASE_PRICE;
    double L_SUM_DISC_PRICE;
    double L_SUM_CHARGE;
    double L_AVG_QTY;
    double L_AVG_PRICE;
    double L_AVG_DISC;
    double L_COUNT_ORDER;
};


// END OF: Q1 SPECIFIC UTILS


// Q1 TSCAN FILTER 

/* Specific filter for this client */

class q1_tscan_filter_t : public tuple_filter_t {

private:
  time_t t;

  struct timeval tv;
  uint mv;

  /* Random Predicates */
  /* TPC-H Specification 2.3.0 */

  /* DELTA random within [60 .. 120] */
  int DELTA;

public:

    /* Initialize the predicates */
    q1_tscan_filter_t() : tuple_filter_t(sizeof(tpch_lineitem_tuple)) {
	t = datestr_to_timet("1998-12-01");

	/* Calculate random predicates */
	gettimeofday(&tv, 0);
	mv = tv.tv_usec * getpid();

	DELTA = 60 + abs((int)(60*(float)(rand_r(&mv))/(float)(RAND_MAX+1)));

	// Predicate: 1998-12-01 - DELTA days
	t = time_add_day(t, -DELTA);
    }


    /* Predication */
    virtual bool select(const tuple_t &input) {

	/* Predicate:
	   L_SHIPDATE <= 1998-12-01 - DELTA DAYS
	*/

	tpch_lineitem_tuple *tuple = (tpch_lineitem_tuple*)input.data;

        // FIXME: reversed on purpose to reduce selectivity...
	if  ( tuple->L_SHIPDATE <= t ) {
            //TRACE(TRACE_ALWAYS, "+");
	    return (true);
	}
	else {
            //TRACE(TRACE_ALWAYS, ".");
	    return (false);
	}
    }
    
    /* Projection */
    virtual void project(tuple_t &d, const tuple_t &s) {

        projected_lineitem_tuple *dest;
        tpch_lineitem_tuple *src;
        dest = (projected_lineitem_tuple *) d.data;
        src = (tpch_lineitem_tuple *) s.data;

        dest->L_RETURNFLAG = src->L_RETURNFLAG;
        dest->L_LINESTATUS = src->L_LINESTATUS;
        dest->L_QUANTITY = src->L_QUANTITY;
        dest->L_EXTENDEDPRICE = src->L_EXTENDEDPRICE;
        dest->L_DISCOUNT = src->L_DISCOUNT;
        dest->L_TAX = src->L_TAX;
    }
};


// END OF: Q1 TSCAN FILTER


// Q1 SORT

// "order by L_RETURNFLAG, L_LINESTATUS"
struct q1_tuple_comparator_t : public tuple_comparator_t {
    virtual int make_key(const tuple_t &tuple) {
        // store the return flag and line status in the 
        projected_lineitem_tuple *item;
        item = (projected_lineitem_tuple *) tuple.data;
        int key = (item->L_RETURNFLAG << 8) | item->L_LINESTATUS;
        return key;
    }
};

// END OF: Q1 SORT


// Q6 AGG

class count_aggregate_t : public tuple_aggregate_t {

private:
    // STATS
    bool first;
    aggregate_tuple tuple;

public:
  
    count_aggregate_t() {
        first = true;
        tuple.L_RETURNFLAG = -1;
        tuple.L_LINESTATUS = -1;
    }
  
    bool aggregate(tuple_t &d, const tuple_t &s) {
        projected_lineitem_tuple *src;
        src = (projected_lineitem_tuple *)s.data;

        // break group?
        bool broken = false;
        bool valid = false;
        broken |= (tuple.L_RETURNFLAG != src->L_RETURNFLAG);
        broken |= (tuple.L_LINESTATUS != src->L_LINESTATUS);
        if(broken)
            valid = break_group(d, src->L_RETURNFLAG, src->L_LINESTATUS);
        //        else
        //            printf(".");

        // cache resused values for convenience
        double L_EXTENDEDPRICE = src->L_EXTENDEDPRICE;
        double L_DISCOUNT = src->L_DISCOUNT;
        double L_QUANTITY = src->L_QUANTITY;
        double L_DISC_PRICE = L_EXTENDEDPRICE * (1 - L_DISCOUNT);

        // update count
        tuple.L_COUNT_ORDER++;
        
        // update sums
        tuple.L_SUM_QTY += L_QUANTITY;
        tuple.L_SUM_BASE_PRICE += L_EXTENDEDPRICE;
        tuple.L_SUM_DISC_PRICE += L_DISC_PRICE;
        tuple.L_SUM_CHARGE += L_DISC_PRICE * (1 + src->L_TAX);
        tuple.L_AVG_QTY += L_QUANTITY;
        tuple.L_AVG_PRICE += L_EXTENDEDPRICE;
        tuple.L_AVG_DISC += L_DISCOUNT;

	if (((int) tuple.L_COUNT_ORDER) % 100 == 0) {
	    TRACE(TRACE_DEBUG, "%lf\n", tuple.L_COUNT_ORDER);
	    fflush(stdout);
	}

        // output valid?
        return valid;
    }

    
    bool eof(tuple_t &d) {
        return break_group(d, -1, -1);
    }

    bool break_group(tuple_t &d, char L_RETURNFLAG, char L_LINESTATUS) {
        bool valid = !first;
        first = false;
        printf("+\n");
        // output?
        if(valid) {
            aggregate_tuple *dest;
            dest = (aggregate_tuple *)d.data;
                
            // compute averages
            tuple.L_AVG_QTY /= tuple.L_COUNT_ORDER;
            tuple.L_AVG_PRICE /= tuple.L_COUNT_ORDER;
            tuple.L_AVG_DISC /= tuple.L_COUNT_ORDER;

            // assign the value to the output
            *dest = tuple;
        }
            
        // reset the aggregate values
        memset(&tuple, 0, sizeof(aggregate_tuple));
        tuple.L_RETURNFLAG = L_RETURNFLAG;
        tuple.L_LINESTATUS = L_LINESTATUS;
        return valid;
    }
};

// END OF: Q6 AGG



/** @fn    : main
 *  @brief : TPC-H Q6
 */

int main() {

    thread_init();

    trace_current_setting = TRACE_ALWAYS;
    int THREAD_POOL_SIZE = 10;

    // creates a TSCAN stage
    stage_container_t* sc;

    sc = new stage_container_t("TSCAN_CONTAINER", new stage_factory<tscan_stage_t>);
    dispatcher_t::register_stage_container(tscan_packet_t::PACKET_TYPE, sc);
    tester_thread_t* tscan_thread = new tester_thread_t(drive_stage, sc, "TSCAN THREAD");
    if ( thread_create(NULL, tscan_thread) ) {
	TRACE(TRACE_ALWAYS, "thread_create failed\n");
	QPIPE_PANIC();
    }

    // creates a FDUMP stage
    sc = new stage_container_t("FDUMP_CONTAINER", new stage_factory<fdump_stage_t>);
    dispatcher_t::register_stage_container(fdump_packet_t::PACKET_TYPE, sc);
    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
	tester_thread_t* fdump_thread = new tester_thread_t(drive_stage, sc, "FDUMP THREAD");
	if ( thread_create( NULL, fdump_thread ) ) {
	    TRACE(TRACE_ALWAYS, "thread_create failed\n");
	    QPIPE_PANIC();
	}
    }

    // creates a FSCAN stage    
    sc = new stage_container_t("FSCAN_CONTAINER", new stage_factory<fscan_stage_t>);
    dispatcher_t::register_stage_container(fscan_packet_t::PACKET_TYPE, sc);
    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
	tester_thread_t* fscan_thread = new tester_thread_t(drive_stage, sc, "FSCAN THREAD");
	if ( thread_create( NULL, fscan_thread ) ) {
	    TRACE(TRACE_ALWAYS, "thread_create failed\n");
	    QPIPE_PANIC();
	}
    }

    // creates a MERGE stage    
    sc = new stage_container_t("MERGE_CONTAINER", new stage_factory<merge_stage_t>);
    dispatcher_t::register_stage_container(merge_packet_t::PACKET_TYPE, sc);
    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
	tester_thread_t* merge_thread = new tester_thread_t(drive_stage, sc, "MERGE THREAD");
	if ( thread_create( NULL, merge_thread ) ) {
	    TRACE(TRACE_ALWAYS, "thread_create failed\n");
	    QPIPE_PANIC();
	}
    }

    // creates a SORT stage
    sc = new stage_container_t("SORT_CONTAINER", new stage_factory<sort_stage_t>);
    dispatcher_t::register_stage_container(sort_packet_t::PACKET_TYPE, sc);
    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
	tester_thread_t* sort_thread = new tester_thread_t(drive_stage, sc, "SORT THREAD");
	if ( thread_create( NULL, sort_thread ) ) {
	    TRACE(TRACE_ALWAYS, "thread_create failed\n");
	    QPIPE_PANIC();
	}
    }

    // creates a AGG stage
    sc = new stage_container_t("AGG_CONTAINER", new stage_factory<aggregate_stage_t>);
    dispatcher_t::register_stage_container(aggregate_packet_t::PACKET_TYPE, sc);
    tester_thread_t* agg_thread = new tester_thread_t(drive_stage, sc, "AGG THREAD");
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
	tuple_buffer_t* tscan_out_buffer = new tuple_buffer_t(sizeof(projected_lineitem_tuple));

        char* tscan_packet_id;
        int tscan_packet_id_ret = asprintf(&tscan_packet_id, "Q1_TSCAN_PACKET");
        assert( tscan_packet_id_ret != -1 );
        tscan_packet_t* q1_tscan_packet = new tscan_packet_t(tscan_packet_id,
                                                             tscan_out_buffer,
                                                             new q1_tscan_filter_t(),
                                                             tpch_lineitem);
   
	// SORT PACKET
	// the output is always a single int
        tuple_buffer_t* sort_out_buffer = new tuple_buffer_t(sizeof(projected_lineitem_tuple));

        char* sort_packet_id;
        int sort_packet_id_ret = asprintf(&sort_packet_id, "Q1_SORT_PACKET");
        assert( sort_packet_id_ret != -1 );
	sort_packet_t* q1_sort_packet = new sort_packet_t(sort_packet_id,
                                                          sort_out_buffer,
                                                          new tuple_filter_t(tscan_out_buffer->tuple_size),
                                                          new q1_tuple_comparator_t(),
                                                          q1_tscan_packet);

        // AGG PACKET CREATION
        // the output consists of 2 int
        tuple_buffer_t* agg_output_buffer = new tuple_buffer_t(sizeof(aggregate_tuple));

        char* agg_packet_id;
        int agg_packet_id_ret = asprintf(&agg_packet_id, "Q1_AGG_PACKET");
        assert( agg_packet_id_ret != -1 );
        aggregate_packet_t* q1_agg_packet = new aggregate_packet_t(agg_packet_id,
                                                                   agg_output_buffer,
                                                                   new tuple_filter_t(agg_output_buffer->tuple_size),
                                                                   new count_aggregate_t(),
                                                                   q1_sort_packet);


        // Dispatch packet
        dispatcher_t::dispatch_packet(q1_agg_packet);
    
        tuple_t output;
	TRACE(TRACE_ALWAYS, "*** Q1 ANSWER ...\n");
	TRACE(TRACE_ALWAYS, "*** SUM_QTY\tSUM_BASE\tSUM_DISC...\n");
        while(!agg_output_buffer->get_tuple(output)) {
            aggregate_tuple *tuple;
            tuple = (aggregate_tuple *) output.data;
            TRACE(TRACE_ALWAYS, "*** %lf\t%lf\t%lf\n",
                  tuple->L_SUM_QTY, tuple->L_SUM_BASE_PRICE,
                  tuple->L_SUM_DISC_PRICE);
        }
        TRACE(TRACE_ALWAYS, "Query executed in %lf ms\n", timer.time_ms());
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
