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

const int DF_CPU_RANGE           = 4;         // cpu range for each table
const int DF_CPU_STEP_TABLES     = 4;         // next-cpu among different tables
const int DF_CPU_STEP_PARTITIONS = 1;         // next-cpu among partitions of the same table

const int DF_NUM_OF_PARTITIONS_PER_TABLE = 1; // number of partitions per table
const int DF_NUM_OF_STANDBY_THRS         = 0; // TODO: (ip) assume main-memory 



// CLASSES

template<class DataType> class partition_t;


// ENUMS


/******************************************************************** 
 *
 * @enum:  WorkerControl
 *
 * @brief: States for controling a worker thread
 *
 * @note:  A thread initially is paused then it starts working until
 *         someone changes the status to stopped.
 *
 ********************************************************************/

enum WorkerControl { WC_PAUSED, WC_ACTIVE, WC_STOPPED };


/******************************************************************** 
 *
 * @enum:  DataOwnerState
 *
 * @brief: Owning status for the worker thread of the data partition
 *
 ********************************************************************/

enum DataOwnerState { DOS_UNDEF, DOS_ALONE, DOS_MULTIPLE };



/******************************************************************** 
 *
 * @enum:  PartitionPolicy
 *
 * @brief: Possible types of a data partition
 *
 *         - PP_RANGE  : range partitioning
 *         - PP_HASH   : hash-based partitioning
 *         - PP_PREFIX : prefix-based partitioning (predicate)
 *
 ********************************************************************/

enum PartitionPolicy { PP_UNDEF, PP_RANGE, PP_HASH, PP_PREFIX };




EXIT_NAMESPACE(dora);

#endif /* __DORA_COMMON_H */
