/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file dora_tpcc.h
 *
 *  @brief The DORA TPC-C class
 *
 *  @author Ippokratis Pandis, Oct 2008
 */


#ifndef __DORA_TPCC_H
#define __DORA_TPCC_H

#include <cstdio>

#include "util.h"
#include "dora.h"
#include "stages/tpcc/shore/shore_tpcc_env.h"


ENTER_NAMESPACE(dora);


using namespace shore;
using namespace tpcc;



/******** Exported variables ********/

class dora_tpcc_db;
extern dora_tpcc_db* _g_dora;

/******** EOF Exported variables ********/

const int DF_ACTION_CACHE_SZ = 100;



class upd_wh_pay_action_impl;
class upd_dist_pay_action_impl;
class upd_cust_pay_action_impl;
class ins_hist_pay_action_impl;


/** PAYMENT RVP **/

// Phase 1 of Payment
// Submits the history packet
class midway_pay_rvp : public rvp_t
{
private:

    // the data needed for communication
    tpcc_warehouse_tuple _awh;
    tpcc_district_tuple  _adist;

    // the list of action to be remembered to be given back
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


// Terminal Phase of Payment
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


/** PAYMENT ACTIONS **/

// ABSTRACT PAYMENT ACTION
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





/******************************************************************** 
 *
 * @class: dora_tpcc
 *
 * @brief: Container class for all the data partitions for the TPC-C database
 *
 ********************************************************************/

class dora_tpcc_db
{
public:

    typedef range_partition_impl<int>   irp_impl; 
    typedef range_part_table_impl<int>  irp_table_impl;
    typedef irp_impl::range_action      irp_action;
    typedef irp_impl::part_key          ikey;

    typedef vector<irp_table_impl*>     irpTablePtrVector;
    typedef irpTablePtrVector::iterator irpTablePtrVectorIt;

    
    typedef action_cache_t<upd_wh_pay_action_impl>   upd_wh_pay_action_cache;
    typedef action_cache_t<upd_dist_pay_action_impl> upd_dist_pay_action_cache;
    typedef action_cache_t<upd_cust_pay_action_impl> upd_cust_pay_action_cache;
    typedef action_cache_t<ins_hist_pay_action_impl> ins_hist_pay_action_cache;

private:

    // member variables

    // the shore env
    ShoreTPCCEnv* _tpccenv;    

    // a vector of pointers to integer-range-partitioned tables
    irpTablePtrVector _irptp_vec;    

    // the list of pointer to irp-tables
    irp_table_impl* _wh_irpt; // WAREHOUSE
    irp_table_impl* _di_irpt; // DISTRICT
    irp_table_impl* _cu_irpt; // CUSTOMER
    irp_table_impl* _hi_irpt; // HISTORY
    irp_table_impl* _no_irpt; // NEW-ORDER
    irp_table_impl* _or_irpt; // ORDER
    irp_table_impl* _it_irpt; // ITEM
    irp_table_impl* _ol_irpt; // ORDER-LINE
    irp_table_impl* _st_irpt; // STOCK


    // cache of actions (per action tpcc trx)
    upd_wh_pay_action_cache*   _upd_wh_pay_cache;        /* pointer to a upd wh payment action cache */
    upd_dist_pay_action_cache* _upd_dist_pay_cache;      /* pointer to a upd dist payment action cache */
    upd_cust_pay_action_cache* _upd_cust_pay_cache;      /* pointer to a upd cust payment action cache */
    ins_hist_pay_action_cache* _ins_hist_pay_cache;      /* pointer to a ins hist payment action cache */


public:

    dora_tpcc_db(ShoreTPCCEnv* tpccenv)
        : _tpccenv(tpccenv)
    {
        assert (_tpccenv);

        // initialize action caches
        _upd_wh_pay_cache   = new upd_wh_pay_action_cache(DF_ACTION_CACHE_SZ);
        _upd_dist_pay_cache = new upd_dist_pay_action_cache(DF_ACTION_CACHE_SZ);
        _upd_cust_pay_cache = new upd_cust_pay_action_cache(DF_ACTION_CACHE_SZ);
        _ins_hist_pay_cache = new ins_hist_pay_action_cache(DF_ACTION_CACHE_SZ);
    }

    ~dora_tpcc_db() { 
        // delete action caches
        if (_upd_wh_pay_cache) {
            // print_cache_stats();
            delete (_upd_wh_pay_cache);
            _upd_wh_pay_cache = NULL;
        }
        if (_upd_dist_pay_cache) {
            // print_cache_stats();
            delete (_upd_dist_pay_cache);
            _upd_dist_pay_cache = NULL;
        }
        if (_upd_cust_pay_cache) {
            // print_cache_stats();
            delete (_upd_cust_pay_cache);
            _upd_cust_pay_cache = NULL;
        }
        if (_ins_hist_pay_cache) {
            // print_cache_stats();
            delete (_ins_hist_pay_cache);
            _ins_hist_pay_cache = NULL;
        }
    }    


    /** Access methods */

    // locks for the transactions - one mutex per transaction
    // needed for having a total order 
    // across actions of different trxs
    //     mcs_lock _nord_lock;
    //     mcs_lock _pay_lock;
    //     mcs_lock _orderst_lock;
    //     mcs_lock _deliv_lock;
    //     mcs_lock _stocklev_lock;

    inline irp_impl* get_part(const table_pos, const int part_pos) {
        assert (table_pos<_irptp_vec.size());        
        return (_irptp_vec[table_pos]->get_part(part_pos));
    }


    // action cache related actions

    // returns the cache
    upd_wh_pay_action_cache* get_upd_wh_pay_cache() { 
        assert (_upd_wh_pay_cache); 
        return (_upd_wh_pay_cache); 
    }
    upd_dist_pay_action_cache* get_upd_dist_pay_cache() { 
        assert (_upd_dist_pay_cache); 
        return (_upd_dist_pay_cache); 
    }
    upd_cust_pay_action_cache* get_upd_cust_pay_cache() { 
        assert (_upd_cust_pay_cache); 
        return (_upd_cust_pay_cache); 
    }
    ins_hist_pay_action_cache* get_ins_hist_pay_cache() { 
        assert (_ins_hist_pay_cache); 
        return (_ins_hist_pay_cache); 
    }


    //
    // borrow and release an action
    //
    // @note: On but borrowing first it makes sure that it is empty
    //
    upd_wh_pay_action_impl* get_upd_wh_pay_action() {
//         assert (_upd_wh_pay_cache);
//         upd_wh_pay_action_impl* paction = _upd_wh_pay_cache->borrow();
//         assert (paction);
//         assert (paction->keys()->size()==0);
        upd_wh_pay_action_impl* paction = new upd_wh_pay_action_impl();
        return (paction);
    }
    void give_action(upd_wh_pay_action_impl* pirpa) {
        assert (_upd_wh_pay_cache);
//         _upd_wh_pay_cache->giveback(pirpa);
        if (pirpa) delete pirpa;
    }

    upd_dist_pay_action_impl* get_upd_dist_pay_action() {
        assert (_upd_dist_pay_cache);
//         upd_dist_pay_action_impl* paction = _upd_dist_pay_cache->borrow();
//         assert (paction);
//         assert (paction->keys()->size()==0);
        upd_dist_pay_action_impl* paction = new upd_dist_pay_action_impl();
        return (paction);
    }
    void give_action(upd_dist_pay_action_impl* pirpa) {
        assert (_upd_dist_pay_cache);
//         _upd_dist_pay_cache->giveback(pirpa);
        if (pirpa) delete pirpa;
    }

    upd_cust_pay_action_impl* get_upd_cust_pay_action() {
//         assert (_upd_cust_pay_cache);
//         upd_cust_pay_action_impl* paction = _upd_cust_pay_cache->borrow();
//         assert (paction);
//         assert (paction->keys()->size()==0);
        upd_cust_pay_action_impl* paction = new upd_cust_pay_action_impl();
        return (paction);
    }
    void give_action(upd_cust_pay_action_impl* pirpa) {
//         assert (_upd_cust_pay_cache);
//         _upd_cust_pay_cache->giveback(pirpa);
        if (pirpa) delete pirpa;
    }

    ins_hist_pay_action_impl* get_ins_hist_pay_action() {
//         assert (_ins_hist_pay_cache);
//         ins_hist_pay_action_impl* paction = _ins_hist_pay_cache->borrow();
//         assert (paction);
//         assert (paction->keys()->size()==0);
        ins_hist_pay_action_impl* paction = new ins_hist_pay_action_impl();
        return (paction);
    }
    void give_action(ins_hist_pay_action_impl* pirpa) {
        assert (_ins_hist_pay_cache);
//         _ins_hist_pay_cache->giveback(pirpa);
        if (pirpa) delete pirpa;
    }


    // Access irp-tables
    irp_table_impl* whs() const { return (_wh_irpt); }
    irp_table_impl* dis() const { return (_di_irpt); }
    irp_table_impl* cus() const { return (_cu_irpt); }
    irp_table_impl* his() const { return (_hi_irpt); }
    irp_table_impl* nor() const { return (_no_irpt); }
    irp_table_impl* ord() const { return (_or_irpt); }
    irp_table_impl* ite() const { return (_it_irpt); }
    irp_table_impl* oli() const { return (_ol_irpt); }
    irp_table_impl* sto() const { return (_st_irpt); }

    // Access specific partitions
    irp_impl* whs(const int pos) const { return (_wh_irpt->get_part(pos)); }
    irp_impl* dis(const int pos) const { return (_di_irpt->get_part(pos)); }
    irp_impl* cus(const int pos) const { return (_cu_irpt->get_part(pos)); }
    irp_impl* his(const int pos) const { return (_hi_irpt->get_part(pos)); }
    irp_impl* nor(const int pos) const { return (_no_irpt->get_part(pos)); }
    irp_impl* ord(const int pos) const { return (_or_irpt->get_part(pos)); }
    irp_impl* ite(const int pos) const { return (_it_irpt->get_part(pos)); }
    irp_impl* oli(const int pos) const { return (_ol_irpt->get_part(pos)); }
    irp_impl* sto(const int pos) const { return (_st_irpt->get_part(pos)); }


    /** Control Database */

    // {Start/Stop/Resume/Pause} the system 
    const int start();
    const int stop();
    const int resume();
    const int pause();
    
    /** Client API */
    
    // enqueues action, false on error
    inline const int enqueue(irp_action* paction, 
                             irp_table_impl* ptable, 
                             const int part_pos) 
    {
        assert (paction);
        assert (ptable);
        return (ptable->enqueue(paction, part_pos));
    }

    /** For debugging */

    // dumps information
    void dump() const;


private:

    // algorithm for deciding the distribution of tables 
    const processorid_t _next_cpu(const processorid_t aprd,
                                  const irp_table_impl* atable,
                                  const int step=DF_CPU_STEP_TABLES);

        
}; // EOF: dora_tpcc_db



EXIT_NAMESPACE(dora);

#endif /** __DORA_TPCC_H */

