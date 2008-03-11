/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file payment.h
 *
 *  @brief Includes all the payment-related files
 */


#ifndef __TPCC_PAYMENT_H
#define __TPCC_PAYMENT_H

// STAGED PAYMENT

///////////////
// BDB-Based //
///////////////

// BDB-STAGED-PAYMENT
#include "stages/tpcc/bdb/payment_upd_wh.h"
#include "stages/tpcc/bdb/payment_upd_distr.h"
#include "stages/tpcc/bdb/payment_upd_cust.h"
#include "stages/tpcc/bdb/payment_ins_hist.h"
#include "stages/tpcc/bdb/payment_finalize.h"

/** @note payment_begin should be declared last */
#include "stages/tpcc/bdb/payment_begin.h"

// BDB-BASELINE-PAYMENT
#include "stages/tpcc/bdb/payment_baseline.h"


///////////////
// IN-MEMORY //
///////////////

// INMEM-STAGED-PAYMENT
#include "stages/tpcc/inmem/inmem_payment_upd_wh.h"
#include "stages/tpcc/inmem/inmem_payment_upd_distr.h"
#include "stages/tpcc/inmem/inmem_payment_upd_cust.h"
#include "stages/tpcc/inmem/inmem_payment_ins_hist.h"
#include "stages/tpcc/inmem/inmem_payment_finalize.h"

/** @note payment_begin should be declared last */
#include "stages/tpcc/inmem/inmem_payment_begin.h"


// INMEM-BASELINE-PAYMENT
#include "stages/tpcc/inmem/inmem_payment_baseline.h"


///////////
// SHORE //
///////////


// SHORE-BASELINE-PAYMENT
#include "stages/tpcc/shore/shore_payment_baseline.h"


// SHORE-STAGED-PAYMENT
#include "stages/tpcc/shore/staged/shore_payment_upd_wh.h"
#include "stages/tpcc/shore/staged/shore_payment_upd_distr.h"
#include "stages/tpcc/shore/staged/shore_payment_upd_cust.h"
#include "stages/tpcc/shore/staged/shore_payment_ins_hist.h"
#include "stages/tpcc/shore/staged/shore_payment_finalize.h"

/** @note payment_begin should be declared last */
#include "stages/tpcc/shore/staged/shore_payment_begin.h"



#endif /* __TPCC_PAYMENT_H */
