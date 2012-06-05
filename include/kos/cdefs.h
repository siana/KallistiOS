/* KallistiOS ##version##

   kos/cdefs.h
   Copyright (C)2002,2004 Dan Potter

   Based loosely around some stuff in BSD's sys/cdefs.h
*/

/** \file   kos/cdefs.h
    \brief  Potentially useful definitions for C Programs.

    This file contains definitions of various __attribute__ directives in
    shorter forms for use in programs (to aid in optimization, mainly).

    \author Dan Potter
*/

#ifndef __KOS_CDEFS_H
#define __KOS_CDEFS_H

#include <sys/cdefs.h>

/* Check GCC version */
#ifndef _arch_ps2
#   if __GNUC__ < 2
#       warning Your GCC is too old. This will probably not work right.
#   endif

#   if __GNUC__ == 2 && __GNUC_MINOR__ < 97
#       warning Your GCC is too old. This will probably not work right.
#   endif
#endif  /* _arch_ps2 */

/* Special function/variable attributes */

/** \brief  Identify a function that will never return. */
#define __noreturn  __attribute__((__noreturn__))

/** \brief  Identify a function that has no side effects other than its return,
            and only uses its arguments for any work. */
#define __pure      __attribute__((__const__))

/** \brief  Identify a function or variable that may be unused. */
#define __unused    __attribute__((__unused__))

/** \brief  Alias for \ref __noreturn. For BSD compatibility. */
#define __dead2     __noreturn  /* BSD compat */

/** \brief  Alias for \ref __pure. Fore BSD compatibility. */
#define __pure2     __pure      /* ditto */

/* Printf/Scanf-like declaration */
/** \brief  Identify a function as accepting formatting like printf().

    Using this macro allows GCC to typecheck calls to printf-like functions,
    which can aid in finding mistakes.

    \param  fmtarg          The argument number (1-based) of the format string.
    \param  firstvararg     The argument number of the first vararg (the ...).
*/
#define __printflike(fmtarg, firstvararg) \
    __attribute__((__format__ (__printf__, fmtarg, firstvararg)))

/** \brief  Identify a function as accepting formatting like scanf().

    Using this macro allows GCC to typecheck calls to scanf-like functions,
    which can aid in finding mistakes.

    \param  fmtarg          The argument number (1-based) of the format string.
    \param  firstvararg     The argument number of the first vararg (the ...).
*/
#define __scanflike(fmtarg, firstvararg) \
    __attribute__((__format__ (__scanf__, fmtarg, firstvararg)))

/* GCC macros for special cases */
/* #if __GNUC__ ==  */

#endif  /* __KOS_CDEFS_H */


