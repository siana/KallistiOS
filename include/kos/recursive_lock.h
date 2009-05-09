/* KallistiOS ##version##
 
   include/kos/recursive_lock.h
   Copyright (C) 2008 Lawrence Sebald
 
*/

/* This file defines a recursive lock mechanism. Basically, think of these as
   a mutex that a single thread can acquire as many times as it wants, but no
   other threads can acquire. */

#ifndef __KOS_RECURSIVE_LOCK_H
#define __KOS_RECURSIVE_LOCK_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <sys/queue.h>
#include <kos/thread.h>

typedef struct recursive_lock {
    /* What thread currently holds the lock, if any? */
    kthread_t *holder;

    /* How many times does this thread hold the lock? */
    int count;
} recursive_lock_t;

/* Allocate a new recursive lock. Returns NULL on failure.
    ENOMEM - Out of memory */
recursive_lock_t *rlock_create();

/* Destroy a recursive lock */
void rlock_destroy(recursive_lock_t *l);

/* Lock a recursive lock. Returns -1 on error.
    EPERM - called inside an interrupt
    EINTR - was interrupted */
int rlock_lock(recursive_lock_t *l);

/* Lock a recursive lock, with timeout (in milliseconds). Returns -1 on error.
    EPERM - called inside an interrupt
    EINTR - was interrupted
    EAGIN - timed out*/
int rlock_lock_timed(recursive_lock_t *l, int timeout);

/* Unlock a recursive lock. Returns -1 on error.
    EPERM - the lock is not held by the current thread */
int rlock_unlock(recursive_lock_t *l);

/* Attempt to lock a recursive lock. If the call to rlock_lock() would normally
   block, return -1 for error.
    EWOULDBLOCK - would block */
int rlock_trylock(recursive_lock_t *l);

/* Return whether the lock is currently held. */
int rlock_is_locked(recursive_lock_t *l);

/* Init / shutdown */
int rlock_init();
void rlock_shutdown();

__END_DECLS

#endif /* !__KOS_RECURSIVE_LOCK_H */
