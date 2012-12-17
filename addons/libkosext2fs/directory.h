/* KallistiOS ##version##

   directory.h
   Copyright (C) 2012 Lawrence Sebald
*/

#ifndef __EXT2_DIRECTORY_H
#define __EXT2_DIRECTORY_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <stdint.h>

typedef struct ext2_dirent {
    uint32_t inode;
    uint16_t rec_len;
    uint8_t name_len;
    uint8_t file_type;
    uint8_t name[];
} ext2_dirent_t;

/* Values for file_type */
#define EXT2_FT_UNKNOWN     0
#define EXT2_FT_REG_FILE    1
#define EXT2_FT_DIR         2
#define EXT2_FT_CHRDEV      3
#define EXT2_FT_BLKDEV      4
#define EXT2_FT_FIFO        5
#define EXT2_FT_SOCK        6
#define EXT2_FT_SYMLINK     7

__END_DECLS
#endif /* !__EXT2_DIRECTORY_H */
