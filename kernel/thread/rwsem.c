/* KallistiOS ##version##

   rwsem.c
   Copyright (C) 2008 Lawrence Sebald
*/

/* Defines reader/writer semaphores */

#include <malloc.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <sys/queue.h>

#include <kos/rwsem.h>
#include <kos/genwait.h>
#include <arch/spinlock.h>

/* Reader/writer semaphore list spinlock */
static spinlock_t mutex;

/* Global list of reader/writer semaphores */
static struct rwsemlist rwsem_list;

/* Allocate a new reader/writer semaphore */
rw_semaphore_t *rwsem_create() {
    rw_semaphore_t *s;

    s = (rw_semaphore_t *)malloc(sizeof(rw_semaphore_t));

    if(!s) {
        errno = ENOMEM;
        return NULL;
    }

    s->read_count = 0;
    s->write_lock = 0;

    spinlock_lock(&mutex);
    LIST_INSERT_HEAD(&rwsem_list, s, list);
    spinlock_unlock(&mutex);

    return s;
}

/* Destroy a reader/writer semaphore */
void rwsem_destroy(rw_semaphore_t *s) {
    /* XXXX: Should really cause anyone waiting to get an error back on their
       wait... hmm. */
    spinlock_lock(&mutex);
    LIST_REMOVE(s, list);
    spinlock_unlock(&mutex);

    free(s);
}

/* Lock a reader/writer semaphore for reading */
int rwsem_read_lock(rw_semaphore_t *s) {
    int old, rv = 0;

    if(irq_inside_int()) {
        dbglog(DBG_WARNING, "rwsem_read_lock: called inside interrupt\n");
        errno = EPERM;
        return -1;
    }

    old = irq_disable();

    /* If the write lock is not held, let the thread proceed */
    if(!s->write_lock) {
        ++s->read_count;
    }
    else {
        /* Block until the write lock is not held any more */
        rv = genwait_wait(s, "rwsem_read_lock", 0, NULL);

        if(rv < 0) {
            assert(errno == EINTR);
            rv = -1;
        }
        else {
            ++s->read_count;
        }
    }

    irq_restore(old);
    return rv;
}

/* Lock a reader/writer semaphore for writing */
int rwsem_write_lock(rw_semaphore_t *s) {
    int old, rv = 0;

    if(irq_inside_int()) {
        dbglog(DBG_WARNING, "rwsem_write_lock: called inside interrupt\n");
        errno = EPERM;
        return -1;
    }

    old = irq_disable();

    /* If the write lock is not held and there are no readers in their critical
       sections, let the thread proceed. */
    if(!s->write_lock && !s->read_count) {
        s->write_lock = 1;
    }
    else {
        /* Block until the write lock is not held and there are no readers
           inside their critical sections */
        rv = genwait_wait(&s->write_lock, "rwsem_write_lock", 0, NULL);

        if(rv < 0) {
            assert(errno == EINTR);
            rv = -1;
        }
        else {
            s->write_lock = 1;
        }
    }

    irq_restore(old);
    return rv;
}

/* Unlock a reader/writer semaphore from a read lock. */
int rwsem_read_unlock(rw_semaphore_t *s) {
    int old;

    old = irq_disable();

    --s->read_count;

    /* If this was the last reader, attempt to wake any writers waiting. */
    if(!s->read_count) {
        genwait_wake_one(&s->write_lock);
    }

    irq_restore(old);

    return 0;
}

/* Unlock a reader/writer semaphore from a write lock. */
int rwsem_write_unlock(rw_semaphore_t *s) {
    int old, woken;

    old = irq_disable();

    s->write_lock = 0;

    /* Give writers priority, attempt to wake any writers first. */
    woken = genwait_wake_cnt(&s->write_lock, 1, 0);

    if(!woken) {
        /* No writers were waiting, wake up any readers. */
        genwait_wake_all(s);
    }

    irq_restore(old);

    return 0;
}

/* Attempt to lock a reader/writer semaphore for reading, but do not block. */
int rwsem_read_trylock(rw_semaphore_t *s) {
    int old, rv;

    old = irq_disable();

    /* Is the write lock held? */
    if(s->write_lock) {
        rv = -1;
        errno = EWOULDBLOCK;
    }
    else {
        rv = 0;
        ++s->read_count;
    }

    irq_restore(old);
    return rv;
}

/* Attempt to lock a reader/writer semaphore for writing, but do not block. */
int rwsem_write_trylock(rw_semaphore_t *s) {
    int old, rv;

    old = irq_disable();

    /* Are there any readers in their critical sections, or is the write lock
       already held, if so we can't do anything about that now. */
    if(s->read_count || s->write_lock) {
        rv = -1;
        errno = EWOULDBLOCK;
    }
    else {
        rv = 0;
        s->write_lock = 1;
    }

    irq_restore(old);
    return rv;
}

/* "Upgrade" a read lock to a write lock. */
int rwsem_read_upgrade(rw_semaphore_t *s) {
    int old, rv = 0;

    if(irq_inside_int()) {
        dbglog(DBG_WARNING, "rwsem_read_upgrade: called inside interrupt\n");
        errno = EPERM;
        return -1;
    }

    old = irq_disable();

    --s->read_count;

    /* If there are still other readers, wait patiently for our turn. */
    if(s->read_count) {
        rv = genwait_wait(&s->write_lock, "rwsem_read_upgrade", 0, NULL);

        if(rv < 0) {
            assert(errno == EINTR);
            rv = -1;
        }
        else {
            s->write_lock = 1;
        }
    }
    else {
        s->write_lock = 1;
    }

    irq_restore(old);
    return rv;
}

/* Attempt to upgrade a read lock to a write lock, but do not block. */
int rwsem_read_tryupgrade(rw_semaphore_t *s) {
    int old, rv;

    old = irq_disable();

    if(s->read_count != 1) {
        rv = -1;
        errno = EWOULDBLOCK;
    }
    else {
        rv = 0;
        s->read_count = 0;
        s->write_lock = 1;
    }

    irq_restore(old);
    return rv;
}

/* Return the current reader count */
int rwsem_read_count(rw_semaphore_t *s) {
    return s->read_count;
}

/* Return the current status of the write lock */
int rwsem_write_locked(rw_semaphore_t *s) {
    return s->write_lock;
}

/* Initialize reader/writer semaphores */
int rwsem_init() {
    LIST_INIT(&rwsem_list);
    spinlock_init(&mutex);
    return 0;
}

/* Shut down reader/writer semaphores */
void rwsem_shutdown() {
    /* XXXX: Do something useful */
}
