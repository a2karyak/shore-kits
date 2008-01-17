/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file tpcc_input.h
 *
 *  @brief Declaration for the (common) inputs for the TPC-C transactions
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __TPCC_INPUT_H
#define __TPCC_INPUT_H


ENTER_NAMESPACE(tpcc);


/** Exported data structures */


////////////////////////////////////////////
// Class payment_input_t
//
// @brief Input for any PAYMENT transaction

class payment_input_t {

public:

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

    // Construction/Destructions
    payment_input_t() { };

    ~payment_input_t() { 
        // if (_c_last) delete (_c_last);
    };


    // Assignment operator
    payment_input_t& operator= (const payment_input_t& rhs) {

        // copy input
        _home_wh_id = rhs._home_wh_id;
        _home_d_id = rhs._home_d_id;
        _v_cust_wh_selection = rhs._v_cust_wh_selection;
        _remote_wh_id = rhs._remote_wh_id;
        _remote_d_id = rhs._remote_d_id;
        _v_cust_ident_selection = rhs._v_cust_ident_selection;
        _c_id = rhs._c_id;
        _h_amount = rhs._h_amount;

        if (rhs._c_last) {
            store_string(_c_last, rhs._c_last);
        }        

        return (*this);
    }


    /** Helper Function */

    void describe( int id = 0 ) {

        if (_c_last) {

            TRACE( TRACE_TRX_FLOW,
                   "\nPAYMENT: ID=%d\nWH_ID=%d\t\tD_ID=%d\n"  \
                   "SEL_WH=%d\tSEL_IDENT=%d\n"  \
                   "REM_WH_ID=%d\tREM_D_ID=%d\n"        \
                   "C_ID=%d\tC_LAST=%s\n"               \
                   "H_AMOUNT=%.2f\tH_DATE=%d\n",        \
                   id,
                   _home_wh_id, 
                   _home_d_id, 
                   _v_cust_wh_selection, 
                   _v_cust_ident_selection,
                   _remote_wh_id, 
                   _remote_d_id,
                   _c_id, 
                   _c_last,
                   _h_amount, 
                   _h_date);
        }
        else {
            
            // If using the standard input, _c_last is NULL
            TRACE( TRACE_TRX_FLOW,                   
                   "\nPAYMENT: ID=%d\nWH_ID=%d\t\tD_ID=%d\n"      \
                   "SEL_WH=%d\tSEL_IDENT=%d\n"  \
                   "REM_WH_ID=%d\tREM_D_ID=%d\n"      \
                   "C_ID=%d\tH_AMOUNT=%.2f\tH_DATE=%d\n",       \
                   id,
                   _home_wh_id, 
                   _home_d_id, 
                   _v_cust_wh_selection, 
                   _v_cust_ident_selection,
                   _remote_wh_id, 
                   _remote_d_id,
                   _c_id, 
                   _h_amount, 
                   _h_date);
        }
    }

}; // EOF payment_input_t



//////////////////////////////////////////////
// Class new_order_input_t
//
// @brief Input for any NEW_ORDER transaction

class new_order_input_t {

public:

    /**
     *  @brief NEW_ORDER transaction inputs:
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

    // Construction/Destructions
    new_order_input_t() { 
        assert ( 1 == 0 ); // (ip) Not implemented yet
    };
    
    ~new_order_input_t() { };

    // Assignment operator
    new_order_input_t& operator= (const new_order_input_t& rhs) {

        assert ( 1 == 0 ); // (ip) Not implemented yet

        // copy input
        _home_wh_id = rhs._home_wh_id;
        _home_d_id = rhs._home_d_id;
        _v_cust_wh_selection = rhs._v_cust_wh_selection;
        _remote_wh_id = rhs._remote_wh_id;
        _remote_d_id = rhs._remote_d_id;
        _v_cust_ident_selection = rhs._v_cust_ident_selection;
        _c_id = rhs._c_id;
        _h_amount = rhs._h_amount;

        if (rhs._c_last) {
            store_string(_c_last, rhs._c_last);
        }        

        return (*this);
    }


    /** Helper Function */

    void describe( int id = 0 ) {

        assert ( 1 == 0 ); // (ip) Not implemented yet

        if (_c_last) {

            TRACE( TRACE_TRX_FLOW,
                   "\nNEW_ORDER: ID=%d\nWH_ID=%d\t\tD_ID=%d\n"  \
                   "SEL_WH=%d\tSEL_IDENT=%d\n"  \
                   "REM_WH_ID=%d\tREM_D_ID=%d\n"        \
                   "C_ID=%d\tC_LAST=%s\n"               \
                   "H_AMOUNT=%.2f\tH_DATE=%d\n",        \
                   id,
                   _home_wh_id, 
                   _home_d_id, 
                   _v_cust_wh_selection, 
                   _v_cust_ident_selection,
                   _remote_wh_id, 
                   _remote_d_id,
                   _c_id, 
                   _c_last,
                   _h_amount, 
                   _h_date);
        }
        else {
            
            // If using the standard input, _c_last is NULL
            TRACE( TRACE_TRX_FLOW,                   
                   "\nPAYMENT: ID=%d\nWH_ID=%d\t\tD_ID=%d\n"      \
                   "SEL_WH=%d\tSEL_IDENT=%d\n"  \
                   "REM_WH_ID=%d\tREM_D_ID=%d\n"      \
                   "C_ID=%d\tH_AMOUNT=%.2f\tH_DATE=%d\n",       \
                   id,
                   _home_wh_id, 
                   _home_d_id, 
                   _v_cust_wh_selection, 
                   _v_cust_ident_selection,
                   _remote_wh_id, 
                   _remote_d_id,
                   _c_id, 
                   _h_amount, 
                   _h_date);
        }
    }

}; // EOF new_order_input_t




//////////////////////////////////////////////
// Class orderline_input_t
//
// @brief Input for any OLDERLINE request
//
// @note Part of the NEW_ORDER transaction

class orderline_input_t {

public:

    /**
     *  @brief ORDERLINE transaction inputs:
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

    // Construction/Destructions
    orderline_input_t() { 
        assert ( 1 == 0 ); // (ip) Not implemented yet        
    };

    virtual ~orderline_input_t() { };

    // Assignment operator
    orderline_input_t& operator= (const orderline_input_t& rhs) {

        assert ( 1 == 0 ); // (ip) Not implemented yet

        // copy input
        _home_wh_id = rhs._home_wh_id;
        _home_d_id = rhs._home_d_id;
        _v_cust_wh_selection = rhs._v_cust_wh_selection;
        _remote_wh_id = rhs._remote_wh_id;
        _remote_d_id = rhs._remote_d_id;
        _v_cust_ident_selection = rhs._v_cust_ident_selection;
        _c_id = rhs._c_id;
        _h_amount = rhs._h_amount;

        if (rhs._c_last) {
            store_string(_c_last, rhs._c_last);
        }        

        return (*this);
    }


    /** Helper Function */

    void describe( int id = 0 ) {

        assert ( 1 == 0 ); // (ip) Not implemented yet

        if (_c_last) {

            TRACE( TRACE_TRX_FLOW,
                   "\nNEW_ORDER: ID=%d\nWH_ID=%d\t\tD_ID=%d\n"  \
                   "SEL_WH=%d\tSEL_IDENT=%d\n"  \
                   "REM_WH_ID=%d\tREM_D_ID=%d\n"        \
                   "C_ID=%d\tC_LAST=%s\n"               \
                   "H_AMOUNT=%.2f\tH_DATE=%d\n",        \
                   id,
                   _home_wh_id, 
                   _home_d_id, 
                   _v_cust_wh_selection, 
                   _v_cust_ident_selection,
                   _remote_wh_id, 
                   _remote_d_id,
                   _c_id, 
                   _c_last,
                   _h_amount, 
                   _h_date);
        }
        else {
            
            // If using the standard input, _c_last is NULL
            TRACE( TRACE_TRX_FLOW,                   
                   "\nPAYMENT: ID=%d\nWH_ID=%d\t\tD_ID=%d\n"      \
                   "SEL_WH=%d\tSEL_IDENT=%d\n"  \
                   "REM_WH_ID=%d\tREM_D_ID=%d\n"      \
                   "C_ID=%d\tH_AMOUNT=%.2f\tH_DATE=%d\n",       \
                   id,
                   _home_wh_id, 
                   _home_d_id, 
                   _v_cust_wh_selection, 
                   _v_cust_ident_selection,
                   _remote_wh_id, 
                   _remote_d_id,
                   _c_id, 
                   _h_amount, 
                   _h_date);
        }
    }

}; // EOF orderline_input_t



EXIT_NAMESPACE(tpcc);


#endif
