// -*- mode:C++ c-basic-offset:4 -*-

/** @file    : test_q4.cpp
 *  @brief   : Unittest for Q4 (joins LINEITEM/ORDERS
 *  @version : 0.1
 *  @history :
 6/16/2006: Initial version
*/ 

#include "engine/thread.h"
#include "engine/core/stage_container.h"
#include "engine/stages/tscan.h"
#include "engine/stages/aggregate.h"
#include "trace.h"
#include "qpipe_panic.h"
#include "engine/dispatcher.h"
#include "tests/tester_thread.h"
#include "engine/util/stopwatch.h"

#include "test_tpch_spec.h"

using namespace qpipe;


// Q4 LINEITEM TSCAN FILTER 

class q4_lineitem_tscan_filter_t : public tuple_filter_t {
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
    q4_lineitem_tscan_filter_t() : tuple_filter_t(sizeof(tpch_lineitem_tuple)) {
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

	tpch_lineitem_tuple *tuple = (tpch_lineitem_tuple*)input.data;

	if  ( ( tuple->L_SHIPDATE >= t1 ) &&
	      ( tuple->L_SHIPDATE < t2 ) &&
	      //	      ( tuple->L_DISCOUNT >= (DISCOUNT - 0.01)) &&
	      ( tuple->L_DISCOUNT >= (DISCOUNT)) &&  // redure cardinality
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
    }
    
    /* Projection */
    virtual void project(tuple_t &dest, const tuple_t &src) {

	/* Should project  L_ORDERKEY */
	tpch_lineitem_tuple *at = (tpch_lineitem_tuple*)(src.data);
	memcpy(dest.data, &at->L_ORDERKEY, sizeof(int));
    }
};

// END OF: Q4 LINEITEM TSCAN FILTER


// Q4 ORDERS TSCAN FILTER

class q4_orders_tscan_filter_t : public tuple_filter_t {
private:
    /* Our predicate is represented by these fields. The predicate stays                                                                                   
       constant throughout the execution of the query. */

    time_t t1;
    time_t t2;

    struct timeval tv;
    uint mn;

    /* Random predicates */
    /* TPC-H specification 2.3.0 */

public:
    /* Initialize the predicates */
    q4_orders_tscan_filter_t() : tuple_filter_t(sizeof(tpch_orders_tuple)) {
        t1 = datestr_to_timet("1997-03-01");
        t2 = datestr_to_timet("1998-06-01");
    }


    /* Predication */
    virtual bool select(const tuple_t &input) {

        tpch_orders_tuple *tuple = (tpch_orders_tuple*)input.data;

        if  (( tuple->O_ORDERDATE >= t1 ) &&
	     ( tuple->O_ORDERDATE < t2  ))
            {
                printf("+");
                return (true);
            }
        else {
            printf(".");
            return (false);
        }
    }

    /* Projection */
    virtual void project(tuple_t &dest, const tuple_t &src) {

        /* Should project  L_ORDERKEY */
        tpch_orders_tuple *at = (tpch_orders_tuple*)(src.data);
        memcpy(dest.data, &at->O_ORDERKEY, sizeof(int));
        memcpy(dest.data, &at->O_ORDERPRIORITY, sizeof(tpch_o_orderpriority));
    }
}

// EN OF: Q4 ORDERS TSCAN FILTER



// Q4 JOIN


// END OF: Q4 JOIN



// Q4 AGG

class count_aggregate_t : public tuple_aggregate_t {

private:
    int count;
    int ORDER_PRIORITY;
    
public:  
    count_aggregate_t() {
	count = 0;
	ORDER_PRIORITY = 0;
    }
  
    bool aggregate(tuple_t &, const tuple_t & src) {

	// update COUNT and ORDER_PRIORITY
	count++;
	int * i_op = (int*)src.data;
	ORDER_PRIORITY = *i_op;
    
	if(count % 10 == 0) {
	    TRACE(TRACE_DEBUG, "%d - %d\n", count, ORDER_PRIORITY);
	    fflush(stdout);
	}

	return false;
    }

    bool eof(tuple_t &dest) {
        int *output = (int*)dest.data;
        output[0] = ORDER_PRIORITY;
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
    for (int i = 0; i < 2; i++) {
	tester_thread_t* tscan_thread = new tester_thread_t(drive_stage, tscan_sc, "TSCAN THREAD");
	if ( thread_create(NULL, tscan_thread) ) {
	    TRACE(TRACE_ALWAYS, "thread_create failed\n");
	    QPIPE_PANIC();
	}
    }

    // creates a AGG stage
    stage_container_t* agg_sc = 
	new stage_container_t("AGGREGATE_CONTAINER", new stage_factory<aggregate_stage_t>);
    dispatcher_t::register_stage_container(aggregate_packet_t::PACKET_TYPE, agg_sc);
    for (int i = 0; i < 2; i++) {
	tester_thread_t* agg_thread = new tester_thread_t(drive_stage, agg_sc, "AGG THREAD");
	if ( thread_create(NULL, agg_thread) ) {
	    TRACE(TRACE_ALWAYS, "thread_create failed\n");
	    QPIPE_PANIC();
	}
    }

    // OPENS THE LINEITEM TABLE
    Db* tpch_lineitem = NULL;

    // OPENS THE ORDERS TABLES
    Db* tpch_orders= NULL;

    DbEnv* dbenv = NULL;

    open_tables(dbenv, tpch_lineitem, tpch_orders);

    for(int i=0; i < 10; i++) {
        stopwatch_t timer;
        
        // LINEITEM TSCAN PACKET
        // the output consists of 1 int
        tuple_buffer_t* lineitem_tscan_out_buffer = new tuple_buffer_t(sizeof(int));
        tuple_filter_t* lineitem_tscan_filter = new q4_lineitem_tscan_filter_t();

        char* lineitem_tscan_packet_id;
        int lineitem_tscan_packet_id_ret = asprintf(&lineitem_tscan_packet_id, "Q4_LINEITEM_TSCAN_PACKET");
        assert( lineitem_tscan_packet_id_ret != -1 );
        tscan_packet_t *q4_lineitem_tscan_packet = new tscan_packet_t(lineitem_tscan_packet_id,
								      lineitem_tscan_out_buffer,
								      lineitem_tscan_filter,
								      tpch_lineitem);


        // ORDERS TSCAN PACKET                                                                                                                         
        // the output consists of 1 int
	tuple_buffer_t* orders_tscan_out_buffer = new tuple_buffer_t(sizeof(int) + sizeof(tpch_o_orderpriority));
        tuple_filter_t* orders_tscan_filter = new q4_orders_tscan_filter_t();

        char* orders_tscan_packet_id;
        int orders_tscan_packet_id_ret = asprintf(&orders_tscan_packet_id, "Q4_ORDERS_TSCAN_PACKET");
        assert( orders_tscan_packet_id_ret != -1 );
        tscan_packet_t *q4_orders_tscan_packet = new tscan_packet_t(orders_tscan_packet_id,
								    orders_tscan_out_buffer,
								    orders_tscan_filter,
								    tpch_orders);

	// JOIN PACKET CREATION
	join_packet_t q4_join_packet;

        // AGG PACKET CREATION
        // the output consists of 2 int
        tuple_buffer_t* agg_output_buffer = new tuple_buffer_t(2*sizeof(int));
        tuple_filter_t* agg_filter = new tuple_filter_t(agg_output_buffer->tuple_size);
        count_aggregate_t*  q4_aggregator = new count_aggregate_t();
    
        char* agg_packet_id;
        int agg_packet_id_ret = asprintf(&agg_packet_id, "Q6_AGGREGATE_PACKET");
        assert( agg_packet_id_ret != -1 );
        aggregate_packet_t* q4_agg_packet = new aggregate_packet_t(agg_packet_id,
                                                                   agg_output_buffer,
                                                                   agg_filter,
                                                                   q4_aggregator,
                                                                   q4_join_packet);

        // Dispatch packet
        dispatcher_t::dispatch_packet(q4_agg_packet);
    
        tuple_t output;
        int * r = NULL;
        while(!agg_output_buffer->get_tuple(output)) {
            r = (int*)output.data;
            TRACE(TRACE_ALWAYS, "*** Q4 Count: %lf. Priority: %lf.  ***\n", r[0], r[1]);
        }
        
        printf("Query executed in %lf ms\n", timer.time_ms());
    }

    close_tables(dbenv, tpch_lineitem, tpch_orders);
    return 0;
}
