/* -*- mode:C++; c-basic-offset:4 -*- */
#ifndef __TPCH_Q1_TYPES_H
#define __TPCH_Q1_TYPES_H

#include "workload/common.h"
#include "workload/tpch/tpch_struct.h"
#include "workload/tpch/tpch_type_convert.h"


ENTER_NAMESPACE(q1);


// the tuples after tablescan projection
struct projected_lineitem_tuple {
    decimal L_QUANTITY;
    decimal L_EXTENDEDPRICE;
    decimal L_DISCOUNT;
    decimal L_TAX;
    char L_RETURNFLAG;
    char L_LINESTATUS;
};


// the final aggregated tuples
struct aggregate_tuple {
    decimal L_SUM_QTY;
    decimal L_SUM_BASE_PRICE;
    decimal L_SUM_DISC_PRICE;
    decimal L_SUM_CHARGE;
    decimal L_AVG_QTY;
    decimal L_AVG_PRICE;
    decimal L_AVG_DISC;
    decimal L_COUNT_ORDER;
    char L_RETURNFLAG;
    char L_LINESTATUS;
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

        /* select random number generator */
        ACQUIRE_PREDICATE_RANDGEN(randgen);

        /* predicates */
	t = datestr_to_timet("1998-12-01");
        DELTA = 60 + randgen.rand(61);
	t = time_add_day(t, -DELTA); // 1998-12-01 - DELTA days
    }


    /* Predication */
    virtual bool select(const tuple_t &input) {

	/* Predicate:
	   L_SHIPDATE <= 1998-12-01 - DELTA DAYS
	*/

	tpch_lineitem_tuple *tuple = aligned_cast<tpch_lineitem_tuple>(input.data);

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
        dest = aligned_cast<projected_lineitem_tuple>(d.data);
        src = aligned_cast<tpch_lineitem_tuple>(s.data);

        dest->L_RETURNFLAG = src->L_RETURNFLAG;
        dest->L_LINESTATUS = src->L_LINESTATUS;
        dest->L_QUANTITY = src->L_QUANTITY;
        dest->L_EXTENDEDPRICE = src->L_EXTENDEDPRICE;
        dest->L_DISCOUNT = src->L_DISCOUNT;
        dest->L_TAX = src->L_TAX;
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
        : key_extractor_t(sizeof(char)*2, offsetof(projected_lineitem_tuple, L_RETURNFLAG))
    {
        assert(sizeof(char) == 1);
    }
    virtual int extract_hint(const char* tuple_data) const {
        // store the return flag and line status in the 
        projected_lineitem_tuple *item;
        item = aligned_cast<projected_lineitem_tuple>(tuple_data);

        int result = (item->L_RETURNFLAG << 8)
            + item->L_LINESTATUS;

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
        projected_lineitem_tuple *src;
        src = aligned_cast<projected_lineitem_tuple>(s.data);
        aggregate_tuple* tuple = aligned_cast<aggregate_tuple>(agg_data);

        // cache resused values for convenience
        decimal L_EXTENDEDPRICE = src->L_EXTENDEDPRICE;
        decimal L_DISCOUNT = src->L_DISCOUNT;
        decimal L_QUANTITY = src->L_QUANTITY;
        decimal L_DISC_PRICE = L_EXTENDEDPRICE * (1 - L_DISCOUNT);

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

        if(0) {
	if ((tuple->L_COUNT_ORDER).to_int() % 100 == 0) {
	    TRACE(TRACE_DEBUG, "%lf\n", tuple->L_COUNT_ORDER.to_double());
	    fflush(stdout);
	}
        }
    }

    
    virtual void finish(tuple_t &d, const char* agg_data) {
            aggregate_tuple *dest;
            dest = aligned_cast<aggregate_tuple>(d.data);
            aggregate_tuple* tuple = aligned_cast<aggregate_tuple>(agg_data);

            *dest = *tuple;
            // compute averages
            dest->L_AVG_QTY /= dest->L_COUNT_ORDER;
            dest->L_AVG_PRICE /= dest->L_COUNT_ORDER;
            dest->L_AVG_DISC /= dest->L_COUNT_ORDER;
    }

    virtual q1_count_aggregate_t* clone() const {
        return new q1_count_aggregate_t(*this);
    }

    virtual c_str to_string() const {
        return "q1_count_aggregate_t";
    }
};


class tpch_q1_process_tuple_t : public process_tuple_t {
    
public:
        
    virtual void begin() {
        TRACE(TRACE_QUERY_RESULTS, "*** Q1 ANSWER ...\n");
        TRACE(TRACE_QUERY_RESULTS, "*** SUM_QTY\tSUM_BASE\tSUM_DISC...\n");
    }
    
    virtual void process(const tuple_t& output) {
        aggregate_tuple *tuple;
        tuple = aligned_cast<aggregate_tuple>(output.data);
        TRACE(TRACE_QUERY_RESULTS, "*** %lf\t%lf\t%lf\n",
              tuple->L_SUM_QTY.to_double(),
              tuple->L_SUM_BASE_PRICE.to_double(),
              tuple->L_SUM_DISC_PRICE.to_double());
    }

};


EXIT_NAMESPACE(q1);


#endif
