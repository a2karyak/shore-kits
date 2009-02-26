/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   dora_order_status.h
 *
 *  @brief:  DORA TPC-C ORDER STATUS
 *
 *  @note:   Definition of RVPs and Actions that synthesize 
 *           the TPC-C OrderStatus trx according to DORA
 *
 *  @author: Ippokratis Pandis, Jan 2009
 */


#ifndef __DORA_TPCC_ORDER_STATUS_H
#define __DORA_TPCC_ORDER_STATUS_H


#include "dora.h"
#include "workload/tpcc/shore_tpcc_env.h"
#include "dora/tpcc/dora_tpcc.h"


ENTER_NAMESPACE(dora);


using namespace shore;
using namespace tpcc;


//
// RVPS
//
// (1) final_ordst_rvp
//


/******************************************************************** 
 *
 * @class: final_ordst_rvp
 *
 * @brief: Terminal Phase of OrderStatus
 *
 ********************************************************************/

class final_ordst_rvp : public terminal_rvp_t
{
private:
    typedef object_cache_t<final_ordst_rvp> rvp_cache;
    DoraTPCCEnv* _ptpccenv;
    rvp_cache* _cache;
public:
    final_ordst_rvp() : terminal_rvp_t(), _cache(NULL), _ptpccenv(NULL) { }
    ~final_ordst_rvp() { _cache=NULL; _ptpccenv=NULL; }

    // access methods
    inline void set(const tid_t& atid, xct_t* axct, const int axctid,
                    trx_result_tuple_t& presult, 
                    DoraTPCCEnv* penv, rvp_cache* pc) 
    { 
        assert (penv);
        _ptpccenv = penv;
        assert (pc);
        _cache = pc;
        _set(atid,axct,axctid,presult,2,2); // 2 intratrx - 2 total actions
    }
    inline void giveback() { 
        assert (_ptpccenv); 
        _cache->giveback(this); }    

    // interface
    w_rc_t run();
    void upd_committed_stats(); // update the committed trx stats
    void upd_aborted_stats(); // update the committed trx stats

}; // EOF: final_ordst_rvp


//
// ACTIONS
//
// (0) ordst_action
// (1) r_cust_ordst_action
// (2) r_ol_ordst_action
// 


/******************************************************************** 
 *
 * @abstract class: ordst_action
 *
 * @brief:          Holds a order status input and a pointer to ShoreTPCCEnv
 *
 ********************************************************************/

class ordst_action : public range_action_impl<int>
{
protected:
    DoraTPCCEnv*   _ptpccenv;
    order_status_input_t _ordstin;

    inline void _ordst_act_set(const tid_t& atid, xct_t* axct, rvp_t* prvp, 
                               const int keylen, const order_status_input_t& ordstin, 
                               DoraTPCCEnv* penv) 
    {
        _range_act_set(atid,axct,prvp,keylen); 
        _ordstin = ordstin;
        assert (penv);
        _ptpccenv = penv;
    }

public:    
    ordst_action() : range_action_impl<int>(), _ptpccenv(NULL) { }
    virtual ~ordst_action() { }

    virtual w_rc_t trx_exec()=0;    
    virtual void calc_keys()=0; 
    
}; // EOF: ordst_action


// R_CUST_ORDST_ACTION
class r_cust_ordst_action : public ordst_action
{
private:
    typedef object_cache_t<r_cust_ordst_action> act_cache;
    act_cache*       _cache;
public:    
    r_cust_ordst_action() : ordst_action() { }
    ~r_cust_ordst_action() { }
    w_rc_t trx_exec();
    void calc_keys() {
        _down.push_back(_ordstin._wh_id);
        _down.push_back(_ordstin._d_id);
        _down.push_back(_ordstin._c_id);
        _up.push_back(_ordstin._wh_id);
        _up.push_back(_ordstin._d_id);
        _up.push_back(_ordstin._c_id);
    }
    inline void set(const tid_t& atid, xct_t* axct, rvp_t* prvp, 
                    const order_status_input_t& pordst,
                    DoraTPCCEnv* penv, act_cache* pc) 
    {
        assert (pc);
        _cache = pc;
        _ordst_act_set(atid,axct,prvp,3,pordst,penv);  // key is (WH|DIST|CUST)
    }
    inline void giveback() { 
        assert (_cache); 
        _cache->giveback(this); }    
   
}; // EOF: upd_wh_ordst_action


// R_OL_ORDST_ACTION
class r_ol_ordst_action : public ordst_action
{
private:
    typedef object_cache_t<r_ol_ordst_action> act_cache;
    act_cache*       _cache;
public:   
    r_ol_ordst_action() : ordst_action() { }
    ~r_ol_ordst_action() { }
    tpcc_order_tuple _aorder;
    w_rc_t trx_exec();    
    void calc_keys() {
        _down.push_back(_ordstin._wh_id);
        _down.push_back(_ordstin._d_id);
        _down.push_back(_aorder.O_ID);
        _up.push_back(_ordstin._wh_id);
        _up.push_back(_ordstin._d_id);
        _up.push_back(_aorder.O_ID);
    }
    inline void set(const tid_t& atid, xct_t* axct, rvp_t* prvp, 
                    const order_status_input_t& pordst,
                    DoraTPCCEnv* penv, act_cache* pc) 
    {
        assert (pc);
        _cache = pc;
        _ordst_act_set(atid,axct,prvp,3,pordst,penv);  // key is (WH|DIST|O_ID)
    }
    inline void postset(const tpcc_order_tuple& aord)
    {
        _aorder = aord;
    }
    inline void giveback() { 
        assert (_cache); 
        _cache->giveback(this); }    

}; // EOF: r_ol_ordst_action



EXIT_NAMESPACE(dora);

#endif /** __DORA_TPCC_ORDER_STATUS_H */

