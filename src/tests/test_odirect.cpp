/* -*- mode:C++; c-basic-offset:4 -*- */

#include "util.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstdlib>
#include <cerrno>



int main()
{

  thread_init();


  int fd = open("/tmp/odirect_file.txt", O_CREAT|O_WRONLY|O_DIRECT, S_IRUSR|S_IWUSR);
  if (fd == -1) {
      TRACE(TRACE_ALWAYS, "open() failed: %s\n",
            strerror(errno));
      return -1;
  }
  

  write(fd, "hello world", strlen("hello world"));
  

  int fsync_ret = fsync(fd);
  if (fsync_ret == -1) {
      TRACE(TRACE_ALWAYS, "fsync() failed: %s\n",
            strerror(errno));
      return -1;
  }

  TRACE(TRACE_ALWAYS, "Opened file with O_DIRECT flag...\n");

  close(fd);
  return 0;
}
