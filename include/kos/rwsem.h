/* KallistiOS ##version##

   include/kos/rwsem.h
   Copyright (C) 2008 Lawrence Sebald

*/

/* This file defines a concept that might be familiar to anyone who's ever
   hacked around on the Linux kernel a bit: reader/writer semaphores. Basically,
   reader/writer semaphores allow an unlimitted readers to occupy the critical
   section at any given time. Since readers, by definition, do not change any
   data other than their own local variables this should in theory be safe.
   Writers on the other hand require exclusive access to the critical section.
   Writers only may proceed into the critical section if the write lock is not
   held on the semaphore and if there are no readers currently in the critical
   section. Also, no reader will be allowed into the critical section if the
   write lock is held. */

#ifndef __KOS_RWSEM_H
#define __KOS_RWSEM_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <sys/queue.h>

/* Reader/writer semaphore structure */
typedef struct rw_semaphore {
    /* List entry for the global list of reader/writer semaphores */
    LIST_ENTRY(rw_semaphore) list;

    /* How many readers currently hold the semaphore lock? */
    int read_count;

    /* Does a writer currently hold the lock? */
    int write_lock;
} rw_semaphore_t;

LIST_HEAD(rwsemlist, rw_semaphore);

/* Allocate a new reader/writer semaphore. Returns NULL on failure.
    ENOMEM - Out of memory */
rw_semaphore_t *rwsem_create();

/* Destroy a reader/writer semaphore */
void rwsem_destroy(rw_semaphore_t *s);

/* Lock a reader/writer semaphore for reading. Returns -1 on error.
    EPERM - called inside an interrupt
    EINTR - was interrupted */
int rwsem_read_lock(rw_semaphore_t *s);

/* Lock a reader/writer semaphore for writing. Returns -1 on error.
    EPERM - called inside an interrupt
    EINTR - was interrupted */
int rwsem_write_lock(rw_semaphore_t *s);

/* Unlock a reader/writer semaphore from a read lock. Returns -1 on error. */
int rwsem_read_unlock(rw_semaphore_t *s);

/* Unlock a reader/writer semaphore from a write lock. Returns -1 on error. */
int rwsem_write_unlock(rw_semaphore_t *s);

/* Attempt to lock a reader/writer semaphore for reading. If the call to
   rwsem_read_lock() would normally block, return -1 for error.
    EWOULDBLOCK - would block */
int rwsem_read_trylock(rw_semaphore_t *s);

/* Attempt to lock a reader/writer semaphore for writing. If the call to
   rwsem_write_lock() would normally block, return -1 for error.
    EWOULDBLOCK - would block */
int rwsem_write_trylock(rw_semaphore_t *s);

/* Return the reader/writer semaphore reader count */
int rwsem_read_count(rw_semaphore_t *s);

/* Return the reader/writer semaphore write lock status */
int rwsem_write_locked(rw_semaphore_t *s);

/* Init / shutdown */
int rwsem_init();
void rwsem_shutdown();

__END_DECLS

#endif /* __KOS_RWSEM_H */
