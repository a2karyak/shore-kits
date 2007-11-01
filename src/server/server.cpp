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
            { TRACE( TRACE_ALWAYS, "Batch mode unimplemented\n"); }
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

    // The default environment is the DSS
    int processing_env = QUERY_ENV;


    // Parse user selection for the environment
    if (argc > 1) {
        
        if ((strcmp(argv[1], "-bdb-trx") == 0)) {
            // BDB underneath
            TRACE( TRACE_ALWAYS,
                   "BDB-StagedTRX\n");
            TRACE( TRACE_ALWAYS,
                   "Transaction Processing Engine with BerkeleyDB storage manager\n");
            processing_env = TRX_BDB_ENV;
        }
        else if ((strcmp(argv[1], "-shore-trx") == 0)) {
            // SHORE underneath
            TRACE( TRACE_ALWAYS,
                   "SHORE-StagedTRX\n");
            TRACE( TRACE_ALWAYS,
                   "Transaction Processing Engine with SHORE storage manager\n");
            processing_env = TRX_SHORE_ENV;
        }
        else if ((strcmp(argv[1], "-mem-trx") == 0)) {
            // In main memory
            TRACE( TRACE_ALWAYS,
                   "InMemory-StagedTRX\n");
            TRACE( TRACE_ALWAYS,
                   "Main Memory Transaction Processing Engine\n");
            processing_env = TRX_MEM_ENV;
        }
    } 

    if (processing_env == QUERY_ENV) {
        TRACE( TRACE_ALWAYS, 
               "Cordoba\n");
        TRACE( TRACE_ALWAYS,
               "Query Execution Engine\n");
    }
    
    register_command_handlers(processing_env);
    register_stage_containers(processing_env);


    TRACE_SET( TRACE_ALWAYS | TRACE_STATISTICS | TRACE_NETWORK | TRACE_CPU_BINDING
              //              | TRACE_QUERY_RESULTS
              //              | TRACE_PACKET_FLOW
              // | TRACE_RECORD_FLOW
              //              | TRACE_TRX_FLOW
              // | TRACE_DEBUG
              );

    if ((argc > 1) && (strcmp(argv[1], "-n") == 0)) {
        return RUN_NETWORK_MODE;
    }    
    
    return RUN_INTERACTIVE_MODE;
}



void qpipe_shutdown() {
    shutdown_command_handlers();    
    TRACE(TRACE_ALWAYS, "Staged Database System exiting...\n");
}
