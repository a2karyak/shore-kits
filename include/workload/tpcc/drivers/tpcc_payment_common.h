/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file tpcc_payment_common.h
 *
 *  @brief Declaration of common functionality for the clients that submit 
 *  PAYMENT_BASELINE or PAYMENT_STAGED transactions
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __TPCC_PAYMENT_DRIVER_COMMON_H
#define __TPCC_PAYMENT_DRIVER_COMMON_H


#include "stages/tpcc/common/payment_functions.h" 


using namespace tpcc_payment;

ENTER_NAMESPACE(tpcc);


/** Exported data structures */


/** Exported functionality */

payment_input_t create_payment_input(int sf);

void allocate_payment_dbts(s_payment_dbt_t* p_dbts);

void deallocate_payment_dbts(s_payment_dbt_t* p_dbts);


EXIT_NAMESPACE(tpcc);

#endif
