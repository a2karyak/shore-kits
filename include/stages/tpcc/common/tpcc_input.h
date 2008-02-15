/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file tpcc_input.h
 *
 *  @brief Declaration of the (common) inputs for the TPC-C trxs
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __TPCC_INPUT_H
#define __TPCC_INPUT_H

#include "util.h"
#include "stages/tpcc/common/tpcc_const.h"


ENTER_NAMESPACE(tpcc);


/** Exported data structures */



/*********************************************************************
 * 
 * @class bstract trx_input_t
 *
 * @brief Base class for the Input of any transaction
 *
 *********************************************************************/

class trx_input_t {
public:
    trx_input_t() { }
    virtual ~trx_input_t() { }
    virtual void describe(int id)=0;
    virtual void gen_input(int sf)=0;

}; // EOF: trx_input_t



/*********************************************************************
 * 
 * payment_input_t
 *
 * Input for any PAYMENT transaction
 *
 *********************************************************************/

class payment_input_t : public trx_input_t {
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

    ~payment_input_t() { };

    payment_input_t& operator= (const payment_input_t& rhs);
    void describe(int id=0);
    void gen_input(int sf);

}; // EOF payment_input_t




/*********************************************************************
 * 
 * new_order_input_t
 *
 * Input for any NEW_ORDER transaction
 *
 *********************************************************************/

class new_order_input_t : public trx_input_t {
public:

    short   _wh_id;         /* input */
    short   _d_id;          /* input: URand(1,10) */
    short   _c_id;          /* input: NURand(1023, 1, 3000) */
    short   _ol_cnt;        /* input: number of items URand(5,15) */
    short   _rbk;           /* input: rollback URand(1,100) */

    struct  _ol_item_info {
        int     _ol_i_id;             /* input: NURand(8191, 1, 100000) */
        short   _ol_supply_wh_select; /* input: URand(1, 100) */
        short   _ol_supply_wh_id;     /* input: x==1 -> URand(1, WHs) */
        short   _ol_quantity;         /* input: URand(1, 10) */        
    }  items[MAX_OL_PER_ORDER];

    /** If _supply_wh_id = _wh_id for each item 
     *  then trx called home, else remote 
     */    

    // Construction/Destructions
    new_order_input_t() { };    
    ~new_order_input_t() { };

    // Assignment operator
    new_order_input_t& operator= (const new_order_input_t& rhs);

    // Methods
    void describe(int id);
    void gen_input(int id);

}; // EOF new_order_input_t




/*********************************************************************
 * 
 * orderline_input_t
 *
 * Input for any ORDERLINE transaction
 *
 *********************************************************************/

class orderline_input_t : public trx_input_t {
public:
    // Construction/Destructions
    orderline_input_t() { 
        assert (false); // (ip) Not implemented yet
    };
    
    ~orderline_input_t() { };


    // Assignment operator
    orderline_input_t& operator= (const orderline_input_t& rhs);

    // Methods
    void describe(int id=0);
    void gen_input(int id);

}; // EOF orderline_input_t




/*********************************************************************
 * 
 * order_status_input_t
 *
 * Input for any ORDER_STATUS transaction
 *
 *********************************************************************/

class order_status_input_t : public trx_input_t {
public:

    short    _w_id;                  /* input */
    short    _d_id;                  /* input */
    int      _c_id;                  /* input */
    char     _c_last[17];            /* input */


    // Construction/Destructions
    order_status_input_t() { 
        assert (false); // (ip) Not implemented yet
    };
    
    ~order_status_input_t() { };


    // Assignment operator
    order_status_input_t& operator= (const order_status_input_t& rhs);

    // Methods
    void describe(int id=0);
    void gen_input(int id);

}; // EOF order_status_input_t




/*********************************************************************
 * 
 * delivery_input_t
 *
 * Input for any DELIVERY transaction
 *
 *********************************************************************/

class delivery_input_t : public trx_input_t {
public:

    short    _w_id;          /* input */
    short    _carrier_id;    /* input */


    // Construction/Destructions
    delivery_input_t() { 
        assert (false); // (ip) Not implemented yet
    };
    
    ~delivery_input_t() { };


    // Assignment operator
    delivery_input_t& operator= (const delivery_input_t& rhs) {
        assert (false); // (ip) Not implemented yet
        return (*this);
    }

    void describe(int id=0);
    void gen_input(int id);

}; // EOF delivery_input_t




/*********************************************************************
 * 
 * stock_level_input_t
 *
 * Input for any STOCK_LEVEL transaction
 *
 *********************************************************************/

class stock_level_input_t : public trx_input_t {
public:

    short    _d_id;          /* input */
    short    _w_id;          /* input */
    short    _threshold;     /* input */


    // Construction/Destructions
    stock_level_input_t() { 
        assert (false); // (ip) Not implemented yet
    };
    
    ~stock_level_input_t() { };


    // Assignment operator
    stock_level_input_t& operator= (const stock_level_input_t& rhs) {
        assert (false); // (ip) Not implemented yet
        return (*this);
    }

    void describe(int id=0);
    void gen_input(int id);

}; // EOF stock_level_input_t



EXIT_NAMESPACE(tpcc);


#endif

