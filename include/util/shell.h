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
#include <signal.h>
#include <errno.h>

#include "util/config.h"
#include "util/history.h"
#include "util/trace.h"




/*********************************************************************
 *
 *  @abstract class: shell_t
 *
 *  @brief:          Base class for shells.
 *
 *  @usage:          - Inherit from this class
 *                   - Implement the process_command() function
 *                   - Call the start() function
 *
 *********************************************************************/


// constants
const int SHELL_COMMAND_BUFFER_SIZE = 64;
const int SHELL_NEXT_CONTINUE       = 1;
const int SHELL_NEXT_QUIT           = 2;


extern "C" void sig_handler_fwd(int sig);


class shell_t 
{
private:

    char* _cmd_prompt;
    int   _cmd_counter;
    bool  _save_history;
    int   _state;
    
protected:
    bool _processing_command;
    
public:


    shell_t(const char* prompt = QPIPE_PROMPT, bool save_history = true) 
        : _cmd_counter(0), _save_history(save_history), 
          _state(SHELL_NEXT_CONTINUE), _processing_command(false)
    {
        _cmd_prompt = new char[SHELL_COMMAND_BUFFER_SIZE];
        if (prompt)
            strncpy(_cmd_prompt, prompt, strlen(prompt));
    }

    virtual ~shell_t() 
    {
        if (_cmd_prompt)
            delete [] _cmd_prompt;       
    }


    static shell_t* &instance() { static shell_t* _instance; return _instance; }

    static void sig_handler(int sig) {
	assert(sig == SIGINT && instance());	
	if( int rval=instance()->SIGINT_handler() )
	    exit(rval);
    }


    // basic shell functionality    
    virtual int process_command(const char* cmd)=0;
    virtual int print_usage(const char* cmd)=0;
    virtual int SIGINT_handler() { return (ECANCELED); /* exit immediately */ }

    int process_one();
    bool check_quit(const char* command);
    bool check_help(const char* command);
    
    const int get_command_cnt() { return (_cmd_counter); }


    int start();

}; // EOF: shell_t



#endif /* __UTIL_SHELL_H */

