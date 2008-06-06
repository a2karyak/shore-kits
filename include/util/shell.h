/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   shell.h
 *
 *  @brief:  Abstract shell class for the test cases
 *
 *  @author: Ippokratis Pandis (ipandis)
 *
 *  @buf:    Should trap on Ctrl+C
 */

#ifndef __UTIL_SHELL_H
#define __UTIL_SHELL_H

#ifdef __SUNPRO_CC
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#else
#include <cstdlib>
#include <cstdio>
#include <cstring>
#endif


#include <readline/readline.h>
#include <readline/history.h>
#include <assert.h>

#include "util/config.h"
#include "util/history.h"
#include "util/trace.h"


/*********************************************************************
 *
 *  abstract class: shell_t
 *
 *  brief:          Base class for shells.
 *
 *  usage:          - Inherit from this class
 *                  - Implement the process_command() function
 *                  - Call the start() function
 *
 *********************************************************************/


// constants
#define SHELL_COMMAND_BUFFER_SIZE 64
#define SHELL_NEXT_CONTINUE       1
#define SHELL_NEXT_QUIT           2

class shell_t {

private:

    char* _cmd_prompt;
    int   _cmd_counter;
    bool  _save_history;
    int   _state;

public:


    shell_t(const char* prompt = QPIPE_PROMPT, bool save_history = true) 
        : _cmd_counter(0), _save_history(save_history), _state(SHELL_NEXT_CONTINUE)
    {
        _cmd_prompt = new char[SHELL_COMMAND_BUFFER_SIZE];
        if (prompt)
            strncpy(_cmd_prompt, prompt, strlen(prompt));
    }

    virtual ~shell_t() 
    {
        if (_cmd_prompt)
            delete (_cmd_prompt);       
    }

    virtual int process_command(const char* cmd)=0;
    virtual int print_usage(const char* cmd)=0;


    // basic shell functionality
    int process_one() { 

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
            // quit command! 
            return (print_usage(command));
        }

        // increase stats
        _cmd_counter++;

        return (process_command(command));
    }

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

    bool check_help(const char* command) { 

        if (( strcasecmp(command, "help") == 0 ) ||
            ( strcasecmp(command, "help;") == 0 ) ||
            ( strcasecmp(command, "h") == 0 ) ||
            ( strcasecmp(command, "h;") == 0 ) )
            // help command!
            return (true);
        
        return (false);
    }
    
    const int get_command_cnt() { return (_cmd_counter); }


    int start() 
    {         
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
        
        return (0);
    }

};



#endif /* __UTIL_SHELL_H */

