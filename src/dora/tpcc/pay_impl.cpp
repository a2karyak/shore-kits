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
    
    trx_result_tuple_t atrt;
    w_rc_t e = _ptpcc_env->staged_pay_updateShoreWarehouse(_ppin, 0, atrt);

    if (atrt.get_state() == POISSONED) {
        TRACE( TRACE_ALWAYS, 
               "Error in Warehouse Update...\n");
        set_decision(AD_ABORT);
        return (e);
    }    

    set_decision(AD_COMMIT);
    return (RCOK);
}
