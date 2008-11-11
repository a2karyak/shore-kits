/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file:   common.h
 *
 *  @brief:  Common classes and enums
 *
 *  @author: Ippokratis Pandis, Oct 2008
 *
 */

#ifndef __DORA_COMMON_H
#define __DORA_COMMON_H

#include "sm_vas.h"

#include "util/namespace.h"



ENTER_NAMESPACE(dora);


// CONSTANTS

const int DF_CPU_RANGE           = 4;         // cpu range for each table
const int DF_CPU_STEP_TABLES     = 4;         // next-cpu among different tables
const int DF_CPU_STEP_PARTITIONS = 1;         // next-cpu among partitions of the same table

const int DF_NUM_OF_PARTITIONS_PER_TABLE = 1; // number of partitions per table
const int DF_NUM_OF_STANDBY_THRS         = 0; // TODO: (ip) assume main-memory 



// CLASSES


class rvp_t;
class terminal_rvp_t;

template<class DataType> class partition_t;

class base_worker_t;
template<class DataType> class worker_t;

class base_action_t;
template<class DataType> class action_t;



// ENUMS


/******************************************************************** 
 *
 * @enum:  eActionDecision
 *
 * @brief: Possible decision of an action
 *
 * @note:  Abort is decided when something goes wrong with own action
 *         Die if some other action (of the same trx) decides to abort
 *
 ********************************************************************/

enum eActionDecision { AD_UNDECIDED, AD_ABORT, AD_DEADLOCK, AD_COMMIT, AD_DIE };



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



/******************************************************************** 
 *
 * @enum:  ePartitionPolicy
 *
 * @brief: Possible types of a data partition
 *
 *         - PP_RANGE  : range partitioning
 *         - PP_HASH   : hash-based partitioning
 *         - PP_PREFIX : prefix-based partitioning (predicate)
 *
 ********************************************************************/

enum ePartitionPolicy { PP_UNDEF, PP_RANGE, PP_HASH, PP_PREFIX };




EXIT_NAMESPACE(dora);

#endif /* __DORA_COMMON_H */
