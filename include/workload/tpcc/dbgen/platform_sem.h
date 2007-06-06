/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file platform_sem.h
 *
 *  @brief Platform independent Semaphore macros
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __PLATFORM_SEM_H
#define __PLATFORM_SEM_H


// Semaphore Macros
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

union semun {
  int val;
  struct semid_ds *buf;
  unsigned short int *array;
} semUnion;

struct sembuf semBuf;

#define SEM_HANDLE int

#define SEM_INIT(hnd, x, name)                                            \
   if ( (hnd = semget(IPC_PRIVATE, 1 , IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)) == -1)                          \
      API_ERROR(__LINE__, "semget", (rc=GEN_ERRCODE));                    \
   semUnion.val = x;                                                      \
   if ( semctl(hnd, 0, SETVAL, semUnion) < 0 )                            \
      API_ERROR(__LINE__, "semctl SETVAL", (rc=GEN_ERRCODE));         

#define SEM_WAIT(hnd)                                                     \
   semBuf.sem_num = 0;                                                    \
   semBuf.sem_op = -1;                                                    \
   semBuf.sem_flg = SEM_UNDO;                                             \
   if ( semop(hnd, &semBuf, 1) < 0 )                                      \
      API_ERROR(__LINE__, "semop wait", (rc=GEN_ERRCODE));

#define SEM_FREE(hnd)                                                     \
   semBuf.sem_num = 0;                                                    \
   semBuf.sem_op = 1;                                                     \
   semBuf.sem_flg = SEM_UNDO;                                             \
   if ( semop(hnd, &semBuf, 1) < 0 )                                      \
      API_ERROR(__LINE__, "semop free", (rc=GEN_ERRCODE));

#define SEM_DESTROY(hnd)                                                  \
   if ( semctl(hnd, 0, IPC_RMID, 0) )                                     \
      API_ERROR(__LINE__, "semctl IPC_RMID", (rc=GEN_ERRCODE)); 



#endif // __PLATFORM_SEM_H
