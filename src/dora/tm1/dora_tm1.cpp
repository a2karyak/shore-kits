/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   dora_tm1.cpp
 *
 *  @brief:  Implementation of the DORA TM1 class
 *
 *  @author: Ippokratis Pandis, Feb 2009
 */

#include "tls.h"

#include "dora/tm1/dora_tm1.h"
#include "dora/tm1/dora_tm1_impl.h"

using namespace shore;
using namespace tm1;


ENTER_NAMESPACE(dora);



// max field counts for (int) keys of tm1 tables
const int sub_IRP_KEY = 1;
const int ai_IRP_KEY  = 2;
const int sf_IRP_KEY  = 2;
const int cf_IRP_KEY  = 3;

// key estimations for each partition of the tm1 tables
const int sub_KEY_EST = 1000;
const int ai_KEY_EST  = 2500;
const int sf_KEY_EST  = 2500;
const int cf_KEY_EST  = 3750;



/****************************************************************** 
 *
 * @fn:    start()
 *
 * @brief: Starts the DORA TM1
 *
 ******************************************************************/

DoraTM1Env::DoraTM1Env(string confname)
    : ShoreTM1Env(confname), DoraEnv()
{ 
}

DoraTM1Env::~DoraTM1Env()
{ 
    stop();
}


/****************************************************************** 
 *
 * @fn:    start()
 *
 * @brief: Starts the DORA TM1
 *
 * @note:  Creates a corresponding number of partitions per table.
 *         The decision about the number of partitions per table may 
 *         be based among others on:
 *         - _env->_sf : the database scaling factor
 *         - _env->_{max,active}_cpu_count: {hard,soft} cpu counts
 *
 ******************************************************************/


int DoraTM1Env::start()
{
    // 1. Read configuration
    // 2. Create partitioned tables
    // 3. Add them to the vector
    // 4. Reset each table
    // 5. Start logger

    conf(); // re-configure

    // Call the pre-start procedure of the dora environment
    DoraEnv::_start(this);

    processorid_t icpu(_starting_cpu);

    TRACE( TRACE_STATISTICS, "Creating tables. SF=(%.1f) - DORA_SF=(%.1f)...\n", 
           _scaling_factor, _sf);    


    // SUBSCRIBER
    GENERATE_DORA_PARTS(sub,sub);


    // ACCESS INFO
    GENERATE_DORA_PARTS(ai,ai);


    // SPECIAL FACILITY
    GENERATE_DORA_PARTS(sf,sf);


    // CALL FORWARDING
    GENERATE_DORA_PARTS(cf,cf);


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
 * @brief: Stops the DORA TM1
 *
 ******************************************************************/

int DoraTM1Env::stop()
{
    TRACE( TRACE_ALWAYS, "Stopping...\n");
    for (int i=0; i<_irptp_vec.size(); i++) {
        _irptp_vec[i]->stop();
    }
    _irptp_vec.clear();

    // Call the meta-stop procedure of the dora environment
    DoraEnv::_stop();

    set_dbc(DBC_STOPPED);
    return (0);
}




/****************************************************************** 
 *
 * @fn:    resume()
 *
 * @brief: Resumes the DORA TM1
 *
 ******************************************************************/

int DoraTM1Env::resume()
{
    assert (0); // TODO (ip)
    set_dbc(DBC_ACTIVE);
    return (0);
}



/****************************************************************** 
 *
 * @fn:    pause()
 *
 * @brief: Pauses the DORA TM1
 *
 ******************************************************************/

int DoraTM1Env::pause()
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

int DoraTM1Env::conf()
{
    ShoreTM1Env::conf();

    TRACE( TRACE_DEBUG, "configuring dora-tm1\n");

    envVar* ev = envVar::instance();

    // Get CPU and binding configuration
    _starting_cpu = ev->getVarInt("dora-cpu-starting",DF_CPU_STEP_PARTITIONS);
    _cpu_table_step = ev->getVarInt("dora-cpu-table-step",DF_CPU_STEP_TABLES);

    _cpu_range = get_active_cpu_count();

    // For TM1 the DORA_SF is !not anymore! different than the SF of the database
    // In particular, for each 'real' SF there is 1 DORA SF
    _sf = upd_sf() * TM1_SUBS_PER_SF / TM1_SUBS_PER_DORA_PART;

    // Get DORA and per table partition SFs
    _dora_sf = ev->getSysVarInt("dora-sf");
    assert (_dora_sf);

    _sf_per_part_sub = _dora_sf * ev->getVarInt("dora-tm1-sf-per-part-sub",1);
    _sf_per_part_ai  = _dora_sf * ev->getVarInt("dora-tm1-sf-per-part-ai",1);
    _sf_per_part_sf  = _dora_sf * ev->getVarInt("dora-tm1-sf-per-part-sf",1);
    _sf_per_part_cf  = _dora_sf * ev->getVarInt("dora-tm1-sf-per-part-cf",1);        

    return (0);
}





/****************************************************************** 
 *
 * @fn:    newrun()
 *
 * @brief: Prepares the DORA TM1 DB for a new run
 *
 ******************************************************************/

int DoraTM1Env::newrun()
{
    TRACE( TRACE_DEBUG, "Preparing for new run...\n");
    for (int i=0; i<_irptp_vec.size(); i++) {
        _irptp_vec[i]->prepareNewRun();
    }
    return (0);
}



/****************************************************************** 
 *
 * @fn:    dump()
 *
 * @brief: Dumps information about all the tables and partitions
 *
 ******************************************************************/

int DoraTM1Env::dump()
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

int DoraTM1Env::info() const
{
    TRACE( TRACE_ALWAYS, "SF      = (%.1f)\n", _scaling_factor);
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
 *  @brief: Prints statistics for DORA-TM1
 *
 ********************************************************************/

int DoraTM1Env::statistics() 
{
    // DORA STATS
    TRACE( TRACE_STATISTICS, "----- DORA -----\n");
    int sz=_irptp_vec.size();
    TRACE( TRACE_STATISTICS, "Tables  = (%d)\n", sz);
    for (int i=0;i<sz;++i) {
        _irptp_vec[i]->statistics();
    }
    DoraEnv::statistics();
    return (0);

    // TM1 STATS
    // disabled
    TRACE( TRACE_STATISTICS, "----- TM1  -----\n");
    ShoreTM1Env::statistics();

    return (0);
}





/******************************************************************** 
 *
 *  Thread-local action and rvp object caches
 *
 ********************************************************************/



////////////////
// GetSubData //
////////////////

DEFINE_DORA_FINAL_RVP_GEN_FUNC(final_gsd_rvp,DoraTM1Env);

DEFINE_DORA_ACTION_GEN_FUNC(r_sub_gsd_action,rvp_t,get_sub_data_input_t,int,DoraTM1Env);



////////////////
// GetNewDest //
////////////////
#ifdef TM1GND2
DEFINE_DORA_MIDWAY_RVP_GEN_FUNC(mid_gnd_rvp,get_new_dest_input_t,DoraTM1Env);
DEFINE_DORA_FINAL_RVP_WITH_PREV_GEN_FUNC(final_gnd_rvp,DoraTM1Env);

DEFINE_DORA_ACTION_GEN_FUNC(r_sf_gnd_action,mid_gnd_rvp,get_new_dest_input_t,int,DoraTM1Env);

DEFINE_DORA_ACTION_GEN_FUNC(r_cf_gnd_action,rvp_t,get_new_dest_input_t,int,DoraTM1Env);
#else

#warning OLD-GetNewDest !!
DEFINE_DORA_FINAL_RVP_GEN_FUNC(final_gnd_rvp,DoraTM1Env);

DEFINE_DORA_ACTION_GEN_FUNC(r_sf_gnd_action,rvp_t,get_new_dest_input_t,int,DoraTM1Env);
DEFINE_DORA_ACTION_GEN_FUNC(r_cf_gnd_action,rvp_t,get_new_dest_input_t,int,DoraTM1Env);
#endif


////////////////
// GetAccData //
////////////////

DEFINE_DORA_FINAL_RVP_GEN_FUNC(final_gad_rvp,DoraTM1Env);

DEFINE_DORA_ACTION_GEN_FUNC(r_ai_gad_action,rvp_t,get_acc_data_input_t,int,DoraTM1Env);



////////////////
// UpdSubData //
////////////////
#ifdef TM1USD2
DEFINE_DORA_MIDWAY_RVP_GEN_FUNC(mid_usd_rvp,upd_sub_data_input_t,DoraTM1Env);
DEFINE_DORA_FINAL_RVP_WITH_PREV_GEN_FUNC(final_usd_rvp,DoraTM1Env);

DEFINE_DORA_ACTION_GEN_FUNC(upd_sf_usd_action,mid_usd_rvp,upd_sub_data_input_t,int,DoraTM1Env);

DEFINE_DORA_ACTION_GEN_FUNC(upd_sub_usd_action,rvp_t,upd_sub_data_input_t,int,DoraTM1Env);
#else

#warning OLD-UpdSubData !!
DEFINE_DORA_FINAL_RVP_GEN_FUNC(final_usd_rvp,DoraTM1Env);

DEFINE_DORA_ACTION_GEN_FUNC(upd_sub_usd_action,rvp_t,upd_sub_data_input_t,int,DoraTM1Env);
DEFINE_DORA_ACTION_GEN_FUNC(upd_sf_usd_action,rvp_t,upd_sub_data_input_t,int,DoraTM1Env);
#endif


////////////
// UpdLoc //
////////////

DEFINE_DORA_FINAL_RVP_GEN_FUNC(final_ul_rvp,DoraTM1Env);

DEFINE_DORA_ACTION_GEN_FUNC(upd_sub_ul_action,rvp_t,upd_loc_input_t,int,DoraTM1Env);



////////////////
// InsCallFwd //
////////////////
#ifdef TM1ICF2
DEFINE_DORA_MIDWAY_RVP_GEN_FUNC(mid1_icf_rvp,ins_call_fwd_input_t,DoraTM1Env);
DEFINE_DORA_MIDWAY_RVP_WITH_PREV_GEN_FUNC(mid2_icf_rvp,ins_call_fwd_input_t,DoraTM1Env);
DEFINE_DORA_FINAL_RVP_WITH_PREV_GEN_FUNC(final_icf_rvp,DoraTM1Env);

DEFINE_DORA_ACTION_GEN_FUNC(r_sub_icf_action,mid1_icf_rvp,ins_call_fwd_input_t,int,DoraTM1Env);
DEFINE_DORA_ACTION_GEN_FUNC(r_sf_icf_action,mid2_icf_rvp,ins_call_fwd_input_t,int,DoraTM1Env);

DEFINE_DORA_ACTION_GEN_FUNC(ins_cf_icf_action,rvp_t,ins_call_fwd_input_t,int,DoraTM1Env);

#else

#warning OLD-InsCallFwd !!

DEFINE_DORA_MIDWAY_RVP_GEN_FUNC(mid_icf_rvp,ins_call_fwd_input_t,DoraTM1Env);
DEFINE_DORA_FINAL_RVP_WITH_PREV_GEN_FUNC(final_icf_rvp,DoraTM1Env);

DEFINE_DORA_ACTION_GEN_FUNC(r_sub_icf_action,mid_icf_rvp,ins_call_fwd_input_t,int,DoraTM1Env);

DEFINE_DORA_ACTION_GEN_FUNC(r_sf_icf_action,rvp_t,ins_call_fwd_input_t,int,DoraTM1Env);
DEFINE_DORA_ACTION_GEN_FUNC(ins_cf_icf_action,rvp_t,ins_call_fwd_input_t,int,DoraTM1Env);
#endif


////////////////
// DelCallFwd //
////////////////

DEFINE_DORA_MIDWAY_RVP_GEN_FUNC(mid_dcf_rvp,del_call_fwd_input_t,DoraTM1Env);
DEFINE_DORA_FINAL_RVP_WITH_PREV_GEN_FUNC(final_dcf_rvp,DoraTM1Env);

DEFINE_DORA_ACTION_GEN_FUNC(r_sub_dcf_action,mid_dcf_rvp,del_call_fwd_input_t,int,DoraTM1Env);

DEFINE_DORA_ACTION_GEN_FUNC(del_cf_dcf_action,rvp_t,del_call_fwd_input_t,int,DoraTM1Env);



EXIT_NAMESPACE(dora);
