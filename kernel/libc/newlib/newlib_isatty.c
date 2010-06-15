/* KallistiOS ##version##

   newlib_isatty.c
   Copyright (C)2004 Dan Potter

*/

#include <sys/reent.h>

int isatty(int fd) {
	return 0;
}

int _isatty_r(struct _reent *reent, int fd) {
    return 0;
}
