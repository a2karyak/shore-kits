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

/** @file:   part_table.cpp
 *
 *  @brief:  Wrapper for each table in DORA
 *
 *  @note:   Implemented as a vector of partitions and a routing table
 *
 *  @author: Ippokratis Pandis, Oct 2008
 */

#include <cstdio>

#include "dora/part_table.h"

using namespace shore;


ENTER_NAMESPACE(dora);


/****************************************************************** 
 *
 * Construction
 *
 ******************************************************************/

part_table_t::part_table_t(ShoreEnv* env, table_desc_t* ptable,
                           const processorid_t aprs,
                           const uint acpurange,
                           const uint keyEstimation,
                           const cvec_t& minKey,
                           const cvec_t& maxKey,
                           const uint pnum) 
    : _env(env), _table(ptable), _pcnt(pnum),
      _start_prs_id(aprs), _next_prs_id(aprs), _prs_range(acpurange), 
      _key_estimation(keyEstimation)
{
    assert (_env);
    assert (_table);        
    assert (aprs<=_env->get_max_cpu_count());
    assert (acpurange<=_env->get_active_cpu_count());

    // Check the key boundaries
    assert (minKey <= maxKey);
    _min.set(minKey);
    _max.set(maxKey);
}


part_table_t::~part_table_t() 
{ 
}


/****************************************************************** 
 *
 * Access methods
 *
 ******************************************************************/

vector<base_partition_t*>* part_table_t::get_vector() 
{ 
    return (&_ppvec); 
}

base_partition_t* part_table_t::get_part(const uint pos) 
{
    assert (pos<_ppvec.size());
    return (_ppvec[pos]);
}


/****************************************************************** 
 *
 * Control table
 *
 ******************************************************************/

// Stop all partitions
w_rc_t part_table_t::stop() 
{
    for (uint i=0; i<_ppvec.size(); i++) {
        _ppvec[i]->stop();
        delete (_ppvec[i]);
    }
    _ppvec.clear();
    return (RCOK);
}


// Prepare all partitions for a new run
w_rc_t part_table_t::prepareNewRun() 
{
    for (uint i=0; i<_ppvec.size(); i++) {
        _ppvec[i]->prepareNewRun();
    }
    return (RCOK);
}


// /////  Action-related methods

// // enqueues action, false on error
// inline int enqueue(Action* paction, const bool bWake, const int part) {
//     assert (part<_pcnt);
//     return (_ppvec[part]->enqueue(paction,bWake));
// }


/****************************************************************** 
 *
 * @fn:    reset()
 *
 * @brief: Resets the partitions
 *
 * @note:  Applies the partition distribution function (next_cpu())
 *
 ******************************************************************/

w_rc_t part_table_t::reset()
{
    TRACE( TRACE_DEBUG, "Reseting (%s)...\n", _table->name());
    _next_prs_id = _start_prs_id;
    for (uint i=0; i<_ppvec.size(); i++) {
        _ppvec[i]->reset(_next_prs_id,-1);
        CRITICAL_SECTION(next_prs_cs, _next_prs_lock);
        _next_prs_id = next_cpu(_next_prs_id);
    }    
    return (RCOK);
}



/****************************************************************** 
 *
 * @fn:    move()
 *
 * @brief: Moves the partitions (= worker threads) to another range
 *         processors
 *
 * @note:  Updates CPU range and calls reset()
 *
 ******************************************************************/

w_rc_t part_table_t::move(const processorid_t aprs, const uint arange) 
{
    // Update processor and range
    _start_prs_id = aprs;
    _prs_range = arange;

    // Call reset so that the workers to move to the new range
    return (reset());
}


/****************************************************************** 
 *
 * @fn:    next_cpu()
 *
 * @brief: The partition distribution function
 *
 * @note:  Very simple (just increases processor id by one) 
 *
 * @note:  This decision  can be based among others on:
 *
 *         - aprd                    - the current cpu
 *         - _env->_max_cpu_count    - the maximum cpu count (hard-limit)
 *         - _env->_active_cpu_count - the active cpu count (soft-limit)
 *         - this->_start_prs_id     - the first assigned cpu for the table
 *         - this->_prs_range        - a range of cpus assigned for the table
 *
 ******************************************************************/

processorid_t part_table_t::next_cpu(const processorid_t& aprd) 
{
    int binding = envVar::instance()->getVarInt("dora-cpu-binding",0);
    if (binding==0) {
        return (PBIND_NONE);
    }

    int partition_step = envVar::instance()->getVarInt("dora-cpu-partition-step",
                                                       DF_CPU_STEP_PARTITIONS);    
    processorid_t nextprs = ((aprd+partition_step) % _env->get_active_cpu_count());
    return (nextprs);
}



/****************************************************************** 
 *
 * Debugging
 *
 ******************************************************************/


void part_table_t::statistics() const 
{
    TRACE( TRACE_STATISTICS, "Table (%s)\n", _table->name());

    worker_stats_t ws_gathered;
    uint stl_sz = 0;

    for (uint i=0; i<_ppvec.size(); i++) {
        // gather worker statistics
        _ppvec[i]->statistics(ws_gathered);

        // gather dora-related structures statistics
        _ppvec[i]->stlsize(stl_sz);
    }

    if (ws_gathered._processed > MINIMUM_PROCESSED) {
        TRACE( TRACE_STATISTICS, "Parts (%d)\n", _pcnt);

        // print worker stats
        ws_gathered.print_and_reset();
        // print dora stl stats
        TRACE( TRACE_STATISTICS, "stl.entries (%d)\n", stl_sz);
    }
}        


void part_table_t::info() const 
{
    TRACE( TRACE_STATISTICS, "Table (%s)\n", _table->name());
    TRACE( TRACE_STATISTICS, "Parts (%d)\n", _pcnt);
}        


void part_table_t::dump() const 
{
    TRACE( TRACE_DEBUG, "Table (%s)\n", _table->name());
    TRACE( TRACE_DEBUG, "Parts (%d)\n", _pcnt);
    for (uint i=0; i<_ppvec.size(); i++) {
        _ppvec[i]->dump();
    }
}        



EXIT_NAMESPACE(dora);

