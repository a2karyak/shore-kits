/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   dora_order_status.h
 *
 *  @brief:  DORA TPC-C STOCK LEVEL
 *
 *  @note:   Definition of RVPs and Actions that synthesize 
 *           the TPC-C StockLevel trx according to DORA
 *
 *  @author: Ippokratis Pandis, Jan 2009
 */


#ifndef __DORA_TPCC_STOCK_LEVEL_H
#define __DORA_TPCC_STOCK_LEVEL_H


#include "dora.h"
#include "workload/tpcc/shore_tpcc_env.h"
#include "dora/tpcc/dora_tpcc.h"


ENTER_NAMESPACE(dora);


using namespace shore;
using namespace tpcc;


//
// RVPS
//
// (1) mid1_stock_rvp
// (2) mid2_stock_rvp
// (3) final_stock_rvp
//



/******************************************************************** 
 *
 * @class: mid1_stock_rvp
 *
 * @brief: Submits the IT(OL) action
 *
 ********************************************************************/

class mid1_stock_rvp : public rvp_t
{
private:
    typedef object_cache_t<mid1_stock_rvp> rvp_cache;
    rvp_cache* _cache;
    DoraTPCCEnv* _ptpccenv;
    // data needed for the next phase
    stock_level_input_t  _slin;
    int                  _next_o_id;
public:
    mid1_stock_rvp() : rvp_t(), _cache(NULL), _ptpccenv(NULL) { }
    ~mid1_stock_rvp() { _cache=NULL; _ptpccenv=NULL; }

    // access methods
    inline void set(const tid_t& atid, xct_t* axct, const int& axctid,
                    trx_result_tuple_t& presult, 
                    const stock_level_input_t& slin,
                    DoraTPCCEnv* penv, rvp_cache* pc) 
    { 
        _slin = slin;
        w_assert3 (penv);
        _ptpccenv = penv;
        w_assert3 (pc);
        _cache = pc;
        _set(atid,axct,axctid,presult,1,1); // 1 intratrx - 1 total actions
    }
    inline void giveback() { w_assert3 (_cache); 
        _cache->giveback(this); }    

    void set_next_o_id(const int& nextid) { _next_o_id = nextid; }

    // the interface
    w_rc_t run();

}; // EOF: mid1_stock_rvp



/******************************************************************** 
 *
 * @class: mid2_stock_rvp
 *
 * @brief: Submits the R(ST) --join-- action
 *
 ********************************************************************/


class mid2_stock_rvp : public rvp_t
{
private:
    typedef object_cache_t<mid2_stock_rvp> rvp_cache;
    rvp_cache* _cache;
    DoraTPCCEnv* _ptpccenv;
    // data needed for the next phase
    stock_level_input_t  _slin;
public:
    mid2_stock_rvp() : rvp_t(), _cache(NULL), _ptpccenv(NULL), _pvwi(NULL) { }
    ~mid2_stock_rvp() { _cache=NULL; _ptpccenv=NULL; _pvwi=NULL; }

    TwoIntVec* _pvwi;

    // access methods
    inline void set(const tid_t& atid, xct_t* axct, const int& axctid,
                    trx_result_tuple_t& presult, 
                    const stock_level_input_t& slin, 
                    DoraTPCCEnv* penv, rvp_cache* pc) 
    { 
        _slin = slin;
        w_assert3 (penv);
        _ptpccenv = penv;
        w_assert3 (pc);
        _cache = pc;
        _set(atid,axct,axctid,presult,1,2); // 1 intratrx - 2 total actions
    }
    inline void giveback() { w_assert3 (_cache); 
        _cache->giveback(this); }    

    // the interface
    w_rc_t run();

}; // EOF: mid2_stock_rvp



/******************************************************************** 
 *
 * @class: final_stock_rvp
 *
 * @brief: Terminal Phase of StockLevel
 *
 ********************************************************************/

class final_stock_rvp : public terminal_rvp_t
{
private:
    typedef object_cache_t<final_stock_rvp> rvp_cache;
    DoraTPCCEnv* _ptpccenv;
    rvp_cache* _cache;
public:
    final_stock_rvp() : terminal_rvp_t(), _cache(NULL), _ptpccenv(NULL) { }
    ~final_stock_rvp() { _cache=NULL; _ptpccenv=NULL; }

    // access methods
    inline void set(const tid_t& atid, xct_t* axct, const int axctid,
                    trx_result_tuple_t& presult, 
                    DoraTPCCEnv* penv, rvp_cache* pc) 
    { 
        w_assert3 (penv);
        _ptpccenv = penv;
        w_assert3 (pc);
        _cache = pc;
        _set(atid,axct,axctid,presult,2,2); // 1 intratrx - 3 total actions
    }
    inline void giveback() { 
        w_assert3 (_ptpccenv); 
        _cache->giveback(this); }    

    // interface
    w_rc_t run();
    void upd_committed_stats(); // update the committed trx stats
    void upd_aborted_stats(); // update the committed trx stats

}; // EOF: final_stock_rvp


//
// ACTIONS
//
// (0) stock_action
// (1) r_dist_stock_action
// (2) r_ol_stock_action
// (3) r_st_stock_action
// 


/******************************************************************** 
 *
 * @abstract class: stock_action
 *
 * @brief:          Holds a stock level input and a pointer to ShoreTPCCEnv
 *
 ********************************************************************/

class stock_action : public range_action_impl<int>
{
protected:
    DoraTPCCEnv*   _ptpccenv;
    stock_level_input_t _slin;

    inline void _stock_act_set(const tid_t& atid, xct_t* axct, rvp_t* prvp, 
                               const int keylen, 
                               const stock_level_input_t& stockin,
                               DoraTPCCEnv* penv) 
    {
        _range_act_set(atid,axct,prvp,keylen); 
        _slin = stockin;
        w_assert3 (penv);
        _ptpccenv = penv;
        trx_upd_keys(); // set the keys
    }

public:    
    stock_action() : range_action_impl<int>(), _ptpccenv(NULL) { }
    virtual ~stock_action() { }

    virtual w_rc_t trx_exec()=0;    
    virtual void calc_keys()=0; 
    
}; // EOF: stock_action


// R_DIST_STOCK_ACTION
class r_dist_stock_action : public stock_action
{
private:
    typedef object_cache_t<r_dist_stock_action> act_cache;
    act_cache*       _cache;
    mid1_stock_rvp*  _pmid1_rvp;
public:    
    r_dist_stock_action() : stock_action() { }
    ~r_dist_stock_action() { }
    w_rc_t trx_exec();
    void calc_keys() {
        _down.push_back(_slin._wh_id);
        _down.push_back(_slin._d_id);
        _up.push_back(_slin._wh_id);
        _up.push_back(_slin._d_id);
    }
    inline void set(const tid_t& atid, xct_t* axct, 
                    mid1_stock_rvp* pmid1_rvp, 
                    const stock_level_input_t& slin,
                    DoraTPCCEnv* penv, act_cache* pc) 
    {
        w_assert3 (pmid1_rvp);
        _pmid1_rvp = pmid1_rvp;
        w_assert3 (pc);
        _cache = pc;
        _stock_act_set(atid,axct,pmid1_rvp,2,slin,penv);  // key is (WH|DIST)
    }
    inline void giveback() { 
        w_assert3 (_cache); 
        _cache->giveback(this); }    
   
}; // EOF: upd_wh_stock_action


// R_OL_STOCK_ACTION
class r_ol_stock_action : public stock_action
{
private:
    typedef object_cache_t<r_ol_stock_action> act_cache;
    act_cache*       _cache;
    int _next_o_id;
    mid2_stock_rvp*  _pmid2_rvp;
public:   
    r_ol_stock_action() : stock_action() { }
    ~r_ol_stock_action() { }
    w_rc_t trx_exec();    
    void calc_keys() {
        _down.push_back(_slin._wh_id);
        _down.push_back(_slin._d_id);
        _up.push_back(_slin._wh_id);
        _up.push_back(_slin._d_id);
    }
    inline void set(const tid_t& atid, xct_t* axct, 
                    mid2_stock_rvp* pmid2_rvp, 
                    const int nextid,
                    const stock_level_input_t& slin,
                    DoraTPCCEnv* penv, act_cache* pc) 
    {
        w_assert3 (pmid2_rvp);
        _pmid2_rvp = pmid2_rvp;
        _next_o_id = nextid;
        w_assert3 (pc);
        _cache = pc;
        _stock_act_set(atid,axct,pmid2_rvp,2,slin,penv);  // key is (WH|DIST)
    }
    inline void giveback() { 
        w_assert3 (_cache); 
        _cache->giveback(this); }    

}; // EOF: r_ol_stock_action


// R_ST_STOCK_ACTION
class r_st_stock_action : public stock_action
{
private:
    typedef object_cache_t<r_st_stock_action> act_cache;
    act_cache*       _cache;
    TwoIntVec* _pvwi;
public:   
    r_st_stock_action() : stock_action() { }
    ~r_st_stock_action() { }
    w_rc_t trx_exec();    
    void calc_keys() {
        _down.push_back(_slin._wh_id);
        _up.push_back(_slin._wh_id);
    }
    inline void set(const tid_t& atid, xct_t* axct, rvp_t* prvp, 
                    const stock_level_input_t& slin, TwoIntVec* pvwi,
                    DoraTPCCEnv* penv, act_cache* pc) 
    {
        w_assert3 (pvwi);
        _pvwi = pvwi;
        w_assert3 (pc);
        _cache = pc;
        _stock_act_set(atid,axct,prvp,1,slin,penv);  // key is (WH)
    }
    inline void giveback() { 
        w_assert3 (_cache); 
        if (_pvwi) delete _pvwi; _pvwi = NULL;
        _cache->giveback(this); }    

}; // EOF: r_st_stock_action



EXIT_NAMESPACE(dora);

#endif /** __DORA_TPCC_STOCK_LEVEL_H */

