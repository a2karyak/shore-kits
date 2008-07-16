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

// default number of threads
const int DF_NUM_OF_THR  = 10;

// maximum number of threads
const int MAX_NUM_OF_THR = 100;

// default number of transactions executed per thread
const int DF_TRX_PER_THR = 100;

// default transaction id to be executed
const int DF_TRX_ID = XCT_PAYMENT;

// default number of iterations
const int DF_NUM_OF_ITERS = 5;

// default use sli
const int DF_USE_SLI = 1;


///////////////////////////////////////////////////////////
// @class test_smt_t
//
// @brief An smthread-based class for tests

class test_smt_t : public thread_t {
private:
    ShoreTPCCEnv* _env;    

    // workload parameters
    int _wh;
    int _trxid;
    int _notrxs;
    int _use_sli;

public:
    int	_rv;
    
    test_smt_t(ShoreTPCCEnv* env, 
               int sWH, int trxId, int numOfTrxs, int useSLI,
               c_str tname) 
	: thread_t(tname), 
          _env(env), _wh(sWH), _trxid(trxId), _notrxs(numOfTrxs), 
          _use_sli(useSLI),
          _rv(0)
    {
        assert (_env);
        assert (_notrxs);
        assert (_wh>=0);
    }


    ~test_smt_t() { }


    w_rc_t run_xcts(ShoreTPCCEnv* env, int xct_type, int num_xct);
    w_rc_t run_one_tpcc_xct(ShoreTPCCEnv* env, int xct_type, int xctid);    

    w_rc_t xct_new_order(ShoreTPCCEnv* env, int xctid);
    w_rc_t xct_payment(ShoreTPCCEnv* env, int xctid);
    w_rc_t xct_order_status(ShoreTPCCEnv* env, int xctid);
    w_rc_t xct_delivery(ShoreTPCCEnv* env, int xctid);
    w_rc_t xct_stock_level(ShoreTPCCEnv* env, int xctid);


    // thread entrance
    void work() {
        if (!_env->is_initialized()) {
            if (_env->init()) {
                // Couldn't initialize the Shore environment
                // cannot proceed
                TRACE( TRACE_ALWAYS, "Couldn't initialize Shore...\n");
                _rv = 1;
                return;
            }
        }

        // check SLI
        ss_m::set_sli_enabled(_use_sli);

        // run test
        _rv = test();
    }

    // methods
    int test() {
        W_DO(_env->loaddata());
        //_env->check_consistency();
        W_DO(run_xcts(_env, _trxid, _notrxs));
        //print_tables();
        return (0);
    }

    void print_tables();

    /** @note Those two functions should be implemented by every
     *        smthread-inherited class that runs using run_smthread()
     */
    inline int retval() { return (_rv); }

}; // EOF: test_smt_t



w_rc_t test_smt_t::xct_new_order(ShoreTPCCEnv* env, int xctid) 
{ 
    assert (env);
    trx_result_tuple_t atrt;
    env->run_new_order(xctid, atrt, _wh);    
    return (RCOK); 
}

w_rc_t test_smt_t::xct_payment(ShoreTPCCEnv* env, int xctid) 
{ 
    assert (env);
    trx_result_tuple_t atrt;
    env->run_payment(xctid, atrt, _wh);    
    return (RCOK); 
}

w_rc_t test_smt_t::xct_order_status(ShoreTPCCEnv* env, int xctid) 
{ 
    assert (env);
    trx_result_tuple_t atrt;
    env->run_order_status(xctid, atrt, _wh);    
    return (RCOK); 
}


w_rc_t test_smt_t::xct_delivery(ShoreTPCCEnv* env, int xctid) 
{ 
    assert (env);
    trx_result_tuple_t atrt;
    env->run_delivery(xctid, atrt, _wh);    
    return (RCOK); 
}


w_rc_t test_smt_t::xct_stock_level(ShoreTPCCEnv* env, int xctid) 
{ 
    assert (env);
    trx_result_tuple_t atrt;
    env->run_stock_level(xctid, atrt, _wh);    
    return (RCOK); 
}


void test_smt_t::print_tables() 
{
    _env->dump();
}


w_rc_t test_smt_t::run_xcts(ShoreTPCCEnv* env, int xct_type, int num_xct)
{
    for (int i=0; i<num_xct; i++) {

        run_one_tpcc_xct(env, xct_type, i);
    }
    return (RCOK);
}


 
w_rc_t test_smt_t::run_one_tpcc_xct(ShoreTPCCEnv* env, int xct_type, int xctid) 
{
    if (xct_type == 0) {        
        xct_type = rand(100);       
    }
    
    switch (xct_type) {
    case XCT_NEW_ORDER:
        W_DO(xct_new_order(env, xctid));  break;
    case XCT_PAYMENT:
        W_DO(xct_payment(env, xctid)); break;
    case XCT_ORDER_STATUS:
        W_DO(xct_order_status(env, xctid)); break;
    case XCT_DELIVERY:
        W_DO(xct_delivery(env, xctid)); break;
    case XCT_STOCK_LEVEL:
        W_DO(xct_stock_level(env, xctid)); break;
    }

    return RCOK;
}


/** EOF: test_tree_smt_t functions */



//////////////////////////////


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
           "%s <NUM_QUERIED> [<SPREAD> <NUM_THRS> <NUM_TRXS> <TRX_ID> <ITERATIONS> <SLI>]\n" \
           "\nParameters:\n" \
           "<NUM_QUERIED> : The number of WHs queried (queried factor)\n" \
           "<SPREAD>      : Whether to spread threads to WHs (0=No, Otherwise=Yes, Default=No) (optional)\n" \
           "<NUM_THRS>    : Number of threads used (optional)\n" \
           "<NUM_TRXS>    : Number of transactions per thread (optional)\n" \
           "<TRX_ID>      : Transaction ID to be executed (0=mix) (optional)\n" \
           "<ITERATIONS>  : Number of iterations (Default=5) (optional)\n" \
           "<SLI>         : Use SLI (Default=1-Yes) (optional)\n",
           command);
    
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
    if (tmp_selectedTrxID>0)
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
        TRACE( TRACE_ALWAYS, "*******\n" \
               "WHs:     (%d)\n" \
               "Spread:  (%s)\n" \
               "Threads: (%d)\n" \
               "Trxs:    (%d)\n" \
               "SLI:     (%s)\n" \
               "Secs:    (%.2f)\n" \
               "TPS:     (%.2f)\n",
               numOfQueriedWHs, (spreadThreads ? "Yes" : "No"), numOfThreads, 
               numOfTrxs, (use_sli ? "Yes" : "No"), delay, 
               numOfThreads*numOfTrxs/delay);
    }

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


