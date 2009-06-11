/* -*- mode:C++; c-basic-offset:4 -*-
     Shore-kits -- Benchmark implementations for Shore-MT
   
                       Copyright (c) 2007-2009
               Ecole Polytechnique Federale de Lausanne

                       Copyright (c) 2007-2008
                      Carnegie-Mellon University
   
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

#ifndef _STOPWATCH_H
#define _STOPWATCH_H

#include <sys/time.h>



/**
 *  @brief a timer object.
 */
class stopwatch_t {
private:
    struct timeval tv;
    long long mark;
public:
    stopwatch_t() {
        reset();
    }
    long long time_us() {
        long long old_mark = mark;
        reset();
        return mark - old_mark;
    }
    double time_ms() {
        return time_us()*1e-3;
    }
    double time() {
        return time_us()*1e-6;
    }
    long long now() {
        gettimeofday(&tv, NULL);
        return tv.tv_usec + tv.tv_sec*1000000ll;
    }
    void reset() {
	mark = now();
    }
};


#endif
