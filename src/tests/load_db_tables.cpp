/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file load_db_tables.cpp
 *
 *  @brief Loads TPC-H tables
 */

#include <cstdio>
#include "util.h"
#include "workload/tpch/tpch_db_load.h"
#include "workload/tpch/tpch_db.h"

using namespace tpch;

int main() {
  thread_init();
  //  db_open();
    
  db_load("tbl");

  //  db_close();
  return 0;
}
