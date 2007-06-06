/* -*- mode:C++; c-basic-offset:4 -*- */

#include <unistd.h>
#include <sys/time.h>
#include <cmath>

#include "util.h"
#include "tests/common.h"
#include "workload/tpch/tpch_db.h"
#include "workload/tpch/tpch_db_load.h"

using namespace tpch;

/**
 *  @brief : Calls a table scan and outputs a specific projection
 */

int main() {

    thread_init();
    
    /* NGM: Removed DB_TRUNCATE flag since it was causing an exception
       to be thrown in table open. As a temporary fix, we will simply
       delete the old database file before rebuilding. */
    db_open(DB_CREATE|DB_THREAD); // Huh? Why is this commented out?
    db_load();
    db_close();

    return 0;
}
