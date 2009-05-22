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

/** @file:   common.h
 *
 *  @brief:  Common classes and enums
 *
 *  @author: Ippokratis Pandis, Oct 2008
 *
 */

#ifndef __SHORE_COMMON_H
#define __SHORE_COMMON_H

#include "sm_vas.h"

#include "util/namespace.h"



ENTER_NAMESPACE(shore);




// CLASSES


class base_worker_t;
class base_action_t;



// ENUMS



/******************************************************************** 
 *
 * @enum:  eWorkerControl
 *
 * @brief: States for controling a worker thread 
 *
 * @note:  A thread initially is paused then it starts working until
 *         someone changes the status to stopped.
 *
 ********************************************************************/

enum eWorkerControl { WC_PAUSED, WC_ACTIVE, WC_STOPPED };




/******************************************************************** 
 *
 * @enum:  eWorkingState
 *
 * @brief: Possible states while worker thread active
 *
 * States:
 *
 * UNDEF     = Unitialized
 * LOOP      = Idle and spinning
 * SLEEP     = Sleeping on the condvar
 * COMMIT_Q  = Serving to-be-committed requests
 * INPUT_Q   = Serving a normal request
 * FINISHED  = Done assigned job but still trx not decided
 *
 ********************************************************************/

enum eWorkingState { WS_UNDEF, 
                     WS_LOOP, 
                     WS_SLEEP, 
                     WS_COMMIT_Q, 
                     WS_INPUT_Q,
                     WS_FINISHED };


/******************************************************************** 
 *
 * @enum:  eDataOwnerState
 *
 * @brief: Owning status for the worker thread of the data partition
 *
 ********************************************************************/

enum eDataOwnerState { DOS_UNDEF, DOS_ALONE, DOS_MULTIPLE };



EXIT_NAMESPACE(shore);

#endif /* __SHORE_COMMON_H */
