/* KallistiOS ##version##

   include/kos/tls.h
   Copyright (C) 2009, 2010 Lawrence Sebald

*/

/** \file   kos/tls.h
    \brief  Thread-local storage support.

    This file contains the definitions used to support key/value pairs of
    thread-local storage in KOS.

    \author Lawrence Sebald
*/

#ifndef __KOS_TLS_H
#define __KOS_TLS_H

#include <sys/cdefs.h>

__BEGIN_DECLS

/** \brief  Thread-local storage key type. */
typedef int kthread_key_t;

/** \cond */
/* Retrieve the next key value (i.e, what key the next kthread_key_create will
   use). This function is not meant for external use (although it won't really
   hurt anything for you to call it). */
kthread_key_t kthread_key_next();
/** \endcond */

/** \brief  Create a new thread-local storage key.

    This function creates a new thread-local storage key that shall be visible
    to all threads. Each thread is then responsible for associating any data
    with the key that it deems fit (by default a thread will have no data
    associated with a newly created key).

    \param  key         The key to use.
    \param  destructor  A destructor for use with this key. If it is non-NULL,
                        and a value associated with the key is non-NULL at
                        thread exit, then the destructor will be called with the
                        value as its argument.
    \retval -1      On failure, and sets errno to one of the following: EPERM if
                    called inside an interrupt and another call is in progress,
                    ENOMEM if out of memory.
    \retval 0       On success.
*/
int kthread_key_create(kthread_key_t *key, void (*destructor)(void *));

/** \brief  Retrieve a value associated with a TLS key.

    This function retrieves the thread-specific data associated with the given
    key.

    \param  key     The key to look up data for.
    \return The data associated with the key, or NULL if the key is not valid or
            no data has been set in the current thread.
*/
void *kthread_getspecific(kthread_key_t key);

/** \brief  Set thread specific data for a key.

    This function sets the thread-specific data associated with the given key.

    \param  key     The key to set data for.
    \param  value   The thread-specific value to use.
    \retval -1      On failure, and sets errno to one of the following: EINVAL
                    if the key is not valid, ENOMEM if out of memory, or EPERM
                    if called inside an interrupt and another call is in
                    progress.
    \retval 0       On success.
*/
int kthread_setspecific(kthread_key_t key, const void *value);

/** \brief  Delete a TLS key.

    This function deletes a TLS key, removing all threads' values for the given
    key. This function <em>does not</em> cause any destructors to be called.

    \param  key     The key to delete.
    \retval -1      On failure, and sets errno to one of the following: EINVAL
                    if the key is invalid, EPERM if unsafe to call free.
    \retval 0       On success.
*/
int kthread_key_delete(kthread_key_t key);

/** \cond */
/* Delete the destructor for a given key. This function is for internal use
   only! */
void kthread_key_delete_destructor(kthread_key_t key);

/* Initialization and shutdown. Once again, internal use only. */
int kthread_tls_init();
void kthread_tls_shutdown();
/** \endcond */

__END_DECLS

#endif /* __KOS_TLS_H */
