/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file pay_impl.cpp
 *
 *  @brief Implementation of tpcc-payment-related actions
 *
 *  @author Ippokratis Pandis (ipandis)
 */


#include "dora/tpcc/pay_impl.h"

using namespace dora;



/****************************************************************** 
 *
 * @fn:    pay_upd_wh_impl::trx_exec()
 *
 * @brief: Update warehouse
 *
 ******************************************************************/

w_rc_t pay_upd_wh_impl::trx_exec() 
{  
    TRACE( TRACE_ALWAYS, "executing...\n");

    assert (0); // TODO

    assert (_pmyworker);
    assert (_pmypartition);

    // WH UPDATE

    return (RCOK);
}
