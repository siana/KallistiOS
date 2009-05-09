/* KallistiOS ##version##
 
   recursive_lock.c
   Copyright (C) 2008, 2009 Lawrence Sebald
*/

/* This file defines recursive locks. */

#include <malloc.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <sys/queue.h>

#include <kos/recursive_lock.h>
#include <kos/thread.h>
#include <kos/genwait.h>

/* Create a recursive lock */
recursive_lock_t *rlock_create() {
    recursive_lock_t *rv;

    rv = (recursive_lock_t *)malloc(sizeof(recursive_lock_t));
    if(!rv) {
        errno = ENOMEM;
        return NULL;
    }

    rv->holder = NULL;
    rv->count = 0;

    return rv;
}

/* Destroy a recursive lock */
void rlock_destroy(recursive_lock_t *l) {
    assert(l);
    assert(!l->count);

    free(l);
}

/* Lock a recursive lock */
int rlock_lock(recursive_lock_t *l) {
    return rlock_lock_timed(l, 0);
}

/* Lock a recursive lock, with timeout (in milliseconds) */
int rlock_lock_timed(recursive_lock_t *l, int timeout) {
    int old, rv = 0;

    if(irq_inside_int()) {
        dbglog(DBG_WARNING, "rlock_lock_timed: called inside interrupt\n");
        errno = EPERM;
        return -1;
    }

    old = irq_disable();

    /* If the lock is not held, let the thread proceed */
    if(!l->count) {
        assert(!l->holder);

        l->count = 1;
        l->holder = thd_current;
    }
    else if(l->holder == thd_current) {
        ++l->count;
    }
    else {
        /* Block until the lock isn't held any more */
        rv = genwait_wait(l, "rlock_lock_timed", timeout, NULL);

        if(rv < 0) {
            assert(errno == EINTR);
            rv = -1;
        }
        else {
            l->count = 1;
            l->holder = thd_current;
        }
    }

    irq_restore(old);
    return rv;
}

/* Unlock a recursive lock */
int rlock_unlock(recursive_lock_t *l) {
    int old, rv = 0;

    old = irq_disable();

    /* Make sure we currently hold the lock */
    if(l->holder != thd_current) {
        rv = -1;
        errno = EPERM;
    }
    else {
        --l->count;

        /* If we're done holding the lock, mark it as such, and signal the next
           thread waiting, if any. */
        if(!l->count) {
            l->holder = NULL;
            genwait_wake_one(l);
        }
    }

    irq_restore(old);
    return rv;
}

/* Attempt to lock a recursive lock */
int rlock_trylock(recursive_lock_t *l) {
    int old, rv = 0;

    old = irq_disable();

    /* Check if the lock is held, if so, check if the current thread holds the
       lock. */
    if(l->count) {
        if(l->holder == thd_current) {
            ++l->count;
        }
        else {
            rv = -1;
            errno = EWOULDBLOCK;
        }
    }
    else {
        l->holder = thd_current;
        l->count = 1;
    }

    irq_restore(old);
    return rv;
}

/* Return whether or not the lock is currently held */
int rlock_is_locked(recursive_lock_t *l) {
    return !!l->count;
}

/* Initialize recursive locks */
int rlock_init() {
    return 0;
}

/* Shut down recursive locks */
void rlock_shutdown() {
}
