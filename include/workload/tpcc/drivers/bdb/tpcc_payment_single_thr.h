/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file tpcc_payment_single_thread.h
 *
 *  @brief Declaration of a "driver" for executing PAYMENT_BASELINE transaction
 *  bypassing the staging mechanism.
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __BDB_TPCC_PAYMENT_SINGLE_THR_DRIVER_H
#define __BDB_TPCC_PAYMENT_SINGLE_THR_DRIVER_H

#include "stages/tpcc/bdb/payment_functions.h"
#include "workload/tpcc/drivers/tpcc_payment_common.h"


using namespace qpipe;
using namespace tpcc;
using namespace tpcc_payment;


ENTER_NAMESPACE(workload);


//------------------------------------------------------------------------
// @class tpcc_payment_single_thr_driver
//
// @brief This is an object that executes the payment_baseline transaction
// in a single threaded fashion.
//
// @note This is not a driver, that is why it can have own state
//

class tpcc_payment_single_thr_driver : public thread_t {

protected:

    // Structure for allocating only once all the Dbts
    s_payment_dbt_t _dbts;

    int _id;
    int _iterations;

public:

    tpcc_payment_single_thr_driver(const c_str& description, 
                                   const int id,
                                   const int iter)
        : thread_t(description), _id(id), _iterations(iter)
      {
          //        TRACE( TRACE_ALWAYS, " + %d constructing\n", _id);
      }


    virtual ~tpcc_payment_single_thr_driver() { }


    virtual void* run() {        

        assert(!_dbts.is_allocated());

        _dbts.allocate();
      
        assert(_dbts.is_allocated());

        printf(" = %d running =\n", _id);
        
        for (int i=0; i < _iterations; i++) {
          
            executePayment();
            //        TRACE( TRACE_ALWAYS, " - %d running(%d) -\n", _id, i);
        }
        
        _dbts.deallocate();
        return (NULL);
    }


    void* executePayment();
};


EXIT_NAMESPACE(workload);

#endif
