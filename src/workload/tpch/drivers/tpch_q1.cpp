/* -*- mode:C++; c-basic-offset:4 -*- */
#include "stages.h"
#include "scheduler.h"

#include "workload/tpch/drivers/tpch_q1.h"

#include "workload/tpch/tpch_struct.h"
#include "workload/tpch/tpch_type_convert.h"
#include "workload/tpch/tpch_env.h"
#include "workload/common/int_comparator.h"


ENTER_NAMESPACE(workload);


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



class q1_tscan_filter_t : public tuple_filter_t {

private:
    time_t t;

    /* Random Predicates */
    /* TPC-H Specification 2.3.0 */

    /* DELTA random within [60 .. 120] */
    int DELTA;

public:

    /* Initialize the predicates */
    q1_tscan_filter_t() : tuple_filter_t(sizeof(tpch_lineitem_tuple)) {

	t = datestr_to_timet("1998-12-01");

	/* Calculate random predicates */
#if 1
        // TPCH spec: "DELTA is randomly selected within [60. 120]"
        thread_t* self = thread_get_self();
        DELTA = 60 + self->rand(61);
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

	tpch_lineitem_tuple tuple;
        memcpy(&tuple, input.data, sizeof(tuple));

	if  ( tuple.L_SHIPDATE <= t ) {
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

        projected_lineitem_tuple dest;
        tpch_lineitem_tuple src;
        memcpy(&src, s.data, sizeof(src));

        dest.L_RETURNFLAG = src.L_RETURNFLAG;
        dest.L_LINESTATUS = src.L_LINESTATUS;
        dest.L_QUANTITY = src.L_QUANTITY;
        dest.L_EXTENDEDPRICE = src.L_EXTENDEDPRICE;
        dest.L_DISCOUNT = src.L_DISCOUNT;
        dest.L_TAX = src.L_TAX;
        memcpy(d.data, &dest, sizeof(dest));
    }
    virtual q1_tscan_filter_t* clone() const {
        return new q1_tscan_filter_t(*this);
    }

    virtual c_str to_string() const {
        return c_str("q1_tscan_filter_t(%d)", DELTA);
    }
};



// "order by L_RETURNFLAG, L_LINESTATUS"
struct q1_key_extract_t : public key_extractor_t {
    q1_key_extract_t()
        : key_extractor_t(sizeof(char)*2, 0)
    {
        assert(sizeof(char) == 1);
    }
    virtual int extract_hint(const char* tuple_data) {
        // store the return flag and line status in the 
        projected_lineitem_tuple item;
        memcpy(&item, tuple_data, sizeof(item));

        int result = (item.L_RETURNFLAG << 8)
            + item.L_LINESTATUS;

        return result;
    }
    virtual q1_key_extract_t* clone() const {
        return new q1_key_extract_t(*this);
    }
};



// aggregate
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
        projected_lineitem_tuple src;
        memcpy(&src, s.data, sizeof(src));
        aggregate_tuple tuple;
        memcpy(&tuple, agg_data, sizeof(tuple));

        // cache resused values for convenience
        double L_EXTENDEDPRICE = src.L_EXTENDEDPRICE;
        double L_DISCOUNT = src.L_DISCOUNT;
        double L_QUANTITY = src.L_QUANTITY;
        double L_DISC_PRICE = L_EXTENDEDPRICE * (1 - L_DISCOUNT);

        // update count
        tuple.L_COUNT_ORDER++;
        
        // update sums
        tuple.L_SUM_QTY += L_QUANTITY;
        tuple.L_SUM_BASE_PRICE += L_EXTENDEDPRICE;
        tuple.L_SUM_DISC_PRICE += L_DISC_PRICE;
        tuple.L_SUM_CHARGE += L_DISC_PRICE * (1 + src.L_TAX);
        tuple.L_AVG_QTY += L_QUANTITY;
        tuple.L_AVG_PRICE += L_EXTENDEDPRICE;
        tuple.L_AVG_DISC += L_DISCOUNT;

        if(0) {
	if (((int) tuple.L_COUNT_ORDER) % 100 == 0) {
	    TRACE(TRACE_DEBUG, "%lf\n", tuple.L_COUNT_ORDER);
	    fflush(stdout);
	}
        }
        memcpy(agg_data, &tuple, sizeof(tuple));
    }

    
    virtual void finish(tuple_t &d, const char* agg_data) {
            aggregate_tuple dest;
            memcpy(&dest, agg_data, sizeof(dest));
            
            // compute averages
            dest.L_AVG_QTY /= dest.L_COUNT_ORDER;
            dest.L_AVG_PRICE /= dest.L_COUNT_ORDER;
            dest.L_AVG_DISC /= dest.L_COUNT_ORDER;
            memcpy(d.data, &dest, sizeof(dest));
    }

    virtual q1_count_aggregate_t* clone() const {
        return new q1_count_aggregate_t(*this);
    }

    virtual c_str to_string() const {
        return "q1_count_aggregate_t";
    }
};



void tpch_q1_driver::submit(void* disp) {

    scheduler::policy_t* dp = (scheduler::policy_t*)disp;
  
    // TSCAN PACKET
    tuple_fifo* tscan_out_buffer =
        new tuple_fifo(sizeof(projected_lineitem_tuple), dbenv);
    tscan_packet_t* q1_tscan_packet =
        new tscan_packet_t("lineitem TSCAN",
                           tscan_out_buffer,
                           new q1_tscan_filter_t(),
                           tpch_lineitem);

    // AGG PACKET CREATION
    guard<tuple_fifo> agg_output_buffer =
        new tuple_fifo(sizeof(aggregate_tuple), dbenv);
    packet_t* q1_agg_packet;
    q1_agg_packet = 
        new partial_aggregate_packet_t("Q1_AGG_PACKET",
                                       agg_output_buffer,
                                       new trivial_filter_t(agg_output_buffer->tuple_size()),
                                       q1_tscan_packet,
                                       new q1_count_aggregate_t(),
                                       new q1_key_extract_t(),
                                       new int_key_compare_t());

    qpipe::query_state_t* qs = dp->query_state_create();
    q1_agg_packet->assign_query_state(qs);
    q1_tscan_packet->assign_query_state(qs);
        
    // Dispatch packet
    dispatcher_t::dispatch_packet(q1_agg_packet);
    
    tuple_t output;
    TRACE(TRACE_QUERY_RESULTS, "*** Q1 ANSWER ...\n");
    TRACE(TRACE_QUERY_RESULTS, "*** SUM_QTY\tSUM_BASE\tSUM_DISC...\n");
    while(agg_output_buffer->get_tuple(output)) {
        aggregate_tuple tuple;
        memcpy(&tuple, output.data, sizeof(tuple));
        TRACE(TRACE_QUERY_RESULTS, "*** %lf\t%lf\t%lf\n",
              tuple.L_SUM_QTY,
              tuple.L_SUM_BASE_PRICE,
              tuple.L_SUM_DISC_PRICE);
    }


    dp->query_state_destroy(qs);
}

EXIT_NAMESPACE(workload);
