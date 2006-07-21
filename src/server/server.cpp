/* -*- mode:C++; c-basic-offset:4 -*- */

#include "engine/thread.h"
#include "server/print.h"
#include "server/config.h"
#include "server/command_set.h"
#include "workload/register_job_drivers.h"
#include "workload/register_stage_containers.h"
#include "workload/tpch/tpch_db.h"




/* internal helper functions */

int  qpipe_init(int argc, char* argv[]);
void qpipe_shutdown(void);



int main(int argc, char* argv[]) {


    // qpipe_init() panics on error
    int run_interactive_mode = qpipe_init(argc, argv);


    if ( run_interactive_mode ) {
        PRINT("Interactive mode...\n");

        while (1) {
            char command_buf[SERVER_COMMAND_BUFFER_SIZE];
            PRINT("%s", QPIPE_PROMPT);
            if ( fgets(command_buf, sizeof(command_buf), stdin) == NULL )
                break;
           
            dispatch_command(command_buf);
        }   
    }


    // qpipe_shutdown() panics on error
    qpipe_shutdown();
    return 0; 
}



int qpipe_init(int, char*[]) {

    thread_init();
    PRINT("QPipe Execution Engine\n");

    register_commands();


    /* open DB tables */
    if ( !db_open() ) {
        TRACE(TRACE_ALWAYS, "db_open() failed\n");
        QPIPE_PANIC();
    }        
    
    /* start up stages */
    register_stage_containers();

    /* register jobs */
    register_job_drivers();
    
    // TRACE_SET(TRACE_ALWAYS | TRACE_CPU_BINDING);
    TRACE_SET(TRACE_ALWAYS);

    return 1;
}



void qpipe_shutdown() {

    /* close DB tables */
    if ( !db_close() )
        TRACE(TRACE_ALWAYS, "db_close() failed\n");
 
    PRINT("... closing db\n");
    PRINT("Thank you for using QPipe\nExiting...\n");
}
