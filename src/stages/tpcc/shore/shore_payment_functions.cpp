/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file shore_payment_functions.cpp
 *
 *  @brief Implementation of common functionality among 
 *  SHORE_PAYMENT_BASELINE and SHORE_PAYMENT_STAGED transactions
 *
 *  @author Ippokratis Pandis (ipandis)
 */


#include "stages/tpcc/shore/shore_payment_functions.h"


using namespace qpipe;
using namespace tpcc;


ENTER_NAMESPACE(tpcc_payment);



/** Forward declaration of helper functions */


// implementation of the single-threaded version of the Shore payment
trx_result_tuple_t executeShorePaymentBaseline(payment_input_t pin,
                                               const int id,
                                               ShoreTPCCEnv* env);

// implementation of the staged version of the Shore payment
trx_result_tuple_t executeShorePaymentStaged(payment_input_t pin,
                                             const int id,
                                             ShoreTPCCEnv* env);


/// Regular functions

int insertShoreHistory(payment_input_t* pin, ShoreTPCCEnv* env);
int updateShoreCustomer(payment_input_t* pin, ShoreTPCCEnv* env);
int updateShoreDistrict(payment_input_t* pin, ShoreTPCCEnv* env);
int updateShoreWarehouse(payment_input_t* pin, ShoreTPCCEnv* env);
int updateShoreCustomerByID(int wh_id, int d_id, int c_id, 
                            decimal h_amount, ShoreTPCCEnv* env);
int updateShoreCustomerByLast(int wh_id, int d_id, char* c_last, 
                              decimal h_amount, ShoreTPCCEnv* env);

/// STAGED functions 

int staged_updateShoreWarehouse(payment_input_t* pin, ShoreTPCCEnv* env);
int staged_updateShoreDistrict(payment_input_t* pin, ShoreTPCCEnv* env);
int staged_updateShoreCustomer(payment_input_t* pin, ShoreTPCCEnv* env);
int staged_insertShoreHistory(payment_input_t* pin, ShoreTPCCEnv* env);

/// Helper functions

void updateShoreCustomerData(tpcc_customer_tuple_key ck, 
                             tpcc_customer_tuple_body* a_cb);


/** Exported Functions */



/** @fn sm_exec_payment_baseline
 *  
 *  @brief Wrapper that creates an smthread first and executes the baseline payment
 */

trx_result_tuple_t sm_exec_payment_baseline(payment_input_t pin, const int id, 
                                            ShoreTPCCEnv* env)
{
    assert (false);
    return (executeShorePaymentBaseline(pin,id,env));
}


/** @fn sm_exec_payment_staged
 *  
 *  @brief Wrapper that creates an smthread first and executes the staged payment
 */

trx_result_tuple_t sm_exec_payment_staged(payment_input_t pin, const int id, 
                                          ShoreTPCCEnv* env)
{
    assert (false);
    return (executeShorePaymentStaged(pin,id,env));
}



/** Private Functions */



/** @fn insertShoreHistory
 *  
 *  @brief Step 5: Insert a new HISTORY row. A new row is inserted into the HISTORY
 *  table with H_C_ID = C_ID, H_C_D_ID = C_D_ID, H_C_W_ID = C_W_ID, H_D_ID = D_ID,
 *  H_W_ID = W_ID and H_DATA = concatenating W_NAME and D_NAME separated by
 *  4 spaces.
 *
 *  @brief Selects which updateShoreCustomerX() function to call
 *
 *  @return 0 on success, non-zero on failure
 */

int insertShoreHistory(payment_input_t* pin, ShoreTPCCEnv* env)
{
    /////////////////////////////
    ///// START INS_HISTORY /////

    assert (env); // ensure that we have a valid environment
    
    TRACE( TRACE_TRX_FLOW, "Step 5: Inserting a new row in HISTORY table\n");
    
    // Preparing HISTORY tuple
    tpcc_history_tuple_body htb;
    htb.H_AMOUNT = pin->_h_amount;    
    
    // FIXME (ip) Modification of the specification. Instead of concatenating
    // W_NAME and D_NAME we do that with W_ID and D_ID
    snprintf(&htb.H_DATA[0], 25, "%d    %d", 
             pin->_home_wh_id, 
             pin->_home_d_id);
    htb.H_DATA[25] = '\0';


    // HISTORY has a key comprised by the first 6 integers
    tpcc_history_tuple_key hk;
    hk.H_C_ID = pin->_c_id;
    hk.H_C_D_ID = pin->_home_d_id;
    hk.H_C_W_ID = pin->_home_wh_id;
    hk.H_D_ID = pin->_home_d_id;
    hk.H_W_ID = pin->_home_wh_id;       
    hk.H_DATE = pin->_h_date;

    // Assume that insertion always succeeds
    assert (false); // (ip) TODO
    //    env->im_histories.insert(hk, htb);
    
    return (0);

    ///// EOF INS_HISTORY /////
    ///////////////////////////
}



/** @fn updateShoreCustomer
 *  
 *  @brief Selects which updateCustomerX() function to call
 *
 *  @return 0 on success, non-zero on failure
 */

int updateShoreCustomer(payment_input_t* pin, ShoreTPCCEnv* env)
{
    //////////////////////////////
    ///// START UPD_CUSTOMER /////

    assert (env); // ensure that we have a valid environment

    // FIXME (ip) : It always chooses the customer by id. It should be 40.
    if (pin->_v_cust_ident_selection < 0) {
        //       if (pin->_v_cust_ident_selection < 40) {
        
        /** Step 4b: Retrieve CUSTOMER based on C_LAST */
        TRACE( TRACE_TRX_FLOW, 
               "Step 4b: Retrieving and Updating the row in the CUSTOMER table " \
               "with matching C_W_ID=(%d) and C_D_ID=(%d) and C_LAST=(%s)\n",
               pin->_home_wh_id,
               pin->_home_d_id, 
               pin->_c_last);
        
        return (updateShoreCustomerByLast(pin->_home_wh_id, 
                                          pin->_home_d_id, 
                                          pin->_c_last, 
                                          pin->_h_amount,
                                          env));
    }
    else {
        /** Step 4a: Retrieve CUSTOMER based on C_ID */
        TRACE( TRACE_TRX_FLOW, 
               "Step 4a: Retrieving and Updating the row in the CUSTOMER table " \
               "with matching C_W_ID=(%d) and C_D_ID=(%d) and C_ID=(%d)\n",
               pin->_home_wh_id,
               pin->_home_d_id, 
               pin->_c_id);
        
        return (updateShoreCustomerByID(pin->_home_wh_id, 
                                        pin->_home_d_id, 
                                        pin->_c_id, 
                                        pin->_h_amount,
                                        env));
    }

    // It should never reach this point, return error
    return (1);
    
    ///// EOF UPD_CUSTOMER /////
    ////////////////////////////
}



/** @fn updateShoreWarehouse
 *
 *  @brief Step 2: Retrieve the row in the WAREHOUSE table with matching W_ID.
 *  W_NAME, W_STREET_1, W_STREET_2, W_CITY, W_STATE and W_ZIP are retrieved.
 *  W_YTD is increased by H_AMOUNT.
 *  
 *  @return 0 on success, non-zero on failure
 */

int updateShoreWarehouse(payment_input_t* pin, ShoreTPCCEnv* env)
{
    ///////////////////////////////
    ///// START UPD_WAREHOUSE /////    

    assert (env); // ensure that we have a valid environment
        
    TRACE( TRACE_TRX_FLOW, 
           "Step 2: Updating the row in the WAREHOUSE table with matching W_ID=%d\n", 
           pin->_home_wh_id );
    
    // WAREHOUSE key: W_ID 
    //tpcc_warehouse_tuple_key wk;

    // Calculate index in array
    //*idx = (pin->_home_wh_id - 1);
    tpcc_warehouse_tuple* warehouse = NULL;

    assert (false); // (ip) TODO


    /*
    // Get the entry
    if ((warehouse = env->im_warehouses.write(*idx)) == NULL) {
        // No entry returned
        THROW2(TrxException, "Called Warehouses.write(%d)\n", *idx);
        *idx = -1;
        return (1);
    }
    else if (warehouse->W_ID != pin->_home_wh_id) {
        // Entry returned but keys do not match
        // Should abort, but also unlock that entry
        
        if (env->im_warehouses.release(*idx)) {
            // Error while trying to release entry
            *idx = -1;
            THROW2(TrxException, "Called Warehouses.release(%d)\n", 
                   pin->_home_wh_id);
        }

        *idx = -1;
        THROW2(TrxException, 
               "Warehouse with id=(%d) not found in the database\n", 
               pin->_home_wh_id);    

        return (1);
    }
    */

    // If we reached this point we have the valid entry.
    // We can update and release lock
    
    assert(warehouse); // make sure that we got a warehouse
    
    // Update warehouse
    warehouse->W_YTD += pin->_h_amount;

    // Assume always success if this point is reached
    return (0);

    ///// EOF UPD_WAREHOUSE //////
    //////////////////////////////
}



/** @fn updateShoreDistrict
 *  
 *  @brief Step 3: Retrieve the row in the DISTRICT table with matching D_W_ID and D_ID.
 *  D_NAME, D_STREET_1, D_STREET_2, D_CITY, D_STATE and D_ZIP are retrieved.
 *  D_YTD is increased by H_AMOUNT.
 *
 *  @note For the in-memory version the Districts are stored in an array. 
 *  We access the array through the latchedArray interface which returns
 *  the entries locked. On sucess the idx of the updated entry is propagated
 *  to the caller function. Otherwise, it tries to unlock here.
 *  
 *  @return 0 on success, non-zero on failure
 */

int updateShoreDistrict(payment_input_t* pin, ShoreTPCCEnv* env)
{
    //////////////////////////////
    ///// START UPD_DISTRICT /////

    assert (env); // ensure that we have a valid environment
    
    TRACE( TRACE_TRX_FLOW, 
           "Step 3: Updating the row in the DISTRICT table with matching " \
           "D_ID=%d and D_W_ID=%d\n", 
           pin->_home_d_id, 
           pin->_home_wh_id);
    
    // DISTRICT key: D_ID, D_W_ID 
    tpcc_district_tuple_key dk;
    dk.D_ID = pin->_home_d_id;
    dk.D_W_ID = pin->_home_wh_id;
    tpcc_district_tuple* district = NULL;

    // Calculate index in the array
    //*idx = ((dk.D_W_ID - 1) * 10) + (dk.D_ID - 1);

    // Get the entry
    assert (false); // (ip) TODO
    /*
    if ((district = env->im_districts.write(*idx)) == NULL) {
        // No entry returned
        *idx = -1;
        THROW3(TrxException, "Called Districts.write(%d,%d)\n",
               dk.D_ID,
               dk.D_W_ID);
        return (1);
    }
    else if ( (district->D_ID != dk.D_ID) || 
              (district->D_W_ID != dk.D_W_ID) ) 
        {
            // Entry returned but keys do not match
            // Should abort, but also unlock that entry
            if (env->im_districts.release(*idx)) {
                // Error while trying to release entry
                *idx = -1;
                THROW3(TrxException, "Called Districts.release(%d,%d)\n",
                       dk.D_ID,
                       dk.D_W_ID);
            }

            *idx = -1;
            THROW3(TrxException, 
                   "District with id=(%d,%d) not found in the database\n", 
                   dk.D_ID,
                   dk.D_W_ID);    

            return (1);
        }
    */

    // If we reached this point we have the valid entry.
    // We can update it. The lock will be released inside the
    // caller (executeShorePaymentX()) function
    
    assert(district); // make sure that we got a district

    // Update district
    district->D_YTD += pin->_h_amount;

    // Assume always success if this point is reached
    return (0);
    
    ///// EOF UPD_DISTRICT /////
    ////////////////////////////
}



/** @fn updateShoreCustomerByID
 *
 *  @brief Step 4, Case 1: The customer is selected based on customer number.
 *  That is, the row in the CUSTOMER table with matching C_W_ID, C_D_ID and C_ID
 *  is selected. C_FIRST, C_MIDDLE, C_LAST, C_STREET_1, C_STREET_2, C_CITY, C_STATE,
 *  C_ZIP, C_PHONE, C_SINCE, C_CREDIT, C_CREDIT_LIM, C_DISCOUNT, C_BALANCE, C_DATA_1,
 *  and C_DATA_2 are retrieved.
 *  C_BALANCE is decreased by H_AMOUNT. 
 *  C_YTD_PAYMENT is increased by H_AMOUNT. 
 *  C_PAYMENT_CNT is increased by 1.
 *  updateCustomerData is called.
 *  
 *  @return 0 on success, non-zero on failure
 */    

int updateShoreCustomerByID(int wh_id, int d_id, 
			    int c_id, decimal h_amount, 
                            ShoreTPCCEnv* env)
{
    assert (false);

    assert (env); // ensure that we have a valid environment

    tpcc_customer_tuple_key ck;
    ck.C_C_ID = c_id;
    ck.C_D_ID = d_id;
    ck.C_W_ID = wh_id;

    tpcc_customer_tuple_body pcb;

    assert (false); // (ip) TODO
    /*
    if (env->im_customers.find(ck, &pcb) == false) {
        THROW4( BdbException,
                "Customer with id=(%d,%d,%d) not found in the database\n", 
                ck.C_C_ID,
                ck.C_D_ID,
                ck.C_W_ID);

        return (1);
    }
    */

    TRACE( TRACE_RECORD_FLOW, 
           "\nBal: (%.2f) Payment: (%.2f) Cnt: (%d) Cred: (%s) Amount: (%.2f)\n",
           pcb.C_BALANCE.to_double(),
           pcb.C_YTD_PAYMENT.to_double(),
           pcb.C_PAYMENT_CNT,
           pcb.C_CREDIT,
           h_amount.to_double());


    // updating customer data
    pcb.C_BALANCE -= h_amount;
    pcb.C_YTD_PAYMENT += h_amount;
    pcb.C_PAYMENT_CNT++;

    TRACE( TRACE_RECORD_FLOW, 
           "\nBal: (%.2f) Payment: (%.2f) Cnt: (%d) Cred: (%s) Amount: (%.2f)\n",
           pcb.C_BALANCE.to_double(),
           pcb.C_YTD_PAYMENT.to_double(),
           pcb.C_PAYMENT_CNT,
           pcb.C_CREDIT,
           h_amount.to_double());    

    
    if (strncmp(pcb.C_CREDIT, "BC", 2) == 0) {
        updateShoreCustomerData(ck, &pcb);
    }

    // Update the Customer
    // @note In the BPTree implementation in order to update simply insert
    // with the same key
    assert (false); // (ip) TODO
    //    env->im_customers.insert(ck, pcb);

    // Assume success if this point is reached
    return (0);
}




/** @fn updateShoreCustomerByLast
 *
 *  @brief Step 4, Case 2: The customer is selected based on customer last name.
 *  That is, all rows in the CUSTOMER table with matching C_W_ID, C_D_ID and C_LAST
 *  are selected sorted by C_FIRST in ascending order. Let (n) be the number of rows
 *  selected. C_ID, C_FIRST, C_MIDDLE, C_LAST, C_STREET_1, C_STREET_2, C_CITY, C_STATE,
 *  C_ZIP, C_PHONE, C_SINCE, C_CREDIT, C_CREDIT_LIM, C_DISCOUNT, C_BALANCE, C_DATA_1,
 *  and C_DATA_2 are retrieved from the row at position (n/2 rounded up to the next
 *  integer) in the sorted set of selected rows.
 *  C_BALANCE is decreased by H_AMOUNT. 
 *  C_YTD_PAYMENT is increased by H_AMOUNT. 
 *  C_PAYMENT_CNT is increased by 1.
 *  updateCustomerData is called.
 *  
 *  @return 0 on success, non-zero on failure
 */

int updateShoreCustomerByLast(int wh_id, int d_id, 
                              char* c_last, decimal h_amount,
                              ShoreTPCCEnv* env)
{
    assert (false);

    assert (env); // ensure that we have a valid environment

    assert(1==0); // FIXME (ip) Not implemented yet

    int iResult = 0;

    tpcc_customer_tuple_body a_cb;
    tpcc_customer_tuple_key ck;
    
    TRACE( TRACE_ALWAYS, "*** TO BE DONE! Updating Customer using C_LAST\n");

    
    /** Retrieving CUSTOMER rows using C_LAST (and a cursor) */
    
    if (strncmp(a_cb.C_CREDIT, "BC", 2) == 0) {
        updateShoreCustomerData(ck, &a_cb);
    } 
   
    return (iResult);
}
    
    
/** @fn updateShoreCustomerData
 *
 *  @brief If the value of C_CREDIT is equal to "BC", then the following history
 *  information: C_ID, C_D_ID, C_W_ID, D_ID, W_ID, and H_AMOUNT, are inserted at
 *  the left of C_DATA field by shifting the existing content of C_DATA to the right
 *  by an equal number of bytes and by discarding the bytes that are shifted out of
 *  the right side of the C_DATA field. The content of the C_DATA field never exceeds
 *  500 characters. The selected customer is updated with the new C_DATA field.
 */
    
void updateShoreCustomerData(tpcc_customer_tuple_key ck, 
                             tpcc_customer_tuple_body* a_cb) 
{
    assert (false);

    assert( a_cb );
    assert( strncmp(a_cb->C_CREDIT, "BC", 2) == 0 );

    // FIXME (ip) Modification instead of writing D_ID, W_ID, and H_AMOUNT
    // we write again C_D_ID, C_W_ID, and C_BALANCE

    char tmp [STRSIZE(500)];

    // writes new info
    sprintf(&tmp[0], "%d:%d:%d:%d:%d:%.2f|",
            ck.C_C_ID,
            ck.C_D_ID,
            ck.C_W_ID,
            ck.C_D_ID,
            ck.C_W_ID,
            a_cb->C_BALANCE.to_double());

    int i = strlen(tmp);

    // copies first half of old data, which is stored 
    // on the first member variable
    strncpy(&tmp[i], a_cb->C_DATA_1, 250);

    // copies part of the second half of old data, which
    // is stored on the second member variable
    strncpy(&tmp[i+250], a_cb->C_DATA_2, 250-i);

    TRACE( TRACE_RECORD_FLOW,
           "Before\nDATA1 - %s\nDATA2 - %s\n",
           a_cb->C_DATA_1,
           a_cb->C_DATA_2);

    // copy back to the structure
    strncpy(a_cb->C_DATA_1, tmp, 250);
    a_cb->C_DATA_1[250] = '\0';
    strncpy(a_cb->C_DATA_2, &tmp[250], 250);
    a_cb->C_DATA_2[250] = '\0';


    TRACE( TRACE_RECORD_FLOW,
           "After\nDATA1 - %s\nDATA2 - %s\n",
           a_cb->C_DATA_1,
           a_cb->C_DATA_2);
}


/** @fn executeShorePaymentBaseline
 *
 *  @brief Executes the SHORE_PAYMENT transaction serially. Wrapper function.
 *  
 *  @return A trx_result_tuple_t with the status of the transaction.
 */
    
trx_result_tuple_t executeShorePaymentBaseline(payment_input_t pin, const int id, 
                                               ShoreTPCCEnv* env)
{        
    assert (env); // ensure that we have a valid environment

    // initialize the result structure
    trx_result_tuple_t aTrxResultTuple(UNDEF, id);

    try {                
        TRACE( TRACE_TRX_FLOW, "*** EXECUTING SHORE-TRX CONVENTIONALLY ***\n");
    
        /** @note PAYMENT TRX 
         *  According to TPC-C benchmark, Revision 5.8.0 pp. 32-35 
         */    

        pin.describe(id);

        /** Starts the trx */


        /** Step 1: The database transaction is started */
        TRACE( TRACE_TRX_FLOW, "Step 1: The database transaction is started\n" );

        if (updateShoreWarehouse(&pin, env)) {
            
            // Failed. Throw exception
            TRACE( TRACE_ALWAYS, 
                   "WAREHOUSE update failed...\nAborting\n");
        }


        if (updateShoreDistrict(&pin, env)) {
            
            // Failed. Throw exception
            THROW1( TrxException, 
                    "DISTRICT update failed...\nAborting\n");
        }


        if (updateShoreCustomer(&pin, env)) {

            // Failed. Throw exception
            THROW1( TrxException,
                    "CUSTOMER update failed...\nAborting\n");
        }

        if (insertShoreHistory(&pin, env)) {

            // Failed. Throw exception
            THROW1( TrxException, 
                    "HISTORY instertion failed...\nAborting\n");
        }

    
        /** Step 6: The database transaction is committed */
        TRACE( TRACE_TRX_FLOW, "Step 6: The transaction is committed\n" );

        // Releases the locks in the Warehouse and the District
        // In reverse order. That is, it first releases the District
        // and then the Warehouse.

        // Release district
        assert (false); // (ip) TODO
        /*
        if (env->im_districts.release(myDistrictIdx)) {
            // Error while trying to release entry
            THROW2(TrxException, "Called Districts.release(%d)\n", 
                   myDistrictIdx);
        }    


        // Release warehouse
        assert (false); // (ip) TODO
        if (env->im_warehouses.release(myWarehouseIdx)) {
            // Error while trying to release entry
            THROW2(TrxException, "Called Warehouses.release(%d)\n", 
                   myWarehouseIdx);
        } 
        */   

    }        
    catch(...) {
        TRACE( TRACE_ALWAYS, 
               "********************************\n" \
               "Unknown Exception - Aborting PAYMENT trx...\n");
        

        aTrxResultTuple.set_state(ROLLBACKED);
        return (aTrxResultTuple);

        // FIXME (ip): We may want to have retries
        //if (++failed_tries > MAX_TRIES) {
        //   txn->abort();
        //   TRACE( TRACE_ALWAYS, "MAX_FAILS - Aborting...\n");        
    }

    // if reached this point transaction succeeded
    aTrxResultTuple.set_state(COMMITTED);
    return (aTrxResultTuple);

} // EOF: executePaymentBaseline



////////////////////////////////////////////////////////////
// STAGED Functions
////////////////////////////////////////////////////////////



/** @fn staged_updateShoreWarehouse
 *
 *  @brief Updates the Shore Warehouse without locking. In a Staged TP no
 *  locking in the Array is required. 
 * 
 *  @return 0 on success, non-zero otherwise
 */

int staged_updateShoreWarehouse(payment_input_t* pin, ShoreTPCCEnv* env) {

    ///////////////////////////////////////
    ///// START STAGED_UPD_WAREHOUSE /////    

    assert (env); // ensure that we have a valid environment
        
    TRACE( TRACE_TRX_FLOW, 
           "Step 2: Updating the row in the WAREHOUSE table with matching W_ID=%d\n", 
           pin->_home_wh_id );

    // Calculate index in array
    int idx = (pin->_home_wh_id - 1);
    tpcc_warehouse_tuple* warehouse = NULL;

    // Get the entry
    assert (false); // (ip) TODO
    /*
    if ((warehouse = env->im_warehouses.read_nl(idx)) == NULL) {
        // No entry returned
        THROW2(TrxException, "Called Warehouses.read_nl(%d)\n", idx);
        return (1);
    }
    */

    // If we reached this point we have the valid entry.
    // We simply update
    assert(warehouse); // make sure that we got a warehouse
    
    // Update warehouse
    warehouse->W_YTD += pin->_h_amount;    

    // Assume always success if this point is reached
    return (0);

    ///// EOF STAGED_UPD_WAREHOUSE //////
    /////////////////////////////////////
}



/** @fn staged_updateShoreDistrict
 *
 *  @brief Updates the Shore District without locking. In a Staged TP no
 *  locking in the Array is required. 
 * 
 *  @return 0 on success, non-zero otherwise
 */

int staged_updateShoreDistrict(payment_input_t* pin, ShoreTPCCEnv* env) {

    //////////////////////////////
    ///// START UPD_DISTRICT /////

    assert (env); // ensure that we have a valid environment
    
    TRACE( TRACE_TRX_FLOW, 
           "Step 3: Updating the row in the DISTRICT table with matching " \
           "D_ID=%d and D_W_ID=%d\n", 
           pin->_home_d_id, 
           pin->_home_wh_id);

    // DISTRICT key: D_ID, D_W_ID 
    tpcc_district_tuple_key dk;
    dk.D_ID = pin->_home_d_id;
    dk.D_W_ID = pin->_home_wh_id;

    tpcc_district_tuple* district = NULL;

    // Calculate index in the array
    int idx = ((dk.D_W_ID - 1) * 10) + (dk.D_ID - 1);

    // Get the entry
    assert (false); // (ip) TODO
    /*
    if ((district = env->im_districts.read_nl(idx)) == NULL) {
        // No entry returned
        THROW3(TrxException, "Called Districts.write(%d,%d)\n",
               dk.D_ID,
               dk.D_W_ID);
        return (1);
    }
    */

    // If we reached this point we have the valid entry.
    // We can update it    
    assert(district); // make sure that we got a district

    // Update district
    district->D_YTD += pin->_h_amount;

    // Assume always success if this point is reached
    return (0);
    
    ///// EOF UPD_DISTRICT /////
    ////////////////////////////
}


int staged_updateShoreCustomer(payment_input_t* pin, ShoreTPCCEnv* env) 
{
    assert (false); // TODO
    return (updateShoreCustomer(pin, env));
}


/** @fn staged_insertShoreHistory
 *
 *  @brief Inserts into Shore History.
 *
 *  @return 0 on success, non-zero otherwise
 */

int staged_insertShoreHistory(payment_input_t* pin, ShoreTPCCEnv* env) 
{
    assert (false); // TODO
    return (insertShoreHistory(pin, env));
}



/** @fn executeShorePaymentStaged
 *
 *  @brief Executes the SHORE_PAYMENT transaction in a Staged manner/
 *  
 *  @return A trx_result_tuple_t with the status of the transaction.
 */
    
trx_result_tuple_t executeShorePaymentStaged(payment_input_t pin, const int id,
                                             ShoreTPCCEnv* env)
{        
    assert (env); // ensure that we have a valid environment

    // initialize the result structure
    trx_result_tuple_t aTrxResultTuple(UNDEF, id);

    try {                
        TRACE( TRACE_TRX_FLOW, "*** EXECUTING SHORE-TRX STAGED ***\n");
    
        /** @note PAYMENT TRX 
         *  According to TPC-C benchmark, Revision 5.8.0 pp. 32-35 
         */    

        pin.describe(id);

        /** Starts the trx */
        assert (false); // TODO
    }        
    catch(...) {
        TRACE( TRACE_ALWAYS, 
               "********************************\n" \
               "Unknown Exception - Aborting PAYMENT trx...\n");
        

        aTrxResultTuple.set_state(ROLLBACKED);
        return (aTrxResultTuple);
    }

    // if reached this point transaction succeeded
    aTrxResultTuple.set_state(COMMITTED);
    return (aTrxResultTuple);

} // EOF: executePaymentBaseline



EXIT_NAMESPACE(tpcc_payment);

