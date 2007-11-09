/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file acounter.h
 *
 *  @brief An acounter_t is a counter with an atomic fetch and
 *  increment operation. This implementation is thread safe.
 *
 *  @author Naju Mancheril (ngm)
 *
 *  @bug None known.
 */
#ifndef __ACOUNTER_H
#define __ACOUNTER_H

#include "util/thread.h"

#ifdef __SUNPRO_CC
#include <atomic.h>
#else
#include "util/sync.h"
#endif


/* exported datatypes */

class acounter_t {

 private:

    uint _value;
    pthread_mutex_t _mutex;
  
 public:

    acounter_t(int value=0)
        : _value(value)
#ifndef __SUNPRO_CC
        ,_mutex(thread_mutex_create())
#endif
    {
    }

    ~acounter_t() {
#ifndef __SUNPRO_CC
        thread_mutex_destroy(_mutex);
#endif
    }

    uint fetch_and_inc() {
#ifndef __SUNPRO_CC
        critical_section_t cs(_mutex);
        _value++;
#else
        atomic_inc_uint(&_value);
#endif


        return (_value);
    }
        
};



#endif
