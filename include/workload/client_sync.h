/* -*- mode:c++; c-basic-offset:4 -*- */

#ifndef _CLIENT_SYNC_H
#define _CLIENT_SYNC_H



// include me last!!!
#include "engine/namespace.h"



/**
 *  @brief The synchronization between the thread that runs the
 *  workload (the runner) and the workload client threads (clients)
 *  can get pretty messy. For example, the runner creates the clients
 *  one at a time. If there is an error creating a client, all the
 *  clients which have been created so far must be terminated. We
 *  created the client_wait_t class to encapsulate this
 *  interaction. The runner creates a client_wait_t instance and
 *  passes a reference to each client thread, which invokes the
 *  wait_for_runner() method. If wait_for_runner() returns true, the
 *  client can continue. If it returns false, the client thread should
 *  simply exit.
 */
class client_wait_t {
public:
    virtual bool wait_for_runner()=0;
    virtual ~client_wait_t() { }
};
  


/**
 *  @brief The implementation of a client_wait_t. We separate the
 *  client interface from the server one so the client can't call
 *  methods its not supposed to call.
 */
class client_sync_t : public client_wait_t {
   
private:

    enum state_t {
        CLIENT_SYNC_STATE_WAIT,
        CLIENT_SYNC_STATE_ERROR,
        CLIENT_SYNC_STATE_CONTINUE
    };

    state_t          _state;
    pthread_mutex_t  _state_mutex;
    pthread_cond_t   _state_cond;

public:

    client_sync_t() :
        _state(CLIENT_SYNC_STATE_WAIT)
    {
        pthread_mutex_init_wrapper(&_state_mutex, NULL);
        pthread_cond_init_wrapper (&_state_cond,  NULL);
    }

    
    virtual ~client_sync_t() {
        pthread_mutex_destroy_wrapper(&_state_mutex);
        pthread_cond_destroy_wrapper (&_state_cond);
    }
    
    
    virtual bool wait_for_runner() {
        
        // wait for state to change from WAIT
        pthread_mutex_lock_wrapper(&_state_mutex);
        while ( _state == CLIENT_SYNC_STATE_WAIT ) {
            pthread_cond_wait_wrapper(&_state_cond, &_state_mutex);
        }
        
        // success (true) if and only if state changed to CONTINUE
        bool ret = _state == CLIENT_SYNC_STATE_CONTINUE;
        pthread_mutex_unlock_wrapper(&_state_mutex);
     
        return ret;
    }


    void signal_continue() {
        critical_section_t cs(&_state_mutex);
        _state = CLIENT_SYNC_STATE_CONTINUE;
        pthread_cond_broadcast_wrapper(&_state_cond);
    }
    
    
    void signal_error() {
        critical_section_t cs(&_state_mutex);
        _state = CLIENT_SYNC_STATE_ERROR;
        pthread_cond_broadcast_wrapper(&_state_cond);
    }

};



// include me last!!!
#include "engine/namespace.h"



#endif
