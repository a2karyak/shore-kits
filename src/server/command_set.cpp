/* -*- mode:C++; c-basic-offset:4 -*- */

#include "server/command_set.h"
#include "server/print.h"
#include "server/command/command_handler.h"
#include "server/command/printer.h"
#include "server/config.h"

#include <map>
#include <string>

using std::map;
using std::string;



/* internal data structures */

static map<string, command_handler_t*> command_mappings;



/* definitions of exported functions */



void register_commands(void) {

    command_mappings["print"] = new printer_t();
}



void dispatch_command(const char* command) {

    // we index command handlers by the first white-space delimited
    // string in the command
    char command_tag[SERVER_COMMAND_BUFFER_SIZE];
    if ( sscanf(command, "%s", command_tag) < 1 )
        // no command ... empty string?
        return;

    command_handler_t* handler = command_mappings[command_tag];
    if ( handler == NULL ) {
        // unregistered command
        PRINT("Unregistered command %s\n", command_tag);
        return;
    }

    // pass the handler the whole command... this lets us register the
    // same handler for multiple commands
    handler->handle(command);
}
