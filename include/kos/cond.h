/* KallistiOS ##version##

   include/kos/cond.h
   Copyright (C)2001,2003 Dan Potter

*/

/** \file   kos/cond.h
    \brief  Condition variables.

    This file contains the definition of a Condition Variable. Condition
    Variables (or condvars for short) are used with a mutex to act as a lock and
    checkpoint pair for threads.

    Basically, things work as follows (for the thread doing work):
    \li The associated mutex is locked.
    \li A predicate is checked to see if its safe to do something.
    \li If it is not safe, you call cond_wait(), which releases the mutex.
    \li When cond_wait() returns, the mutex is reaquired, and work can go on.
    \li Update any predicates so that we konw that the work is done, and unlock
        the mutex.

    Meanwhile, the thread updating the condition works as follows:
    \li Lock the mutex associated with the condvar.
    \li Produce work to be done.
    \li Call cond_signal() (with the associated mutex still locked), so that any
        threads waiting on the condvar will know they can continue on when the
        mutex is released, also update any predicates that say whether work can
        be done.
    \li Unlock the mutex so that worker threads can acquire the mutex and do
        whatever work needs to be done.

    Condition variables can be quite useful when used properly, and provide a
    fairly easy way to wait for work to be ready to be done.

    \author Dan Potter
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

/** \brief  Condition variable.

    There are no public members of this structure for you to actually do
    anything with in your code, so don't try.

    \headerfile kos/cond.h
*/
typedef struct condvar {
    /** \cond */
    /* List entry for the global list of condvars */
    LIST_ENTRY(condvar) g_list;
    /** \endcond */
} condvar_t;

/* \cond */
LIST_HEAD(condlist, condvar);
/* \endcond */

/** \brief  Initializer for a transient condvar. */
#define COND_INITIALIZER    { { 0 } }

/** \brief  Allocate a new condition variable.

    This function allocates and initializes a new condition variable for use.

    \return                 The created condvar on success. NULL is returned on
                            failure and errno is set as appropriate.

    \par    Error Conditions:
    \em     ENOMEM - out of memory
*/
condvar_t *cond_create();

/** \brief  Free a condition variable.

    This function frees a condition variable, releasing all memory associated
    with it (but not with the mutex that is associated with it). This will also
    wake all threads waiting on the condition.
*/
void cond_destroy(condvar_t *cv);

/** \brief  Wait on a condition variable.

    This function will wait on the condition variable, unlocking the mutex and
    putting the calling thread to sleep as one atomic operation. The wait in
    this function has no timeout, and will sleep forever if the condition is not
    signalled.

    \param  cv              The condition to wait on
    \param  m               The associated mutex
    \retval 0               On success
    \retval -1              On error, sets errno as appropriate

    \par    Error Conditions:
    \em     EPERM - Called inside an interrupt \n
    \em     EINTR - Was interrupted
*/
int cond_wait(condvar_t *cv, mutex_t * m);

/** \brief  Wait on a condition variable, using a recursive_lock_t.

    This function acts exactly like the cond_wait() function, except instead of
    a mutex_t as its lock, it uses a recursive_lock_t. Generally, this should
    not be preferred, as deadlock may occur if the calling thread has locked
    the lock more than once (since this function only unlocks it once). This was
    added specifically because it was required for GCC's C++0x threading code.

    \param  cv              The condition to wait on
    \param  l               The associated lock
    \retval 0               On success
    \retval -1              On error, sets errno as appropriate

    \par    Error Conditions:
    \em     EPERM - Called inside an interrupt \n
    \em     EINTR - Was interrupted

    \author Lawrence Sebald
*/
int cond_wait_recursive(condvar_t *cv, recursive_lock_t *l);

/** \brief  Wait on a condition variable with a timeout.

    This function will wait on the condition variable, unlocking the mutex and
    putting the calling thread to sleep as one atomic operation. If the timeout
    elapses before the condition is signalled, this function will return error.
    If a timeout of 0 is given, the call is equivalent to cond_wait() (there is
    no timeout).

    \param  cv              The condition to wait on
    \param  m               The associated mutex
    \param  timeout         The number of milliseconds before timeout
    \retval 0               On success
    \retval -1              On error, sets errno as appropriate

    \par    Error Conditions:
    \em     EPERM - Called inside an interrupt \n
    \em     EINTR - Was interrupted \n
    \em     EAGAIN - timed out
*/
int cond_wait_timed(condvar_t *cv, mutex_t * m, int timeout);

/** \brief  Wait on a condition variable with a timeout, using a recursive lock.

    This function will wait on the condition variable, unlocking the lock and
    putting the calling thread to sleep as one atomic operation. If the timeout
    elapses before the condition is signalled, this function will return error.
    If a timeout of 0 is given, the call is equivalent to cond_wait() (there is
    no timeout). The rant about cond_wait_recursive() being evil applies here
    as well.

    \param  cv              The condition to wait on
    \param  l               The associated recursive lock.
    \param  timeout         The number of milliseconds before timeout
    \retval 0               On success
    \retval -1              On error, sets errno as appropriate

    \par    Error Conditions:
    \em     EPERM - Called inside an interrupt \n
    \em     EINTR - Was interrupted \n
    \em     EAGAIN - timed out

    \author Lawrence Sebald
*/
int cond_wait_timed_recursive(condvar_t *cv, recursive_lock_t *l, int timeout);

/** \brief  Signal a single thread waiting on the condition variable.

    This function will wake up a single thread that is waiting on the condition.
    The calling thread should be holding the associated mutex or recursive lock
    before calling this to guarantee sane behavior.

    \param  cv              The condition to signal
*/
void cond_signal(condvar_t *cv);

/** \brief  Signal all threads waiting on the condition variable.

    This function will wake up all threads that are waiting on the condition.
    The calling thread should be holding the associated mutex or recursive lock
    before calling this to guarantee sane behavior.

    \param  cv              The condition to signal
*/
void cond_broadcast(condvar_t *cv);

/* Init / shutdown */
/** \cond */
int cond_init();
void cond_shutdown();
/** \endcond */

__END_DECLS

#endif  /* __KOS_COND_H */

