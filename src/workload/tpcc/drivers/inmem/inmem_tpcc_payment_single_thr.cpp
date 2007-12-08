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



using namespace qpipe;
using namespace workload;
using namespace tpcc;
using namespace tpcc_payment;



/** @fn singleThrPayment
 *
 *  @brief Execute a Payment transaction in the same thread context
 */

trx_result_tuple_t*
inmem_tpcc_payment_single_thr_driver::singleThrPayment(payment_input_t pin) 
{
    return(new trx_result_tuple_t(executeInMemPaymentBaseline(pin, _id, _env)));
}



#ifdef WITH_HELPER_THREAD

/** @fn singleThrPaymentHelper
 *
 *  @brief Execute a Payment transaction spawning a new thread for that
 */

trx_result_tuple_t*
inmem_tpcc_payment_single_thr_driver::singleThrPaymentHelper(payment_input_t pin) 
{
    // Runs the TRX in a HELPER thread
    array_guard_t<pthread_t> helper_id = new pthread_t[1];
    inmem_tpcc_payment_single_thr_helper* helper;
    static c_str name = "INMEM-PAY-SINGLE-HELPER";
    // Creates a new thread (helper) which executes the transaction
    // We do this trick in order to be fair with the staged version and not execute
    // the whole transaction in the context of the client
    try {

        helper =
            new inmem_tpcc_payment_single_thr_helper(name,
                                                     pin,
                                                     _id,
                                                     _env);

        helper_id[0] = thread_create(helper);        
    }
    catch (QPipeException &e) {
        
        TRACE( TRACE_ALWAYS, "Exception reached root level. Description:\n" \
               "%s\n", e.what());
        
        workload::workload_t::wait_for_clients(helper_id, 1);
        return (new trx_result_tuple_t(POISSONED,0));
    }    

    workload::workload_t::wait_for_clients(helper_id, 1);
    return (new trx_result_tuple_t(COMMITTED,0));

} // EOF singleThrPaymentHelper


#endif // WITH_HELPER_THREAD


#ifdef WITH_POOLED_THREAD

/** @fn singleThrPaymentPool
 *
 *  @brief Execute a Payment transaction using the same helper thread
 *
 *  @note Uses a condition for the synchronization between the two threads 
 */

trx_result_tuple_t*
inmem_tpcc_payment_single_thr_driver::singleThrPaymentPool() 
{
    // Runs the TRX in a HELPER thread
    array_guard_t<pthread_t> helper_id = new pthread_t[1];
    payment_input_t pin;

    try {
        // Spawn thread
        helper_id[0] = thread_create(_helper);

        for (int i=0; i<_iterations; i++) {

            // if first iteration should not wait
            if (i != 0)
                sem_wait(&_poolsem);

            // Set params for the new run
            pin = create_payment_input(_sf);
            _helper->setParams(pin);

            // if last iteration singal end
            if (i == _iterations-1)
                _helper->setDone();

            // Signal
            sem_post(&_sem);
        }
    }
    catch (QPipeException &e) {
        
        TRACE( TRACE_ALWAYS, "Exception reached root level. Description:\n" \
               "%s\n", e.what());
        
        workload::workload_t::wait_for_clients(helper_id, 1);
        return (new trx_result_tuple_t(POISSONED,0));
    }    

    workload::workload_t::wait_for_clients(helper_id, 1);
    return (new trx_result_tuple_t(COMMITTED,0));   
}

#endif // WITH_POOLED_THREAD
