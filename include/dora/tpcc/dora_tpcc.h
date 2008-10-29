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

// #include "dora/tpcc/dora_payment.h"
// #include "dora/tpcc/dora_mbench.h"

#include "stages/tpcc/shore/shore_tpcc_env.h"


ENTER_NAMESPACE(dora);


using namespace shore;
using namespace tpcc;



/******** Exported variables ********/

class dora_tpcc_db;
extern dora_tpcc_db* _g_dora;

/******** EOF Exported variables ********/

const int DF_ACTION_CACHE_SZ = 100;


// Forward declarations
class upd_wh_pay_action_impl;
class upd_dist_pay_action_impl;
class upd_cust_pay_action_impl;
class ins_hist_pay_action_impl;    
typedef action_cache_t<upd_wh_pay_action_impl>   upd_wh_pay_action_cache;
typedef action_cache_t<upd_dist_pay_action_impl> upd_dist_pay_action_cache;
typedef action_cache_t<upd_cust_pay_action_impl> upd_cust_pay_action_cache;
typedef action_cache_t<ins_hist_pay_action_impl> ins_hist_pay_action_cache;

class upd_wh_mb_action_impl;
class upd_cust_mb_action_impl;
typedef action_cache_t<upd_wh_mb_action_impl> upd_wh_mb_action_cache;
typedef action_cache_t<upd_cust_mb_action_impl> upd_cust_mb_action_cache;



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

//     // caches of actions 
//     // one cache per action of each of the supported trxs
//     upd_wh_pay_action_cache*   _upd_wh_pay_cache;        /* pointer to a upd wh payment action cache */
//     upd_dist_pay_action_cache* _upd_dist_pay_cache;      /* pointer to a upd dist payment action cache */
//     upd_cust_pay_action_cache* _upd_cust_pay_cache;      /* pointer to a upd cust payment action cache */
//     ins_hist_pay_action_cache* _ins_hist_pay_cache;      /* pointer to a ins hist payment action cache */

//     upd_wh_mb_action_cache* _mb_wh_cache;
//     upd_cust_mb_action_cache* _mb_cust_cache;

public:
    

    dora_tpcc_db(ShoreTPCCEnv* tpccenv);
    ~dora_tpcc_db();

    /** Partition-related methods */

    inline irp_impl* get_part(const table_pos, const int part_pos) {
        assert (table_pos<_irptp_vec.size());        
        return (_irptp_vec[table_pos]->get_part(part_pos));
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



    /** action cache related actions */

//     // returns the cache
//     upd_wh_pay_action_cache* get_upd_wh_pay_cache();
//     upd_dist_pay_action_cache* get_upd_dist_pay_cache();
//     upd_cust_pay_action_cache* get_upd_cust_pay_cache();
//     ins_hist_pay_action_cache* get_ins_hist_pay_cache();

//     upd_wh_mb_action_cache* get_upd_wh_mb_action_cache();
//     upd_cust_mb_action_cache* get_upd_cust_mb_action_cache();


    // borrow and release an action
    upd_wh_pay_action_impl* get_upd_wh_pay_action();
    void give_action(upd_wh_pay_action_impl* pirpa);
    upd_dist_pay_action_impl* get_upd_dist_pay_action();
    void give_action(upd_dist_pay_action_impl* pirpa);
    upd_cust_pay_action_impl* get_upd_cust_pay_action();
    void give_action(upd_cust_pay_action_impl* pirpa);
    ins_hist_pay_action_impl* get_ins_hist_pay_action();
    void give_action(ins_hist_pay_action_impl* pirpa);

    upd_wh_mb_action_impl* get_upd_wh_mb_action();
    void give_action(upd_wh_mb_action_impl* pirpa);
    upd_cust_mb_action_impl* get_upd_cust_mb_action();
    void give_action(upd_cust_mb_action_impl* pirpa);



private:

    // algorithm for deciding the distribution of tables 
    const processorid_t _next_cpu(const processorid_t aprd,
                                  const irp_table_impl* atable,
                                  const int step=DF_CPU_STEP_TABLES);

        
}; // EOF: dora_tpcc_db



EXIT_NAMESPACE(dora);

#endif /** __DORA_TPCC_H */

