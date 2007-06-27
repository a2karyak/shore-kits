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

#include "workload/tpcc/drivers/tpcc_payment_single_thr.h"


using namespace qpipe;
using namespace tpcc;


ENTER_NAMESPACE(workload);


void tpcc_payment_single_thr_driver::allocate_dbts() {

    assert (!_allocated);

    TRACE( TRACE_TRX_FLOW, "Allocating Dbts...\n");

    allocate_payment_dbts(&_dbts);
    _allocated = 1;
}


void tpcc_payment_single_thr_driver::deallocate_dbts() {

    assert (_allocated);

    TRACE( TRACE_TRX_FLOW, "Dellocating Dbts...\n");

    deallocate_payment_dbts(&_dbts);
    _allocated = 0;
}


void tpcc_payment_single_thr_driver::reset_dbts() {

    TRACE( TRACE_TRX_FLOW, "Resetting Dbts...\n");

    reset_payment_dbts(&_dbts);
}



/** @fn executePayment
 *
 *  @brief Execute a Payment transaction
 */

void* tpcc_payment_single_thr_driver::executePayment() {

    assert(_allocated);

    tpcc_payment::payment_input_t pin = create_payment_input(RANGE);
    DbTxn* txn = NULL;

    trx_result_tuple_t aTrxResultTuple = executePaymentBaseline(&pin,
                                                                _id, 
                                                                txn,
                                                                &_dbts);

    TRACE( TRACE_TRX_FLOW, "DONE. NOTIFYING CLIENT\n" );

    return (NULL);
}

                                                

EXIT_NAMESPACE(workload);
