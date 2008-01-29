/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file shore_tpcc_env.cpp
 *
 *  @brief Declaration of the Shore TPC-C environment (database)
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#include "stages/tpcc/shore/shore_tpcc_env.h"


using namespace shore;

ENTER_NAMESPACE(tpcc);


/** Exported variables */

ShoreTPCCEnv* shore_env;


/** Exported functions */


/********
 ******** Caution: The functions below should be invoked inside
 ********          the context of a smthread
 ********/


int ShoreTPCCEnv::loaddata() 
{
    assert (false); // (ip) TODO
    return (0);
}


w_rc_t xct_new_order(new_order_input_t* no_input, const int xct_id) { return (RCOK); }
w_rc_t xct_payment(payment_input_t * pay_input, const int xct_id) { return (RCOK); }
w_rc_t xct_order_status(order_status_input_t* status_input, const int xct_id) { return (RCOK); }
w_rc_t xct_delivery(delivery_input_t* deliv_input, const int xct_id) { return (RCOK); }
w_rc_t xct_stock_level(stock_level_input_t* level_input, const int xct_id) { return (RCOK); }



EXIT_NAMESPACE(tpcc);
