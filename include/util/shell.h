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

class shell_t {

private:

    char* _cmd_prompt;
    int   _cmd_counter;
    bool  _save_history;
    int   _state;

    // basic shell functionality
    int process_one_command();    
    bool check_quit(const char* command);

protected:

    // constants
    static const int PROMPT_COMMAND_BUFFER_SIZE = 64;
    static const int PROMPT_NEXT_CONTINUE = 1;
    static const int PROMPT_NEXT_QUIT     = 2;


public: 

    // constructor - destructor
    shell_t(const char* command = QPIPE_PROMPT, bool save_history = true)
        : _cmd_counter(0), _save_history(save_history)    
    {
        _cmd_prompt = new char[PROMPT_COMMAND_BUFFER_SIZE];
        if (command)
            strncpy(_cmd_prompt, command, strlen(command));

        TRACE( TRACE_DEBUG, "Shell (%s) : created\n", _cmd_prompt);
    }


    virtual ~shell_t() {

        TRACE( TRACE_DEBUG, "Shell (%s): (%d) commands processed...\n", 
               _cmd_prompt, _cmd_counter);

        if (_cmd_prompt)
            delete (_cmd_prompt);
    }


    // shell entry point
    const int start();


    // override this function for processing arbitraty commands
    virtual int process_command(const char* command) {

        TRACE( TRACE_ALWAYS, "(%s)\n", command);
        return (PROMPT_NEXT_CONTINUE);
    }


    // override for usage
    virtual void print_usage(int argc, char* argv[]) {
        
        TRACE( TRACE_ALWAYS, "Usage:\n"); 
    }



}; // EOF: shell_t 


#endif /* __UTIL_SHELL_H */

