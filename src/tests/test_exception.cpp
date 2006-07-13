
#include <cstdlib>
#include <cstdio>
#include <exception>
#include <string>
#include <sstream>

#include "trace.h"
#include "engine/thread.h"
#include "engine.h"




int main(int, char**) {

  thread_init();

  try {
    throw QPIPE_EXCEPTION("hello world");
  } catch (QPipeException& e) {
    printf("%s\n", e.what());
  }

  throw QPIPE_EXCEPTION("hello2");

  return 0;
}
