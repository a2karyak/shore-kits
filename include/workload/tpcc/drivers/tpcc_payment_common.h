/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file tpcc_payment_common.h
 *
 *  @brief Declaration of function that creates a PAYMENT input
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __TPCC_PAYMENT_INPUT_COMMON_H
#define __TPCC_PAYMENT_INPUT_COMMON_H


#include "stages/tpcc/common/tpcc_input.h" 


ENTER_NAMESPACE(tpcc);

/** Exported functionality */

payment_input_t create_payment_input(int sf);

EXIT_NAMESPACE(tpcc);

#endif
