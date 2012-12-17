/* KallistiOS ##version##

   ext2internal.h
   Copyright (C) 2012 Lawrence Sebald
*/

#include "block.h"
#include "superblock.h"
#include <kos/blockdev.h>

#ifndef __EXT2_EXT2INTERNAL_H
#define __EXT2_EXT2INTERNAL_H

typedef struct ext2_cache {
    int valid;
    uint8_t *data;
    uint32_t block;
} ext2_cache_t;

struct ext2fs_struct {
    kos_blockdev_t *dev;
    ext2_superblock_t sb;
    uint32_t block_size;
    
    uint32_t bg_count;
    ext2_bg_desc_t *bg;
    
    ext2_cache_t **icache;
    ext2_cache_t **dcache;
    ext2_cache_t **bcache;
    int cache_size;
};

#endif /* !__EXT2_EXT2INTERNAL_H */
