/* -*- mode:C++; c-basic-offset:4 -*- */

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
const int br_IRP_KEY  = 1;
const int te_IRP_KEY  = 1;
const int ac_IRP_KEY  = 1;
const int hi_IRP_KEY  = 1;

// key estimations for each partition of the tpcb tables
const int br_KEY_EST  = 100;
const int te_KEY_EST  = 100;
const int ac_KEY_EST  = 100;
const int hi_KEY_EST  = 100;




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
#ifdef CFG_FLUSHER
    _flusher = new dora_flusher_t(this, c_str("DFlusher")); 
    assert(_flusher.get());
    _flusher->fork();
#endif
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

    // Call the pre-start procedure of the dora environment
    DoraEnv::_start();

    processorid_t icpu(_starting_cpu);

    TRACE( TRACE_STATISTICS, "Creating tables. SF=(%d) - DORA_SF=(%d)...\n", 
           _scaling_factor, _sf);    


    // BRANCH
    GENERATE_DORA_PARTS(br,branch);


    // TELLER
    GENERATE_DORA_PARTS(te,teller);


    // ACCOUNT
    GENERATE_DORA_PARTS(ac,account);


    // HISTORY
    GENERATE_DORA_PARTS(hi,history);


    TRACE( TRACE_DEBUG, "Starting tables...\n");
    for (int i=0; i<_irptp_vec.size(); i++) {
        _irptp_vec[i]->reset();
    }

    set_dbc(DBC_ACTIVE);
    return (0);
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
    TRACE( TRACE_ALWAYS, "Stopping...\n");
    for (int i=0; i<_irptp_vec.size(); i++) {
        _irptp_vec[i]->stop();
    }
    _irptp_vec.clear();
    set_dbc(DBC_STOPPED);
    return (0);
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

    TRACE( TRACE_DEBUG, "configuring dora-tpcb\n");

    envVar* ev = envVar::instance();

    // Get CPU and binding configuration
    _starting_cpu = ev->getVarInt("dora-cpu-starting",DF_CPU_STEP_PARTITIONS);
    _cpu_table_step = ev->getVarInt("dora-cpu-table-step",DF_CPU_STEP_TABLES);

    _cpu_range = get_active_cpu_count();

    // For TPCB the DORA_SF is !not anymore! different than the SF of the database
    // In particular, for each 'real' SF there is 1 DORA SF
    _sf = upd_sf(); // * TPCB_SUBS_PER_SF / TPCB_SUBS_PER_DORA_PART;

    // Get DORA and per table partition SFs
    _dora_sf = ev->getSysVarInt("dora-sf");
    assert (_dora_sf);

    _sf_per_part_br  = _dora_sf * ev->getVarInt("dora-tpcb-sf-per-part-br",1);
    _sf_per_part_te  = _dora_sf * ev->getVarInt("dora-tpcb-sf-per-part-te",1);
    _sf_per_part_ac  = _dora_sf * ev->getVarInt("dora-tpcb-sf-per-part-ac",1);
    _sf_per_part_hi  = _dora_sf * ev->getVarInt("dora-tpcb-sf-per-part-hi",1);        

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
    TRACE( TRACE_DEBUG, "Preparing for new run...\n");
    for (int i=0; i<_irptp_vec.size(); i++) {
        _irptp_vec[i]->prepareNewRun();
    }
    return (0);
}



/****************************************************************** 
 *
 * @fn:    set()
 *
 * @brief: Given a map of strings it updates the db environment
 *
 ******************************************************************/

int DoraTPCBEnv::set(envVarMap* vars)
{
    TRACE( TRACE_DEBUG, "Reading set...\n");
    for (envVarConstIt cit = vars->begin(); cit != vars->end(); ++cit)
        TRACE( TRACE_DEBUG, "(%s) (%s)\n", 
               cit->first.c_str(), 
               cit->second.c_str());
    TRACE( TRACE_DEBUG, "*** unimplemented ***\n");
    return (0);
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
    int sz=_irptp_vec.size();
    TRACE( TRACE_ALWAYS, "Tables  = (%d)\n", sz);
    for (int i=0; i<sz; i++) {
        _irptp_vec[i]->dump();
    }
    return (0);
}


/****************************************************************** 
 *
 * @fn:    info()
 *
 * @brief: Information about the current state of DORA
 *
 ******************************************************************/

int DoraTPCBEnv::info()
{
    TRACE( TRACE_ALWAYS, "SF      = (%d)\n", _scaling_factor);
    int sz=_irptp_vec.size();
    TRACE( TRACE_ALWAYS, "Tables  = (%d)\n", sz);
    for (int i=0;i<sz;++i) {
        _irptp_vec[i]->info();
    }
    return (0);
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
    // DORA STATS
    TRACE( TRACE_STATISTICS, "----- DORA -----\n");
    int sz=_irptp_vec.size();
    TRACE( TRACE_STATISTICS, "Tables  = (%d)\n", sz);
    for (int i=0;i<sz;++i) {
        _irptp_vec[i]->statistics();
    }
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
