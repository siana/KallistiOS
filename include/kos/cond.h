/* KallistiOS ##version##

   include/kos/cond.h
   Copyright (C)2001,2003 Dan Potter

*/

#ifndef __KOS_COND_H
#define __KOS_COND_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <arch/types.h>
#include <sys/queue.h>
#include <kos/thread.h>
#include <kos/mutex.h>
#include <kos/recursive_lock.h>

/* Condition structure */
typedef struct condvar {
	/* List entry for the global list of condvars */
	LIST_ENTRY(condvar)	g_list;
} condvar_t;

LIST_HEAD(condlist, condvar);

/* Allocate a new condvar. Sets errno to ENOMEM on failure. */
condvar_t *cond_create();

/* Free a condvar */
void cond_destroy(condvar_t *cv);

/* Wait on a condvar; if there is an associated mutex to unlock
   while waiting, then pass that as well. Returns -1 on error.
     EPERM - called inside interrupt
     EINTR - wait was interrupted */
int cond_wait(condvar_t *cv, mutex_t * m);

/* Same as above, but uses a recursive lock instead of a mutex.
   Note that using this is a really bad idea, since if the lock hasn't
   been completely unlocked by the single unlock that happens in this
   function, you will likely end up with a deadlock. This was only
   added for support for GCC's C++0x threading support. */
int cond_wait_recursive(condvar_t *cv, recursive_lock_t *l);

/* Wait on a condvar; if there is an associated mutex to unlock
   while waiting, then pass that as well. If more than 'timeout'
   milliseconds passes and we still haven't been signaled, return
   an error code (-1). Return success (0) when we are woken normally.
   Note: if 'timeout' is zero, this call is equivalent to 
   cond_wait above.
     EPERM   - called inside interrupt
     EAGAIN  - timed out
     EINTR   - was interrupted */
int cond_wait_timed(condvar_t *cv, mutex_t * m, int timeout);

/* Save as above, but uses a recursive lock instead of a mutex.
   The note about deadlocks with cond_wait_recursive above still
   applies here. */
int cond_wait_timed_recursive(condvar_t *cv, recursive_lock_t *l, int timeout);

/* Signal a single thread waiting on the condvar; you should be
   holding any associated mutex before doing this! */
void cond_signal(condvar_t *cv);

/* Signal all threads waiting on the condvar; you should be holding
   any associated mutex before doing this! */
void cond_broadcast(condvar_t *cv);

/* Init / shutdown */
int cond_init();
void cond_shutdown();

__END_DECLS

#endif	/* __KOS_COND_H */

