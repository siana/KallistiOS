/* KallistiOS ##version##

   kos/cdefs.h
   Copyright (C)2002,2004 Dan Potter

   Based loosely around some stuff in BSD's sys/cdefs.h
*/

#ifndef __KOS_CDEFS_H
#define __KOS_CDEFS_H

#include <sys/cdefs.h>

/* Check GCC version */
#ifndef _arch_ps2
#	if __GNUC__ < 2
#		warning Your GCC is too old. This will probably not work right.
#	endif

#	if __GNUC__ == 2 && __GNUC_MINOR__ < 97
#		warning Your GCC is too old. This will probably not work right.
#	endif
#endif	/* _arch_ps2 */

/* Special function/variable attributes */
#define __noreturn	__attribute__((__noreturn__))
#define __pure		__attribute__((__const__))
#define __unused	__attribute__((__unused__))

#define __dead2		__noreturn	/* BSD compat */
#define __pure2		__pure		/* ditto */

/* Printf/Scanf-like declaration */
#define __printflike(fmtarg, firstvararg) \
	__attribute__((__format__ (__printf__, fmtarg, firstvararg)))

#define __scanflike(fmtarg, firstvararg) \
	__attribute__((__format__ (__printf__, fmtarg, firstvararg)))

/* GCC macros for special cases */
/* #if __GNUC__ ==  */

#endif	/* __KOS_CDEFS_H */


