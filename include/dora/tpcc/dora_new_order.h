/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   dora_payment.h
 *
 *  @brief:  DORA TPC-C NEW_ORDER
 *
 *  @note:   Definition of RVPs and Actions that synthesize 
 *           the TPC-C NewOrder trx according to DORA
 *
 *  @author: Ippokratis Pandis, Nov 2008
 */


#ifndef __DORA_TPCC_NEW_ORDER_H
#define __DORA_TPCC_NEW_ORDER_H


#include "dora/tpcc/dora_tpcc.h"
#include "workload/tpcc/shore_tpcc_env.h"


ENTER_NAMESPACE(dora);


using namespace shore;
using namespace tpcc;


//
// RVPS
//
// (1) final_nord_rvp
// (2) midway_nord_rvp
//




/******************************************************************** 
 *
 * @class: final_nord_rvp
 *
 * @brief: Terminal Phase of NewOrder
 *
 ********************************************************************/

class final_nord_rvp : public terminal_rvp_t
{
private:
    typedef object_cache_t<final_nord_rvp> rvp_cache;
    DoraTPCCEnv* _ptpccenv;
    rvp_cache* _cache;
public:
    final_nord_rvp() : terminal_rvp_t(), _cache(NULL), _ptpccenv(NULL) { }
    ~final_nord_rvp() { _cache=NULL; _ptpccenv=NULL; }

    // access methods
    inline void set(const tid_t& atid, xct_t* axct, const int axctid,
                    trx_result_tuple_t& presult, int ol_cnt,
                    DoraTPCCEnv* penv, rvp_cache* pc) 
    { 
        assert (ol_cnt>0);
        w_assert3 (penv);
        _ptpccenv = penv;
        w_assert3 (pc);
        _cache = pc;
        // [OL_CNT+4] intratrx - [3*OL_CNT+5] total actions        
        _set(atid,axct,axctid,presult,ol_cnt+4,3*ol_cnt+5);
    }
    inline void giveback() { 
        w_assert3 (_ptpccenv); 
        _cache->giveback(this); }    

    // interface
    w_rc_t run();
    void upd_committed_stats(); // update the committed trx stats
    void upd_aborted_stats(); // update the committed trx stats

}; // EOF: final_nord_rvp



/******************************************************************** 
 *
 * @class: midway_nord_rvp
 *
 * @brief: The midway RVP, submits the following actions:
 *         - 1 x Insert (ORD)
 *         - 1 x Insert (NORD)
 *         - _OL_CNT x Insert (OL)
 *
 * @note:  All those actions need the D_NEXT_O_ID, as well as,
 *         for each OL it needs also the _item_amount and astock
 *        
 ********************************************************************/

class midway_nord_rvp : public rvp_t
{
private:
    typedef object_cache_t<midway_nord_rvp> rvp_cache;
    rvp_cache* _cache;
    DoraTPCCEnv* _ptpccenv;
public:
    // data needed for the next phase
    new_order_input_t _noin;
    final_nord_rvp* _final_rvp;

    midway_nord_rvp() : rvp_t(), _cache(NULL), _ptpccenv(NULL) { }
    ~midway_nord_rvp() { _cache=NULL; _ptpccenv=NULL; }

    // access methods
    inline void set(const tid_t& atid, xct_t* axct, const int& axctid,
                    const new_order_input_t& noin, 
                    final_nord_rvp* prvp,
                    DoraTPCCEnv* penv, rvp_cache* pc) 
    { 
        _noin = noin;
        w_assert3 (prvp);
        _final_rvp = prvp;
        _ptpccenv = penv;
        w_assert3 (pc);
        _cache = pc;
        // [2*OL_CNT+1] intratrx - [2*OL_CNT+1] total actions        
        _set(atid,axct,axctid,prvp->result(),2*noin._ol_cnt+1,2*noin._ol_cnt+1); 
    }
    inline void giveback() { w_assert3 (_cache); 
        _cache->giveback(this); }    
   
    // the interface
    w_rc_t run();

}; // EOF: midway_nord_rvp



//
// ACTIONS
//
// (0) nord_action
//
// (1) r_wh_nord_action       -- start -> final
// (2) r_cust_nord_action
//
// (3) upd_dist_nord_action   -- start -> midway
// (4) r_item_nord_action
// (5) upd_sto_nord_action
//
// (6) ins_ord_nord_action    -- midway -> final
// (7) ins_nord_nord_action
// (8) ins_ol_nord_action
// 


/******************************************************************** 
 *
 * @abstract class: nord_action
 *
 * @brief:          Basic nord action
 *
 ********************************************************************/

class nord_action : public range_action_impl<int>
{
protected:
    DoraTPCCEnv*   _ptpccenv;
    int _wh_id;
    int _d_id;

    inline void _nord_act_set(const tid_t& atid, xct_t* axct, rvp_t* prvp, 
                              const int keylen,
                              const int whid, const int did,
                              DoraTPCCEnv* penv) 
    {
        _range_act_set(atid,axct,prvp,keylen); 
        _wh_id = whid;
        _d_id = did;
        w_assert3 (penv);
        _ptpccenv = penv;
        trx_upd_keys(); // set the keys
    }

public:    
    nord_action() : range_action_impl<int>(), _ptpccenv(NULL) { }
    virtual ~nord_action() { }

    virtual w_rc_t trx_exec()=0;    
    virtual void calc_keys()=0; 
    
}; // EOF: nord_action




/******************************************************************** 
 *
 * - Start -> Final
 *
 * (1) R_WH_NORD_ACTION
 * (2) R_CUST_NORD_ACTION
 *
 ********************************************************************/

// R_WH_NORD_ACTION
class r_wh_nord_action : public nord_action
{
private:
    typedef object_cache_t<r_wh_nord_action> act_cache;
    act_cache*       _cache;
public:    
    r_wh_nord_action() : nord_action() { }
    ~r_wh_nord_action() { }
    w_rc_t trx_exec();    
    void calc_keys() {
        _down.push_back(_wh_id);
        _up.push_back(_wh_id);
    }
    inline void set(const tid_t& atid, xct_t* axct, rvp_t* prvp,
                    const int whid, const int did,
                    DoraTPCCEnv* penv, act_cache* pc) 
    {
        w_assert3 (pc);
        _cache = pc;
        _nord_act_set(atid,axct,prvp,1,whid,did,penv);  // key is (WH)
    }
    inline void giveback() { 
        w_assert3 (_cache); 
        _cache->giveback(this); }    
   
}; // EOF: r_wh_nord_action


// R_CUST_NORD_ACTION
class r_cust_nord_action : public nord_action
{
private:
    typedef object_cache_t<r_cust_nord_action> act_cache;
    act_cache*       _cache;
    int _c_id;
public:    
    r_cust_nord_action() : nord_action() { }
    ~r_cust_nord_action() { }
    w_rc_t trx_exec();    
    void calc_keys() {
        _down.push_back(_wh_id);
        _down.push_back(_d_id);
        _down.push_back(_c_id);
        _up.push_back(_wh_id);
        _up.push_back(_d_id);
        _up.push_back(_c_id);
    }
    inline void set(const tid_t& atid, xct_t* axct, rvp_t* prvp,
                    const int whid, const int did, const int cid,
                    DoraTPCCEnv* penv, act_cache* pc) 
    {
        _c_id = cid;
        w_assert3 (pc);
        _cache = pc;
        _nord_act_set(atid,axct,prvp,3,whid,did,penv);  // key is (WH|D|C)
    }
    inline void giveback() { 
        w_assert3 (_cache); 
        _cache->giveback(this); }    
   
}; // EOF: r_cust_nord_action





/******************************************************************** 
 *
 * - Start -> Midway
 *
 * (3) UPD_DIST_NORD_ACTION
 * (4) R_ITEM_NORD_ACTION
 * (5) UPD_ITEM_NORD_ACTION
 *
 ********************************************************************/


// UPD_DIST_NORD_ACTION
class upd_dist_nord_action : public nord_action
{
private:
    typedef object_cache_t<upd_dist_nord_action> act_cache;
    act_cache*       _cache;
    midway_nord_rvp* _pmidway_rvp;
public:    
    upd_dist_nord_action() : nord_action() { }
    ~upd_dist_nord_action() { }
    w_rc_t trx_exec();    
    void calc_keys() {
        _down.push_back(_wh_id);
        _down.push_back(_d_id);
        _up.push_back(_wh_id);
        _up.push_back(_d_id);
    }
    inline void set(const tid_t& atid, xct_t* axct, 
                    midway_nord_rvp* pmidway_rvp, 
                    const int whid, const int did,
                    DoraTPCCEnv* penv, act_cache* pc) 
    {
        w_assert3 (pmidway_rvp);
        _pmidway_rvp = pmidway_rvp;
        w_assert3 (pc);
        _cache = pc;
        _nord_act_set(atid,axct,pmidway_rvp,2,whid,did,penv);  // key is (WH|D)
    }
    inline void giveback() { 
        w_assert3 (_cache); 
        _cache->giveback(this); }    
   
}; // EOF: upd_dist_nord_action


// R_ITEM_NORD_ACTION
class r_item_nord_action : public nord_action
{
private:
    typedef object_cache_t<r_item_nord_action> act_cache;
    act_cache*       _cache;
    midway_nord_rvp* _pmidway_rvp;
    int _ol_idx;
public:    
    r_item_nord_action() : nord_action(), _ol_idx(-1) { }
    ~r_item_nord_action() { }
    w_rc_t trx_exec();    
    void calc_keys() {
        _down.push_back(_pmidway_rvp->_noin.items[_ol_idx]._ol_i_id);
        _up.push_back(_pmidway_rvp->_noin.items[_ol_idx]._ol_i_id);
    }
    inline void set(const tid_t& atid, xct_t* axct, 
                    midway_nord_rvp* pmidway_rvp, 
                    const int whid, const int did, const int olidx,
                    DoraTPCCEnv* penv, act_cache* pc) 
    {
        _ol_idx = olidx;
        w_assert3 (pmidway_rvp);
        _pmidway_rvp = pmidway_rvp;
        w_assert3 (pc);
        _cache = pc;
        _nord_act_set(atid,axct,pmidway_rvp,1,whid,did,penv);  // key is (I)
    }
    inline void giveback() { 
        w_assert3 (_cache); 
        _cache->giveback(this); }    
   
}; // EOF: r_item_nord_action


// UPD_STO_NORD_ACTION
class upd_sto_nord_action : public nord_action
{
private:
    typedef object_cache_t<upd_sto_nord_action> act_cache;
    act_cache*       _cache;
    midway_nord_rvp* _pmidway_rvp;
    int _ol_idx;
public:    
    upd_sto_nord_action() : nord_action(), _ol_idx(-1) { }
    ~upd_sto_nord_action() { }
    w_rc_t trx_exec();    
    void calc_keys() {
        _down.push_back(_pmidway_rvp->_noin.items[_ol_idx]._ol_supply_wh_id);
        _down.push_back(_pmidway_rvp->_noin.items[_ol_idx]._ol_i_id);
        _up.push_back(_pmidway_rvp->_noin.items[_ol_idx]._ol_supply_wh_id);
        _up.push_back(_pmidway_rvp->_noin.items[_ol_idx]._ol_i_id);
    }
    inline void set(const tid_t& atid, xct_t* axct, 
                    midway_nord_rvp* pmidway_rvp, 
                    const int whid, const int did, const int olidx,
                    DoraTPCCEnv* penv, act_cache* pc) 
    {
        _ol_idx = olidx;
        w_assert3 (pmidway_rvp);
        _pmidway_rvp = pmidway_rvp;
        w_assert3 (pc);
        _cache = pc;
        _nord_act_set(atid,axct,pmidway_rvp,2,whid,did,penv);  // key is (WH|I)
    }
    inline void giveback() { 
        w_assert3 (_cache); 
        _cache->giveback(this); }    
   
}; // EOF: upd_sto_nord_action





/******************************************************************** 
 *
 * - Midway -> Final
 *
 * (6) INS_ORD_NORD_ACTION
 * (7) INS_NORD_NORD_ACTION
 * (8) INS_OL_NORD_ACTION
 *
 ********************************************************************/


// INS_ORD_NORD_ACTION
class ins_ord_nord_action : public nord_action
{
private:
    typedef object_cache_t<ins_ord_nord_action> act_cache;
    act_cache*       _cache;

    int    _d_next_o_id;
    int    _c_id;
    time_t _tstamp;
    int    _ol_cnt;
    int    _all_local;
public:    
    ins_ord_nord_action() : nord_action(), 
                            _d_next_o_id(-1), _c_id(-1), 
                            _ol_cnt(-1), _all_local(-1) 
    { }
    ~ins_ord_nord_action() { }
    w_rc_t trx_exec();    
    void calc_keys() {
        _down.push_back(_wh_id);
        _down.push_back(_d_id);
        _down.push_back(_d_next_o_id);
        _up.push_back(_wh_id);
        _up.push_back(_d_id);
        _up.push_back(_d_next_o_id);
    }
    inline void set(const tid_t& atid, xct_t* axct, 
                    rvp_t* prvp, 
                    const int whid, const int did, const int nextoid,
                    const int cid, const time_t tstamp, int olcnt, int alllocal,
                    DoraTPCCEnv* penv, act_cache* pc) 
    {
        _d_next_o_id = nextoid;
        _c_id = cid;
        _tstamp = tstamp;
        _ol_cnt = olcnt;
        _all_local = alllocal;
        w_assert3 (pc);
        _cache = pc;
        _nord_act_set(atid,axct,prvp,3,whid,did,penv);  // key is (WH|D|OID)
    }
    inline void giveback() { 
        w_assert3 (_cache); 
        _cache->giveback(this); }    
   
}; // EOF: ins_ord_nord_action


// INS_NORD_NORD_ACTION
class ins_nord_nord_action : public nord_action
{
private:
    typedef object_cache_t<ins_nord_nord_action> act_cache;
    act_cache*       _cache;

    int    _d_next_o_id;
public:    
    ins_nord_nord_action() : nord_action(), _d_next_o_id(-1) { }
    ~ins_nord_nord_action() { }
    w_rc_t trx_exec();    
    void calc_keys() {
        _down.push_back(_wh_id);
        _down.push_back(_d_id);
        _down.push_back(_d_next_o_id);
        _up.push_back(_wh_id);
        _up.push_back(_d_id);
        _up.push_back(_d_next_o_id);
    }
    inline void set(const tid_t& atid, xct_t* axct, 
                    rvp_t* prvp, 
                    const int whid, const int did, const int nextoid,
                    DoraTPCCEnv* penv, act_cache* pc) 
    {
        _d_next_o_id = nextoid;
        w_assert3 (pc);
        _cache = pc;
        _nord_act_set(atid,axct,prvp,3,whid,did,penv);  // key is (WH|D|OID)
    }
    inline void giveback() { 
        w_assert3 (_cache); 
        _cache->giveback(this); }    
   
}; // EOF: ins_nord_nord_action



// INS_OL_NORD_ACTION
class ins_ol_nord_action : public nord_action
{
private:
    typedef object_cache_t<ins_ol_nord_action> act_cache;
    act_cache*       _cache;

    int          _ol_idx;
    int          _d_next_o_id;
    ol_item_info _item_info;
    time_t       _tstamp;
public:    
    ins_ol_nord_action() : nord_action(), _ol_idx(-1), _d_next_o_id(-1) { }
    ~ins_ol_nord_action() { }
    w_rc_t trx_exec();    
    void calc_keys() {
        _down.push_back(_wh_id);
        _down.push_back(_d_id);
        _down.push_back(_d_next_o_id);
        _down.push_back(_ol_idx);
        _up.push_back(_wh_id);
        _up.push_back(_d_id);
        _up.push_back(_d_next_o_id);
        _up.push_back(_ol_idx);
    }
    inline void set(const tid_t& atid, xct_t* axct, 
                    rvp_t* prvp, 
                    const int whid, const int did, const int nextoid,
                    const int olidx, 
                    const ol_item_info& iteminfo, time_t tstamp,
                    DoraTPCCEnv* penv, act_cache* pc) 
    {
        _ol_idx = olidx;
        _d_next_o_id = nextoid;
        _item_info = iteminfo;
        _tstamp = tstamp;
        w_assert3 (pc);
        _cache = pc;
        _nord_act_set(atid,axct,prvp,4,whid,did,penv);  // key is (WH|D|OID|OLIDX)
    }
    inline void giveback() { 
        w_assert3 (_cache); 
        _cache->giveback(this); }    
   
}; // EOF: ins_ol_nord_action




EXIT_NAMESPACE(dora);

#endif /** __DORA_TPCC_NEW_ORDER_H */

