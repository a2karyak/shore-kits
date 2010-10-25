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

/** @file:   dora_tpcb.cpp
 *
 *  @brief:  Implementation of the DORA TPCB class
 *
 *  @author: Ippokratis Pandis
 *  @date:   July 2009
 */

#include "tls.h"

#include "dora/tpcb/dora_tpcb.h"
#include "dora/tpcb/dora_tpcb_impl.h"

using namespace shore;
using namespace tpcb;


ENTER_NAMESPACE(dora);



// max field counts for (int) keys of tpcb tables
const uint br_IRP_KEY  = 1;
const uint te_IRP_KEY  = 1;
const uint ac_IRP_KEY  = 1;
const uint hi_IRP_KEY  = 1;

// key estimations for each partition of the tpcb tables
const uint br_KEY_EST  = 100;
const uint te_KEY_EST  = 100;
const uint ac_KEY_EST  = 100;
const uint hi_KEY_EST  = 100;




/****************************************************************** 
 *
 * @fn:    construction/destruction
 *
 * @brief: If configured, it creates and starts the flusher 
 *
 ******************************************************************/
    
DoraTPCBEnv::DoraTPCBEnv(string confname)
    : ShoreTPCBEnv(confname)
{ 
}

DoraTPCBEnv::~DoraTPCBEnv() 
{ 
    stop();
}


/****************************************************************** 
 *
 * @fn:    start()
 *
 * @brief: Starts the DORA TPCB
 *
 * @note:  Creates a corresponding number of partitions per table.
 *         The decision about the number of partitions per table may 
 *         be based among others on:
 *         - _env->_sf : the database scaling factor
 *         - _env->_{max,active}_cpu_count: {hard,soft} cpu counts
 *
 ******************************************************************/

int DoraTPCBEnv::start()
{
    // 1. Creates partitioned tables
    // 2. Adds them to the vector
    // 3. Resets each table

    conf(); // re-configure
    processorid_t icpu(_starting_cpu);

    // BRANCH
    GENERATE_DORA_PARTS(br,branch);

    // TELLER
    GENERATE_DORA_PARTS(te,teller);

    // ACCOUNT
    GENERATE_DORA_PARTS(ac,account);

    // HISTORY
    GENERATE_DORA_PARTS(hi,history);

    // Call the post-start procedure of the dora environment
    DoraEnv::_post_start(this);
    return (0);
}



/******************************************************************** 
 *
 *  @fn:    update_partitioning()
 *
 *  @brief: Applies the baseline partitioning to the TPC-B tables
 *
 ********************************************************************/

w_rc_t DoraTPCBEnv::update_partitioning() 
{
    // Pulling this partitioning out of the thin air
    int minKeyVal = 0;
    int maxKeyVal = get_sf()+1;

    char* minKey = (char*)malloc(sizeof(int));
    memset(minKey,0,sizeof(int));
    memcpy(minKey,&minKeyVal,sizeof(int));

    char* maxKey = (char*)malloc(sizeof(int));
    memset(maxKey,0,sizeof(int));
    memcpy(maxKey,&maxKeyVal,sizeof(int));

    // Branches: [ 0 .. #Branches+1 )
    _pbranch_desc->set_partitioning(minKey,sizeof(int),maxKey,sizeof(int),_parts_br);

    // History: does not have account we use the same with Branches
    _phistory_desc->set_partitioning(minKey,sizeof(int),maxKey,sizeof(int),_parts_hi);

    // Tellers:  [ 0 .. (#Branches*TPCB_TELLERS_PER_BRANCH)+1 )
    maxKeyVal = (get_sf()*TPCB_TELLERS_PER_BRANCH)+1;
    memset(maxKey,0,sizeof(int));
    memcpy(maxKey,&maxKeyVal,sizeof(int));
    _pteller_desc->set_partitioning(minKey,sizeof(int),maxKey,sizeof(int),_parts_te);

    // Accounts: [ 0 .. (#Branches*TPCB_ACCOUNTS_PER_BRANCH)+1 )
    maxKeyVal = (get_sf()*TPCB_ACCOUNTS_PER_BRANCH)+1;
    memset(maxKey,0,sizeof(int));
    memcpy(maxKey,&maxKeyVal,sizeof(int));
    _paccount_desc->set_partitioning(minKey,sizeof(int),maxKey,sizeof(int),_parts_ac);

    free (minKey);
    free (maxKey);

    return (RCOK);
}





/****************************************************************** 
 *
 * @fn:    stop()
 *
 * @brief: Stops the DORA TPCB
 *
 ******************************************************************/

int DoraTPCBEnv::stop()
{
    // Call the post-stop procedure of the dora environment
    return (DoraEnv::_post_stop(this));
}


/****************************************************************** 
 *
 * @fn:    resume()
 *
 * @brief: Resumes the DORA TPCB
 *
 ******************************************************************/

int DoraTPCBEnv::resume()
{
    assert (0); // IP: Not implement yet
    set_dbc(DBC_ACTIVE);
    return (0);
}



/****************************************************************** 
 *
 * @fn:    pause()
 *
 * @brief: Pauses the DORA TPCB
 *
 ******************************************************************/

int DoraTPCBEnv::pause()
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

int DoraTPCBEnv::conf()
{
    ShoreTPCBEnv::conf();

    envVar* ev = envVar::instance();

    // Get CPU and binding configuration
    _cpu_range = get_active_cpu_count();
    _starting_cpu = ev->getVarInt("dora-cpu-starting",DF_CPU_STEP_PARTITIONS);
    _cpu_table_step = ev->getVarInt("dora-cpu-table-step",DF_CPU_STEP_TABLES);

    // For each table read the ratio of partition per CPU and calculate the
    // number of partition to create (depending the number of CPUs available).
    double br_PerCPU = ev->getVarDouble("dora-ratio-tpcb-br",0);
    _parts_br = ( br_PerCPU>0 ? (_cpu_range * br_PerCPU) : 1);

    double te_PerCPU = ev->getVarDouble("dora-ratio-tpcb-te",0);
    _parts_te = ( te_PerCPU>0 ? (_cpu_range * te_PerCPU) : 1);

    double ac_PerCPU = ev->getVarDouble("dora-ratio-tpcb-ac",0);
    _parts_ac = ( ac_PerCPU>0 ? (_cpu_range * ac_PerCPU) : 1);

    double hi_PerCPU = ev->getVarDouble("dora-ratio-tpcb-hi",0);
    _parts_hi = ( hi_PerCPU>0 ? (_cpu_range * hi_PerCPU) : 1);

    return (0);
}





/****************************************************************** 
 *
 * @fn:    newrun()
 *
 * @brief: Prepares the DORA TPCB DB for a new run
 *
 ******************************************************************/

int DoraTPCBEnv::newrun()
{
    return (DoraEnv::_newrun(this));
}


/****************************************************************** 
 *
 * @fn:    dump()
 *
 * @brief: Dumps information about all the tables and partitions
 *
 ******************************************************************/

int DoraTPCBEnv::dump()
{
    return (DoraEnv::_dump(this));
}


/****************************************************************** 
 *
 * @fn:    info()
 *
 * @brief: Information about the current state of DORA
 *
 ******************************************************************/

int DoraTPCBEnv::info() const
{
    return (DoraEnv::_info(this));
}


/******************************************************************** 
 *
 *  @fn:    statistics
 *
 *  @brief: Prints statistics for DORA-TPCB
 *
 ********************************************************************/

int DoraTPCBEnv::statistics() 
{
    DoraEnv::_statistics(this);
    return (0);

    // TPCB STATS
    TRACE( TRACE_STATISTICS, "----- TPCB  -----\n");
    ShoreTPCBEnv::statistics();
    return (0);
}



/******************************************************************** 
 *
 *  Thread-local action and rvp object caches
 *
 ********************************************************************/



////////////////
// AcctUpdate //
////////////////

DEFINE_DORA_FINAL_RVP_GEN_FUNC(final_au_rvp,DoraTPCBEnv);

DEFINE_DORA_ACTION_GEN_FUNC(upd_br_action,rvp_t,acct_update_input_t,int,DoraTPCBEnv);
DEFINE_DORA_ACTION_GEN_FUNC(upd_te_action,rvp_t,acct_update_input_t,int,DoraTPCBEnv);
DEFINE_DORA_ACTION_GEN_FUNC(upd_ac_action,rvp_t,acct_update_input_t,int,DoraTPCBEnv);
DEFINE_DORA_ACTION_GEN_FUNC(ins_hi_action,rvp_t,acct_update_input_t,int,DoraTPCBEnv);



EXIT_NAMESPACE(dora);
