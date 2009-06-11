/* -*- mode:C++; c-basic-offset:4 -*-
     Shore-kits -- Benchmark implementations for Shore-MT
   
                       Copyright (c) 2007-2009
               Ecole Polytechnique Federale de Lausanne

                       Copyright (c) 2007-2008
                      Carnegie-Mellon University

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
/** @file:   shore_shell.h
 *
 *  @brief:  Abstract shell class for Shore environments 
 *
 *  @author: Ippokratis Pandis, Sept 2008
 */

#ifndef __SHORE_SHELL_H
#define __SHORE_SHELL_H

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
#include "sm/shore/shore_env.h"
#include "sm/shore/shore_helper_loader.h"
#include "sm/shore/shore_client.h"


ENTER_NAMESPACE(shore);

using std::map;


extern "C" void alarm_handler(int sig);

extern bool volatile _g_canceled;



// default database size (scaling factor)
const int DF_SF            = 10;
extern int _theSF;

// Default values for the power-runs //

// default queried factor
const int DF_NUM_OF_QUERIED_SF    = 10;

// default transaction id to be executed
const int DF_TRX_ID                = -1;

// Default values for the warmups //

// default number of queried SF during warmup
const int DF_WARMUP_QUERIED_SF = 10;



/*********************************************************************
 *
 *  @class: fake_io_delay_cmd_t
 *
 *  @brief: Handler for the "iodelay" command
 *
 *********************************************************************/

// cmd - IODELAY
struct fake_iodelay_cmd_t : public command_handler_t {
    ShoreEnv* _env;
    fake_iodelay_cmd_t(ShoreEnv* env) : _env(env) { };
    ~fake_iodelay_cmd_t() { }
    void setaliases() { 
        _name = string("iodelay"); 
        _aliases.push_back("iodelay"); _aliases.push_back("io"); }
    const int handle(const char* cmd);
    void usage();
    const string desc() { return (string("Sets the fake I/O disk delay")); }
};






/*********************************************************************
 *
 *  @abstract class: shore_shell_t
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

class shore_shell_t : public shell_t 
{
protected:

    ShoreEnv* _env;

    // supported cmds
    guard<fake_iodelay_cmd_t> _fakeioer;   


    // supported trxs
    typedef map<int,string>            mapSupTrxs;
    typedef mapSupTrxs::iterator       mapSupTrxsIt;
    typedef mapSupTrxs::const_iterator mapSupTrxsConstIt;

    mapSupTrxs _sup_trxs;

public:

    shore_shell_t(const char* prompt) 
        : shell_t(prompt), _env(NULL)
    {
        // install signal handler
        struct sigaction sa;
        struct sigaction sa_old;
        sa.sa_flags = 0;
        sigemptyset(&sa.sa_mask);
        sa.sa_handler = &alarm_handler;
        if(sigaction(SIGALRM, &sa, &sa_old) < 0)
            exit(1);        
    }

    virtual ~shore_shell_t() 
    { 
        if (_env) {
            _env->stop();
            close_smt_t* clt = new close_smt_t(_env, c_str("clt"));
            assert (clt);
            clt->fork(); // pointer is deleted by clt thread
            clt->join();
            int rv = clt->_rv;
            if (rv) {
                fprintf( stderr, "Error in closing thread...\n");
            }
            delete (clt);
            clt = NULL;
        }
    }

    // access methods
    ShoreEnv* db() { return(_env); }

    // shell interface
    virtual void pre_process_cmd();
    virtual int process_command(const char* command, const char* command_tag);
    virtual int print_usage(const char* command);
    virtual int SIGINT_handler();

    // supported commands and their usage
    virtual int process_cmd_TEST(const char* command, const char* command_tag);
    virtual int process_cmd_MEASURE(const char* command, const char* command_tag);
    virtual int process_cmd_WARMUP(const char* command, const char* command_tag);    
    virtual int process_cmd_LOAD(const char* command, const char* command_tag);        
    virtual int process_cmd_TRXS(const char* command, const char* command_tag); 
    //    virtual int process_cmd_RESTART(const char* command, const char* command_tag)=0;

    virtual void usage_cmd_TEST();
    virtual void usage_cmd_MEASURE();
    virtual void usage_cmd_WARMUP();    
    virtual void usage_cmd_LOAD();    

    virtual const int register_commands()
    {
        return (0);
    }


    // Instanciate and close the Shore environment
    virtual const int inst_test_env(int argc, char* argv[])=0;

    // supported trxs and binding policies
    virtual const int load_trxs_map(void)=0;
    void print_sup_trxs(void) const;
    const char* translate_trx(const int iSelectedTrx) const;


    // virtual implementation of the {WARMUP/TEST/MEASURE} 
    // WARMUP/LOAD are virtual
    // TEST/MEASURE are pure virtual
    virtual int _cmd_WARMUP_impl(const int iQueriedSF, const int iTrxs, 
                                 const int iDuration, const int iIterations);
    virtual int _cmd_LOAD_impl(void);

    virtual int _cmd_TEST_impl(const int iQueriedSF, const int iSpread,
                               const int iNumOfThreads, const int iNumOfTrxs,
                               const int iSelectedTrx, const int iIterations)=0;
    virtual int _cmd_MEASURE_impl(const int iQueriedSF, const int iSpread,
                                  const int iNumOfThreads, const int iDuration,
                                  const int iSelectedTrx, const int iIterations)=0;    


protected:
    
    void print_MEASURE_info(const int iQueriedSF, const int iSpread, 
                            const int iNumOfThreads, const int iDuration,
                            const int iSelectedTrx, const int iIterations);

    void print_TEST_info(const int iQueriedSF, const int iSpread, 
                         const int iNumOfThreads, const int iNumOfTrxs,
                         const int iSelectedTrx, const int iIterations);

}; // EOF: shore_shell_t


EXIT_NAMESPACE(shore);

#endif /* __TESTER_SHORE_SHELL_H */

