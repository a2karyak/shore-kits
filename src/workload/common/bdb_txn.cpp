/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file bdb_txn.cpp
 *
 *  @brief Implementation of the extension of the DbTxn class 
 *  in order to allow multiple threads share the same DbTxn handle.
 *
 *  @author Ippokratis Pandis (ipandis)
 */


#include "workload/common/bdb_txn.h"

ENTER_NAMESPACE(workload);

DbStageTxn::DbStageTxn() {

  // Initialize the mutex
  pthread_mutex_init(&dbtxn_mutex, NULL);
}



DbStageTxn::~DbStageTxn() {

  // Destroy the mutex
  pthread_mutex_destroy(&dbtxn_mutex);
}



/** @fn acquire_txn()
 *  @brief Get the corresponding lock
 */

void DbStageTxn::acquire_txn() {

  pthread_mutex_lock(&dbtxn_mutex);
}



/** @fn release_txn()
 *  @brief Release the corresponding lock
 */

void DbStageTxn::release_txn() {

  pthread_mutex_unlock(&dbtxn_mutex);
}


EXIT_NAMESPACE(workload);
