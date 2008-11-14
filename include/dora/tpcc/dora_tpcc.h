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

#include "dora/tpcc/dora_payment.h"
#include "dora/tpcc/dora_mbench.h"

#include "workload/tpcc/shore_tpcc_env.h"

#include "dora.h"


ENTER_NAMESPACE(dora);


using namespace shore;
using namespace tpcc;


const int DF_ACTION_CACHE_SZ = 100;



/******************************************************************** 
 *
 * @class: dora_tpcc
 *
 * @brief: Container class for all the data partitions for the TPC-C database
 *
 ********************************************************************/

class DoraTPCCEnv : public ShoreTPCCEnv
{
public:

    typedef range_partition_impl<int>   irpImpl; 
    typedef range_part_table_impl<int>  irpTableImpl;

    typedef irpImpl::RangeAction      irpAction;
    typedef irpImpl::PartKey          irpImplKey;

    typedef vector<irpTableImpl*>       irpTablePtrVector;
    typedef irpTablePtrVector::iterator irpTablePtrVectorIt;

private:

    // a vector of pointers to integer-range-partitioned tables
    irpTablePtrVector _irptp_vec;    

    // the list of pointer to irp-tables
    guard<irpTableImpl> _wh_irpt; // WAREHOUSE
    guard<irpTableImpl> _di_irpt; // DISTRICT
    guard<irpTableImpl> _cu_irpt; // CUSTOMER
    guard<irpTableImpl> _hi_irpt; // HISTORY
    guard<irpTableImpl> _no_irpt; // NEW-ORDER
    guard<irpTableImpl> _or_irpt; // ORDER
    guard<irpTableImpl> _it_irpt; // ITEM
    guard<irpTableImpl> _ol_irpt; // ORDER-LINE
    guard<irpTableImpl> _st_irpt; // STOCK

public:
    

    DoraTPCCEnv(string confname)
        : ShoreTPCCEnv(confname)
    { }
    ~DoraTPCCEnv() { }




    //// Control Database

    // {Start/Stop/Resume/Pause} the system 
    const int start();
    const int stop();
    const int resume();
    const int pause();
    const int newrun();
    const int set(envVarMap* vars);
    const int dump();
    virtual const int conf() { return (ShoreTPCCEnv::conf()); }
    virtual const int info();    
    virtual const int statistics();    

    //// Client API
    
    // enqueues action, false on error
    inline const int enqueue(irpAction* paction, 
                             irpTableImpl* ptable, 
                             const int part_pos) 
    {
        assert (paction);
        assert (ptable);
        return (ptable->enqueue(paction, part_pos));
    }



    //// Partition-related methods

    inline irpImpl* get_part(const table_pos, const int part_pos) {
        assert (table_pos<_irptp_vec.size());        
        return (_irptp_vec[table_pos]->get_part(part_pos));
    }

    // Access irp-tables
    inline irpTableImpl* whs() { return (_wh_irpt.get()); }
    inline irpTableImpl* dis() { return (_di_irpt.get()); }
    inline irpTableImpl* cus() { return (_cu_irpt.get()); }
    inline irpTableImpl* his() { return (_hi_irpt.get()); }
    inline irpTableImpl* nor() { return (_no_irpt.get()); }
    inline irpTableImpl* ord() { return (_or_irpt.get()); }
    inline irpTableImpl* ite() { return (_it_irpt.get()); }
    inline irpTableImpl* oli() { return (_ol_irpt.get()); }
    inline irpTableImpl* sto() { return (_st_irpt.get()); }

    // Access specific partitions
    inline irpImpl* whs(const int pos) { return (_wh_irpt->get_part(pos)); }
    inline irpImpl* dis(const int pos) { return (_di_irpt->get_part(pos)); }
    inline irpImpl* cus(const int pos) { return (_cu_irpt->get_part(pos)); }
    inline irpImpl* his(const int pos) { return (_hi_irpt->get_part(pos)); }
    inline irpImpl* nor(const int pos) { return (_no_irpt->get_part(pos)); }
    inline irpImpl* ord(const int pos) { return (_or_irpt->get_part(pos)); }
    inline irpImpl* ite(const int pos) { return (_it_irpt->get_part(pos)); }
    inline irpImpl* oli(const int pos) { return (_ol_irpt->get_part(pos)); }
    inline irpImpl* sto(const int pos) { return (_st_irpt->get_part(pos)); }



    //// atomic trash stacks


    // TPC-C Payment
    atomic_class_stack<final_pay_rvp>  _final_pay_rvp_pool;
    atomic_class_stack<midway_pay_rvp> _midway_pay_rvp_pool;

    atomic_class_stack<upd_wh_pay_action>     _upd_wh_pay_pool; 
    atomic_class_stack<upd_dist_pay_action>   _upd_dist_pay_pool;    
    atomic_class_stack<upd_cust_pay_action>   _upd_cust_pay_pool;    
    atomic_class_stack<ins_hist_pay_action>   _ins_hist_pay_pool;    


    // MBenches
    atomic_class_stack<final_mb_rvp>   _final_mb_rvp_pool;

    atomic_class_stack<upd_wh_mb_action>      _upd_wh_mb_pool;    
    atomic_class_stack<upd_cust_mb_action>    _upd_cust_mb_pool;    



    //// TRXs


    // --- kit DORA trxs --- //

    // --- baseline mbench --- //
    w_rc_t run_mbench_cust(const int xct_id, trx_result_tuple_t& atrt, int specificWH);
    w_rc_t run_mbench_wh(const int xct_id, trx_result_tuple_t& atrt, int specificWH);



    /* --- kit dora trxs --- */

    /* --- with input specified --- */
    w_rc_t dora_new_order(const int xct_id, new_order_input_t& anoin, trx_result_tuple_t& atrt);
    w_rc_t dora_payment(const int xct_id, payment_input_t& apin, trx_result_tuple_t& atrt);
    w_rc_t dora_order_status(const int xct_id, order_status_input_t& aordstin, trx_result_tuple_t& atrt);
    w_rc_t dora_delivery(const int xct_id, delivery_input_t& adelin, trx_result_tuple_t& atrt);
    w_rc_t dora_stock_level(const int xct_id, stock_level_input_t& astoin, trx_result_tuple_t& atrt);

    /* --- without input specified --- */
    w_rc_t dora_new_order(const int xct_id, trx_result_tuple_t& atrt, int specificWH);
    w_rc_t dora_payment(const int xct_id, trx_result_tuple_t& atrt, int specificWH);
    w_rc_t dora_order_status(const int xct_id, trx_result_tuple_t& atrt, int specificWH);
    w_rc_t dora_delivery(const int xct_id, trx_result_tuple_t& atrt, int specificWH);
    w_rc_t dora_stock_level(const int xct_id, trx_result_tuple_t& atrt, int specificWH);

    /* --- dora mbench --- */
    w_rc_t dora_mbench_cust(const int xct_id, trx_result_tuple_t& atrt, int whid);
    w_rc_t dora_mbench_wh(const int xct_id, trx_result_tuple_t& atrt, int whid);



private:

    // algorithm for deciding the distribution of tables 
    const processorid_t _next_cpu(const processorid_t aprd,
                                  const irpTableImpl* atable,
                                  const int step=DF_CPU_STEP_TABLES);

        
}; // EOF: DoraTPCCEnv



EXIT_NAMESPACE(dora);

#endif /** __DORA_TPCC_H */

