/* -*- mode:C++; c-basic-offset:4 -*- */
// CC -m64 -xarch=ultraT1 -xs -g -I $SHORE_INCLUDE_DIR shore_tpcc_kit_shell.cpp -o shore_tpcc_load -L $SHORE_LIB_DIR -mt -lsm -lsthread -lfc -lcommon -lpthread

/** @file:   shore_tpcc_kit_shell.cpp
 *
 *  @brief:  Test shore tpc-c kit with shell
 *
 *  @author: Ippokratis Pandis, July 2008
 *
 */

#include "tests/common.h"
#include "stages/tpcc/shore/shore_tpcc_env.h"
#include "sm/shore/shore_helper_loader.h"

#include "util/shell.h"


using namespace shore;
using namespace tpcc;


// Default parameters for the runs

// default value if spread threads at WHs
const int DF_SPREAD_THREADS_TO_WHS = 1;

// default transaction id to be executed
const int DF_TRX_ID = XCT_PAYMENT;

// default use sli
const int DF_USE_SLI = 1;


//////////////////////////////


class tpcc_kit_shell_t : public shell_t 
{
private:

    // helper functions
    const char* translate_trx_id(const int trx_id);
 
public:

    tpcc_kit_shell_t(const char* prompt) 
        : shell_t(prompt)
        {
        }

    ~tpcc_kit_shell_t() 
    { 
        TRACE( TRACE_ALWAYS, "Exiting... (%d) commands processed\n",
               get_command_cnt());
    }

    // shell interface
    int process_command(const char* command);
    int print_usage(const char* command);
    virtual int SIGINT_handler();

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


/** tpcc_kit_shell_t interface */


int tpcc_kit_shell_t::print_usage(const char* command) 
{
    assert (command);

    TRACE( TRACE_ALWAYS, "\nUsage:\n" \
           "test <NUM_QUERIED> [<SPREAD> <NUM_THRS> <NUM_TRXS> <TRX_ID> <ITERATIONS> <SLI>]\n" \
           "\nParameters:\n" \
           "<NUM_QUERIED> : The number of WHs queried (queried factor)\n" \
           "<SPREAD>      : Whether to spread threads to WHs (0=No, Otherwise=Yes, Default=No) (optional)\n" \
           "<NUM_THRS>    : Number of threads used (optional)\n" \
           "<NUM_TRXS>    : Number of transactions per thread (optional)\n" \
           "<TRX_ID>      : Transaction ID to be executed (0=mix) (optional)\n" \
           "<ITERATIONS>  : Number of iterations (Default=5) (optional)\n" \
           "<SLI>         : Use SLI (Default=1-Yes) (optional)\n");
    
    TRACE( TRACE_ALWAYS, "\n\nCurrently numOfWHs = (%d)\n", _numOfWHs);

    return (SHELL_NEXT_CONTINUE);
}


int tpcc_kit_shell_t::process_command(const char* command)
{
    /* 0. Parse Parameters */
    int numOfQueriedWHs     = DF_NUM_OF_QUERIED_WHS;
    int tmp_numOfQueriedWHs = DF_NUM_OF_QUERIED_WHS;
    int spreadThreads       = DF_SPREAD_THREADS_TO_WHS;
    int tmp_spreadThreads   = DF_SPREAD_THREADS_TO_WHS;
    int numOfThreads        = DF_NUM_OF_THR;
    int tmp_numOfThreads    = DF_NUM_OF_THR;
    int numOfTrxs           = DF_TRX_PER_THR;
    int tmp_numOfTrxs       = DF_TRX_PER_THR;
    int selectedTrxID       = DF_TRX_ID;
    int tmp_selectedTrxID   = DF_TRX_ID;
    int iterations          = DF_NUM_OF_ITERS;
    int tmp_iterations      = DF_NUM_OF_ITERS;
    int use_sli             = DF_USE_SLI;
    int tmp_use_sli         = DF_USE_SLI;


    char command_tag[SERVER_COMMAND_BUFFER_SIZE];

    // make sure any previous abort is cleared
    test_smt_t::resume_test();
    
    // Parses new test run data
    if ( sscanf(command, "%s %d %d %d %d %d %d %d",
                &command_tag,
                &tmp_numOfQueriedWHs,
                &tmp_spreadThreads,
                &tmp_numOfThreads,
                &tmp_numOfTrxs,
                &tmp_selectedTrxID,
                &tmp_iterations,
                &tmp_use_sli) < 2 ) 
    {
        print_usage(command_tag);
        return (SHELL_NEXT_CONTINUE);
    }


    // REQUIRED Parameters

    // 1- number of queried warehouses - numOfQueriedWHs
    if ((tmp_numOfQueriedWHs>0) && (tmp_numOfQueriedWHs<=_numOfWHs))
        numOfQueriedWHs = tmp_numOfQueriedWHs;
    else
        numOfQueriedWHs = _numOfWHs;
    assert (numOfQueriedWHs <= _numOfWHs);


    // OPTIONAL Parameters

    // 2- spread trxs
    spreadThreads = tmp_spreadThreads;

    // 3- number of threads - numOfThreads
    if ((tmp_numOfThreads>0) && (tmp_numOfThreads<=MAX_NUM_OF_THR)) {
        numOfThreads = tmp_numOfThreads;
        if (spreadThreads && (numOfThreads > numOfQueriedWHs))
            numOfThreads = numOfQueriedWHs;
    }
    else {
        numOfThreads = numOfQueriedWHs;
    }
    
    // 4- number of trxs - numOfTrxs
    if (tmp_numOfTrxs>0)
        numOfTrxs = tmp_numOfTrxs;

    // 5- selected trx
    if (tmp_selectedTrxID>=0)
        selectedTrxID = tmp_selectedTrxID;

    // 6- number of iterations - iterations
    if (tmp_iterations>0)
        iterations = tmp_iterations;

    // 7- use sli   
    use_sli = tmp_use_sli;


    // Print out configuration
    TRACE( TRACE_ALWAYS, "\n\n" \
           "Queried WHs    : %d\n" \
           "Spread Threads : %s\n" \
           "Num of Threads : %d\n" \
           "Num of Trxs    : %d\n" \
           "Trx            : %s\n" \
           "Iterations     : %d\n" \
           "Use SLI        : %s\n", 
           numOfQueriedWHs, (spreadThreads ? "Yes" : "No"), 
           numOfThreads, numOfTrxs, translate_trx_id(selectedTrxID), 
           iterations, (use_sli ? "Yes" : "No"));

    test_smt_t* testers[MAX_NUM_OF_THR];
    for (int j=0; j<iterations; j++) {

        TRACE( TRACE_ALWAYS, "Iteration [%d of %d]\n",
               (j+1), iterations);

        int wh_id = 0;
	stopwatch_t timer;

        for (int i=0; i<numOfThreads; i++) {
            // create & fork testing threads
            if (spreadThreads)
                wh_id = i+1;
            testers[i] = new test_smt_t(_g_shore_env, wh_id, selectedTrxID, 
                                        numOfTrxs, use_sli,
                                        c_str("tpcc%d", i));
            testers[i]->fork();

        }

        /* 2. join the tester threads */
        for (int i=0; i<numOfThreads; i++) {
            testers[i]->join();
            if (testers[i]->_rv) {
                TRACE( TRACE_ALWAYS, "Error in testing...\n");
                TRACE( TRACE_ALWAYS, "Exiting...\n");
                assert (false);
            }    
            delete (testers[i]);
        }

	double delay = timer.time();
        int nords = _g_shore_env->get_tmp_tpcc_stats()->get_no_com();
        TRACE( TRACE_ALWAYS, "*******\n" \
               "WHs:     (%d)\n" \
               "Spread:  (%s)\n" \
               "SLI:     (%s)\n" \
               "Threads: (%d)\n" \
               "Trxs:    (%d)\n" \
               "Secs:    (%.2f)\n" \
               "nords:   (%d)\n" \
               "TPS:     (%.2f)\n" \
               "tpm-C:   (%.2f)\n",
               numOfQueriedWHs, 
               (spreadThreads ? "Yes" : "No"), (use_sli ? "Yes" : "No"), 
               numOfThreads, numOfTrxs, delay, nords, 
               numOfThreads*numOfTrxs/delay,
               60*nords/delay);

        _g_shore_env->reset_tmp_tpcc_stats();
    }

    return (SHELL_NEXT_CONTINUE);
}

int tpcc_kit_shell_t::SIGINT_handler() {
    if(_processing_command) {
	test_smt_t::abort_test();
	return 0;
    }

    // fallback...
    return shell_t::SIGINT_handler();
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
               //               | TRACE_DEBUG
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


