/* -*- mode:C++; c-basic-offset:4 -*- */

#include "util.h"
#include "core/tuple_fifo_directory.h"
#include "server/print.h"
#include "server/interactive_mode.h"
#include "server/network_mode.h"
#include "server/command_set.h"
#include "server/config.h"
#include "workload/register_stage_containers.h"


/* internal helper functions */

int  qpipe_init(int argc, char* argv[]);
void qpipe_shutdown(void);

int processing_env = QUERY_ENV;


/* internal constants */

#define RUN_INTERACTIVE_MODE 1
#define RUN_NETWORK_MODE     2
#define RUN_BATCH_MODE       3


int main(int argc, char* argv[]) {

    try {

        // qpipe_init() panics on error
        int mode = qpipe_init(argc, argv);

        switch (mode) {
        case RUN_INTERACTIVE_MODE:
            interactive_mode();
            break;
        case RUN_NETWORK_MODE:
            network_mode(QPIPE_NETWORK_MODE_DEFAULT_LISTEN_PORT);
            break;
        case RUN_BATCH_MODE:
            TRACE( TRACE_ALWAYS, "Batch mode unimplemented\n");
            break;
        default:
            interactive_mode();
            break;
        }
    }
    catch (...) {
        // catches any arbitrary exception raised
        TRACE( TRACE_ALWAYS, "Exception reached main()\tExiting...\n");
        qpipe_shutdown();
        return (-1);
    }
         

    // qpipe_shutdown() panics on error
    qpipe_shutdown();
    return 0; 
}



int qpipe_init(int argc, char* argv[]) {

    thread_init();
    qpipe::tuple_fifo_directory_t::open_once();


    int environment = QUERY_ENV;

    // Parse TPC-H / TPC-C environment
    if ((argc > 1) && (strcmp(argv[1], "-trx") == 0)) {
        TRACE( TRACE_ALWAYS,
               "StagedTRX - Transaction Processing Engine\n");
        processing_env = TRX_ENV;
    } 
    else {
        TRACE( TRACE_ALWAYS, 
               "Cordoba - Query Execution Engine\n");
    }
    
    register_command_handlers(processing_env);
    register_stage_containers(processing_env);


    TRACE_SET( TRACE_ALWAYS | TRACE_STATISTICS | TRACE_NETWORK | TRACE_CPU_BINDING
              //              | TRACE_QUERY_RESULTS
              //              | TRACE_PACKET_FLOW
              //              | TRACE_TRX_FLOW
              );

    if ((argc > 1) && (strcmp(argv[1], "-n") == 0)) {
        return RUN_NETWORK_MODE;
    }    
    
    return RUN_INTERACTIVE_MODE;
}



void qpipe_shutdown() {
    shutdown_command_handlers();
    
    if (processing_env == QUERY_ENV) {
        TRACE(TRACE_ALWAYS, "Thank you for using Cordoba\nExiting...\n");
    }
    else {
        TRACE(TRACE_ALWAYS, "Thank you for using StagedTRX\nExiting...\n");
    }
}
