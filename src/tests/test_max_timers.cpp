/* -*- mode:C++; c-basic-offset:4 -*- */

/**
 * @brief Test how many thread-specific timers we can create...
 */
#include "util.h"
#include "tests/common.h"
#include <time.h>
#include <sys/time.h>



#ifndef _POSIX_THREAD_CPUTIME
//#warning No per-thread timers!
#endif



void* thread_main(void* count_arg)
{

    int* countp = (int*)count_arg;
   /* create timer */
    timer_t timerid;
    if (timer_create(CLOCK_THREAD_CPUTIME_ID, NULL, &timerid)) {
        TRACE(TRACE_ALWAYS, "timer_create() failed. count = %d\n", *countp);
        abort();
    }
    TRACE(TRACE_ALWAYS, "Created a timer with id %d\n", timerid);
    free(countp);
    return NULL;
}


int main(int, char**)
{

  int count = 0;
  while (1) {
      c_str thread_name("THREAD_%d", count);
      int* count_arg = (int*)malloc(sizeof(int));
      *count_arg = count;
      tester_thread_t* thread = new tester_thread_t(thread_main, count_arg, thread_name);
      thread_create(thread, NULL);
      count++;
  }

  return 0;
}
