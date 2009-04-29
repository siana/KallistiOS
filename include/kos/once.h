/* KallistiOS ##version##

   include/kos/once.h
   Copyright (C) 2009 Lawrence Sebald

*/

#ifndef __KOS_ONCE_H
#define __KOS_ONCE_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <sys/queue.h>

typedef int kthread_once_t;

#define KTHREAD_ONCE_INIT 0

/* Run a function once. Returns -1 on failure.
    ENOMEM - Out of memory
    EPERM - called inside an interrupt
    EINTR - was interrupted */
int kthread_once(kthread_once_t *once_control, void (*init_routine)(void));

#endif /* !__KOS_ONCE_H */
