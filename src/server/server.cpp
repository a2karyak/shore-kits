/* -*- mode:C++; c-basic-offset:4 -*- */

#include "engine/thread.h"
#include "server/print.h"
#include "server/interactive_mode.h"
#include "server/command_set.h"
#include "workload/register_job_drivers.h"
#include "workload/register_stage_containers.h"



/* internal helper functions */

int  qpipe_init(int argc, char* argv[]);
void qpipe_shutdown(void);



int main(int argc, char* argv[]) {

    // qpipe_init() panics on error
    int run_interactive_mode = qpipe_init(argc, argv);


    if ( run_interactive_mode )
        interactive_mode();


    // qpipe_shutdown() panics on error
    qpipe_shutdown();
    return 0; 
}



int qpipe_init(int, char*[]) {

    thread_init();
    PRINT("QPipe Execution Engine\n");

    register_command_handlers();

    
    /* start up stages */
    register_stage_containers();

    /* register jobs */
    register_job_drivers();
    
    // TRACE_SET(TRACE_ALWAYS | TRACE_CPU_BINDING);
    TRACE_SET(TRACE_ALWAYS);

    return 1;
}



void qpipe_shutdown() {
    shutdown_command_handlers();
    PRINT("Thank you for using QPipe\nExiting...\n");
}
