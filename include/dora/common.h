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

#include "util/namespace.h"


ENTER_NAMESPACE(dora);


// CONSTANTS

const int DF_CPU_RANGE           = 8;         // cpu range for each table
const int DF_CPU_STARTING        = 2;         // starting cpu
const int DF_CPU_STEP_TABLES     = 16;        // next-cpu among different tables
const int DF_CPU_STEP_PARTITIONS = 2;         // next-cpu among partitions of the same table

const int DF_NUM_OF_PARTITIONS_PER_TABLE = 1; // number of partitions per table
const int DF_NUM_OF_STANDBY_THRS         = 0; // TODO: (ip) assume main-memory 



// CLASSES


class rvp_t;
class terminal_rvp_t;

template<class DataType> class partition_t;
template<class DataType> class dora_worker_t;

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
