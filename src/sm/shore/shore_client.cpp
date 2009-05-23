/* -*- mode:C++; c-basic-offset:4 -*-
     Shore-kits -- Benchmark implementations for Shore-MT
   
                       Copyright (c) 2007-2009
      Data Intensive Applications and Systems Labaratory (DIAS)
               Ecole Polytechnique Federale de Lausanne
   
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

/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   shore_client_env.cpp
 *
 *  @brief:  Implementation base class for Shore clients
 *
 *  @author: Ippokratis Pandis, July 2008
 */

#include "sm/shore/shore_client.h"

ENTER_NAMESPACE(shore);


/********************************************************************* 
 *
 *  @fn:    abort/resume_test
 *
 *  @brief: Used for SIGINT functionality
 *
 *********************************************************************/

static bool _abort_test = false;
void base_client_t::abort_test() {
    _abort_test = true;
}
void base_client_t::resume_test() {
    _abort_test = false;
}
bool base_client_t::is_test_aborted() {
    return _abort_test;
}



static pthread_mutex_t client_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t client_cond = PTHREAD_COND_INITIALIZER;
static int client_needed_count;
extern void shell_expect_clients(int count) {
    client_needed_count = count;
}
extern void shell_await_clients() {
    CRITICAL_SECTION(cs, client_mutex);
    while(client_needed_count)
	pthread_cond_wait(&client_cond, &client_mutex);
}
void client_ready() {
    CRITICAL_SECTION(cs, client_mutex);
    if(! --client_needed_count)
	pthread_cond_signal(&client_cond);
}

/********************************************************************* 
 *
 *  @fn:    run_xcts
 *
 *  @brief: Entry point for running the trxs 
 *
 *********************************************************************/

w_rc_t base_client_t::run_xcts(int xct_type, int num_xct)
{
    int i=0;

    client_ready();

    _env->env_thread_init();
    
    // retrieve the default think time
    _think_time = envVar::instance()->getVarInt("db-cl-thinktime",THINK_TIME);

    bool time_based = _measure_type == MT_TIME_DUR;
    assert(time_based || _measure_type == MT_NUM_OF_TRXS);
    
    while( (time_based? _env->get_measure() != MST_DONE : i < num_xct)
	   && !is_test_aborted())
	{
	    if(_env->get_measure() == MST_PAUSE) {
		usleep(10000);
		continue;
	    }
	    
	    run_one_xct(xct_type, i++);
	    
	    // exponentially-distributed think time
	    if(_think_time > 0)
		usleep(int(-_think_time*::log(sthread_t::drand())));
	}

    _env->env_thread_fini();
    
    return (RCOK);
}


EXIT_NAMESPACE(shore);





