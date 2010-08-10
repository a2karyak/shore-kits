/* -*- mode:C++; c-basic-offset:4 -*-
     Shore-kits -- Benchmark implementations for Shore-MT
   
                       Copyright (c) 2007-2009
      Data Intensive Applications and Systems Labaratory (DIAS)
               Ecole Polytechnique Federale de Lausanne
   
                         All Rights Reserved.
   
   Permission to use, copy, modify and distribute this software and
   its documentation is hereby granted, provided that both the
   copyright notice and this permission notice appear in all copies of
   the software, derivative works or modified versions, and any
   portions thereof, and that both notices appear in supporting
   documentation.
   
   This code is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. THE AUTHORS
   DISCLAIM ANY LIABILITY OF ANY KIND FOR ANY DAMAGES WHATSOEVER
   RESULTING FROM THE USE OF THIS SOFTWARE.
*/

/** @file:   dora_env.h
 *
 *  @brief:  A generic Dora environment class
 *
 *  @note:   All the database need to inherit both from a shore_env, as well as, a dora_env
 *
 *  @author: Ippokratis Pandis, Jun 2009
 */


#ifndef __DORA_ENV_H
#define __DORA_ENV_H


#include <cstdio>

#include "tls.h"
#include "util.h"
#include "shore.h"

#include "dora/range_table_i.h"

#ifdef CFG_FLUSHER
#include "dora/dflusher.h"
#endif

using namespace shore;

ENTER_NAMESPACE(dora);


/******************************************************************** 
 *
 * @class: dora_env
 *
 * @brief: Generic container class for all the data partitions for
 *         DORA databases. 
 *
 * @note:  All the DORA databases so far use range partitioning over a 
 *         single integer (the SF number) as the identifier. This version 
 *         of DoraEnv is customized for this. That is, the partitioning 
 *         is only range, and DataType = int.
 *
 ********************************************************************/

class DoraEnv
{
public:

    typedef range_partition_i<int>      irpImpl; 
    typedef range_table_i<int>          irpTableImpl;

    typedef irpImpl::RangeAction        irpAction;

    typedef std::vector<irpTableImpl*>  irpTablePtrVector;
    typedef irpTablePtrVector::iterator irpTablePtrVectorIt;

    typedef vector<base_action_t*>      baseActionsList;

protected:

    // A vector of pointers to integer-range-partitioned tables
    irpTablePtrVector _irptp_vec;    

    // Setup variables
    int _starting_cpu;
    int _cpu_table_step;
    int _cpu_range;
    int _sf;

    int _dora_sf;

#ifdef CFG_FLUSHER
    // The dora-flusher thread
    guard<dora_flusher_t> _flusher;
#endif

public:
    
    DoraEnv();

    virtual ~DoraEnv();

    //// Client API
    
    // Enqueues action, non zero on error
    inline int enqueue(irpAction* paction,
                       const bool bWake, 
                       irpTableImpl* ptable, 
                       const int part_pos) 
    {
        assert (paction);
        assert (ptable);
        return (ptable->enqueue(paction, bWake, part_pos));
    }

    // Return the partition responsible for the specific integer identifier
    inline irpImpl* decide_part(irpTableImpl* atable, const int aid) {
        cvec_t acv(&aid,sizeof(int));
        return (atable->getPartByCVKey(acv));
    }      


#ifdef CFG_FLUSHER
    inline void enqueue_toflush(terminal_rvp_t* arvp) {
        assert (_flusher.get());
        _flusher->enqueue_toflush(arvp);
    }
#endif

    //// Partition-related methods

    irpImpl* table_part(const uint table_pos, const uint part_pos);


protected:

    int _post_start(ShoreEnv* penv);
    int _post_stop(ShoreEnv* penv);
    int _newrun(ShoreEnv* penv);
    int _dump(ShoreEnv* penv);
    int _info(const ShoreEnv* penv) const;
    int _statistics(ShoreEnv* penv);

    // algorithm for deciding the distribution of tables 
    processorid_t _next_cpu(const processorid_t& aprd,
                            const irpTableImpl* atable,
                            const int step=DF_CPU_STEP_TABLES);
    
}; // EOF: DoraEnv



EXIT_NAMESPACE(dora);

#endif /** __DORA_ENV_H */

