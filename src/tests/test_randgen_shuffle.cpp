/* -*- mode:C++; c-basic-offset:4 -*- */

#include "util.h"


int main(int, char**)
{
  thread_init();

  TRACE(TRACE_ALWAYS, "hello world from %s\n", thread_get_self()->thread_name().data());

  int n = 20;
  int A[n];
  
  for (int i = 0; i < n; i++)
      A[i] = i;

  randgen_util_t::shuffle(A, n, thread_get_self()->randgen());
  
  for (int i = 0; i < n; i++)
      printf("A[%d] = %d\n", i, A[i]);

  return 0;
}
