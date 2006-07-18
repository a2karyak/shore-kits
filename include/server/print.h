/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _PRINT_H
#define _PRINT_H

#include "trace.h"

#define PRINT(format, rest...) TRACE(TRACE_ALWAYS, format, ##rest)

#endif
