#include <cstdio>
#include "util.h"
#include "workload/tpch/tpch_db_load.h"
#include "workload/tpch/tpch_db.h"

int main() {
  util_init();
  //  db_open();
    
  db_load("tbl");

  //  db_close();
  return 0;
}
