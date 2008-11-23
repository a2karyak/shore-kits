/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   dora_mbench.h
 *
 *  @brief:  DORA MBENCHES
 *
 *  @note:   Definition of RVPs and Actions that synthesize 
 *           the mbenches according to DORA
 *
 *  @author: Ippokratis Pandis, Oct 2008
 */


#ifndef __DORA_MBENCHES_H
#define __DORA_MBENCHES_H


#include "dora.h"
#include "workload/tpcc/shore_tpcc_env.h"


ENTER_NAMESPACE(dora);


using namespace shore;
using namespace tpcc;



/******************************************************************** 
 *
 * @class: final_mb_rvp
 *
 * @brief: Generic Terminal Phase of MBenches
 *
 * @note:  Updates the counter for the OTHER trxs of ShoreTPCCEnv
 *
 ********************************************************************/

class final_mb_rvp : public terminal_rvp_t
{
private:
    typedef atomic_class_stack<final_mb_rvp> rvp_pool;
    rvp_pool* _pool;
    ShoreTPCCEnv* _ptpccenv;
public:
    final_mb_rvp() : terminal_rvp_t(), _pool(NULL), _ptpccenv(NULL) { }
    ~final_mb_rvp() { _pool=NULL; _ptpccenv=NULL; }
    
    // access methods
    inline void set(const tid_t& atid, xct_t* axct, const int axctid,
                    trx_result_tuple_t& presult, 
                    const int intra_trx_cnt, const int total_actions,
                    ShoreTPCCEnv* penv, rvp_pool* pp) 
    { 
        _set(atid,axct,axctid,presult,intra_trx_cnt,total_actions);
        assert (penv);
        _ptpccenv = penv;
        assert (pp);
        _pool = pp;
    }
    inline void giveback() { assert (_pool); _pool->destroy(this); }

    // the interface
    w_rc_t run() { assert (_ptpccenv); return (_run(_ptpccenv)); }
    void upd_committed_stats(); // update the committed trx stats
    void upd_aborted_stats(); // update the committed trx stats

}; // EOF: final_mb_rvp



/******************************************************************** 
 *
 * MBench Actions
 *
 * @class: Generic class for mbench actions
 *
 ********************************************************************/

class mb_action : public range_action_impl<int>
{
protected:
    ShoreTPCCEnv* _ptpccenv;
    int _whid;

    inline void _mb_act_set(const tid_t& atid, xct_t* axct, rvp_t* prvp, 
                            const int keylen, ShoreTPCCEnv* penv, 
                            const int awh)
    {
        _range_act_set(atid,axct,prvp,keylen); 
        assert (penv);
        _ptpccenv = penv;
        _whid=awh;
        set_key_range(); // set key range for this action
    }

public:
    mb_action() : range_action_impl<int>(), _ptpccenv(NULL), _whid(-1) { }
    virtual ~mb_action() { }

    // interface
    virtual w_rc_t trx_exec()=0; // pure virtual    
    virtual void calc_keys()=0; // pure virtual 

    const bool trx_acq_locks(); // all mbenches are unique probes

}; // EOF: mb_action


/******************************************************************** 
 *
 * MBench WH
 *
 * @class: upd_wh_mb_action
 *
 * @brief: Updates a WH
 *
 ********************************************************************/

class upd_wh_mb_action : public mb_action
{
private:
    typedef atomic_class_stack<upd_wh_mb_action> act_pool;
    act_pool*       _pool;
public:        
    upd_wh_mb_action() : mb_action() { }    
    ~upd_wh_mb_action() { }
    inline void set(const tid_t& atid, xct_t* apxct, rvp_t* aprvp, 
                    const int awh, ShoreTPCCEnv* penv, act_pool* pp) 
    { 
        assert (pp);
        _pool = pp;
        _mb_act_set(atid,apxct,aprvp,1,penv,awh);  // key is (WH)
        // call to mb_act_set() should be the last one
        // because it will call the calc_keys() 
    }
    
    w_rc_t trx_exec();        
    void calc_keys() {
        _down.push_back(_whid);
        _up.push_back(_whid);
    }

    inline void giveback() { assert (_pool); _pool->destroy(this); }    
}; // EOF: upd_wh_mb_action



/******************************************************************** 
 *
 * MBench CUST
 *
 * @class: upd_cust_mb_action
 *
 * @brief: Updates a CUSTOMER
 *
 ********************************************************************/

// UPD_CUST_MB_ACTION
class upd_cust_mb_action : public mb_action
{
private:
    typedef atomic_class_stack<upd_cust_mb_action> act_pool;
    act_pool*       _pool;
    int _did;
    int _cid;
public:    
    upd_cust_mb_action() : mb_action(), _did(-1), _cid(-1) { }    
    ~upd_cust_mb_action() { }

    inline void set(const tid_t& atid, xct_t* apxct, rvp_t* aprvp, 
                    const int awh, ShoreTPCCEnv* penv, act_pool* pp) 
    { 
        _did = URand(1,10); // generate the other two needed vars
        _cid = NURand(1023,1,3000);
        assert (pp);
        _pool = pp;
        _mb_act_set(atid,apxct,aprvp,1,penv,awh);  // key is (WH|DIST|CUST)
        // call to mb_act_set() should be the last one
        // because it will call the calc_keys() 
    }

    w_rc_t trx_exec();        
    void calc_keys() {
        _down.push_back(_whid);
        _down.push_back(_did);
        _down.push_back(_cid);
        _up.push_back(_whid);
        _up.push_back(_did);
        _up.push_back(_cid);
    }

    inline void giveback() { assert (_pool); _pool->destroy(this); }    
}; // EOF: upd_cust_mb_action


EXIT_NAMESPACE(dora);

#endif /** __DORA_MBENCHES_H */

