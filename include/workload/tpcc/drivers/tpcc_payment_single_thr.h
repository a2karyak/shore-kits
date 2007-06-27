/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file tpcc_payment_single_thread.h
 *
 *  @brief Declaration of a "driver" for executing PAYMENT_BASELINE transaction
 *  bypassing the staging mechanism.
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __TPCC_PAYMENT_SINGLE_THR_DRIVER_H
#define __TPCC_PAYMENT_SINGLE_THR_DRIVER_H


#include "workload/tpcc/drivers/tpcc_payment_common.h"
#include "stages/tpcc/common/payment_functions.h"


using namespace qpipe;
using namespace tpcc;


ENTER_NAMESPACE(workload);


class tpcc_payment_single_thr_driver : public thread_t {
//class tpcc_payment_single_thr_driver {

protected:

    // Structure for allocating only once all the Dbts
    s_payment_dbt_t _dbts;

    virtual void allocate_dbts();
    virtual void deallocate_dbts();
    virtual void reset_dbts();

    int _allocated;
    int _id;
    int _iterations;

public:

    tpcc_payment_single_thr_driver(const c_str& description, 
                                   const int id,
                                   const int iter)
      //      : thread_t(description), _allocated(0), _id(id), _iterations(iter)
        : thread_t(description), _allocated(0), _id(id), _iterations(iter)
      {
        printf( " + %d constructing\n", _id);
        //        TRACE( TRACE_ALWAYS, " + %d constructing\n", _id);
      }


    virtual ~tpcc_payment_single_thr_driver() { }


    virtual void* run() {        

      assert(!_allocated);

      allocate_dbts();

      assert(_allocated);

      for (int i=0; i < _iterations; i++) {

        //executePayment();
        printf(" = %d running (%d) =\n", _id, i);
        //TRACE( TRACE_ALWAYS, " - %d running(%d) -\n", _id, i);
      }

      deallocate_dbts();
      return (NULL);
    }


    void* executePayment();
};


EXIT_NAMESPACE(workload);

#endif
