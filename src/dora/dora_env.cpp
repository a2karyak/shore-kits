/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   dora_env.cpp
 *
 *  @brief:  Implementation of a DORA environment class.
 *           Common functionality among DORA environments.
 *
 *  @author: Ippokratis Pandis, Jun 2009
 */

#include "dora/dora_env.h"

using namespace shore;

ENTER_NAMESPACE(dora);


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
 *         - this->_sf               - the scaling factor of the tm1-db
 *         - this->_vec.size()       - the number of tables
 *         - atable                  - the current table
 *
 ******************************************************************/

const processorid_t DoraEnv::_next_cpu(const processorid_t& aprd,
                                       const irpTableImpl* atable,
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
