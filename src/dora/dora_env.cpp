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

/** @file:   dora_env.cpp
 *
 *  @brief:  Implementation of a DORA environment class.
 *           Common functionality among DORA environments.
 *
 *  @author: Ippokratis Pandis, Jun 2009
 */

#include "dora/common.h"
#include "dora/dora_env.h"

using namespace shore;

ENTER_NAMESPACE(dora);



/******************************************************************** 
 *
 *  @fn:    DoraEnv construction/destruction
 *
 *  @brief: Prints statistics for DORA Env. If DFlusher is enabled,
 *          it prints those stats. 
 *
 ********************************************************************/

DoraEnv::DoraEnv()
{ }

DoraEnv::~DoraEnv()
{ }

typedef range_partition_impl<int> irpImpl;

irpImpl* DoraEnv::table_part(const uint table_pos, const uint part_pos) 
{
    assert (table_pos<_irptp_vec.size());        
    return (_irptp_vec[table_pos]->get_part(part_pos));
}


/******************************************************************** 
 *
 *  @fn:    statistics
 *
 *  @brief: Prints statistics for DORA Env. If DFlusher is enabled,
 *          it prints those stats. 
 *
 ********************************************************************/

int DoraEnv::statistics()
{
#ifdef CFG_FLUSHER
    _flusher->statistics();
#endif
    return (0);
}


/****************************************************************** 
 *
 * @fn:    _start()
 *
 * @brief: If configured, starts the flusher
 *
 * @note:  Should be called before the start function of the specific
 *         DORA environment instance
 *
 ******************************************************************/

int DoraEnv::_start(ShoreEnv* penv)
{
#ifdef CFG_FLUSHER
    TRACE( TRACE_ALWAYS, "Creating dora-flusher...\n");
    _flusher = new dora_flusher_t(penv, c_str("DFlusher")); 
    assert(_flusher.get());
    _flusher->fork();
    _flusher->start();
#endif
    return (0);
}


/****************************************************************** 
 *
 * @fn:    _stop()
 *
 * @brief: If configured, stops the flusher
 *
 * @note:  Should be called after the stop function of the specific
 *         DORA environment instance
 *
 ******************************************************************/

int DoraEnv::_stop()
{
#ifdef CFG_FLUSHER
    TRACE( TRACE_ALWAYS, "Stopping dora-flusher...\n");
    _flusher->stop();
    _flusher->join();
#endif
    return (0);
}

/****************************************************************** 
 *
 * @fn:    _next_cpu()
 *
 * @brief: Deciding the distribution of tables
 *
 * @note:  Very simple (just increases processor id by DF_CPU_STEP) 
 *
 * @note:  This decision  can be based among others on:
 *
 *         - aprd                    - the current cpu
 *         - this->_max_cpu_count    - the maximum cpu count (hard-limit)
 *         - this->_active_cpu_count - the active cpu count (soft-limit)
 *         - this->_start_prs_id     - the first assigned cpu for the table
 *         - this->_prs_range        - a range of cpus assigned for the table
 *         - this->_sf               - the scaling factor of the tm1-db
 *         - this->_vec.size()       - the number of tables
 *         - atable                  - the current table
 *
 ******************************************************************/

processorid_t DoraEnv::_next_cpu(const processorid_t& aprd,
                                 const irpTableImpl* /* atable */,
                                 const int step)
{    
    int binding = envVar::instance()->getVarInt("dora-cpu-binding",0);
    if (binding==0)
        return (PBIND_NONE);

    int activecpu = envVar::instance()->getVarInt("active-cpu-count",64);
    processorid_t nextprs = ((aprd+step) % activecpu);
    TRACE( TRACE_DEBUG, "(%d) -> (%d)\n", aprd, nextprs);
    return (nextprs);
}


EXIT_NAMESPACE(dora);
