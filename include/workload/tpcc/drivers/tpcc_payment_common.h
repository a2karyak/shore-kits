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

#include "stages/tpcc/common/payment_functions.h" /* for payment_input_t */

using namespace tpcc_payment;

ENTER_NAMESPACE(tpcc);

payment_input_t create_payment_input(int sf);

EXIT_NAMESPACE(tpcc);

#endif
