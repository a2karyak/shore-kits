/* -*- mode:C++; c-basic-offset:4 -*- */

#include "util.h"
#include "server/command_set.h"
#include "server/print.h"
#include "server/config.h"
#include "server/command/command_handler.h"
#include "server/command/printer.h"
#include "server/command/tpch_handler.h"
#include "server/command/load_handler.h"
#include "server/command/tpcc_handler.h"
#include "server/command/tracer.h"

#include "workload/register_stage_containers.h"

#include <map>


#ifdef __SUNPRO_CC
#include <strings.h>
#else
#include <string>
#endif


using std::map;
using std::string;



/* internal data structures */

static map<c_str, command_handler_t*> command_mappings;



/* internal helper functions */

static void add_command(const c_str &command_tag, command_handler_t* handler);
void add_command(const char*  command_tag, command_handler_t* handler);
static void dispatch_command(const char* command);
bool check_quit(const char* command);


/* definitions of exported functions */


/**
 *  @brief Register a set of command tag-command handler mappings. All
 *  command handlers should be registered here, in this function.
 *
 *  @param int - select between query and transaction processing environment
 *
 *  @return void
 */
void register_command_handlers( const int environment ) {

    // Initialize the specified handler (by invoking the init()
    // method) and insert the handler into the command set using the
    // specified command tag.

    // Utilities
    add_command("print", new printer_t());
    add_command("tracer", new tracer_t());

    // Data Loads
    add_command("load", new load_handler_t());

    if (environment == QUERY_ENV) {
        // Query Processing
        add_command("tpch",  new tpch_handler_t());
    }
    else {
        // Trx Processing
        add_command("tpcc", new tpcc_handler_t());
    }
}



/**
 *  @brief Process specifed command (which should be either the direct
 *  input of an fgets() or a list of command line args concatenated
 *  together.
 *
 *  We now distinguish between a QUIT command and a SHUTDOWN
 *  command. The two are equivalent when QPIPE is being run from the
 *  command line, since they terminate the server process. When QPIPE
 *  is being run in network mode, QUIT will terminate the current
 *  connection and let the server accept a new connection. SHUTDOWN
 *  will terminate the current connection and force the server process
 *  to exit.
 *
 *  @param command The command to run. This command may be mutated.
 *
 *  @return 0 to continue executing if called from interactive
 *  mode. Non-zero it command specified that program should quit.
 */

int process_command(const char* command) {

    // check for quit...
    if ( check_quit(command) )
        // quit command!
        return PROCESS_NEXT_QUIT;

    // check for shutdown...
    if ( strcasecmp(command, "shutdown") == 0 )
        // quit command!
        return PROCESS_NEXT_SHUTDOWN;
           
    dispatch_command(command);
    return PROCESS_NEXT_CONTINUE;
}



void shutdown_command_handlers(void) {
    
    map<c_str, command_handler_t*>::iterator iter;
    for (iter = command_mappings.begin(); iter != command_mappings.end(); ) {
        
        command_handler_t* handler = iter->second;

        // TODO Get rid of this check by using non-mutating lookups in
        // the map...
        if (handler != NULL) {

            handler->shutdown();
            
            // We would like to delete the command handlers, but we don't
            // really own them. We don't know how many times this same
            // handler has been registered with different tags. To avoid
            // double-delete, allow memory leak. It doesn't "really"
            // matter since the server is shutting down.
            
            // TODO Fix this by keeping a separate list where all handlers
            // are stored exactly once. Delete every element of that list.
        }

        ++iter;
    }
    
}



/* definitions of internal helper functions */


/**
 *  @brief Add specified command handler to the set of command tag
 *  mappings after initializing it. The init method of a command
 *  handler is responsible for "doing the right thing" if the init()
 *  method is called more than once.
 *
 *  @param command_tag The command tag to register the handler for.
 *
 *  @param handler The command handler to register. Handler will be
 *  initialized.
 *
 *  @return void
 */
static void add_command(const c_str &command_tag, command_handler_t* handler) {

    // error checking... make sure no previous mapping exists
    assert(command_mappings.find(command_tag) == command_mappings.end());
    handler->init();
    command_mappings[command_tag] = handler;
}



/**
 *  @brief Same as add_command(), overloaded for char* command_tag.
 *
 *  @param command_tag The command tag to register the handler for.
 *
 *  @param handler The command handler to register. Handler will be
 *  initialized.
 *
 *  @return void
 */
void add_command(const char* command_tag, command_handler_t* handler) {
    c_str tag("%s", command_tag);
    add_command(tag, handler);
}



/**
 *  @brief Dispatch the specified command. This is a helper function
 *  used by run_command() to parse the command tag from the specified
 *  command and execute the registered handler, if one exists.
 *
 *  @param command The command to dispatch.
 *
 *  @return void
 */
static void dispatch_command(const char* command) {

    // we index command handlers by the first white-space delimited
    // string in the command
    char command_tag[SERVER_COMMAND_BUFFER_SIZE];
    if ( sscanf(command, "%s", command_tag) < 1 )
        // no command ... empty string?
        return;

    command_handler_t* handler = command_mappings[command_tag];
    if ( handler == NULL ) {
        // unregistered command
        TRACE(TRACE_ALWAYS, "Unregistered command %s\n", command_tag);
        return;
    }

    // pass the handler the whole command... this lets us register the
    // same handler for multiple commands
    handler->handle_command(command);
}



/** @fn check_quit
 *
 *  @brief Check if user asked the system to quit. Now, it accepts not
 *  only the "quit" command, but also other variations, such asL
 *  "q", "quit;", "exit", "exit;" 
 *
 *  @param command The command tag to register the handler for.
 *
 *  @return true if command is to quit.
 */
bool check_quit(const char* command) {

    if (( strcasecmp(command, "quit") == 0 ) ||
        ( strcasecmp(command, "quit;") == 0 ) ||
        ( strcasecmp(command, "q") == 0 ) ||
        ( strcasecmp(command, "q;") == 0 ) ||
        ( strcasecmp(command, "exit") == 0 ) ||
        ( strcasecmp(command, "exit;") == 0 ) )
        // quit command!
        return (true);

    return (false);
}

