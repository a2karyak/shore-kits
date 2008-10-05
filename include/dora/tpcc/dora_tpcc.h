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
    typedef irp_impl::part_action       iaction;
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


public:

    dora_tpcc_db(ShoreTPCCEnv* tpccenv)
        : _tpccenv(tpccenv)
    {
        assert (_tpccenv);
    }

    ~dora_tpcc_db() { }    


    /** Access methods */

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

    // {Start/Stop} the system 
    const int start();
    const int stop();

    
    /** Client API */
    
    // enqueues action, false on error
    inline const int enqueue(iaction* paction, 
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

