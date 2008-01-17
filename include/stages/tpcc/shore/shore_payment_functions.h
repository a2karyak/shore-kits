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

#include "workload/tpcc/shore_tpcc_env.h"


using namespace qpipe;
using namespace tpcc;



ENTER_NAMESPACE(tpcc_payment);


/** Exported functions */

int insertShoreHistory(payment_input_t* pin, 
                       ss_m* pssm, ShoreTPCCEnv* env);

int updateShoreCustomer(payment_input_t* pin, 
                        ss_m* pssm, ShoreTPCCEnv* env);

int updateShoreDistrict(payment_input_t* pin, 
                        ss_m* pssm, ShoreTPCCEnv* env);

int updateShoreWarehouse(payment_input_t* pin,
                         ss_m* pssm, ShoreTPCCEnv* env);

int updateShoreCustomerByID(int wh_id, int d_id, int c_id, decimal h_amount,
                            ss_m* pssm, ShoreTPCCEnv* env);

int updateShoreCustomerByLast(int wh_id, int d_id, 
			      char* c_last, decimal h_amount,
                              ss_m* pssm, ShoreTPCCEnv* env);

// implementation of the single-threaded version of the Shore payment
trx_result_tuple_t executeShorePaymentBaseline(payment_input_t pin,
                                               const int id,
                                               ShoreTPCCEnv* env);



/// STAGED Functions 

int staged_updateShoreWarehouse(payment_input_t* pin, ShoreTPCCEnv* env);
int staged_updateShoreDistrict(payment_input_t* pin, ShoreTPCCEnv* env);
int staged_updateShoreCustomer(payment_input_t* pin, ShoreTPCCEnv* env);
int staged_insertShoreHistory(payment_input_t* pin, ShoreTPCCEnv* env);



/** Exported variable */

extern ShoreTPCCEnv* shore_env;



EXIT_NAMESPACE(tpcc_payment);

#endif
