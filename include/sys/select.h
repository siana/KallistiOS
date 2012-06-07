/* KallistiOS ##version##

   sys/select.h
   Copyright (C) 2012 Lawrence Sebald

*/

/** \file   select.h
    \brief  Definitions for the select() function.

    This file contains the definitions needed for using the select() function,
    as directed by the POSIX 2008 standard (aka The Open Group Base
    Specifications Issue 7). Currently the functionality defined herein only
    really works for sockets, and that is likely how it will stay for some time.
 
    \author Lawrence Sebald
*/

#ifndef __SYS_SELECT_H
#define __SYS_SELECT_H

#include <sys/cdefs.h>
#include <sys/types.h>

__BEGIN_DECLS

#include <time.h>

/* <sys/types.h> defines fd_set and friends for us, so there's really not much
   that we have to do here... */

/** \brief  Timeout value for the select() function.
    \headerfile sys/select.h
*/
struct timeval {
    time_t tv_sec;          /**< \brief Seconds */
    suseconds_t tv_usec;    /**< \brief Microseconds */
};

int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *errorfds,
           struct timeval *timeout);

__END_DECLS

#endif /* !__SYS_SELECT_H */
