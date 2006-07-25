/* -*- mode:C++; c-basic-offset:4 -*- */

#include "server/command_set.h"
#include "server/config.h"
#include "server/print.h"

#include <cstdlib>
#include <cstdio>
#include <cstring>

#define USE_READLINE 1



#if USE_READLINE

#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>


static bool next_readline() {


    static char *command = (char*)NULL;


    // If the buffer has already been allocated, return the memory to
    // the free pool.
    if (command != NULL) {
        free(command);
        command = (char*)NULL;
    }


    // Get a line from the user.
    command = readline (QPIPE_PROMPT);
    if (command == NULL) {
        // EOF
        return false;
    }


    // history control
    if (*command)
        // non-empty line...
        add_history(command);
    

    if ( process_command(command) )
        // quit/exit command
        return false;


    // continue...
    return true;
}

#else

static bool next_fgets() {


    char command[SERVER_COMMAND_BUFFER_SIZE];
    PRINT("%s", QPIPE_PROMPT);
    if ( fgets(command, sizeof(command), stdin) == NULL )
        // EOF
        return false;


    // chomp off trailing '\n', if it exists
    int len = strlen(command);
    if ( command[len-1] == '\n' )
        command[len-1] = '\0';
    

    if ( process_command(command) )
        // quit/exit command
        return false;

    // continue...
    return true;
}

#endif







void interactive_mode(void) {


    PRINT("Interactive mode...\n");


    bool running = true;
    while (running) {

#if USE_READLINE
        running = next_readline();
#else
        running = next_fgets();
#endif

    }   
}
