/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file shore_payment_functions.h
 *
 *  @brief Common functionality among SHORE_PAYMENT_BASELINE 
 *  and SHORE_PAYMENT_STAGED transactions
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __SHORE_TPCC_PAYMENT_FUNCTIONS_H
#define __SHORE_TPCC_PAYMENT_FUNCTIONS_H


#include "stages/tpcc/common/tpcc_struct.h"
#include "stages/tpcc/common/trx_packet.h"
#include "stages/tpcc/common/tpcc_input.h"

#include "stages/tpcc/shore/shore_tpcc_env.h"
#include "stages/tpcc/shore/shore_tools.h"


using namespace qpipe;
using namespace tpcc;
using namespace shore;


ENTER_NAMESPACE(tpcc_payment);


/** Exported functions */

typedef trx_smthread_t<payment_input_t> payment_thr_t;

trx_result_tuple_t* sm_exec_payment_baseline(payment_input_t* ppin, 
                                             const int id, 
                                             ShoreTPCCEnv* env);


trx_result_tuple_t* sm_exec_payment_staged(payment_input_t* ppin, 
                                           const int id, 
                                           ShoreTPCCEnv* env);



EXIT_NAMESPACE(tpcc_payment);

#endif
