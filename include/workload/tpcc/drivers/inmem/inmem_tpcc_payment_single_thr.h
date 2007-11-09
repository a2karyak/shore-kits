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

#include "stages/tpcc/inmem/inmem_payment_functions.h"


using namespace qpipe;
using namespace tpcc;
using namespace tpcc_payment;


ENTER_NAMESPACE(workload);


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

public:

    inmem_tpcc_payment_single_thr_helper(const c_str& description, 
                                         payment_input_t a_pin,
                                         const int a_id,
                                         InMemTPCCEnv* env)
        : thread_t(description), _pin(a_pin), _id(a_id), _env(env)
    {
        //TRACE( TRACE_ALWAYS, " + %d constructing\n", _id);
    }


    virtual ~inmem_tpcc_payment_single_thr_helper() { }


    virtual void* run() {        

        assert (_env);
        assert (_id>=0);

        trx_result_tuple_t aTrxResultTuple = executeInMemPaymentBaseline(_pin,
                                                                         _id, 
                                                                         _env);

        TRACE( TRACE_TRX_FLOW, "DONE. NOTIFYING CLIENT\n" );
        
        return (NULL);
    }

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

public:

    inmem_tpcc_payment_single_thr_driver(const c_str& description, 
                                         const int id,
                                         const int iter,
                                         const int sf,
                                         InMemTPCCEnv* env)
        : thread_t(description), _id(id), _iterations(iter), _sf(sf), _env(env)
    {
        //TRACE( TRACE_ALWAYS, " + %d constructing\n", _id);
    }


    virtual ~inmem_tpcc_payment_single_thr_driver() { }


    virtual void* run() {        
        
        for (int i=0; i < _iterations; i++) {
          
            executeInMemPayment(_env, _sf);
        }
        
        return (NULL);
    }


    void* executeInMemPayment(InMemTPCCEnv* env, int sf);

}; // EOF inmem_tpcc_payment_single_thr_driver


EXIT_NAMESPACE(workload);

#endif
