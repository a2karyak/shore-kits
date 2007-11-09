/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file tpcc_new_order_common.cpp
 *
 *  @brief Implements function that creates a NEW_ORDER input
 *
 *  @author Ippokratis Pandis (ipandis)
 */

# include "util.h"

# include "workload/tpcc/drivers/common.h"
# include "workload/tpcc/drivers/tpcc_new_order_common.h"


ENTER_NAMESPACE(tpcc);


/** @fn create_new_order_input
 *
 *  @brief Creates the input for a new NEW_ORDER request, 
 *  given the scaling factor (sf) of the database
 */

new_order_input_t create_new_order_input(int sf) {

    // check scaling factor
    assert(sf > 0);

    // produce NEW_ORDER params according to tpcc spec v.5.4
    new_order_input_t noin;
    
    assert ( 1 == 0 ); // (ip) Not implemented yet

    /*
#ifndef USE_SAME_INPUT

    pin._home_wh_id = URand(1, sf);
    pin._home_d_id = URand(1, 10);
    pin._v_cust_wh_selection = URand(1, 100); // 85 - 15
    pin._remote_wh_id = URand(1, sf);
    pin._remote_d_id = URand(1, 10);
    pin._v_cust_ident_selection = URand(1, 100); // 60 - 40
    pin._c_id = NURand(1023, 1, 3000);
    
    // Calls the function that returns the correct cust_last
    store_string(pin._c_last,  generate_cust_last(NURand(255, 0, 999)));
    
    pin._h_amount = (long)URand(100, 500000)/(long)100.00;
    pin._h_date = time(NULL);

#else
    // Same input
    pin._home_wh_id = 1;
    pin._home_d_id =  1;
    pin._v_cust_wh_selection = 50;
    pin._remote_wh_id = 1;
    pin._remote_d_id =  1;
    pin._v_cust_ident_selection = 50;
    pin._c_id =  1500;        
    pin._c_last = NULL;
    pin._h_amount = 1000.00;
    pin._h_date = time(NULL);
#endif        
    */

    return (noin);
}




/** @fn create_orderline_input
 *
 *  @brief Creates the input for a insert ORDERLINE request, 
 *  given the scaling factor (sf) of the database
 */

orderline_input_t create_orderline_input(int sf) {

    // check scaling factor
    assert(sf > 0);

    // produce insert ORDERLINE params according to tpcc spec v.5.4
    orderline_input_t olin;
    
    assert ( 1 == 0 ); // (ip) Not implemented yet

    /*
#ifndef USE_SAME_INPUT

    pin._home_wh_id = URand(1, sf);
    pin._home_d_id = URand(1, 10);
    pin._v_cust_wh_selection = URand(1, 100); // 85 - 15
    pin._remote_wh_id = URand(1, sf);
    pin._remote_d_id = URand(1, 10);
    pin._v_cust_ident_selection = URand(1, 100); // 60 - 40
    pin._c_id = NURand(1023, 1, 3000);
    
    // Calls the function that returns the correct cust_last
    store_string(pin._c_last,  generate_cust_last(NURand(255, 0, 999)));
    
    pin._h_amount = (long)URand(100, 500000)/(long)100.00;
    pin._h_date = time(NULL);

#else
    // Same input
    pin._home_wh_id = 1;
    pin._home_d_id =  1;
    pin._v_cust_wh_selection = 50;
    pin._remote_wh_id = 1;
    pin._remote_d_id =  1;
    pin._v_cust_ident_selection = 50;
    pin._c_id =  1500;        
    pin._c_last = NULL;
    pin._h_amount = 1000.00;
    pin._h_date = time(NULL);
#endif        
    */

    return (olin);
}



EXIT_NAMESPACE(tpcc);
