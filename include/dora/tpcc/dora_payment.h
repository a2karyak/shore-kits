/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   dora_payment.h
 *
 *  @brief:  DORA TPC-C PAYMENT
 *
 *  @note:   Definition of RVPs and Actions that synthesize 
 *           the TPC-C Payment trx according to DORA
 *
 *  @author: Ippokratis Pandis, Oct 2008
 */


#ifndef __DORA_TPCC_PAYMENT_H
#define __DORA_TPCC_PAYMENT_H


#include "dora/tpcc/dora_tpcc.h"


ENTER_NAMESPACE(dora);


using namespace shore;
using namespace tpcc;


//
// RVPS
//
// (1) midway_pay_rvp
// (2) final_pay_rvp
//


/******************************************************************** 
 *
 * @class: midway_pay_rvp
 *
 * @brief: Submits the history packet, 
 *         Passes the warehouse and district tuples to the next phase
 *
 ********************************************************************/

class midway_pay_rvp : public rvp_t
{
private:
    // the data needed for communication
    tpcc_warehouse_tuple _awh;
    tpcc_district_tuple  _adist;
    // the list of actions to be remembered to be given back
    upd_wh_pay_action_impl*   _puw;
    upd_dist_pay_action_impl* _pud;
    upd_cust_pay_action_impl* _puc;
    // pointer to the payment input
    payment_input_t           _pin;
    // pointer to the shore env
    ShoreTPCCEnv* _ptpccenv;
public:

    midway_pay_rvp(tid_t atid, xct_t* axct,
                   trx_result_tuple_t &presult,
                   ShoreTPCCEnv* penv, 
                   payment_input_t ppin) 
        : rvp_t(atid, axct, presult, 3),  // consists of three packets
          _ptpccenv(penv), _pin(ppin)
    { }

    ~midway_pay_rvp() { }
    
    // access methods for the communicated data
    tpcc_warehouse_tuple* wh() { return (&_awh); }
    tpcc_district_tuple* dist() { return (&_adist); }    
    // access methods for the list of actions
    void set_puw(upd_wh_pay_action_impl*   puw) { _puw = puw; }
    void set_pud(upd_dist_pay_action_impl* pud) { _pud = pud; }
    void set_puc(upd_cust_pay_action_impl* puc) { _puc = puc; }
    // the interface
    w_rc_t run();
    void  cleanup();

}; // EOF: midway_pay_rvp



/******************************************************************** 
 *
 * @class: final_pay_rvp
 *
 * @brief: Terminal Phase of Payment
 *
 ********************************************************************/

class final_pay_rvp : public terminal_rvp_t
{
private:

    // the list of actions to be remembered to be given back
    ins_hist_pay_action_impl* _pih;
    // pointer to the shore env
    ShoreTPCCEnv* _ptpccenv;

public:

    final_pay_rvp(tid_t atid, xct_t* axct,
                  trx_result_tuple_t &presult,
                  ShoreTPCCEnv* penv) 
        : terminal_rvp_t(atid, axct, presult, 1), 
          _ptpccenv(penv) 
    { }
    
    ~final_pay_rvp() { }

    // access methods for the list of actions
    void set_pih(ins_hist_pay_action_impl* pih) { _pih = pih; }
    // required 
    void upd_committed_stats(); // update the committed trx stats
    void upd_aborted_stats(); // update the committed trx stats
    // the interface
    w_rc_t run() { assert (_ptpccenv); return (_run(_ptpccenv)); }
    void cleanup();

}; // EOF: final_pay_rvp


//
// ACTIONS
//
// (0) pay_action_impl - generic that holds a payment input
//
// (1) upd_wh_pay_action_impl
// (2) upd_dist_pay_action_impl
// (3) upd_cust_pay_action_impl
// (4) ins_hist_pay_action_impl
// 


/******************************************************************** 
 *
 * @abstract class: pay_action_impl
 *
 * @brief:          Holds a payment input and a pointer to ShoreTPCCEnv
 *
 ********************************************************************/

class pay_action_impl : public range_action_impl<int>
{
protected:

    ShoreTPCCEnv*   _ptpccenv;
    payment_input_t _pin;

public:
    
    pay_action_impl() : range_action_impl(), _ptpccenv(NULL) { }
    virtual ~pay_action_impl() { }
    virtual w_rc_t trx_exec()=0; // pure virtual
    virtual void calc_keys()=0; // pure virtual 
    virtual void set_input(tid_t atid,
                           xct_t* apxct,
                           rvp_t* prvp,
                           ShoreTPCCEnv* penv, 
                           const payment_input_t& pin) 
    {
        assert (apxct);
        assert (prvp);
        assert (penv);
        set(atid, apxct, prvp);
        _ptpccenv = penv;
        _pin = pin;
        set_key_range(); // set key range for this action
    }
    
}; // EOF: pay_action_impl


// UPD_WH_PAY_ACTION
class upd_wh_pay_action_impl : public pay_action_impl
{
public:    
    upd_wh_pay_action_impl() : pay_action_impl() { }
    ~upd_wh_pay_action_impl() { }
    w_rc_t trx_exec();    
    midway_pay_rvp* _m_rvp;
    void calc_keys() {
        _down.push_back(_pin._home_wh_id);
        _up.push_back(_pin._home_wh_id);
    }
}; // EOF: upd_wh_pay_action_impl

// UPD_DIST_PAY_ACTION
class upd_dist_pay_action_impl : public pay_action_impl
{
public:   
    upd_dist_pay_action_impl() : pay_action_impl() { }
    ~upd_dist_pay_action_impl() { }
    w_rc_t trx_exec();    
    midway_pay_rvp* _m_rvp;
    void calc_keys() {
        _down.push_back(_pin._home_wh_id);
        _down.push_back(_pin._home_d_id);
        _up.push_back(_pin._home_wh_id);
        _up.push_back(_pin._home_d_id);
    }
}; // EOF: upd_wh_pay_action_impl

// UPD_CUST_PAY_ACTION
class upd_cust_pay_action_impl : public pay_action_impl
{
public:    
    upd_cust_pay_action_impl() : pay_action_impl() { }
    ~upd_cust_pay_action_impl() { }
    w_rc_t trx_exec();   
    midway_pay_rvp* _m_rvp;
    void calc_keys() {
        _down.push_back(_pin._home_wh_id);
        _down.push_back(_pin._home_d_id);
        _down.push_back(_pin._c_id);
        _up.push_back(_pin._home_wh_id);
        _up.push_back(_pin._home_d_id);
        _up.push_back(_pin._c_id);
    }
}; // EOF: upd_cust_pay_action_impl

// INS_HIST_PAY_ACTION
class ins_hist_pay_action_impl : public pay_action_impl
{
public:    
    ins_hist_pay_action_impl() : pay_action_impl() { }
    ~ins_hist_pay_action_impl() { }
    w_rc_t trx_exec();    
    tpcc_warehouse_tuple _awh;
    tpcc_district_tuple _adist;    
    void calc_keys() {
        _down.push_back(_pin._home_wh_id);
        _up.push_back(_pin._home_wh_id);
    }
}; // EOF: ins_hist_pay_action_impl



EXIT_NAMESPACE(dora);

#endif /** __DORA_TPCC_PAYMENT_H */

