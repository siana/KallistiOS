#ifndef _SYS__TYPES_H
#define _SYS__TYPES_H

#include <sys/lock.h>

// This part copied from newlib's sys/_types.h.
#ifndef __off_t_defined
typedef long _off_t;
#endif

#ifndef __dev_t_defined
typedef short __dev_t;
#endif

#ifndef __uid_t_defined
typedef unsigned short __uid_t;
#endif
#ifndef __gid_t_defined
typedef unsigned short __gid_t;
#endif

#ifndef __off64_t_defined
__extension__ typedef long long _off64_t;
#endif

#ifndef __fpos_t_defined
typedef long _fpos_t;
#endif

#ifdef __LARGE64_FILES
#ifndef __fpos64_t_defined
typedef _off64_t _fpos64_t;
#endif
#endif

#if defined(__INT_MAX__) && __INT_MAX__ == 2147483647
typedef int _ssize_t;
#else
typedef long _ssize_t;
#endif

#define __need_wint_t
#include <stddef.h>

#ifndef __mbstate_t_defined
/* Conversion state information.  */
typedef struct
{
  int __count;
  union
  {
    wint_t __wch;
    unsigned char __wchb[4];
  } __value;            /* Value so far.  */
} _mbstate_t;
#endif

#ifndef __flock_t_defined
typedef __newlib_recursive_lock_t _flock_t;
#endif

#ifndef __iconv_t_defined
/* Iconv descriptor type */
typedef void *_iconv_t;
#endif


// This part inserted to fix newlib brokenness.
#define FD_SETSIZE 1024

// And this is for old KOS source compatability.
#include <arch/types.h>

// Include stuff to make pthreads work as well.
#include <sys/_pthread.h>

#endif	/* _SYS__TYPES_H */
