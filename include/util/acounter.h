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
#ifndef _ACOUNTER_H
#define _ACOUNTER_H

#include "util/thread.h"
#include "util/sync.h"



/* exported datatypes */

class acounter_t {

 private:

    pthread_mutex_t _mutex;
    int _value;
  
 public:

    acounter_t(int value=0)
        : _mutex(thread_mutex_create()),
          _value(value)
    {
    }

    ~acounter_t() {
        thread_mutex_destroy(_mutex);
    }

    int fetch_and_inc() {
        critical_section_t cs(_mutex);
        return _value++;
    }
        
};



#endif
