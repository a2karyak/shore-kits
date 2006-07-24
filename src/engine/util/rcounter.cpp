/* -*- mode:C++; c-basic-offset:4 -*- */

#include "engine/util/rcounter.h"



rcounter_t::rcounter_t(int initial_count=0) {

  pthread_mutex_init_wrapper(&_mutex, NULL);
  pthread_cond_init_wrapper (&_cond,  NULL);
  _count = initial_count;
}



rcounter_t::~rcounter_t() {

  pthread_mutex_destroy_wrapper(&_mutex);
  pthread_cond_destroy_wrapper (&_cond);
}



void rcounter_t::increment() {
  critical_section_t cs(&_mutex);
  if (++_count == 0)
    pthread_cond_signal_wrapper(&_cond);
}



void rcounter_t::decrement() {
  critical_section_t cs(&_mutex);
  if (--_count == 0)
    pthread_cond_signal_wrapper(&_cond);
}



void rcounter_t::wait_for_zero() {
  // Looks weird using critical_section_t object. Use raw pthread
  // wrappers instead.
  pthread_mutex_lock_wrapper(&_mutex);
  while ( _count != 0 ) {
    pthread_cond_wait_wrapper(&_cond, &_mutex);
  }
  pthread_mutex_unlock_wrapper(&_mutex);
}
