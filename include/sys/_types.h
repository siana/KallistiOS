/** \file   sys/_types.h
    \brief  Internal typedefs.

    This file contains internal typedefs required by libc. You probably
    shouldn't use any of these in your code. Most of these are copied from
    newlib's %sys/_types.h.
*/

#ifndef _SYS__TYPES_H
#define _SYS__TYPES_H

#include <sys/lock.h>

// This part copied from newlib's sys/_types.h.
#ifndef __off_t_defined
/** \brief  File offset type. */
typedef long _off_t;
#endif

#ifndef __dev_t_defined
/** \brief  Device ID type. */
typedef short __dev_t;
#endif

#ifndef __uid_t_defined
/** \brief  User ID type. */
typedef unsigned short __uid_t;
#endif
#ifndef __gid_t_defined
/** \brief  Group ID type. */
typedef unsigned short __gid_t;
#endif

#ifndef __off64_t_defined
/** \brief  64-bit file offset type. */
__extension__ typedef long long _off64_t;
#endif

#ifndef __fpos_t_defined
/** \brief  File position type. */
typedef long _fpos_t;
#endif

#ifdef __LARGE64_FILES
#ifndef __fpos64_t_defined
/** \brief  64-bit file position type. */
typedef _off64_t _fpos64_t;
#endif
#endif

#if defined(__INT_MAX__) && __INT_MAX__ == 2147483647
/** \brief  Signed size type. */
typedef int _ssize_t;
#else
typedef long _ssize_t;
#endif

/** \cond */
#define __need_wint_t
#include <stddef.h>
/** \endcond */

#ifndef __mbstate_t_defined
/** \brief  Conversion state information.
    \headerfile sys/_types.h
*/
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
/** \brief  File lock type. */
typedef __newlib_recursive_lock_t _flock_t;
#endif

#ifndef __iconv_t_defined
/** \brief  Iconv descriptor type. */
typedef void *_iconv_t;
#endif


// This part inserted to fix newlib brokenness.
/** \brief  Size of an fd_set. */
#define FD_SETSIZE 1024

// And this is for old KOS source compatability.
#include <arch/types.h>

// Include stuff to make pthreads work as well.
#include <sys/_pthread.h>

#endif	/* _SYS__TYPES_H */
