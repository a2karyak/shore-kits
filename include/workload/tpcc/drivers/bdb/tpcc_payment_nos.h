/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file tpcc_payment_baseline.h
 *
 *  @brief Declaration of the driver that executes PAYMENT_BASELINE transactions
 *  without employing the staging mechanism.
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __TPCC_PAYMENT_NOS_DRIVER_H
#define __TPCC_PAYMENT_NOS_DRIVER_H


#include "workload/driver.h"

#include "stages/tpcc/common/tpcc_trx_input.h"


using namespace qpipe;
using namespace tpcc;


ENTER_NAMESPACE(workload);


//----------------------------------------------------------------------------
// @class tpcc_payment_nos_driver 
//
// @brief This is the driver that creates and fires a fixed for now number of
// tpcc_payment_single_thr threds.
// 
// @note The workload_client will report incorrect statistics for this driver.
//
// @todo Enable the user from the command line to determine the number of
// threads and iterations.
//

class tpcc_payment_nos_driver : public driver_t {

public:

    tpcc_payment_nos_driver(const c_str& description)
        : driver_t(description) 
    {  
    }

    virtual ~tpcc_payment_nos_driver() { }
    
    virtual void submit(void* disp, memObject_t* mem);
};


EXIT_NAMESPACE(workload);

#endif
