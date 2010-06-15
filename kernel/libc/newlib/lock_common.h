/* KallistiOS ##version##

   lock_common.h
   Copyright (C)2004 Dan Potter

*/

#ifndef __NEWLIB_LOCK_COMMON_H
#define __NEWLIB_LOCK_COMMON_H

typedef struct {
	void	* owner;
	int	nest;
	volatile int lock;
} __newlib_recursive_lock_t;

#define __NEWLIB_RECURSIVE_LOCK_INIT { (void *)0, 0, 0 }

typedef volatile int __newlib_lock_t;
#define __NEWLIB_LOCK_INIT 0

void __newlib_lock_init(__newlib_lock_t *);
void __newlib_lock_close(__newlib_lock_t *);
void __newlib_lock_acquire(__newlib_lock_t *);
void __newlib_lock_try_acquire(__newlib_lock_t *);
void __newlib_lock_release(__newlib_lock_t *);

void __newlib_lock_init_recursive(__newlib_recursive_lock_t *);
void __newlib_lock_close_recursive(__newlib_recursive_lock_t *);
void __newlib_lock_acquire_recursive(__newlib_recursive_lock_t *);
void __newlib_lock_try_acquire_recursive(__newlib_recursive_lock_t *);
void __newlib_lock_release_recursive(__newlib_recursive_lock_t *);


#endif // __NEWLIB_LOCK_COMMON_H
