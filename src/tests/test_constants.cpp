/* -*- mode:C++; c-basic-offset:4 -*- */
#include "util.h"
#include <signal.h>


int main(int, char**)
{
  thread_init();

  TRACE(TRACE_ALWAYS, "SIGSTKSZ = %d\n", SIGSTKSZ);

  return 0;
}
