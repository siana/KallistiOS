/* KallistiOS ##version##

   dirent.h
   Copyright (C)2003 Dan Potter

*/

/** \file   dirent.h
    \brief  Standard POSIX dirent functionality

    This partially implements the standard POSIX dirent.h functionality.

    \author Dan Potter
*/

#ifndef __SYS_DIRENT_H
#define __SYS_DIRENT_H

#include <unistd.h>
#include <arch/types.h>
#include <kos/fs.h>

/** \brief The POSIX dirent which describes a directory entry
 */
struct dirent {
    int d_ino; /**< \brief the file number */
    off_t   d_off; /**< \brief the file offset */
    uint16  d_reclen; /**< \brief the record length */
    uint8   d_type; /**< \brief the type */
    char    d_name[256]; /**< \brief the entry name */
};

/** \brief the DIR structure in KOS

    In KOS, DIR * is just an fd, but we use a struct so we can also include the
    POSIX dirent.
*/
typedef struct {
    file_t      fd; /**< \brief the file descriptor */
    struct dirent   d_ent; /**< \brief the POSIX dirent */
} DIR;

// Standard UNIX dir functions. Not all of these are fully functional
// right now due to lack of support in KOS.

// All of these work.
/** \brief Opens a directory based on the specified name

    The directory specified by name is opened if it exists and returns a
    directory structure that must be later closed with closedir.

    \param name The string name of the dir to open.
    \return A directory structure that can be used with readdir
    \note I believe you can use relative paths with opendir, but it depends on
        the current working directory (getcwd)
    \see closedir
    \see readdir
*/
DIR *opendir(const char *name);

/** \brief Closes a currently opened directory

    Close a DIR that was previously opened with opendir.

    \param dir The DIR that was returned from an opendir.
    \return 0 on success, or -1 on error.
*/
int closedir(DIR *dir);

/** \brief Read the contents of an open directory

    Read the contents of an open directory and returns a pointer to the current
    directory entry. Recurring calls to readdir return the next directory entry.

    \note Do not free the returned dirent
    \param dir The directory structure that was returned from an opendir
    \return A pointer to the current diretory entry or NULL when there are no
        more entries.
*/
struct dirent *readdir(DIR *dir);

/** \brief Not implemented */
void rewinddir(DIR *dir);
/** \brief Not implemented */
int scandir(const char *dir, struct dirent ***namelist,
            int(*filter)(const struct dirent *),
            int(*compar)(const struct dirent **, const struct dirent **));
/** \brief Not implemented */
void seekdir(DIR *dir, off_t offset);
/** \brief Not implemented */
off_t telldir(DIR *dir);

#endif

