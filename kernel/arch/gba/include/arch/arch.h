/* KallistiOS ##version##

   arch/gba/arch.h
   (c)2001 Dan Potter

*/

#ifndef __ARCH_ARCH_H
#define __ARCH_ARCH_H

#include <kos/cdefs.h>
#include <arch/types.h>

/* Number of timer ticks per second (if using threads) */
#define HZ		100

#define PAGESIZE	4096

/* Default thread stack size (if using threads) */
#define THD_STACK_SIZE	8192

/* Do we need symbol prefixes? */
#define ELF_SYM_PREFIX "_"
#define ELF_SYM_PREFIX_LEN 1
/* Default video mode */

/* Default serial parameters */

/* Panic function */
void panic(char *str);

/* Prototype for the portable kernel main() */
int kernel_main(const char *args);

/* Kernel C-level entry point */
int arch_main();

/* Kernel "quit" point */
void arch_exit();

/* Kernel "reboot" call */
void arch_reboot();

/* Use this macro to determine the level of initialization you'd like in
   your program by default. The defaults line will be fine for most things. */
#define KOS_INIT_FLAGS(flags)	uint32 __kos_init_flags = (flags)

extern uint32 __kos_init_flags;

/* Defaults */
#define INIT_DEFAULT \
	INIT_IRQ

/* Define a romdisk for your program, if you'd like one */
#define KOS_INIT_ROMDISK(rd)	void * __kos_romdisk = (rd)

extern void * __kos_romdisk;

/* State that you don't want a romdisk */
#define KOS_INIT_ROMDISK_NONE	NULL

/* Constants for the above */
#define INIT_NONE		0		/* Kernel enables */
#define INIT_IRQ		1
#define INIT_MALLOCSTATS	8

/* CPU sleep */
#define arch_sleep()

#endif	/* __ARCH_ARCH_H */

