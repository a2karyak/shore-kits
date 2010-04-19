/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   dora_tpcc.cpp
 *
 *  @brief:  Implementation of the DORA TPC-C class
 *
 *  @author: Ippokratis Pandis, Oct 2008
 */

#include "tls.h"

#include "dora/tpcc/dora_tpcc.h"

#include "dora/tpcc/dora_mbench.h"
#include "dora/tpcc/dora_payment.h"
#include "dora/tpcc/dora_order_status.h"
#include "dora/tpcc/dora_stock_level.h"
#include "dora/tpcc/dora_delivery.h"
#include "dora/tpcc/dora_new_order.h"

using namespace shore;
using namespace tpcc;


ENTER_NAMESPACE(dora);



// max field counts for (int) keys of tpc-c tables
const int whs_IRP_KEY = 1;
const int dis_IRP_KEY = 2;
const int cus_IRP_KEY = 3;
const int his_IRP_KEY = 6;
const int nor_IRP_KEY = 3;
const int ord_IRP_KEY = 4;
const int ite_IRP_KEY = 1;
const int oli_IRP_KEY = 4;
const int sto_IRP_KEY = 2;

// key estimations for each partition of the tpc-c tables
const int whs_KEY_EST = 100;
const int dis_KEY_EST = 1000;
const int cus_KEY_EST = 1000;
const int his_KEY_EST = 1000;
const int nor_KEY_EST = 1000;
const int ord_KEY_EST = 1000;
const int ite_KEY_EST = 1000;
const int oli_KEY_EST = 1000;
const int sto_KEY_EST = 1000;



/****************************************************************** 
 *
 * @fn:    construction/destruction
 *
 * @brief: If configured, it creates and starts the flusher 
 *
 ******************************************************************/
    
DoraTPCCEnv::DoraTPCCEnv(string confname)
    : ShoreTPCCEnv(confname)
{ 
}

DoraTPCCEnv::~DoraTPCCEnv() 
{ 
    stop();
}


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

int DoraTPCCEnv::start()
{
    // 1. Creates partitioned tables
    // 2. Adds them to the vector
    // 3. Resets each table

    conf(); // re-configure

    // Call the pre-start procedure of the dora environment
    DoraEnv::_start(this);

    processorid_t icpu(_starting_cpu);

    TRACE( TRACE_STATISTICS, "Creating tables. SF=(%d)...\n", _sf);    


    // WAREHOUSE
    GENERATE_DORA_PARTS(whs,warehouse);


    // DISTRICT
    GENERATE_DORA_PARTS(dis,district);


    // HISTORY
    GENERATE_DORA_PARTS(his,history);


    // CUSTOMER
    GENERATE_DORA_PARTS(cus,customer);

   
    // NEW-ORDER
    GENERATE_DORA_PARTS(nor,new_order);


    // ORDER
    GENERATE_DORA_PARTS(ord,order);


    // ITEM
    GENERATE_DORA_PARTS(ite,item);


    // ORDER-LINE
    GENERATE_DORA_PARTS(oli,order_line);


    // STOCK
    GENERATE_DORA_PARTS(sto,stock);


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

int DoraTPCCEnv::stop()
{
    TRACE( TRACE_ALWAYS, "Stopping...\n");
    for (int i=0; i<_irptp_vec.size(); i++) {
        _irptp_vec[i]->stop();
    }
    _irptp_vec.clear();

    // Call the meta-stop procedure of the dora environment
    DoraEnv::_stop();

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

int DoraTPCCEnv::resume()
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

int DoraTPCCEnv::pause()
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

int DoraTPCCEnv::conf()
{
    ShoreTPCCEnv::conf();

    TRACE( TRACE_DEBUG, "configuring dora-tpcc\n");

    envVar* ev = envVar::instance();

    // Get CPU and binding configuration
    _starting_cpu = ev->getVarInt("dora-cpu-starting",DF_CPU_STEP_PARTITIONS);
    _cpu_table_step = ev->getVarInt("dora-cpu-table-step",DF_CPU_STEP_TABLES);

    _cpu_range = get_active_cpu_count();
    _sf = upd_sf();

    // Get DORA and per table partition SFs
    _dora_sf = ev->getSysVarInt("dora-sf");
    assert (_dora_sf);

    _sf_per_part_whs = _dora_sf * ev->getVarInt("dora-tpcc-wh-per-part-wh",1);
    _sf_per_part_dis = _dora_sf * ev->getVarInt("dora-tpcc-wh-per-part-dist",1);
    _sf_per_part_cus = _dora_sf * ev->getVarInt("dora-tpcc-wh-per-part-cust",1);
    _sf_per_part_his = _dora_sf * ev->getVarInt("dora-tpcc-wh-per-part-hist",1);
    _sf_per_part_nor = _dora_sf * ev->getVarInt("dora-tpcc-wh-per-part-nord",1);
    _sf_per_part_ord = _dora_sf * ev->getVarInt("dora-tpcc-wh-per-part-ord",1);
    _sf_per_part_ite = _dora_sf * ev->getVarInt("dora-tpcc-wh-per-part-item",1);
    _sf_per_part_oli = _dora_sf * ev->getVarInt("dora-tpcc-wh-per-part-oline",1);
    _sf_per_part_sto = _dora_sf * ev->getVarInt("dora-tpcc-wh-per-part-stock",1);
    
    return (0);
}





/****************************************************************** 
 *
 * @fn:    newrun()
 *
 * @brief: Prepares the DORA TPC-C DB for a new run
 *
 ******************************************************************/

int DoraTPCCEnv::newrun()
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

int DoraTPCCEnv::set(envVarMap* vars)
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

int DoraTPCCEnv::dump()
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

int DoraTPCCEnv::info()
{
    TRACE( TRACE_ALWAYS, "SF      = (%.1f)\n", _scaling_factor);
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

int DoraTPCCEnv::statistics() 
{
    int sz=_irptp_vec.size();
    TRACE( TRACE_ALWAYS, "Tables  = (%d)\n", sz);
    for (int i=0;i<sz;++i) {
        _irptp_vec[i]->statistics();
    }

    // first print any parent statistics
    ShoreTPCCEnv::statistics();

    return (0);
}




/******************************************************************** 
 *
 *  Thread-local action and rvp object caches
 *
 ********************************************************************/


// MBenches RVP //

DEFINE_DORA_FINAL_RVP_GEN_FUNC(final_mb_rvp,DoraTPCCEnv);

DEFINE_DORA_ACTION_GEN_FUNC(upd_wh_mb_action,rvp_t,mbench_wh_input_t,int,DoraTPCCEnv);
DEFINE_DORA_ACTION_GEN_FUNC(upd_cust_mb_action,rvp_t,mbench_cust_input_t,int,DoraTPCCEnv);


///////////////////////////////////////////////////////////////////////////////////////

// TPC-C PAYMENT


DEFINE_DORA_MIDWAY_RVP_GEN_FUNC(midway_pay_rvp,payment_input_t,DoraTPCCEnv);
DEFINE_DORA_FINAL_RVP_WITH_PREV_GEN_FUNC(final_pay_rvp,DoraTPCCEnv);


DEFINE_DORA_ACTION_GEN_FUNC(upd_wh_pay_action,midway_pay_rvp,payment_input_t,int,DoraTPCCEnv);
DEFINE_DORA_ACTION_GEN_FUNC(upd_dist_pay_action,midway_pay_rvp,payment_input_t,int,DoraTPCCEnv);
DEFINE_DORA_ACTION_GEN_FUNC(upd_cust_pay_action,midway_pay_rvp,payment_input_t,int,DoraTPCCEnv);

DEFINE_DORA_ACTION_GEN_FUNC(ins_hist_pay_action,rvp_t,payment_input_t,int,DoraTPCCEnv);
//const tpcc_warehouse_tuple& awh
//const tpcc_district_tuple& adist


///////////////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////////////////

// TPC-C ORDER STATUS

DEFINE_DORA_MIDWAY_RVP_GEN_FUNC(mid1_ordst_rvp,order_status_input_t,DoraTPCCEnv);

DEFINE_DORA_MIDWAY_RVP_WITH_PREV_GEN_FUNC(mid2_ordst_rvp,order_status_input_t,DoraTPCCEnv);

DEFINE_DORA_FINAL_RVP_WITH_PREV_GEN_FUNC(final_ordst_rvp,DoraTPCCEnv);

DEFINE_DORA_ACTION_GEN_FUNC(r_cust_ordst_action,mid1_ordst_rvp,order_status_input_t,int,DoraTPCCEnv);
DEFINE_DORA_ACTION_GEN_FUNC(r_ord_ordst_action,mid2_ordst_rvp,order_status_input_t,int,DoraTPCCEnv);

DEFINE_DORA_ACTION_GEN_FUNC(r_ol_ordst_action,rvp_t,order_status_input_t,int,DoraTPCCEnv);



///////////////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////////////////

// TPC-C STOCK LEVEL

DEFINE_DORA_MIDWAY_RVP_GEN_FUNC(mid1_stock_rvp,stock_level_input_t,DoraTPCCEnv);
DEFINE_DORA_MIDWAY_RVP_WITH_PREV_GEN_FUNC(mid2_stock_rvp,stock_level_input_t,DoraTPCCEnv);

DEFINE_DORA_FINAL_RVP_WITH_PREV_GEN_FUNC(final_stock_rvp,DoraTPCCEnv);


DEFINE_DORA_ACTION_GEN_FUNC(r_dist_stock_action,mid1_stock_rvp,stock_level_input_t,int,DoraTPCCEnv);
DEFINE_DORA_ACTION_GEN_FUNC(r_ol_stock_action,mid2_stock_rvp,stock_level_input_t,int,DoraTPCCEnv);

DEFINE_DORA_ACTION_GEN_FUNC(r_st_stock_action,rvp_t,stock_level_input_t,int,DoraTPCCEnv);



///////////////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////////////////

// TPC-C DELIVERY

DEFINE_DORA_MIDWAY_RVP_GEN_FUNC(mid1_del_rvp,delivery_input_t,DoraTPCCEnv);
//final_del_rvp* frvp
//const int d_id

DEFINE_DORA_MIDWAY_RVP_WITH_PREV_GEN_FUNC(mid2_del_rvp,delivery_input_t,DoraTPCCEnv);
//final_del_rvp* frvp
//const int d_id

DEFINE_DORA_FINAL_RVP_GEN_FUNC(final_del_rvp,DoraTPCCEnv);


DEFINE_DORA_ACTION_GEN_FUNC(del_nord_del_action,mid1_del_rvp,delivery_input_t,int,DoraTPCCEnv);
//const int d_id

DEFINE_DORA_ACTION_GEN_FUNC(upd_ord_del_action,mid2_del_rvp,delivery_input_t,int,DoraTPCCEnv);
//const int d_id
//const int o_id

DEFINE_DORA_ACTION_GEN_FUNC(upd_oline_del_action,mid2_del_rvp,delivery_input_t,int,DoraTPCCEnv);
//const int d_id
//const int o_id

DEFINE_DORA_ACTION_GEN_FUNC(upd_cust_del_action,rvp_t,delivery_input_t,int,DoraTPCCEnv);
//const int d_id
//const int o_id
//const int amount


///////////////////////////////////////////////////////////////////////////////////////




///////////////////////////////////////////////////////////////////////////////////////

// TPC-C NEWORDER

// RVP
DEFINE_DORA_MIDWAY_RVP_GEN_FUNC(mid_nord_rvp,new_order_input_t,DoraTPCCEnv);

DEFINE_DORA_FINAL_RVP_WITH_PREV_GEN_FUNC(final_nord_rvp,DoraTPCCEnv);


// Start -> Midway
DEFINE_DORA_ACTION_GEN_FUNC(r_wh_nord_action,mid_nord_rvp,no_item_nord_input_t,int,DoraTPCCEnv);

DEFINE_DORA_ACTION_GEN_FUNC(upd_dist_nord_action,mid_nord_rvp,no_item_nord_input_t,int,DoraTPCCEnv);

DEFINE_DORA_ACTION_GEN_FUNC(r_cust_nord_action,mid_nord_rvp,no_item_nord_input_t,int,DoraTPCCEnv);

DEFINE_DORA_ACTION_GEN_FUNC(r_item_nord_action,mid_nord_rvp,new_order_input_t,int,DoraTPCCEnv);

DEFINE_DORA_ACTION_GEN_FUNC(upd_sto_nord_action,mid_nord_rvp,new_order_input_t,int,DoraTPCCEnv);


// Midway -> Final
DEFINE_DORA_ACTION_GEN_FUNC(ins_ord_nord_action,rvp_t,no_item_nord_input_t,int,DoraTPCCEnv);

DEFINE_DORA_ACTION_GEN_FUNC(ins_nord_nord_action,rvp_t,no_item_nord_input_t,int,DoraTPCCEnv);

DEFINE_DORA_ACTION_GEN_FUNC(ins_ol_nord_action,rvp_t,new_order_input_t,int,DoraTPCCEnv);


///////////////////////////////////////////////////////////////////////////////////////




EXIT_NAMESPACE(dora);

