/* -*- mode:C++; c-basic-offset:4 -*- */
// CC -m64 -xarch=ultraT1 -xs -g -I $SHORE_INCLUDE_DIR shore_tpcc_kit_shell.cpp -o shore_tpcc_load -L $SHORE_LIB_DIR -mt -lsm -lsthread -lfc -lcommon -lpthread

/** @file:   shore_tpcc_kit_shell.cpp
 *
 *  @brief:  Test shore tpc-c kit with shell
 *
 *  @author: Ippokratis Pandis, July 2008
 */

#include "tests/common.h"
#include "stages/tpcc/shore/shore_tpcc_env.h"
#include "sm/shore/shore_helper_loader.h"

#include "util/shell.h"


using namespace shore;
using namespace tpcc;


// Default parameters for the runs

//////////////////////////////


class tpcc_kit_shell_t : public shore_kit_shell_t 
{ 
public:

    tpcc_kit_shell_t(const char* prompt) 
        : shore_kit_shell_t(prompt, _g_shore_env)
    {
        // load supported trxs and binding policies maps
        load_trxs_map();
        load_bp_map();
    }
    ~tpcc_kit_shell_t() { }

    // impl of supported commands
    virtual int _cmd_TEST_impl(const int iQueriedWHs, const int iSpread,
                               const int iNumOfThreads, const int iNumOfTrxs,
                               const int iSelectedTrx, const int iIterations,
                               const int iUseSLI, const eBindingType abt);
    virtual int _cmd_MEASURE_impl(const int iQueriedWHs, const int iSpread,
                                  const int iNumOfThreads, const int iDuration,
                                  const int iSelectedTrx, const int iIterations,
                                  const int iUseSLI, const eBindingType abt);    

    virtual void append_trxs_map(void) { }
    virtual void append_bp_map(void) { }

}; // EOF: tpcc_kit_shell_t

/** cmd: TEST **/

int tpcc_kit_shell_t::_cmd_TEST_impl(const int iQueriedWHs, 
                                     const int iSpread,
                                     const int iNumOfThreads, 
                                     const int iNumOfTrxs,
                                     const int iSelectedTrx, 
                                     const int iIterations,
                                     const int iUseSLI,
                                     const eBindingType abt)
{
    assert (_env->is_initialized());    

    // print test information
    print_TEST_info(iQueriedWHs, iSpread, iNumOfThreads, 
                    iNumOfTrxs, iSelectedTrx, iIterations, iUseSLI, abt);

    client_smt_t* testers[MAX_NUM_OF_THR];
    for (int j=0; j<iIterations; j++) {

        TRACE( TRACE_ALWAYS, "Iteration [%d of %d]\n",
               (j+1), iIterations);

        // reset starting cpu and wh id
        _current_prs_id = _start_prs_id;
        int wh_id = 0;

        // set measurement state to measure - start counting everything
        _env->set_measure(MST_MEASURE);
	stopwatch_t timer;

        for (int i=0; i<iNumOfThreads; i++) {
            // create & fork testing threads
            if (iSpread)
                wh_id = i+1;
            testers[i] = new client_smt_t(_env, MT_NUM_OF_TRXS,
                                          wh_id, iSelectedTrx, 
                                          iNumOfTrxs, iUseSLI,
                                          c_str("tpcc%d", i),
                                          _current_prs_id);
            testers[i]->fork();
            _current_prs_id = next_cpu(abt, _current_prs_id);
        }

        /* 2. join the tester threads */
        for (int i=0; i<iNumOfThreads; i++) {
            testers[i]->join();
            if (testers[i]->_rv) {
                TRACE( TRACE_ALWAYS, "Error in testing...\n");
                TRACE( TRACE_ALWAYS, "Exiting...\n");
                assert (false);
            }    
            delete (testers[i]);
        }

	double delay = timer.time();

        // print throughput and reset session stats
        print_throughput(iQueriedWHs, iSpread, iNumOfThreads, iUseSLI, delay, abt);
        _env->reset_session_tpcc_stats();
    }

    // set measurement state
    _env->set_measure(MST_DONE);
    return (SHELL_NEXT_CONTINUE);
}


/** cmd: MEASURE **/

int tpcc_kit_shell_t::_cmd_MEASURE_impl(const int iQueriedWHs, 
                                        const int iSpread,
                                        const int iNumOfThreads, 
                                        const int iDuration,
                                        const int iSelectedTrx, 
                                        const int iIterations,
                                        const int iUseSLI,
                                        const eBindingType abt)
{
    assert (_env->is_initialized());    
    // print measurement info
    print_MEASURE_info(iQueriedWHs, iSpread, iNumOfThreads, iDuration, 
                       iSelectedTrx, iIterations, iUseSLI, abt);

    // create and fork client threads
    client_smt_t* testers[MAX_NUM_OF_THR];
    for (int j=0; j<iIterations; j++) {

        TRACE( TRACE_ALWAYS, "Iteration [%d of %d]\n",
               (j+1), iIterations);

        // reset starting cpu and wh id
        _current_prs_id = _start_prs_id;
        int wh_id = 0;

        // set measurement state
        _env->set_measure(MST_WARMUP);

        // create threads
        for (int i=0; i<iNumOfThreads; i++) {
            // create & fork testing threads
            if (iSpread)
                wh_id = i+1;
            testers[i] = new client_smt_t(_env, MT_TIME_DUR,
                                          wh_id, iSelectedTrx, 
                                          0, iUseSLI,
                                          c_str("tpcc-cl-%d", i),
                                          _current_prs_id);

            assert (testers[i]);
            testers[i]->fork();
            _current_prs_id = next_cpu(abt, _current_prs_id);
        }

        // TODO: give them some time (2secs) to start-up
        sleep(DF_WARMUP_INTERVAL);

        // set measurement state
        _env->set_measure(MST_MEASURE);
	alarm(iDuration);
	stopwatch_t timer;

        /* 2. join the tester threads */
        for (int i=0; i<iNumOfThreads; i++) {
            testers[i]->join();
            if (testers[i]->_rv) {
                TRACE( TRACE_ALWAYS, "Error in testing...\n");
                TRACE( TRACE_ALWAYS, "Exiting...\n");
                assert (false);
            }    
            assert (testers[i]);
            delete (testers[i]);
        }

	double delay = timer.time();
	alarm(0); // cancel the alarm, if any

        // print throughput and reset session stats
        print_throughput(iQueriedWHs, iSpread, iNumOfThreads, iUseSLI, delay, abt);
        _env->reset_session_tpcc_stats();
    }
    // set measurement state
    _env->set_measure(MST_DONE);

    return (SHELL_NEXT_CONTINUE);
}

/** EOF: tpcc_kit_shell_t functions */


//////////////////////////////


//////////////////////////////

int main(int argc, char* argv[]) 
{
    // initialize cordoba threads
    thread_init();

    TRACE_SET( TRACE_ALWAYS | TRACE_STATISTICS | TRACE_NETWORK | TRACE_CPU_BINDING
               //              | TRACE_QUERY_RESULTS
               //              | TRACE_PACKET_FLOW
               //               | TRACE_RECORD_FLOW
               //| TRACE_TRX_FLOW
               //| TRACE_DEBUG
              );

    /* 1. Instanciate the Shore environment */
    if (inst_test_env(argc, argv))
        return (1);

    /* 2. Make sure that the correct schema is used */
    if (_g_shore_env->get_sysname().compare("dora")==0) {
        TRACE( TRACE_ALWAYS, "Incorrect schema at configuration file\nExiting...\n");
        return (1);
    }

    /* 3. Make sure data is loaded */
    w_rc_t rcl = _g_shore_env->loaddata();
    if (rcl.is_error()) {
        return (SHELL_NEXT_QUIT);
    }

    /* 4. Start processing commands */
    tpcc_kit_shell_t kit_shell("(tpcckit) ");
    kit_shell.start();

    /* 5. Close the Shore environment */
    if (_g_shore_env)
        close_test_env();

    return (0);
}
