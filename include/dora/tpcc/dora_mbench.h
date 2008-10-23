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
    final_mb_rvp(tid_t atid, xct_t* axct,
                 trx_result_tuple_t &presult,
                 ShoreTPCCEnv* penv,
                 const int intra_trx_cnt) 
        : terminal_rvp_t(atid, axct, presult, intra_trx_cnt), 
          _ptpccenv(penv) 
    { }
    virtual ~final_mb_rvp() { }
    // required 
    void upd_committed_stats(); // update the committed trx stats
    void upd_aborted_stats(); // update the committed trx stats
    // the interface
    w_rc_t run() { assert (_ptpccenv); return (_run(_ptpccenv)); }
    virtual void cleanup() { } // no clean up to do
}; // EOF: final_mb_rvp



/******************************************************************** 
 *
 * MBench WH
 *
 * @class: upd_wh_mb_action_impl
 *
 * @brief: Updates a WH
 *
 ********************************************************************/

// final_mb_wh_rvp
class final_mb_wh_rvp : public final_mb_rvp
{
public:
    // the list of actions to be remembered to be given back
    upd_wh_mb_action_impl* _puwh;

    final_mb_wh_rvp(tid_t atid, xct_t* axct,
                    trx_result_tuple_t &presult,
                    ShoreTPCCEnv* penv) 
        : final_mb_rvp(atid, axct, presult, penv, 1),
          _puwh(NULL)
    { }
    ~final_mb_wh_rvp() { }
    void cleanup() {
        assert (_puwh);
        _g_dora->give_action(_puwh);
    }
}; // EOF: final_mb_wh_rvp

// UPD_WH_MB_ACTION
class upd_wh_mb_action_impl : public range_action_impl<int>
{
private:
    ShoreTPCCEnv* _ptpccenv;
    int _whid;
public:    
    upd_wh_mb_action_impl() : range_action_impl<int>(), 
                              _ptpccenv(NULL), _whid(-1) { }
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

// final_mb_cust_rvp
class final_mb_cust_rvp : public final_mb_rvp
{
public:
    // the list of actions to be remembered to be given back
    upd_cust_mb_action_impl* _puc;
    final_mb_cust_rvp(tid_t atid, xct_t* axct,
                    trx_result_tuple_t &presult,
                    ShoreTPCCEnv* penv) 
        : final_mb_rvp(atid, axct, presult, penv, 1),
          _puc(NULL)
    { }
    ~final_mb_cust_rvp() { }
    void cleanup() {
        assert (_puc);
        _g_dora->give_action(_puc);
    }
}; // EOF: final_mb_cust_rvp

// UPD_CUST_MB_ACTION
class upd_cust_mb_action_impl : public range_action_impl<int>
{
private:
    ShoreTPCCEnv* _ptpccenv;
    int _whid;
    int _did;
    int _cid;
public:    
    upd_cust_mb_action_impl() : 
        range_action_impl<int>(),
        _ptpccenv(NULL), _whid(-1) { }
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

