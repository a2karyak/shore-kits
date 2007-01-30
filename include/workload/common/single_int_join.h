// -*- mode:C++; c-basic-offset:4 -*-

#ifndef _SINGLE_INT_JOIN_H
#define _SINGLE_INT_JOIN_H

#include "core.h"

using namespace qpipe;

ENTER_NAMESPACE(workload);



struct single_int_join_t : public tuple_join_t {

    single_int_join_t()
        : tuple_join_t(sizeof(int), 0,
                       sizeof(int), 0,
                       sizeof(int),
                       sizeof(int))
    {
    }

    virtual void join(tuple_t &dest,
                      const tuple_t &left,
                      const tuple_t &)
    {
        /* The two integers are equal. Just copy this value to the
           output. */
        dest.assign(left);
    }

    virtual c_str to_string() const {
        return "single_int_join_t";
    }
};      

EXIT_NAMESPACE(workload);


#endif
