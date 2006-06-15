/* -*- mode:C++; c-basic-offset:4 -*- */

#include "stages/bnl_caching_join.h"
#include "stages/fdump.h"
#include "qpipe_panic.h"
#include "trace/trace.h"
#include "tuple.h"



const char* bnl_caching_join_packet_t::PACKET_TYPE = "BNL_CACHING_JOIN";



const char* bnl_caching_join_stage_t::DEFAULT_STAGE_NAME = "BNL_CACHING_JOIN_STAGE";



/**
 *  @brief Join the left and right relations.
 *
 *  @return 0 on success. Non-zero on unrecoverable error. The stage
 *  should terminate all queries it is processing.
 */

stage_t::result_t bnl_caching_join_stage_t::process_packet() {

    adaptor_t* adaptor = _adaptor;
    bnl_caching_join_packet_t* packet = (bnl_caching_join_packet_t*)adaptor->get_packet();


    int right_tuple_size = packet->_right_buffer->tuple_size;
    string cache_filename;
    if ( create_cache_file(cache_filename, packet) ) {
	TRACE(TRACE_ALWAYS, "create_cache_file() failed\n");
	return stage_t::RESULT_ERROR;
    }
    TRACE(TRACE_ALWAYS, "Dumped right subtree into cache file %s\n", cache_filename.c_str());
    

    tuple_buffer_t* input_buffer = packet->_left_buffer;
    input_buffer->init_buffer();


    // read the file; make sure the buffer pages get deleted
    page_guard_t tuple_page;
    while ( (tuple_page = input_buffer->get_page()) != NULL ) {
	TRACE(TRACE_ALWAYS, "Read another page of %d tuples\n",
	      tuple_page->tuple_count());
    }


    return stage_t::RESULT_STOP;
}



/**
 *  @brief Cache the sequence of tuples from the right subtree.
 *
 *  @param name On success, the name of cache file will be stored
 *  here.
 *
 *  @return 0 on success. Non-zero on error.
 */
int bnl_caching_join_stage_t::create_cache_file(string &name, bnl_caching_join_packet_t* packet) {


    // create temporary cache file
    char name_template[] = "tmp/bnl-cache.XXXXXX";
    int fd = mkstemp(name_template);
    if(fd < 0) {
        TRACE(TRACE_ALWAYS, "mkstemp() failed on %s\n",
              name_template);
	return -1;
    }
    if ( close(fd) ) {
	TRACE(TRACE_ALWAYS, "fclose() failed on temp file %s\n",
	      name_template);
	return -1;
    }
   
    
    // dump the right subtree into a cache file
    tuple_buffer_t signal_buffer(sizeof(int));
    fdump_packet_t* fdump_packet =
	new fdump_packet_t("BNL_CACHING_JOIN_FDUMP_PACKET",
			   &signal_buffer,
			   packet->_right_buffer,
			   packet->client_buffer,
			   name_template);
    dispatcher_t::dispatch_packet(fdump_packet);
    if ( !signal_buffer.wait_for_input() ) {
	// should never happen
	TRACE(TRACE_ALWAYS, "signal_buffer.wait_for_input() returning data!\n");
	QPIPE_PANIC();
    }

   
    name = string(name_template);
    return 0;
}
