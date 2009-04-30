/* KallistiOS ##version##

   include/kos/tls.h
   Copyright (C) 2009 Lawrence Sebald

*/

/* This file defines methods for accessing thread-local storage, added in KOS
   1.3.0. */

#ifndef __KOS_TLS_H
#define __KOS_TLS_H

#include <sys/cdefs.h>
__BEGIN_DECLS

/* Thread-local storage key type. */
typedef int kthread_key_t;

/* Retrieve the next key value (i.e, what key the next kthread_key_create will
   use). */
kthread_key_t kthread_key_next();

/* Create a new TLS key. Returns non-zero on failure.
    EPERM - called inside an interrupt and another call is in progress
    ENOMEM - out of memory */
int kthread_key_create(kthread_key_t *key, void (*destructor)(void *));

/* Get the value stored for a given TLS key. Returns NULL if the key is not
   valid or not set in the current thread. */
void *kthread_getspecific(kthread_key_t key);

/* Set the value for a given TLS key. Returns non-zero on failure.
    EINVAL - the key is not valid
    ENOMEM - out of memory
    EPERM - called inside an interrupt and another call is in progress */
int kthread_setspecific(kthread_key_t key, const void *value);

/* Delete a TLS key, removing all threads' values for the given key. This does
   not call any destructors. Returns non-zero on failure.
    EINVAL - the key is not valid
    EPERM - unsafe to utilize free */
int kthread_key_delete(kthread_key_t key);

/* Delete the destructor for a given key. */
void kthread_key_delete_destructor(kthread_key_t key);

int kthread_tls_init();
void kthread_tls_shutdown();

__END_DECLS

#endif /* __KOS_TLS_H */
