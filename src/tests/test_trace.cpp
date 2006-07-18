/* -*- mode:C++; c-basic-offset:4 -*- */

#include "engine/thread.h"
#include "trace.h"


int main(int, char**)
{
  thread_init();

  TRACE(TRACE_ALWAYS, "hello world from %s\n", thread_get_self()->get_thread_name().data());

  c_str hm("hello moon");

  for (int i = 0; i < 10; i++) {
      printf("i = %d: \n", i);
      c_str hm2(hm);
      c_str gnm("good night moon");
      c_str hm3(hm);
  }

  TRACE(TRACE_ALWAYS,
        "If you are seeing this message, c_str's probably work."
        "Turn on DEBUG_C_STR in c_str.h for a more thorough check.");

  return 0;
}
