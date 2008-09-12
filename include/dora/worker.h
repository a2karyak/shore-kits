/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file worker.h
 *
 *  @brief Wrapper for the worker threads in DORA
 *
 *  @author Ippokratis Pandis (ipandis)
 */


#ifndef __DORA_WORKER_H
#define __DORA_WORKER_H

#include <cstdio>

#include "util.h"
#include "sm/shore/shore_env.h"


ENTER_NAMESPACE(dora);


using namespace shore;



/******************************************************************** 
 *
 * @enum  WorkerState
 *
 * @brief Possible states of a worker thread
 *
 ********************************************************************/

enum WorkerState { UNDEF, ALONE, MULTIPLE };


/******************************************************************** 
 *
 * @class worker_t
 *
 * @brief An smthread-based class for the worker threads
 *
 ********************************************************************/

class worker_t : public thread_t 
{
private:

    ShoreEnv*   _env;    
    WorkerState _state;


public:

    worker_t(ShoreEnv* env, 
             c_str tname) 
      : thread_t(tname), _env(env)
    {
        assert (_env);
    }

    ~worker_t() { }


    // thread entrance
    void work();

}; // EOF: worker_t



EXIT_NAMESPACE(dora);

#endif /** __DORA_WORKER_H */

