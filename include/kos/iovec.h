/* KallistiOS ##version##

   kos/iovec.h
   Copyright (C)2001 Dan Potter

*/

/** \file   kos/iovec.h
    \brief  Scatter/Gather arrays.

    This file contains the definition of a scatter/gather array.

    \author Dan Potter
*/

#ifndef __KOS_IOVEC_H
#define __KOS_IOVEC_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <stddef.h>

/** \brief  Scatter/Gather array.
    \headerfile kos/iovec.h
*/
typedef struct iovec {
    char    *iov_base;  /**< \brief Base address */
    size_t  iov_len;    /**< \brief Length */
} iovec_t;

__END_DECLS

#endif	/* __KOS_IOVEC_H */

