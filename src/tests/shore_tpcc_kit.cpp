/* -*- mode:C++; c-basic-offset:4 -*- */
// CC -m64 -xarch=ultraT1 -xs -g -I $SHORE_INCLUDE_DIR shore_tpcc_load.cpp -o shore_tpcc_load -L $SHORE_LIB_DIR -mt -lsm -lsthread -lfc -lcommon -lpthread

#include "tests/common.h"
#include "stages/tpcc/shore/shore_tpcc_env.h"
#include "sm/shore/shore_helper_loader.h"


using namespace shore;
using namespace tpcc;


// default number of threads
#define DF_NUM_OF_THR  10

// default number of transactions executed per thread
#define DF_TRX_PER_THR 100


///////////////////////////////////////////////////////////
// @class test_smt_t
//
// @brief An smthread-based class for tests

class test_smt_t : public thread_t {
private:
    ShoreTPCCEnv* _env;    
    tpcc_random_gen_t _tpccrnd;         

public:
    int	_rv;
    
    test_smt_t(ShoreTPCCEnv* env, c_str tname) 
	: thread_t(tname), 
          _env(env), _rv(0)
    {
        TRACE( TRACE_ALWAYS, "Hello...\n");
        _tpccrnd = tpcc_random_gen_t(NULL);
    }


    ~test_smt_t() {
        TRACE( TRACE_ALWAYS, "Bye...\n");
    }



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

    w_rc_t tpcc_run_xct(ShoreTPCCEnv* env, int xct_type = 0, int num_xct = DF_TRX_PER_THR);
    w_rc_t tpcc_run_one_xct(ShoreTPCCEnv* env, int xct_type = 0, int xctid = 0);    

    w_rc_t xct_new_order(ShoreTPCCEnv* env, int xctid);
    w_rc_t xct_payment(ShoreTPCCEnv* env, int xctid);
    w_rc_t xct_order_status(ShoreTPCCEnv* env, int xctid);
    w_rc_t xct_delivery(ShoreTPCCEnv* env, int xctid);
    w_rc_t xct_stock_level(ShoreTPCCEnv* env, int xctid);

    void print_tables();


    // methods
    int test() {
        W_DO(_env->loaddata());
        //_env->check_consistency();
        W_DO(tpcc_run_xct(_env, XCT_PAYMENT)); // run only PAYMENT
        print_tables();
        return (0);
    }

    /** @note Those two functions should be implemented by every
     *        smthread-inherited class that runs using run_smthread()
     */
    inline int retval() { return (_rv); }

}; // EOF: test_smt_t



w_rc_t test_smt_t::xct_new_order(ShoreTPCCEnv* env, int xctid) 
{ 
    assert (env);
    trx_result_tuple_t atrt;
    env->run_new_order(xctid, atrt);    
    return (RCOK); 
}

w_rc_t test_smt_t::xct_payment(ShoreTPCCEnv* env, int xctid) 
{ 
    assert (env);
    trx_result_tuple_t atrt;
    env->run_payment(xctid, atrt);    
    return (RCOK); 
}

w_rc_t test_smt_t::xct_order_status(ShoreTPCCEnv* env, int xctid) 
{ 
    assert (env);
    trx_result_tuple_t atrt;
    env->run_order_status(xctid, atrt);    
    return (RCOK); 
}


w_rc_t test_smt_t::xct_delivery(ShoreTPCCEnv* env, int xctid) 
{ 
    assert (env);
    trx_result_tuple_t atrt;
    env->run_delivery(xctid, atrt);    
    return (RCOK); 
}


w_rc_t test_smt_t::xct_stock_level(ShoreTPCCEnv* env, int xctid) 
{ 
    assert (env);
    trx_result_tuple_t atrt;
    env->run_stock_level(xctid, atrt);    
    return (RCOK); 
}


void test_smt_t::print_tables() 
{
    _env->dump();
}


w_rc_t test_smt_t::tpcc_run_xct(ShoreTPCCEnv* env, int xct_type, int num_xct)
{
    for (int i=0; i<num_xct; i++) {
        //        TRACE( TRACE_DEBUG, "%d . ", i);
        tpcc_run_one_xct(env, xct_type, i);
        sleep(10);
    }
    return (RCOK);
}


 
w_rc_t test_smt_t::tpcc_run_one_xct(ShoreTPCCEnv* env, int xct_type, int xctid) 
{
    int  this_type = xct_type;
    if (this_type == 0) {        
        this_type = _tpccrnd.random_xct_type(rand()%100);
    }
    
    switch (this_type) {
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



// uncomment below to run 4 threads concurrently
//#define USE_MT_TPCC_THREADS


int main(int argc, char* argv[]) 
{
    // initialize cordoba threads
    thread_init();

    // Instanciate the Shore Environment
    shore_env = new ShoreTPCCEnv("shore.conf", 1, 1);

    int numOfThreads = DF_NUM_OF_THR;
    int numOfTrxs    = DF_TRX_PER_THR;

    if (argc>1) {
        int tmp_thr = atoi(argv[1]);
        assert (tmp_thr);
        numOfThreads = tmp_thr;
    }
    if (argc>2) {
        int tmp_trx = atoi(argv[2]);
        assert (tmp_trx);
        numOfTrxs = tmp_trx;
    }
    
    // Load data to the Shore Database
    TRACE( TRACE_ALWAYS, "Starting (%d) threads (%d) trxs each...\n");
    guard<test_smt_t> tt1 = new test_smt_t(shore_env, c_str("tt1"));

#ifdef USE_MT_TPCC_THREADS 
    guard<test_smt_t> tt2 = new test_smt_t(shore_env, c_str("tt2"));
    guard<test_smt_t> tt3 = new test_smt_t(shore_env, c_str("tt3"));
    guard<test_smt_t> tt4 = new test_smt_t(shore_env, c_str("tt4"));
#endif

    /* 1. fork the loading threads */
    tt1->fork();

#ifdef USE_MT_TPCC_THREADS
    tt2->fork();
    tt3->fork();
    tt4->fork();
#endif    

    /* 2. join the loading threads */
    tt1->join();        
    if (tt1->_rv) {
        TRACE( TRACE_ALWAYS, "Error in testing...\n");
        TRACE( TRACE_ALWAYS, "Exiting...\n");
        assert (false);
    }    

#ifdef USE_MT_TPCC_THREADS
    tt2->join();        
    if (tt2->_rv) {
        TRACE( TRACE_ALWAYS, "Error in testing...\n");
        TRACE( TRACE_ALWAYS, "Exiting...\n");
        assert (false);
    }    

    tt3->join();        
    if (tt3->_rv) {
        TRACE( TRACE_ALWAYS, "Error in testing...\n");
        TRACE( TRACE_ALWAYS, "Exiting...\n");
        assert (false);
    }    

    tt4->join();        
    if (tt4->_rv) {
        TRACE( TRACE_ALWAYS, "Error in testing...\n");
        TRACE( TRACE_ALWAYS, "Exiting...\n");
        assert (false);
    }    
#endif


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
