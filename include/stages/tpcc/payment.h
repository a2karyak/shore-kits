/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef __TPCC_PAYMENT_H
#define __TPCC_PAYMENT_H

// STAGED PAYMENT
#include "stages/tpcc/payment_upd_wh.h"
#include "stages/tpcc/payment_upd_distr.h"
#include "stages/tpcc/payment_upd_cust.h"
#include "stages/tpcc/payment_ins_hist.h"
#include "stages/tpcc/payment_finalize.h"

/** @note payment_begin should be declared last */
#include "stages/tpcc/payment_begin.h"


// BASELINE PAYMENT
#include "stages/tpcc/payment_baseline.h"


#endif 
