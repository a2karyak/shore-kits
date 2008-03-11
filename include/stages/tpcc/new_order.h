/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file new_order.h
 *
 *  @brief Includes all the new_order-related files
 */


#ifndef __TPCC_NEW_ORDER_H
#define __TPCC_NEW_ORDER_H

// STAGED NEW_ORDER

///////////
// SHORE //
///////////


// SHORE-BASELINE-NEW_ORDER
#include "stages/tpcc/shore/shore_new_order_baseline.h"



// SHORE-STAGED-PAYMENT
#include "stages/tpcc/shore/staged/shore_new_order_outside_loop.h"
#include "stages/tpcc/shore/staged/shore_new_order_one_ol.h"
#include "stages/tpcc/shore/staged/shore_new_order_finalize.h"

/** @note payment_begin should be declared last */
#include "stages/tpcc/shore/staged/shore_new_order_begin.h"



#endif 
