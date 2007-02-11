/* -*- mode:C++; c-basic-offset:4 -*- */

#include "util.h"
#include "server/print.h"
#include "server/interactive_mode.h"
#include "server/network_mode.h"
#include "server/command_set.h"
#include "server/config.h"
#include "workload/register_stage_containers.h"



/* internal helper functions */

int  qpipe_init(int argc, char* argv[]);
void qpipe_shutdown(void);



/* internal constants */

#define RUN_INTERACTIVE_MODE 1
#define RUN_NETWORK_MODE     2



int main(int argc, char* argv[]) {

    // qpipe_init() panics on error
    int mode = qpipe_init(argc, argv);
    
    
    if ( mode == RUN_INTERACTIVE_MODE )
        interactive_mode();
    if ( mode == RUN_NETWORK_MODE )
        network_mode(QPIPE_NETWORK_MODE_DEFAULT_LISTEN_PORT);
    

    // qpipe_shutdown() panics on error
    qpipe_shutdown();
    return 0; 
}



int qpipe_init(int argc, char* argv[]) {

    thread_init();
    TRACE(TRACE_ALWAYS, "QPipe Execution Engine\n");
    
    register_command_handlers();
    register_stage_containers();

    TRACE_SET(TRACE_ALWAYS | TRACE_STATISTICS | TRACE_NETWORK | TRACE_CPU_BINDING
              | TRACE_PACKET_FLOW);

    if ((argc > 1) && (strcmp(argv[1], "-n") == 0)) {
        return RUN_NETWORK_MODE;
    }    
    
    return RUN_INTERACTIVE_MODE;
}



void qpipe_shutdown() {
    shutdown_command_handlers();
    TRACE(TRACE_ALWAYS, "Thank you for using QPipe\nExiting...\n");
}
