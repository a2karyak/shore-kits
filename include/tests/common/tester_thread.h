/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef __TESTER_THREAD_H
#define __TESTER_THREAD_H

#include "util.h"



class tester_thread_t : public thread_t {

private:

    void (*start) (void*);
    void* start_arg;

public:

    void work() {
        start(start_arg);        
    }


    tester_thread_t(void (*start_routine)(void *), void* arg, const c_str &name)
        : thread_t(name),
          start(start_routine), start_arg(arg)
    {
    }

};



#endif /* __TESTER_THREAD_H */
