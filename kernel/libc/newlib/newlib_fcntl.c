/* KallistiOS ##version##

   newlib_fcntl.c
   Copyright (C) 2012 Lawrence Sebald

*/

#include <sys/reent.h>
#include <sys/fcntl.h>
#include <stdarg.h>

#include <kos/fs.h>

int _fcntl_r(struct _reent *reent, int fd, int cmd, int arg) {
	return fs_fcntl(fd, cmd, arg);
}
