// -*- mode:C++; c-basic-offset:4 -*-

#ifndef _Q6_AGGREGATE_H
#define _Q6_AGGREGATE_H

#include "engine/functors.h"
#include "trace.h"
#include <math.h>

using namespace qpipe;



class q6_count_aggregate_t : public tuple_aggregate_t {

private:
    int count;
    double sum;
    
public:
  
    q6_count_aggregate_t() {
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


#endif
