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

#include "dora/tpcc/dora_tpcc.h"


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
    // pointer to the shore env
    ShoreTPCCEnv* _ptpccenv;
public:

    final_mb_rvp(tid_t atid, xct_t* axct, const int axctid,
                 trx_result_tuple_t &presult, const int intra_trx_cnt, 
                 ShoreTPCCEnv* penv) 
        : terminal_rvp_t(atid, axct, axctid, presult, intra_trx_cnt), 
          _ptpccenv(penv) 
    { }

    virtual ~final_mb_rvp() { }

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

class mb_action_impl : public range_action_impl<int>
{
protected:
    ShoreTPCCEnv* _ptpccenv;
    int _whid;
public:
    mb_action_impl() :
        range_action_impl<int>(), _ptpccenv(NULL), _whid(-1) 
    { }

    virtual ~mb_action_impl() { }

    // interface
    virtual w_rc_t trx_exec()=0; // pure virtual    
    const bool trx_acq_locks();
    virtual void calc_keys()=0; // pure virtual 

}; // EOF: mb_action_impl


/******************************************************************** 
 *
 * MBench WH
 *
 * @class: upd_wh_mb_action_impl
 *
 * @brief: Updates a WH
 *
 ********************************************************************/

class upd_wh_mb_action_impl : public mb_action_impl
{
public:        
    upd_wh_mb_action_impl()  { }    
    ~upd_wh_mb_action_impl() { }

    void set_input(tid_t atid, xct_t* apxct, rvp_t* aprvp, 
                   ShoreTPCCEnv* penv, const int awh) { 
        assert (apxct);
        assert (aprvp);
        assert (penv);
        set(atid, apxct, aprvp);
        _ptpccenv = penv;
        _whid=awh; 
        set_key_range();
    }
    
    w_rc_t trx_exec();        
    void calc_keys() {
        _down.push_back(_whid);
        _up.push_back(_whid);
    }
}; // EOF: upd_wh_mb_action_impl



/******************************************************************** 
 *
 * MBench CUST
 *
 * @class: upd_cust_mb_action_impl
 *
 * @brief: Updates a CUSTOMER
 *
 ********************************************************************/

// UPD_CUST_MB_ACTION
class upd_cust_mb_action_impl : public mb_action_impl
{
private:
    int _did;
    int _cid;
public:    

    upd_cust_mb_action_impl() :
        _did(-1), _cid(-1)
    { }    
    ~upd_cust_mb_action_impl() { }
    
    void set_input(tid_t atid, xct_t* apxct, rvp_t* aprvp,
                   ShoreTPCCEnv* penv, const int awh) {
        assert (apxct);
        assert (aprvp);
        assert (penv);
        set(atid, apxct, aprvp);
        _ptpccenv = penv; 
        _whid=awh;
        _did = URand(1,10); // generate the other two needed vars
        _cid = NURand(1023,1,3000);
        set_key_range();
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

}; // EOF: upd_cust_mb_action_impl


EXIT_NAMESPACE(dora);

#endif /** __DORA_MBENCHES_H */

