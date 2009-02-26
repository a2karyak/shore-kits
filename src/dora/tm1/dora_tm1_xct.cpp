/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   dora_tm1_xct.cpp
 *
 *  @brief:  Declaration of the DORA TM1 transactions
 *
 *  @author: Ippokratis Pandis, Feb 2009
 */


#include "dora/tm1/dora_tm1_impl.h"

#include "dora/tm1/dora_tm1.h"


ENTER_NAMESPACE(dora);

using namespace shore;
using namespace tm1;


typedef range_partition_impl<int>   irpImpl; 


/******** Exported functions  ********/


/********
 ******** Caution: The functions below should be invoked inside
 ********          the context of a smthread
 ********/


/******************************************************************** 
 *
 * TM1 DORA TRXS
 *
 * (1) The dora_XXX functions are wrappers to the real transactions
 * (2) The xct_dora_XXX functions are the implementation of the transactions
 *
 ********************************************************************/


/******************************************************************** 
 *
 * TM1 DORA TRXs Wrappers
 *
 * @brief: They are wrappers to the functions that execute the transaction
 *         body. Their responsibility is to:
 *
 *         1. Prepare the corresponding input
 *         2. Check the return of the trx function and abort the trx,
 *            if something went wrong
 *         3. Update the tm1 db environment statistics
 *
 ********************************************************************/


// --- without input specified --- //

DEFINE_DORA_WITHOUT_INPUT_TRX_WRAPPER(DoraTM1Env,get_sub_data);
DEFINE_DORA_WITHOUT_INPUT_TRX_WRAPPER(DoraTM1Env,get_new_dest);
DEFINE_DORA_WITHOUT_INPUT_TRX_WRAPPER(DoraTM1Env,get_acc_data);
DEFINE_DORA_WITHOUT_INPUT_TRX_WRAPPER(DoraTM1Env,upd_sub_data);
DEFINE_DORA_WITHOUT_INPUT_TRX_WRAPPER(DoraTM1Env,upd_loc);
DEFINE_DORA_WITHOUT_INPUT_TRX_WRAPPER(DoraTM1Env,ins_call_fwd);
DEFINE_DORA_WITHOUT_INPUT_TRX_WRAPPER(DoraTM1Env,del_call_fwd);



// --- with input specified --- //

/******************************************************************** 
 *
 * DORA TM1 GET_SUB_DATA
 *
 ********************************************************************/

w_rc_t DoraTM1Env::dora_get_sub_data(const int xct_id, 
                                     trx_result_tuple_t& atrt, 
                                     get_sub_data_input_t& in)
{
    // 1. Initiate transaction
    tid_t atid;   

#ifndef ONLYDORA
    W_DO(_pssm->begin_xct(atid));
#endif

    TRACE( TRACE_TRX_FLOW, "Begin (%d)\n", atid);

    xct_t* pxct = smthread_t::me()->xct();


    // 2. Detatch self from xct
#ifndef ONLYDORA
    assert (pxct);
    smthread_t::me()->detach_xct(pxct);
#endif

    TRACE( TRACE_TRX_FLOW, "Detached from (%d)\n", atid);


    // 3. Setup the final RVP
    final_gsd_rvp* frvp = new_final_gsd_rvp(atid,pxct,xct_id,atrt);    

    // 4. Generate the actions
    r_sub_gsd_action* r_sub = new_r_sub_gsd_action(atid,pxct,frvp,in);


    // 5a. Decide about partition
    // 5b. Enqueue

    {        
        irpImpl* my_sub_part = decide_part(sub(),in._s_id);
        assert (my_sub_part);

        // SUB_PART_CS
        CRITICAL_SECTION(sub_part_cs, my_sub_part->_enqueue_lock);
        if (my_sub_part->enqueue(r_sub)) {
            TRACE( TRACE_DEBUG, "Problem in enqueueing R_SUB_GSD\n");
            assert (0); 
            return (RC(de_PROBLEM_ENQUEUE));
        }
    }

    return (RCOK); 
}



/******************************************************************** 
 *
 * DORA TM1 GET_NEW_DEST
 *
 ********************************************************************/

w_rc_t DoraTM1Env::dora_get_new_dest(const int xct_id, 
                                     trx_result_tuple_t& atrt, 
                                     get_new_dest_input_t& in)
{
    // 1. Initiate transaction
    tid_t atid;   

#ifndef ONLYDORA
    W_DO(_pssm->begin_xct(atid));
#endif

    TRACE( TRACE_TRX_FLOW, "Begin (%d)\n", atid);

    xct_t* pxct = smthread_t::me()->xct();


    // 2. Detatch self from xct
#ifndef ONLYDORA
    assert (pxct);
    smthread_t::me()->detach_xct(pxct);
#endif

    TRACE( TRACE_TRX_FLOW, "Detached from (%d)\n", atid);


    // 3. Setup the final RVP
    final_gnd_rvp* frvp = new_final_gnd_rvp(atid,pxct,xct_id,atrt);    

    // 4. Generate the actions
    r_sf_gnd_action* r_sf = new_r_sf_gnd_action(atid,pxct,frvp,in);
    r_cf_gnd_action* r_cf = new_r_cf_gnd_action(atid,pxct,frvp,in);


    // 5a. Decide about partition
    // 5b. Enqueue

    {        
        irpImpl* my_sf_part = decide_part(sf(),in._s_id);
        irpImpl* my_cf_part = decide_part(cf(),in._s_id);

        // SF_PART_CS
        CRITICAL_SECTION(sf_part_cs, my_sf_part->_enqueue_lock);
        if (my_sf_part->enqueue(r_sf)) {
            TRACE( TRACE_DEBUG, "Problem in enqueueing R_SF\n");
            assert (0); 
            return (RC(de_PROBLEM_ENQUEUE));
        }

        // CF_PART_CS
        CRITICAL_SECTION(cf_part_cs, my_cf_part->_enqueue_lock);
        sf_part_cs.exit();
        if (my_cf_part->enqueue(r_cf)) {
            TRACE( TRACE_DEBUG, "Problem in enqueueing R_CF\n");
            assert (0); 
            return (RC(de_PROBLEM_ENQUEUE));
        }
    }

    return (RCOK); 
}




/******************************************************************** 
 *
 * DORA TM1 GET_ACC_DATA
 *
 ********************************************************************/

w_rc_t DoraTM1Env::dora_get_acc_data(const int xct_id, 
                                     trx_result_tuple_t& atrt, 
                                     get_acc_data_input_t& in)
{
    // 1. Initiate transaction
    tid_t atid;   

#ifndef ONLYDORA
    W_DO(_pssm->begin_xct(atid));
#endif

    TRACE( TRACE_TRX_FLOW, "Begin (%d)\n", atid);

    xct_t* pxct = smthread_t::me()->xct();


    // 2. Detatch self from xct
#ifndef ONLYDORA
    assert (pxct);
    smthread_t::me()->detach_xct(pxct);
#endif

    TRACE( TRACE_TRX_FLOW, "Detached from (%d)\n", atid);


    // 3. Setup the final RVP
    final_gad_rvp* frvp = new_final_gad_rvp(atid,pxct,xct_id,atrt);    

    // 4. Generate the actions
    r_ai_gad_action* r_ai = new_r_ai_gad_action(atid,pxct,frvp,in);


    // 5a. Decide about partition
    // 5b. Enqueue

    {        
        irpImpl* my_ai_part = decide_part(ai(),in._s_id);

        // AI_PART_CS
        CRITICAL_SECTION(ai_part_cs, my_ai_part->_enqueue_lock);
        if (my_ai_part->enqueue(r_ai)) {
            TRACE( TRACE_DEBUG, "Problem in enqueueing R_AI_GAD\n");
            assert (0); 
            return (RC(de_PROBLEM_ENQUEUE));
        }
    }

    return (RCOK); 
}



/******************************************************************** 
 *
 * DORA TM1 UPD_SUB_DATA
 *
 ********************************************************************/

w_rc_t DoraTM1Env::dora_upd_sub_data(const int xct_id, 
                                     trx_result_tuple_t& atrt, 
                                     upd_sub_data_input_t& in)
{
    // 1. Initiate transaction
    tid_t atid;   

#ifndef ONLYDORA
    W_DO(_pssm->begin_xct(atid));
#endif

    TRACE( TRACE_TRX_FLOW, "Begin (%d)\n", atid);

    xct_t* pxct = smthread_t::me()->xct();


    // 2. Detatch self from xct
#ifndef ONLYDORA
    assert (pxct);
    smthread_t::me()->detach_xct(pxct);
#endif

    TRACE( TRACE_TRX_FLOW, "Detached from (%d)\n", atid);


    // 3. Setup the final RVP
    final_usd_rvp* frvp = new_final_usd_rvp(atid,pxct,xct_id,atrt);    

    // 4. Generate the actions
    upd_sub_usd_action* upd_sub = new_upd_sub_usd_action(atid,pxct,frvp,in);
    upd_sf_usd_action* upd_sf = new_upd_sf_usd_action(atid,pxct,frvp,in);


    // 5a. Decide about partition
    // 5b. Enqueue

    {        
        irpImpl* my_sub_part = decide_part(sub(),in._s_id);
        irpImpl* my_sf_part = decide_part(sf(),in._s_id);

        // SUB_PART_CS
        CRITICAL_SECTION(sub_part_cs, my_sub_part->_enqueue_lock);
        if (my_sub_part->enqueue(upd_sub)) {
            TRACE( TRACE_DEBUG, "Problem in enqueueing UPD_SUB\n");
            assert (0); 
            return (RC(de_PROBLEM_ENQUEUE));
        }

        // SF_PART_CS
        CRITICAL_SECTION(sf_part_cs, my_sf_part->_enqueue_lock);
        sub_part_cs.exit();
        if (my_sf_part->enqueue(upd_sf)) {
            TRACE( TRACE_DEBUG, "Problem in enqueueing UPD_SF\n");
            assert (0); 
            return (RC(de_PROBLEM_ENQUEUE));
        }
    }

    return (RCOK); 
}


/******************************************************************** 
 *
 * DORA TM1 UPD_LOC
 *
 ********************************************************************/

w_rc_t DoraTM1Env::dora_upd_loc(const int xct_id, 
                                trx_result_tuple_t& atrt, 
                                upd_loc_input_t& in)
{
    // 1. Initiate transaction
    tid_t atid;   

    w_rc_t e = RCOK;
    bool bAbort = false;

#ifndef ONLYDORA
    W_DO(_pssm->begin_xct(atid));
#endif

    TRACE( TRACE_TRX_FLOW, "Begin (%d)\n", atid);

    xct_t* pxct = smthread_t::me()->xct();

#ifndef ONLYDORA
    assert (pxct);
#endif


    // 2. Do the secondary index probe
    row_impl<subscriber_t>* prsub = _psub_man->get_tuple();
    assert (prsub);

    // allocate space for the larger table representation
    rep_row_t areprow(_psub_man->ts());
    areprow.set(_psub_desc->maxsize()); 
    prsub->_rep = &areprow;


    final_ul_rvp* frvp = NULL;
    upd_sub_ul_action* upd_sub = NULL;


    { // make gotos safe

        // 1. Probe Subscriber sec index
        TRACE( TRACE_TRX_FLOW, 
               "App: %d UL:sub-nbr-idx-nl (%d)\n", xct_id, in._s_id);
#ifndef ONLYDORA
        e = _psub_man->sub_nbr_idx_nl(_pssm, prsub, in._sub_nbr);
#endif
        if (e.is_error()) { bAbort = true; goto done; }

        prsub->get_value(0, in._s_id);

    } // goto


    // 3. Detatch self from xct
#ifndef ONLYDORA
    assert (pxct);
    smthread_t::me()->detach_xct(pxct);
#endif
    TRACE( TRACE_TRX_FLOW, "Detached from (%d)\n", atid);
    

    // Now it is ready to create and enqueue the 1 action

    // 4. Setup the final RVP
    frvp = new_final_ul_rvp(atid,pxct,xct_id,atrt);    

    // 5. Generate the actions
    upd_sub = new_upd_sub_ul_action(atid,pxct,frvp,in);


    // 6a. Decide about partition
    // 6b. Enqueue

    {        
        irpImpl* my_sub_part = decide_part(sub(),in._s_id);

        // SUB_PART_CS
        CRITICAL_SECTION(sub_part_cs, my_sub_part->_enqueue_lock);
        if (my_sub_part->enqueue(upd_sub)) {
            TRACE( TRACE_DEBUG, "Problem in enqueueing UPD_SUB_UL\n");
            assert (0); 
            return (RC(de_PROBLEM_ENQUEUE));
        }
    }


done:

    // 8. Return the tuples to the cache
    _psub_man->give_tuple(prsub);

    // 9. if something went wrong no action has been enqueued yet.
    //    Therefore, it is responsibility of the dispatcher to execute the 
    //    RVP abort code, in order to release any locks acquired during the 
    //    secondary indexes probes.

    if (bAbort) {
        
        TRACE( TRACE_TRX_FLOW, "Dispatcher needs to abort (%d)\n", atid);

        frvp->abort();
        e = frvp->run();
        if (e.is_error()) {
            TRACE( TRACE_ALWAYS, "Problem running rvp for xct (%d) [0x%x]\n",
                   atid, e.err_num());
        }
            
        // delete rvp
        frvp->giveback();
        frvp = NULL;
    }

    return (e); 
}



/******************************************************************** 
 *
 * DORA TM1 INS_CALL_FWD
 *
 ********************************************************************/

w_rc_t DoraTM1Env::dora_ins_call_fwd(const int xct_id, 
                                     trx_result_tuple_t& atrt, 
                                     ins_call_fwd_input_t& in)
{
    // 1. Initiate transaction
    tid_t atid;   

    w_rc_t e = RCOK;
    bool bAbort = false;

#ifndef ONLYDORA
    W_DO(_pssm->begin_xct(atid));
#endif

    TRACE( TRACE_TRX_FLOW, "Begin (%d)\n", atid);

    xct_t* pxct = smthread_t::me()->xct();

#ifndef ONLYDORA
    assert (pxct);
#endif


    // 2. Do the secondary index probe
    row_impl<subscriber_t>* prsub = _psub_man->get_tuple();
    assert (prsub);

    // allocate space for the larger table representation
    rep_row_t areprow(_psub_man->ts());
    areprow.set(_psub_desc->maxsize()); 
    prsub->_rep = &areprow;

    final_icf_rvp* frvp = NULL;
    r_sf_icf_action* r_sf = NULL;
    ins_cf_icf_action* ins_cf = NULL;
    

    { // make gotos safe

        // 1. Probe Subscriber sec index
        TRACE( TRACE_TRX_FLOW, 
               "App: %d ICF:sub-nbr-idx-nl (%d)\n", 
               xct_id, in._s_id);
#ifndef ONLYDORA
        e = _psub_man->sub_nbr_idx_nl(_pssm, prsub, in._sub_nbr);
#endif
        if (e.is_error()) { bAbort = true; goto done; }

        prsub->get_value(0, in._s_id);

    } // goto


    // 3. Detatch self from xct
#ifndef ONLYDORA
    assert (pxct);
    smthread_t::me()->detach_xct(pxct);
#endif
    TRACE( TRACE_TRX_FLOW, "Detached from (%d)\n", atid);
    

    // Now it is ready to create and enqueue the 1 action

    // 4. Setup the final RVP
    frvp = new_final_icf_rvp(atid,pxct,xct_id,atrt);    

    // 5. Generate the actions
    r_sf = new_r_sf_icf_action(atid,pxct,frvp,in);
    ins_cf = new_ins_cf_icf_action(atid,pxct,frvp,in);


    // 6a. Decide about partition
    // 6b. Enqueue

    {        
        irpImpl* my_sf_part = decide_part(sf(),in._s_id);
        irpImpl* my_cf_part = decide_part(cf(),in._s_id);

        // SF_PART_CS
        CRITICAL_SECTION(sf_part_cs, my_sf_part->_enqueue_lock);
        if (my_sf_part->enqueue(r_sf)) {
            TRACE( TRACE_DEBUG, "Problem in enqueueing R_SF_ICF\n");
            assert (0); 
            return (RC(de_PROBLEM_ENQUEUE));
        }

        // CF_PART_CS
        CRITICAL_SECTION(cf_part_cs, my_cf_part->_enqueue_lock);
        cf_part_cs.exit();
        if (my_cf_part->enqueue(ins_cf)) {
            TRACE( TRACE_DEBUG, "Problem in enqueueing INS_CF_ICF\n");
            assert (0); 
            return (RC(de_PROBLEM_ENQUEUE));
        }
    }


done:

    // 8. Return the tuples to the cache
    _psub_man->give_tuple(prsub);

    // 9. if something went wrong no action has been enqueued yet.
    //    Therefore, it is responsibility of the dispatcher to execute the 
    //    RVP abort code, in order to release any locks acquired during the 
    //    secondary indexes probes.

    if (bAbort) {
        
        TRACE( TRACE_TRX_FLOW, "Dispatcher needs to abort (%d)\n", atid);

        frvp->abort();
        e = frvp->run();
        if (e.is_error()) {
            TRACE( TRACE_ALWAYS, "Problem running rvp for xct (%d) [0x%x]\n",
                   atid, e.err_num());
        }
            
        // delete rvp
        frvp->giveback();
        frvp = NULL;
    }

    return (e); 
}



/******************************************************************** 
 *
 * DORA TM1 DEL_CALL_FWD
 *
 ********************************************************************/

w_rc_t DoraTM1Env::dora_del_call_fwd(const int xct_id, 
                                     trx_result_tuple_t& atrt, 
                                     del_call_fwd_input_t& in)
{
    // 1. Initiate transaction
    tid_t atid;   

    w_rc_t e = RCOK;
    bool bAbort = false;

#ifndef ONLYDORA
    W_DO(_pssm->begin_xct(atid));
#endif

    TRACE( TRACE_TRX_FLOW, "Begin (%d)\n", atid);

    xct_t* pxct = smthread_t::me()->xct();

#ifndef ONLYDORA
    assert (pxct);
#endif


    // 2. Do the secondary index probe
    row_impl<subscriber_t>* prsub = _psub_man->get_tuple();
    assert (prsub);

    // allocate space for the larger table representation
    rep_row_t areprow(_psub_man->ts());
    areprow.set(_psub_desc->maxsize()); 
    prsub->_rep = &areprow;


    final_dcf_rvp* frvp = NULL;
    del_cf_dcf_action* del_cf = NULL;


    { // make gotos safe

        // 1. Probe Subscriber sec index
        TRACE( TRACE_TRX_FLOW, 
               "App: %d DCF:sub-nbr-idx-nl (%d)\n", 
               xct_id, in._s_id);
        e = _psub_man->sub_nbr_idx_nl(_pssm, prsub, in._sub_nbr);
        if (e.is_error()) { bAbort = true; goto done; }

        prsub->get_value(0, in._s_id);

    } // goto


    // 3. Detatch self from xct
#ifndef ONLYDORA
    assert (pxct);
    smthread_t::me()->detach_xct(pxct);
#endif
    TRACE( TRACE_TRX_FLOW, "Detached from (%d)\n", atid);
    

    // Now it is ready to create and enqueue the 1 action

    // 4. Setup the final RVP
    frvp = new_final_dcf_rvp(atid,pxct,xct_id,atrt);    

    // 5. Generate the actions
    del_cf = new_del_cf_dcf_action(atid,pxct,frvp,in);


    // 6a. Decide about partition
    // 6b. Enqueue

    {        
        irpImpl* my_cf_part  = decide_part(cf(),in._s_id);

        // CF_PART_CS
        CRITICAL_SECTION(cf_part_cs, my_cf_part->_enqueue_lock);
        if (my_cf_part->enqueue(del_cf)) {
            TRACE( TRACE_DEBUG, "Problem in enqueueing DEL_CF_ICF\n");
            assert (0); 
            return (RC(de_PROBLEM_ENQUEUE));
        }
    }


done:

    // 8. Return the tuples to the cache
    _psub_man->give_tuple(prsub);

    // 9. if something went wrong no action has been enqueued yet.
    //    Therefore, it is responsibility of the dispatcher to execute the 
    //    RVP abort code, in order to release any locks acquired during the 
    //    secondary indexes probes.

    if (bAbort) {
        
        TRACE( TRACE_TRX_FLOW, "Dispatcher needs to abort (%d)\n", atid);

        frvp->abort();
        e = frvp->run();
        if (e.is_error()) {
            TRACE( TRACE_ALWAYS, "Problem running rvp for xct (%d) [0x%x]\n",
                   atid, e.err_num());
        }
            
        // delete rvp
        frvp->giveback();
        frvp = NULL;
    }

    return (e); 
}





EXIT_NAMESPACE(dora);
