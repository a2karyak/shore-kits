/* -*- mode:C++; c-basic-offset:4 -*- */
// CC -m64 -xarch=ultraT1 -xs -g -I $SHORE_INCLUDE_DIR shore_tpcc_load.cpp -o shore_tpcc_load -L $SHORE_LIB_DIR -mt -lsm -lsthread -lfc -lcommon -lpthread

// file: shore_tpcc_trees.cpp
// desc: Tests the performance of the Shore B-tree under various consistency modes.


#include "tests/common.h"
#include "stages/tpcc/shore/shore_tpcc_env.h"
#include "sm/shore/shore_helper_loader.h"


using namespace shore;
using namespace tpcc;


// default database size (scaling factor)
const int DF_NUM_OF_WHS = 10;

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

//////////////////////////////


//////////////////////////////


const char* translate_trx_id(const int trx_id) 
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
        assert (false);
        return ("Error!!");
    }
}

void print_usage(char* argv[]) 
{
    TRACE( TRACE_ALWAYS, "\nUsage:\n" \
           "%s <NUM_WHS> <NUM_QUERIED>  <TRX_ID> <UPD_TRX> [<NUM_THREADS> <NUM_TRXS> <ITERATIONS>]\n" \
           "\nParameters:\n" \
           "<NUM_WHS>     : The number of WHs of the DB (scaling factor)\n" \
           "<NUM_QUERIED> : The number of WHs queried (queried factor)\n" \
           "<TRX_ID>      : Transaction ID to be executed (0=mix)\n" \
           "<UPD_TRX>     : Will the trx update the tuple or only probe?\n" \
           "<NUM_THREADS> : Number of concurrent threads (Default=1) (optional)\n" \
           "<NUM_TRXS>    : Number of trxs per thread (Default=100) (optional)\n" \
           "<ITERATIONS>  : Number of iterations (Default=5) (optional)\n",
           argv[0]);
}

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


    /* 0. Parse Parameters */
    int numOfWHs        = DF_NUM_OF_WHS;    
    int numOfQueriedWHs = DF_NUM_OF_QUERIED_WHS;
    int selectedTrxID   = DF_TRX_ID;
    int updTuple        = DF_UPDATE_TUPLE;
    int numOfThreads    = DF_NUM_OF_THREADS;
    int numOfTrxs       = DF_TRX_PER_THR;
    int iterations      = DF_NUM_OF_ITERS;

    if (argc<5) {
        print_usage(argv);
        return (1);
    }

    // required
    int tmp = atoi(argv[1]);
    if (tmp>0)
        numOfWHs = tmp;

    tmp = atoi(argv[2]);
    if (tmp>0)
        numOfQueriedWHs = tmp;
    assert (numOfQueriedWHs <= numOfWHs);

    tmp = atoi(argv[3]);
    if (tmp>0)
        selectedTrxID = tmp;

    tmp = atoi(argv[4]);
    updTuple = tmp;

    // optional
    if (argc>5) {
        tmp = atoi(argv[5]);
        if ((tmp>0) && (tmp<=numOfQueriedWHs) && (tmp<=MAX_NUM_OF_THR)) {
            numOfThreads = tmp;
        }
        else {
            assert (false);
            numOfThreads = numOfQueriedWHs;
        }
    }

    if (argc>6) {
        tmp = atoi(argv[6]);
        if (tmp>0)
            numOfTrxs = tmp;
    }

    if (argc>7) {
        tmp = atoi(argv[7]);
        if (tmp>0)
            iterations = tmp;
    }

    // Print out configuration
    TRACE( TRACE_ALWAYS, "\n\n" \
           "Num of WHs     : %d\n" \
           "Queried WHs    : %d\n" \
           "Trxs ID        : %s\n" \
           "Update Tuple   : %s\n" \
           "Num of Threads : %d\n" \
           "Num of Trxs    : %d\n" \
           "Iterations     : %d\n", 
           numOfWHs, numOfQueriedWHs, translate_trx_id(selectedTrxID), 
           (updTuple ? "Yes" : "No"), numOfThreads, numOfTrxs, iterations);

    /* 1. Instanciate the Shore Environment */
    shore_env = new ShoreTPCCEnv("shore.conf", numOfWHs, numOfQueriedWHs);


    // the initialization must be executed in a shore context
    db_init_smt_t* initializer = new db_init_smt_t(c_str("init"), shore_env);
    initializer->fork();
    initializer->join();        
    if (initializer) {
        delete (initializer);
        initializer = NULL;
    }    
    shore_env->print_sf();
    
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
               "Threads: (%d)\nTrxs:    (%d)\nSecs:    (%.2f)\nTPS:     (%.2f)\n",
               numOfThreads, numOfTrxs, delay, numOfThreads*numOfTrxs/delay);
    }

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
