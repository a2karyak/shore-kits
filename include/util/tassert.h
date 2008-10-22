/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef __UTIL_TASSERT_H
#define __UTIL_TASSERT_H

#include <cstdlib>
#include "util/trace.h"


/* exported macros */

#define TASSERT(cond) {                          \
    if (!(cond)) {                               \
      TRACE(TRACE_ALWAYS,                        \
            "Assertion \'%s\' failed\n",         \
            (#cond));                            \
      abort();                                   \
    }                                            \
  }


#endif /** __UTIL_TASSERT_H */
