/* -*- mode:C++; c-basic-offset:4 -*- */

#include "server/command/tpch_handler.h"
#include "server/print.h"
#include "workload/tpch/tpch_db.h"
#include "workload/register_job_drivers.h"
#include "qpipe_panic.h"



pthread_mutex_t tpch_handler_t::state_mutex = PTHREAD_MUTEX_INITIALIZER;

tpch_handler_t::state_t tpch_handler_t::state = TPCH_HANDLER_UNINITIALIZED;



/**
 *  @brief Initialize TPC-H handler. We must invoke db_open() to
 *  initial our global table environment, must we must ensure that
 *  this happens exactly once, despite the fact that we may register
 *  the same tpch_handler_t instance, or multiple tpch_handler_t
 *  instances for different command tags.
 */
void tpch_handler_t::init() {

    // use a global thread-safe state machine to ensure that db_open()
    // is called exactly once

    critical_section_t cs(&state_mutex);

    if ( state == TPCH_HANDLER_UNINITIALIZED ) {
        /* open DB tables */
        db_open();

        /* register jobs */
        register_job_drivers();

        state = TPCH_HANDLER_INITIALIZED;
    }
}



void tpch_handler_t::shutdown() {

    // use a global thread-safe state machine to ensure that
    // db_close() is called exactly once

    critical_section_t cs(&state_mutex);

    if ( state == TPCH_HANDLER_INITIALIZED ) {
        /* close DB tables */
        PRINT("... closing db\n");
        db_close();
        state = TPCH_HANDLER_SHUTDOWN;
    }
}



void tpch_handler_t::handle_command(const char* command) {
    PRINT("TPCH ACTION HANDLER\n");
}
