/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file trx_packet.h
 *
 *  @brief A trx_packet is a normal packet with a transaction id, status identifier,
 *  and a corresponding db-specific (currently BerkeleyDB) transaction handle.
 *
 *  @author Ippokratis Pandis (ipandis)
 */


#ifndef __TRX_PACKET_H
#define __TRX_PACKET_H

#include <cstdio>

#include "core.h"
#include "stages/common/process_tuple.h"


ENTER_NAMESPACE(qpipe);


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


/** Exported classes and data structures */


struct alloc_guard {
    char* _ptr;
    alloc_guard(int size) : _ptr((char*) pool()->alloc(size)) { }
    ~alloc_guard() { pool()->free(_ptr); }
    operator char*() { return _ptr; }
    DECLARE_POOL_ALLOC_POOL(alloc_guard);
};

/******************************************************************** 
 *
 * @class trx_packet_t
 *
 * @brief Definition of the transactional packets.
 *        A transactional packet has a unique identifier for the 
 *        TRX it belongs to.
 *
 ********************************************************************/

class trx_packet_t : public packet_t 
{
  
protected:
    
    /** @brief The id of the transaction */
    int _trx_id;

    /** @brief The status of the transaction */
    TrxState _trx_state;

public:


    /* see trx_packet.cpp for documentation */  

    trx_packet_t(const c_str     &packet_id,
                 const c_str     &packet_type,
                 tuple_fifo*     output_buffer,
                 tuple_filter_t* output_filter,
                 query_plan*     trx_plan,
                 bool _merge,
                 bool _unreserve,
                 const int       trx_id = NO_VALID_TRX_ID);

    virtual ~trx_packet_t(void);

    /** Member access functions */

    inline int get_trx_id() {
        return (_trx_id);
    }
    
    inline void set_trx_id(int a_trx_id) {
        // Cannot set a NO_VALID trx id
        assert (a_trx_id > NO_VALID_TRX_ID);        
        _trx_id = a_trx_id;
    }

    inline TrxState trx_state() {
        return (_trx_state);
    }
    
    inline void set_trx_state(TrxState a_trx_state) {
        // Cannot set a UNDEF state
        assert (a_trx_state > UNDEF);        
        _trx_state = a_trx_state;
    }


    /** Exported Functions */
    

    virtual void rollback() {
        TRACE( TRACE_ALWAYS, "TRX - %d SHOULD ROLLBACK!\n", _trx_id);
    }

    virtual void commit() {
        TRACE( TRACE_ALWAYS, "TRX - %d SHOULD COMMIT!\n", _trx_id);
    }


    /** Helper Functions */

    /** @brief Describes the requested transaction */

    virtual void describe_trx() {
        TRACE(TRACE_ALWAYS, "A TRX request with id=%d\n", _trx_id);
    }


    /** @brief Returns true if compared trx_packet_t has the same _trx_id */
    
    static bool is_same_trx(trx_packet_t const* a_trx, trx_packet_t const* b_trx) {
        
        if ( !a_trx || !b_trx || a_trx->_trx_id != b_trx->_trx_id)
            return (false);
        
        return (true);
    }
    
}; // EOF trx_packet


struct condex {
    pthread_cond_t _cond;
    pthread_mutex_t _lock;
    bool _fired;
    //    condex() : _fired(false) { pthread_cond_init(&_cond); pthread_mutex_init(&_lock); }
    void signal() {
	CRITICAL_SECTION(cs, _lock);
	_fired = true;
	pthread_cond_signal(&_cond);
    }
    void wait() {
	CRITICAL_SECTION(cs, _lock);
	while(!_fired)
	    pthread_cond_wait(&_cond,&_lock);
	_fired = false;
    }
};

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

    /** Member variables */

    TrxState R_STATE;
    int R_ID;
    condex* _notify;

    
    /** Private access methods */

    inline void set_id(int anID) { R_ID = anID; }

public:

    /** construction - destruction */

    trx_result_tuple_t() { reset(UNDEF, -1, NULL); }

    trx_result_tuple_t(TrxState aTrxState, int anID) { 
        reset(aTrxState, anID, NULL);
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
    void set_notify(condex* notify) { _notify = notify; }
    condex* get_notify() { return _notify; }
    
    inline int get_id() { return (R_ID); }

    inline void set_state(TrxState aState) { 
       assert ((aState >= UNDEF) && (aState <= ROLLBACKED));
       R_STATE = aState;
    }
    
    inline TrxState get_state() { return (R_STATE); }

    inline char* say_state() { return (translate_state(R_STATE)); }

    inline void reset(TrxState aTrxState, int anID, condex* notify=NULL) {
        // check for validity of inputs
        assert ((aTrxState >= UNDEF) && (aTrxState <= ROLLBACKED));
        assert (anID >= NO_VALID_TRX_ID);

        R_STATE = aTrxState;
        R_ID = anID;
	_notify = notify;
    }
        
}; // EOF: trx_result_tuple_t



/**
 *  @brief Definition of a transaction result tuple
 */

class trx_result_process_tuple_t : public process_tuple_t 
{
private:
    trx_packet_t* _packet;

public:

    trx_result_process_tuple_t(trx_packet_t* a_trx_packet) 
        : process_tuple_t() 
    {
        assert(a_trx_packet);
        _packet = a_trx_packet;
    }

    virtual void begin() {
        TRACE( TRACE_QUERY_RESULTS, "*** TRX ***\n");
    }


    //    inline void set_trx_packet(trx_packet_t* a_trx_packet) {
    //        _packet = a_trx_packet;
    //    }


    virtual void process(const tuple_t& output) {

        assert (_packet);

        trx_result_tuple_t* r = aligned_cast<trx_result_tuple_t>(output.data);
        TRACE( TRACE_QUERY_RESULTS, "*** TRX=%d\tRESULT=%s\n",
               r->get_id(),r->say_state());

        // FIXME (ip) Is this assertion correct?
        //        assert (_packet->get_trx_id() == r->get_id());

	//        _packet->set_trx_state(r->get_state());
    }


    virtual ~trx_result_process_tuple_t() { }


}; // EOF trx_result_process_tuple_t


EXIT_NAMESPACE(qpipe);

#endif 
