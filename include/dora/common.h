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



EXIT_NAMESPACE(dora);

#endif /* __DORA_COMMON_H */
