/* KallistiOS ##version##

   include/kos/rwsem.h
   Copyright (C) 2008, 2010 Lawrence Sebald

*/

/** \file   kos/rwsem.h
    \brief  Definition for a reader/writer semaphore.

    This file defines a concept of reader/writer semaphores. Basically, this
    type of lock allows an unlimited number of "readers" to acquire the lock at
    a time, but only one "writer" (and only if no readers hold the lock).
    Readers, by definition, should not change any global data (since they are
    defined to only be reading), and since this is the case it is safe to allow
    multiple readers to access global data that is shared amongst threads.
    Writers on the other hand require exclusive access since they will be
    changing global data in the critical section, and they cannot share with
    a reader either (since the reader might attempt to read while the writer is
    changing data).

    \author Lawrence Sebald
*/

#ifndef __KOS_RWSEM_H
#define __KOS_RWSEM_H

#include <sys/cdefs.h>

__BEGIN_DECLS

#include <sys/queue.h>

/** \brief  Reader/writer semaphore structure.

    All members of this structure should be considered to be private, it is not
    safe to change anything in here yourself.

    \headerfile kos/rwsem.h
*/
typedef struct rw_semaphore {
    /** \cond */
    /* Entry into the global list of r/w semaphores. */
    LIST_ENTRY(rw_semaphore) list;
    /** \endcond */

    /** \brief  The number of readers that are currently holding the lock. */
    int read_count;

    /* \brief   The state of the write lock. */
    int write_lock;
} rw_semaphore_t;

/** \cond */
LIST_HEAD(rwsemlist, rw_semaphore);
/** \endcond */

/** \brief  Allocate a reader/writer semaphore.

    This function allocates a new reader/writer lock that is initially not
    locked either for reading or writing.

    \return The created semaphore, or NULL on failure (errno will be set to
            ENOMEM to indicate that the system is out of memory).
*/
rw_semaphore_t *rwsem_create();

/** \brief  Destroy a reader/writer semaphore.

    This function cleans up a reader/writer semaphore. It is an error to attempt
    to destroy a r/w semaphore that is locked either for reading or writing.

    \param  s       The r/w semaphore to destroy. It must be completely
                    unlocked.
*/
void rwsem_destroy(rw_semaphore_t *s);

/** \brief  Lock a reader/writer semaphore for reading.

    This function attempts to lock the r/w semaphore for reading. If the
    semaphore is locked for writing, this function will block until it is
    possible to obtain the lock for reading. This function is <b>NOT</b> safe to
    call inside of an interrupt.

    \param  s       The r/w semaphore to lock.
    \retval -1      On error, errno will be set to EPERM if this function is
                    called inside an interrupt or EINTR if it is interrupted.
    \retval 0       On success.
    \sa     rwsem_write_lock
    \sa     rwsem_read_trylock
*/
int rwsem_read_lock(rw_semaphore_t *s);

/** \brief  Lock a reader/writer semaphore for writing.

    This function attempts to lock the r/w semaphore for writing. If the
    semaphore is locked for reading or writing, this function will block until
    it is possible to obtain the lock for writing. This function is <b>NOT</b>
    safe to call inside of an interrupt.

    \param  s       The r/w semaphore to lock.
    \retval -1      On error, errno will be set to EPERM if this function is
                    called inside an interrupt or EINTR if it is interrupted.
    \retval 0       On success.
    \sa     rwsem_read_lock
    \sa     rwsem_write_trylock
*/
int rwsem_write_lock(rw_semaphore_t *s);

/** \brief  Unlock a reader/writer semaphore from a read lock.

    This function releases one instance of the read lock on the r/w semaphore.

    \param  s       The r/w semaphore to release the read lock on.
    \retval 0       On success (no error conditions defined).
    \sa     rwsem_write_unlock
*/
int rwsem_read_unlock(rw_semaphore_t *s);

/** \brief  Unlock a reader/writer semaphore from a write lock.

    This function releases one instance of the write lock on the r/w semaphore.

    \param  s       The r/w semaphore to release the write lock on.
    \retval 0       On success (no error conditions defined).
    \sa     rwsem_read_unlock
*/
int rwsem_write_unlock(rw_semaphore_t *s);

/** \brief  Attempt to lock a reader/writer semaphore for reading.

    This function attempts to lock the r/w semaphore for reading. If for any
    reason rwsem_read_lock would normally block, this function will return an
    error. This function is safe to call inside an interrupt.

    \param  s       The r/w semaphore to attempt to lock.
    \retval -1      On error, errno will be set to EWOULDBLOCK if a call to
                    rwsem_read_lock would normally block.
    \retval 0       On success.
    \sa     rwsem_write_trylock
    \sa     rwsem_read_lock
*/
int rwsem_read_trylock(rw_semaphore_t *s);

/** \brief  Attempt to lock a reader/writer semaphore for writing.

    This function attempts to lock the r/w semaphore for writing. If for any
    reason rwsem_write_lock would normally block, this function will return an
    error. This function is safe to call inside an interrupt.

    \param  s       The r/w semaphore to attempt to lock.
    \retval -1      On error, errno will be set to EWOULDBLOCK if a call to
                    rwsem_write_lock would normally block.
    \retval 0       On success.
    \sa     rwsem_read_trylock
    \sa     rwsem_write_lock
*/
int rwsem_write_trylock(rw_semaphore_t *s);

/** \brief  Upgrade a thread from reader status to writer status.

    This function will upgrade the lock on the calling thread from a reader
    state to a writer state. If it cannot do this at the moment, it will block
    until it is possible. This function is <b>NOT</b> safe to call inside an
    interrupt.

    \param  s       The r/w semaphore to upgrade.
    \retval -1      On error, errno will be set to EPERM if called inside an
                    interrupt or EINTR if interrupted.
    \retval 0       On success.
    \sa     rwsem_read_tryupgrade
*/
int rwsem_read_upgrade(rw_semaphore_t *s);

/** \brief  Attempt to upgrade a thread from reader status to writer status.

    This function will attempt to upgrade the lock on the calling thread to
    writer status. If for any reason rwsem_read_upgrade would block, this
    function will return an error. This function is safe to call inside an
    interrupt. Note that on error, the read lock is still held!

    \param  s       The r/w semaphore to upgrade.
    \retval -1      On error, errno will be set to EWOULDBLOCK if a call to
                    rwsem_read_upgrade would normally block.
    \retval 0       On success.
    \sa     rwsem_read_upgrade
*/
int rwsem_read_tryupgrade(rw_semaphore_t *s);

/** \brief  Read the reader count on the reader/writer semaphore.

    This function is not a safe way to see if the lock will be locked by any
    readers when you get around to locking it, so do not use it in this way.

    \param  s       The r/w semaphore to count the readers on.
    \return The number of readers holding the r/w semaphore.
*/
int rwsem_read_count(rw_semaphore_t *s);

/** \brief  Read the state of the writer lock on the reader/writer semaphore.

    This function is not a safe way to see if the lock will be locked by a
    writer by the time you get around to doing something with it, so don't try
    to use it for that purpose.

    \param  s       The r/w semaphore to check the writer status on.
    \return The status of the writer lock of the r/w semaphore.
*/
int rwsem_write_locked(rw_semaphore_t *s);

/* Init / shutdown */
/** \cond */
int rwsem_init();
void rwsem_shutdown();
/** \endcond */

__END_DECLS

#endif /* __KOS_RWSEM_H */
