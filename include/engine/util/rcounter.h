/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _RCOUNTER_H
#define _RCOUNTER_H

#include "engine/sync.h"


class rcounter_t {

private:

    pthread_mutex_t _mutex;
    pthread_cond_t  _cond;
    int             _count;

public:

    rcounter_t(int initial_count);
    ~rcounter_t();

    void increment();
    void decrement();
    
    void wait_for_zero();
};



#endif
