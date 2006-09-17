// -*- mode:C++; c-basic-offset:4 -*-
#ifndef _INT_COMPARATOR_H
#define _INT_COMPARATOR_H

#include "core.h"

using namespace qpipe;

struct int_key_compare_t : public key_compare_t {
    virtual int operator()(const void*, const void*) {
        // should be unreachable!
        assert(false);
    }
    virtual key_compare_t* clone() const {
        return new int_key_compare_t(*this);
    }
};

typedef default_key_extractor_t int_key_extractor_t;

#endif
