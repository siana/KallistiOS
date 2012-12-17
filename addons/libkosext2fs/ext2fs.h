/* KallistiOS ##version##

   ext2fs.h
   Copyright (C) 2012 Lawrence Sebald
*/

#ifndef __EXT2_EXT2FS_H
#define __EXT2_EXT2FS_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <kos/blockdev.h>

/* Opaque ext2 filesystem type */
struct ext2fs_struct;
typedef struct ext2fs_struct ext2_fs_t;

uint32_t ext2_block_size(const ext2_fs_t *fs);
uint32_t ext2_log_block_size(const ext2_fs_t *fs);

ext2_fs_t *ext2_fs_init(kos_blockdev_t *bd);
void ext2_fs_shutdown(ext2_fs_t *fs);

int ext2_block_read_nc(ext2_fs_t *fs, uint32_t block_num, uint8_t *rv);
uint8_t *ext2_block_read(ext2_fs_t *fs, uint32_t block_num, int cache);

#define EXT2_CACHE_INODE    0
#define EXT2_CACHE_DIR      1
#define EXT2_CACHE_DATA     2

__END_DECLS

#endif /* !__EXT2_EXT2FS_H */
