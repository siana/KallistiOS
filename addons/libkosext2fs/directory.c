/* KallistiOS ##version##

   directory.c
   Copyright (C) 2013 Lawrence Sebald
*/

#include <string.h>

#include "ext2fs.h"
#include "ext2internal.h"
#include "directory.h"
#include "inode.h"

int ext2_dir_is_empty(ext2_fs_t *fs, const struct ext2_inode *dir) {
    uint32_t off, i, blocks;
    ext2_dirent_t *dent;
    uint8_t *buf;

    blocks = dir->i_blocks / (2 << fs->sb.s_log_block_size);

    for(i = 0; i < blocks; ++i) {
        off = 0;
        buf = ext2_inode_read_block(fs, dir, (uint32_t)i);

        while(off < fs->block_size) {
            dent = (ext2_dirent_t *)(buf + off);

            /* Make sure we don't trip and fall on a malformed entry. */
            if(!dent->rec_len)
                return -1;

            if(dent->inode) {
                /* Check if this is one of the few things we allow in an empty
                   directory (namely '.' and '..'). */
                if(dent->name_len > 2)
                    return 0;
                if(dent->name[0] != '.')
                    return 0;
                if(dent->name_len == 2 && dent->name[1] != '.')
                    return 0;
            }

            off += dent->rec_len;
        }
    }

    return 1;
}

ext2_dirent_t *ext2_dir_entry(ext2_fs_t *fs, const struct ext2_inode *dir,
                              const char *fn) {
    uint32_t off, i, blocks;
    ext2_dirent_t *dent;
    uint8_t *buf;
    size_t len = strlen(fn);

    blocks = dir->i_blocks / (2 << fs->sb.s_log_block_size);

    for(i = 0; i < blocks; ++i) {
        off = 0;
        buf = ext2_inode_read_block(fs, dir, (uint32_t)i);

        while(off < fs->block_size) {
            dent = (ext2_dirent_t *)(buf + off);

            /* Make sure we don't trip and fall on a malformed entry. */
            if(!dent->rec_len)
                return NULL;

            if(dent->inode) {
                /* Check if this what we're looking for. */
                if(dent->name_len == len && !memcmp(dent->name, fn, len))
                    return dent;
            }

            off += dent->rec_len;
        }
    }

    /* Didn't find it, oh well. */
    return NULL;
}
