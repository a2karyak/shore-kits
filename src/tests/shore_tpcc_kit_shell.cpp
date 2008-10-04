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
private:

    // helper functions
    const char* translate_trx_id(const int trx_id);
 
public:

    tpcc_kit_shell_t(const char* prompt) 
        : shore_kit_shell_t(prompt, _g_shore_env)
        {
        }

    ~tpcc_kit_shell_t() 
    { 
    }

    // impl of supported commands
    virtual int _cmd_TEST_impl(const int iQueriedWHs, const int iSpread,
                               const int iNumOfThreads, const int iNumOfTrxs,
                               const int iSelectedTrx, const int iIterations,
                               const int iUseSLI);
    virtual int _cmd_MEASURE_impl(const int iQueriedWHs, const int iSpread,
                                  const int iNumOfThreads, const int iDuration,
                                  const int iSelectedTrx, const int iIterations,
                                  const int iUseSLI);    

}; // EOF: tpcc_kit_shell_t



/** tpcc_kit_shell_t helper functions */


const char* tpcc_kit_shell_t::translate_trx_id(const int trx_id) 
{
    switch (trx_id) {
    case (XCT_NEW_ORDER):
        return ("NewOrder");
        break;
    case (XCT_PAYMENT):
        return ("Payment");
        break;
    case (XCT_ORDER_STATUS):
        return ("OrderStatus");
        break;
    case (XCT_DELIVERY):
        return ("Delivery");
        break;
    case (XCT_STOCK_LEVEL):
        return ("StockLevel");
        break;
    default:
        return ("Mix");
    }
}



/** tpcc_kit_shell_t functions */


/** cmd: TEST **/


int tpcc_kit_shell_t::_cmd_TEST_impl(const int iQueriedWHs, 
                                     const int iSpread,
                                     const int iNumOfThreads, 
                                     const int iNumOfTrxs,
                                     const int iSelectedTrx, 
                                     const int iIterations,
                                     const int iUseSLI)
{
    assert (_g_shore_env->is_initialized());    

    // Print out configuration
    TRACE( TRACE_ALWAYS, "\n\n" \
           "Queried WHs    : %d\n" \
           "Spread Threads : %s\n" \
           "Num of Threads : %d\n" \
           "Num of Trxs    : %d\n" \
           "Trx            : %s\n" \
           "Iterations     : %d\n" \
           "Use SLI        : %s\n", 
           iQueriedWHs, (iSpread ? "Yes" : "No"), 
           iNumOfThreads, iNumOfTrxs, translate_trx_id(iSelectedTrx),
           iIterations, (iUseSLI ? "Yes" : "No"));

    test_smt_t* testers[MAX_NUM_OF_THR];
    for (int j=0; j<iIterations; j++) {

        TRACE( TRACE_ALWAYS, "Iteration [%d of %d]\n",
               (j+1), iIterations);

        // set measurement state to measure - start counting everything
        _g_shore_env->set_measure(MST_MEASURE);

        int wh_id = 0;
	stopwatch_t timer;

        for (int i=0; i<iNumOfThreads; i++) {
            // create & fork testing threads
            if (iSpread)
                wh_id = i+1;
            testers[i] = new test_smt_t(_g_shore_env, MT_NUM_OF_TRXS,
                                        wh_id, iSelectedTrx, 
                                        iNumOfTrxs, iUseSLI,
                                        c_str("tpcc%d", i));
            testers[i]->fork();

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
        int trxs_att  = _g_shore_env->get_tmp_tpcc_stats()->get_total_attempted();
        int trxs_com  = _g_shore_env->get_tmp_tpcc_stats()->get_total_committed();
        int nords_com = _g_shore_env->get_tmp_tpcc_stats()->get_no_com();
        TRACE( TRACE_ALWAYS, "*******\n" \
               "WHs:      (%d)\n" \
               "Spread:   (%s)\n" \
               "SLI:      (%s)\n" \
               "Threads:  (%d)\n" \
               "Trxs Att: (%d)\n" \
               "Trxs Com: (%d)\n" \
               "NOrd Com: (%d)\n"   \
               "Secs:     (%.2f)\n" \
               "TPS:      (%.2f)\n" \
               "tpm-C:    (%.2f)\n",
               iQueriedWHs, 
               (iSpread ? "Yes" : "No"), (iUseSLI ? "Yes" : "No"), 
               iNumOfThreads, trxs_att, trxs_com, nords_com, 
               delay, 
               trxs_com/delay,
               60*nords_com/delay);

        // reset single run stats
        _g_shore_env->reset_tmp_tpcc_stats();
    }

    // set measurement state
    _g_shore_env->set_measure(MST_DONE);

    return (SHELL_NEXT_CONTINUE);
}


/** cmd: MEASURE **/

int tpcc_kit_shell_t::_cmd_MEASURE_impl(const int iQueriedWHs, 
                                        const int iSpread,
                                        const int iNumOfThreads, 
                                        const int iDuration,
                                        const int iSelectedTrx, 
                                        const int iIterations,
                                        const int iUseSLI)
{
    assert (_g_shore_env->is_initialized());    

    // Print out configuration
    TRACE( TRACE_ALWAYS, "\n\n" \
           "Queried WHs    : %d\n" \
           "Spread Threads : %s\n" \
           "Num of Threads : %d\n" \
           "Duration       : %d\n" \
           "Trx            : %s\n" \
           "Iterations     : %d\n" \
           "Use SLI        : %s\n", 
           iQueriedWHs, (iSpread ? "Yes" : "No"), 
           iNumOfThreads, iDuration, translate_trx_id(iSelectedTrx), 
           iIterations, (iUseSLI ? "Yes" : "No"));

    test_smt_t* testers[MAX_NUM_OF_THR];
    for (int j=0; j<iIterations; j++) {

        TRACE( TRACE_ALWAYS, "Iteration [%d of %d]\n",
               (j+1), iIterations);

        // set measurement state
        _g_shore_env->set_measure(MST_WARMUP);

        int wh_id = 0;

        // create threads
        for (int i=0; i<iNumOfThreads; i++) {
            // create & fork testing threads
            if (iSpread)
                wh_id = i+1;
            testers[i] = new test_smt_t(_g_shore_env, MT_TIME_DUR,
                                        wh_id, iSelectedTrx, 
                                        0, iUseSLI,
                                        c_str("tpcc%d", i));

            if (!testers[i]) {
                TRACE( TRACE_ALWAYS, "Problem creating (%d) thread\n", i);
                assert (0); // should not happen
            }

            testers[i]->fork();
        }

        // TODO: give them some time (2secs) to start-up
        sleep(DF_WARMUP_INTERVAL);

        // set measurement state
        _g_shore_env->set_measure(MST_MEASURE);
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
            delete (testers[i]);
        }

	double delay = timer.time();
	alarm(0); // cancel the alarm, if any

        int trxs_att  = _g_shore_env->get_tmp_tpcc_stats()->get_total_attempted();
        int trxs_com  = _g_shore_env->get_tmp_tpcc_stats()->get_total_committed();
        int nords_com = _g_shore_env->get_tmp_tpcc_stats()->get_no_com();
        TRACE( TRACE_ALWAYS, "*******\n" \
               "WHs:      (%d)\n" \
               "Spread:   (%s)\n" \
               "SLI:      (%s)\n" \
               "Threads:  (%d)\n" \
               "Trxs Att: (%d)\n" \
               "Trxs Com: (%d)\n" \
               "NOrd Com: (%d)\n"   \
               "Secs:     (%.2f)\n" \
               "TPS:      (%.2f)\n" \
               "tpm-C:    (%.2f)\n",
               iQueriedWHs, 
               (iSpread ? "Yes" : "No"), (iUseSLI ? "Yes" : "No"), 
               iNumOfThreads, trxs_att, trxs_com, nords_com, 
               delay, 
               trxs_com/delay,
               60*nords_com/delay);

        // reset run stats
        _g_shore_env->reset_tmp_tpcc_stats();
    }


    // set measurement state
    _g_shore_env->set_measure(MST_DONE);

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
               //               | TRACE_TRX_FLOW
               //| TRACE_DEBUG
              );

    /* 1. Instanciate the Shore environment */
    if (inst_test_env(argc, argv))
        return (1);


    /* 2. Start processing commands */
    tpcc_kit_shell_t kit_shell("(tpcckit) ");
    kit_shell.start();

    /* 3. Close the Shore environment */
    if (_g_shore_env)
        close_test_env();

    return (0);
}
