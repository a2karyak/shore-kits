// -*- mode:C++; c-basic-offset:4 -*-

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
#include "engine/stages/partial_aggregate.h"

#include "tests/common.h"
#include "workload/common.h"
#include "workload/tpch/tpch_db.h"

#include <unistd.h>
#include <sys/time.h>
#include <math.h>

using namespace qpipe;

extern uint32_t trace_current_setting;


// Q1 SPECIFIC UTILS
/* Declaration of some constants */



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
#if 0
	gettimeofday(&tv, 0);
	mv = tv.tv_usec * getpid();

 	DELTA = 60 + abs((int)(60*(float)(rand_r(&mv))/(float)(RAND_MAX+1)));
#else
        DELTA = 90;
#endif

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
    virtual q1_tscan_filter_t* clone() {
        return new q1_tscan_filter_t(*this);
    }
};


// END OF: Q1 TSCAN FILTER


// Q1 SORT

// "order by L_RETURNFLAG, L_LINESTATUS"
struct q1_key_extract_t : public key_extractor_t {
    q1_key_extract_t()
        : key_extractor_t(sizeof(char)*2, 0)
    {
        assert(sizeof(char) == 1);
    }
    virtual int extract_hint(const char* tuple_data) {
        // store the return flag and line status in the 
        projected_lineitem_tuple *item;
        item = (projected_lineitem_tuple *) tuple_data;

        int result = (item->L_RETURNFLAG << 8)
            + item->L_LINESTATUS;

        return result;
    }
    virtual q1_key_extract_t* clone() {
        return new q1_key_extract_t(*this);
    }
};

// END OF: Q1 SORT


// Q6 AGG

class q1_count_aggregate_t : public tuple_aggregate_t {

private:
    q1_key_extract_t _extractor;

public:
  
    q1_count_aggregate_t()
        : tuple_aggregate_t(sizeof(aggregate_tuple))
    {
    }
    virtual key_extractor_t* key_extractor() { return &_extractor; }

    virtual void aggregate(char* agg_data, const tuple_t &s) {
        projected_lineitem_tuple *src;
        src = (projected_lineitem_tuple *)s.data;
        aggregate_tuple* tuple = (aggregate_tuple*) agg_data;

        // cache resused values for convenience
        double L_EXTENDEDPRICE = src->L_EXTENDEDPRICE;
        double L_DISCOUNT = src->L_DISCOUNT;
        double L_QUANTITY = src->L_QUANTITY;
        double L_DISC_PRICE = L_EXTENDEDPRICE * (1 - L_DISCOUNT);

        // update count
        tuple->L_COUNT_ORDER++;
        
        // update sums
        tuple->L_SUM_QTY += L_QUANTITY;
        tuple->L_SUM_BASE_PRICE += L_EXTENDEDPRICE;
        tuple->L_SUM_DISC_PRICE += L_DISC_PRICE;
        tuple->L_SUM_CHARGE += L_DISC_PRICE * (1 + src->L_TAX);
        tuple->L_AVG_QTY += L_QUANTITY;
        tuple->L_AVG_PRICE += L_EXTENDEDPRICE;
        tuple->L_AVG_DISC += L_DISCOUNT;

	if (((int) tuple->L_COUNT_ORDER) % 100 == 0) {
	    TRACE(TRACE_DEBUG, "%lf\n", tuple->L_COUNT_ORDER);
	    fflush(stdout);
	}
    }

    
    virtual void finish(tuple_t &d, const char* agg_data) {
            aggregate_tuple *dest;
            dest = (aggregate_tuple *)d.data;
            aggregate_tuple* tuple = (aggregate_tuple*) agg_data;

            *dest = *tuple;
            // compute averages
            dest->L_AVG_QTY /= dest->L_COUNT_ORDER;
            dest->L_AVG_PRICE /= dest->L_COUNT_ORDER;
            dest->L_AVG_DISC /= dest->L_COUNT_ORDER;
    }

    virtual q1_count_aggregate_t* clone() {
        return new q1_count_aggregate_t(*this);
    }
};

// END OF: Q6 AGG



/** @fn    : main
 *  @brief : TPC-H Q6
 */

int main() {

    thread_init();

    if ( !db_open() ) {
        TRACE(TRACE_ALWAYS, "db_open() failed\n");
        QPIPE_PANIC();
    }        
    
    
    
    trace_current_setting = TRACE_ALWAYS;
    
    register_stage<tscan_stage_t>();
    register_stage<partial_aggregate_stage_t>();

    for(int i=0; i < 10; i++) {
        stopwatch_t timer;
        
        // TSCAN PACKET
	tuple_buffer_t* tscan_out_buffer = new tuple_buffer_t(sizeof(projected_lineitem_tuple));

        char* tscan_packet_id;
        int tscan_packet_id_ret = asprintf(&tscan_packet_id, "Q1_TSCAN_PACKET");
        assert( tscan_packet_id_ret != -1 );
        tscan_packet_t* q1_tscan_packet = new tscan_packet_t(tscan_packet_id,
                                                             tscan_out_buffer,
                                                             new q1_tscan_filter_t(),
                                                             tpch_lineitem);

        // AGG PACKET CREATION
        tuple_buffer_t* agg_output_buffer = new tuple_buffer_t(sizeof(aggregate_tuple));

        char* agg_packet_id;
        int agg_packet_id_ret = asprintf(&agg_packet_id, "Q1_AGG_PACKET");
        assert( agg_packet_id_ret != -1 );
        packet_t* q1_agg_packet;
        q1_agg_packet = new partial_aggregate_packet_t(agg_packet_id,
                                                       agg_output_buffer,
                                                       new trivial_filter_t(agg_output_buffer->tuple_size),
                                                                   q1_tscan_packet,
                                                       new q1_count_aggregate_t(),
                                                       new q1_key_extract_t(),
                                                       new int_key_compare_t());


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
        TRACE(TRACE_ALWAYS, "Query executed in %.3lf s\n", timer.time());
    }



    if ( !db_close() )
        TRACE(TRACE_ALWAYS, "db_close() failed\n");
    return 0;
}
