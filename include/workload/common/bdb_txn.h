/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file bdb_txn.h
 *
 *  @brief Extension of the DbTxn class in order to allow multiple
 *  threads share the same DbTxn handle.
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __BDB_TXN_H
#define __BDB_TXN_H

#include <db_cxx.h>

#include "util/namespace.h"

ENTER_NAMESPACE(workload);


/** Exported data structures */

/** @note According to BDB documentation DBTxn is not free-threaded.
 *  However, they can be used by multiple threads of control so long
 *  as the application serializes access to the handle. We serialize 
 *  the access acros threads of control by using a corresponding pthread
 *  mutex.
 */
class DbStageTxn {

protected:
  DbTxn* dbtxn;
  pthread_mutex_t dbtxn_mutex;

public:
    
  DbStageTxn();
  ~DbStageTxn();
  
  /** Exported functions */
  void acquire_txn();
  void release_txn();
};

EXIT_NAMESPACE(wokrload);

#endif
