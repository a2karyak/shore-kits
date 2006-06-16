/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _DISPATCHER_H
#define _DISPATCHER_H

#include "thread.h"
#include "stage_container.h"
#include "util/static_hash_map_struct.h"



// include me last!!!
#include "lib/namespace.h"



/* exported constants */

#define DISPATCHER_NUM_STATIC_HASH_MAP_BUCKETS 64



/* exported datatypes */


class stage_t;
class packet_t;

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
    pthread_mutex_t directory_lock;

    // stage directory
    struct static_hash_map_s  stage_directory;
    struct static_hash_node_s stage_directory_buckets[DISPATCHER_NUM_STATIC_HASH_MAP_BUCKETS];

    dispatcher_t();
    ~dispatcher_t();
    
    // methods
    void _register_stage_container(const char* packet_type, stage_container_t* sc);
    void _dispatch_packet(packet_t* packet);
    
    
    static dispatcher_t* _instance;
    static dispatcher_t* instance() {
    	if ( _instance == NULL ) {
	    _instance = new dispatcher_t();
	}
	return _instance;
    }
    
public:

    static void register_stage_container(const char* packet_type, stage_container_t* sc);
    static void dispatch_packet(packet_t* packet);
};



#include "lib/namespace.h"
#endif
