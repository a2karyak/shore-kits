/* -*- mode:C++; c-basic-offset:4 -*- */

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
    void reset() {
        gettimeofday(&tv, NULL);
        mark = tv.tv_usec + tv.tv_sec*1000000ll;
    }
};


#endif
