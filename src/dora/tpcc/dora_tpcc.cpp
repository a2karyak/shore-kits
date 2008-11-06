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



/******** Exported variables ********/

dora_tpcc_db* _g_dora;



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

const int dora_tpcc_db::start()
{
    TRACE( TRACE_DEBUG, "Creating tables...\n");
 
    // 1. Creates partitioned tables
    // 2. Add them to the vector
    // 3. Reset each table

    int range = _tpccenv->get_active_cpu_count();
    processorid_t icpu(0);
    int sf = _tpccenv->get_sf();

    // used for setting up the key ranges
    irpImplKey partDown;
    irpImplKey partUp;

    // we are doing the partitioning based on the number of warehouses
    int aboundary = 0;
    partDown.push_back(aboundary);
    partUp.push_back(sf);

    // WAREHOUSE
    _wh_irpt = new irpTableImpl(_tpccenv, _tpccenv->warehouse(), icpu, range, WH_IRP_KEY);
    if (!_wh_irpt) {
        TRACE( TRACE_ALWAYS, "Problem in creating (WAREHOUSE) irp-table\n");
        assert (0); // should not happen
        return (de_GEN_TABLE);
    }
    assert (_wh_irpt);    
    // creates SF partitions for warehouses
    for (int i=0; i<sf; i++) {
        _wh_irpt->create_one_part();
        partDown.reset();
        partDown.push_back(i);
        partUp.reset();
        aboundary=i+1;
        partUp.push_back(aboundary);
        _wh_irpt->get_part(i)->resize(partDown,partUp);
    }
    _irptp_vec.push_back(_wh_irpt);
    icpu = _next_cpu(icpu, _wh_irpt, sf);

    // DISTRICT
    _di_irpt = new irpTableImpl(_tpccenv, _tpccenv->district(), icpu, range, DI_IRP_KEY);
    if (!_di_irpt) {
        TRACE( TRACE_ALWAYS, "Problem in creating (DISTRICT) irp-table\n");
        assert (0); // should not happen
        return (de_GEN_TABLE);
    }
    assert (_di_irpt);
    // creates SF partitions for districts
    for (int i=0; i<sf; i++) {
        _di_irpt->create_one_part();
        partDown.reset();
        partDown.push_back(i);
        partUp.reset();
        aboundary=i+1;
        partUp.push_back(aboundary);
        _di_irpt->get_part(i)->resize(partDown,partUp);
    }
    _irptp_vec.push_back(_di_irpt);
    icpu = _next_cpu(icpu, _di_irpt, sf);    

    // HISTORY
    _hi_irpt = new irpTableImpl(_tpccenv, _tpccenv->history(), icpu, range, HI_IRP_KEY);
    if (!_hi_irpt) {
        TRACE( TRACE_ALWAYS, "Problem in creating (HISTORY) irp-table\n");
        assert (0); // should not happen
        return (de_GEN_TABLE);
    }
    assert (_hi_irpt);
    // creates SF partitions for districts
    for (int i=0; i<sf; i++) {
        _hi_irpt->create_one_part();
        partDown.reset();
        partDown.push_back(i);
        partUp.reset();
        aboundary=i+1;
        partUp.push_back(aboundary);
        _hi_irpt->get_part(i)->resize(partDown,partUp);
    }
    _irptp_vec.push_back(_hi_irpt);
    icpu = _next_cpu(icpu, _hi_irpt, sf);    


    // CUSTOMER
    _cu_irpt = new irpTableImpl(_tpccenv, _tpccenv->customer(), icpu, range, CU_IRP_KEY);
    if (!_cu_irpt) {
        TRACE( TRACE_ALWAYS, "Problem in creating (CUSTOMER) irp-table\n");
        assert (0); // should not happen
        return (de_GEN_TABLE);
    }
    assert (_cu_irpt);
    // creates SF partitions for customers
    for (int i=0; i<sf; i++) {
        _cu_irpt->create_one_part();
        partDown.reset();
        partDown.push_back(i);
        partUp.reset();
        aboundary=i+1;
        partUp.push_back(aboundary);
        _cu_irpt->get_part(i)->resize(partDown,partUp);
    }
    _irptp_vec.push_back(_cu_irpt);
    icpu = _next_cpu(icpu, _cu_irpt, sf);


    /*
    // NEW-ORDER
    _no_irpt = new irpTableImpl(_tpccenv, _tpccenv->new_order(), icpu, range, NO_IRP_KEY);
    if (!_no_irpt) {
        TRACE( TRACE_ALWAYS, "Problem in creating (NEW-ORDER) irp-table\n");
        assert (0); // should not happen
        return (de_GEN_TABLE);
    }
    assert (_no_irpt);
    _no_irpt->create_one_part();
    _irptp_vec.push_back(_no_irpt);
    icpu = _next_cpu(icpu, _no_irpt);    


    // ORDER
    _or_irpt = new irpTableImpl(_tpccenv, _tpccenv->order(), icpu, range, OR_IRP_KEY);
    if (!_or_irpt) {
        TRACE( TRACE_ALWAYS, "Problem in creating (ORDER) irp-table\n");
        assert (0); // should not happen
        return (de_GEN_TABLE);
    }
    assert (_or_irpt);
    _or_irpt->create_one_part();
    _irptp_vec.push_back(_or_irpt);
    icpu = _next_cpu(icpu, _or_irpt);    


    // ITEM
    _it_irpt = new irpTableImpl(_tpccenv, _tpccenv->item(), icpu, range, IT_IRP_KEY);
    if (!_hi_irpt) {
        TRACE( TRACE_ALWAYS, "Problem in creating (ITEM) irp-table\n");
        assert (0); // should not happen
        return (de_GEN_TABLE);
    }
    assert (_it_irpt);
    _it_irpt->create_one_part();
    _irptp_vec.push_back(_it_irpt);
    icpu = _next_cpu(icpu, _it_irpt);    


    // ORDER-LINE
    _ol_irpt = new irpTableImpl(_tpccenv, _tpccenv->orderline(), icpu, range, OL_IRP_KEY);
    if (!_ol_irpt) {
        TRACE( TRACE_ALWAYS, "Problem in creating (ORDER-LINE) irp-table\n");
        assert (0); // should not happen
        return (de_GEN_TABLE);
    }
    assert (_ol_irpt);
    _ol_irpt->create_one_part();
    _irptp_vec.push_back(_ol_irpt);
    icpu = _next_cpu(icpu, _ol_irpt);    


    // STOCK
    _st_irpt = new irpTableImpl(_tpccenv, _tpccenv->stock(), icpu, range, ST_IRP_KEY);
    if (!_st_irpt) {
        TRACE( TRACE_ALWAYS, "Problem in creating (STOCK) irp-table\n");
        assert (0); // should not happen
        return (de_GEN_TABLE);
    }
    assert (_st_irpt);
    _st_irpt->create_one_part();
    _irptp_vec.push_back(_st_irpt);
    icpu = _next_cpu(icpu, _st_irpt);    
    */

    TRACE( TRACE_ALWAYS, "Starting tables...\n");
    for (int i=0; i<_irptp_vec.size(); i++) {
        _irptp_vec[i]->reset();
    }

    return (0);
}



/****************************************************************** 
 *
 * @fn:    stop()
 *
 * @brief: Stops the DORA TPC-C
 *
 ******************************************************************/

const int dora_tpcc_db::stop()
{
    TRACE( TRACE_DEBUG, "Stopping...\n");
    for (int i=0; i<_irptp_vec.size(); i++) {
        _irptp_vec[i]->stop();
    }
    return (0);
}




/****************************************************************** 
 *
 * @fn:    resume()
 *
 * @brief: Resumes the DORA TPC-C
 *
 ******************************************************************/

const int dora_tpcc_db::resume()
{
    assert (0); // TODO (ip)
    return (0);
}



/****************************************************************** 
 *
 * @fn:    pause()
 *
 * @brief: Pauses the DORA TPC-C
 *
 ******************************************************************/

const int dora_tpcc_db::pause()
{
    assert (0); // TODO (ip)
    return (0);
}



/****************************************************************** 
 *
 * @fn:    dump()
 *
 * @brief: Dumps information about all the tables and partitions
 *
 ******************************************************************/

void dora_tpcc_db::dump() const
{
    for (int i=0; i<_irptp_vec.size(); i++) {
        _irptp_vec[i]->dump();
    }
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
 *         - _tpccenv->_max_cpu_count    - the maximum cpu count (hard-limit)
 *         - _tpccenv->_active_cpu_count - the active cpu count (soft-limit)
 *         - this->_start_prs_id     - the first assigned cpu for the table
 *         - this->_prs_range        - a range of cpus assigned for the table
 *         - _tpccenv->_sf               - the scaling factor of the tpcc-db
 *         - this->_vec.size()       - the number of tables
 *         - atable                  - the current table
 *
 ******************************************************************/

const processorid_t dora_tpcc_db::_next_cpu(const processorid_t aprd,
                                            const irpTableImpl* atable,
                                            const int step)
{
    processorid_t nextprs = ((aprd+step) % _tpccenv->get_active_cpu_count());
    TRACE( TRACE_DEBUG, "(%d) -> (%d)\n", aprd, nextprs);
    return (nextprs);
}









/////////////////////////////////////////////////////////////////////


dora_tpcc_db::dora_tpcc_db(ShoreTPCCEnv* tpccenv)
    : _tpccenv(tpccenv)
{
    assert (_tpccenv);

//     // initialize action caches
//     _upd_wh_pay_cache   = new upd_wh_pay_action_cache(DF_ACTION_CACHE_SZ);
//     _upd_dist_pay_cache = new upd_dist_pay_action_cache(DF_ACTION_CACHE_SZ);
//     _upd_cust_pay_cache = new upd_cust_pay_action_cache(DF_ACTION_CACHE_SZ);
//     _ins_hist_pay_cache = new ins_hist_pay_action_cache(DF_ACTION_CACHE_SZ);

//     _mb_wh_cache   = new upd_wh_mb_action_cache(DF_ACTION_CACHE_SZ);
//     _mb_cust_cache = new upd_cust_mb_action_cache(DF_ACTION_CACHE_SZ);
}


dora_tpcc_db::~dora_tpcc_db() 
{ 
//     // delete action caches
//     if (_upd_wh_pay_cache) {
//         // print_cache_stats();
//         delete (_upd_wh_pay_cache);
//         _upd_wh_pay_cache = NULL;
//     }
//     if (_upd_dist_pay_cache) {
//         // print_cache_stats();
//         delete (_upd_dist_pay_cache);
//         _upd_dist_pay_cache = NULL;
//     }
//     if (_upd_cust_pay_cache) {
//         // print_cache_stats();
//         delete (_upd_cust_pay_cache);
//         _upd_cust_pay_cache = NULL;
//     }
//     if (_ins_hist_pay_cache) {
//         // print_cache_stats();
//         delete (_ins_hist_pay_cache);
//         _ins_hist_pay_cache = NULL;
//     }


//     if (_mb_wh_cache) {
//         // print_cache_stats();
//         delete (_mb_wh_cache);
//         _mb_wh_cache = NULL;
//     }
//     if (_mb_cust_cache) {
//         // print_cache_stats();
//         delete (_mb_cust_cache);
//         _mb_cust_cache = NULL;
//     }
}    




/////////////////////////////////////////////////////////////////////



// // action cache related actions

// // returns the cache
// upd_wh_pay_action_cache* dora_tpcc_db::get_upd_wh_pay_cache() { 
//     assert (_upd_wh_pay_cache); 
//     return (_upd_wh_pay_cache); 
// }
// upd_dist_pay_action_cache* dora_tpcc_db::get_upd_dist_pay_cache() { 
//     assert (_upd_dist_pay_cache); 
//     return (_upd_dist_pay_cache); 
// }
// upd_cust_pay_action_cache* dora_tpcc_db::get_upd_cust_pay_cache() { 
//     assert (_upd_cust_pay_cache); 
//     return (_upd_cust_pay_cache); 
// }
// ins_hist_pay_action_cache* dora_tpcc_db::get_ins_hist_pay_cache() { 
//     assert (_ins_hist_pay_cache); 
//     return (_ins_hist_pay_cache); 
// }


// upd_wh_mb_action_cache* dora_tpcc_db::get_upd_wh_mb_action_cache() {
//     assert (_mb_wh_cache); 
//     return (_mb_wh_cache); 
// }
// upd_cust_mb_action_cache* dora_tpcc_db::get_upd_cust_mb_action_cache() {
//     assert (_mb_cust_cache); 
//     return (_mb_cust_cache); 
// }



//
// borrow and release an action
//
// @note: On borrowing, first it makes sure that it is empty
//
upd_wh_pay_action_impl* dora_tpcc_db::get_upd_wh_pay_action() {
    //         assert (_upd_wh_pay_cache);
    //         upd_wh_pay_action_impl* paction = _upd_wh_pay_cache->borrow();
    //         assert (paction);
    //         assert (paction->keys()->size()==0);
    upd_wh_pay_action_impl* paction = new upd_wh_pay_action_impl();
    return (paction);
}
void dora_tpcc_db::give_action(upd_wh_pay_action_impl* pirpa) {
    //assert (_upd_wh_pay_cache);
    //         _upd_wh_pay_cache->giveback(pirpa);
    assert (pirpa);
    delete (pirpa);
}

upd_dist_pay_action_impl* dora_tpcc_db::get_upd_dist_pay_action() {
    //assert (_upd_dist_pay_cache);
    //         upd_dist_pay_action_impl* paction = _upd_dist_pay_cache->borrow();
    //         assert (paction);
    //         assert (paction->keys()->size()==0);
    upd_dist_pay_action_impl* paction = new upd_dist_pay_action_impl();
    return (paction);
}
void dora_tpcc_db::give_action(upd_dist_pay_action_impl* pirpa) {
    //assert (_upd_dist_pay_cache);
    //         _upd_dist_pay_cache->giveback(pirpa);
    assert (pirpa);
    delete (pirpa);
}

upd_cust_pay_action_impl* dora_tpcc_db::get_upd_cust_pay_action() {
    //         assert (_upd_cust_pay_cache);
    //         upd_cust_pay_action_impl* paction = _upd_cust_pay_cache->borrow();
    //         assert (paction);
    //         assert (paction->keys()->size()==0);
    upd_cust_pay_action_impl* paction = new upd_cust_pay_action_impl();
    return (paction);
}
void dora_tpcc_db::give_action(upd_cust_pay_action_impl* pirpa) {
    //         assert (_upd_cust_pay_cache);
    //         _upd_cust_pay_cache->giveback(pirpa);
    assert (pirpa);
    delete (pirpa);
}

ins_hist_pay_action_impl* dora_tpcc_db::get_ins_hist_pay_action() {
    //         assert (_ins_hist_pay_cache);
    //         ins_hist_pay_action_impl* paction = _ins_hist_pay_cache->borrow();
    //         assert (paction);
    //         assert (paction->keys()->size()==0);
    ins_hist_pay_action_impl* paction = new ins_hist_pay_action_impl();
    return (paction);
}
void dora_tpcc_db::give_action(ins_hist_pay_action_impl* pirpa) {
    //assert (_ins_hist_pay_cache);
    //         _ins_hist_pay_cache->giveback(pirpa);
    assert (pirpa);
    delete (pirpa);
}



upd_wh_mb_action_impl* dora_tpcc_db::get_upd_wh_mb_action() {
    upd_wh_mb_action_impl* paction = new upd_wh_mb_action_impl();
    return (paction);
}
void dora_tpcc_db::give_action(upd_wh_mb_action_impl* pirpa) {
    assert (pirpa);
    delete (pirpa);
}
upd_cust_mb_action_impl* dora_tpcc_db::get_upd_cust_mb_action() {
    upd_cust_mb_action_impl* paction = new upd_cust_mb_action_impl();
    return (paction);
}
void dora_tpcc_db::give_action(upd_cust_mb_action_impl* pirpa) {
    assert (pirpa);
    delete (pirpa);
}



EXIT_NAMESPACE(dora);

