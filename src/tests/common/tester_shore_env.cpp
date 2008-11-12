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
    // 1. Instanciate the Shore Environment
    _g_shore_env = new ShoreTPCCEnv(SHORE_CONF_FILE);

    /* 2. Initialize the Shore Environment - sets also SF */
    // the initialization must be executed in a shore context
    db_init_smt_t* initializer = new db_init_smt_t(c_str("init"), _g_shore_env);
    assert (initializer);
    initializer->fork();
    initializer->join(); 
    int rv = initializer->rv();

    delete (initializer);
    initializer = NULL;

    if (rv) {
        TRACE( TRACE_ALWAYS, "Exiting...\n");
        return (rv);
    }

    assert (_g_shore_env);
    _g_shore_env->print_sf();
    _numOfWHs = _g_shore_env->get_sf();

    /* 2. If param numOfQueriedWHs */
    if (argc>1) {
        int numQueriedOfWHs = atoi(argv[1]);
        _g_shore_env->set_qf(numQueriedOfWHs);
    }
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
           "%s [<NUM_QUERIED_WHS>]\n" \
           "\nParameters:\n" \
           "<NUM_QUERIED_WHS>     : (optional) The number of WHs of the DB (database scaling factor)\n" \
           ,argv[0]);
}


/********************************************************************* 
 *
 *  @fn:    print_tables
 *
 *  @brief: Dumps the contents of all the tables 
 *
 *  @note:  Used only for debugging
 *
 *********************************************************************/

void client_smt_t::print_tables() 
{
    _env->dump();
}


/********************************************************************* 
 *
 *  @fn:    abort/resume_test
 *
 *  @brief: Used for SIGINT functionality
 *
 *********************************************************************/

static bool _abort_test = false;
void client_smt_t::abort_test() {
    _abort_test = true;
}
void client_smt_t::resume_test() {
    _abort_test = false;
}



/********************************************************************* 
 *
 *  @fn:    submit_batch
 *
 *  @brief: Submits a batch of trxs and always uses the last trx to
 *          wait on its cond var
 *
 *********************************************************************/

void client_smt_t::submit_batch(ShoreTPCCEnv* env, int xct_type, 
                              int& trx_cnt, const int batch_sz) 
{       
    assert (batch_sz);
    for(int j=1; j <= batch_sz; j++) {
        if (j == batch_sz) {
            _cp->take_one = true;
            _cp->index = 1-_cp->index;
        }
        run_one_tpcc_xct(env, xct_type, trx_cnt++);
        _cp->take_one = false;
    }
}


/********************************************************************* 
 *
 *  @fn:    run_xcts
 *
 *  @brief: Entry point for running the trxs 
 *
 *********************************************************************/

w_rc_t client_smt_t::run_xcts(ShoreTPCCEnv* env, int xct_type, int num_xct)
{
    int i=0;

    switch (_measure_type) {

        // case of number-of-trxs-based measurement
    case (MT_NUM_OF_TRXS):

        // submit a batch of num_xct trxs
        submit_batch(env, xct_type, i, num_xct);
        // wait for the batch to complete
        // note: in the test case there is no double buffering
        _cp->wait[_cp->index].wait();
        break;

        // case of duration-based measurement
    case (MT_TIME_DUR):

        // retrieve the default batch size
        int batchsz = envVar::instance()->getVarInt("cl-batchsz",BATCH_SIZE);
	
	// submit the first two batches...
	submit_batch(env, xct_type, i, batchsz);
	submit_batch(env, xct_type, i, batchsz);

	// main loop
        while (true) {
	    // wait for the first to complete
	    _cp->wait[1-_cp->index].wait();

	    // check for exit...
	    if(_abort_test || _env->get_measure() == MST_DONE)
		break;

	    // submit a replacement batch...
	    submit_batch(env, xct_type, i, batchsz);
        }
	
	// wait for the last batch to complete...
	_cp->wait[_cp->index].wait();	
        break;
    }

    return (RCOK);
}



/********************************************************************* 
 *
 *  @fn:    run_one_tpcc_xct
 *
 *  @brief: Entry point for running one trx 
 *
 *  @note:  The execution of this trx will not be stopped even if the
 *          measure internal has expired.
 *
 *********************************************************************/
 
w_rc_t client_smt_t::run_one_tpcc_xct(ShoreTPCCEnv* env, int xct_type, int xctid) 
{
    // if BASELINE TPC-C MIX
    if (xct_type == XCT_MIX) {        
        xct_type = random_xct_type(rand(100));
    }

    // if DORA TPC-C MIX
    if (xct_type == XCT_DORA_MIX) {        
        xct_type = XCT_DORA_MIX + random_xct_type(rand(100));
    }
    
    switch (xct_type) {
        
        // TPC-C BASELINE
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
        // MBENCH BASELINE
    case XCT_MBENCH_WH:
        W_DO(xct_mbench_wh(env, xctid)); break;
    case XCT_MBENCH_CUST:
        W_DO(xct_mbench_cust(env, xctid)); break;

        // TPC-C DORA
    case XCT_DORA_NEW_ORDER:
        W_DO(xct_dora_new_order(env, xctid));  break;
    case XCT_DORA_PAYMENT:
        W_DO(xct_dora_payment(env, xctid)); break;
    case XCT_DORA_ORDER_STATUS:
        W_DO(xct_dora_order_status(env, xctid)); break;
    case XCT_DORA_DELIVERY:
        W_DO(xct_dora_delivery(env, xctid)); break;
    case XCT_DORA_STOCK_LEVEL:
        W_DO(xct_dora_stock_level(env, xctid)); break;
        // MBENCH DORA
    case XCT_DORA_MBENCH_WH:
        W_DO(xct_dora_mbench_wh(env, xctid)); break;
    case XCT_DORA_MBENCH_CUST:
        W_DO(xct_dora_mbench_cust(env, xctid)); break;
    }
    return (RCOK);
}


/********************************************************************* 
 *
 *  client_smt_t - regular xct functions
 *
 *********************************************************************/

w_rc_t client_smt_t::xct_new_order(ShoreTPCCEnv* env, int xctid) 
{ 
    assert (env);
    trx_result_tuple_t atrt;
    if (_cp->take_one) atrt.set_notify(_cp->wait+_cp->index);
    env->run_new_order(xctid, atrt, _wh);    
    return (RCOK); 
}

w_rc_t client_smt_t::xct_payment(ShoreTPCCEnv* env, int xctid) 
{ 
    assert (env);
    trx_result_tuple_t atrt;
    if (_cp->take_one) 
        atrt.set_notify(_cp->wait+_cp->index);
    env->run_payment(xctid, atrt, _wh);    
    return (RCOK); 
}

w_rc_t client_smt_t::xct_order_status(ShoreTPCCEnv* env, int xctid) 
{ 
    assert (env);
    trx_result_tuple_t atrt;
    if (_cp->take_one) atrt.set_notify(_cp->wait+_cp->index);
    env->run_order_status(xctid, atrt, _wh);    
    return (RCOK); 
}


w_rc_t client_smt_t::xct_delivery(ShoreTPCCEnv* env, int xctid) 
{ 
    assert (env);
    trx_result_tuple_t atrt;
    if (_cp->take_one) atrt.set_notify(_cp->wait+_cp->index);
    env->run_delivery(xctid, atrt, _wh);
    return (RCOK); 
}


w_rc_t client_smt_t::xct_stock_level(ShoreTPCCEnv* env, int xctid) 
{ 
    assert (env);
    trx_result_tuple_t atrt;
    if (_cp->take_one) atrt.set_notify(_cp->wait+_cp->index);
    env->run_stock_level(xctid, atrt, _wh);    
    return (RCOK); 
}



w_rc_t client_smt_t::xct_mbench_wh(ShoreTPCCEnv* env, int xctid) 
{ 
    assert (env);
    trx_result_tuple_t atrt;
    if (_cp->take_one) atrt.set_notify(_cp->wait+_cp->index);
    env->run_mbench_wh(xctid, atrt, _wh);    
    return (RCOK); 
}

w_rc_t client_smt_t::xct_mbench_cust(ShoreTPCCEnv* env, int xctid) 
{ 
    assert (env);
    trx_result_tuple_t atrt;
    if (_cp->take_one) atrt.set_notify(_cp->wait+_cp->index);
    env->run_mbench_cust(xctid, atrt, _wh);    
    return (RCOK); 
}



/********************************************************************* 
 *
 *  client_smt_t - dora xct functions
 *
 *********************************************************************/

w_rc_t client_smt_t::xct_dora_new_order(ShoreTPCCEnv* env, int xctid) 
{ 
    assert (env);
    trx_result_tuple_t atrt;
    if (_cp->take_one) atrt.set_notify(_cp->wait+_cp->index);
    env->dora_new_order(xctid, atrt, _wh);    
    return (RCOK); 
}

w_rc_t client_smt_t::xct_dora_payment(ShoreTPCCEnv* env, int xctid) 
{ 
    assert (env);
    trx_result_tuple_t atrt;
    if (_cp->take_one) atrt.set_notify(_cp->wait+_cp->index);
    env->dora_payment(xctid, atrt, _wh);    
    return (RCOK); 
}

w_rc_t client_smt_t::xct_dora_order_status(ShoreTPCCEnv* env, int xctid) 
{ 
    assert (env);
    trx_result_tuple_t atrt;
    if (_cp->take_one) atrt.set_notify(_cp->wait+_cp->index);
    env->dora_order_status(xctid, atrt, _wh);    
    return (RCOK); 
}


w_rc_t client_smt_t::xct_dora_delivery(ShoreTPCCEnv* env, int xctid) 
{ 
    assert (env);
    trx_result_tuple_t atrt;
    if (_cp->take_one) atrt.set_notify(_cp->wait+_cp->index);
    env->dora_delivery(xctid, atrt, _wh);    
    return (RCOK); 
}


w_rc_t client_smt_t::xct_dora_stock_level(ShoreTPCCEnv* env, int xctid) 
{ 
    assert (env);
    trx_result_tuple_t atrt;
    if (_cp->take_one) atrt.set_notify(_cp->wait+_cp->index);
    env->dora_stock_level(xctid, atrt, _wh);    
    return (RCOK); 
}


w_rc_t client_smt_t::xct_dora_mbench_wh(ShoreTPCCEnv* env, int xctid) 
{ 
    assert (env);
    trx_result_tuple_t atrt;
    if (_cp->take_one) atrt.set_notify(_cp->wait+_cp->index);
    env->dora_mbench_wh(xctid, atrt, _wh);    
    return (RCOK); 
}

w_rc_t client_smt_t::xct_dora_mbench_cust(ShoreTPCCEnv* env, int xctid) 
{ 
    assert (env);
    trx_result_tuple_t atrt;
    if (_cp->take_one) atrt.set_notify(_cp->wait+_cp->index);
    env->dora_mbench_cust(xctid, atrt, _wh);    
    return (RCOK); 
}


/** EOF: client_smt_t functions */




