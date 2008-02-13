/* -*- mode:C++; c-basic-offset:4 -*- */
// CC -m64 -xarch=ultraT1 -xs -g -I $SHORE_INCLUDE_DIR shore_tpcc_load.cpp -o shore_tpcc_load -L $SHORE_LIB_DIR -mt -lsm -lsthread -lfc -lcommon -lpthread

#include "tests/common.h"
#include "stages/tpcc/common/tpcc_payment_common.h"
#include "stages/tpcc/shore/shore_tpcc_env.h"


using namespace shore;
using namespace tpcc;


///////////////////////////////////////////////////////////
// @class test_smt_t
//
// @brief An smthread-based class for tests

class test_smt_t : public smthread_t {
private:
    ShoreTPCCEnv* _env;    
    c_str _tname;
    tpcc_random_gen_t _tpccrnd;         

public:
    int	_rv;
    
    test_smt_t(ShoreTPCCEnv* env, c_str tname) 
	: smthread_t(t_regular), 
          _env(env), _tname(tname), _rv(0)
    {
        cout << "Hello... " << endl;
        _tpccrnd = tpcc_random_gen_t(NULL);
    }


    ~test_smt_t() {
        cout << "Bye... " << endl;
    }


    // thread entrance
    void run() {
        if (!_env->is_initialized()) {
            if (_env->init()) {
                // Couldn't initialize the Shore environment
                // cannot proceed
                cout << "Couldn't initialize Shore " << endl;
                _rv = 1;
                return;
            }
        }

        // run test
        _rv = test();
    }

    w_rc_t tpcc_run_xct(ShoreTPCCEnv* env, int num_xct = 10, int xct_type = 0);
    w_rc_t tpcc_run_one_xct(ShoreTPCCEnv* env, int xct_type = 0, int xctid = 0);    

    w_rc_t xct_new_order(ShoreTPCCEnv* env, int xctid);
    w_rc_t xct_payment(ShoreTPCCEnv* env, int xctid);
    w_rc_t xct_order_status(ShoreTPCCEnv* env, int xctid);
    w_rc_t xct_delivery(ShoreTPCCEnv* env, int xctid);
    w_rc_t xct_stock_level(ShoreTPCCEnv* env, int xctid);

    void print_tables();


    // methods
    int test() {
        _env->loaddata();
        //_env->check_consistency();
        tpcc_run_xct(_env);
        //print_tables();
        return (0);
    }

    /** @note Those two functions should be implemented by every
     *        smthread-inherited class that runs using run_smthread()
     */
    inline int retval() { return (_rv); }
    inline c_str tname() { return (_tname); }

}; // EOF: test_smt_t



w_rc_t test_smt_t::xct_new_order(ShoreTPCCEnv* env, int xctid) 
{ cout << "NEW_ORDER... " << endl; return (RCOK); }

w_rc_t test_smt_t::xct_order_status(ShoreTPCCEnv* env, int xctid) 
{ cout << "ORDER_STATUS... " << endl; return (RCOK); }

w_rc_t test_smt_t::xct_delivery(ShoreTPCCEnv* env, int xctid) 
{ cout << "DELIVERY... " << endl; return (RCOK); }

w_rc_t test_smt_t::xct_stock_level(ShoreTPCCEnv* env, int xctid) 
{ cout << "STOCK_LEVEL... " << endl; return (RCOK); }


w_rc_t test_smt_t::xct_payment(ShoreTPCCEnv* env, int xctid) 
{ 
    assert (env);
    cout << "PAYMENT... " << endl;    

    payment_input_t pin = create_payment_input();
    trx_result_tuple_t trt;
    
    w_rc_t e = _env->xct_payment(&pin, xctid, trt);

    return (RCOK); 
}


void test_smt_t::print_tables() {

    /* describes all the tables */
    _env->warehouse()->print_table(_env->db());
    _env->district()->print_table(_env->db());
    _env->customer()->print_table(_env->db());
    _env->history()->print_table(_env->db());
    _env->new_order()->print_table(_env->db());
    _env->order()->print_table(_env->db());
    _env->orderline()->print_table(_env->db());
    _env->item()->print_table(_env->db());
    _env->stock()->print_table(_env->db());
}


w_rc_t test_smt_t::tpcc_run_xct(ShoreTPCCEnv* env, int num_xct, int xct_type)
{
    for (int i=0; i<num_xct; i++) {
        cout << i << ". ";
        tpcc_run_one_xct(env, xct_type, i);
    }
    return (RCOK);
}


 
w_rc_t test_smt_t::tpcc_run_one_xct(ShoreTPCCEnv* env, int xct_type, int xctid) 
{
    int  this_type = xct_type;
    if (this_type == 0) {        
        this_type = _tpccrnd.random_xct_type();
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



///////////////////////////////////////////////////////////
// @class close_smt_t
//
// @brief An smthread-based class for tests

class close_smt_t : public smthread_t {
private:
    ShoreTPCCEnv* _env;    
    c_str _tname;

public:
    int	_rv;
    
    close_smt_t(ShoreTPCCEnv* env, c_str tname) 
	: smthread_t(t_regular), 
          _env(env), _tname(tname), _rv(0)
    {
    }

    ~close_smt_t() {
    }


    // thread entrance
    void run() {
        assert (_env);
        if (_env) {
            delete (_env);
            _env = NULL;
        }        
    }


    /** @note Those two functions should be implemented by every
     *        smthread-inherited class that runs using run_smthread()
     */
    inline int retval() { return (_rv); }
    inline c_str tname() { return (_tname); }

}; // EOF: close_smt_t



int main(int argc, char* argv[]) 
{
    // Instanciate the Shore Environment
    shore_env = new ShoreTPCCEnv("shore.conf", 1, 1);
    
    // Load data to the Shore Database
    int* r = NULL;
    TRACE( TRACE_ALWAYS, "Starting...\n");
    test_smt_t* tt = new test_smt_t(shore_env, c_str("tt"));
    run_smthread<test_smt_t,int>(tt, r);

    if (*r) {
        cerr << "Error in loading... " << endl;
        cerr << "Exiting... " << endl;
        return (1);
    }

    if (r) {
        delete (r);
        r = NULL;
    }

    if (tt) {
        delete (tt);
        tt = NULL;
    }

    // close Shore env
    close_smt_t* clt = new close_smt_t(shore_env, c_str("clt"));
    run_smthread<close_smt_t,int>(clt, r);

    if (*r) {
        cerr << "Error in loading... " << endl;
        cerr << "Exiting... " << endl;
        return (1);
    }

    if (r) {
        delete (r);
        r = NULL;
    }

    if (clt) {
        delete (clt);
        clt = NULL;
    }



    return (0);
}
