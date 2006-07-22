// -*- mode:C++; c-basic-offset:4 -*-

#include "trace.h"
#include "qpipe_panic.h"
#include "tests/common.h"
#include "workload/tpch/tpch_db.h"

#include <unistd.h>
#include <sys/time.h>
#include <math.h>



/**
 *  @brief : Calls a table scan and outputs a specific projection
 */

int main() {

    thread_init();
    db_open(DB_CREATE|DB_TRUNCATE|DB_THREAD);
    db_load();
    db_close();
    return 0;
}
