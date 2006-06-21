// -*- mode:C++; c-basic-offset:4 -*-

#ifndef _COUNT_AGGREGATE_H
#define _COUNT_AGGREGATE_H


// count tuples, output single int
class count_aggregate_t : public tuple_aggregate_t {

private:
    default_key_extractor_t _extractor;
    
public:
  
    count_aggregate_t()
        : tuple_aggregate_t(sizeof(int)),
          _extractor(0)
    {
    }
  
    virtual key_extractor_t* key_extractor() { return &_extractor; }
    
    virtual void aggregate(char* agg_data, const tuple_t &) {
        ++*(int*) agg_data;
    }

    virtual void finish(tuple_t &dest, const char* agg_data) {
        memcpy(dest.data, agg_data, tuple_size());
    }

    virtual count_aggregate_t* clone() {
        return new count_aggregate_t(*this);
    }
};


#endif
