/* KallistiOS ##version##

   directory.c
   Copyright (C) 2013 Lawrence Sebald
*/

#include <string.h>
#include <errno.h>

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
        buf = ext2_inode_read_block(fs, dir, i, NULL);

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
        buf = ext2_inode_read_block(fs, dir, i, NULL);

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

int ext2_dir_rm_entry(ext2_fs_t *fs, const struct ext2_inode *dir,
                      const char *fn, uint32_t *inode) {
    uint32_t off, i, blocks, bn;
    ext2_dirent_t *dent, *prev;
    uint8_t *buf;
    size_t len = strlen(fn);

    /* Don't even bother if we're mounted read-only. */
    if(!(fs->mnt_flags & EXT2FS_MNT_FLAG_RW))
        return -EROFS;

    blocks = dir->i_blocks / (2 << fs->sb.s_log_block_size);

    for(i = 0; i < blocks; ++i) {
        off = 0;
        prev = NULL;
        dent = NULL;
        buf = ext2_inode_read_block(fs, dir, i, &bn);

        while(off < fs->block_size) {
            prev = dent;
            dent = (ext2_dirent_t *)(buf + off);

            /* Make sure we don't trip and fall on a malformed entry. */
            if(!dent->rec_len)
                return -EIO;

            if(dent->inode) {
                /* Check if this what we're looking for. */
                if(dent->name_len == len && !memcmp(dent->name, fn, len)) {
                    /* Return the inode number to the calling function. */
                    *inode = dent->inode;

                    if(prev) {
                        /* Remove it from the chain and clear the entry. */
                        prev->rec_len += dent->rec_len;
                        memset(dent, 0, dent->rec_len);
                    }
                    else {
                        /* This is the first entry in a block, so simply mark
                           the entry as invalid, and clear the filename and such
                           from it. */
                        dent->inode = 0;
                        memset(dent->name, 0, dent->name_len);
                        dent->name_len = dent->file_type = 0;
                    }

                    /* Mark the block as dirty so that it gets rewritten to the
                       block device. */
                    ext2_block_mark_dirty(fs, bn);
                    return 0;
                }
            }

            off += dent->rec_len;
        }
    }

    /* Didn't find it, oh well. */
    return -ENOENT;
}
