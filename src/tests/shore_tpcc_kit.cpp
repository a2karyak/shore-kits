/* -*- mode:C++; c-basic-offset:4 -*- */
// CC -m64 -xarch=ultraT1 -xs -g -I $SHORE_INCLUDE_DIR shore_tpcc_load.cpp -o shore_tpcc_load -L $SHORE_LIB_DIR -mt -lsm -lsthread -lfc -lcommon -lpthread

#include "tests/common.h"
#include "stages/tpcc/shore/shore_tpcc_env.h"
#include "sm/shore/shore_helper_loader.h"


using namespace shore;
using namespace tpcc;


// default database size (scaling factor)
const int DF_NUM_OF_WHS = 10;

// default queried number of warehouses (queried factor)
const int DF_NUM_OF_QUERIED_WHS = 10;

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

    tpcc_random_gen_t _tpccrnd; // (ip) deprecated

public:
    int	_rv;
    
    test_smt_t(ShoreTPCCEnv* env, 
               int sWH, int trxId, int numOfTrxs, 
               c_str tname) 
	: thread_t(tname), 
          _env(env), _wh(sWH), _trxid(trxId), _notrxs(numOfTrxs), _rv(0)
    {
        assert (_env);
        assert (_notrxs);
        assert (_wh>=0);

        _tpccrnd = tpcc_random_gen_t(NULL); // (ip) deprecated
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
        xct_type = _tpccrnd.random_xct_type(rand()%100);
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


//////////////////////////////


//////////////////////////////


const char* translate_trx_id(const int trx_id) 
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
    default:
        return ("Mix");
    }
}

void print_usage(char* argv[]) 
{
    TRACE( TRACE_ALWAYS, "\nUsage:\n" \
           "%s <NUM_WHS> <NUM_QUERIED> [<SPREAD> <NUM_THRS> <NUM_TRXS> <TRX_ID>]\n" \
           "\nParameters:\n" \
           "<NUM_WHS>     : The number of WHs of the DB (scaling factor)\n" \
           "<NUM_QUERIED> : The number of WHs queried (queried factor)\n" \
           "<SPREAD>      : Whether to spread threads to WHs (0=No, Otherwise=Yes, Default=No) (optional)\n" \
           "<NUM_THRS>    : Number of threads used (optional)\n" \
           "<NUM_TRXS>    : Number of transactions per thread (optional)\n" \
           "<TRX_ID>      : Transaction ID to be executed (0=mix) (optional)\n" \
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
               //               | TRACE_DEBUG
              );


    /* 0. Parse Parameters */
    int numOfWHs        = DF_NUM_OF_WHS;    
    int numOfQueriedWHs = DF_NUM_OF_QUERIED_WHS;
    int spreadThreads   = DF_SPREAD_THREADS_TO_WHS;
    int numOfThreads    = DF_NUM_OF_THR;
    int numOfTrxs       = DF_TRX_PER_THR;
    int selectedTrxID   = DF_TRX_ID;
    int iterations      = DF_NUM_OF_ITERS;

    if (argc<3) {
        print_usage(argv);
        return (1);
    }

    int tmp = atoi(argv[1]);
    if (tmp>0)
        numOfWHs = tmp;

    tmp = atoi(argv[2]);
    if (tmp>0)
        numOfQueriedWHs = tmp;
    assert (numOfQueriedWHs <= numOfWHs);

    if (argc>3)
        spreadThreads = atoi(argv[3]);
    
    if (argc>4) {
        tmp = atoi(argv[4]);
        if ((tmp>0) && (tmp<=MAX_NUM_OF_THR)) {
            numOfThreads = tmp;
            if (spreadThreads && (tmp > numOfQueriedWHs))
                numOfThreads = numOfQueriedWHs;
        }
    }

    if (argc>5) {
        tmp = atoi(argv[5]);
        if (tmp>0)
            numOfTrxs = tmp;
    }

    if (argc>6) {
        selectedTrxID = atoi(argv[6]);
    }

    if (argc>7) {
        iterations = atoi(argv[7]);
    }

    // Print out configuration
    TRACE( TRACE_ALWAYS, "\n\n" \
           "Num of WHs     : %d\n" \
           "Queried WHs    : %d\n" \
           "Spread Threads : %s\n" \
           "Num of Threads : %d\n" \
           "Num of Trxs    : %d\n" \
           "Trx            : %s\n" \
           "Iterations     : %d\n", 
           numOfWHs, numOfQueriedWHs, (spreadThreads ? "Yes" : "No"), 
           numOfThreads, numOfTrxs, translate_trx_id(selectedTrxID), 
           iterations);

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
    
    test_smt_t* testers[MAX_NUM_OF_THR];
    for (int j=0; j<iterations; j++) {

        TRACE( TRACE_ALWAYS, "Iteration [%d of %d]\n",
               (j+1), iterations);
        TRACE( TRACE_ALWAYS, "Starting (%d) threads with (%d) trxs each...\n",
               numOfThreads, numOfTrxs);

	stopwatch_t timer;
        int wh_id = 0;
        for (int i=0; i<numOfThreads; i++) {
            // create & fork (numOfThreads) testing threads
            if (spreadThreads)
                wh_id = i+1;
            testers[i] = new test_smt_t(shore_env, wh_id, selectedTrxID,
                                        numOfTrxs, c_str("tt%d", i));
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
