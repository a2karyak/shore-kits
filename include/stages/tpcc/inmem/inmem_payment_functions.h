/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file inmem_payment_functions.h
 *
 *  @brief Common functionality among PAYMENT_BASELINE and PAYMENT_STAGED
 *  transactions
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __INMEM_TPCC_PAYMENT_FUNCTIONS_H
#define __INMEM_TPCC_PAYMENT_FUNCTIONS_H


#include "stages/tpcc/common/tpcc_struct.h"
#include "stages/tpcc/common/trx_packet.h"
#include "stages/tpcc/common/tpcc_input.h"

#include "workload/tpcc/inmem_tpcc_env.h"


using namespace qpipe;
using namespace tpcc;



ENTER_NAMESPACE(tpcc_payment);


/** Exported functions */

int insertInMemHistory(payment_input_t* pin, 
                       InMemTPCCEnv* env);

int updateInMemCustomer(payment_input_t* pin, 
                        InMemTPCCEnv* env);

int updateInMemDistrict(payment_input_t* pin, int* idx, 
                        InMemTPCCEnv* env);

int updateInMemWarehouse(payment_input_t* pin, int* idx, 
                         InMemTPCCEnv* env);

int updateInMemCustomerByID(int wh_id, int d_id, int c_id, 
                            decimal h_amount, 
			    InMemTPCCEnv* env);

int updateInMemCustomerByLast(int wh_id, int d_id, 
			      char* c_last,
                              decimal h_amount, 
			      InMemTPCCEnv* env);


// implementation of the single-threaded version of the inmemory payment
trx_result_tuple_t executeInMemPaymentBaseline(payment_input_t* pin,
                                               const int id,
                                               InMemTPCCEnv* env);


/** Exported variable */

extern InMemTPCCEnv* inmem_env;



EXIT_NAMESPACE(tpcc_payment);

#endif
