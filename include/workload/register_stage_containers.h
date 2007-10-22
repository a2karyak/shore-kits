/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file register_stage_containers.h
 *
 *  @brief Definition of the function for the  registration of the stages
 *  on the system, given the environment
 */

#ifndef _REGISTER_STAGE_CONTAINERS_H
#define _REGISTER_STAGE_CONTAINERS_H

// Engine environment

// DSS environment
#define QUERY_ENV     1

// variations of OLTP environments
#define TRX_BDB_ENV   2
#define TRX_SHORE_ENV 3
#define TRX_MEM_ENV   4

void register_stage_containers(int environment);

#endif // _REGISTER_STAGE_CONTAINERS_H
