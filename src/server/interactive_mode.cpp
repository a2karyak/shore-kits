/* -*- mode:C++; c-basic-offset:4 -*- */

#include "server/command_set.h"
#include "server/config.h"
#include "server/print.h"

#include <cstdio>
#include <cstring>


void interactive_mode(void) {


    PRINT("Interactive mode...\n");


    while (1) {

        char command[SERVER_COMMAND_BUFFER_SIZE];
        PRINT("%s", QPIPE_PROMPT);
        if ( fgets(command, sizeof(command), stdin) == NULL )
            // EOF
            break;


        // chomp off trailing '\n', if it exists
        int len = strlen(command);
        if ( command[len-1] == '\n' )
            command[len-1] = '\0';


        if ( process_command(command) )
            // quit/exit command
            break;
    }   
}
