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

#include "tls.h"

#include "util.h"
#include "workload/tpcc/shore_tpcc_env.h"
#include "dora.h"


ENTER_NAMESPACE(dora);


using namespace shore;
using namespace tpcc;




const int KEYPTR_PER_ACTION_POOL_SZ = 60;
const int KALREQ_PER_ACTION_POOL_SZ = 30;
const int DT_PER_ACTION_POOL_SZ = 360;


typedef vector<pair<int,int> > TwoIntVec;
typedef TwoIntVec::iterator    TwoIntVecIt;

typedef Pool* PoolPtr;

const int ACTIONS_PER_RVP_POOL_SZ = 30; // should be comparable with batch_sz


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


// Forward declarations

// MBenches
class final_mb_rvp;
class upd_wh_mb_action;
class upd_cust_mb_action;

// TPC-C Payment
class final_pay_rvp;
class midway_pay_rvp;
class upd_wh_pay_action;
class upd_dist_pay_action;
class upd_cust_pay_action;
class ins_hist_pay_action;

// TPC-C OrderStatus
class final_ordst_rvp;
class r_cust_ordst_action;
class r_ol_ordst_action;

// TPC-C StockLevel
class mid1_stock_rvp;
class mid2_stock_rvp;
class final_stock_rvp;
class r_dist_stock_action;
class r_ol_stock_action;
class r_st_stock_action;

// TPC-C Delivery
class mid1_del_rvp;
class mid2_del_rvp;
class final_del_rvp;
class del_nord_del_action;
class upd_ord_del_action;
class upd_oline_del_action;
class upd_cust_del_action;

// TPC-C NewOrder
class midway_nord_rvp;
class final_nord_rvp;
class r_wh_nord_action;
class r_cust_nord_action;
class upd_dist_nord_action;
class r_item_nord_action;
class upd_sto_nord_action;
class ins_ord_nord_action;
class ins_nord_nord_action;
class ins_ol_nord_action;


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


    // MBenches
    final_mb_rvp* NewFinalMbRvp(const tid_t& atid, xct_t* axct, const int axctid, 
                                trx_result_tuple_t& presult);

    upd_wh_mb_action* NewUpdWhMbAction(const tid_t& atid, xct_t* axct, rvp_t* prvp,
                                       const int whid);

    upd_cust_mb_action* NewUpdCustMbAction(const tid_t& atid, xct_t* axct, rvp_t* prvp,
                                           const int whid);


    // TPC-C Payment
    midway_pay_rvp* NewMidwayPayRvp(const tid_t& atid, xct_t* axct, const int axctid,
                                    trx_result_tuple_t& presult,
                                    const payment_input_t& pin);

    final_pay_rvp* NewFinalPayRvp(const tid_t& atid, xct_t* axct, const int axctid, 
                                  trx_result_tuple_t& presult,
                                  baseActionsList& actions);

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



    // TPC-C OrderStatus
    final_ordst_rvp* NewFinalOrdStRvp(const tid_t& atid, xct_t* axct, const int axctid, 
                                      trx_result_tuple_t& presult);

    r_cust_ordst_action* NewRCustOrdStAction(const tid_t& atid, xct_t* axct, rvp_t* prvp,
                                             const order_status_input_t& ordstin);

    r_ol_ordst_action* NewROlOrdStAction(const tid_t& atid, xct_t* axct, rvp_t* prvp,
                                         const order_status_input_t& ordstin,
                                         const tpcc_order_tuple& aorder);


    // TPC-C StockLevel
    mid1_stock_rvp* NewMid1StockRvp(const tid_t& atid, xct_t* axct, const int axctid, 
                                    trx_result_tuple_t& presult,
                                    const stock_level_input_t& slin);

    mid2_stock_rvp* NewMid2StockRvp(const tid_t& atid, xct_t* axct, const int axctid, 
                                    trx_result_tuple_t& presult,
                                    const stock_level_input_t& slin,
                                    baseActionsList& actions);

    final_stock_rvp* NewFinalStockRvp(const tid_t& atid, xct_t* axct, const int axctid, 
                                      trx_result_tuple_t& presult,
                                      baseActionsList& actions);

    r_dist_stock_action* NewRDistStockAction(const tid_t& atid, xct_t* axct, 
                                             mid1_stock_rvp* prvp,
                                             const stock_level_input_t& slin);

    r_ol_stock_action* NewROlStockAction(const tid_t& atid, xct_t* axct, 
                                         mid2_stock_rvp* prvp,
                                         const stock_level_input_t& slin, 
                                         const int nextid);

    r_st_stock_action* NewRStStockAction(const tid_t& atid, xct_t* axct, 
                                         rvp_t* prvp,
                                         const stock_level_input_t& slin, TwoIntVec* pvwi);



    // TPC-C Delivery
    mid1_del_rvp* NewMid1DelRvp(const tid_t& atid, xct_t* axct, const int axctid, 
                                final_del_rvp* frvp, const int d_id,
                                const delivery_input_t& din);

    mid2_del_rvp* NewMid2DelRvp(const tid_t& atid, xct_t* axct, const int axctid, 
                                final_del_rvp* frvp, const int d_id,
                                const delivery_input_t& din,
                                baseActionsList& actions);

    final_del_rvp* NewFinalDelRvp(const tid_t& atid, xct_t* axct, const int axctid, 
                                  trx_result_tuple_t& presult);

    del_nord_del_action* NewDelNordDelAction(const tid_t& atid, xct_t* axct, 
                                             mid1_del_rvp* prvp,
                                             const delivery_input_t& din,
                                             const int d_id);

    upd_ord_del_action* NewUpdOrdDelAction(const tid_t& atid, xct_t* axct, 
                                           mid2_del_rvp* prvp,
                                           const delivery_input_t& din,
                                           const int d_id, const int o_id);

    upd_oline_del_action* NewUpdOlineDelAction(const tid_t& atid, xct_t* axct, 
                                               mid2_del_rvp* prvp,
                                               const delivery_input_t& din,
                                               const int d_id, const int o_id);

    upd_cust_del_action* NewUpdCustDelAction(const tid_t& atid, xct_t* axct, 
                                             rvp_t* prvp,
                                             const delivery_input_t& din,
                                             const int d_id, const int c_id, const int amount);




    // TPC-C NewOrder
    midway_nord_rvp* NewMidwayNordRvp(const tid_t& atid, xct_t* axct, const int axctid, 
                                      final_nord_rvp* frvp, 
                                      const new_order_input_t& noin);

    final_nord_rvp* NewFinalNordRvp(const tid_t& atid, xct_t* axct, const int axctid, 
                                    trx_result_tuple_t& presult, const int ol_cnt);

    r_wh_nord_action* NewRWhNordAction(const tid_t& atid, xct_t* axct, 
                                       rvp_t* prvp,
                                       const int whid, const int did);

    r_cust_nord_action* NewRCustNordAction(const tid_t& atid, xct_t* axct, 
                                           rvp_t* prvp,
                                           const int whid, const int did, 
                                           const int cid);

    upd_dist_nord_action* NewUpdDistNordAction(const tid_t& atid, xct_t* axct, 
                                               midway_nord_rvp* pmidway_rvp,
                                               const int whid, const int did);

    r_item_nord_action* NewRItemNordAction(const tid_t& atid, xct_t* axct, 
                                           midway_nord_rvp* pmidway_rvp,
                                           const int whid, const int did,
                                           const int olidx);

    upd_sto_nord_action* NewUpdStoNordAction(const tid_t& atid, xct_t* axct, 
                                             midway_nord_rvp* pmidway_rvp,
                                             const int whid, const int did,
                                             const int olidx);

    ins_ord_nord_action* NewInsOrdNordAction(const tid_t& atid, xct_t* axct, 
                                             rvp_t* prvp,
                                             const int whid, const int did, 
                                             const int nextoid, const int cid, 
                                             const time_t tstamp, int olcnt, 
                                             int alllocal);

    ins_nord_nord_action* NewInsNordNordAction(const tid_t& atid, xct_t* axct, 
                                               rvp_t* prvp,
                                               const int whid, const int did, 
                                               const int nextoid);

    ins_ol_nord_action* NewInsOlNordAction(const tid_t& atid, xct_t* axct, 
                                           rvp_t* prvp,
                                           const int whid, const int did, 
                                           const int nextoid, const int olidx,
                                           const ol_item_info& iteminfo,
                                           time_t tstamp);


private:

    // algorithm for deciding the distribution of tables 
    const processorid_t _next_cpu(const processorid_t& aprd,
                                  const irpTableImpl* atable,
                                  const int step=DF_CPU_STEP_TABLES);



        
}; // EOF: DoraTPCCEnv



EXIT_NAMESPACE(dora);

#endif /** __DORA_TPCC_H */

