/* -*- mode:C++; c-basic-offset:4 -*- */

#include "server/config.h"
#include "server/print.h"
#include "server/command_set.h"
#include "server/history.h"
#include "server/process_next_command_using_fgets.h"


#ifdef __SUNPRO_CC
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#else
#include <cstdlib>
#include <cstdio>
#include <cstring>
#endif


#if USE_READLINE


#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>


static int process_next_command_using_readline() {


    static char *command = (char*)NULL;


    // If the buffer has already been allocated, return the memory to
    // the free pool.
    if (command != NULL) {
        free(command);
        command = (char*)NULL;
    }


    // Get a line from the user.
    command = readline(QPIPE_PROMPT);
    if (command == NULL) {
        // EOF
        return PROCESS_NEXT_QUIT;
    }


    // history control
    if (*command)
        // non-empty line...
        add_history(command);
    

    return process_command(command);
}


#endif



void interactive_mode(void) {

    TRACE(TRACE_ALWAYS, "Interactive mode...\n");

    
#if USE_READLINE
    bool save_history = history_open();
#endif


    /* Interactive mode only distinguishes between a
       PROCESS_NEXT_CONTINUE and everything else (quit, shutdown,
       etc). Everything else results in a termination of interactive
       mode. */
    
    int state = PROCESS_NEXT_CONTINUE;
    while (state == PROCESS_NEXT_CONTINUE) {
#if USE_READLINE
        state = process_next_command_using_readline();
#else
        state = process_next_command_using_fgets(stdin, true);
#endif
    }


#if USE_READLINE
    if (save_history) {
        TRACE(TRACE_ALWAYS, "Saving history...\n");
        history_close();
    }
#endif


}
