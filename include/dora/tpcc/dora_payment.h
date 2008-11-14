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


#include "dora.h"
#include "workload/tpcc/shore_tpcc_env.h"


ENTER_NAMESPACE(dora);


using namespace shore;
using namespace tpcc;

class DoraTPCCEnv;


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
    typedef atomic_class_stack<midway_pay_rvp> rvp_pool;
    rvp_pool* _pool;
    DoraTPCCEnv* _ptpccenv;
    // data needed for the next phase
    tpcc_warehouse_tuple _awh;
    tpcc_district_tuple  _adist;
    payment_input_t      _pin;
public:
    midway_pay_rvp() : rvp_t(), _pool(NULL), _ptpccenv(NULL) { }
    ~midway_pay_rvp() { _pool=NULL; _ptpccenv=NULL; }

    // access methods
    inline void set(tid_t atid, xct_t* axct, const int& axctid,
                    trx_result_tuple_t& presult, 
                    const payment_input_t& ppin, 
                    DoraTPCCEnv* penv, rvp_pool* pp) 
    { 
        _set(atid,axct,axctid,presult,3,3);
        _pin = ppin;
        assert (penv);
        _ptpccenv = penv;
        assert (pp);
        _pool = pp;
    }
    inline void giveback() { assert (_pool); _pool->destroy(this); }    

    tpcc_warehouse_tuple* wh() { return (&_awh); }
    tpcc_district_tuple* dist() { return (&_adist); }    

    // the interface
    w_rc_t run();

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
    typedef atomic_class_stack<final_pay_rvp> rvp_pool;
    rvp_pool* _pool;
    DoraTPCCEnv* _ptpccenv;
public:
    final_pay_rvp() : terminal_rvp_t(), _pool(NULL), _ptpccenv(NULL) { }
    ~final_pay_rvp() { _pool=NULL; _ptpccenv=NULL; }

    // access methods
    inline void set(tid_t atid, xct_t* axct, const int axctid,
                    trx_result_tuple_t& presult, 
                    DoraTPCCEnv* penv, rvp_pool* pp) 
    { 
        _set(atid,axct,axctid,presult,1,4);
        assert (penv);
        _ptpccenv = penv;
        assert (pp);
        _pool = pp;
    }
    inline void giveback() { assert (_pool); _pool->destroy(this); }    

    // interface
    w_rc_t run();
    void upd_committed_stats(); // update the committed trx stats
    void upd_aborted_stats(); // update the committed trx stats

}; // EOF: final_pay_rvp


//
// ACTIONS
//
// (0) pay_action - generic that holds a payment input
//
// (1) upd_wh_pay_action
// (2) upd_dist_pay_action
// (3) upd_cust_pay_action
// (4) ins_hist_pay_action
// 


/******************************************************************** 
 *
 * @abstract class: pay_action
 *
 * @brief:          Holds a payment input and a pointer to ShoreTPCCEnv
 *                  Also implements the trx_acq_locks since all the
 *                  Payment actions are probes to a single tuple
 *
 ********************************************************************/

class pay_action : public range_action_impl<int>
{
protected:
    DoraTPCCEnv*   _ptpccenv;
    payment_input_t _pin;

    inline void _pay_act_set(tid_t atid, xct_t* axct, rvp_t* prvp, 
                             const int keylen, const payment_input_t& pin, 
                             DoraTPCCEnv* penv) 
    {
        _range_act_set(atid,axct,prvp,keylen); 
        _pin = pin;
        assert (penv);
        _ptpccenv = penv;
        set_key_range(); // set key range for this action
    }

public:
    
    pay_action() 
        : range_action_impl<int>(), _ptpccenv(NULL) 
    { }
    virtual ~pay_action() { }

    virtual w_rc_t trx_exec()=0;    
    virtual void calc_keys()=0; 

    const bool trx_acq_locks(); // all payment actions are unique probes
    
}; // EOF: pay_action


// UPD_WH_PAY_ACTION
class upd_wh_pay_action : public pay_action
{
private:
    typedef atomic_class_stack<upd_wh_pay_action> act_pool;
    act_pool*       _pool;
public:    
    upd_wh_pay_action() : pay_action() { }
    ~upd_wh_pay_action() { }
    w_rc_t trx_exec();    
    midway_pay_rvp* _m_rvp;
    void calc_keys() {
        _down.push_back(_pin._home_wh_id);
        _up.push_back(_pin._home_wh_id);
    }
    inline void set(tid_t atid, xct_t* axct, rvp_t* prvp, 
                    const payment_input_t& pin,
                    DoraTPCCEnv* penv, 
                    midway_pay_rvp* amrvp, act_pool* pp) 
    {
        _pay_act_set(atid,axct,prvp,1,pin,penv);  // key is (WH)
        assert (amrvp);
        _m_rvp = amrvp;
        assert (pp);
        _pool = pp;
    }

    inline void giveback() { assert (_pool); _pool->destroy(this); }    
}; // EOF: upd_wh_pay_action

// UPD_DIST_PAY_ACTION
class upd_dist_pay_action : public pay_action
{
private:
    typedef atomic_class_stack<upd_dist_pay_action> act_pool;
    act_pool*       _pool;
public:   
    upd_dist_pay_action() : pay_action() { }
    ~upd_dist_pay_action() { }
    w_rc_t trx_exec();    
    midway_pay_rvp* _m_rvp;
    void calc_keys() {
        _down.push_back(_pin._home_wh_id);
        _down.push_back(_pin._home_d_id);
        _up.push_back(_pin._home_wh_id);
        _up.push_back(_pin._home_d_id);
    }
    inline void set(tid_t atid, xct_t* axct, rvp_t* prvp, 
                    const payment_input_t& pin,
                    DoraTPCCEnv* penv, 
                    midway_pay_rvp* amrvp, act_pool* pp) 
    {
        _pay_act_set(atid,axct,prvp,2,pin,penv);  // key is (WH|DIST)
        assert (amrvp);
        _m_rvp = amrvp;
        assert (pp);
        _pool = pp;
    }

    inline void giveback() { assert (_pool); _pool->destroy(this); }    
}; // EOF: upd_wh_pay_action

// UPD_CUST_PAY_ACTION
class upd_cust_pay_action : public pay_action
{
private:
    typedef atomic_class_stack<upd_cust_pay_action> act_pool;
    act_pool*       _pool;
public:    
    upd_cust_pay_action() : pay_action() { }
    ~upd_cust_pay_action() { }
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
    inline void set(tid_t atid, xct_t* axct, rvp_t* prvp, 
                    const payment_input_t& pin,
                    DoraTPCCEnv* penv, 
                    midway_pay_rvp* amrvp, act_pool* pp) 
    {
        _pay_act_set(atid,axct,prvp,3,pin,penv);  // key is (WH|DIST|CUST)
        assert (amrvp);
        _m_rvp = amrvp;
        assert (pp);
        _pool = pp;
    }

    inline void giveback() { assert (_pool); _pool->destroy(this); }    
}; // EOF: upd_cust_pay_action

// INS_HIST_PAY_ACTION
class ins_hist_pay_action : public pay_action
{
private:
    typedef atomic_class_stack<ins_hist_pay_action> act_pool;
    act_pool*       _pool;
public:    
    ins_hist_pay_action() : pay_action() { }
    ~ins_hist_pay_action() { }
    w_rc_t trx_exec();    
    tpcc_warehouse_tuple _awh;
    tpcc_district_tuple _adist;    
    void calc_keys() {
        _down.push_back(_pin._home_wh_id);
        _up.push_back(_pin._home_wh_id);
    }
    inline void set(tid_t atid, xct_t* axct, rvp_t* prvp, 
                    const payment_input_t& pin,
                    tpcc_warehouse_tuple awh,
                    tpcc_district_tuple adist,
                    DoraTPCCEnv* penv, act_pool* pp) 
    {
        _pay_act_set(atid,axct,prvp,1,pin,penv);  // key is (HIST)
        _awh = awh;
        _adist = adist;
        assert (pp);
        _pool = pp;
    }

    inline void giveback() { assert (_pool); _pool->destroy(this); }    
}; // EOF: ins_hist_pay_action



EXIT_NAMESPACE(dora);

#endif /** __DORA_TPCC_PAYMENT_H */

