/* KallistiOS ##version##

   cond.c
   Copyright (C) 2001, 2003 Dan Potter
   Copyright (C) 2012 Lawrence Sebald
*/

/* Defines condition variables, which are like semaphores that automatically
   signal all waiting processes when a signal() is called. */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>

#include <kos/thread.h>
#include <kos/limits.h>
#include <kos/cond.h>
#include <kos/genwait.h>

#include <kos/dbglog.h>

/**************************************/

/* Allocate a new condvar */
condvar_t *cond_create() {
    condvar_t *cv;

    dbglog(DBG_WARNING, "Creating condvar with deprecated cond_create(). "
           "Please update your code!\n");

    /* Create a condvar structure */
    if(!(cv = (condvar_t *)malloc(sizeof(condvar_t)))) {
        errno = ENOMEM;
        return NULL;
    }

    cv->initted = 1;
    cv->dynamic = 1;

    return cv;
}

int cond_init(condvar_t *cv) {
    cv->initted = 1;
    return 0;
}

/* Free a condvar */
void cond_destroy(condvar_t *cv) {
    /* Give all sleeping threads a timed out error */
    genwait_wake_all_err(cv, ETIMEDOUT);

    cv->initted = 0;

    /* Free the memory */
    if(cv->dynamic)
        free(cv);
}

int cond_wait_timed(condvar_t *cv, mutex_t *m, int timeout) {
    int old, rv;

    if(irq_inside_int()) {
        dbglog(DBG_WARNING, "cond_wait: called inside interrupt\n");
        errno = EPERM;
        return -1;
    }

    old = irq_disable();

    /* First of all, release the associated mutex */
    assert(mutex_is_locked(m));
    mutex_unlock(m);

    /* Now block us until we're signaled */
    rv = genwait_wait(cv, timeout ? "cond_wait_timed" : "cond_wait", timeout,
                      NULL);

    /* Re-lock our mutex */
    mutex_lock(m);

    /* Ok, ready to return */
    irq_restore(old);

    return rv;
}

int cond_wait(condvar_t *cv, mutex_t *m) {
    return cond_wait_timed(cv, m, 0);
}

void cond_signal(condvar_t *cv) {
    int old = 0;

    old = irq_disable();

    /* Wake any one thread who's waiting */
    genwait_wake_one(cv);

    irq_restore(old);
}

void cond_broadcast(condvar_t *cv) {
    int old = 0;

    old = irq_disable();

    /* Wake all threads who are waiting */
    genwait_wake_all(cv);

    irq_restore(old);
}
