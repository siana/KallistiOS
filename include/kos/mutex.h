/* KallistiOS ##version##

   include/kos/mutex.h
   Copyright (C)2001,2003 Dan Potter

*/

/** \file   kos/mutex.h
    \brief  Mutual exclusion locks.

    This file defines mutual exclusion locks (or mutexes for short). The concept
    of a mutex is one of the most common types of locks in a multi-threaded
    environment. Mutexes do exactly what they sound like, they keep two (or
    more) threads mutually exclusive from one another. A mutex is used around
    a block of code to prevent two threads from interfereing with one another
    when only one would be appropriate to be in the block at a time.

    KallistiOS simply implements its mutexes as a wrapper around semaphores with
    an initial count of 1.

    \author Dan Potter
    \see    kos/sem.h
*/

#ifndef __KOS_MUTEX_H
#define __KOS_MUTEX_H

#include <sys/cdefs.h>
__BEGIN_DECLS

/* These are just wrappers around semaphores */
#include <kos/sem.h>

/** \brief  Mutual exclusion lock type.

    KOS mutexes are just wrappers around semaphores.

    \headerfile kos/mutex.h
*/
typedef semaphore_t mutex_t;

/** \brief  Allocate a new mutex.

    This function allocates and initializes a new mutex for use.

    \return                 The created mutex on success. NULL is returned on
                            failure and errno is set as appropriate.
 
    \par    Error Conditions:
    \em     ENOMEM - out of memory
*/
mutex_t * mutex_create();

/** \brief  Free a mutex.

    This function frees a mutex, releasing all memory associated with it. It is
    your responsibility to make sure that all threads waiting on the mutex are
    taken care of before destroying the mutex.
*/
void mutex_destroy(mutex_t * m);

/** \brief  Lock a mutex.

    This function will lock a mutex, if it is not already locked by another
    thread. If it is locked by another thread already, this function will block
    until the mutex has been acquired for the calling thread.

    This does not protect against a thread obtaining the same mutex twice or any
    other deadlock conditions. Also, this function is not safe to call inside an
    interrupt. For mutex locks inside interrupts, use mutex_trylock().

    \param  m               The mutex to acquire
    \retval 0               On success
    \retval -1              On error, sets errno as appropriate

    \par    Error Conditions:
    \em     EPERM - called inside an interrupt \n
    \em     EINTR - was interrupted
*/
int mutex_lock(mutex_t * m);

/** \brief  Lock a mutex (with a timeout).

    This function will attempt to lock a mutex. If the lock can be acquired
    immediately, the function will return immediately. If not, the function will
    block for up to the specified number of milliseconds to wait for the lock.
    If the lock cannot be acquired in this timeframe, this function will return
    an error.

    Much like mutex_lock(), this function does not protect against deadlocks
    and is not safe to call in an interrupt.

    \param  m               The mutex to acquire
    \param  timeout         The number of milliseconds to wait for the lock
    \retval 0               On success
    \retval -1              On error, errno will be set as appropriate

    \par    Error Conditions:
    \em     EPERM - called inside an interrupt \n
    \em     EINTR - was interrupted \n
    \em     EAGAIN - timed out while blocking
*/
int mutex_lock_timed(mutex_t * m, int timeout);

/** \brief  Check if a mutex is locked.

    This function will check whether or not a mutex is currently locked. This is
    not a thread-safe way to determine if the mutex will be locked by the time
    you get around to doing it. If you wish to attempt to lock a mutex without
    blocking, look at mutex_trylock(), not this.

    \param  m               The mutex to check
    \retval 0               If the mutex is not currently locked
    \retval 1               If the mutex is currently locked
*/
int mutex_is_locked(mutex_t * m);

/** \brief  Attempt to lock a mutex.

    This function will attempt to acquire the mutex for the calling thread,
    returning immediately whether or not it could be acquired. If the mutex
    cannot be acquired, an error will be returned.

    This function is safe to call inside an interrupt.

    \param  m               The mutex to attempt to acquire
    \retval 0               On successfully acquiring the mutex
    \retval -1              If the mutex cannot be acquired without blocking

    \par    Error Conditions:
    \em     EAGAIN - the mutex is already locked (mutex_lock() would block)
*/
int mutex_trylock(mutex_t * m);

/** \brief  Unlock a mutex.

    This function will unlock a mutex, allowing other threads to acquire it.
    This function does not check if the thread that is calling it holds the
    mutex. It is your responsibility to make sure you only unlock mutexes that
    you have locked.

    \param  m               The mutex to unlock
*/
void mutex_unlock(mutex_t * m);

__END_DECLS

#endif	/* __KOS_MUTEX_H */

