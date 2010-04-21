/* KallistiOS ##version##

   assert.h
   Copyright (C)2002,2004 Dan Potter

*/

#ifndef __ASSERT_H
#define __ASSERT_H

#include <kos/cdefs.h>
__BEGIN_DECLS

/**
	\file assert.h
	\brief Standard C Assertions

	This file contains the standard C assertions to raise an assertion or to
	change the assertion handler.

	\author Dan Potter
*/

/* This is nice and simple, modeled after the BSD one like most of KOS;
   the addition here is assert_msg(), which allows you to provide an
   error message. */
#define _assert(e) assert(e)

/* __FUNCTION__ is not ANSI, it's GCC, but we depend on GCC anyway.. */
#ifdef NDEBUG
#	define assert(e) ((void)0)
#	define assert_msg(e, m) ((void)0)
#else
/**
	\brief standard C assertion

	\param e a value or expression to be evaluated as true or false

	\return if e is true (void)0 is returned, otherwise, the function does not
	return and abort() is called
*/
#	define assert(e)        ((e) ? (void)0 : __assert(__FILE__, __LINE__, #e, NULL, __FUNCTION__))

/**
	\brief assert with a custom message

	\param e a value or expression to be evaluated as true or false
	\param m a const char * message

	\return if e is true (void)0 is returned, otherwise the function does not
	return, a custom message is displayed, and abort() is called
*/
#	define assert_msg(e, m) ((e) ? (void)0 : __assert(__FILE__, __LINE__, #e, m, __FUNCTION__))
#endif

/* Defined in assert.c */
void __assert(const char *file, int line, const char *expr,
	const char *msg, const char *func);

/**
	\brief assert handler

	An assertion handler can be a user defined assertion handler,
	otherwise the default is used, which calls abort()

	\param file The filename where the assertion happened
	\param line The line number where the assertion happened
	\param expr The expression that raised the assertion
	\param msg A custom message for why the assertion happened
	\param func The function name from which the assertion happened

	\sa assert_set_handler
*/
typedef void (*assert_handler_t)(const char * file, int line, const char * expr,
	const char * msg, const char * func);

/**
	\brief set an "assert handler" to call on an assert

	By default, this will print a message and call abort().

	\return the old assert_handler_t address

	\sa assert_handler_t
*/
assert_handler_t assert_set_handler(assert_handler_t hnd);

__END_DECLS

#endif	/* __ASSERT_H */

