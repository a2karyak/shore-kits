/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file payment_functions.h
 *
 *  @brief Common functionality among BDB_PAYMENT_BASELINE and BDB_PAYMENT_STAGED
 *  transactions
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __BDB_TPCC_PAYMENT_FUNCTIONS_H
#define __BDB_TPCC_PAYMENT_FUNCTIONS_H


#include <db_cxx.h>

#include "memory/mem_obj.h"
#include "stages/tpcc/common/tpcc_struct.h"
#include "stages/tpcc/common/trx_packet.h"
#include "stages/tpcc/common/tpcc_input.h"

using namespace qpipe;
using namespace tpcc;



ENTER_NAMESPACE(tpcc_payment);



/** Exported data structures */


class s_payment_dbt_t : public memObject_t {

public: 

    /** Member variables */
    
    // UPD_WAREHOUSE
    Dbt whData;
    //Dbt whKey;
    

    // UPD_DISTRICT
    Dbt distData;
    //Dbt distKey;

    // UPD_CUSTOMER
    Dbt custData;
    //Dbt custKey;

    // INS_HISTORY
    // (ip) ?


    /** Construction */

    inline s_payment_dbt_t () { };

    virtual ~s_payment_dbt_t() {

        if (_allocated)
            deallocate();
    };


    /** Memory management functions */

    virtual void allocate();
    
    virtual void deallocate();

    virtual void reset();

}; // EOF s_payment_dbt_t



/** Exported functions */


int insertHistory(payment_input_t* pin, DbTxn* txn, s_payment_dbt_t* a_p_dbts);

int updateCustomer(payment_input_t* pin, DbTxn* txn, s_payment_dbt_t* a_p_dbts);

int updateDistrict(payment_input_t* pin, DbTxn* txn, s_payment_dbt_t* a_p_dbts);

int updateWarehouse(payment_input_t* pin, DbTxn* txn, s_payment_dbt_t* a_p_dbts);


int updateCustomerByID(DbTxn* txn, int wh_id, int d_id, int c_id,
                       decimal h_amount, s_payment_dbt_t* a_p_dbts);

int updateCustomerByLast(DbTxn* txn, int wh_id, int d_id, char* c_last, 
                         decimal h_amount, s_payment_dbt_t* a_p_dbts);


// implementation of the single-threaded version of the payment
trx_result_tuple_t executePaymentBaseline(payment_input_t* pin,
                                          const int id,
                                          DbTxn* txn,
                                          s_payment_dbt_t* a_dbts);


/*
void allocate_payment_dbts(s_payment_dbt_t* p_dbts);

void deallocate_payment_dbts(s_payment_dbt_t* p_dbts);

void reset_payment_dbts(s_payment_dbt_t* p_dbts);
*/


EXIT_NAMESPACE(tpcc_payment);


#endif
