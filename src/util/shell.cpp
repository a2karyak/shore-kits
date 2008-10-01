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


void sig_handler_fwd(int sig)
{
    shell_t::sig_handler(sig);
}



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

int shell_t::start() 
{
    /* 0. Install SIGINT handler */
    instance() = this;
    struct sigaction sa;
    struct sigaction sa_old;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    //sa.sa_handler = &shell_t::sig_handler;
    sa.sa_handler = &sig_handler_fwd;

    if(sigaction(SIGINT, &sa, &sa_old) < 0)
        exit(1);
	
    /* 1. Open saved command history (optional) */
    if (_save_history)
        _save_history = history_open();
        
    /* 2. Command loop */
    _state = SHELL_NEXT_CONTINUE;
    while (_state == SHELL_NEXT_CONTINUE) {
        _state = process_one();
    }    
        
    /* 3. Save command history (optional) */
    if (_save_history) {
        TRACE( TRACE_ALWAYS, "Saving history...\n");
        history_close();
    }

    /* 4. Restore old signal handler (probably unnecessary) */
    sigaction(SIGINT, &sa_old, 0);
	
    return (0);
}



/*********************************************************************
 *
 *  @fn:    process_one
 *  
 *  @brief: Basic checks done by any shell
 *
 *********************************************************************/

int shell_t::process_one() 
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
        return (SHELL_NEXT_QUIT);
    }
        
    // history control
    if (*command) {
        // non-empty line...
        add_history(command);
    }
        
    // check for quit...
    if ( check_quit(command) ) {
        // quit command! 
        return (SHELL_NEXT_QUIT);
    }

    // check for help...
    if ( check_help(command) ) {
        // help command! 
        return (print_usage(command));
    }

    // increase stats
    _cmd_counter++;

    _processing_command = true;
    int rval = process_command(command);
    _processing_command = false;
    return (rval);
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


/*********************************************************************
 *
 *  @fn:    check_help
 *  
 *  @brief: Checks for a set of known help commands
 *
 *********************************************************************/

bool shell_t::check_help(const char* command) 
{    
    if (( strcasecmp(command, "help") == 0 ) ||
        ( strcasecmp(command, "help;") == 0 ) ||
        ( strcasecmp(command, "h") == 0 ) ||
        ( strcasecmp(command, "h;") == 0 ) )
        // help command!
        return (true);
        
    return (false);
}

