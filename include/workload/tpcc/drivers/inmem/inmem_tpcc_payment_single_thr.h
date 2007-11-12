/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file inmem_tpcc_payment_single_thread.h
 *
 *  @brief Declaration of a "driver" for executing PAYMENT_BASELINE transaction
 *  bypassing the staging mechanism.
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __INMEM_TPCC_PAYMENT_SINGLE_THR_DRIVER_H
#define __INMEM_TPCC_PAYMENT_SINGLE_THR_DRIVER_H

#include "util.h"
#include "stages/tpcc/inmem/inmem_payment_functions.h"
#include "workload/tpcc/drivers/tpcc_payment_common.h"

#include <semaphore.h>

using namespace qpipe;
using namespace tpcc;
using namespace tpcc_payment;


ENTER_NAMESPACE(workload);


// Different operation modes
//#define WITH_HELPER_THREAD
#define WITH_POOLED_THREAD



//------------------------------------------------------------------------
// @class inmem_tpcc_payment_single_thr_helper
//
// @brief This is the helper thread that executes the inmem_payment_baseline 
// transaction 
//
// @note This is not a driver, that is why it can have own state
//

class inmem_tpcc_payment_single_thr_helper : public thread_t {

protected:

    payment_input_t _pin;
    int _id;
    InMemTPCCEnv* _env;
    trx_result_tuple_t _result;
    int _done;

public:

#ifdef WITH_POOLED_THREAD
    sem_t* _psem;
    sem_t* _ppoolsem;
#endif

    inmem_tpcc_payment_single_thr_helper(const c_str& description, 
                                         payment_input_t a_pin,
                                         const int a_id,      
#ifdef WITH_POOLED_THREAD
                                         sem_t* driver_sem,
                                         sem_t* pool_sem,
#endif
                                         InMemTPCCEnv* env)
        : thread_t(description), _pin(a_pin), _id(a_id), _env(env)
#ifdef WITH_POOLED_THREAD
          ,_done(0)
          ,_psem(driver_sem)
          ,_ppoolsem(pool_sem)
#endif   
    {
    }

    virtual ~inmem_tpcc_payment_single_thr_helper() { }


    virtual void* run() {        
        assert (_env);
        assert (_id>=0);

#ifdef WITH_POOLED_THREAD
        do {
            sem_wait(_psem);
#endif
            _result = executeInMemPaymentBaseline(_pin, _id, _env);

#ifdef WITH_POOLED_THREAD
            sem_post(_ppoolsem);
        } while (!_done);
        _result = executeInMemPaymentBaseline(_pin, _id, _env);
#endif

        return (NULL);
    }


    // Access methods

    void setParams(payment_input_t a_pin) {
        _pin = a_pin;
    }

    void setDone() {
        _done = 1;
    }

    trx_result_tuple_t getResult() { return (_result); }

}; // EOF inmem_tpcc_payment_single_thr_helper



//------------------------------------------------------------------------
// @class inmem_tpcc_payment_single_thr_driver
//
// @brief This is an object that executes the inmem_payment_baseline 
// transaction in a single threaded fashion.
//
// @note This is not a driver, that is why it can have own state
//

class inmem_tpcc_payment_single_thr_driver : public thread_t {

protected:

    int _id;
    int _iterations;
    int _sf;
    InMemTPCCEnv* _env;

#ifdef WITH_POOLED_THREAD
    inmem_tpcc_payment_single_thr_helper* _helper;
    sem_t _sem;
    sem_t _poolsem;
#endif

public:

    inmem_tpcc_payment_single_thr_driver(const c_str& description, 
                                         const int id,
                                         const int iter,
                                         const int sf,
                                         InMemTPCCEnv* env)
        : thread_t(description), _id(id), _iterations(iter), _sf(sf), _env(env)
    {
#ifdef WITH_POOLED_THREAD
        sem_init(&_sem, 0, 0);
        sem_init(&_poolsem, 0, 0);
        _helper = new inmem_tpcc_payment_single_thr_helper(c_str("INMEM-PAY-SINGLE-POOL"),
                                                           create_payment_input(_sf),
                                                           _id,
                                                           &_sem,
                                                           &_poolsem,
                                                           _env);
#endif
    }

    virtual ~inmem_tpcc_payment_single_thr_driver() { 
#ifdef WITH_POOLED_THREAD
        sem_destroy(&_sem);
        sem_destroy(&_poolsem);
#endif
    }

    // Three variations of the single-threaded trx execution
    trx_result_tuple_t* singleThrPayment(payment_input_t pin);

#ifdef WITH_HELPER_THREAD
    trx_result_tuple_t* singleThrPaymentHelper(payment_input_t pin);
#endif

#ifdef WITH_POOLED_THREAD
    trx_result_tuple_t* singleThrPaymentPool();
#endif    
    


    // Thread-entry function
    virtual void* run() {        

#ifdef WITH_HELPER_THREAD
#ifdef WITH_POOLED_THREAD
#warning "Both WITH_HELPER_THREAD and WITH_POOLED_THREAD defined..."
#warning "It will abort at run-time..."
        assert (1==0); // Both cannot be defined.
#endif
#endif

        trx_result_tuple_t* aTrxResult;


#ifdef WITH_POOLED_THREAD
        aTrxResult = singleThrPaymentPool();
#else
        for (int i=0; i < _iterations; i++) {            

#ifdef WITH_HELPER_THREAD          
            aTrxResult = singleThrPaymentHelper(create_payment_input(_sf));
#else
            aTrxResult = singleThrPayment(create_payment_input(_sf));
#endif // WITH_HELPER_THREAD

            if (!(aTrxResult->get_state() == COMMITTED)) {
                TRACE( TRACE_ALWAYS, "Problem with IT=(%d)\n", i);
            }            
        }
#endif // WITH_POOLED_THREAD

        delete aTrxResult;        
        return (NULL);
    }


}; // EOF inmem_tpcc_payment_single_thr_driver


EXIT_NAMESPACE(workload);

#endif
