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
#include "workload/tpcc/shore_tpcc_env.h"
#include "dora.h"


ENTER_NAMESPACE(dora);


using namespace shore;
using namespace tpcc;




const int KEYPTR_PER_ACTION_POOL_SZ = 60;
const int KALREQ_PER_ACTION_POOL_SZ = 30;
const int DT_PER_ACTION_POOL_SZ = 360;



typedef Pool* PoolPtr;

const int ACTIONS_PER_RVP_POOL_SZ = 30; // should be comparable with batch_sz


                //_poolArray[0] = _baseActionPtrPool = new Pool(sizeof(base_action_t*), ACTIONS_PER_RVP_POOL_SZ); \

#define DECLARE_RVP_CACHE(Type)                         \
    struct Type##_cache   {                             \
            guard< object_cache_t<Type> > _cache;       \
            guard<Pool> _baseActionPtrPool;             \
            guard<PoolPtr> _poolArray;                  \
            Type##_cache() {                            \
                _poolArray = new PoolPtr[1];            \
                _cache = new object_cache_t<Type>(_poolArray.get()); }  \
            ~Type##_cache() { _cache.done(); _baseActionPtrPool.done(); } };


#define DECLARE_TLS_RVP_CACHE(Type)              \
    DECLARE_RVP_CACHE(Type);                     \
    DECLARE_TLS(Type##_cache,my_##Type##_cache);


//                 _poolArray[0] = _keyPtrPool = new Pool(sizeof(key_wrapper_t<Datatype>*), KEYPTR_PER_ACTION_POOL_SZ); \
//                 _poolArray[1] = _kalReqPool = new Pool(sizeof(KALReq_t<Datatype>), KALREQ_PER_ACTION_POOL_SZ); \
//                 _poolArray[2] = _dtPool = new Pool(sizeof(Datatype), DT_PER_ACTION_POOL_SZ); \

#define DECLARE_ACTION_CACHE(Type,Datatype)             \
    struct Type##_cache  {                              \
            guard<object_cache_t<Type> > _cache;        \
            guard<Pool> _keyPtrPool;                    \
            guard<Pool> _kalReqPool;                    \
            guard<Pool> _dtPool;                        \
            guard<PoolPtr> _poolArray;                  \
            Type##_cache() {                            \
                _poolArray = new PoolPtr[3];            \
                _cache = new object_cache_t<Type>(_poolArray.get()); }  \
            ~Type##_cache() { _cache.done(); } };



#define DECLARE_TLS_ACTION_CACHE(Type,Datatype)      \
    DECLARE_ACTION_CACHE(Type,Datatype);             \
    DECLARE_TLS(Type##_cache,my_##Type##_cache);





const int DF_ACTION_CACHE_SZ = 100;


// Forward decl

// TPC-C Payment
class final_pay_rvp;
class midway_pay_rvp;
class upd_wh_pay_action;
class upd_dist_pay_action;
class upd_cust_pay_action;
class ins_hist_pay_action;

// MBenches
class final_mb_rvp;
class upd_wh_mb_action;
class upd_cust_mb_action;




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

    typedef irpImpl::RangeAction  irpAction;

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


    // tpcc setup variables
    int _starting_cpu;
    int _cpu_table_step;
    int _cpu_range;
    int _sf;

    int _wh_per_part_wh;
    int _wh_per_part_dist;
    int _wh_per_part_cust;
    int _wh_per_part_hist;
    int _wh_per_part_nord;
    int _wh_per_part_ord;
    int _wh_per_part_item;
    int _wh_per_part_oline;
    int _wh_per_part_stock;

    //guard<Pool> _int_dtpool;

public:
    

    DoraTPCCEnv(string confname)
        : ShoreTPCCEnv(confname)
    { 
        //_int_dtpool = new Pool(sizeof(int),4);
    }

    virtual ~DoraTPCCEnv() 
    { 
        stop();
        //_int_dtpool.done();
    }




    //// Control Database

    // {Start/Stop/Resume/Pause} the system 
    const int start();
    const int stop();
    const int resume();
    const int pause();
    const int newrun();
    const int set(envVarMap* vars);
    const int dump();
    virtual const int conf();
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

    inline irpImpl* table_part(const int table_pos, const int part_pos) {
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




    //typedef PooledVec<base_action_t*>::Type baseActionsList;
    typedef vector<base_action_t*> baseActionsList;


    // TPC-C Payment
    final_pay_rvp* NewFinalPayRvp(const tid_t& atid, xct_t* axct, const int axctid, 
                                  trx_result_tuple_t& presult,
                                  baseActionsList& actions);

    midway_pay_rvp* NewMidayPayRvp(const tid_t& atid, xct_t* axct, const int axctid,
                                   trx_result_tuple_t& presult,
                                   const payment_input_t& pin);

    upd_wh_pay_action* NewUpdWhPayAction(const tid_t& atid, xct_t* axct, midway_pay_rvp* prvp,
                                         const payment_input_t& pin);

    upd_dist_pay_action* NewUpdDistPayAction(const tid_t& atid, xct_t* axct, midway_pay_rvp* prvp,
                                             const payment_input_t& pin);

    upd_cust_pay_action* NewUpdCustPayAction(const tid_t& atid, xct_t* axct, midway_pay_rvp* prvp,
                                             const payment_input_t& pin);

    ins_hist_pay_action* NewInsHistPayAction(const tid_t& atid, xct_t* axct, rvp_t* prvp,
                                             const payment_input_t& pin,
                                             const tpcc_warehouse_tuple& awh,
                                             const tpcc_district_tuple& adist);

    // MBenches
    final_mb_rvp* NewFinalMbRvp(const tid_t& atid, xct_t* axct, const int axctid, 
                                trx_result_tuple_t& presult);

    upd_wh_mb_action* NewUpdWhMbAction(const tid_t& atid, xct_t* axct, rvp_t* prvp,
                                       const int whid);

    upd_cust_mb_action* NewUpdCustMbAction(const tid_t& atid, xct_t* axct, rvp_t* prvp,
                                           const int whid);



private:

    // algorithm for deciding the distribution of tables 
    const processorid_t _next_cpu(const processorid_t& aprd,
                                  const irpTableImpl* atable,
                                  const int step=DF_CPU_STEP_TABLES);



        
}; // EOF: DoraTPCCEnv



EXIT_NAMESPACE(dora);

#endif /** __DORA_TPCC_H */

