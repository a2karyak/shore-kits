
#include "thread.h"
#include "trace/trace.h"


using namespace qpipe;

int main(int, char**)
{
  thread_init();

  TRACE(TRACE_ALWAYS, "hello world from %s\n", thread_get_self_name());

  return 0;
}
