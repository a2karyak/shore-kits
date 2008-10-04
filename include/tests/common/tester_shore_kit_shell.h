/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   tester_shore_kit_shell.h
 *
 *  @brief:  Abstract shell class for testing Shore environments 
 *
 *  @author: Ippokratis Pandis (ipandis), Sept 2008
 *
 */

#ifndef __TESTER_SHORE_KIT_SHELL_H
#define __TESTER_SHORE_KIT_SHELL_H

#ifdef __SUNPRO_CC
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#else
#include <cstdlib>
#include <cstdio>
#include <cstring>
#endif

#include "util/shell.h"
#include "tests/common/tester_shore_env.h"
#include "sm/shore/shore_env.h"


extern "C" void alarm_handler(int sig);

extern bool volatile _g_canceled;



/******************************************************************** 
 *
 * @enum:  DBControl
 *
 * @brief: States of a controlled DB
 *
 ********************************************************************/

enum DBControl { DBC_UNDEF, DBC_PAUSED, DBC_ACTIVE, DBC_STOPPED };


/*********************************************************************
 *
 *  @abstract class: db_iface
 *
 *  @brief:          Interface of basic shell commands for dbs
 *
 *  @usage:          - Inherit from this class
 *                   - Implement the process_{START/STOP/PAUSE/RESUME} fuctions
 *
 *********************************************************************/

class db_iface
{
private:

    DBControl  _dbc;
    //tatas_lock _dbc_lock;

public:

    db_iface() { }

    virtual ~db_iface() { }


    // Access methods

    const DBControl dbc() const { 
        //CRITICAL_SECTION(dbc_cs, _dbc_lock);
        return(_dbc); 
    }
    
    void set_dbc(const DBControl adbc) {
        assert (adbc!=DBC_UNDEF);
        //CRITICAL_SECTION(dbc_cs, _dbc_lock);
        _dbc = adbc;
    }

    
    // Interface 
    
    virtual const int process_START_cmd()=0;
    virtual const int process_STOP_cmd()=0;
    virtual const int process_PAUSE_cmd()=0;
    virtual const int process_RESUME_cmd()=0;
    
    
}; // EOF: db_iface



/*********************************************************************
 *
 *  @abstract class: shore_kit_shell_t
 *
 *  @brief: Base class for shells for Shore environment
 *
 *  @usage: - Inherit from this class
 *          - Implement the process_{TEST/MEASURE/WARMUP} fuctions
 *          - Call the start() function
 *
 *
 *  @note:  Supported commands - { TEST/MEASURE/WARMUP/LOAD }
 *  @note:  To add new command function process_command() should be overridden 
 *  @note:  SIGINT handling
 *
 *********************************************************************/

class shore_kit_shell_t : public shell_t 
{
protected:

    ShoreEnv* _env;

public:

    shore_kit_shell_t(const char* prompt, ShoreEnv* env) 
        : shell_t(prompt), _env(env)
        {
            assert (_env);

            // install signal handler
	    struct sigaction sa;
	    struct sigaction sa_old;
	    sa.sa_flags = 0;
	    sigemptyset(&sa.sa_mask);
	    sa.sa_handler = &alarm_handler;
	    if(sigaction(SIGALRM, &sa, &sa_old) < 0)
		exit(1);
        }

    virtual ~shore_kit_shell_t() 
    { 
    }

    // shell interface
    virtual int process_command(const char* command);
    virtual int print_usage(const char* command);
    virtual int SIGINT_handler();

    // supported commands and their usage
    virtual int process_cmd_TEST(const char* command, char* command_tag);
    virtual int process_cmd_MEASURE(const char* command, char* command_tag);
    virtual int process_cmd_WARMUP(const char* command, char* command_tag);    
    virtual int process_cmd_LOAD(const char* command, char* command_tag);        

    virtual void usage_cmd_TEST();
    virtual void usage_cmd_MEASURE();
    virtual void usage_cmd_WARMUP();    
    virtual void usage_cmd_LOAD();    

    // virtual implementation of the {WARMUP/TEST/MEASURE} 
    // WARMUP/LOAD are virtual
    // TEST/MEASURE are pure virtual
    virtual int _cmd_WARMUP_impl(const int iQueriedWHs, const int iTrxs, 
                                 const int iDuration, const int iIterations);
    virtual int _cmd_LOAD_impl();


    virtual int _cmd_TEST_impl(const int iQueriedWHs, const int iSpread,
                               const int iNumOfThreads, const int iNumOfTrxs,
                               const int iSelectedTrx, const int iIterations,
                               const int iUseSLI)=0;
    virtual int _cmd_MEASURE_impl(const int iQueriedWHs, const int iSpread,
                                  const int iNumOfThreads, const int iDuration,
                                  const int iSelectedTrx, const int iIterations,
                                  const int iUseSLI)=0;    

}; // EOF: shore_kit_shell_t



#endif /* __TESTER_SHORE_KIT_SHELL_H */

