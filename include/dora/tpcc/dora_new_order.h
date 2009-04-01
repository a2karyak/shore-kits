/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   dora_new_order.h
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
                    trx_result_tuple_t& presult, 
                    DoraTPCCEnv* penv, rvp_cache* pc) 
    { 
        assert (penv);
        _ptpccenv = penv;
        assert (pc);
        _cache = pc;

        // NewOrder does not have 1 intratrx and 1 total actions but we set them at postset()
        _set(atid,axct,axctid,presult,1,1);
    }
    inline void postset(int ol_cnt)
    {
        // [OL_CNT+4] intratrx - [3*OL_CNT+5] total actions   
        assert (ol_cnt);
        _countdown.reset(ol_cnt+4);
        _actions.reserve(3*ol_cnt+5);
    }

    inline void giveback() { 
        assert (_ptpccenv); 
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
    bool _bWake;
public:
    // data needed for the next phase
    new_order_input_t _noin;
    final_nord_rvp* _final_rvp;

    midway_nord_rvp() : rvp_t(), _cache(NULL), _ptpccenv(NULL) { }
    ~midway_nord_rvp() { _cache=NULL; _ptpccenv=NULL; }

    // access methods
    inline void set(const tid_t& atid, xct_t* axct, const int& axctid,
                    trx_result_tuple_t& presult, 
                    const new_order_input_t& noin, const bool bWake,
                    DoraTPCCEnv* penv, rvp_cache* pc) 
    { 
        _noin = noin;
        _bWake = bWake;
        _ptpccenv = penv;
        assert (pc);
        _cache = pc;
        // [2*OL_CNT+1] intratrx - [2*OL_CNT+1] total actions        
        _set(atid,axct,axctid,presult,2*noin._ol_cnt+1,2*noin._ol_cnt+1); 
    }
    inline void postset(final_nord_rvp* prvp)
    {
        assert (prvp);
        _final_rvp = prvp;        
    }
    inline void giveback() { assert (_cache); 
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
        assert (penv);
        _ptpccenv = penv;
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
    void calc_keys();
    inline void set(const tid_t& atid, xct_t* axct, rvp_t* prvp,
                    const int whid, 
                    DoraTPCCEnv* penv, act_cache* pc) 
    {
        assert (pc);
        _cache = pc;
        _nord_act_set(atid,axct,prvp,1,whid,0,penv);  // key is (WH)
    }
    inline void postset(const int did)
    {
        _d_id = did;
    }
    inline void giveback() { 
        assert (_cache); 
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
    void calc_keys();
    inline void set(const tid_t& atid, xct_t* axct, rvp_t* prvp,
                    const int whid, 
                    DoraTPCCEnv* penv, act_cache* pc) 
    {
        assert (pc);
        _cache = pc;
        _nord_act_set(atid,axct,prvp,3,whid,0,penv);  // key is (WH|D|C)
    }
    inline void postset(const int did, const int cid)
    {
        _d_id = did;
        _c_id = cid;
    }
    inline void giveback() { 
        assert (_cache); 
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
    void calc_keys();
    inline void set(const tid_t& atid, xct_t* axct, 
                    midway_nord_rvp* pmidway_rvp, 
                    const int whid, 
                    DoraTPCCEnv* penv, act_cache* pc) 
    {
        assert (pmidway_rvp);
        _pmidway_rvp = pmidway_rvp;
        assert (pc);
        _cache = pc;
        _nord_act_set(atid,axct,pmidway_rvp,2,whid,0,penv);  // key is (WH|D)
    }
    inline void postset(const int did)
    {
        _d_id = did;
    }
    inline void giveback() { 
        assert (_cache); 
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
    void calc_keys();
    inline void set(const tid_t& atid, xct_t* axct, 
                    midway_nord_rvp* pmidway_rvp, 
                    const int whid, 
                    DoraTPCCEnv* penv, act_cache* pc) 
    {
        assert (pmidway_rvp);
        _pmidway_rvp = pmidway_rvp;
        assert (pc);
        _cache = pc;
        _nord_act_set(atid,axct,pmidway_rvp,1,whid,0,penv);  // key is (I)
    }
    inline void postset(const int did, const int olidx)
    {
        _d_id = did;
        _ol_idx = olidx;
    }
    inline void giveback() { 
        assert (_cache); 
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
    void calc_keys();
    inline void set(const tid_t& atid, xct_t* axct, 
                    midway_nord_rvp* pmidway_rvp, 
                    const int whid,
                    DoraTPCCEnv* penv, act_cache* pc) 
    {
        assert (pmidway_rvp);
        _pmidway_rvp = pmidway_rvp;
        assert (pc);
        _cache = pc;
        _nord_act_set(atid,axct,pmidway_rvp,2,whid,0,penv);  // key is (WH|I)
    }
    inline void postset(const int did, const int olidx)
    {
        _d_id = did;
        _ol_idx = olidx;
    }
    inline void giveback() { 
        assert (_cache); 
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
    void calc_keys();
    inline void set(const tid_t& atid, xct_t* axct, 
                    rvp_t* prvp, 
                    const int whid, 
                    DoraTPCCEnv* penv, act_cache* pc) 
    {
        assert (pc);
        _cache = pc;
        _nord_act_set(atid,axct,prvp,3,whid,0,penv);  // key is (WH|D|OID)
    }
    inline void postset(const int did, const int nextoid,
                        const int cid, const time_t tstamp, 
                        const int olcnt, const int alllocal)
    {
        _d_id = did;

        _d_next_o_id = nextoid;
        _c_id = cid;
        _tstamp = tstamp;
        _ol_cnt = olcnt;
        _all_local = alllocal;
    }
    inline void giveback() { 
        assert (_cache); 
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
    void calc_keys();
    inline void set(const tid_t& atid, xct_t* axct, 
                    rvp_t* prvp, const int whid, 
                    DoraTPCCEnv* penv, act_cache* pc) 
    {
        assert (pc);
        _cache = pc;
        _nord_act_set(atid,axct,prvp,3,whid,0,penv);  // key is (WH|D|OID)
    }
    inline void postset(const int did, const int nextoid)
    {
        _d_id = did;
        
        _d_next_o_id = nextoid;
    }
    inline void giveback() { 
        assert (_cache); 
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
    void calc_keys();
    inline void set(const tid_t& atid, xct_t* axct, 
                    rvp_t* prvp, const int whid,
                    DoraTPCCEnv* penv, act_cache* pc) 
    {
        assert (pc);
        _cache = pc;
        _nord_act_set(atid,axct,prvp,4,whid,0,penv);  // key is (WH|D|OID|OLIDX)
    }
    inline void postset(const int did, const int nextoid,
                        const int olidx, const ol_item_info& iteminfo, 
                        const time_t tstamp)
    {
        _d_id = did;

        _ol_idx = olidx;
        _d_next_o_id = nextoid;
        _item_info = iteminfo;
        _tstamp = tstamp;
    }
    inline void giveback() { 
        assert (_cache); 
        _cache->giveback(this); }    
   
}; // EOF: ins_ol_nord_action




EXIT_NAMESPACE(dora);

#endif /** __DORA_TPCC_NEW_ORDER_H */

