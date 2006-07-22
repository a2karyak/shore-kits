// -*- mode:C++; c-basic-offset:4 -*-

#ifndef _Q6_TSCAN_FILTER_H
#define _Q6_TSCAN_FILTER_H

#include "engine/functors.h"
#include "workload/tpch/tpch_struct.h"
#include "workload/tpch/tpch_type_convert.h"
#include "trace.h"
#include <math.h>

using namespace qpipe;



class q6_tscan_filter_t : public tuple_filter_t {

private:

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

        DISCOUNT = .06;
        QUANTITY = 24;
        if  ( ( tuple->L_SHIPDATE >= t1 ) &&
              ( tuple->L_SHIPDATE < t2 ) &&
              ( tuple->L_DISCOUNT >= (DISCOUNT - 0.01)) &&
              ( tuple->L_DISCOUNT <= (DISCOUNT + 0.01)) &&
              ( tuple->L_QUANTITY < (QUANTITY)) )
        {
            if (_echo) printf("+");
            return (true);
        }
        else {
            if (_echo) printf(".");
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

    virtual q6_tscan_filter_t* clone() {
        return new q6_tscan_filter_t(*this);
    }
};


#endif
