/* -*- mode:C++; c-basic-offset:4 -*- */

#include "engine/thread.h"
#include "trace.h"
#include <map>
#include <string>

using std::string;



int main(int, char**)
{
  thread_init();
  std::map<string, int> s;

  s["hello"] = 5;

  TRACE(TRACE_ALWAYS, "hello maps to %d\n", s["hello"]);

  return 0;
}
