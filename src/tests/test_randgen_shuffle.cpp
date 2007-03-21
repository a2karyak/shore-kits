/* -*- mode:C++; c-basic-offset:4 -*- */

#include "util.h"


int main(int, char**)
{
  thread_init();

  TRACE(TRACE_ALWAYS, "hello world from %s\n", thread_get_self()->thread_name().data());

  TRACE(TRACE_ALWAYS, "Method 1:\n");
  int n = 20;
  int A[n];
  for (int i = 0; i < n; i++)
      A[i] = i;
  randgen_t randgen = *thread_get_self()->randgen();

  randgen_t copy1 = randgen;
  randgen_util_t::shuffle(A, n, &copy1);
  for (int i = 0; i < n; i++)
      printf("A[%d] = %d\n", i, A[i]);

  TRACE(TRACE_ALWAYS, "Method 2 ... should produce same results:\n");
  randgen_t copy2 = randgen;
  randgen_util_t::random_in_range(A, n, 0, 19, &copy2);
  for (int i = 0; i < n; i++)
      printf("A[%d] = %d\n", i, A[i]);


  return 0;
}
