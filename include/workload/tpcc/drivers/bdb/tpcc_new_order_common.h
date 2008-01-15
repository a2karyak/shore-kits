/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file tpcc_new_order_common.h
 *
 *  @brief Declaration of function that creates a NEW_ORDER input
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __TPCC_NEW_ORDER_INPUT_COMMON_H
#define __TPCC_NEW_ORDER_INPUT_COMMON_H

#include "stages/tpcc/common/tpcc_scaling_factor.h"
#include "stages/tpcc/common/tpcc_input.h" 


ENTER_NAMESPACE(tpcc);


/** Exported functionality */

new_order_input_t create_new_order_input(int sf = QUERIED_TPCC_SCALING_FACTOR);

orderline_input_t create_orderline_input(int sf = QUERIED_TPCC_SCALING_FACTOR);

EXIT_NAMESPACE(tpcc);

#endif
