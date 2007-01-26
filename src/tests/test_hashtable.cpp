/* -*- mode:C++; c-basic-offset:4 -*- */

#include "util.h"
#include "util/hashtable.h"


int main(int, char**)
{
  thread_init();

  TRACE(TRACE_ALWAYS, "hello world from %s\n", thread_get_self()->thread_name().data());

  c_str hm("hello moon");

  return 0;
}
