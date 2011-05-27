/* KallistiOS ##version##

   include/kos/genwait.h
   Copyright (c)2003 Dan Potter

*/

/** \file   kos/genwait.h
    \brief  Generic wait system.

    The generic wait system in KOS, like many other portions of KOS, is based on
    an idea from the BSD kernel. It allows you to sleep on any random object and
    later wake up any threads that happen to be sleeping on thta object. All of
    KOS' sync primatives (other than spinlocks) are based on this concept, and
    it can be used for some fairly useful things.

    \author Dan Potter
*/

#ifndef __KOS_GENWAIT_H
#define __KOS_GENWAIT_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <sys/queue.h>
#include <kos/thread.h>

/** \brief  Sleep on an object.

    This function sleeps on the specified object. You are not allowed to call
    this function inside an interrupt.

    \param  obj             The object to sleep on
    \param  mesg            A message to show in the status
    \param  timeout         If not woken before this many milliseconds have
                            passed, wake up anyway
    \param  callback        If non-NULL, call this function with obj as its
                            argument if the wait times out (but before the
                            calling thread has been woken back up)
    \retval 0               On successfully being woken up (not by timeout)
    \retval -1              On error or being woken by timeout

    \par    Error Conditions:
    \em     EAGAIN - on timeout
*/
int genwait_wait(void * obj, const char * mesg, int timeout, void (*callback)(void *));

/* Wake up N threads waiting on the given object. If cnt is <=0, then we
   wake all threads. Returns the number of threads actually woken. */
/** \brief  Wake up a number of threads sleeping on an object.

    This function wakes up the specified number of threads sleeping on the
    object specified.

    \param  obj             The object to wake threads that are sleeping on it
    \param  cnt             The number of threads to wake, if <= 0, wake all
    \return                 The number of threads woken
*/
int genwait_wake_cnt(void * obj, int cnt);

/** \brief  Wake up all threads sleeping on an object.

    This function simply calls genwait_wake_cnt(obj, -1).

    \param  obj             The thread to wake threads that are sleeping on it
    \see    genwait_wake_cnt()
*/
void genwait_wake_all(void * obj);

/** \brief  Wake up one thread sleeping on an object.

    This function simply calls genwait_wake_cnt(obj, 1).

    \param  obj             The thread to wake threads that are sleeping on it
    \see    genwait_wake_cnt()
*/
void genwait_wake_one(void * obj);

/** \brief  Look for timed out genwait_wait() calls.

    There should be no reason you need to call this function, it is called
    internally by the scheduler for you.

    \param  now             The current system time, in milliseconds since boot
*/
void genwait_check_timeouts(uint64 now);

/** \brief  Look for the next timeout event time.

    This function looks up when the next genwait_wait() call will timeout. This
    function is for the internal use of the scheduler, and should not be called
    from user code.

    \return                 The next timeout time in milliseconds since boot, or
                            0 if there are no pending genwait_wait() calls
*/
uint64 genwait_next_timeout();

/** \cond */
/* Initialize the genwait system */
int genwait_init();

/* Shut down the genwait system */
void genwait_shutdown();
/** \endcond */


__END_DECLS

#endif	/* __KOS_GENWAIT_H */

