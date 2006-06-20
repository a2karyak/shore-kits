// -*- mode:C++; c-basic-offset:4 -*-

#ifndef _COUNT_AGGREGATE_H
#define _COUNT_AGGREGATE_H


// count tuples, output single int
class count_aggregate_t : public tuple_aggregate_t {

private:
    int count;
    
public:
  
    count_aggregate_t() {
        count = 0;
    }
  
    bool aggregate(tuple_t &, const tuple_t &) {
        count++;
        return false;
    }

    bool eof(tuple_t &dest) {
        *(int*)dest.data = count;
        return true;
    }
};


#endif
