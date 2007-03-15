/* -*- mode:C++; c-basic-offset:4 -*- */

#include "util.h"


int main(int, char**)
{
  thread_init();

  acounter_t a(1);
  
  for (int i = 0; i < 10; i++) {
      TRACE(TRACE_ALWAYS, "a.fetch_and_inc returns %d\n",
            a.fetch_and_inc());
  }

  return 0;
}
