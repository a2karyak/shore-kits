/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   tester_shore_env.cpp
 *
 *  @brief:  Implementation of the common tester functions related to the Shore
 *           environment.
 *
 *  @author: Ippokratis Pandis, July 2008
 *
 */

#include "tests/common/tester_shore_env.h"

using namespace shore;


int _numOfWHs = DF_NUM_OF_WHS;



/********************************************************************* 
 *
 *  @fn:      inst_test_env
 *
 *  @brief:   Instanciates the Shore environment, 
 *            Opens the database and sets the appropriate number of WHs
 *  
 *  @returns: 1 on error
 *
 *********************************************************************/

int inst_test_env(int argc, char* argv[]) 
{
    /* 1. Parse numOfWHs */
    if (argc<2) {
        print_wh_usage(argv);
        return (1);
    }

    int tmp_numOfWHs = atoi(argv[1]);
    if (tmp_numOfWHs>0)
        _numOfWHs = tmp_numOfWHs;

    /* 2. Instanciate the Shore Environment */
    _g_shore_env = new ShoreTPCCEnv("shore.conf", _numOfWHs, _numOfWHs);

    // the initialization must be executed in a shore context
    db_init_smt_t* initializer = new db_init_smt_t(c_str("init"), _g_shore_env);
    initializer->fork();
    initializer->join();        
    if (initializer) {
        delete (initializer);
        initializer = NULL;
    }    
    assert (_g_shore_env);
    _g_shore_env->print_sf();

    return (0);
}



/********************************************************************* 
 *
 *  @fn:      close_test_env
 *
 *  @brief:   Close the Shore environment, 
 *  
 *  @returns: 1 on error
 *
 *********************************************************************/

int close_test_env() 
{
    // close Shore env
    close_smt_t* clt = new close_smt_t(_g_shore_env, c_str("clt"));
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


/********************************************************************* 
 *
 *  @fn:    print_wh_usage
 *
 *  @brief: Displays an error message
 *
 *********************************************************************/

void print_wh_usage(char* argv[]) 
{
    TRACE( TRACE_ALWAYS, "\nUsage:\n" \
           "%s <NUM_WHS>\n" \
           "\nParameters:\n" \
           "<NUM_WHS>     : The number of WHs of the DB (database scaling factor)\n" \
           ,argv[0]);
}





/********************************************************************* 
 *
 *  test_smt_t functions
 *
 *********************************************************************/

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




