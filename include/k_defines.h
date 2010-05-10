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

/** @file:   k_defines.h
 *
 *  @brief:  Defines the correct includes, atomic primitives, etc..
 *           Used so that a single shore-kits codebase to compile with
 *           both Shore-MT and Shore-SM-6.X.X (the x86_64 or Nancy's port)
 *
 *  @author: Ippokratis Pandis, May 2010
 *
 */

#ifndef __KITS_DEFINES_H
#define __KITS_DEFINES_H

// If we use the x86-ported branch (aka Nancy's branch) then w_defines.h should
// be included instead of atomic.h

#ifdef CFG_SHORE_6

#include <w_defines.h>
#include <atomic_templates.h>

#else // CFG_SHORE_MT

#include <atomic.h>

#endif


#ifdef __SUNPRO_CC
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#else
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cstdarg>
#endif


#endif /** __KITS_DEFINES_H */
