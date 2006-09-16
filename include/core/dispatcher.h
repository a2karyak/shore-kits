/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _DISPATCHER_H
#define _DISPATCHER_H

#include "core/tuple.h"
#include "core/packet.h"
#include "core/stage_container.h"


ENTER_NAMESPACE(qpipe);


/* exported constants */

static const int DISPATCHER_NUM_STATIC_HASH_MAP_BUCKETS = 64;



typedef struct dispatcher_cpu_s* dispatcher_cpu_t;


/* exported functions */
/**
 *  @brief Exports dispatcher_cpu_t datatype. QPIPE worker threads may
 *  invoke dispatcher_cpu_bind_self() to bind themselves to the
 *  specified CPU.
 */
void dispatcher_cpu_bind_self(dispatcher_cpu_t cpu);
int  dispatcher_cpu_get_unique_id(dispatcher_cpu_t cpu);



/* exported datatypes */

/**
 *  @brief QPIPE dispatcher that dispatches all packets. All stages
 *  should register themselves with the dispatcher on startup. This
 *  registration should all be done before creating any additional
 *  threads.
 *
 *  Currently, the dispatcher is a singleton, but we provide static
 *  wrappers around our methods to avoid littering the code with
 *  dispatcher_t::instance() calls.
 */

class dispatcher_t {

protected:

    // synch vars

    // stage directory
    struct static_hash_map_s  stage_directory;
    struct static_hash_node_s stage_directory_buckets[DISPATCHER_NUM_STATIC_HASH_MAP_BUCKETS];

   
    dispatcher_t();
    ~dispatcher_t();

    
    // methods
    void _register_stage_container(const c_str &packet_type, stage_container_t* sc);
    void _dispatch_packet(packet_t* packet);
    
    
    static pthread_mutex_t _instance_lock;
    static dispatcher_t*   _instance;

    static dispatcher_t* instance() {
        
        critical_section_t cs(_instance_lock);
        if ( _instance == NULL )
            _instance = new dispatcher_t();

        // TODO Move over to a static init() function that is
        // guaranteed to be called by the root thread on startup. Then
        // this function can simply assert that _instance has been
        // initialized.
	return _instance;
    }
    
public:

    static void init() {
        _instance = new dispatcher_t();
    }

    static void register_stage_container(const c_str &packet_type, stage_container_t* sc);
    
    template<template<class> class Guard>
    static void dispatch_packet(Guard<packet_t> &packet) {
        dispatch_packet(packet.release());
    }
    
    static void dispatch_packet(packet_t* packet);
};


EXIT_NAMESPACE(qpipe);



#endif
