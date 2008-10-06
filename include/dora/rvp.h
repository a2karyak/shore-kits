/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file rvp.h
 *
 *  @brief Rendezvous points
 *
 *  @author Ippokratis Pandis, Oct 2008
 */


#ifndef __DORA_RVP_H
#define __DORA_RVP_H

#include "util/countdown.h"

#include "dora.h"


ENTER_NAMESPACE(dora);


/******************************************************************** 
 *
 * @class: rvp_t
 *
 * @brief: Abstract class for rendez-vous points
 *
 * @note:  This point will contain information only needed to be
 *         communication among different phases of the trx
 *
 ********************************************************************/

class rvp_t
{
protected:

    // the countdown
    countdown_t    _countdown;
    ActionDecision _decision;

    // trx-specific
    xct_t*         _xct; // Not the owner
    tid_t          _tid;

public:

    rvp_t() : _xct(NULL), _decision(AD_UNDECIDED) { }

    virtual ~rvp_t() { }
    
    /** access methods */

    // TRX-ID Related

    inline xct_t* get_xct() { return (_xct); }
    inline tid_t  get_tid() { return (_tid); }
   
    inline void set(tid_t atid, xct_t* axct) {
        assert (axct);
        assert (_tid == ss_m::xct_to_tid(axct));
        _tid = atid;
        _xct = axct;
    }

    // Decision related

    inline void set_decision(const ActionDecision& ad) { _decision = ad; }
    inline ActionDecision get_decision() { return (_decision); }

    /** trx-related operations */
    virtual w_rc_t run()=0; // default action on rvp - commit trx

    virtual void cleanup()=0; // does any clean up

    bool post(bool is_error=false) { return (_countdown.post(is_error)); }

    // for the cache
    void reset() {
        _xct = NULL;
        _decision = AD_UNDECIDED;
    }    

private:

    // copying not allowed
    rvp_t(rvp_t const &);
    void operator=(rvp_t const &);
    
}; // EOF: rvp_t


/******************************************************************** 
 *
 * @class: terminal_rvp_t
 *
 * @brief: Abstract class for the terminal (commiting) rendez-vous points
 *
 * @note:  This point will try to commit and update the appropriate stats
 *
 ********************************************************************/

class terminal_rvp_t : public rvp_t
{
public:

    terminal_rvp_t() : rvp_t() { }
    virtual ~terminal_rvp_t() { }
    
    virtual void cleanup()=0; // does any clean up

    /** trx-related operations */
    virtual w_rc_t run()=0; // default action on rvp - commit trx    

    virtual void upd_committed_stats()=0; // update the committed trx stats
    virtual void upd_aborted_stats()=0;   // update the committed trx stats

protected:

    w_rc_t _run(ShoreEnv* penv);

}; // EOF: terminal_rvp_t



EXIT_NAMESPACE(dora);

#endif /** __DORA_RVP_H */

