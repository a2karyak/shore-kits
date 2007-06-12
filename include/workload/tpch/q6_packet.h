// -*- mode:C++; c-basic-offset:4 -*-

#ifndef _Q6_PACKET_H
#define _Q6_PACKET_H


#include "core.h"
#include "scheduler.h"
#include "workload/tpch/tpch_struct.h"
#include "workload/tpch/tpch_type_convert.h"
#include "workload/common/process_tuple.h"
#include "workload/common/predicates.h"

using namespace qpipe;
using namespace scheduler;
using namespace workload;

#define TRACE_Q6_AGGREGATE 0


struct q6_agg_t {
    decimal sum;
    int count;
};


class q6_tscan_filter_t : public tuple_filter_t {

private:

    and_predicate_t _filter;
    bool _echo;

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
    decimal DISCOUNT;

    /* QUANTITY is randon [24 .. 25] */
    decimal QUANTITY;

public:

    /* Initialize the predicates */ 
   q6_tscan_filter_t(bool echo=false)
        : tuple_filter_t(sizeof(tpch_lineitem_tuple)),
          _echo(echo)
    {
        thread_t* self = thread_get_self();

        // DATE
        t1 = datestr_to_timet("1993-01-01");
        t1 = time_add_year(t1, self->rand(5));
        t2 = time_add_year(t1, 1);

        // DISCOUNT
        DISCOUNT = .02 + self->rand(8)*.01;

        // QUANTITY
        QUANTITY = 24 + .01*self->rand(101);

        //        TRACE(TRACE_DEBUG, "Q6 - DISCOUNT = %.2f. QUANTITY = %.2f\n", 
        //      DISCOUNT.to_double(), QUANTITY.to_double());

        if(0) {
            // override for validation run
            DATE = datestr_to_timet("1994-01-01");
	    t1 = DATE;
	    t2 = time_add_year(t1, 1);
            QUANTITY = 24;
            DISCOUNT = .06;
        }
        size_t offset;
        predicate_t* p;

        /* Predicate:
           L_SHIPDATE >= DATE AND
           L_SHIPDATE < DATE + 1 YEAR AND
           L_DISCOUNT BETWEEN DISCOUNT - 0.01 AND DISCOUNT + 0.01 AND
           L_QUANTITY < QUANTITY
        */

        offset = offsetof(tpch_lineitem_tuple, L_SHIPDATE);
        p = new scalar_predicate_t<time_t, greater_equal>(t1, offset);
        _filter.add(p);
        p = new scalar_predicate_t<time_t, less>(t2, offset);
        _filter.add(p);

        offset = offsetof(tpch_lineitem_tuple, L_DISCOUNT);
        p = new scalar_predicate_t<decimal, greater_equal>(DISCOUNT-.01, offset);
        _filter.add(p);
        p = new scalar_predicate_t<decimal, less_equal>(DISCOUNT+.01, offset);
        _filter.add(p);
        
        offset = offsetof(tpch_lineitem_tuple, L_QUANTITY);
        p = new scalar_predicate_t<decimal, less>(QUANTITY, offset);
        _filter.add(p);
    }


    /* Predication */
    virtual bool select(const tuple_t &input) {
        return _filter.select(input);
    }
    
    /* Projection */
    virtual void project(tuple_t &dest, const tuple_t &src) {

        /* Should project L_EXTENDEDPRICE & L_DISCOUNT */

        // Calculate L_EXTENDEDPRICE
        tpch_lineitem_tuple *at = aligned_cast<tpch_lineitem_tuple>(src.data);
	decimal *dt = aligned_cast<decimal>(dest.data);
	dt[0] = at->L_EXTENDEDPRICE;
	dt[1] = at->L_DISCOUNT;
    }

    virtual q6_tscan_filter_t* clone() const {
        return new q6_tscan_filter_t(*this);
    }

    virtual c_str to_string() const {
        char* date = timet_to_datestr(DATE);
        c_str result("q6_tscan_filter_t(%s, %f, %f)",
		     date, QUANTITY.to_double(), DISCOUNT.to_double());
        free(date);
        return result;
    }
};


class q6_count_aggregate_t : public tuple_aggregate_t {

    default_key_extractor_t _extractor;

public:

    q6_count_aggregate_t()
        : tuple_aggregate_t(sizeof(q6_agg_t)),
          _extractor(0, 0)
    {
    }

    virtual key_extractor_t* key_extractor() { return &_extractor; }
    
    virtual void aggregate(char* agg_data, const tuple_t & src) {
        q6_agg_t* agg = aligned_cast<q6_agg_t>(agg_data);
	decimal * d = aligned_cast<decimal>(src.data);
        
	// update COUNT and SUM
	agg->count++;
	agg->sum += d[0] * d[1];
    
	if(TRACE_Q6_AGGREGATE && (agg->count % 10 == 0))
	    TRACE(TRACE_ALWAYS, "%d - %lf\n",
                  agg->count,
                  agg->sum.to_double());
    }

    virtual void finish(tuple_t &dest, const char* agg_data) {
        q6_agg_t* agg = aligned_cast<q6_agg_t>(agg_data);
        q6_agg_t* output = aligned_cast<q6_agg_t>(dest.data);
        output->count = agg->count;
        output->sum   = agg->sum;
    }

    virtual q6_count_aggregate_t* clone() const {
        return new q6_count_aggregate_t(*this);
    }

    virtual c_str to_string() const {
        return "q6_count_aggregate_t";
    }
};


class q6_process_tuple_t : public process_tuple_t {
public:
    virtual void process(const tuple_t& output) {
        q6_agg_t* r = aligned_cast<q6_agg_t>(output.data);
        TRACE(TRACE_QUERY_RESULTS,
              "*** Q6 Count: %u. Sum: %.2f.  ***\n", r->count, (float)r->sum.to_double());
    }
};


packet_t* create_q6_packet(const c_str &client_prefix, policy_t* dp);


#endif
