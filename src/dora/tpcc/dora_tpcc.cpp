/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file dora_tpcc.cpp
 *
 *  @brief Implementation of the DORA TPC-C class
 *
 *  @author Ippokratis Pandis, Oct 2008
 */


#include "dora/tpcc/dora_tpcc.h"

#include "dora/tpcc/dora_payment.h"
#include "dora/tpcc/dora_mbench.h"


using namespace dora;
using namespace shore;
using namespace tpcc;



ENTER_NAMESPACE(dora);



// max field counts for (int) keys of tpc-c tables
const int WH_IRP_KEY = 1;
const int DI_IRP_KEY = 2;
const int CU_IRP_KEY = 3;
const int HI_IRP_KEY = 6;
const int NO_IRP_KEY = 3;
const int OR_IRP_KEY = 4;
const int IT_IRP_KEY = 1;
const int OL_IRP_KEY = 4;
const int ST_IRP_KEY = 2;

// key estimations for each partition of the tpc-c tables
const int WH_KEY_EST = 100;
const int DI_KEY_EST = 1000;
const int CU_KEY_EST = 30000;
const int HI_KEY_EST = 100;
const int NO_KEY_EST = 3;
const int OR_KEY_EST = 4;
const int IT_KEY_EST = 1;
const int OL_KEY_EST = 4;
const int ST_KEY_EST = 2;



/****************************************************************** 
 *
 * @fn:    start()
 *
 * @brief: Starts the DORA TPC-C
 *
 * @note:  Creates a corresponding number of partitions per table.
 *         The decision about the number of partitions per table may 
 *         be based among others on:
 *         - _env->_sf : the database scaling factor
 *         - _env->_{max,active}_cpu_count: {hard,soft} cpu counts
 *
 ******************************************************************/

const int DoraTPCCEnv::start()
{
    // 1. Creates partitioned tables
    // 2. Adds them to the vector
    // 3. Resets each table

    conf(); // re-configure

    processorid_t icpu(_starting_cpu);

    TRACE( TRACE_STATISTICS, "Creating tables. SF=(%d)...\n", _sf);    

    // WAREHOUSE
    _wh_irpt = new irpTableImpl(this, warehouse(), icpu, _cpu_range, 
                                WH_IRP_KEY, WH_KEY_EST, _wh_per_part_wh, _sf);
    if (!_wh_irpt) {
        TRACE( TRACE_ALWAYS, "Problem in creating (WAREHOUSE) irp-table\n");
        assert (0); // should not happen
        return (de_GEN_TABLE);
    }
    _irptp_vec.push_back(_wh_irpt.get());
    icpu = _next_cpu(icpu, _wh_irpt, _cpu_table_step);


    // DISTRICT
    _di_irpt = new irpTableImpl(this, district(), icpu, _cpu_range, 
                                DI_IRP_KEY, DI_KEY_EST, _wh_per_part_dist, _sf);
    if (!_di_irpt) {
        TRACE( TRACE_ALWAYS, "Problem in creating (DISTRICT) irp-table\n");
        assert (0); // should not happen
        return (de_GEN_TABLE);
    }
    _irptp_vec.push_back(_di_irpt.get());
    icpu = _next_cpu(icpu, _di_irpt, _cpu_table_step);    


    // HISTORY
    _hi_irpt = new irpTableImpl(this, history(), icpu, _cpu_range, 
                                HI_IRP_KEY, HI_KEY_EST, _wh_per_part_hist, _sf);
    if (!_hi_irpt) {
        TRACE( TRACE_ALWAYS, "Problem in creating (HISTORY) irp-table\n");
        assert (0); // should not happen
        return (de_GEN_TABLE);
    }
    _irptp_vec.push_back(_hi_irpt.get());
    icpu = _next_cpu(icpu, _hi_irpt, _cpu_table_step);    


    // CUSTOMER
    _cu_irpt = new irpTableImpl(this, customer(), icpu, _cpu_range, 
                                CU_IRP_KEY, CU_KEY_EST, _wh_per_part_cust, _sf);
    if (!_cu_irpt) {
        TRACE( TRACE_ALWAYS, "Problem in creating (CUSTOMER) irp-table\n");
        assert (0); // should not happen
        return (de_GEN_TABLE);
    }
    _irptp_vec.push_back(_cu_irpt.get());
    icpu = _next_cpu(icpu, _cu_irpt, _cpu_table_step);

   
    // NEW-ORDER
    _no_irpt = new irpTableImpl(this, new_order(), icpu, _cpu_range, 
                                NO_IRP_KEY, NO_KEY_EST, _wh_per_part_nord, _sf);
    if (!_no_irpt) {
        TRACE( TRACE_ALWAYS, "Problem in creating (NEW-ORDER) irp-table\n");
        assert (0); // should not happen
        return (de_GEN_TABLE);
    }
    _irptp_vec.push_back(_no_irpt.get());
    icpu = _next_cpu(icpu, _no_irpt, _cpu_table_step);    


    // ORDER
    _or_irpt = new irpTableImpl(this, order(), icpu, _cpu_range, 
                                OR_IRP_KEY, OR_KEY_EST, _wh_per_part_ord, _sf);
    if (!_or_irpt) {
        TRACE( TRACE_ALWAYS, "Problem in creating (ORDER) irp-table\n");
        assert (0); // should not happen
        return (de_GEN_TABLE);
    }
    _irptp_vec.push_back(_or_irpt.get());
    icpu = _next_cpu(icpu, _or_irpt, _cpu_table_step);    


    // ITEM
    _it_irpt = new irpTableImpl(this, item(), icpu, _cpu_range, 
                                IT_IRP_KEY, IT_KEY_EST, _wh_per_part_item, _sf);
    if (!_hi_irpt) {
        TRACE( TRACE_ALWAYS, "Problem in creating (ITEM) irp-table\n");
        assert (0); // should not happen
        return (de_GEN_TABLE);
    }
    _irptp_vec.push_back(_it_irpt.get());
    icpu = _next_cpu(icpu, _it_irpt, _cpu_table_step);    


    // ORDER-LINE
    _ol_irpt = new irpTableImpl(this, orderline(), icpu, _cpu_range, 
                                OL_IRP_KEY, OL_KEY_EST, _wh_per_part_oline, _sf);
    if (!_ol_irpt) {
        TRACE( TRACE_ALWAYS, "Problem in creating (ORDER-LINE) irp-table\n");
        assert (0); // should not happen
        return (de_GEN_TABLE);
    }
    _irptp_vec.push_back(_ol_irpt.get());
    icpu = _next_cpu(icpu, _ol_irpt, _cpu_table_step);    


    // STOCK
    _st_irpt = new irpTableImpl(this, stock(), icpu, _cpu_range, 
                                ST_IRP_KEY, ST_KEY_EST, _wh_per_part_stock, _sf);
    if (!_st_irpt) {
        TRACE( TRACE_ALWAYS, "Problem in creating (STOCK) irp-table\n");
        assert (0); // should not happen
        return (de_GEN_TABLE);
    }
    assert (_st_irpt);
    _irptp_vec.push_back(_st_irpt.get());
    icpu = _next_cpu(icpu, _st_irpt, _cpu_table_step);    


    TRACE( TRACE_DEBUG, "Starting tables...\n");
    for (int i=0; i<_irptp_vec.size(); i++) {
        _irptp_vec[i]->reset();
    }

    set_dbc(DBC_ACTIVE);
    return (0);
}



/****************************************************************** 
 *
 * @fn:    stop()
 *
 * @brief: Stops the DORA TPC-C
 *
 ******************************************************************/

const int DoraTPCCEnv::stop()
{
    TRACE( TRACE_ALWAYS, "Stopping...\n");
    for (int i=0; i<_irptp_vec.size(); i++) {
        _irptp_vec[i]->stop();
        //delete (_irptp_vec[i]); // guard
    }
    _irptp_vec.clear();
    set_dbc(DBC_STOPPED);
    return (0);
}




/****************************************************************** 
 *
 * @fn:    resume()
 *
 * @brief: Resumes the DORA TPC-C
 *
 ******************************************************************/

const int DoraTPCCEnv::resume()
{
    assert (0); // TODO (ip)
    set_dbc(DBC_ACTIVE);
    return (0);
}



/****************************************************************** 
 *
 * @fn:    pause()
 *
 * @brief: Pauses the DORA TPC-C
 *
 ******************************************************************/

const int DoraTPCCEnv::pause()
{
    assert (0); // TODO (ip)
    set_dbc(DBC_PAUSED);
    return (0);
}



/****************************************************************** 
 *
 * @fn:    conf()
 *
 * @brief: Re-reads configuration
 *
 ******************************************************************/

const int DoraTPCCEnv::conf()
{
    ShoreTPCCEnv::conf();

    TRACE( TRACE_DEBUG, "configuring dora-tpcc\n");

    envVar* ev = envVar::instance();

    _starting_cpu = ev->getVarInt("dora-cpu-starting",DF_CPU_STEP_PARTITIONS);
    _cpu_table_step = ev->getVarInt("dora-cpu-table-step",DF_CPU_STEP_TABLES);

    _cpu_range = get_active_cpu_count();
    _sf = upd_sf();


    _wh_per_part_wh    = ev->getVarInt("dora-tpcc-wh-per-part-wh",0);
    _wh_per_part_dist  = ev->getVarInt("dora-tpcc-wh-per-part-dist",0);
    _wh_per_part_cust  = ev->getVarInt("dora-tpcc-wh-per-part-cust",0);
    _wh_per_part_hist  = ev->getVarInt("dora-tpcc-wh-per-part-hist",0);
    _wh_per_part_nord  = ev->getVarInt("dora-tpcc-wh-per-part-nord",0);
    _wh_per_part_ord   = ev->getVarInt("dora-tpcc-wh-per-part-ord",0);
    _wh_per_part_item  = ev->getVarInt("dora-tpcc-wh-per-part-item",0);
    _wh_per_part_oline = ev->getVarInt("dora-tpcc-wh-per-part-oline",0);
    _wh_per_part_stock = ev->getVarInt("dora-tpcc-wh-per-part-stock",0);
    

    return (0);
}





/****************************************************************** 
 *
 * @fn:    newrun()
 *
 * @brief: Prepares the DORA TPC-C DB for a new run
 *
 ******************************************************************/

const int DoraTPCCEnv::newrun()
{
    TRACE( TRACE_DEBUG, "Preparing for new run...\n");
    for (int i=0; i<_irptp_vec.size(); i++) {
        _irptp_vec[i]->prepareNewRun();
    }
    return (0);
}



/****************************************************************** 
 *
 * @fn:    set()
 *
 * @brief: Given a map of strings it updates the db environment
 *
 ******************************************************************/

const int DoraTPCCEnv::set(envVarMap* vars)
{
    TRACE( TRACE_DEBUG, "Reading set...\n");
    for (envVarConstIt cit = vars->begin(); cit != vars->end(); ++cit)
        TRACE( TRACE_DEBUG, "(%s) (%s)\n", 
               cit->first.c_str(), 
               cit->second.c_str());
    TRACE( TRACE_DEBUG, "*** unimplemented ***\n");
    return (0);
}


/****************************************************************** 
 *
 * @fn:    dump()
 *
 * @brief: Dumps information about all the tables and partitions
 *
 ******************************************************************/

const int DoraTPCCEnv::dump()
{
    int sz=_irptp_vec.size();
    TRACE( TRACE_ALWAYS, "Tables  = (%d)\n", sz);
    for (int i=0; i<sz; i++) {
        _irptp_vec[i]->dump();
    }
    return (0);
}


/****************************************************************** 
 *
 * @fn:    info()
 *
 * @brief: Information about the current state of DORA
 *
 ******************************************************************/

const int DoraTPCCEnv::info()
{
    TRACE( TRACE_ALWAYS, "SF      = (%d)\n", _scaling_factor);
    int sz=_irptp_vec.size();
    TRACE( TRACE_ALWAYS, "Tables  = (%d)\n", sz);
    for (int i=0;i<sz;++i) {
        _irptp_vec[i]->info();
    }
    return (0);
}



/******************************************************************** 
 *
 *  @fn:    statistics
 *
 *  @brief: Prints statistics for DORA 
 *
 ********************************************************************/

const int DoraTPCCEnv::statistics() 
{
    int sz=_irptp_vec.size();
    TRACE( TRACE_ALWAYS, "Tables  = (%d)\n", sz);
    for (int i=0;i<sz;++i) {
        _irptp_vec[i]->statistics();
    }
    return (0);
}





/****************************************************************** 
 *
 * @fn:    _next_cpu()
 *
 * @brief: Deciding the distribution of tables
 *
 * @note:  Very simple (just increases processor id by DF_CPU_STEP) 

 * @note:  This decision  can be based among others on:
 *
 *         - aprd                    - the current cpu
 *         - this->_max_cpu_count    - the maximum cpu count (hard-limit)
 *         - this->_active_cpu_count - the active cpu count (soft-limit)
 *         - this->_start_prs_id     - the first assigned cpu for the table
 *         - this->_prs_range        - a range of cpus assigned for the table
 *         - this->_sf               - the scaling factor of the tpcc-db
 *         - this->_vec.size()       - the number of tables
 *         - atable                  - the current table
 *
 ******************************************************************/

const processorid_t DoraTPCCEnv::_next_cpu(const processorid_t& aprd,
                                           const irpTableImpl* atable,
                                           const int step)
{    
    int binding = envVar::instance()->getVarInt("dora-cpu-binding",0);
    if (binding==0)
        return (PBIND_NONE);

    processorid_t nextprs = ((aprd+step) % this->get_active_cpu_count());
    //TRACE( TRACE_DEBUG, "(%d) -> (%d)\n", aprd, nextprs);
    return (nextprs);
}






/******************************************************************** 
 *
 *  Thread-local action and rvp object caches
 *
 ********************************************************************/


DECLARE_TLS_RVP_CACHE(final_pay_rvp);
DECLARE_TLS_RVP_CACHE(midway_pay_rvp);


DECLARE_TLS_ACTION_CACHE(upd_wh_pay_action,int);
DECLARE_TLS_ACTION_CACHE(upd_dist_pay_action,int);
DECLARE_TLS_ACTION_CACHE(upd_cust_pay_action,int);
DECLARE_TLS_ACTION_CACHE(ins_hist_pay_action,int);

     


// TPC-C Payment RVPs

final_pay_rvp* 
DoraTPCCEnv::NewFinalPayRvp(const tid_t& atid, xct_t* axct, const int axctid, 
                            trx_result_tuple_t& presult,
                            baseActionsList& actions)
{
    final_pay_rvp* myrvp = my_final_pay_rvp_cache->_cache->borrow();
    assert (myrvp);
    myrvp->set(atid,axct,axctid,presult,this,my_final_pay_rvp_cache->_cache.get());
    myrvp->copy_actions(actions);
    return (myrvp);    
}


midway_pay_rvp*  
DoraTPCCEnv::NewMidayPayRvp(const tid_t& atid, xct_t* axct, const int axctid,
                            trx_result_tuple_t& presult,
                            const payment_input_t& pin)
{
    midway_pay_rvp* myrvp = my_midway_pay_rvp_cache->_cache->borrow();
    assert (myrvp);
    myrvp->set(atid,axct,axctid,presult,pin,this,my_midway_pay_rvp_cache->_cache.get());
    return (myrvp);    
}


// TPC-C Payment Actions

upd_wh_pay_action*  
DoraTPCCEnv::NewUpdWhPayAction(const tid_t& atid, xct_t* axct, midway_pay_rvp* prvp,
                               const payment_input_t& pin)
{
    upd_wh_pay_action* myaction = my_upd_wh_pay_action_cache->_cache->borrow();
    assert (myaction);
    myaction->set(atid,axct,prvp,pin,this,my_upd_wh_pay_action_cache->_cache.get());
    prvp->add_action(myaction);
    return (myaction);        
}


upd_dist_pay_action*  
DoraTPCCEnv::NewUpdDistPayAction(const tid_t& atid, xct_t* axct, midway_pay_rvp* prvp,
                                 const payment_input_t& pin)
{
    upd_dist_pay_action* myaction = my_upd_dist_pay_action_cache->_cache->borrow();
    assert (myaction);
    myaction->set(atid,axct,prvp,pin,this,my_upd_dist_pay_action_cache->_cache.get());
    prvp->add_action(myaction);
    return (myaction);            
}



upd_cust_pay_action*  
DoraTPCCEnv::NewUpdCustPayAction(const tid_t& atid, xct_t* axct, midway_pay_rvp* prvp,
                                 const payment_input_t& pin)
{
    upd_cust_pay_action* myaction = my_upd_cust_pay_action_cache->_cache->borrow();
    assert (myaction);
    myaction->set(atid,axct,prvp,pin,this,my_upd_cust_pay_action_cache->_cache.get());
    prvp->add_action(myaction);
    return (myaction);        
}


ins_hist_pay_action*  
DoraTPCCEnv::NewInsHistPayAction(const tid_t& atid, xct_t* axct, rvp_t* prvp,
                                 const payment_input_t& pin,
                                 const tpcc_warehouse_tuple& awh,
                                 const tpcc_district_tuple& adist)
{
    ins_hist_pay_action* myaction = my_ins_hist_pay_action_cache->_cache->borrow();
    assert (myaction);
    myaction->set(atid,axct,prvp,pin,awh,adist,this,my_ins_hist_pay_action_cache->_cache.get());
    prvp->add_action(myaction);
    return (myaction);    
}


///////////////////////////////////////////////////////////////////////////////////////

// MBenches RVP //



DECLARE_TLS_RVP_CACHE(final_mb_rvp);

DECLARE_TLS_ACTION_CACHE(upd_wh_mb_action,int);
DECLARE_TLS_ACTION_CACHE(upd_cust_mb_action,int);


final_mb_rvp*  
DoraTPCCEnv::NewFinalMbRvp(const tid_t& atid, xct_t* axct, const int axctid, 
                           trx_result_tuple_t& presult)
{
    final_mb_rvp* myrvp = my_final_mb_rvp_cache->_cache->borrow();
    assert (myrvp);
    myrvp->set(atid,axct,axctid,presult,1,1,this,my_final_mb_rvp_cache->_cache.get());
    return (myrvp);    
}



// MBenches Actions

upd_wh_mb_action*  
DoraTPCCEnv::NewUpdWhMbAction(const tid_t& atid, xct_t* axct, rvp_t* prvp,
                              const int whid)
{
    upd_wh_mb_action* myaction = my_upd_wh_mb_action_cache->_cache->borrow();
    assert (myaction);
    myaction->set(atid,axct,prvp,whid,this,my_upd_wh_mb_action_cache->_cache.get());
    prvp->add_action(myaction);
    return (myaction);    
}


upd_cust_mb_action*  
DoraTPCCEnv::NewUpdCustMbAction(const tid_t& atid, xct_t* axct, rvp_t* prvp,
                                const int whid)
{
    upd_cust_mb_action* myaction = my_upd_cust_mb_action_cache->_cache->borrow();
    assert (myaction);
    myaction->set(atid,axct,prvp,whid,this,my_upd_cust_mb_action_cache->_cache.get());
    prvp->add_action(myaction);
    return (myaction);    
}



EXIT_NAMESPACE(dora);

