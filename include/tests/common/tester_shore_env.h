/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   tester_shore_env.h
 *
 *  @brief:  Defines common tester functions and variables related to the 
 *           Shore environment.
 *
 *  @author: Ippokratis Pandis, July 2008
 *
 */

#ifndef __TESTER_SHORE_ENV_H
#define __TESTER_SHORE_ENV_H


#include "stages/tpcc/shore/shore_tpcc_env.h"
#include "sm/shore/shore_helper_loader.h"


using namespace shore;
using namespace tpcc;


//// Default values for the environment

// default database size (scaling factor)
const int DF_NUM_OF_WHS = 10;
extern int _numOfWHs;


// default queried number of warehouses (queried factor)
const int DF_NUM_OF_QUERIED_WHS = 10;

// Instanciate and close the Shore environment
int inst_test_env(int argc, char* argv[]);
int close_test_env();

// Helper
void print_wh_usage(char* argv[]);



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



#endif /** __TESTER_SHORE_ENV_H */
