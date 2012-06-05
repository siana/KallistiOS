/* KallistiOS ##version##

   pure_virtual.c
   Copyright (C)2003 Dan Potter

   Provides a libsupc++ function for using pure virtuals. Thanks to
   Bero for the info.
 */

#include <arch/arch.h>

void __cxa_pure_virtual() {
    panic("Pure virtual method called");
}

