/* -*- mode:C++; c-basic-offset:4 -*- */
// CC -m64 -xarch=ultraT1 -xs -g -I $SHORE_INCLUDE_DIR shore_tpcc_load.cpp -o shore_tpcc_load -L $SHORE_LIB_DIR -mt -lsm -lsthread -lfc -lcommon -lpthread

// file: shore_tpcc_trees.cpp
// desc: Tests the performance of the Shore B-tree under various consistency modes.


#include "tests/common.h"
#include "stages/tpcc/shore/shore_tpcc_env.h"
#include "sm/shore/shore_helper_loader.h"

#include "util/shell.h"


using namespace shore;
using namespace tpcc;


// Instanciate and close the Shore environment
int inst_env(int argc, char* argv[]);
int close_env();

// default database size (scaling factor)
const int DF_NUM_OF_WHS = 10;
int _numOfWHs           = DF_NUM_OF_WHS;    


// default queried number of warehouses (queried factor)
const int DF_NUM_OF_QUERIED_WHS = 10;

// default type of trx
const int DF_UPDATE_TUPLE = 0;

// default number of threads
const int DF_NUM_OF_THREADS = 1;
const int MAX_NUM_OF_THR    = 100;

// default number of transactions executed per thread
const int DF_TRX_PER_THR = 100;

// enum of the various trees
const int XCT_CUST_KVL_LOCK_TREE  = 10;
const int XCT_CUST_NO_LOCK_TREE   = 11;
const int XCT_STOCK_KVL_LOCK_TREE = 12;
const int XCT_STOCK_NO_LOCK_TREE  = 13;


// default transaction id to be executed
const int DF_TRX_ID = XCT_CUST_KVL_LOCK_TREE;

// default number of iterations
const int DF_NUM_OF_ITERS = 5;


// commit every that many trx
const int COMMIT_INTERVAL = 2;


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

public:
    int	_rv;
    
    test_tree_smt_t(ShoreTPCCEnv* env, 
                    int sWH, int trxID, int updTuple, int numOfTrxs, 
                    c_str tname) 
	: thread_t(tname), _env(env), 
          _wh(sWH), _trxid(trxID), _updtuple(updTuple), _notrxs(numOfTrxs),
          _rv(0)
    {
        assert (_env);
        assert (_notrxs);
        assert (_wh>=0);
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
        _rv = test_trees();
    }

    /** @note Those two functions should be implemented by every
     *        smthread-inherited class that runs using run_smthread()
     */
    inline int retval() { return (_rv); }

}; // EOF: test_tree_smt_t


w_rc_t test_tree_smt_t::test_trees()
{
    W_DO(_env->loaddata());

    int flag = COMMIT_INTERVAL;
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
            flag += COMMIT_INTERVAL;
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
        e = _env->customer_man()->index_probe_by_name(_env->db(), 
                                                      "C_INDEX_NOLOCK", 
                                                      prcust, 
                                                      in_c, in_wh, in_d); 
    }
    else {
        if (updtuple) {
            TRACE( TRACE_DEBUG, "CUST-TREE-KVL-LOCK-WITH-UPD\n");
            e = _env->customer_man()->index_probe_forupdate(_env->db(), 
                                                            prcust, 
                                                            in_c, in_wh, in_d); 
        }
        else {
            TRACE( TRACE_DEBUG, "CUST-TREE-KVL-LOCK-NO-UPD\n");
            e = _env->customer_man()->index_probe_by_name(_env->db(), 
                                                          "C_INDEX", prcust, 
                                                          in_c, in_wh, in_d); 
        }
    }

    if (e) {
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
        e = _env->customer_man()->update_discount_balance(_env->db(),
                                                          prcust,
                                                          c_discount,
                                                          c_balance);
            
        if (e) {
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

    // shell interface
    int process_command(const char* command);
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
           "%s <NUM_QUERIED>  <TRX_ID> <UPD_TRX> [<NUM_THREADS> <NUM_TRXS> <ITERATIONS>]\n" \
           "\nParameters:\n" \
           "<NUM_QUERIED> : The number of WHs queried (queried factor)\n" \
           "<TRX_ID>      : Transaction ID to be executed (0=mix)\n" \
           "<UPD_TRX>     : Will the trx update the tuple or only probe?\n" \
           "<NUM_THREADS> : Number of concurrent threads (Default=1) (optional)\n" \
           "<NUM_TRXS>    : Number of trxs per thread (Default=100) (optional)\n" \
           "<ITERATIONS>  : Number of iterations (Default=5) (optional)\n",
           command);
    
    TRACE( TRACE_ALWAYS, "\n\nCurrently numOfWHs = (%d)\n", _numOfWHs);

    return (SHELL_NEXT_CONTINUE);
}


int tree_test_shell_t::process_command(const char* command)
{
    /* 0. Parse Parameters */
    int numOfQueriedWHs     = DF_NUM_OF_QUERIED_WHS;
    int tmp_numOfQueriedWHs = DF_NUM_OF_QUERIED_WHS;
    int selectedTrxID       = DF_TRX_ID;
    int tmp_selectedTrxID   = DF_TRX_ID;
    int updTuple            = DF_UPDATE_TUPLE;
    int tmp_updTuple        = DF_UPDATE_TUPLE;
    int numOfThreads        = DF_NUM_OF_THREADS;
    int tmp_numOfThreads    = DF_NUM_OF_THREADS;
    int numOfTrxs           = DF_TRX_PER_THR;
    int tmp_numOfTrxs       = DF_TRX_PER_THR;
    int iterations          = DF_NUM_OF_ITERS;
    int tmp_iterations      = DF_NUM_OF_ITERS;

    char command_tag[SERVER_COMMAND_BUFFER_SIZE];

    // Parses new test run data
    if ( sscanf(command, "%s %d %d %d %d %d %d",
                &command_tag,
                &tmp_numOfQueriedWHs,
                &tmp_selectedTrxID,
                &tmp_updTuple,
                &tmp_numOfThreads,
                &tmp_numOfTrxs,
                &tmp_iterations) < 4 ) 
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

    // 2- selected trx
    if (tmp_selectedTrxID>0)
        selectedTrxID = tmp_selectedTrxID;

    // 3- do update trx
    updTuple = tmp_updTuple;



    // OPTIONAL Parameters


    // 4- number of threads - numOfThreads
    if ((tmp_numOfThreads>0) && (tmp_numOfThreads<=numOfQueriedWHs) && 
        (tmp_numOfThreads<=MAX_NUM_OF_THR)) {
            numOfThreads = tmp_numOfThreads;
        }
    else {
        numOfThreads = numOfQueriedWHs;
    }
    
    // 5- number of trxs - numOfTrxs
    if (tmp_numOfTrxs>0)
        numOfTrxs = tmp_numOfTrxs;

    // 6- number of iterations - iterations
    if (tmp_iterations>0)
        iterations = tmp_iterations;


    // Print out configuration
    TRACE( TRACE_ALWAYS, "\n\n" \
           "Queried WHs    : %d\n" \
           "Trxs ID        : %s\n" \
           "Update Tuple   : %s\n" \
           "Num of Threads : %d\n" \
           "Num of Trxs    : %d\n" \
           "Iterations     : %d\n", 
           numOfQueriedWHs, translate_trx_id(selectedTrxID), 
           (updTuple ? "Yes" : "No"), numOfThreads, numOfTrxs, iterations);

    return (SHELL_NEXT_CONTINUE);

    test_tree_smt_t* testers[MAX_NUM_OF_THR];
    for (int j=0; j<iterations; j++) {

        TRACE( TRACE_ALWAYS, "Iteration [%d of %d]\n",
               (j+1), iterations);

        int wh_id = 0;
	stopwatch_t timer;

        for (int i=0; i<numOfThreads; i++) {

            // create & fork testing threads
            wh_id++;
            testers[i] = new test_tree_smt_t(shore_env, wh_id, selectedTrxID, 
                                             updTuple, numOfTrxs, c_str("tt%d", i));
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
               "Threads: (%d)\n" \
               "Trxs:    (%d)\n" \
               "Secs:    (%.2f)\n" \
               "TPS:     (%.2f)\n",
               numOfThreads, numOfTrxs, delay, numOfThreads*numOfTrxs/delay);
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
               | TRACE_DEBUG
              );

    /* 1. Instanciate the Shore environment */
    if (inst_env(argc, argv))
        return (1);


    /* 2. Start processing commands */
    tree_test_shell_t tshell("(tree) ");
    tshell.start();

    /* 3. Close the Shore environment */
    if (shore_env)
        close_env();

    return (0);
}


void print_wh_usage(char* argv[]) 
{
    TRACE( TRACE_ALWAYS, "\nUsage:\n" \
           "%s <NUM_WHS>\n" \
           "\nParameters:\n" \
           "<NUM_WHS>     : The number of WHs of the DB (database scaling factor)\n" \
           ,argv[0]);
}

// Instanciate the Shore environment, 
// Opens the database and sets the appropriate number of WHs
// Returns 1 on error
int inst_env(int argc, char* argv[]) 
{
    /* 1. Parse numOfWHs */
    if (argc<2) {
        print_wh_usage(argv);
        return (1);
    }

    int tmp_numOfWHs = atoi(argv[1]);
    if (tmp_numOfWHs>0)
        _numOfWHs = tmp_numOfWHs;

    /* 2. Initialize Shore environment */
    /* 1. Instanciate the Shore Environment */
    shore_env = new ShoreTPCCEnv("shore.conf", _numOfWHs, _numOfWHs);


    // the initialization must be executed in a shore context
    db_init_smt_t* initializer = new db_init_smt_t(c_str("init"), shore_env);
    initializer->fork();
    initializer->join();        
    if (initializer) {
        delete (initializer);
        initializer = NULL;
    }    
    shore_env->print_sf();


    return (0);
}


int close_env() {

    // close Shore env
    close_smt_t* clt = new close_smt_t(shore_env, c_str("clt"));
    clt->fork();
    clt->join();
    if (clt->_rv) {
        TRACE( TRACE_ALWAYS, "Error in closing thread...\n");
        return (1);
    }

    if (clt) {
        delete (clt);
        clt = NULL;
    }

    return (0);
}

