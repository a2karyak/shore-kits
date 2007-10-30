/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file payment.h
 *
 *  @brief Includes all the payment-related files
 *  10/29/07: Including InMemory Payment stages
 */


#ifndef __TPCC_PAYMENT_H
#define __TPCC_PAYMENT_H

// STAGED PAYMENT

///////////////
// BDB-Based //
///////////////

#include "stages/tpcc/payment_upd_wh.h"
#include "stages/tpcc/payment_upd_distr.h"
#include "stages/tpcc/payment_upd_cust.h"
#include "stages/tpcc/payment_ins_hist.h"
#include "stages/tpcc/payment_finalize.h"

/** @note payment_begin should be declared last */
#include "stages/tpcc/payment_begin.h"

// BDB-BASELINE
#include "stages/tpcc/payment_baseline.h"


///////////////
// IN-MEMORY //
///////////////

// INMEM-BASELINE
#include "stages/tpcc/inmem/inmem_payment_baseline.h"




#endif 
