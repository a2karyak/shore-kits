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
#include <db_cxx.h>

#include "workload/common/process_tuple.h"
#include "core.h"


// using namespace workload;


ENTER_NAMESPACE(qpipe);


# define NO_VALID_TRX_ID -1


/**
 *  @brief Possible states of a transaction
 */

enum TrxState { UNDEF, UNSUBMITTED, SUBMITTED, POISSONED, 
		COMMITTED, ROLLBACKED };

c_str translate_state(TrxState aState);


/**
 *  @brief Definition of the transactional packets: A transactional packet has 
 *  a unique identifier for the TRX it belongs to.
 */

class trx_packet_t : public packet_t 
{
  
protected:
    
    /** @brief The id of the transaction */
    int _trx_id;

    /** @brief The status of the transaction */
    TrxState _trx_state;

public:


    /** @brief The corresponding transaction handle of this packet */
    DbTxn* _trx_txn;


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
        
        assert (a_trx_id > NO_VALID_TRX_ID);
        
        _trx_id = a_trx_id;
    }

    inline TrxState trx_state() {
        return (_trx_state);
    }
    
    inline void set_trx_state(TrxState a_trx_state) {
        
        assert (a_trx_state > UNDEF);
        
        _trx_state = a_trx_state;
    }

    inline DbTxn* get_trx_txn() {
        return (_trx_txn);
    }
    
    
    inline void set_trx_txn(DbTxn* a_trx_txn) {        
        _trx_txn = a_trx_txn;
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
    
}; /* trx_packet */



/** @class trx_result_tuple
 *  @brief Class used to represent the result of a transaction
 */

class trx_result_tuple {

private:

    /** Member variables */

    TrxState R_STATE;
    int R_ID;

    /** Private access methods */

    inline void set_id(int anID) { R_ID = anID; }

public:

    /** construction - destruction */

    trx_result_tuple() { reset(UNDEF, -1); }

    trx_result_tuple(TrxState aTrxState, int anID) { reset(aTrxState, anID); }

    ~trx_result_tuple() { }

    trx_result_tuple(const trx_result_tuple& t); // copy constructor
    
    trx_result_tuple& operator=(const trx_result_tuple& t); // copy assingment


    /** Access methods */

    inline int get_id() { return (R_ID); }

    inline void set_state(TrxState aState) { 
       assert ((aState >= UNDEF) && (aState <= ROLLBACKED));
       R_STATE = aState;
    }
    
    inline TrxState get_state() { return (R_STATE); }

    inline c_str say_state() { return (translate_state(R_STATE)); }

    inline void reset(TrxState aTrxState, int anID);
    
    
}; // EOF: trx_result_tuple



/**
 *  @brief Definition of a transaction result tuple
 */

class trx_result_process_tuple_t : public process_tuple_t {

private:
    trx_packet_t* _packet;

public:

    trx_result_process_tuple_t(trx_packet_t* a_trx_packet) 
        : process_tuple_t() 
    {
        assert(a_trx_packet != NULL);
        _packet = a_trx_packet;
    }

    virtual void begin() {
        TRACE( TRACE_ALWAYS, "*** TRX ***\n");
    }


    void set_trx_packet(trx_packet_t* a_trx_packet) {
        _packet = a_trx_packet;
    }


    virtual void process(const tuple_t& output) {

        assert (_packet != NULL);

        trx_result_tuple* r = aligned_cast<trx_result_tuple>(output.data);
        TRACE( TRACE_ALWAYS, "*** TRX=%d\tRESULT=%s\n",
               r->get_id(),
               r->say_state().data());

        assert (_packet->get_trx_id() == r->get_id());

        _packet->set_trx_state(r->get_state());
    }


    virtual ~trx_result_process_tuple_t() { }


}; /* trx_result_process_tuple_t */


EXIT_NAMESPACE(qpipe);

#endif 
