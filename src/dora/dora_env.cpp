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

typedef range_partition_i<int> irpImpl;


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



/******************************************************************** 
 *
 *  @fn:    table_part
 *
 *  @brief: Return partition (part_pos) of table (table_pos)
 *
 ********************************************************************/

irpImpl* DoraEnv::table_part(const uint table_pos, const uint part_pos) 
{
    assert (table_pos<_irptp_vec.size());        
    return (_irptp_vec[table_pos]->getPartByIdx(part_pos));
}


/******************************************************************** 
 *
 *  @fn:    statistics
 *
 *  @brief: Prints statistics for DORA Env. If DFlusher is enabled,
 *          it prints those stats. 
 *
 ********************************************************************/

int DoraEnv::_statistics(ShoreEnv* /* penv */)
{
    // DORA STATS
    TRACE( TRACE_STATISTICS, "----- DORA -----\n");
    uint sz=_irptp_vec.size();
    TRACE( TRACE_STATISTICS, "Tables  = (%d)\n", sz);
    for (uint i=0;i<sz;++i) {
        _irptp_vec[i]->statistics();
    }

#ifdef CFG_FLUSHER
    _flusher->statistics();
#endif
    return (0);
}


/****************************************************************** 
 *
 * @fn:    _post_start()
 *
 * @brief: Resets the tables and starts the flusher, if using one.
 *
 * @note:  Should be called before the start function of the specific
 *         DORA environment instance
 *
 ******************************************************************/

int DoraEnv::_post_start(ShoreEnv* penv)
{
#ifdef CFG_FLUSHER
    // Start the flusher
    TRACE( TRACE_ALWAYS, "Creating dora-flusher...\n");
    _flusher = new dora_flusher_t(penv, c_str("DFlusher")); 
    assert(_flusher.get());
    _flusher->fork();
    _flusher->start();
#endif

    // Reset the tables
    for (uint_t i=0; i<_irptp_vec.size(); i++) {
        _irptp_vec[i]->reset();
    }

    penv->set_dbc(DBC_ACTIVE);
    return (0);
}


/****************************************************************** 
 *
 * @fn:    _post_stop()
 *
 * @brief: Stops the tables and the flusher, if using one.
 *
 * @note:  Should be called after the stop function of the specific
 *         DORA environment instance, if any.
 *
 ******************************************************************/

int DoraEnv::_post_stop(ShoreEnv* penv)
{
    // Stopping the tables
    TRACE( TRACE_ALWAYS, "Stopping...\n");
    for (uint i=0; i<_irptp_vec.size(); i++) {
        _irptp_vec[i]->stop();
    }
    _irptp_vec.clear();

#ifdef CFG_FLUSHER
    // Stopping the flusher
    TRACE( TRACE_ALWAYS, "Stopping dora-flusher...\n");
    _flusher->stop();
    _flusher->join();
#endif

    penv->set_dbc(DBC_STOPPED);
    return (0);
}



/****************************************************************** 
 *
 * @fn:    _newrun()
 *
 * @brief: Calls the prepareNewRun for all the tables
 *
 ******************************************************************/

int DoraEnv::_newrun(ShoreEnv* /* penv */)
{
    TRACE( TRACE_DEBUG, "Preparing for new run...\n");
    for (uint i=0; i<_irptp_vec.size(); i++) {
        _irptp_vec[i]->prepareNewRun();
    }
    return (0);
}



/****************************************************************** 
 *
 * @fn:    _dump()
 *
 * @brief: Calls the dump for all the tables
 *
 ******************************************************************/

int DoraEnv::_dump(ShoreEnv* /* penv */)
{
    uint sz=_irptp_vec.size();
    TRACE( TRACE_ALWAYS, "Tables  = (%d)\n", sz);
    for (uint i=0; i<sz; i++) {
        _irptp_vec[i]->dump();
    }
    return (0);
}


/****************************************************************** 
 *
 * @fn:    _info()
 *
 * @brief: Calls the info for all the tables
 *
 ******************************************************************/

int DoraEnv::_info(const ShoreEnv* penv) const
{
    TRACE( TRACE_ALWAYS, "SF      = (%.1f)\n", penv->get_sf());
    uint sz=_irptp_vec.size();
    TRACE( TRACE_ALWAYS, "Tables  = (%d)\n", sz);
    for (uint i=0;i<sz;++i) {
        _irptp_vec[i]->info();
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
 *
 * @note:  This decision  can be based among others on:
 *
 *         - aprd                    - the current cpu
 *         - this->_max_cpu_count    - the maximum cpu count (hard-limit)
 *         - this->_active_cpu_count - the active cpu count (soft-limit)
 *         - this->_start_prs_id     - the first assigned cpu for the table
 *         - this->_prs_range        - a range of cpus assigned for the table
 *         - this->_sf               - the scaling factor of the db
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
