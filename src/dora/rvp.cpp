/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   lockman.cpp
 *
 *  @brief:  Implementation of Rendezvous points for DORA
 *
 *  @author: Ippokratis Pandis, Oct 2008
 */

#include "dora/rvp.h"


using namespace dora;


/****************************************************************** 
 *
 * @fn:    _run()
 *
 * @brief: Code executed at terminal rendez-vous point
 *
 * @note:  The default action is to commit. However, it may be overlapped
 * @note:  There are hooks for updating the correct stats
 *
 ******************************************************************/

w_rc_t terminal_rvp_t::_run(ShoreEnv* penv)
{
    assert (_xct);
    assert (penv);
    assert (_result);

    // try to commit
    w_rc_t ecommit = penv->db()->commit_xct();
    
    if (ecommit.is_error()) {
        TRACE( TRACE_ALWAYS, "Xct (%d) commit failed [0x%x]\n",
               _tid, ecommit.err_num());

        upd_aborted_stats(); // hook - update aborted stats

        w_rc_t eabort = penv->db()->abort_xct();
        if (eabort.is_error()) {
            TRACE( TRACE_ALWAYS, "Xct (%d) abort failed [0x%x]\n",
                   _tid, eabort.err_num());
        }

        // do clean up
        cleanup();
        return (ecommit);
    }
    upd_committed_stats(); // hook - update committed stats
    
    TRACE( TRACE_TRX_FLOW, "Xct (%d) completed\n", _tid);

    assert (0); // TODO (ip) - Signal Cond Var

    // do clean up
    cleanup();
    return (RCOK);
}
