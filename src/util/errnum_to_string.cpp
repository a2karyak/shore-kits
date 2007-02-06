
#include "util/errnum_to_string.h"
#include "util/alloc_printf.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>



/* constants */

#define INITIAL_STRERROR_BUFFER_SIZE 100
#define STRERROR_BUFFER_MAX_SIZE     1024



/* helper functions */

char* strerror_copy(int errnum);



/* definitions of exported functions */

char* errnum_to_string(int errnum)
{

  char const* const_errstr = NULL;

#define CASE(errnum) { case errnum: const_errstr = #errnum; break; }
 
  switch (errnum)
  {
    CASE(E2BIG);
    CASE(EACCES);
    CASE(EADDRINUSE);
    CASE(EADDRNOTAVAIL);
    CASE(EAFNOSUPPORT);
    CASE(EAGAIN);
    CASE(EALREADY);
    CASE(EBADE);
    CASE(EBADF);
    CASE(EBADFD);
    CASE(EBADMSG);
    CASE(EBADR);
    CASE(EBADRQC);
    CASE(EBADSLT);
    CASE(EBUSY);
    CASE(ECANCELED);
    CASE(ECHILD);
    CASE(ECHRNG);
    CASE(ECOMM);
    CASE(ECONNABORTED);
    CASE(ECONNREFUSED);
    CASE(ECONNRESET);
    CASE(EDEADLK); /* should be the same as EDEADLOCK */
    CASE(EDESTADDRREQ);
    CASE(EDOM);
    CASE(EDQUOT);
    CASE(EEXIST);
    CASE(EFAULT);
    CASE(EFBIG);
    CASE(EHOSTDOWN);
    CASE(EHOSTUNREACH);
    CASE(EIDRM);
    CASE(EILSEQ);
    CASE(EINPROGRESS);
    CASE(EINTR);
    CASE(EINVAL);
    CASE(EIO);
    CASE(EISCONN);
    CASE(EISDIR);
    /*CASE(EISNAM);*/
    /* CASE(EKEYEXPIRED); */
    /* CASE(EKEYREJECTED); */
    /* CASE(EKEYREVOKED); */
    CASE(EL2HLT);
    CASE(EL2NSYNC);
    CASE(EL3HLT);
    CASE(EL3RST);
    CASE(ELIBACC);
    CASE(ELIBBAD);
    CASE(ELIBMAX);
    CASE(ELIBSCN);
    CASE(ELIBEXEC);
    CASE(ELOOP);
    //    CASE(EMEDIUMTYPE); // not defined
    CASE(EMFILE); // defined twice (?)
    CASE(EMLINK);
    CASE(EMSGSIZE);
    CASE(EMULTIHOP);
    CASE(ENAMETOOLONG);
    CASE(ENETDOWN);
    CASE(ENETRESET);
    CASE(ENETUNREACH);
    CASE(ENFILE);
    CASE(ENOBUFS);
    CASE(ENODATA);
    CASE(ENODEV);
    CASE(ENOENT);
    CASE(ENOEXEC);
    /* CASE(ENOKEY); */
    CASE(ENOLCK);
    CASE(ENOLINK);
    //    CASE(ENOMEDIUM);
    CASE(ENOMEM);
    CASE(ENOMSG);
    CASE(ENONET);
    CASE(ENOPKG);
    CASE(ENOPROTOOPT);
    CASE(ENOSPC);
    CASE(ENOSR);
    CASE(ENOSTR);
    CASE(ENOSYS);
    CASE(ENOTBLK);
    CASE(ENOTCONN);
    CASE(ENOTDIR);
    CASE(ENOTEMPTY);
    CASE(ENOTSOCK);
    CASE(ENOTSUP);
    CASE(ENOTTY);
    CASE(ENOTUNIQ);
    CASE(ENXIO);
    /* CASE(EOPNOTSUPP); */ /* same value as ENOTSUP in Linux */
    CASE(EOVERFLOW);
    //     CASE(EPERM); // defined twice?
    CASE(EPFNOSUPPORT);
    CASE(EPIPE);
    CASE(EPROTO);
    CASE(EPROTONOSUPPORT);
    CASE(EPROTOTYPE);
    CASE(ERANGE);
    CASE(EREMCHG);
    CASE(EREMOTE);
    //    CASE(EREMOTEIO); // not defined
    CASE(ERESTART);
    CASE(EROFS);
    CASE(ESHUTDOWN);
    CASE(ESPIPE);
    CASE(ESOCKTNOSUPPORT);
    CASE(ESRCH);
    CASE(ESTALE);
    CASE(ESTRPIPE);
    CASE(ETIME);
    CASE(ETIMEDOUT);
    CASE(ETXTBSY);
    //    CASE(EUCLEAN); // not defined
    CASE(EUNATCH);
    CASE(EUSERS);
    /* CASE(EWOULDBLOCK); */ /* same value as EAGAIN in Linux */
    CASE(EXDEV);
    CASE(EXFULL);

  default:
    break;
  }


  /* constant error strings */
  if (const_errstr != NULL)
    return alloc_printf(NULL, NULL, "(%s)", const_errstr);


  /* unregistered constants should be dealt with through
     strerror_copy() */
  return strerror_copy(errnum);
}



/* definitions of helper functions */


char* strerror_copy(int errnum)
{

#if 0


  /* For some reason, Linux links in the obsolete strerror_r() which
     returns a char*. It should be using the SUSv3 version which is
     guaranteed to use the user-supplied buffer. This code removed
     with an #if 0 should do the trick when we have the SUSv3
     strerror_r() linked. */
  size_t size = INITIAL_STRERROR_BUFFER_SIZE;
  while (size < STRERROR_BUFFER_MAX_SIZE)
  {
    char errstr[size];
    memset(errstr, 0, sizeof(errstr);

    int strerror_ret = strerror_r(errnum, errstr, size);
    if ((strerror_ret == -1) && (errno == ERANGE))
    {
      size *= 2;
      continue;
    }

    /* The only other error is an invalid errnum value, which should
       never happen. */
    assert(strerror_ret == 0);
    return alloc_printf(NULL, NULL, "(%s)", errstr);
  }

  
  /* If we are here, we are stuck. If this happens, try increasing
     STRERROR_BUFFER_MAX_SIZE. */
  return "UNKNOWN_ERROR";


#else


  char const* strerror_ret = strerror(errnum);
  return alloc_printf(NULL, NULL, "(%s)", strerror_ret);


#endif

}
