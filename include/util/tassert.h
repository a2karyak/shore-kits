/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _TASSERT_H
#define _TASSERT_H

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


#endif
