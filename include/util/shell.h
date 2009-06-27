/* -*- mode:C++; c-basic-offset:4 -*-
     Shore-kits -- Benchmark implementations for Shore-MT
   
                       Copyright (c) 2007-2009
      Data Intensive Applications and Systems Labaratory (DIAS)
               Ecole Polytechnique Federale de Lausanne
   
                         All Rights Reserved.
   
   Permission to use, copy, modify and distribute this software and
   its documentation is hereby granted, provided that both the
   copyright notice and this permission notice appear in all copies of
   the software, derivative works or modified versions, and any
   portions thereof, and that both notices appear in supporting
   documentation.
   
   This code is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. THE AUTHORS
   DISCLAIM ANY LIABILITY OF ANY KIND FOR ANY DAMAGES WHATSOEVER
   RESULTING FROM THE USE OF THIS SOFTWARE.
*/

/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   shell.h
 *
 *  @brief:  Abstract shell class for the test cases
 *
 *  @author: Ippokratis Pandis (ipandis)
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

#include <map>

#include <readline/readline.h>
#include <readline/history.h>
#include <assert.h>
#include <signal.h>
#include <errno.h>

#include "util/command/command_handler.h"
#include "util/command/tracer.h"
#include "util/envvar.h"

#include "util.h"


using namespace std;



extern "C" void sig_handler_fwd(int sig);


typedef map<string,command_handler_t*> cmdMap;
typedef cmdMap::iterator cmdMapIt;



/*********************************************************************
 *
 *  @brief: Few basic commands
 *
 *********************************************************************/

class envVar;


struct quit_cmd_t : public command_handler_t {
    void setaliases();
    const int handle(const char* cmd) { return (SHELL_NEXT_QUIT); }
    const string desc() { return (string("Quit")); }               
};


struct help_cmd_t : public command_handler_t {
    cmdMap* _pcmds; // pointer to the supported commands
    help_cmd_t(cmdMap* pcmds) : _pcmds(pcmds) { assert(pcmds); }
    ~help_cmd_t() { }
    void setaliases();
    const int handle(const char* cmd);
    void usage() { 
        TRACE( TRACE_ALWAYS, "HELP       - prints usage\n"); 
        TRACE( TRACE_ALWAYS, "HELP <cmd> - prints detailed help for <cmd>\n"); 
    }
    const string desc() { return (string("Help - Use 'help <cmd>' for usage of specific cmd")); }              
    void list_cmds();
}; 


struct set_cmd_t : public command_handler_t {
    envVar* ev;
    void init() { ev = envVar::instance(); }
    void setaliases();
    const int handle(const char* cmd);
    void usage();
    const string desc() { return (string("Sets env vars")); }               
};

struct env_cmd_t : public command_handler_t {
    envVar* ev;
    void init() { ev = envVar::instance(); }
    void setaliases();
    const int handle(const char* cmd);
    void usage();
    const string desc() { return (string("Prints env vars")); }               
};

struct conf_cmd_t : public command_handler_t {
    envVar* ev;
    void init() { ev = envVar::instance(); }
    void setaliases();
    const int handle(const char* cmd);
    void usage();
    const string desc() { return (string("Rereads env vars")); }               
};


#ifdef __sparcv9
struct cpustat_cmd_t : public command_handler_t {
    processinfo_t myinfo;
    void setaliases();
    const int handle(const char* cmd);
    void usage();
    const string desc() { return (string("Process cpu usage/statitics")); }
};
#endif


struct echo_cmd_t : public command_handler_t {
    void setaliases() { _name = string("echo"); _aliases.push_back("echo"); }
    const int handle(const char* cmd) {
        printf("%s\n", cmd+strlen("echo "));
        return (SHELL_NEXT_CONTINUE);
    }
    void usage() { TRACE( TRACE_ALWAYS, "usage: echo <string>\n"); }
    const string desc() { return string("Echoes its input arguments to the screen"); }
};



struct break_cmd_t : public command_handler_t {
    void setaliases() { _name = string("break"); _aliases.push_back("break"); }
    const int handle(const char* cmd) {
        raise(SIGINT);
        return (SHELL_NEXT_CONTINUE);
    }
    void usage() { TRACE( TRACE_ALWAYS, "usage: break\n"); }
    const string desc() { return string("Breaks into a debugger by raising ^C " \
                                        "(terminates program if no debugger is active)"); }
};




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



class shell_t 
{
protected:

    bool  _save_history;
    int   _state;

    // cmds
    guard<quit_cmd_t> _quiter;
    guard<help_cmd_t> _helper;
    guard<set_cmd_t>  _seter;
    guard<env_cmd_t>  _enver;
    guard<conf_cmd_t> _confer;
    guard<trace_cmd_t>   _tracer;
    guard<echo_cmd_t> _echoer;
    guard<break_cmd_t> _breaker;

#ifdef __sparcv9
    guard<cpustat_cmd_t> _cpustater;
#endif

    const int _register_commands();    

protected:

    mcs_lock _lock;
    cmdMap _cmds;
    cmdMap _aliases;
    bool _processing_command;

    char* _cmd_prompt;
    int   _cmd_counter;
    
public:

    shell_t(const char* prompt = SCLIENT_PROMPT, bool save_history = true) 
        : _cmd_counter(0), _save_history(save_history), 
          _state(SHELL_NEXT_CONTINUE), _processing_command(false)
    {
        _cmd_prompt = new char[SHELL_COMMAND_BUFFER_SIZE];
        if (prompt) strncpy(_cmd_prompt, prompt, strlen(prompt));
        _register_commands();
    }

    virtual ~shell_t() { 
        if (_cmd_prompt) delete [] _cmd_prompt;
        _cmd_prompt = NULL;
    }


    static shell_t* &instance() { static shell_t* _instance; return _instance; }

    const int get_command_cnt() { return (_cmd_counter); }

    static void sig_handler(int sig) {
	assert(sig == SIGINT && instance());	
	if( int rval=instance()->SIGINT_handler() )
	    exit(rval);
    }

    // should register own commands
    virtual const int register_commands()=0;
    const int add_cmd(command_handler_t* acmd);
    const int init_cmds();
    const int close_cmds();

    // basic shell functionality    
    //    virtual int process_command(const char* cmd, const char* cmd_tag)=0;

    virtual int SIGINT_handler() { return (ECANCELED); /* exit immediately */ }     
    int start();
    int process_one();
    virtual int process_command(const char* command, const char* command_tag)=0;

}; // EOF: shell_t



#endif /* __UTIL_SHELL_H */

