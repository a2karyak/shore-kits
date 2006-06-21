// -*- mode:C++; c-basic-offset:4 -*-

#include "trace.h"
#include "qpipe_panic.h"
#include "tests/common.h"

#include <unistd.h>
#include <sys/time.h>
#include <math.h>



/**
 *  @brief : Calls a table scan and outputs a specific projection
 */

int main() {

    thread_init();

    if ( !db_open(DB_CREATE|DB_TRUNCATE|DB_THREAD) ) {
        TRACE(TRACE_ALWAYS, "db_open() failed\n");
        QPIPE_PANIC();
    }        


    if ( !db_load() )
        TRACE(TRACE_ALWAYS, "db_load() failed\n");


    if ( !db_close() )
        TRACE(TRACE_ALWAYS, "db_close() failed\n");
    return 0;
}
