/* KallistiOS ##version##

   include/kos/sem.h
   Copyright (C)2001,2003 Dan Potter

*/

/** \file   kos/sem.h
    \brief  Semaphores.

    This file defines semaphores. A semaphore is a synchronization primitive
    that allows a spcified number of threads to be in its critical section at a
    single point of time. Another way to think of it is that you have a
    predetermined number of resources available, and the semaphore maintains the
    resources.

    A special case of semaphores are mutual exclusion locks. Mutual exclusion
    locks are simply semaphores that have a count of 1 initially.

    \author Dan Potter
    \see    kos/mutex.h
*/

#ifndef __KOS_SEM_H
#define __KOS_SEM_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <arch/types.h>
#include <sys/queue.h>
#include <kos/thread.h>

/** \brief  Semaphore type.

    This structure defines a semaphore. There are no public members of this
    structure for you to actually do anything with in your code, so don't try.

    \headerfile kos/sem.h
*/
typedef struct semaphore {
    /** \cond */
    /* List entry for the global list of semaphores */
    LIST_ENTRY(semaphore)   g_list;

    /* Basic semaphore info */
    int     count;      /* The semaphore count */
    /** \endcond */
} semaphore_t;

/** \cond */
LIST_HEAD(semlist, semaphore);
/** \endcond */

/** \brief  Allocate a new semaphore.

    This function allocates and initializes a new semaphore for use.

    \param  value           The initial count of the semaphore (the number of
                            threads to allow in the critical section at a time)

    \return                 The created semaphore on success. NULL is returned
                            on failure and errno is set as appropriate.

    \par    Error Conditions:
    \em     ENOMEM - out of memory
*/
semaphore_t *sem_create(int value);

/** \brief  Free a semaphore.

    This function frees a semaphore, releasing all memory associated with it. It
    is your responsibility to make sure that all threads waiting on the
    semaphore are taken care of before destroying the semaphore.
*/
void sem_destroy(semaphore_t *sem);

/** \brief  Wait on a semaphore.

    This function will decrement the semaphore's count and return, if resources
    are available. Otherwise, the function will block until the resources become
    available.

    This function does not protect you against doing things that will cause a
    deadlock. This function is not safe to call in an interrupt. See
    sem_trywait() for a safe function to call in an interrupt.

    \param  sem             The semaphore to wait on
    \retval 0               On success
    \retval -1              On error, sets errno as appropriate

    \par    Error Conditions:
    \em     EPERM - called inside an interrupt \n
    \em     EINTR - was interrupted
*/
int sem_wait(semaphore_t *sem);

/** \brief  Wait on a semaphore (with a timeout).

    This function will decrement the semaphore's count and return, if resources
    are available. Otherwise, the function will block until the resources become
    available or the timeout expires.

    This function does not protect you against doing things that will cause a
    deadlock. This function is not safe to call in an interrupt. See
    sem_trywait() for a safe function to call in an interrupt.

    \param  sem             The semaphore to wait on
    \param  timeout         The maximum number of milliseconds to block
    \retval 0               On success
    \retval -1              On error, sets errno as appropriate

    \par    Error Conditions:
    \em     EPERM - called inside an interrupt \n
    \em     EINTR - was interrupted \n
    \em     EAGAIN - timed out while blocking
 */
int sem_wait_timed(semaphore_t *sem, int timeout);

/* Attempt to wait on a semaphore. If the semaphore would block,
   then return an error instead of actually blocking. Note that this
   function, unlike the other waits, DOES work inside an interrupt.
     EAGAIN - would block */
/** \brief  "Wait" on a semaphore without blocking.

    This function will decrement the semaphore's count and return, if resources
    are available. Otherwise, it will return an error.

    This function does not protect you against doing things that will cause a
    deadlock. This function, unlike the other waiting functions is safe to call
    inside an interrupt.

    \param  sem             The semaphore to "wait" on
    \retval 0               On success
    \retval -1              On error, sets errno as appropriate

    \par    Error Conditions:
    \em     EAGAIN - resources are not available (sem_wait() would block)
*/
int sem_trywait(semaphore_t *sem);

/** \brief  Signal a semaphore.

    This function will release resources associated with a semaphore, signalling
    a waiting thread to continue on, if any are waiting. It is your
    responsibility to make sure you only release resources you have.

    \param  sem             The semaphore to signal
*/
void sem_signal(semaphore_t *sem);

/** \brief  Retrieve the number of available resources.

    This function will retrieve the count of available resources for a
    semaphore. This is not a thread-safe way to make sure resources will be
    available when you get around to waiting, so don't use it as such.

    \param  sem             The semaphore to check
    \return                 The count of the semaphore (the number of resources
                            currently available)
*/
int sem_count(semaphore_t *sem);

/** \cond */
/* Init / shutdown */
int sem_init();
void sem_shutdown();
/** \endcond */

__END_DECLS

#endif  /* __KOS_SEM_H */

