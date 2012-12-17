/* KallistiOS ##version##

   block.c
   Copyright (C) 2012 Lawrence Sebald
*/

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "ext2fs.h"
#include "block.h"
#include "ext2internal.h"

int ext2_read_blockgroups(ext2_fs_t *fs, uint32_t start_block) {
    uint8_t *buf;
    ext2_bg_desc_t *ptr = fs->bg;
    uint32_t bg_per_block;
    int block_size = 1024 << fs->sb.s_log_block_size;
    uint32_t count = fs->bg_count;

    if(!(buf = (uint8_t *)malloc(block_size)))
        return -ENOMEM;

    bg_per_block = block_size / sizeof(ext2_bg_desc_t);

    while(count) {
        if(ext2_block_read_nc(fs, start_block++, buf)) {
            free(buf);
            return -EIO;
        }

        if(count < bg_per_block) {
            memcpy(ptr, buf, count * sizeof(ext2_bg_desc_t));
            count = 0;
        }
        else {
            memcpy(ptr, buf, bg_per_block * sizeof(ext2_bg_desc_t));
            ptr += bg_per_block;
            count -= bg_per_block;
        }
    }

    free(buf);
    return 0;
}
