/* -*- mode:C++; c-basic-offset:4 -*- */

/** @file platform_io.h
 *
 *  @brief Platform independent I/O macros
 *
 *  @author Ippokratis Pandis (ipandis)
 */

#ifndef __PLATFORM_IO_H
#define __PLATFORM_IO_H


#include <unistd.h>
#include <fcntl.h>


#define GEN_ERRCODE	errno

// I/O Macros
#define IOH_INIT(hnd, type, name)                                          \
   hnd->fd = -1;                                                           \
   hnd->type = type;                                                       \
   hnd->name = name;

#define IOH_CREATE(hnd)                                                    \
   if (hnd->type == IOH_PIPE) {                                            \
      rc = mkfifo(hnd->name, 0666);                                        \
   } else {                                                                \
      rc = 0;                                                              \
   }

#define IOH_OPEN(hnd)                                                      \
   if (hnd->type == IOH_FILE_APPEND) {                                     \
     hnd->fd = open(hnd->name, O_WRONLY | O_CREAT | O_APPEND, 0666);       \
   } else {                                                                \
     hnd->fd = open(hnd->name, O_WRONLY | O_CREAT | O_TRUNC, 0666);        \
   }                                                                       \
   if (hnd->fd == -1) {                                                    \
      rc = -1;                                                             \
   } else {                                                                \
      rc = 0;                                                              \
   }

#define IOH_WRITE(hnd, buff, num, num2)                                    \
   rc = write(hnd->fd, buff, num);                                         \
   if (rc >= 0) {                                                          \
      num2 = rc;                                                           \
      rc = 0;                                                              \
   }

#define IOH_FLUSH(hnd)	rc = 0;
#define IOH_CLOSE(hnd) 	rc = close(hnd->fd);
#define IOH_DELETE(hnd)	if (hnd->type == IOH_PIPE) { rc = unlink(hnd->name); }

typedef unsigned int IOH_NUM;
typedef int IOH_HND;



// Common I/O Macros and Definitions
#define IOH_FILE 1
#define IOH_PIPE 2
#define IOH_FILE_APPEND 3

#define IOH_ERRMSG(hnd, msg)                                               \
  if (rc != 0) {                                                           \
    fprintf(stderr, "Error %d %s fd %d (%d, %s)\n", GEN_ERRCODE, msg,      \
                     hnd->fd, hnd->type, hnd->name);                       \
    return rc;                                                             \
  }

struct _ioh {
  IOH_HND fd;
  int type;
  char *name;
};

typedef struct _ioh ioHandle;


// Generic I/O Routine Prototypes
int GenericOpen(ioHandle *hnd, int type, char *name);
int GenericWrite(ioHandle *hnd, char *Buffer, unsigned int numBytes);
int GenericClose(ioHandle *hnd);


// Generic I/O Routines
int GenericOpen(ioHandle *hnd, int type, char *name) {
   int rc = 0;

   IOH_INIT(hnd, type, name)

   IOH_CREATE(hnd)
   IOH_ERRMSG(hnd, "creating")

   IOH_OPEN(hnd)
   IOH_ERRMSG(hnd, "opening")

   return rc;
}


int GenericWrite(ioHandle *hnd, char *Buffer, unsigned int numBytes) {
   int rc = 0;
   int numBytesWritten = -1;

   IOH_WRITE(hnd, Buffer, numBytes, numBytesWritten)
   IOH_ERRMSG(hnd, "writing")
   if (numBytes != numBytesWritten) {
      fprintf(stderr, 
              "Truncated data writing to fd %d (%d, %s)\n", 
              hnd->fd, hnd->type, hnd->name);
      rc = -1;
   }
   return rc;
}


int GenericClose(ioHandle *hnd) {
   int rc = 0;

   IOH_FLUSH(hnd)
   IOH_ERRMSG(hnd, "flushing")

   IOH_CLOSE(hnd)
   IOH_ERRMSG(hnd, "closing")
 
   IOH_DELETE(hnd)
   IOH_ERRMSG(hnd, "deleting")

   return rc;
}

#endif // __PLATFORM_IO_H
