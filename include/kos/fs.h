/* KallistiOS ##version##

   kos/fs.h
   Copyright (C) 2000, 2001, 2002, 2003 Dan Potter
   Copyright (C) 2012 Lawrence Sebald

*/

#ifndef __KOS_FS_H
#define __KOS_FS_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <sys/types.h>
#include <kos/limits.h>
#include <time.h>
#include <sys/queue.h>
#include <stdarg.h>

#include <kos/nmmgr.h>

/** \file   kos/fs.h
    \brief  Virtual filesystem support.

    This file contains the interface to the virtual filesystem (VFS) of KOS. The
    functions defined in this file make up the base of the filesystem operations
    that can be performed by programs. The functions in here are abstracted by
    various other layers in libc, and shouldn't be necessarily used (for
    portability reasons). However, if you want only to interact with KOS in your
    programs, feel free to use them to your heart's content!

    \author Dan Potter
    \author Lawrence Sebald
*/

/** \brief  Directory entry.

    All VFS handlers must conform to this interface in their directory entries.

    \headerfile kos/fs.h
*/
typedef struct kos_dirent {
    int size;               /**< \brief Size of the file in bytes. */
    char name[MAX_FN_LEN];  /**< \brief Name of the file. */
    time_t time;            /**< \brief Last access/mod/change time (depends on VFS) */
    uint32 attr;            /**< \brief Attributes of the file. */
} dirent_t;

/* Forward declaration */
struct vfs_handler;

/** \brief  File status information.

    This structure, while different from the standard POSIX stat structure,
    provides much of the same information. We limit it to only what is relevant
    for KOS.

    \headerfile kos/fs.h.
*/
typedef struct {
    struct vfs_handler *dev;    /**< \brief The VFS handler for this file/dir */
    uint32 unique;              /**< \brief A unique identifier on the VFS for this file/dir */
    uint32 type;                /**< \brief File/Dir type */
    uint32 attr;                /**< \brief Attributes */
    off_t size;                 /**< \brief Total file size, if applicable */
    time_t time;                /**< \brief Last access/mod/change time (depends on VFS) */
} stat_t;

/* stat_t.unique */
/**< \brief stat_t.unique: Constant to use denoting the file has no unique ID */
#define STAT_UNIQUE_NONE    0

/* stat_t.type */
/** \brief stat_t.type: Unknown / undefined / not relevant */
#define STAT_TYPE_NONE  0

/** \brief stat_t.type: Standard file */
#define STAT_TYPE_FILE  1

/** \brief stat_t.type: Standard directory */
#define STAT_TYPE_DIR   2

/** \brief stat_t.type: A virtual device of some sort (pipe, socket, etc) */
#define STAT_TYPE_PIPE  3

/** \brief stat_t.type: Meta data */
#define STAT_TYPE_META  4

/* stat_t.attr */
#define STAT_ATTR_NONE  0x00    /**< \brief stat_t.attr: No attributes */
#define STAT_ATTR_R     0x01    /**< \brief stat_t.attr: Read-capable */
#define STAT_ATTR_W     0x02    /**< \brief stat_t.attr: Write-capable */

/** \brief stat_t.attr: Read/Write capable */
#define STAT_ATTR_RW    (STAT_ATTR_R | STAT_ATTR_W)

/** \brief  File descriptor type */
typedef int file_t;

/** \brief  Invalid file handle constant (for open failure, etc) */
#define FILEHND_INVALID	((file_t)-1)

/** \brief  VFS handler interface.

    All VFS handlers must implement this interface.

    \headerfile kos/fs.h
*/
typedef struct vfs_handler {
    /** \brief Name manager handler header */
    nmmgr_handler_t nmmgr;

    /* Some VFS-specific pieces */
    /** \brief Allow VFS cacheing; 0=no, 1=yes */
    int cache;
    /** \brief Pointer to private data for the handler */
    void *privdata;

    /** \brief Open a file on the given VFS; return a unique identifier */
    void *(*open)(struct vfs_handler *vfs, const char *fn, int mode);

    /** \brief Close a previously opened file */
    void (*close)(void *hnd);

    /** \brief Read from a previously opened file */
    ssize_t (*read)(void *hnd, void *buffer, size_t cnt);

    /** \brief Write to a previously opened file */
    ssize_t (*write)(void *hnd, const void *buffer, size_t cnt);

    /** \brief Seek in a previously opened file */
    off_t (*seek)(void *hnd, off_t offset, int whence);

    /** \brief Return the current position in a previously opened file */
    off_t (*tell)(void *hnd);

    /** \brief Return the total size of a previously opened file */
    size_t (*total)(void *hnd);

    /** \brief Read the next directory entry in a directory opened with O_DIR */
    dirent_t *(*readdir)(void *hnd);

    /** \brief Execute a device-specific call on a previously opened file */
    int (*ioctl)(void *hnd, void *data, size_t size);

    /** \brief Rename/move a file on the given VFS */
    int (*rename)(struct vfs_handler *vfs, const char *fn1, const char *fn2);

    /** \brief Delete a file from the given VFS */
    int (*unlink)(struct vfs_handler *vfs, const char *fn);

    /** \brief "Memory map" a previously opened file */
    void *(*mmap)(void *fd);

    /** \brief Perform an I/O completion (async I/O) for a previously opened file */
    int (*complete)(void *fd, ssize_t *rv);

    /** \brief Get status information on a file on the given VFS */
    int (*stat)(struct vfs_handler *vfs, const char *fn, stat_t *rv);

    /** \brief Make a directory on the given VFS */
    int (*mkdir)(struct vfs_handler *vfs, const char *fn);

    /** \brief Remove a directory from the given VFS */
    int (*rmdir)(struct vfs_handler *vfs, const char *fn);

    /** \brief Manipulate file control flags on the given file. */
    int (*fcntl)(void *fd, int cmd, va_list ap);
} vfs_handler_t;

/** \brief  The number of distinct file descriptors that can be in use at a
            time.
*/
#define FD_SETSIZE	1024

/** \cond */
/* This is the private struct that will be used as raw file handles
   underlying descriptors. */
struct fs_hnd;

/* The kernel-wide file descriptor table. These will reference to open files. */
extern struct fs_hnd *fd_table[FD_SETSIZE];
/** \endcond */

/* Open modes */
#include <sys/fcntl.h>
/** \defgroup open_modes            File open modes

    @{
*/
#define O_MODE_MASK 0x0f		/**< \brief Mask for mode numbers */
//#define O_TRUNC		0x0100		/* Truncate */
#define O_ASYNC     0x0200      /**< \brief Open for asynchronous I/O */
//#define O_NONBLOCK	0x0400		/* Open for non-blocking I/O */
#define O_DIR       0x1000      /**< \brief Open as directory */
#define O_META      0x2000      /**< \brief Open as metadata */
/** @} */

/** \defgroup seek_modes            Seek modes

    These are the values you can pass for the whence parameter to fs_seek().

    @{
*/
#define SEEK_SET    0           /**< \brief Set position to offset. */
#define SEEK_CUR    1           /**< \brief Seek from current position. */
#define SEEK_END    2           /**< \brief Seek from end of file. */
/** @} */

/* Standard file descriptor functions */
/** \brief  Open a file on the VFS.

    This function opens the specified file, returning a new file descriptor to
    access the file.

    \param  fn              The path to open.
    \param  mode            The mode to use with opening the file. This may
                            include the standard open modes (O_RDONLY, O_WRONLY,
                            etc), as well as values from the \ref open_modes
                            list. Multiple values can be ORed together.
    \return                 The new file descriptor on success, -1 on error.
*/
file_t fs_open(const char *fn, int mode);

/** \brief  Close an opened file.

    This function closes the specified file descriptor, releasing all resources
    associated with the descriptor.

    \param  hnd             The file descriptor to close.
*/
void fs_close(file_t hnd);

/** \brief  Read from an opened file.

    This function reads into the specified buffer from the file at its current
    file pointer.

    \param  hnd             The file descriptor to read from.
    \param  buffer          The buffer to read into.
    \param  cnt             The size of the buffer (or the number of bytes
                            requested).
    \return                 The number of bytes read, or -1 on error. Note that
                            this may not be the full number of bytes requested.
*/
ssize_t fs_read(file_t hnd, void *buffer, size_t cnt);

/** \brief  Write to an opened file.

    This function writes the specfied buffer into the file at the current file
    pointer.

    \param  hnd             The file descriptor to write into.
    \param  buffer          The data to write into the file.
    \param  cnt             The size of the buffer, in bytes.
    \return                 The number of bytes written, or -1 on failure. Note
                            that the number of bytes written may be less than
                            what was requested.
*/
ssize_t fs_write(file_t hnd, const void *buffer, size_t cnt);

/** \brief  Seek to a new position within a file.

    This function moves the file pointer to the specified position within the
    file (the base of this position is determined by the whence parameter).

    \param  hnd             The file descriptor to move the pointer for.
    \param  offset          The offset in bytes from the specified base.
    \param  whence          The base of the pointer move. This should be one of
                            the \ref seek_modes values.
    \return                 The new position of the file pointer.
*/
off_t fs_seek(file_t hnd, off_t offset, int whence);

/** \brief  Retrieve the position of the pointer within a file.

    This function retrieves the current location of the file pointer within an
    opened file. This is an offset in bytes from the start of the file.

    \param  hnd             The file descriptor to retrieve the pointer from.
    \return                 The offset within the file for the pointer.
*/
off_t fs_tell(file_t hnd);

/** \brief  Retrieve the length of an opened file.

    This file retrieves the length of the file associated with the given file
    descriptor.

    \param  hnd             The file descriptor to retrieve the size from.
    \return                 The length of the file on success, -1 on failure.
*/
size_t fs_total(file_t hnd);

/** \brief  Read an entry from an opened directory.

    This function reads the next entry from the directory specified by the given
    file descriptor.

    \param  hnd             The opened directory's file descriptor.
    \return                 The next entry, or NULL on failure.
*/
dirent_t *fs_readdir(file_t hnd);

/** \brief  Execute a device-specific command on a file descriptor.

    The types and formats of the commands are device/filesystem specific, and
    are not documented here. Each filesystem may define any commands that are
    specific to it with its implementation of this function.

    \param  hnd             The file descriptor to operate on.
    \param  data            The command to send.
    \param  size            The size of the command, in bytes.
    \return                 -1 on failure.
*/
int fs_ioctl(file_t hnd, void *data, size_t size);

/** \brief  Rename the specified file to the given filename.

    This function renames the file specified by the first argument to the second
    argument. The two paths should be on the same filesystem.

    \param  fn1             The existing file to rename.
    \param  fn2             The new filename to rename to.
    \return                 0 on success, -1 on failure.
*/
int fs_rename(const char *fn1, const char *fn2);

/** \brief  Delete the specified file.

    This function deletes the specified file from the filesystem. This should
    only be used for files, not for directories. For directories, use fs_rmdir()
    instead of this function.

    \param  fn              The path to remove.
    \return                 0 on success, -1 on failure.
*/
int fs_unlink(const char *fn);

/** \brief  Change the current working directory of the current thread.

    This function changes the current working directory for the current thread.
    Any relative paths passed into file-related functions will be relative to
    the path that is changed to.

    \param  fn              The path to set as the current working directory.
    \return                 0 on success, -1 on failure.
*/
int fs_chdir(const char *fn);

/** \brief  Memory-map a previously opened file.

    This file "maps" the opened file into memory, reading the whole file into a
    buffer, and returning that buffer. The returned buffer should not be freed,
    as it will be freed when the file is closed. Bytes written into the buffer,
    up to the original length of the file, will be written back to the file when
    it is closed, assuming that the file is opened for writing.

    Note that some of the filesystems in KallistiOS do not support this
    operation.

    \param  hnd             The descriptor to memory map.
    \return                 The memory mapped buffer, or NULL on failure.
*/
void *fs_mmap(file_t hnd);

/** \brief  Perform an I/O completion on the given file descriptor.

    This function is used with asynchronous I/O to perform an I/O completion on
    the given file descriptor. Most filesystems do not support this operation
    on KallistiOS.

    \param  fd              The descriptor to complete I/O on.
    \param  rv              A buffer to store the size of the I/O in.
    \return                 0 on success, -1 on failure.
*/
int fs_complete(file_t fd, ssize_t *rv);

/** \brief  Retrieve information about the specified path.

    This function retrieves the stat_t structure for the given path on the VFS.
    This function is similar to the standard POSIX function stat(), but provides
    slightly different data than it does.

    \param  fn              The path to retrieve information about.
    \param  rv              The buffer to store stat information in.
    \return                 0 on success, -1 on failure.
*/
int fs_stat(const char *fn, stat_t *rv);

/** \brief  Create a directory.

    This function creates the specified directory, if possible.

    \param  fn              The path of the directory to create.
    \return                 0 on success, -1 on failure.
*/
int fs_mkdir(const char *fn);

/** \brief  Remove a directory by name.

    This function removes the specified directory. The directory shall only be
    removed if it is empty.

    \param  fn              The path of the directory to remove.
    \return                 0 on success, -1 on failure.
*/
int fs_rmdir(const char *fn);

/** \brief  Manipulate file control flags.

    This function implements the standard C fcntl function.

    \param  fd              The file descriptor to use.
    \param  cmd             The command to run.
    \param  ...             Arguments for the command specified.
    \return                 -1 on error (generally).
*/
int fs_fcntl(file_t fd, int cmd, ...);

/** \brief  Duplicate a file descriptor.

    This function duplicates the specified file descriptor, returning a new file
    descriptor that can be used to access the file. This is equivalent to the
    standard POSIX function dup().

    \param  oldfd           The old file descriptor to duplicate.
    \return                 The new file descriptor on success, -1 on failure.
*/
file_t fs_dup(file_t oldfd);

/** \brief  Duplicate a file descriptor onto the specified descriptor.

    This function duplicates the specified file descriptor onto the other file
    descriptor provided. If the newfd parameter represents an open file, that
    file will be closed before the old descriptor is duplicated onto it. This is
    equivalent to the standard POSIX function dup2().

    \param  oldfd           The old file descriptor to duplicate.
    \param  newfd           The descriptor to copy into.
    \return                 The new file descriptor on success, -1 on failure.
*/
file_t fs_dup2(file_t oldfd, file_t newfd);

/** \brief  Create a "transient" file descriptor.

    This function creates and opens a new file descriptor that isn't associated
    directly with a file on the filesystem. This is used internally to actually
    open files, and should (in general) not be called by user code. Effectively,
    if you're trying to implement your own filesystem handler in your code, you
    may need this function, otherwise you should just ignore it.

    \param  vfs             The VFS handler structure to use for the file.
    \param  hnd             Internal handle data for the file.
    \return                 The opened descriptor on success, -1 on failure.
*/
file_t fs_open_handle(vfs_handler_t *vfs, void *hnd);

/** \brief  Retrieve the VFS Handler for a file descriptor.

    This function retrieves the Handler structure for the VFS of the specified
    file descriptor. There is generally no reason to call this function in user
    code, as it is meant for use internally.

    \param  fd              The file descriptor to retrieve the handler for.
    \return                 The VFS' handler structure.
*/
vfs_handler_t *fs_get_handler(file_t fd);

/** \brief  Retrieve the internal handle for a file descriptor.

    This function retrieves the internal file handle data of the specified file
    descriptor. There is generally no reason to call this function in user code,
    as it is meant for use internally.

    \param  fd              The file descriptor to retrieve the handler for.
    \return                 The internal handle for the file descriptor.
*/
void *fs_get_handle(file_t fd);

/** \brief  Get the current working directory of the running thread.
    \return                 The current working directory.
*/
const char *fs_getwd();

/* Couple of util functions */

/** \brief  Copy a file.

    This function copies the file at src to dst on the filesystem.

    \param  src             The filename to copy from.
    \param  dst             The filename to copy to.
    \return                 The number of bytes copied successfully.
*/
ssize_t	fs_copy(const char *src, const char *dst);

/** \brief  Open and read a whole file into RAM.

    This function opens the specified file, reads it into memory (allocating the
    necessary space with malloc), and closes the file. The caller is responsible
    for freeing the memory when they are done with it.

    \param  src             The filename to open and read.
    \param  out_ptr         A pointer to the buffer on success, NULL otherwise.
    \return                 The size of the file on success, -1 otherwise.
*/
ssize_t fs_load(const char *src, void **out_ptr);

/** \brief  Initialize the virtual filesystem.

    This is normally done for you by default when KOS starts. In general, there
    should be no reason for you to call this function.

    \retval 0               On success.
*/
int fs_init();

/** \brief  Shut down the virtual filesystem.

    This is done for you by the normal shutdown procedure of KOS. There should
    not really be any reason for you to call this function yourself.
*/
void fs_shutdown();

__END_DECLS

#endif	/* __KOS_FS_H */
