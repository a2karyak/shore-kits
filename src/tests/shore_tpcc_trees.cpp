/* -*- mode:C++; c-basic-offset:4 -*- */
// CC -m64 -xarch=ultraT1 -xs -g -I $SHORE_INCLUDE_DIR shore_tpcc_load.cpp -o shore_tpcc_load -L $SHORE_LIB_DIR -mt -lsm -lsthread -lfc -lcommon -lpthread

// file: shore_tpcc_trees.cpp
// desc: Tests the performance of the Shore B-tree under various consistency modes.


#include "tests/common.h"
#include "workload/tpcc/shore_tpcc_env.h"

#include "util/shell.h"


using namespace shore;
using namespace tpcc;


// enum of the various tree tests
const int XCT_CUST_KVL_LOCK_TREE  = 10;
const int XCT_CUST_NO_LOCK_TREE   = 11;
const int XCT_STOCK_KVL_LOCK_TREE = 12;
const int XCT_STOCK_NO_LOCK_TREE  = 13;


//// Default values of the parameters for the run

// default type of trx
const int DF_UPDATE_TUPLE = 0;

// default commit every that many trx
const int DF_COMMIT_INTERVAL = 2;



///////////////////////////////////////////////////////////
// @class test_tree_smt_t
//
// @brief An smthread-based class for testing the trees

class test_tree_smt_t : public thread_t {
private:
    ShoreTPCCEnv* _env;    

    // workload parameters
    int _wh;
    int _trxid;
    int _updtuple;
    int _notrxs;
    int _ci;

public:
    int	_rv;
    
    test_tree_smt_t(ShoreTPCCEnv* env, 
                    int sWH, int trxID, int updTuple, 
                    int numOfTrxs, int commitInterval,
                    c_str tname) 
	: thread_t(tname), _env(env), 
          _wh(sWH), _trxid(trxID), _updtuple(updTuple), 
          _notrxs(numOfTrxs), _ci(commitInterval),
          _rv(0)
    {
        assert (_env);
        assert (_wh>=0);
        assert (_notrxs);
        assert (_ci);
    }


    ~test_tree_smt_t() { }

    w_rc_t test_trees();

    w_rc_t xct_cust_tree(ShoreTPCCEnv* env, bool nolock, int updtuple, rep_row_t arp);
    w_rc_t xct_stock_tree(ShoreTPCCEnv* env, bool nolock, int updtuple, rep_row_t arp);


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
        // run test
        w_rc_t e = test_trees();
        if (e.is_error()) {
            TRACE( TRACE_ALWAYS, "Tree testing failed [0x%x]\n", e.err_num());
            w_rc_t e_abort = _env->db()->abort_xct();
            if (e_abort.is_error()) {
                TRACE( TRACE_ALWAYS, "Aborting failed [0x%x]\n", e_abort.err_num());
            }
        }

        _rv = e.is_error();
    }

    /** @note Those two functions should be implemented by every
     *        smthread-inherited class that runs using run_smthread()
     */
    inline int retval() { return (_rv); }

}; // EOF: test_tree_smt_t


w_rc_t test_tree_smt_t::test_trees()
{
    W_DO(_env->loaddata());

    int flag = _ci;
    rep_row_t areprow(_env->customer_man()->ts());

    W_DO(_env->db()->begin_xct());

    for (int i=0; i<_notrxs; i++) {

        switch (_trxid) {
        case XCT_CUST_KVL_LOCK_TREE:
            xct_cust_tree(_env, false, _updtuple, areprow);
            break;
        case XCT_CUST_NO_LOCK_TREE:
            xct_cust_tree(_env, true, _updtuple, areprow);
            break;
        case XCT_STOCK_KVL_LOCK_TREE:
            xct_stock_tree(_env, false, _updtuple, areprow);
            break;
        case XCT_STOCK_NO_LOCK_TREE:
            xct_stock_tree(_env, true, _updtuple, areprow);
            break;    
        default:
            assert (false);
            printf("nada..\n");
            break;
        }        


        if (i > flag) {
            W_DO(_env->db()->commit_xct());
            flag += _ci;
            W_DO(_env->db()->begin_xct());
        }


    }    

    W_DO(_env->db()->commit_xct());

    return (RCOK);
}


w_rc_t test_tree_smt_t::xct_cust_tree(ShoreTPCCEnv* env, bool nolock, 
                                      int updtuple, rep_row_t arp) 
{ 
    assert (env);
    assert (_wh>0);

    // prepare the random input (customer key to probe)
    int in_wh = _wh;
    int in_d  = rand(DISTRICTS_PER_WAREHOUSE) + 1;
    int in_c  = rand(CUSTOMERS_PER_DISTRICT) + 1;
    decimal c_discount; 
    decimal c_balance;
    w_rc_t e;
    row_impl<customer_t>* prcust = _env->customer_man()->get_tuple();
    prcust->_rep = &arp;

    TRACE( TRACE_DEBUG, "(%d) (%d) (%d)\n", in_wh, in_d, in_c);

    if (nolock) {
        TRACE( TRACE_DEBUG, "CUST-TREE-NO-LOCK\n");
        e = _env->customer_man()->cust_index_probe_by_name(_env->db(), 
                                                           "C_INDEX_NL", 
                                                           prcust, 
                                                           in_wh, in_d, in_c); 
    }
    else {
        if (updtuple) {
            TRACE( TRACE_DEBUG, "CUST-TREE-KVL-LOCK-WITH-UPD\n");
            e = _env->customer_man()->cust_index_probe_forupdate(_env->db(), 
                                                                 prcust, 
                                                                 in_wh, in_d, in_c); 
        }
        else {
            TRACE( TRACE_DEBUG, "CUST-TREE-KVL-LOCK-NO-UPD\n");
            e = _env->customer_man()->cust_index_probe_by_name(_env->db(), 
                                                               "C_INDEX", prcust, 
                                                               in_wh, in_d, in_c); 
        }
    }

    if (e.is_error()) {
        TRACE( TRACE_ALWAYS, "Probe failed [0x%x]\n", e.err_num());
        _env->customer_man()->give_tuple(prcust);
        return (e);
    }

    prcust->get_value(15, c_discount);
    prcust->get_value(16, c_balance);
    TRACE( TRACE_DEBUG, "(%d) (%d) (%d) (%.3f) (%.2f)\n", in_wh, in_d, in_c, 
           c_discount.to_double(), c_balance.to_double());
    

    if (updtuple) {
        // if should also update the tuple
        c_discount += 0.0001;
        c_balance++;
        TRACE( TRACE_DEBUG, "UPD-TUPLE (%.3f) (%.2f)\n", 
               c_discount.to_double(), c_balance.to_double());
        e = _env->customer_man()->cust_update_discount_balance(_env->db(),
                                                               prcust,
                                                               c_discount,
                                                               c_balance);
            
        if (e.is_error()) {
            TRACE( TRACE_ALWAYS, "Tuple update failed [0x%x]\n", e.err_num());
            _env->customer_man()->give_tuple(prcust);
            return (e);
        }            
    }


    // probe and optional tuple upldate succeeded
    _env->customer_man()->give_tuple(prcust);
    return (RCOK); 
}

w_rc_t test_tree_smt_t::xct_stock_tree(ShoreTPCCEnv* env, bool nolock, 
                                       int updtuple, rep_row_t arp) 
{ 
    assert (env);

    assert(false); // need to generate random stock input

    row_impl<stock_t>* prst = _env->stock_man()->get_tuple();
    prst->_rep = &arp;

    if (nolock) {
        TRACE( TRACE_DEBUG, "STOCK-TREE-NO-LOCK\n");
        assert(false); // need to call index_probe_no_lock
    }
    else {
        TRACE( TRACE_DEBUG, "STOCK-TREE-KVL-LOCK\n");
        assert(false); // need to call index_probe
    }
    return (RCOK); 
}

/** EOF: test_tree_smt_t functions */


//////////////////////////////


//////////////////////////////


class tree_test_shell_t : public shell_t 
{
private:

    // helper functions
    const char* translate_trx_id(const int trx_id);
 
public:

    tree_test_shell_t(const char* prompt) 
        : shell_t(prompt)
        {
        }

    ~tree_test_shell_t() 
    { 
        TRACE( TRACE_ALWAYS, "Exiting... (%d) commands processed\n",
               get_command_cnt());
    }

    const int register_commands() { return (SHELL_NEXT_CONTINUE); }

    // shell interface
    int process_command(const char* command, const char* cmd_tag);
    int print_usage(const char* command);

}; // EOF: tree_test_shell_t



/** tree_test_shell_t helper functions */


const char* tree_test_shell_t::translate_trx_id(const int trx_id) 
{
    switch (trx_id) {
    case (XCT_CUST_KVL_LOCK_TREE):
        return ("CustomerProbe-KVL");
        break;
    case (XCT_CUST_NO_LOCK_TREE):
        return ("CustomerProbe-NoLock");
        break;
    case (XCT_STOCK_KVL_LOCK_TREE):
        return ("StockProbe-KVL");
        break;
    case (XCT_STOCK_NO_LOCK_TREE):
        return ("StockProbe-NoLock");
        break;
    default:
        TRACE( TRACE_ALWAYS, "Unknown TRX\n");
        assert (false);
        return ("Error!!");
    }
}


/** tree_test_shell_t interface */


int tree_test_shell_t::print_usage(const char* command) 
{
    assert (command);

    TRACE( TRACE_ALWAYS, "\nUsage:\n" \
           "test <NUM_QUERIED>  <TRX_ID> <UPD_TRX> [<NUM_THREADS> <NUM_TRXS> <ITERATIONS> <COMMIT_INT>]\n" \
           "\nParameters:\n" \
           "<NUM_QUERIED> : The number of WHs queried (queried factor)\n" \
           "<TRX_ID>      : Transaction ID to be executed (0=mix)\n" \
           "<UPD_TRX>     : Will the trx update the tuple or only probe?\n" \
           "<NUM_THREADS> : Number of concurrent threads (Default=1) (optional)\n" \
           "<NUM_TRXS>    : Number of trxs per thread (Default=100) (optional)\n" \
           "<ITERATIONS>  : Number of iterations (Default=5) (optional)\n" \
           "<COMMIT_INT>  : Commit Interval (Default=10) (optional)\n");
    
    TRACE( TRACE_ALWAYS, "\n\nCurrent Scaling factor = (%d)\n", _theSF);

    return (SHELL_NEXT_CONTINUE);
}


int tree_test_shell_t::process_command(const char* cmd, const char* cmd_tag)
{
    /* 0. Parse Parameters */
    int numOfQueriedSF     = DF_NUM_OF_QUERIED_WHS;
    int tmp_numOfQueriedSF = DF_NUM_OF_QUERIED_WHS;
    int selectedTrxID       = DF_TRX_ID;
    int tmp_selectedTrxID   = DF_TRX_ID;
    int updTuple            = DF_UPDATE_TUPLE;
    int tmp_updTuple        = DF_UPDATE_TUPLE;
    int numOfThreads        = DF_NUM_OF_THR;
    int tmp_numOfThreads    = DF_NUM_OF_THR;
    int numOfTrxs           = DF_TRX_PER_THR;
    int tmp_numOfTrxs       = DF_TRX_PER_THR;
    int iterations          = DF_NUM_OF_ITERS;
    int tmp_iterations      = DF_NUM_OF_ITERS;
    int commit_interval     = DF_COMMIT_INTERVAL;
    int tmp_ci              = DF_COMMIT_INTERVAL;

    // update the SF
    int tmp_sf = envVar::instance()->getSysVarInt("sf");
    if (tmp_sf) {
        TRACE( TRACE_STATISTICS, "Updated SF (%d)\n", tmp_sf);
        _theSF = tmp_sf;
    }


    // Parses new test run data
    if ( sscanf(cmd, "%s %d %d %d %d %d %d %d",
                &cmd_tag,
                &tmp_numOfQueriedSF,
                &tmp_selectedTrxID,
                &tmp_updTuple,
                &tmp_numOfThreads,
                &tmp_numOfTrxs,
                &tmp_iterations,
                &tmp_ci) < 4 ) 
    {
        print_usage(cmd_tag);
        return (SHELL_NEXT_CONTINUE);
    }


    // REQUIRED Parameters

    // 1- number of queried warehouses - numOfQueriedSF
    if ((tmp_numOfQueriedSF>0) && (tmp_numOfQueriedSF<=_theSF))
        numOfQueriedSF = tmp_numOfQueriedSF;
    else
        numOfQueriedSF = _theSF;
    assert (numOfQueriedSF <= _theSF);

    // 2- selected trx
    if (tmp_selectedTrxID>=0)
        selectedTrxID = tmp_selectedTrxID;

    // 3- do update trx
    updTuple = tmp_updTuple;


    // OPTIONAL Parameters

    // 4- number of threads - numOfThreads
    if ((tmp_numOfThreads>0) && (tmp_numOfThreads<=numOfQueriedSF) && 
        (tmp_numOfThreads<=MAX_NUM_OF_THR)) {
            numOfThreads = tmp_numOfThreads;
        }
    else {
        numOfThreads = numOfQueriedSF;
    }
    
    // 5- number of trxs - numOfTrxs
    if (tmp_numOfTrxs>0)
        numOfTrxs = tmp_numOfTrxs;

    // 6- number of iterations - iterations
    if (tmp_iterations>0)
        iterations = tmp_iterations;

    // 7- commit interval (trxs per commit) - commit_interval
    if (tmp_ci>0)
        commit_interval = tmp_ci;


    // Print out configuration
    TRACE( TRACE_ALWAYS, "\n\n" \
           "Queried WHs     : %d\n" \
           "Trxs ID         : %s\n" \
           "Update Tuple    : %s\n" \
           "Num of Threads  : %d\n" \
           "Num of Trxs     : %d\n" \
           "Iterations      : %d\n" \
           "Commit Interval : %d\n", 
           numOfQueriedSF, translate_trx_id(selectedTrxID), 
           (updTuple ? "Yes" : "No"), numOfThreads, numOfTrxs, 
           iterations, commit_interval);

    test_tree_smt_t* testers[MAX_NUM_OF_THR];
    for (int j=0; j<iterations; j++) {

        TRACE( TRACE_ALWAYS, "Iteration [%d of %d]\n",
               (j+1), iterations);

        int wh_id = 0;
	stopwatch_t timer;

        for (int i=0; i<numOfThreads; i++) {

            // create & fork testing threads
            wh_id++;
            assert(0);testers[i] = NULL;
            //            new test_tree_smt_t(_g_shore_env, wh_id, selectedTrxID, 
            //                                              updTuple, numOfTrxs, commit_interval,
            //                                              c_str("tt%d", i));
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
               "Update:  (%s)\n" \
               "Threads: (%d)\n" \
               "Trxs:    (%d)\n" \
               "Secs:    (%.2f)\n" \
               "TPS:     (%.2f)\n",
               numOfQueriedSF, (updTuple ? "Yes" : "No"), numOfThreads, 
               numOfTrxs, delay, numOfThreads*numOfTrxs/delay);
    }

    return (SHELL_NEXT_CONTINUE);
}

/** EOF: tree_test_shell_t functions */




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

//     /* 1. Instanciate the Shore environment */
//     if (inst_test_env(argc, argv))
//         return (1);


    /* 2. Start processing commands */
    tree_test_shell_t tshell("(tree) ");
    tshell.start();

//     /* 3. Close the Shore environment */
//     if (_g_shore_env)
//         close_test_env();

    return (0);
}


