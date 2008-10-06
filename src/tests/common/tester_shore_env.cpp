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
    /* 1. Instanciate the Shore Environment */
    _g_shore_env = new ShoreTPCCEnv("shore.conf");

    /* 2. Initialize the Shore Environment - sets also SF */
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

void test_smt_t::print_tables() 
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
void test_smt_t::abort_test() {
    _abort_test = true;
}
void test_smt_t::resume_test() {
    _abort_test = false;
}


/********************************************************************* 
 *
 *  @fn:    run_xcts
 *
 *  @brief: Entry point for running the trxs 
 *
 *********************************************************************/
// horrible dirty hack..
static __thread struct condex_pair {
    condex wait[2];
    int index;
    bool take_one;
} my_condex = {
    {
	{PTHREAD_COND_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, false},
	{PTHREAD_COND_INITIALIZER, PTHREAD_MUTEX_INITIALIZER, false},
    },
    0, false
};

w_rc_t test_smt_t::run_xcts(ShoreTPCCEnv* env, int xct_type, int num_xct)
{
    int i=0;

    switch (_measure_type) {

        // case of number-of-trxs-based measurement
    case (MT_NUM_OF_TRXS):
        for (i=0; i<num_xct; i++) {
            if(_abort_test)
                break;
            run_one_tpcc_xct(env, xct_type, i);
        }
        break;

        // case of duration-based measurement
    case (MT_TIME_DUR): {
	static int const BATCH_SIZE = 30;
	struct condex_pair* cp = &my_condex;
	
#define SUBMIT_BATCH()					\
	for(int j=1; j <= BATCH_SIZE; j++) {		\
	    if(j == BATCH_SIZE) {			\
		cp->take_one = true;			\
		cp->index = 1 - cp->index;		\
	    }						\
	    run_one_tpcc_xct(env, xct_type, i++);	\
	    cp->take_one = false;			\
	}
	
	
	// submit the first two batches...
	SUBMIT_BATCH();
	SUBMIT_BATCH();

	// main loop
        while (true) {
	    // wait for the first to complete
	    cp->wait[1-cp->index].wait();

	    // check for exit...
	    if(_abort_test || _env->get_measure() == MST_DONE)
		break;

	    // submit a replacement batch...
	    SUBMIT_BATCH();
        }
	
	// wait for the last batch to complete...
	cp->wait[cp->index].wait();	
        break;
    }
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
 
w_rc_t test_smt_t::run_one_tpcc_xct(ShoreTPCCEnv* env, int xct_type, int xctid) 
{
    if (xct_type == 0) {        
        xct_type = random_xct_type(rand(100));
    }
    
    switch (xct_type) {
        
        // BASELINE TRXS
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


        // DORA TRXS
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
    }

    return (RCOK);
}

/********************************************************************* 
 *
 *  test_smt_t - regular xct functions
 *
 *********************************************************************/

w_rc_t test_smt_t::xct_new_order(ShoreTPCCEnv* env, int xctid) 
{ 
    assert (env);
    trx_result_tuple_t atrt;
    struct condex_pair* cp = &my_condex;
    if(cp->take_one) atrt.set_notify(cp->wait+cp->index);
    env->run_new_order(xctid, atrt, _wh);    
    return (RCOK); 
}

w_rc_t test_smt_t::xct_payment(ShoreTPCCEnv* env, int xctid) 
{ 
    assert (env);
    trx_result_tuple_t atrt;
    struct condex_pair* cp = &my_condex;
    if(cp->take_one) atrt.set_notify(cp->wait+cp->index);
    env->run_payment(xctid, atrt, _wh);    
    return (RCOK); 
}

w_rc_t test_smt_t::xct_order_status(ShoreTPCCEnv* env, int xctid) 
{ 
    assert (env);
    trx_result_tuple_t atrt;
    struct condex_pair* cp = &my_condex;
    if(cp->take_one) atrt.set_notify(cp->wait+cp->index);
    env->run_order_status(xctid, atrt, _wh);    
    return (RCOK); 
}


w_rc_t test_smt_t::xct_delivery(ShoreTPCCEnv* env, int xctid) 
{ 
    assert (env);
    trx_result_tuple_t atrt;
    struct condex_pair* cp = &my_condex;
    if(cp->take_one) atrt.set_notify(cp->wait+cp->index);
    env->run_delivery(xctid, atrt, _wh);
    return (RCOK); 
}


w_rc_t test_smt_t::xct_stock_level(ShoreTPCCEnv* env, int xctid) 
{ 
    assert (env);
    trx_result_tuple_t atrt;
    struct condex_pair* cp = &my_condex;
    if(cp->take_one) atrt.set_notify(cp->wait+cp->index);
    env->run_stock_level(xctid, atrt, _wh);    
    return (RCOK); 
}



/********************************************************************* 
 *
 *  test_smt_t - dora xct functions
 *
 *********************************************************************/

w_rc_t test_smt_t::xct_dora_new_order(ShoreTPCCEnv* env, int xctid) 
{ 
    assert (env);
    trx_result_tuple_t atrt;
    struct condex_pair* cp = &my_condex;
    if(cp->take_one) atrt.set_notify(cp->wait+cp->index);
    env->dora_new_order(xctid, atrt, _wh);    
    return (RCOK); 
}

w_rc_t test_smt_t::xct_dora_payment(ShoreTPCCEnv* env, int xctid) 
{ 
    assert (env);
    trx_result_tuple_t atrt;
    struct condex_pair* cp = &my_condex;
    if(cp->take_one) atrt.set_notify(cp->wait+cp->index);
    env->dora_payment(xctid, atrt, _wh);    
    return (RCOK); 
}

w_rc_t test_smt_t::xct_dora_order_status(ShoreTPCCEnv* env, int xctid) 
{ 
    assert (env);
    trx_result_tuple_t atrt;
    struct condex_pair* cp = &my_condex;
    if(cp->take_one) atrt.set_notify(cp->wait+cp->index);
    env->dora_order_status(xctid, atrt, _wh);    
    return (RCOK); 
}


w_rc_t test_smt_t::xct_dora_delivery(ShoreTPCCEnv* env, int xctid) 
{ 
    assert (env);
    trx_result_tuple_t atrt;
    struct condex_pair* cp = &my_condex;
    if(cp->take_one) atrt.set_notify(cp->wait+cp->index);
    env->dora_delivery(xctid, atrt, _wh);    
    return (RCOK); 
}


w_rc_t test_smt_t::xct_dora_stock_level(ShoreTPCCEnv* env, int xctid) 
{ 
    assert (env);
    trx_result_tuple_t atrt;
    struct condex_pair* cp = &my_condex;
    if(cp->take_one) atrt.set_notify(cp->wait+cp->index);
    env->dora_stock_level(xctid, atrt, _wh);    
    return (RCOK); 
}



/** EOF: test_smt_t functions */




