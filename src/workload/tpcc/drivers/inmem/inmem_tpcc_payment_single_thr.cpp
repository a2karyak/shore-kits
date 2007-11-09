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

#include "workload/workload.h"
#include "workload/tpcc/drivers/inmem/inmem_tpcc_payment_single_thr.h"
#include "workload/tpcc/drivers/tpcc_payment_common.h"


using namespace qpipe;
using namespace workload;
using namespace tpcc;
using namespace tpcc_payment;


// (ip) To be removed in order to test the additional helper thread
//#define NO_HELPER_THREAD


/** @fn executeInMemPayment
 *
 *  @brief Execute a Payment transaction
 */

void* inmem_tpcc_payment_single_thr_driver::executeInMemPayment(InMemTPCCEnv* env, int SF) 
{
    payment_input_t pin = create_payment_input(SF);


#ifdef NO_HELPER_THREAD

    // Executes the transaction in the same thread
    trx_result_tuple_t aTrxResultTuple = executeInMemPaymentBaseline(pin,
                                                                     _id, 
                                                                     env);

    TRACE( TRACE_TRX_FLOW, "DONE. NOTIFYING CLIENT\n" );

#else

    array_guard_t<pthread_t> helper_id = new pthread_t[1];

    // Creates a new thread (helper) which executes the transaction
    // We do this trick in order to be fair with the staged version and not execute
    // the whole transaction in the context of the client

    try {

        inmem_tpcc_payment_single_thr_helper* helper =
            new inmem_tpcc_payment_single_thr_helper(c_str("INMEM-PAY-SINGLE-HELPER"),
                                                     pin,
                                                     _id,
                                                     env);

        helper_id[0] = thread_create(helper);        
    }
    catch (QPipeException &e) {
        
        TRACE( TRACE_ALWAYS, "Exception reached root level. Description:\n" \
               "%s\n", e.what());
        
        workload::workload_t::wait_for_clients(helper_id, 1);

        return (NULL);
    }    

    workload::workload_t::wait_for_clients(helper_id, 1);


#endif

    return (NULL);
}
