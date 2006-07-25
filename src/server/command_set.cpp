/* -*- mode:C++; c-basic-offset:4 -*- */

#include "server/command_set.h"
#include "server/print.h"
#include "server/config.h"
#include "server/command/command_handler.h"
#include "server/command/printer.h"
#include "server/command/tpch_handler.h"
#include "engine/util/c_str.h"

#include <map>
#include <string>

using std::map;
using std::string;



/* internal data structures */

static map<c_str, command_handler_t*> command_mappings;



/* internal helper functions */

static void add_command(const c_str &command_tag, command_handler_t* handler);
static void add_command(const char*  command_tag, command_handler_t* handler);
static void dispatch_command(const char* command);



/* definitions of exported functions */


/**
 *  @brief Register a set of command tag-command handler mappings. All
 *  command handlers should be registered here, in this function.
 *
 *  @return void
 */
void register_command_handlers(void) {

    // Initialize the specified handler (by invoking the init()
    // method) and insert the handler into the command set using the
    // specified command tag.

    add_command("print", new printer_t());
    add_command("tpch",  new tpch_handler_t());
}



/**
 *  @brief Process specifed command (which should be either the direct
 *  input of an fgets() or a list of command line args concatenated
 *  together.
 *
 *  @param command The command to run. This command may be mutated.
 *
 *  @return 0 to continue executing if called from interactive
 *  mode. Non-zero it command specified that program should quit.
 */
int process_command(const char* command) {

    // check for quit...
    if ( strcasecmp(command, "quit") == 0 )
        // quit command!
        return 1;
           
    dispatch_command(command);
    return 0;
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
static void add_command(const char* command_tag, command_handler_t* handler) {
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
        PRINT("Unregistered command %s\n", command_tag);
        return;
    }

    // pass the handler the whole command... this lets us register the
    // same handler for multiple commands
    handler->handle_command(command);
}
