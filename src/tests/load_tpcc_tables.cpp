/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file load_tpcc_tables.cpp
 *
 *  @brief Loads TPC-C tables
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#include <cstdio>
#include "util.h"
#include "workload/tpcc/tpcc_db_load.h"
#include "workload/tpcc/tpcc_db.h"

using namespace tpcc;

int main() {

  thread_init();
  //  db_open();
    
  db_tpcc_load("tpcc_database");

  //  db_close();
  return 0;
}
