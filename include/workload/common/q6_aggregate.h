// -*- mode:C++; c-basic-offset:4 -*-

#ifndef _Q6_AGGREGATE_H
#define _Q6_AGGREGATE_H

#include "core.h"
#include <cmath>

using namespace qpipe;

#define TRACE_AGGREGATE 0



class q6_count_aggregate_t : public tuple_aggregate_t {

    struct agg_t {
        decimal sum;
        int count;
    };

    default_key_extractor_t _extractor;
public:

    q6_count_aggregate_t()
        : tuple_aggregate_t(sizeof(agg_t)),
          _extractor(0, 0)
    {
    }

    virtual key_extractor_t* key_extractor() { return &_extractor; }
    
    virtual void aggregate(char* agg_data, const tuple_t & src) {
        agg_t* agg = aligned_cast<agg_t>(agg_data);
	decimal * d = aligned_cast<decimal>(src.data);
        
	// update COUNT and SUM
	agg->count++;
	agg->sum += d[0] * d[1];
    
	if(TRACE_AGGREGATE && (agg->count % 10 == 0)) {

	    TRACE(TRACE_DEBUG, "%d - %lf\n", agg->count, agg->sum.to_double());
	    fflush(stdout);
	}
    }

    virtual void finish(tuple_t &dest, const char* agg_data) {
        agg_t* agg = aligned_cast<agg_t>(agg_data);
        decimal *output = aligned_cast<decimal>(dest.data);
        output[0] = agg->sum;
        output[1] = agg->count;
    }

    virtual q6_count_aggregate_t* clone() const {
        return new q6_count_aggregate_t(*this);
    }

    virtual c_str to_string() const {
        return "q6_count_aggregate_t";
    }
};


#endif
