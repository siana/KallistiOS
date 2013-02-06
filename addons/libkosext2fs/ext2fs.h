/* KallistiOS ##version##

   ext2fs.h
   Copyright (C) 2012, 2013 Lawrence Sebald
*/

#ifndef __EXT2_EXT2FS_H
#define __EXT2_EXT2FS_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <kos/blockdev.h>

/* Tunable filesystem parameters. These must be set at compile time. */

/* Logarithm (base 2) of the maximum number of entries in the inode cache.
   Thus, the inode cache will take up (2^n) * 128 bytes of total space in RAM.
   Set this to a larger number to ensure you can have a lot of files open at
   once. Also, note that this is a global cache, regardless of how many
   filesystems you actually have mounted. */
#define EXT2_LOG_MAX_INODES     7

/* Logarithm (base 2) of the number of head nodes in the inode hash table. The
   larger this is, the more entries the root array for the inode hash table will
   have, and thus the lower the probability of collisions in the table. Each
   entry in the array is a list, so collisions aren't fatal or anything like
   that. Setting this to something larger than EXT2_LOG_MAX_INODES is somewhat
   silly. I suggest either setting it the same or slightly less than the above
   constant. */
#define EXT2_LOG_INODE_HASH     (EXT2_LOG_MAX_INODES - 2)

/* End tunable filesystem parameters. */

/* Opaque ext2 filesystem type */
struct ext2fs_struct;
typedef struct ext2fs_struct ext2_fs_t;

uint32_t ext2_block_size(const ext2_fs_t *fs);
uint32_t ext2_log_block_size(const ext2_fs_t *fs);

/* Initialize low-level structures (like the global inode cache). If you don't
   call this before calling ext2_fs_init(), it will be called for you before
   mounting the first filesystem. */
int ext2_init(void);

ext2_fs_t *ext2_fs_init(kos_blockdev_t *bd);
void ext2_fs_shutdown(ext2_fs_t *fs);

int ext2_block_read_nc(ext2_fs_t *fs, uint32_t block_num, uint8_t *rv);
uint8_t *ext2_block_read(ext2_fs_t *fs, uint32_t block_num, int cache);

#define EXT2_CACHE_INODE    0
#define EXT2_CACHE_DIR      1
#define EXT2_CACHE_DATA     2

__END_DECLS

#endif /* !__EXT2_EXT2FS_H */
