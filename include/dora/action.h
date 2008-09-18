/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file action.h
 *
 *  @brief Encapsulation of each action in DORA
 *
 *  @author Ippokratis Pandis (ipandis)
 */


#ifndef __DORA_ACTION_H
#define __DORA_ACTION_H

#include <cstdio>

#include "util.h"

#include "sm/shore/shore_env.h"


ENTER_NAMESPACE(dora);


using namespace shore;


/******************************************************************** 
 *
 * @enum:  ActionDecision
 *
 * @brief: Possible decision of an action
 *
 * @note:  Abort is decided when something goes wrong with own action
 *         Die if some other action (of the same trx) decides to abort
 *
 ********************************************************************/

enum ActionDecision { AD_UNDECIDED, AD_ABORT, AD_DEADLOCK, AD_COMMIT, AD_DIE };


/******************************************************************** 
 *
 * @class: action_t
 *
 * @brief: Abstract template-based class for the actions
 *
 * @note:  Actions are similar with packets in staged dbs
 *
 ********************************************************************/

template <class DataType>
class action_t
{
protected:

    ShoreEnv*    _env;    

    int          _field_count;
    DataType*    _down;
    DataType*    _up;

    countdown_t* _prvp;

    // trx-specific
    xct_t*         _xct;
    tid_t          _tid;
    ActionDecision _decision;

public:

    action_t(ShoreEnv* env, int field_count, countdown_t* prvp, xct_t* pxct)
        : _env(env), 
          _field_count(field_count), _down(NULL), _up(NULL),
          _prvp(prvp), _xct(pxct), _decision(AD_UNDECIDED)
    {
        assert (_env);
        assert (_field_count>0);
        assert (_prvp);
        assert (_xct);

        _down = new DataType[_field_count];
        _up   = new DataType[_field_count];

        _tid = ss_m::xct_to_tid(_xct);
    }

    virtual ~action_t() {
        if (_down)
            delete [] _down;
        
        if (_up)
            delete [] _up;
    }

    
    /** access methods */

    inline xct_t* get_xct() { return (_xct); }
    inline tid_t  get_tid() { return (_tid); }

    inline countdown_t* get_rvp() { return (_prvp); }    
    
    inline int update_xct(xct_t* axct) {
        assert (axct);
        _xct = axct;
        _tid = ss_m::xct_to_tid(_xct);
        return (0);
    }

    
    inline void set_decision(const ActionDecision& ad) { _decision = ad; }
    inline ActionDecision get_decision() { return (_decision); }

    
    /** trx-related operations */
    virtual w_rc_t trx_exec()=0;     // pure virtual
    virtual w_rc_t trx_rvp();        // default action on rvp - commit trx


private:

    // copying not allowed
    action_t(action_t const &);
    void operator=(action_t const &);
    
}; // EOF: action_t



/****************************************************************** 
 *
 * @fn:    trx_rvp()
 *
 * @brief: Code executed at rvp (rendez-vous point)
 *
 * @note:  The default action is to commit. However, it may be overlapped
 *
 ******************************************************************/

template <class DataType>
w_rc_t action_t<DataType>::trx_rvp()
{
    assert (_env);
    assert (_xct);

    w_rc_t ecommit = _env->db()->commit_xct();
    
    if (ecommit.is_error()) {
        TRACE( TRACE_ALWAYS, "Xct (%d) commit failed [0x%x]\n",
               _tid, ecommit.err_num());

        // TODO: if (_measure == MST_MEASURE) update_aborted_stats();

        w_rc_t eabort = _env->db()->abort_xct();
        if (eabort.is_error()) {
            TRACE( TRACE_ALWAYS, "Xct (%d) abort faied [0x%x]\n",
                   _tid, eabort.err_num());
        }

        return (ecommit);
    }


    TRACE( TRACE_TRX_FLOW, "Xct (%d) Payment completed\n", _tid);

    /*
    if (_measure == MST_MEASURE) {
        _tpcc_stats.inc_pay_com();
        _tmp_tpcc_stats.inc_pay_com();
        _env_stats.inc_trx_com();
    }
    */


    return (RCOK);
}



EXIT_NAMESPACE(dora);

#endif /** __DORA_ACTION_H */

