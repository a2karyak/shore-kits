/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file tpcc_payment_common.cpp
 *
 *  @brief Implements common functionality for the clients that submit 
 *  PAYMENT_BASELINE or PAYMENT_STAGED transactions
 *
 *  @author Ippokratis Pandis (ipandis)
 */

# include "util.h"

# include "workload/tpcc/drivers/common.h"
# include "workload/tpcc/drivers/tpcc_payment_common.h"


using namespace qpipe;
using namespace tpcc_payment;


ENTER_NAMESPACE(tpcc);


/** @fn create_payment_input
 *
 *  @brief Creates the input for a new PAYMENT request, 
 *  given the scaling factor (sf) of the database
 */

payment_input_t create_payment_input(int sf) {

    // check scaling factor
    assert(sf > 0);

    // produce PAYMENT params according to tpcc spec v.5.4
    payment_input_t pin;

#ifndef USE_SAME_INPUT

    pin._home_wh_id = URand(1, sf);
    pin._home_d_id = URand(1, 10);
    pin._v_cust_wh_selection = URand(1, 100); // 85 - 15
    pin._remote_wh_id = URand(1, sf);
    pin._remote_d_id = URand(1, 10);
    pin._v_cust_ident_selection = URand(1, 100); // 60 - 40
    pin._c_id = NURand(1023, 1, 3000);
    
    char * tCustLast = generate_cust_last(NURand(255, 0, 999));
    store_string(pin._c_last,  tCustLast);
    
    pin._h_amount = (long)URand(100, 500000)/(long)100.00;
    pin._h_date = time(NULL);
        
    if (tCustLast)
        delete (tCustLast);        
#else
    pin._home_wh_id = 1;
    pin._home_d_id =  1;
    pin._v_cust_wh_selection = 50;
    pin._remote_wh_id = 1;
    pin._remote_d_id =  1;
    pin._v_cust_ident_selection = 50;
    pin._c_id =  1500;        
    //    pin._c_last = NULL;
    pin._h_amount = 1000.00;
    pin._h_date = time(NULL);
#endif        

    return (pin);
}


/** @fn allocate_payment_dbts
 *
 *  @brief Allocates the required dbts for the PAYMENT_* transactions
 *  and sets their flag as memory allocated by user.
 */

void allocate_payment_dbts(s_payment_dbt_t* p_dbts) {

    assert (p_dbts);

    /** @note The caller (tpcc_payment_*_driver) is the owner of
     *  those dbts
     */

    memset(&p_dbts->whData, 0, sizeof(Dbt));
    p_dbts->whData.set_data(malloc(sizeof(tpcc_warehouse_tuple)));
    p_dbts->whData.set_ulen(sizeof(tpcc_warehouse_tuple));
    p_dbts->whData.set_flags(DB_DBT_USERMEM);


    memset(&p_dbts->distData, 0, sizeof(Dbt));
    p_dbts->distData.set_data(malloc(sizeof(tpcc_district_tuple)));
    p_dbts->distData.set_ulen(sizeof(tpcc_district_tuple));
    p_dbts->distData.set_flags(DB_DBT_USERMEM);


    memset(&p_dbts->custData, 0, sizeof(Dbt));
    p_dbts->custData.set_data(malloc(sizeof(tpcc_customer_tuple)));
    p_dbts->custData.set_ulen(sizeof(tpcc_customer_tuple));
    p_dbts->custData.set_flags(DB_DBT_USERMEM);
}


/** @fn deallocate_payment_dbts
 *
 *  @brief Deallocates the required dbts for the PAYMENT_* transactions
 */

void deallocate_payment_dbts(s_payment_dbt_t* p_dbts) {

    assert (p_dbts);

    if (p_dbts->whData.get_data())
        delete ((tpcc_warehouse_tuple*)p_dbts->whData.get_data());

    if (p_dbts->distData.get_data())
        delete ((tpcc_district_tuple*)p_dbts->distData.get_data());
    
    if (p_dbts->custData.get_data())
        delete ((tpcc_customer_tuple*)p_dbts->custData.get_data());
}



/** @fn reset_payment_dbts
 *
 *  @brief Resets the required dbts for the PAYMENT_* transactions
 */

void reset_payment_dbts(s_payment_dbt_t* p_dbts) {

    assert (p_dbts);
        
    // FIXME (ip) Do we really need to do anything here?
}


EXIT_NAMESPACE(tpcc);
