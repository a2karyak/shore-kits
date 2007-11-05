/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file tpcc_payment_single_thr.cpp
 *
 *  @brief Implements client that submits PAYMENT_BASELINE transaction requests,
 *  according to the TPCC specification, bypassing the staging mechanism.
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#include "scheduler.h"
#include "util.h"

#include "workload/common.h"

#include "workload/tpcc/drivers/inmem/inmem_tpcc_payment_single_thr.h"
#include "workload/tpcc/drivers/tpcc_payment_common.h"


using namespace qpipe;
using namespace workload;
using namespace tpcc;
using namespace tpcc_payment;


/** @fn executeInMemPayment
 *
 *  @brief Execute a Payment transaction
 */

void* inmem_tpcc_payment_single_thr_driver::executeInMemPayment(InMemTPCCEnv* env, int SF) 
{
    payment_input_t pin = create_payment_input(SF);

    trx_result_tuple_t aTrxResultTuple = executeInMemPaymentBaseline(pin,
                                                                     _id, 
                                                                     env);

    TRACE( TRACE_TRX_FLOW, "DONE. NOTIFYING CLIENT\n" );

    return (NULL);
}
