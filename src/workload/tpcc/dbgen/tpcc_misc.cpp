/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file tpccmisc.cpp
 *
 *  @brief Misc routines for the TPC-C data table generation
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <sys/time.h>

#include "workload/tpcc/dbgen/tpcc_misc.h"

// fn: Current time in SECONDS, precision SECONDS
double current_time(void) {
  /* use time() to get seconds */
  return(time(NULL));
}

// fn: Current time in SECONDS, precision MILLISECONDS
double current_time_ms(void) {

  /* gettimeofday() returns seconds and microseconds */
  /* convert to fractional seconds */
  struct timeval t;
  gettimeofday(&t,NULL);
  return (t.tv_sec + (double)t.tv_usec/(1000*1000));

}
