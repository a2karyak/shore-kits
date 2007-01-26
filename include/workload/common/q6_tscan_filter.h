// -*- mode:C++; c-basic-offset:4 -*-

#ifndef _Q6_TSCAN_FILTER_H
#define _Q6_TSCAN_FILTER_H

#include "core.h"
#include "workload/tpch/tpch_struct.h"
#include "workload/tpch/tpch_type_convert.h"
#include "workload/common/predicates.h"
#include <cmath>

using namespace qpipe;



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
    double DISCOUNT;

    /* QUANTITY is randon [24 .. 25] */
    double QUANTITY;

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

        TRACE(TRACE_DEBUG, "Q6 - DISCOUNT = %.2f. QUANTITY = %.2f\n", DISCOUNT, QUANTITY);

        if(0) {
            // override for validation run
            DATE = datestr_to_timet("1994-01-01");
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
        p = new scalar_predicate_t<double, greater_equal>(DISCOUNT-.01, offset);
        _filter.add(p);
        p = new scalar_predicate_t<double, less_equal>(DISCOUNT+.01, offset);
        _filter.add(p);
        
        offset = offsetof(tpch_lineitem_tuple, L_QUANTITY);
        p = new scalar_predicate_t<double, less>(QUANTITY, offset);
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
	double *dt = aligned_cast<double>(dest.data);
	dt[0] = at->L_EXTENDEDPRICE;
	dt[1] = at->L_DISCOUNT;
    }

    virtual q6_tscan_filter_t* clone() const {
        return new q6_tscan_filter_t(*this);
    }

    virtual c_str to_string() const {
        char* date = timet_to_datestr(DATE);
        c_str result("q6_tscan_filter_t(%s, %f, %f)", date, QUANTITY, DISCOUNT);
        free(date);
        return result;
    }
};


#endif
