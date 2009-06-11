/* -*- mode:C++; c-basic-offset:4 -*-
     Shore-kits -- Benchmark implementations for Shore-MT
   
                       Copyright (c) 2007-2009
               Ecole Polytechnique Federale de Lausanne

                       Copyright (c) 2007-2008
                      Carnegie-Mellon University
   
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
    // 0. Install SIGINT handler
    instance() = this;
    struct sigaction sa;
    struct sigaction sa_old;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    //sa.sa_handler = &shell_t::sig_handler;
    sa.sa_handler = &sig_handler_fwd;

    if(sigaction(SIGINT, &sa, &sa_old) < 0)
        exit(1);
	
    // 1. Init all commands
    init_cmds();

    // 2. Open saved command history (optional)
    if (_save_history)
        _save_history = history_open();
        
    // 3. Command loop
    _state = SHELL_NEXT_CONTINUE;
    while (_state == SHELL_NEXT_CONTINUE) {
        _state = process_one();
    }    
        
    // 4. Save command history (optional)
    if (_save_history) {
        TRACE( TRACE_ALWAYS, "Saving history...\n");
        history_close();
    }

    // 5. Close all commands
    close_cmds();

    // 6. Restore old signal handler (probably unnecessary)
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
    char *cmd = (char*)NULL;
        
    // Get a line from the user.
    cmd = readline(_cmd_prompt);
    if (cmd == NULL) {
        // EOF
        return (SHELL_NEXT_QUIT);
    }

    char cmd_tag[SERVER_COMMAND_BUFFER_SIZE];
    CRITICAL_SECTION(sh_cs,_lock);

    if ( sscanf(cmd, "%s", &cmd_tag) < 1) {
        _helper->list_cmds();
        return (SHELL_NEXT_CONTINUE);
    }
        
    // history control
    if (*cmd) {
        // non-empty line...
        add_history(cmd);
    }

    // increase stats
    _cmd_counter++;

    _processing_command = true;

    int rval;
    cmdMapIt cmdit = _aliases.find(cmd_tag);
    if (cmdit == _aliases.end()) {
        rval = process_command(cmd, cmd_tag);
    }
    else {
        rval = cmdit->second->handle(cmd);
    }

    _processing_command = false;
    return (rval);
}


/*********************************************************************
 *
 *  @fn:    add_cmd()
 *  
 *  @brief: Registers a shell command    
 *
 *********************************************************************/

const int shell_t::add_cmd(command_handler_t* acmd) 
{
    assert (acmd);
    assert (!acmd->name().empty());
    cmdMapIt cmdit;

    // register main name
    cmdit = _cmds.find(acmd->name());
    if (cmdit!=_cmds.end()) {
        TRACE( TRACE_ALWAYS, "Cmd (%s) already registered\n", acmd->name().c_str());
        return (0);
    }
    else {
        TRACE( TRACE_DEBUG, "Registering cmd (%s)\n", acmd->name().c_str());        
        _cmds[acmd->name()] = acmd;
    }

    // register aliases
    int regs=0; // counts aliases registered
    vector<string>* apl = acmd->aliases();
    assert (apl);
    for (vector<string>::iterator alit = apl->begin(); alit != apl->end(); ++alit) {
        cmdit = _aliases.find(*alit);
        if (cmdit!=_aliases.end()) {
            TRACE( TRACE_ALWAYS, "Alias (%s) already registered\n", (*alit).c_str());
        }
        else {
            TRACE( TRACE_DEBUG, "Registering alias (%s)\n", (*alit).c_str());
            _aliases[*alit]=acmd;
            ++regs;
        }
    }
    assert (regs); // at least one alias should be registered
    return (0);
}

       
const int shell_t::init_cmds()
{
    CRITICAL_SECTION(sh_cs,_lock);
    for (cmdMapIt it = _cmds.begin(); it != _cmds.end(); ++it) {
        it->second->init();
    }
    return (0);
}

const int shell_t::close_cmds()
{
    CRITICAL_SECTION(sh_cs,_lock);
    for (cmdMapIt it = _cmds.begin(); it != _cmds.end(); ++it) {
        it->second->close();
    }
    return (0);
}


/*********************************************************************
 *
 *  @brief: Few basic commands
 *
 *********************************************************************/


const int shell_t::_register_commands() 
{
    // add own

    _tracer = new trace_cmd_t();        
    _tracer->setaliases();
    add_cmd(_tracer.get());

    _cpustater = new cpustat_cmd_t();        
    _cpustater->setaliases();
    add_cmd(_cpustater.get());

    _confer = new conf_cmd_t();        
    _confer->setaliases();
    add_cmd(_confer.get());

    _enver = new env_cmd_t();        
    _enver->setaliases();
    add_cmd(_enver.get());

    _seter = new set_cmd_t();        
    _seter->setaliases();
    add_cmd(_seter.get());

    _helper = new help_cmd_t(&_cmds);        
    _helper->setaliases();
    add_cmd(_helper.get());

    _quiter = new quit_cmd_t();        
    _quiter->setaliases();
    add_cmd(_quiter.get());

    _echoer = new echo_cmd_t();        
    _echoer->setaliases();
    add_cmd(_echoer.get());

    _breaker = new break_cmd_t();        
    _breaker->setaliases();
    add_cmd(_breaker.get());


    return (0);
}





/*********************************************************************
 *
 *  QUIT
 *
 *********************************************************************/

void quit_cmd_t::setaliases() 
{
    _name = string("quit");
    _aliases.push_back("quit");
    _aliases.push_back("q");
    _aliases.push_back("exit");
}


/*********************************************************************
 *
 *  HELP
 *
 *********************************************************************/

void help_cmd_t::setaliases() 
{
    _name = string("help");
    _aliases.push_back("help");
    _aliases.push_back("h");
}


void help_cmd_t::list_cmds()
{
    TRACE( TRACE_ALWAYS, "Available commands (help <cmd>): \n\n");
    for (cmdMapIt it = _pcmds->begin(); it != _pcmds->end(); ++it) {
        TRACE( TRACE_ALWAYS, " %s - %s\n", it->first.c_str(), it->second->desc().c_str());
    }
}

const int help_cmd_t::handle(const char* cmd) 
{
    char help_tag[SERVER_COMMAND_BUFFER_SIZE];
    char cmd_tag[SERVER_COMMAND_BUFFER_SIZE];    
    if ( sscanf(cmd, "%s %s", &help_tag, &cmd_tag) < 2) {
        // prints the list of commands
        list_cmds();
        return (SHELL_NEXT_CONTINUE);
    }
    // otherwise prints usage of a specific command
    cmdMapIt it = _pcmds->find(cmd_tag);
    if (it==_pcmds->end()) {
        TRACE( TRACE_ALWAYS,"Cmd (%s) not found\n", cmd_tag);
        return (SHELL_NEXT_CONTINUE);
    }
    it->second->usage();
    return (SHELL_NEXT_CONTINUE);
}


/*********************************************************************
 *
 *  @brief: Shell environment variables related functions 
 *
 *********************************************************************/


/*********************************************************************
 *
 *  SET
 *
 *********************************************************************/

void set_cmd_t::setaliases() 
{    
    _name = string("set");
    _aliases.push_back("set");
    _aliases.push_back("s");
}

const int set_cmd_t::handle(const char* cmd) 
{
    assert (ev);
    ev->parseSetReq(cmd);
    return (SHELL_NEXT_CONTINUE);
}

void set_cmd_t::usage(void)
{
    TRACE( TRACE_ALWAYS, "SET Usage:\n\n"                               \
           "*** set [<PARAM_NAME=PARAM_VALUE>*]\n"                      \
           "\nParameters:\n"                                            \
           "<PARAM_NAME>  : The name of the environment variable to set\n" \
           "<PARAM_VALUE> : The new value of the env variable\n\n");
}


/*********************************************************************
 *
 *  ENV
 *
 *********************************************************************/

void env_cmd_t::setaliases() 
{    
    _name = string("env");
    _aliases.push_back("env");
    _aliases.push_back("e");
}

const int env_cmd_t::handle(const char* cmd)
{    
    assert (ev);
    char cmd_tag[SERVER_COMMAND_BUFFER_SIZE];
    char env_tag[SERVER_COMMAND_BUFFER_SIZE];    
    if ( sscanf(cmd, "%s %s", &cmd_tag, &env_tag) < 2) {
        // prints all the env
        ev->printVars();    
        return (SHELL_NEXT_CONTINUE);
    }
    ev->checkVar(env_tag);
    return (SHELL_NEXT_CONTINUE);
}

void env_cmd_t::usage(void)
{
    TRACE( TRACE_ALWAYS, "ENV Usage:\n\n"                               \
           "*** env [PARAM]\n"                      \
           "\nParameters:\n"                                            \
           "env         - Print all the environment variables\n" \
           "env <PARAM> - Print the value of a specific env variable\n\n");
}


/*********************************************************************
 *
 *  CONF
 *
 *********************************************************************/

void conf_cmd_t::setaliases() 
{    
    _name = string("conf");
    _aliases.push_back("conf");
    _aliases.push_back("c");
}

const int conf_cmd_t::handle(const char* cmd)
{    
    ev->refreshVars();
    return (SHELL_NEXT_CONTINUE);
}

void conf_cmd_t::usage(void)
{
    TRACE( TRACE_ALWAYS, "CONF - Tries to reread all the set env vars from the config file\n");
}


/*********************************************************************
 *
 *  CPUSTAT
 *
 *********************************************************************/

void cpustat_cmd_t::setaliases() 
{    
    _name = string("cpu");
    _aliases.push_back("cpu");
    _aliases.push_back("cpustats");
}

const int cpustat_cmd_t::handle(const char* cmd)
{    
    myinfo.print();
    return (SHELL_NEXT_CONTINUE);
}

void cpustat_cmd_t::usage(void)
{
    TRACE( TRACE_ALWAYS, "CPUSTAT - Prints cpu usage for the process\n");
}
