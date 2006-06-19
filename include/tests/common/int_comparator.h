
#ifndef _INT_COMPARATOR_H
#define _INT_COMPARATOR_H

struct int_comparator_t : public tuple_comparator_t {
  virtual int make_key(const tuple_t &tuple) {
    return *(int*)tuple.data;
  }       
};

#endif
