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

#include "stages/tpcc/shore/shore_tpcc_env.h"

#include "dora.h"


ENTER_NAMESPACE(dora);


using namespace shore;
using namespace tpcc;



/******** Exported variables ********/

class dora_tpcc_db;
extern dora_tpcc_db* _g_dora;

/******** EOF Exported variables ********/

const int DF_ACTION_CACHE_SZ = 100;


// // Forward declarations

// // TPC-C PAYMENT
// class midway_pay_rvp;
// class final_pay_rvp;
// class upd_wh_pay_action_impl;
// class upd_dist_pay_action_impl;
// class upd_cust_pay_action_impl;
// class ins_hist_pay_action_impl;    


// // MBenches
// class final_mb_rvp;
// class upd_wh_mb_action_impl;
// class upd_cust_mb_action_impl;



/******************************************************************** 
 *
 * @class: dora_tpcc
 *
 * @brief: Container class for all the data partitions for the TPC-C database
 *
 ********************************************************************/

class dora_tpcc_db : public db_iface
{
public:

    typedef range_partition_impl<int>   irpImpl; 
    typedef range_part_table_impl<int>  irpTableImpl;

    typedef irpImpl::RangeAction      irpAction;
    typedef irpImpl::PartKey          irpImplKey;

    typedef vector<irpTableImpl*>       irpTablePtrVector;
    typedef irpTablePtrVector::iterator irpTablePtrVectorIt;

private:

    // the shore env
    ShoreTPCCEnv* _tpccenv;    

    // a vector of pointers to integer-range-partitioned tables
    irpTablePtrVector _irptp_vec;    

    // the list of pointer to irp-tables
    irpTableImpl* _wh_irpt; // WAREHOUSE
    irpTableImpl* _di_irpt; // DISTRICT
    irpTableImpl* _cu_irpt; // CUSTOMER
    irpTableImpl* _hi_irpt; // HISTORY
    irpTableImpl* _no_irpt; // NEW-ORDER
    irpTableImpl* _or_irpt; // ORDER
    irpTableImpl* _it_irpt; // ITEM
    irpTableImpl* _ol_irpt; // ORDER-LINE
    irpTableImpl* _st_irpt; // STOCK

public:
    

    dora_tpcc_db(ShoreTPCCEnv* tpccenv)
        : db_iface(), _tpccenv(tpccenv)
    {
        assert (_tpccenv);
    }
    ~dora_tpcc_db() { 
        if (dbc()!=DBC_STOPPED) stop();
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
    irpTableImpl* whs() const { return (_wh_irpt); }
    irpTableImpl* dis() const { return (_di_irpt); }
    irpTableImpl* cus() const { return (_cu_irpt); }
    irpTableImpl* his() const { return (_hi_irpt); }
    irpTableImpl* nor() const { return (_no_irpt); }
    irpTableImpl* ord() const { return (_or_irpt); }
    irpTableImpl* ite() const { return (_it_irpt); }
    irpTableImpl* oli() const { return (_ol_irpt); }
    irpTableImpl* sto() const { return (_st_irpt); }

    // Access specific partitions
    irpImpl* whs(const int pos) const { return (_wh_irpt->get_part(pos)); }
    irpImpl* dis(const int pos) const { return (_di_irpt->get_part(pos)); }
    irpImpl* cus(const int pos) const { return (_cu_irpt->get_part(pos)); }
    irpImpl* his(const int pos) const { return (_hi_irpt->get_part(pos)); }
    irpImpl* nor(const int pos) const { return (_no_irpt->get_part(pos)); }
    irpImpl* ord(const int pos) const { return (_or_irpt->get_part(pos)); }
    irpImpl* ite(const int pos) const { return (_it_irpt->get_part(pos)); }
    irpImpl* oli(const int pos) const { return (_ol_irpt->get_part(pos)); }
    irpImpl* sto(const int pos) const { return (_st_irpt->get_part(pos)); }



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


private:

    // algorithm for deciding the distribution of tables 
    const processorid_t _next_cpu(const processorid_t aprd,
                                  const irpTableImpl* atable,
                                  const int step=DF_CPU_STEP_TABLES);

        
}; // EOF: dora_tpcc_db



EXIT_NAMESPACE(dora);

#endif /** __DORA_TPCC_H */

