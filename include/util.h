/* -*- mode:C++; c-basic-offset:4 -*-
     Shore-kits -- Benchmark implementations for Shore-MT
   
                       Copyright (c) 2007-2009
      Data Intensive Applications and Systems Labaratory (DIAS)
               Ecole Polytechnique Federale de Lausanne
   
                         All Rights Reserved.
   
   Permission to use, copy, modify and distribute this software and
   its documentation is hereby granted, provided that both the
   copyright notice and this permission notice appear in all copies of
   the software, derivative works or modified versions, and any
   portions thereof, and that both notices appear in supporting
   documentation.
   
   This code is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. THE AUTHORS
   DISCLAIM ANY LIABILITY OF ANY KIND FOR ANY DAMAGES WHATSOEVER
   RESULTING FROM THE USE OF THIS SOFTWARE.
*/

/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef __UTIL_H
#define __UTIL_H

#include "util/compat.h"
#include "util/c_str.h"
#include "util/decimal.h"
#include "util/exception.h"
#include "util/guard.h"
#include "util/namespace.h"
#include "util/stopwatch.h"
#include "util/sync.h"
#include "util/thread.h"
#include "util/time_util.h"
#include "util/trace.h"
#include "util/tassert.h"
#include "util/randgen.h"
#include "util/store_string.h"
#include "util/pool_alloc.h"
#include "util/file.h"
#include "util/progress.h"
#include "util/history.h"
#include "util/countdown.h"
#include "util/confparser.h"
#include "util/envvar.h"
#include "util/shell.h"
#include "util/condex.h"
#include "util/random_input.h"
#include "util/atomic_ops.h"

#ifdef __sparcv9
#include "util/prcinfo.h"
#endif

#ifdef __GNUC__
#include "util/w_strlcpy.h"
#endif

#endif /** __UTIL_H */
