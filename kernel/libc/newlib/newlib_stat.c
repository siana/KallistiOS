/* KallistiOS ##version##

   newlib_stat.c
   Copyright (C) 2004 Dan Potter
   Copyright (C) 2011 Lawrence Sebald

*/

#include <unistd.h>
#include <sys/stat.h>
#include <sys/reent.h>
#include <kos/fs.h>
#include <errno.h>

int _stat_r(struct _reent * reent, const char * fn, struct stat * pstat) {
    file_t fp = fs_open(fn, O_RDONLY);
    mode_t md = S_IFREG;

    /* If we couldn't get it as a file, try as a directory */
    if(fp == FILEHND_INVALID) {
        fp = fs_open(fn, O_RDONLY | O_DIR);
        md = S_IFDIR;
    }

    /* If we still don't have it, then we're not going to get it. */
    if(fp == FILEHND_INVALID) {
        reent->_errno = ENOENT;
        return -1;
    }

    /* This really doesn't convey all that much information, but it should help
       with at least some uses of stat. */
    pstat->st_mode = md;
    pstat->st_size = (off_t)fs_total(fp);

    /* Clean up after ourselves. */
    fs_close(fp);

    return 0;
}
