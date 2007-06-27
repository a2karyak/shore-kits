/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file payment_functions.h
 *
 *  @brief Common functionality among PAYMENT_BASELINE and PAYMENT_STAGED
 *  transactions
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __TPCC_PAYMENT_FUNCTIONS_H
#define __TPCC_PAYMENT_FUNCTIONS_H


#include <db_cxx.h>

#include "stages/tpcc/common/tpcc_struct.h"
#include "stages/tpcc/common/trx_packet.h"

using namespace qpipe;
using namespace tpcc;



ENTER_NAMESPACE(tpcc_payment);


/** Exported data structures */

struct payment_input_t {

    /**
     *  @brief PAYMENT transaction inputs:
     *  
     *  1) HOME_WH_ID int [1 .. SF] : home warehouse id
     *  2) HOME_D_ID int [1 .. 10]  : home district id
     *  3) V_CUST_WH_SELECTION int [1 .. 100] : customer warehouse selection ( 85% - 15%)
     *  4) REMOTE_WH_ID int [1 .. SF] : remote warehouse id (optional)
     *  5) REMOTE_D_ID int [1 .. 10] : remote district id (optional)
     *  6) V_CUST_IDENT_SELECTION int [1 .. 100] : customer identification selection ( 60% - 40%)
     *  7) C_ID int : customer id (C_ID = NURand(1023, 1, 3000) (optional)
     *  8) C_LAST char* : customer lastname (using NURand(255, 0, 999) (optional)
     *  9) H_AMOUNT long [1.00 .. 5,000.00] : the payment amount
     *  10) H_DATE char* : the payment time
     */

    int _home_wh_id;
    int _home_d_id;
    int _v_cust_wh_selection;
    int _remote_wh_id;
    int _remote_d_id;
    int _v_cust_ident_selection;
    int _c_id;
    char _c_last[16];
    double _h_amount;
    int _h_date;

}; // EOF payment_input_t



/** @brief The purpose of this structure is to allocate only once per driver 
 *  the needed space for the data to be retrieved or used as keys. Otherwise
 *  for each iteration, BDB will allocate (using malloc()) new space.
 */

struct s_payment_dbt_t {

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
    // FIXME (ip) ?

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


/** Helper Functions */

inline void describe(payment_input_t pin, int id) {

#ifndef USE_SAME_INPUT

    TRACE( TRACE_TRX_FLOW,
           "\nPAYMENT - TRX=%d\n" \
           "WH_ID=%d\t\tD_ID=%d\n" \
           "SEL_WH=%d\tSEL_IDENT=%d\n" \
           "REM_WH_ID=%d\tREM_D_ID=%d\n" \
           "C_ID=%d\tC_LAST=%s\n" \
           "H_AMOUNT=%.2f\tH_DATE=%d\n", \
           id,
           pin._home_wh_id, 
           pin._home_d_id, 
           pin._v_cust_wh_selection, 
           pin._v_cust_ident_selection,
           pin._remote_wh_id, 
           pin._remote_d_id,
           pin._c_id, 
           pin._c_last,
           pin._h_amount, 
           pin._h_date);

#else
    // If using the standard input, _c_last is NULL
    TRACE( TRACE_TRX_FLOW,
           "\nPAYMENT - TRX=%d\n" \
           "WH_ID=%d\t\tD_ID=%d\n" \
           "SEL_WH=%d\tSEL_IDENT=%d\n" \
           "REM_WH_ID=%d\tREM_D_ID=%d\n" \
           "C_ID=%d\tH_AMOUNT=%.2f\tH_DATE=%d\n", \
           id,
           pin._home_wh_id, 
           pin._home_d_id, 
           pin._v_cust_wh_selection, 
           pin._v_cust_ident_selection,
           pin._remote_wh_id, 
           pin._remote_d_id,
           pin._c_id, 
           pin._h_amount, 
           pin._h_date);
#endif
}


EXIT_NAMESPACE(tpcc_payment);


#endif
