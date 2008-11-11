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
#include "stages/tpcc/shore/shore_tpcc_env.h" // TODO (ip) It should not get the specific TPCC Shore Env


using std::map;


extern "C" void alarm_handler(int sig);

extern bool volatile _g_canceled;




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

    ShoreTPCCEnv* _env;
    processorid_t _start_prs_id;
    processorid_t _current_prs_id;    

    // supported trxs
    typedef map<int,string>                     mapSupTrxs;
    typedef mapSupTrxs::iterator                mapSupTrxsIt;
    typedef mapSupTrxs::const_iterator          mapSupTrxsConstIt;
    mapSupTrxs _sup_trxs;

    // supported binding policies
    typedef map<eBindingType,string>            mapBindPols;
    typedef mapBindPols::iterator               mapBindPolsIt;    
    mapBindPols _sup_bps;

public:

    shore_kit_shell_t(const char* prompt, ShoreTPCCEnv* env, 
                      processorid_t acpustart = PBIND_NONE) 
        : shell_t(prompt), _env(env),
          _start_prs_id(acpustart), _current_prs_id(acpustart)
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

    virtual ~shore_kit_shell_t()  { }

    // shell interface
    virtual int process_command(const char* command);
    virtual int print_usage(const char* command);
    virtual int SIGINT_handler();

    // supported commands and their usage
    virtual int process_cmd_TEST(const char* command, char* command_tag);
    virtual int process_cmd_MEASURE(const char* command, char* command_tag);
    virtual int process_cmd_WARMUP(const char* command, char* command_tag);    
    virtual int process_cmd_LOAD(const char* command, char* command_tag);        
    virtual int process_cmd_TRXS(const char* command, char* command_tag); 

    virtual void usage_cmd_TEST();
    virtual void usage_cmd_MEASURE();
    virtual void usage_cmd_WARMUP();    
    virtual void usage_cmd_LOAD();    


    // supported trxs and binding policies
    void print_sup_trxs(void) const;
    void print_sup_bp(void);
    void load_trxs_map(void);
    void load_bp_map(void);
    virtual void append_trxs_map(void)=0;
    virtual void append_bp_map(void)=0;
    const char* translate_trx(const int iSelectedTrx) const;
    const char* translate_bp(const eBindingType abt);


    // virtual implementation of the {WARMUP/TEST/MEASURE} 
    // WARMUP/LOAD are virtual
    // TEST/MEASURE are pure virtual
    virtual int _cmd_WARMUP_impl(const int iQueriedWHs, const int iTrxs, 
                                 const int iDuration, const int iIterations);
    virtual int _cmd_LOAD_impl(void);

    virtual int _cmd_TEST_impl(const int iQueriedWHs, const int iSpread,
                               const int iNumOfThreads, const int iNumOfTrxs,
                               const int iSelectedTrx, const int iIterations,
                               const int iUseSLI, const eBindingType abt)=0;
    virtual int _cmd_MEASURE_impl(const int iQueriedWHs, const int iSpread,
                                  const int iNumOfThreads, const int iDuration,
                                  const int iSelectedTrx, const int iIterations,
                                  const int iUseSLI, const eBindingType abt)=0;    

    // for the client processor binding policy
    virtual processorid_t next_cpu(const eBindingType abt,
                                   const processorid_t aprd);


protected:

    void print_throughput(const int iQueriedWHs, const int iSpread, 
                          const int iNumOfThreads, const int iUseSLI, 
                          const double delay, const eBindingType abt);
    
    void print_MEASURE_info(const int iQueriedWHs, const int iSpread, 
                            const int iNumOfThreads, const int iDuration,
                            const int iSelectedTrx, const int iIterations,
                            const int iUseSLI, const eBindingType abt);

    void print_TEST_info(const int iQueriedWHs, const int iSpread, 
                         const int iNumOfThreads, const int iNumOfTrxs,
                         const int iSelectedTrx, const int iIterations,
                         const int iUseSLI, const eBindingType abt);

}; // EOF: shore_kit_shell_t



#endif /* __TESTER_SHORE_KIT_SHELL_H */

