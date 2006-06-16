/* -*- mode:C++; c-basic-offset:4 -*- */

#ifndef _BNL_CACHING_JOIN_H
#define _BNL_CACHING_JOIN_H

#include <cstdio>
#include <string>

#include "db_cxx.h"
#include "tuple.h"
#include "packet.h"
#include "stage.h"
#include "dispatcher.h"
#include "trace/trace.h"
#include "qpipe_panic.h"



using namespace std;
using namespace qpipe;



/* exported datatypes */


/**
 *  @brief
 */
struct bnl_caching_join_packet_t : public packet_t {
  
    static const char* PACKET_TYPE;

    tuple_join_t* _join;

    tuple_buffer_t* _left_buffer;
    tuple_buffer_t* _right_buffer;

    /**
     *  @brief FSCAN packet constructor. Raw FSCANS should not be
     *  mergeable based on not mergeable. They should be merged at the
     *  meta-stage.
     */
    bnl_caching_join_packet_t(char* packet_id,
			      tuple_buffer_t* out_buffer,
			      tuple_filter_t* filt,
			      tuple_join_t*   join,
			      tuple_buffer_t* client_buffer,
			      tuple_buffer_t* left_buffer,
			      tuple_buffer_t* right_buffer)
	: packet_t(packet_id, PACKET_TYPE, out_buffer, filt, client_buffer, false),
	  _join(join),
	  _left_buffer(left_buffer),
	  _right_buffer(right_buffer)
    {
    }
  
    virtual ~bnl_caching_join_packet_t() { }

    virtual void terminate_inputs() {
	//	_left_buffer->close();
	_right_buffer->close();
    }

};



/**
 *  @brief FSCAN stage. Reads tuples from a file on the local file
 *  system.
 */
class bnl_caching_join_stage_t : public stage_t {

public:

    static const char* DEFAULT_STAGE_NAME;

    bnl_caching_join_stage_t() { }
    
    virtual ~bnl_caching_join_stage_t() { }
    
protected:

    virtual result_t process_packet();

    int create_cache_file(string &name, bnl_caching_join_packet_t* packet);
};



#endif
