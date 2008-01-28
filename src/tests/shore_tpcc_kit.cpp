/* -*- mode:C++; c-basic-offset:4 -*- */
// CC -m64 -xarch=ultraT1 -xs -g -I $SHORE_INCLUDE_DIR shore_tpcc_load.cpp -o shore_tpcc_load -L $SHORE_LIB_DIR -mt -lsm -lsthread -lfc -lcommon -lpthread

#include "tests/common.h"

#include "stages/tpcc/shore/shore_env.h"
#include "stages/tpcc/shore/shore_tpcc_random.h"
#include "stages/tpcc/shore/shore_tpcc_schema.h"
#include "stages/tpcc/shore/shore_tpcc_env.h"


using namespace tpcc;
using namespace shore;



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

    w_rc_t tpcc_run_xct(int num_xct = 100, int xct_type = 0);
    w_rc_t tpcc_run_one_xct(int xct_type = 0);    

    w_rc_t xct_new_order(ss_m* shore, istream& is = cin);
    w_rc_t xct_payment(ss_m* shore, istream& is = cin);
    w_rc_t xct_order_status(ss_m* shore, istream& is = cin);
    w_rc_t xct_delivery(ss_m* shore, istream& is = cin);
    w_rc_t xct_stock_level(ss_m* shore, istream& is = cin);

    void print_tables();


    // methods
    int test() {
        tpcc_run_xct();
        print_tables();
        return (0);
    }

    /** @note Those two functions should be implemented by every
     *        smthread-inherited class that runs using run_smthread()
     */
    inline int retval() { return (_rv); }
    inline c_str tname() { return (_tname); }

}; // EOF: test_smt_t



w_rc_t test_smt_t::xct_new_order(ss_m * shore, istream & is) { cout << "NEW_ORDER... " << endl; return (RCOK); }
w_rc_t test_smt_t::xct_payment(ss_m * shore, istream & is) { cout << "PAYMENT... " << endl; return (RCOK); }
w_rc_t test_smt_t::xct_order_status(ss_m * shore, istream & is) { cout << "ORDER_STATUS... " << endl; return (RCOK); }
w_rc_t test_smt_t::xct_delivery(ss_m * shore, istream & is) { cout << "DELIVERY... " << endl; return (RCOK); }
w_rc_t test_smt_t::xct_stock_level(ss_m * shore, istream & is) { cout << "STOCK_LEVEL... " << endl; return (RCOK); }


void test_smt_t::print_tables() {

    /* describes all the tables */
    _warehouse.print_table(_env->db());
    _district.print_table(_env->db());
    _customer.print_table(_env->db());
    _history.print_table(_env->db());
    _new_order.print_table(_env->db());
    _order.print_table(_env->db());
    _order_line.print_table(_env->db());
    _item.print_table(_env->db());
    _stock.print_table(_env->db());
}


w_rc_t test_smt_t::tpcc_run_xct(int num_xct, int xct_type)
{
    for (int i=0; i<num_xct; i++) {
        cout << i << ". ";
        tpcc_run_one_xct(xct_type);
    }
    return (RCOK);
}



w_rc_t test_smt_t::tpcc_run_one_xct(int xct_type) 
{
    int  this_type = xct_type;

    if (this_type == 0) {

        this_type = _tpccrnd.random_xct_type();
    }
    
    switch (this_type) {
    case XCT_NEW_ORDER:
        W_DO(xct_new_order(_env->db()));  break;
    case XCT_PAYMENT:
        W_DO(xct_payment(_env->db())); break;
    case XCT_ORDER_STATUS:
        W_DO(xct_order_status(_env->db())); break;
    case XCT_DELIVERY:
        W_DO(xct_delivery(_env->db())); break;
    case XCT_STOCK_LEVEL:
        W_DO(xct_stock_level(_env->db())); break;
    }
    return RCOK;
}



int main(int argc, char* argv[]) 
{
    // Instanciate the Shore Environment
    shore_env = new ShoreTPCCEnv("shore.conf");
    
    // Load data to the Shore Database
    int* r = NULL;
    TRACE( TRACE_ALWAYS, "Starting... ");
    test_smt_t* tt = new test_smt_t(shore_env, c_str("tt"));
    run_smthread<test_smt_t,int>(tt, r);
    //     if (*r) {
    //         cerr << "Error in loading... " << endl;
    //         cerr << "Exiting... " << endl;
    //         return (1);
    //     }
    delete (tt);
    return (0);
}
