// -*- mode:C++; c-basic-offset:4 -*-
#ifndef _INT_COMPARATOR_H
#define _INT_COMPARATOR_H

struct int_key_compare_t : public key_compare_t {
    virtual int operator()(const void*, const void*) {
        // should be unreachable!
        assert(false);
    }
    virtual key_compare_t* clone() { return new int_key_compare_t(*this); }
};

struct int_key_extractor_t : public key_extractor_t {
  virtual void extract_key(void* key, const void* tuple_data) {
      memcpy(key, tuple_data, key_size());
  }       
    virtual key_extractor_t* clone() { return new int_key_extractor_t(*this); }
};

#endif
