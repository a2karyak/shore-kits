#include <cstdio>
#include "engine/thread.h"
#include "workload/tpch/tpch_db_load.h"
#include "workload/tpch/tpch_db.h"

int main() {
  thread_init();
  db_open(DB_CREATE);
    
  db_load("tbl");

  db_close();
  return 0;
}
