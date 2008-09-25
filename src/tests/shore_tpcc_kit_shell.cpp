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

extern "C" void alarm_handler(int sig) {
    _g_shore_env->set_measure(MST_DONE);
}

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
	    struct sigaction sa;
	    struct sigaction sa_old;
	    sa.sa_flags = 0;
	    sigemptyset(&sa.sa_mask);
	    sa.sa_handler = &alarm_handler;
	    if(sigaction(SIGALRM, &sa, &sa_old) < 0)
		exit(1);
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

    // supported commands
    int process_cmd_TEST(const char* command, char* command_tag);
    int process_cmd_MEASURE(const char* command, char* command_tag);

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

    TRACE( TRACE_ALWAYS, "\n\nSupported commands: TEST/MEASURE\n\n" );

    TRACE( TRACE_ALWAYS, "TEST Usage:\n\n" \
           "test <NUM_QUERIED> [<SPREAD> <NUM_THRS> <NUM_TRXS> <TRX_ID> <ITERATIONS> <SLI>]\n" \
           "\nParameters:\n" \
           "<NUM_QUERIED> : The number of WHs queried (queried factor)\n" \
           "<SPREAD>      : Whether to spread threads to WHs (0=No, Otherwise=Yes, Default=No) (optional)\n" \
           "<NUM_THRS>    : Number of threads used (optional)\n" \
           "<NUM_TRXS>    : Number of transactions per thread (optional)\n" \
           "<TRX_ID>      : Transaction ID to be executed (0=mix) (optional)\n" \
           "<ITERATIONS>  : Number of iterations (Default=5) (optional)\n" \
           "<SLI>         : Use SLI (Default=1-Yes) (optional)\n\n");

    TRACE( TRACE_ALWAYS, "MEASURE Usage:\n\n" \
           "measure <NUM_QUERIED> [<SPREAD> <NUM_THRS> <DURATION> <TRX_ID> <ITERATIONS> <SLI>]\n" \
           "\nParameters:\n" \
           "<NUM_QUERIED> : The number of WHs queried (queried factor)\n" \
           "<SPREAD>      : Whether to spread threads to WHs (0=No, Otherwise=Yes, Default=No) (optional)\n" \
           "<NUM_THRS>    : Number of threads used (optional)\n" \
           "<DURATION>    : Duration of experiment in secs (Default=10) (optional)\n" \
           "<TRX_ID>      : Transaction ID to be executed (0=mix) (optional)\n" \
           "<ITERATIONS>  : Number of iterations (Default=5) (optional)\n" \
           "<SLI>         : Use SLI (Default=1-Yes) (optional)\n");
    
    TRACE( TRACE_ALWAYS, "\n\nCurrently numOfWHs = (%d)\n", _numOfWHs);

    return (SHELL_NEXT_CONTINUE);
}




int tpcc_kit_shell_t::process_command(const char* command)
{

    char command_tag[SERVER_COMMAND_BUFFER_SIZE];

    // make sure any previous abort is cleared
    test_smt_t::resume_test();

    if ( sscanf(command, "%s", &command_tag) < 1) {
        print_usage(command_tag);
        return (SHELL_NEXT_CONTINUE);
    }


    // TEST cmd
    if (strcasecmp(command_tag, "TEST") == 0) {
        return (process_cmd_TEST(command, command_tag));
    }
    
    if (strcasecmp(command_tag, "MEASURE") == 0) {
        return (process_cmd_MEASURE(command, command_tag));
    }
    else {
        print_usage(command_tag);
        return (SHELL_NEXT_CONTINUE);
    }        

    assert (0); // should never reach this point
    return (SHELL_NEXT_CONTINUE);
}

static bool volatile canceled = false;

int tpcc_kit_shell_t::process_cmd_TEST(const char* command, char* command_tag)
{
    canceled = false;
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

        // set measurement state to measure - start counting everything
        _g_shore_env->set_measure(MST_MEASURE);

        int wh_id = 0;
	stopwatch_t timer;

        for (int i=0; i<numOfThreads; i++) {
            // create & fork testing threads
            if (spreadThreads)
                wh_id = i+1;
            testers[i] = new test_smt_t(_g_shore_env, MT_NUM_OF_TRXS,
                                        wh_id, selectedTrxID, 
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
               numOfQueriedWHs, 
               (spreadThreads ? "Yes" : "No"), (use_sli ? "Yes" : "No"), 
               numOfThreads, trxs_att, trxs_com, nords_com, 
               delay, 
               trxs_com/delay,
               60*nords_com/delay);

        _g_shore_env->reset_tmp_tpcc_stats();
    }

    // set measurement state
    _g_shore_env->set_measure(MST_DONE);

    return (SHELL_NEXT_CONTINUE);
}

int tpcc_kit_shell_t::process_cmd_MEASURE(const char* command, char* command_tag)
{
    canceled = false;
    TRACE( TRACE_ALWAYS, "measuring...\n");

    /* 0. Parse Parameters */
    int numOfQueriedWHs     = DF_NUM_OF_QUERIED_WHS;
    int tmp_numOfQueriedWHs = DF_NUM_OF_QUERIED_WHS;
    int spreadThreads       = DF_SPREAD_THREADS_TO_WHS;
    int tmp_spreadThreads   = DF_SPREAD_THREADS_TO_WHS;
    int numOfThreads        = DF_NUM_OF_THR;
    int tmp_numOfThreads    = DF_NUM_OF_THR;
    int duration            = DF_DURATION;
    int tmp_duration        = DF_DURATION;
    int selectedTrxID       = DF_TRX_ID;
    int tmp_selectedTrxID   = DF_TRX_ID;
    int iterations          = DF_NUM_OF_ITERS;
    int tmp_iterations      = DF_NUM_OF_ITERS;
    int use_sli             = DF_USE_SLI;
    int tmp_use_sli         = DF_USE_SLI;
    
    // Parses new test run data
    if ( sscanf(command, "%s %d %d %d %d %d %d %d",
                &command_tag,
                &tmp_numOfQueriedWHs,
                &tmp_spreadThreads,
                &tmp_numOfThreads,
                &tmp_duration,
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
    
    // 4- duration of measurement - duration
    if (tmp_duration>0)
        duration = tmp_duration;

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
           "Duration       : %d\n" \
           "Trx            : %s\n" \
           "Iterations     : %d\n" \
           "Use SLI        : %s\n", 
           numOfQueriedWHs, (spreadThreads ? "Yes" : "No"), 
           numOfThreads, duration, translate_trx_id(selectedTrxID), 
           iterations, (use_sli ? "Yes" : "No"));

    test_smt_t* testers[MAX_NUM_OF_THR];
    for (int j=0; j<iterations; j++) {

        TRACE( TRACE_ALWAYS, "Iteration [%d of %d]\n",
               (j+1), iterations);


        // set measurement state
        _g_shore_env->set_measure(MST_WARMUP);


        int wh_id = 0;

        // create threads
        for (int i=0; i<numOfThreads; i++) {
            // create & fork testing threads
            if (spreadThreads)
                wh_id = i+1;
            testers[i] = new test_smt_t(_g_shore_env, MT_TIME_DUR,
                                        wh_id, selectedTrxID, 
                                        0, use_sli,
                                        c_str("tpcc%d", i));
            testers[i]->fork();

        }

        // TODO: give them some time (2secs) to start-up
        sleep(DF_WARMUP_INTERVAL);

        // set measurement state
        _g_shore_env->set_measure(MST_MEASURE);
	alarm(duration);
	stopwatch_t timer;

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
               numOfQueriedWHs, 
               (spreadThreads ? "Yes" : "No"), (use_sli ? "Yes" : "No"), 
               numOfThreads, trxs_att, trxs_com, nords_com, 
               delay, 
               trxs_com/delay,
               60*nords_com/delay);

        _g_shore_env->reset_tmp_tpcc_stats();
    }


    // set measurement state
    _g_shore_env->set_measure(MST_DONE);

    return (SHELL_NEXT_CONTINUE);
}

int tpcc_kit_shell_t::SIGINT_handler() {
    if(_processing_command && !canceled) {
	canceled = true;
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


