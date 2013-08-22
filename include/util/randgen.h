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

#ifndef __RANDGEN_H
#define __RANDGEN_H

#include "sm_vas.h"

class randgen_t 
{
public:

    randgen_t() {
        /* prefer not to reset: sthreads actually supplies a much better seed than this
           interface allows anyway */
    }

    void reset(unsigned int seed) {
        sthread_t::tls_rng()->seed(&seed, 1);
    }
    
    int rand() {
        return sthread_t::me()->rand() & 0x7fffffff;
    }
    
    /**
     * Returns a pseudorandom, uniformly distributed int value between
     * 0 (inclusive) and the specified value (exclusive).
     */
    int rand(int n) {
        assert(n > 0);
        return sthread_t::me()->randn(n);
    }
    
};


#endif
