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
 *  @fn:    setVar/getVar/readConfVar
 *  
 *  @brief: Environment variables manipulation
 *          
 *********************************************************************/


// helper: reads the conf file for a specific param
// !!! the caller should have the lock !!!
string envVar::_readConfVar(const string& sParam, const string& sDefValue)
{
    if (sParam.empty()||sDefValue.empty()) {
        TRACE( TRACE_ALWAYS, "Invalid Param or Value input\n");
        return ("");
    }
    assert (_pfparser);
    string tmp;
    // probe config file
    _pfparser->readInto(tmp,sParam,sDefValue); 
    // set entry in the env map
    _evm[sParam] = tmp;
    TRACE( TRACE_DEBUG, "(%s) (%s)\n", sParam.c_str(), tmp.c_str());
    return (tmp);
}

// sets a new parameter
const int envVar::setVar(const string& sParam, const string& sValue)
{
    if ((!sParam.empty())&&(!sValue.empty())) {
        TRACE( TRACE_DEBUG, "(%s) (%s)\n", sParam.c_str(), sValue.c_str());
        CRITICAL_SECTION(evm_cs,_lock);
        _evm[sParam] = sValue;
        return (_evm.size());
    }
    return (0);
}

const int envVar::setVarInt(const string& sParam, const int& iValue)
{    
    return (setVar(sParam,_toString(iValue)));
}


// refreshes all the env vars from the conf file
const int envVar::refreshVars(void)
{
    TRACE( TRACE_DEBUG, "Refreshing environment variables\n");
    CRITICAL_SECTION(evm_cs,_lock);
    for (envVarIt it= _evm.begin(); it != _evm.end(); ++it)
        _readConfVar(it->first,it->second);    
    return (0);
}


// checks the map for a specific param
// if it doesn't find it checks also the config file
string envVar::getVar(const string& sParam, const string& sDefValue)
{
    if (sParam.empty()) {
        TRACE( TRACE_ALWAYS, "Invalid Param input\n");
        return ("");
    }

    CRITICAL_SECTION(evm_cs,_lock);
    envVarIt it = _evm.find(sParam);
    if (it==_evm.end()) {        
        //TRACE( TRACE_DEBUG, "(%s) param not set. Searching conf\n", sParam.c_str()); 
        return (_readConfVar(sParam,sDefValue));
    }
    return (it->second);
}

int envVar::getVarInt(const string& sParam, const int& iDefValue)
{
    return (atoi(getVar(sParam,_toString(iDefValue)).c_str()));
}

// checks if a specific param is set at the map or (fallback) the conf file
void envVar::checkVar(const string& sParam)
{
    string r;
    CRITICAL_SECTION(evm_cs,_lock);
    // first searches the map
    envVarIt it = _evm.find(sParam);
    if (it!=_evm.end()) {
        r = it->second + " (map)";
    }
    else {
        // if not found on map, searches the conf file
        if (_pfparser->keyExists(sParam)) {
            _pfparser->readInto(r,sParam,string("Not found"));
            r = r + " (conf)";
        }
        else {
            r = string("Not found");
        }        
    }
    TRACE( TRACE_ALWAYS, "%s -> %s\n", sParam.c_str(), r.c_str()); 
}


// prints all the env vars
void envVar::printVars(void)
{
    TRACE( TRACE_DEBUG, "Environment variables\n");
    CRITICAL_SECTION(evm_cs,_lock);
    for (envVarConstIt cit= _evm.begin(); cit != _evm.end(); ++cit)
        TRACE( TRACE_DEBUG, "%s -> %s\n", cit->first.c_str(), cit->second.c_str()); 
}

// sets as input another conf file
void envVar::setConfFile(const string& sConfFile)
{
    assert (!sConfFile.empty());
    CRITICAL_SECTION(evm_cs,_lock);
    _cfname = sConfFile;
    _pfparser = new ConfigFile(_cfname);
    assert (_pfparser);
}

// parses a SET request
const int envVar::parseOneSetReq(const string& in)
{
    string param;
    string value;
    boost::char_separator<char> valuesep("=");            

    tokenizer valuetok(in, valuesep);  
    tokit vit = valuetok.begin();
    if (vit == valuetok.end()) {
        TRACE( TRACE_DEBUG, "(%s) is malformed\n", in);
        return(1);
    }
    param = *vit;
    ++vit;
    if (vit == valuetok.end()) {
        TRACE( TRACE_DEBUG, "(%s) is malformed\n", in);
        return(2);
    }
    value = *vit;
    TRACE( TRACE_DEBUG, "%s -> %s\n", param.c_str(), value.c_str()); 
    CRITICAL_SECTION(evm_cs,_lock);
    _evm[param] = value;
    return(0);
}
    
// parses a string of (multiple) SET requests
const int envVar::parseSetReq(const string& in)
{
    int cnt=0;

    // FORMAT: SET [<clause_name>=<clause_value>]*
    boost::char_separator<char> clausesep(" ");
    tokenizer clausetok(in, clausesep);
    tokit cit = clausetok.begin();
    ++cit; // omit the SET cmd
    for (; cit != clausetok.end(); ++cit) {
        parseOneSetReq(*cit);
        ++cnt;
    }
    return (cnt);
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


    char command_tag[SERVER_COMMAND_BUFFER_SIZE];

    if ( sscanf(command, "%s", &command_tag) < 1) {
        print_usage(command_tag);
        return (SHELL_NEXT_CONTINUE);
    }
        
    // history control
    if (*command) {
        // non-empty line...
        add_history(command);
    }
        
    // check for quit...
    if (check_quit(&command_tag[0])) {
        // quit command! 
        return (SHELL_NEXT_QUIT);
    }

    // check for help...
    if (check_help(&command_tag[0])) {
        // help command! 
        return (print_usage(&command_tag[0]));
    }

    // check for set...
    if (check_set(&command_tag[0])) {
        // set command! 
        return (parse_set(command));
    }

    // check for reconf...
    if (check_reconf(&command_tag[0])) {
        // set command! 
        return (reconf());
    }

    // check for env...
    if (check_env(&command_tag[0])) {
        // env command! 
        return (serve_env(command));
    }

    // increase stats
    _cmd_counter++;

    _processing_command = true;
    int rval = process_command(command, command_tag);
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



/*********************************************************************
 *
 *  @fn:    {usage,check,parse,print}_{set,env}
 *  
 *  @brief: Shell environment variables related functions 
 *
 *********************************************************************/

bool shell_t::check_set(const char* command) 
{    
    if (( strcasecmp(command, "set") == 0 ) ||
        ( strcasecmp(command, "set;") == 0 ) ||
        ( strcasecmp(command, "s") == 0 ) ||
        ( strcasecmp(command, "s;") == 0 ) )
        // set command!
        return (true);        
    return (false);
}


void shell_t::usage_set(void) const
{
    TRACE( TRACE_ALWAYS, "SET Usage:\n\n"                               \
           "*** set [<PARAM_NAME=PARAM_VALUE>*]\n"                      \
           "\nParameters:\n"                                            \
           "<PARAM_NAME>  : The name of the environment variable to set\n" \
           "<PARAM_VALUE> : The new value of the env variable\n\n");
}


int shell_t::parse_set(const char* command) 
{
    envVar::instance()->parseSetReq(command);
    return (SHELL_NEXT_CONTINUE);
}


bool shell_t::check_env(const char* command) 
{    
    if (( strcasecmp(command, "env") == 0 ) ||
        ( strcasecmp(command, "env;") == 0 ) ||
        ( strcasecmp(command, "e") == 0 ) ||
        ( strcasecmp(command, "e;") == 0 ) )
        // set command!
        return (true);        
    return (false);
}

int shell_t::serve_env(const char* command)
{    
    char cmd_tag[SERVER_COMMAND_BUFFER_SIZE];
    char env_tag[SERVER_COMMAND_BUFFER_SIZE];    
    if ( sscanf(command, "%s %s", &cmd_tag, &env_tag) < 2) {
        // prints all the env
        envVar::instance()->printVars();    
        return (SHELL_NEXT_CONTINUE);
    }
    envVar::instance()->checkVar(env_tag);
    return (SHELL_NEXT_CONTINUE);
}


bool shell_t::check_reconf(const char* command) 
{    
    if (( strcasecmp(command, "reconf") == 0 ) ||
        ( strcasecmp(command, "r;") == 0 ) ||
        ( strcasecmp(command, "conf") == 0 ) ||
        ( strcasecmp(command, "c;") == 0 ) )
        // set command!
        return (true);        
    return (false);
}

int shell_t::reconf(void)
{    
    envVar::instance()->refreshVars();
    return (SHELL_NEXT_CONTINUE);
}
