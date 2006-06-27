
#include "trace.h"
#include "engine/thread.h"

#include <bitset>
#include <vector>

using std::bitset;
using std::vector;

int main(int, char**) {
  
  bitset<2048> bs;
  TRACE(TRACE_ALWAYS, "sizeof(bs) = %d\n", sizeof(bs));

  vector<bool> bv(2048);
  bv.reserve(2048);
  TRACE(TRACE_ALWAYS, "sizeof(bv) = %d\n", sizeof(bv));
  TRACE(TRACE_ALWAYS, "bv.capacity() = %d\n", bv.capacity());
  bv.clear();
  
  
  return 0;
}
