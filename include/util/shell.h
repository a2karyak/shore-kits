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


#include <iostream>
#include <boost/tokenizer.hpp>

#include <map>

#include <readline/readline.h>
#include <readline/history.h>
#include <assert.h>
#include <signal.h>
#include <errno.h>

#include "util.h"


using namespace std;



/*********************************************************************
 *
 *  @class: envVar
 *
 *  @brief: Encapsulates the "environment" variables functionality.
 *          It does two things. First, it has the functions that parse
 *          a config file. Second, it stores all the parsed params to
 *          a map of <string,string>. The params may be either read from
 *          the config file or set at runtime.
 *
 *  @note:  Singleton
 *
 *  @usage: - Get instance
 *          - Call setVar()/getVar() for setting/getting a specific variable.
 *          - Call readConfVar() to parse the conf file for a specific variable.
 *            The read value will be stored at the map.
 *          - Call parseSetReq() for parsing and setting a set of params
 *
 *********************************************************************/

const string ENVCONFFILE = "shore.conf";

class envVar 
{
private:

    typedef map<string,string>        envVarMap;
    typedef envVarMap::iterator       envVarIt;
    typedef envVarMap::const_iterator envVarConstIt;

    typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
    typedef tokenizer::iterator  tokit;

    envVarMap _evm;
    string _cfname;
    mcs_lock _lock;
    guard<ConfigFile> _pfparser;

    envVar(const string sConfFile=ENVCONFFILE) 
        : _cfname(sConfFile)
    { 
        assert (!_cfname.empty());
        _pfparser = new ConfigFile(_cfname);
        assert (_pfparser);
    }
    ~envVar() { }

    // Helpers

    template <class T>
    string _toString(const T& arg)
    {
        ostringstream out;
        out << arg;
        return (out.str());
    }

    // reads the conf file for a specific param
    // !!! the caller should have the lock !!!
    string _readConfVar(const string& sParam, const string& sDefValue); 
    
public:

    static envVar* instance() { static envVar _instance; return (&_instance); }

    // refreshes all the env vars from the conf file
    const int refreshVars(void);

    // sets a new parameter
    const int setVar(const string& sParam, const string& sValue);
    const int setVarInt(const string& sParam, const int& iValue);

    // retrieves a specific param from the map. if not found, searches the conf file
    // and updates the map
    // @note: after this call the map will have an entry about sParam
    string getVar(const string& sParam, const string& sDefValue);  
    int getVarInt(const string& sParam, const int& iDefValue);  

    // checks if a specific param is set at the map, or, if not at the map, at the conf file
    // @note: this call does not update the map 
    void checkVar(const string& sParam);      

    // sets as input another conf file
    void setConfFile(const string& sConfFile);

    // prints all the env vars
    void printVars(void);

    // parses a SET request
    const int parseOneSetReq(const string& in);
    
    // parses a string of SET requests
    const int parseSetReq(const string& in);

}; // envVar



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
    virtual int process_command(const char* cmd, const char* cmd_tag)=0;
    virtual int print_usage(const char* cmd)=0;
    virtual int SIGINT_handler() { return (ECANCELED); /* exit immediately */ }

    int process_one();
    bool check_quit(const char* command);
    bool check_help(const char* command);

    // shell environment variables
    bool check_set(const char* command);
    void usage_set(void) const;
    int  parse_set(const char* command);
    bool check_env(const char* command);
    int  serve_env(const char* command);
    bool check_reconf(const char* command);
    int  reconf(void);
    
    const int get_command_cnt() { return (_cmd_counter); }

    int start();

}; // EOF: shell_t



#endif /* __UTIL_SHELL_H */

