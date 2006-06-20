// -*- mode:C++; c-basic-offset:4 -*-

#ifndef _SINGLE_INT_JOIN_H
#define _SINGLE_INT_JOIN_H

#include "engine/core/tuple.h"
#include "engine/core/stage.h"
#include <cstring>

using namespace qpipe;



struct single_int_join_t : public tuple_join_t {

    single_int_join_t()
        : tuple_join_t(sizeof(int), sizeof(int), sizeof(int), sizeof(int))
    {
    }

    virtual void get_left_key(char *key, const tuple_t &tuple) {
        memcpy(key, tuple.data, left_tuple_size());
    }

    virtual void get_right_key(char *key, const tuple_t &tuple) {
        memcpy(key, tuple.data, right_tuple_size());
    }

    virtual void join(tuple_t &dest,
                      const tuple_t &left,
                      const tuple_t &)
    {
        dest.assign(left);
    }
};      



#endif
