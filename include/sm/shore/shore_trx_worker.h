/* -*- mode:C++; c-basic-offset:4 -*-
     Shore-kits -- Benchmark implementations for Shore-MT
   
                       Copyright (c) 2007-2009
               Ecole Polytechnique Federale de Lausanne

                       Copyright (c) 2007-2008
                      Carnegie-Mellon University

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
/** @file:  shore_trx_result.h
 *
 *  @author Ippokratis Pandis, Nov 2008
 */


#ifndef __SHORE_TRX_WORKER_H
#define __SHORE_TRX_WORKER_H


ENTER_NAMESPACE(shore);


const int NO_VALID_TRX_ID = -1;

/******************************************************************** 
 *
 * @enum  TrxState
 *
 * @brief Possible states of a transaction
 *
 ********************************************************************/

enum TrxState { UNDEF, UNSUBMITTED, SUBMITTED, POISSONED, 
		COMMITTED, ROLLBACKED };

static char* sTrxStates[6] = { "Undef", "Unsubmitted", "Submitted",
                               "Poissoned", "Committed", "Rollbacked" };


/** Helper functions */

/** @fn translate_state
 *  @brief Displays in a friendly way a TrxState
 */

inline char* translate_state(TrxState aState) {
    assert ((aState >= 0) && (aState < 6));
    return (sTrxStates[aState]);
}


/******************************************************************** 
 *
 * @class trx_result_tuple_t
 *
 * @brief Class used to represent the result of a transaction
 *
 ********************************************************************/

class trx_result_tuple_t 
{
private:

    TrxState R_STATE;
    int R_ID;
    condex* _notify;
   
public:

    /** construction - destruction */

    trx_result_tuple_t() { reset(UNDEF, -1, NULL); }

    trx_result_tuple_t(TrxState aTrxState, int anID, condex* apcx = NULL) { 
        reset(aTrxState, anID, apcx);
    }

    ~trx_result_tuple_t() { }

    /** @fn copy constructor */
    trx_result_tuple_t(const trx_result_tuple_t& t) {
	reset(t.R_STATE, t.R_ID, t._notify);
    }      

    /** @fn copy assingment */
    trx_result_tuple_t& operator=(const trx_result_tuple_t& t) {        
        reset(t.R_STATE, t.R_ID, t._notify);        
        return (*this);
    }
    
    /** @fn equality operator */
    friend bool operator==(const trx_result_tuple_t& t, 
                           const trx_result_tuple_t& s) 
    {       
        return ((t.R_STATE == s.R_STATE) && (t.R_ID == s.R_ID));
    }


    /** Access methods */
    condex* get_notify() const { return (_notify); }
    void set_notify(condex* notify) { _notify = notify; }
    
    inline int get_id() const { return (R_ID); }
    inline void set_id(const int aID) { R_ID = aID; }

    inline TrxState get_state() { return (R_STATE); }
    inline char* say_state() { return (translate_state(R_STATE)); }
    inline void set_state(TrxState aState) { 
       assert ((aState >= UNDEF) && (aState <= ROLLBACKED));
       R_STATE = aState;
    }

    inline void reset(TrxState aTrxState, int anID, condex* notify) {
        // check for validity of inputs
        assert ((aTrxState >= UNDEF) && (aTrxState <= ROLLBACKED));
        assert (anID >= NO_VALID_TRX_ID);

        R_STATE = aTrxState;
        R_ID = anID;
	_notify = notify;
    }
        
}; // EOF: trx_result_tuple_t



EXIT_NAMESPACE(shore);

#endif /** __SHORE_TRX_WORKER_H */

