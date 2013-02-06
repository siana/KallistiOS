/* KallistiOS ##version##

   ext2fs.c
   Copyright (C) 2012, 2013 Lawrence Sebald
*/

#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include "superblock.h"
#include "inode.h"
#include "utils.h"
#include "block.h"
#include "ext2fs.h"
#include "directory.h"
#include "ext2internal.h"

static int initted = 0;

/* This is basically the same as bgrad_cache from fs_iso9660 */
static void make_mru(ext2_fs_t *fs, ext2_cache_t **cache, int block) {
    int i;
    ext2_cache_t *tmp;

    /* Don't try it with the end block */
    if(block < 0 || block >= fs->cache_size - 1)
        return;

    /* Make a copy and scoot everything down */
    tmp = cache[block];

    for(i = block; i < fs->cache_size - 1; ++i) {
        cache[i] = cache[i + 1];
    }

    cache[fs->cache_size - 1] = tmp;
}

/* XXXX: This needs locking! */
static uint8_t *read_cache(ext2_fs_t *fs, ext2_cache_t **cache, uint32_t bl) {
    int i;
    uint8_t *rv;

    /* Look through the cache from the most recently used to the least recently
       used entry. */
    for(i = fs->cache_size - 1; i >= 0 && cache[i]->valid; --i) {
        if(cache[i]->block == bl) {
            rv = cache[i]->data;
            make_mru(fs, cache, i);
            goto out;
        }
    }

    /* If we didn't get anything, did we end up with an invalid entry or do we
       need to boot someone out? */
    if(i < 0)
        i = 0;

    /* Try to read the block in question. */
    if(ext2_block_read_nc(fs, bl, cache[i]->data))
        return NULL;

    cache[i]->block = bl;
    cache[i]->valid = 1;
    rv = cache[i]->data;
    make_mru(fs, cache, i);

out:
    return rv;
}

int ext2_block_read_nc(ext2_fs_t *fs, uint32_t block_num, uint8_t *rv) {
    int fs_per_block = fs->sb.s_log_block_size - fs->dev->l_block_size + 10;

    if(fs_per_block < 0)
        /* This should never happen, as the ext2 block size must be at least
           as large as the sector size of the block device itself. */
        return -EINVAL;

    if(fs->sb.s_blocks_count <= block_num)
        return -EINVAL;

    if(fs->dev->read_blocks(fs->dev, block_num << fs_per_block,
                            1 << fs_per_block, rv))
        return -EIO;

    return 0;
}

uint8_t *ext2_block_read(ext2_fs_t *fs, uint32_t block_num, int cache) {
    uint8_t *rv;
    ext2_cache_t **cb;

    /* Figure out what cache we're looking at */
    switch(cache) {
        case EXT2_CACHE_INODE:
            cb = fs->icache;
            break;

        case EXT2_CACHE_DIR:
            cb = fs->dcache;
            break;

        case EXT2_CACHE_DATA:
            cb = fs->bcache;
            break;

        default:
            errno = EINVAL;
            return NULL;
    }

    /* Try to read from it. */
    if(!(rv = read_cache(fs, cb, block_num))) {
        errno = EIO;
        return NULL;
    }

    return rv;
}

uint32_t ext2_block_size(const ext2_fs_t *fs) {
    return fs->block_size;
}

uint32_t ext2_log_block_size(const ext2_fs_t *fs) {
    return fs->sb.s_log_block_size + 10;
}

int ext2_init(void) {
    ext2_inode_init();
    initted = 1;

    return 0;
}

ext2_fs_t *ext2_fs_init(kos_blockdev_t *bd) {
    ext2_fs_t *rv;
    uint32_t bc;
    int j;
    int block_size;

#ifdef EXT2FS_DEBUG
    uint32_t tmp;
    uint32_t p3 = 3, p5 = 5, p7 = 7, i;
#endif

    /* Make sure we've initialized any of the lower-level stuff. */
    if(!initted) {
        if(ext2_init())
            return NULL;
    }

    if(bd->init(bd))
        return NULL;

    if(!(rv = (ext2_fs_t *)malloc(sizeof(ext2_fs_t)))) {
        bd->shutdown(bd);
        return NULL;
    }

    rv->dev = bd;

    /* Read in the all-important superblock. */
    if(ext2_read_superblock(&rv->sb, bd)) {
        free(rv);
        bd->shutdown(bd);
        return NULL;
    }

    rv->block_size = block_size = 1024 << rv->sb.s_log_block_size;

#ifdef EXT2FS_DEBUG
    ext2_print_superblock(&rv->sb);
#endif

    /* Figure out how many block groups we have, based on the number of blocks
       and the blocks per group.
       Should we check this against the inodes too? */
    bc = rv->sb.s_blocks_count - rv->sb.s_first_data_block;
    rv->bg_count = bc / rv->sb.s_blocks_per_group;
    if(bc % rv->sb.s_blocks_per_group)
        ++rv->bg_count;

#ifdef EXT2FS_DEBUG
    dbglog(DBG_KDEBUG, "ext2fs has %" PRIu32 " block groups\n", rv->bg_count);

    dbglog(DBG_KDEBUG, "Superblocks are stored on the following blocks:\n");
    tmp = rv->sb.s_first_data_block;

    if(rv->sb.s_rev_level == EXT2_GOOD_OLD_REV) {
        while(tmp < bc) {
            dbglog(DBG_KDEBUG, "%" PRIu32 "\n", tmp);
            tmp += rv->sb.s_blocks_per_group;
        }
    }
    else {
        dbglog(DBG_KDEBUG, "%" PRIu32 "\n", tmp);
        tmp += rv->sb.s_blocks_per_group;
        if(tmp < bc)
            dbglog(DBG_KDEBUG, "%" PRIu32 "\n", tmp);

        while(tmp < bc) {
            if(p3 < p5 && p3 < p7) {
                tmp = rv->sb.s_first_data_block +
                    rv->sb.s_blocks_per_group * p3;

                if(tmp < bc)
                    dbglog(DBG_KDEBUG, "%" PRIu32 "\n", tmp);
                p3 *= 3;
            }
            else if(p5 < p3 && p5 < p7) {
                tmp = rv->sb.s_first_data_block +
                    rv->sb.s_blocks_per_group * p5;

                if(tmp < bc)
                    dbglog(DBG_KDEBUG, "%" PRIu32 "\n", tmp);
                p5 *= 5;
            }
            else {
                tmp = rv->sb.s_first_data_block +
                    rv->sb.s_blocks_per_group * p7;

                if(tmp < bc)
                    dbglog(DBG_KDEBUG, "%" PRIu32 "\n", tmp);
                p7 *= 7;
            }
        }
    }
#endif /* EXT2FS_DEBUG */

    /* Allocate space for and read in the block group descriptors. */
    if(!(rv->bg = (ext2_bg_desc_t *)malloc(sizeof(ext2_bg_desc_t) *
                                           rv->bg_count))) {
        free(rv);
        bd->shutdown(bd);
        return NULL;
    }

    if(ext2_read_blockgroups(rv, rv->sb.s_first_data_block + 1)) {
        free(rv->bg);
        free(rv);
        bd->shutdown(bd);
        return NULL;
    }

#ifdef EXT2FS_DEBUG
    for(i = 0; i < rv->bg_count; ++i) {
        dbglog(DBG_KDEBUG, "Block Group %" PRIu32 " info:\n", i);
        dbglog(DBG_KDEBUG, "Block Bitmap @ %" PRIu32 "\n",
               rv->bg[i].bg_block_bitmap);
        dbglog(DBG_KDEBUG, "Inode Bitmap @ %" PRIu32 "\n",
               rv->bg[i].bg_inode_bitmap);
        dbglog(DBG_KDEBUG, "Inode Table @ %" PRIu32 "\n",
               rv->bg[i].bg_inode_table);
        dbglog(DBG_KDEBUG, "Free blocks: %" PRIu16 "\n",
               rv->bg[i].bg_free_blocks_count);
        dbglog(DBG_KDEBUG, "Free inodes: %" PRIu16 "\n",
               rv->bg[i].bg_free_inodes_count);
        dbglog(DBG_KDEBUG, "Directory inodes: %" PRIu16 "\n",
               rv->bg[i].bg_used_dirs_count);
    }
#endif /* EXT2FS_DEBUG */

    /* Make space for the caches. */
    if(!(rv->icache = (ext2_cache_t **)malloc(sizeof(ext2_cache_t *) * 16))) {
        free(rv->bg);
        free(rv);
        bd->shutdown(bd);
        return NULL;
    }

    if(!(rv->dcache = (ext2_cache_t **)malloc(sizeof(ext2_cache_t *) * 16))) {
        free(rv->icache);
        free(rv->bg);
        free(rv);
        bd->shutdown(bd);
        return NULL;
    }

    if(!(rv->bcache = (ext2_cache_t **)malloc(sizeof(ext2_cache_t *) * 16))) {
        free(rv->dcache);
        free(rv->icache);
        free(rv->bg);
        free(rv);
        bd->shutdown(bd);
        return NULL;
    }
    
    for(j = 0; j < 16; ++j) {
        if(!(rv->icache[j] = (ext2_cache_t *)malloc(sizeof(ext2_cache_t))))
            goto out_cache;

        if(!(rv->dcache[j] = (ext2_cache_t *)malloc(sizeof(ext2_cache_t))))
            goto out_cache;

        if(!(rv->bcache[j] = (ext2_cache_t *)malloc(sizeof(ext2_cache_t))))
            goto out_cache;
    }

    for(j = 0; j < 16; ++j) {
        if(!(rv->icache[j]->data = (uint8_t *)malloc(block_size)))
            goto out_bcache;

        if(!(rv->dcache[j]->data = (uint8_t *)malloc(block_size)))
            goto out_bcache;

        if(!(rv->bcache[j]->data = (uint8_t *)malloc(block_size)))
            goto out_bcache;

        rv->icache[j]->valid = rv->dcache[j]->valid = rv->bcache[j]->valid = 0;
    }

    rv->cache_size = 16;

    return rv;

out_bcache:
    for(; j >= 0; --j) {
        free(rv->bcache[j]->data);
        free(rv->dcache[j]->data);
        free(rv->icache[j]->data);
    }

    j = 15;
out_cache:
    for(; j >= 0; --j) {
        free(rv->bcache[j]);
        free(rv->dcache[j]);
        free(rv->icache[j]);
    }

    free(rv->bcache);
    free(rv->dcache);
    free(rv->icache);
    free(rv->bg);
    free(rv);
    bd->shutdown(bd);
    return NULL;
}

void ext2_fs_shutdown(ext2_fs_t *fs) {
    int i;

    for(i = 0; i < fs->cache_size; ++i) {
        free(fs->icache[i]->data);
        free(fs->icache[i]);
        free(fs->dcache[i]->data);
        free(fs->dcache[i]);
        free(fs->bcache[i]->data);
        free(fs->bcache[i]);
    }

    free(fs->bcache);
    free(fs->dcache);
    free(fs->icache);

    fs->dev->shutdown(fs->dev);
    free(fs->bg);
    free(fs);
}
