/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file tpch_type_convert.h
 *
 *  @brief Definition of TPC-H type conversion functions
 */


#ifndef __TPCH_TYPE_CONVERT_H
#define __TPCH_TYPE_CONVERT_H

#include "workload/tpch/tpch_struct.h"

tpch_l_shipmode modestr_to_shipmode(char const* tmp);
tpch_o_orderpriority prioritystr_to_orderpriorty(char const* tmp);
tpch_n_name nnamestr_to_nname(char const* tmp);

#endif


