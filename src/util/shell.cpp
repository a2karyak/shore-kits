/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   shell.cpp
 *
 *  @brief:  Implementation of an abstract shell class for the test cases
 *
 *  @author: Ippokratis Pandis (ipandis)
 *
 *  @buf:    Should trap on Ctrl+C
 */


#include "util/shell.h"


/*********************************************************************
 *
 *  @fn:    start
 *  
 *  @brief: Starts a loop of commands. 
 *          
 *  @note:  Exits only if the processed command returns 
 *          PROMPT_NEXT_QUIT. 
 *
 *********************************************************************/

const int shell_t::start() 
{
    /* 1. Open saved command history (optional) */
    if (_save_history)
        _save_history = history_open();


    /* 2. Command loop */
    _state = PROMPT_NEXT_CONTINUE;
    while (_state == PROMPT_NEXT_CONTINUE) {
        _state = process_one_command();
    }    


    /* 3. Save command history (optional) */
    if (_save_history) {
        TRACE( TRACE_ALWAYS, "Saving history...\n");
        history_close();
    }

    return (0);
}



/*********************************************************************
 *
 *  @fn:    process_one_command
 *  
 *  @brief: Basic checks done by any shell
 *
 *********************************************************************/

int shell_t::process_one_command() 
{
    char *command = (char*)NULL;

    // If the buffer has already been allocated, return the memory to
    // the free pool.
    if (command != NULL) {
        free(command);
        command = (char*)NULL;
    }

    // Get a line from the user.
    command = readline(_cmd_prompt);
    if (command == NULL) {
        // EOF
        return (PROMPT_NEXT_QUIT);
    }

    // history control
    if (*command) {
        // non-empty line...
        add_history(command);
    }

    // check for quit...
    if ( check_quit(command) ) {
        // quit command! 
       return (PROMPT_NEXT_QUIT);
    }

    // update stats
    _cmd_counter++;

    return (process_command(command));
}



/*********************************************************************
 *
 *  @fn:    check_quit
 *  
 *  @brief: Checks for a set of known quit commands
 *
 *********************************************************************/

bool shell_t::check_quit(const char* command) 
{
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

