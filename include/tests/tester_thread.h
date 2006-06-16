/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef __TESTER_THREAD_H
#define __TESTER_THREAD_H

#include "engine/thread.h"
#include <stdarg.h>



class tester_thread_t : public thread_t {

private:

  void* (*start) (void*);
  void* start_arg;

public:

  void* run() {
    return start(start_arg);
  }


  tester_thread_t(void* (*start_routine)(void *), void* arg, const char* format, ...) 
    : start(start_routine), start_arg(arg)
  {
    va_list ap;
    va_start(ap, format);
    init_thread_name_v(format, ap);
    va_end(ap);
  }

};



#endif /* __TESTER_THREAD_H */
