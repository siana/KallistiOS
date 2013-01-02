/* KallistiOS ##version##

   inode.c
   Copyright (C) 2012, 2013 Lawrence Sebald
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <limits.h>

#include "inode.h"
#include "utils.h"
#include "ext2fs.h"
#include "ext2internal.h"
#include "directory.h"

ext2_inode_t *ext2_inode_read(ext2_fs_t *fs, uint32_t inode_num) {
    uint32_t bg, index;
    uint8_t *buf;
    int in_per_block;
    uint32_t inode_block;

    in_per_block = (fs->block_size) / fs->sb.s_inode_size;

    /* Figure out what block group and index within that group the inode in
       question is. */
    bg = (inode_num - 1) / fs->sb.s_inodes_per_group;
    index = (inode_num - 1) % fs->sb.s_inodes_per_group;

    if(inode_num > fs->sb.s_inodes_count)
        return NULL;

    if(!(buf = ext2_block_read(fs, fs->bg[bg].bg_inode_bitmap,
                               EXT2_CACHE_INODE)))
        return NULL;

    if(!ext2_bit_is_set((uint32_t *)buf, index))
        return NULL;

    /* Read the block containing the inode in.
       TODO: Should we check if the block is marked as in use? */
    inode_block = fs->bg[bg].bg_inode_table + (index / in_per_block);
    index %= in_per_block;

    if(!(buf = ext2_block_read(fs, inode_block, EXT2_CACHE_INODE)))
        return NULL;

    /* Return the inode in question  */
    return (ext2_inode_t *)(buf + (index * fs->sb.s_inode_size));
}

static ext2_dirent_t *search_dir(uint8_t *buf, int block_size,
                                 const char *token, int *err) {
    int block_offset = 0;
    ext2_dirent_t *dent;
    char name[256];

    while(block_offset < block_size) {
        dent = (ext2_dirent_t *)(buf + block_offset);

        /* Make sure we don't trip and fall on a malformed entry. */
        if(!dent->rec_len) {
            *err = -EIO;
            return NULL;
        }

        if(dent->inode) {
            /* See if this is what we're looking for. */
            memcpy(name, dent->name, dent->name_len);
            name[dent->name_len] = 0;

            if(!strcmp(name, token))
                return dent;
        }

        block_offset += dent->rec_len;
    }

    return NULL;
}

static ext2_dirent_t *search_indir(ext2_fs_t *fs, const uint32_t *iblock,
                                   int block_size, const char *token,
                                   int *err) {
    uint8_t *buf;
    int i, block_ents;
    ext2_dirent_t *rv;

    /* We're going to need this buffer... */
    if(!(buf = (uint8_t *)malloc(block_size))) {
        *err = -ENOMEM;
        return NULL;
    }

    block_ents = block_size >> 2;

    /* Search through each block until we get to the end. */
    for(i = 0; i < block_ents && iblock[i]; ++i) {
        if(ext2_block_read_nc(fs, iblock[i], buf)) {
            free(buf);
            *err = -EIO;
            return NULL;
        }

        if((rv = search_dir(buf, block_size, token, err))) {
            free(buf);
            *err = 0;
            return rv;
        }
        else if(*err) {
            free(buf);
            return NULL;
        }
    }

    free(buf);
    *err = 0;
    return NULL;
}

static ext2_dirent_t *search_indir_23(ext2_fs_t *fs, const uint32_t *iblock,
                                      int block_size, const char *token,
                                      int *err, int triple) {
    uint32_t *buf;
    int i, block_ents;
    ext2_dirent_t *rv;

    /* We're going to need this buffer... */
    if(!(buf = (uint32_t *)malloc(block_size))) {
        *err = -ENOMEM;
        return NULL;
    }

    block_ents = block_size >> 2;

    if(!triple) {
        /* Search through each indirect block until we get to the end. */
        for(i = 0; i < block_ents && iblock[i]; ++i) {
            if(ext2_block_read_nc(fs, iblock[i], (uint8_t *)buf)) {
                free(buf);
                *err = -EIO;
                return NULL;
            }

            if((rv = search_indir(fs, buf, block_size, token, err))) {
                free(buf);
                *err = 0;
                return rv;
            }
        }
    }
    else {
        /* Search through each doubly-indirect block until we get to the end. */
        for(i = 0; i < block_size && iblock[i]; ++i) {
            if(ext2_block_read_nc(fs, iblock[i], (uint8_t *)buf)) {
                free(buf);
                *err = -EIO;
                return NULL;
            }

            if((rv = search_indir_23(fs, buf, block_size, token, err, 0))) {
                free(buf);
                *err = 0;
                return rv;
            }
        }
    }

    free(buf);
    *err = 0;
    return NULL;
}

int ext2_inode_by_path(ext2_fs_t *fs, const char *path, ext2_inode_t *rv,
                       uint32_t *inode_num, int rlink, ext2_dirent_t **rdent) {
    ext2_inode_t *inode, *last;
    char *ipath, *cxt, *token;
    int blocks, i, block_size;
    uint8_t *buf;
    uint32_t *iblock;
    ext2_dirent_t *dent;
    int err = 0;
    size_t tmp_sz;
    char *symbuf;
    int links_derefed = 0;

    if(!path || !fs || !rv)
        return -EFAULT;

    /* Read the root directory inode first. */
    if(!(inode = ext2_inode_read(fs, EXT2_ROOT_INO)))
        return -EIO;

    /* We're going to tokenize the string into its component parts, so make a
       copy of the path to go from here. */
    if(!(ipath = strdup(path)))
        return -ENOMEM;

    token = strtok_r(ipath, "/", &cxt);

    /* If we get nothing back here, they gave us /. Give them back the root
       directory inode. */
    if(!token) {
        free(ipath);
        *rv = *inode;
        return 0;
    }

    block_size = fs->block_size;

    while(token) {
        last = inode;

        /* If this isn't a directory, give up now. */
        if(!(inode->i_mode & EXT2_S_IFDIR)) {
            free(ipath);
            return -ENOTDIR;
        }

        blocks = inode->i_blocks / (2 << fs->sb.s_log_block_size);

        /* Run through any direct blocks in the inode. */
        for(i = 0; i < blocks && inode->i_block[i] && i < 12; ++i) {
            /* Grab the block, looking in the directory cache. */
            if(!(buf = ext2_block_read(fs, inode->i_block[i],
                                       EXT2_CACHE_DIR))) {
                free(ipath);
                return -EIO;
            }

            /* Search through the directory block */
            if((dent = search_dir(buf, block_size, token, &err))) {
                goto next_token;
            }
            else if(err) {
                free(ipath);
                return err;
            }
        }

        /* Short circuit... */
        if(!inode->i_block[i])
            goto out;

        /* Next, look through the indirect block. */
        if(!(iblock = (uint32_t *)ext2_block_read(fs, inode->i_block[12],
                                                  EXT2_CACHE_DIR))) {
            free(ipath);
            return -EIO;
        }

        if((dent = search_indir(fs, iblock, block_size, token, &err))) {
            goto next_token;
        }
        else if(err) {
            free(ipath);
            return err;
        }

        /* Next, look through the doubly-indirect block. */
        if(inode->i_block[13]) {
            /* Grab the block, looking in the directory cache. */
            if(!(iblock = (uint32_t *)ext2_block_read(fs, inode->i_block[13],
                                                      EXT2_CACHE_DIR))) {
                free(ipath);
                return -EIO;
            }

            if((dent = search_indir_23(fs, iblock, block_size, token, &err,
                                       0))) {
                goto next_token;
            }
            else if(err) {
                free(ipath);
                return err;
            }
        }

        /* Finally, try the triply-indirect block... God help us if we actually
           have to look all the way through one of these... */
        if(inode->i_block[14]) {
            /* Grab the block, looking in the directory cache. */
            if(!(iblock = (uint32_t *)ext2_block_read(fs, inode->i_block[14],
                                                      EXT2_CACHE_DIR))) {
                free(ipath);
                return -EIO;
            }

            if((dent = search_indir_23(fs, iblock, block_size, token, &err,
                                       1))) {
                goto next_token;
            }
            else if(err) {
                free(ipath);
                return err;
            }
        }

out:
        /* If we get here, we didn't find the next entry. Return that error. */
        if((token = strtok_r(NULL, "/", &cxt))) {
            free(ipath);
            return -ENOTDIR;
        }
        else {
            free(ipath);
            return -ENOENT;
        }

next_token:
        token = strtok_r(NULL, "/", &cxt);
        if(!(inode = ext2_inode_read(fs, dent->inode))) {
            free(ipath);
            return -EIO;
        }

        /* Are we supposed to resolve symbolic links? If we have one and we're
           supposed to resolve them, do it. */
        if((inode->i_mode & 0xF000) == EXT2_S_IFLNK &&
           (rlink == 1 || (rlink == 2 && token))) {
            /* Make sure we don't fall into an infinite loop... */
            if(links_derefed++ > SYMLOOP_MAX) {
                free(ipath);
                return -ELOOP;
            }

            tmp_sz = PATH_MAX;

            if(!(symbuf = (char *)malloc(PATH_MAX))) {
                free(ipath);
                return -ENOMEM;
            }

            if(ext2_resolve_symlink(fs, inode, symbuf, &tmp_sz)) {
                free(symbuf);
                free(ipath);
                return -EIO;
            }

            /* Make sure we got it all */
            if(tmp_sz >= PATH_MAX) {
                free(symbuf);
                free(ipath);
                return -ENAMETOOLONG;
            }

            /* For now, drop any absolute pathnames that we might encounter. At
               some point, I'll probably revisit this decision, but for now
               that's just how it is. */
            if(symbuf[0] == '/') {
                free(symbuf);
                free(ipath);
                return -EXDEV;
            }

            /* Tack on the rest of the path to the symbolic link. This is
               horribly inefficient, but it should work fine. */
            while(token) {
                if((tmp_sz += strlen(token) + 1) >= PATH_MAX) {
                    free(symbuf);
                    free(ipath);
                    return -ENAMETOOLONG;
                }

                strcat(symbuf, "/");
                strcat(symbuf, token);
                token = strtok_r(NULL, "/", &cxt);
            }

            /* Continue our search for the object in question, now that we've
               resolved the link... */
            free(ipath);
            ipath = symbuf;
            token = strtok_r(ipath, "/", &cxt);
            inode = last;
        }
    }

    /* Well, looks like we have it, return the inode. */
    *rv = *inode;
    *inode_num = dent->inode;
    free(ipath);

    if(rdent)
        *rdent = dent;
    return 0;
}

uint8_t *ext2_inode_read_block(ext2_fs_t *fs, const ext2_inode_t *inode,
                               uint32_t block_num) {
    int cache;
    uint32_t blks_per_ind, ibn;
    uint32_t *iblock;
    uint8_t *rv;

    /* Figure out what cache to read from first. */
    if(inode->i_mode & EXT2_S_IFDIR)
        cache = EXT2_CACHE_DIR;
    else
        cache = EXT2_CACHE_DATA;

    /* If we're reading a direct block, this is easy. */
    if(block_num < 12)
        return ext2_block_read(fs, inode->i_block[block_num], cache);

    blks_per_ind = fs->block_size >> 2;
    block_num -= 12;

    /* Are we looking at the singly-indirect block? */
    if(block_num < blks_per_ind) {
        if(!(iblock = (uint32_t *)ext2_block_read(fs, inode->i_block[12],
                                                  cache)))
            return NULL;

        rv = ext2_block_read(fs, iblock[block_num], cache);
        return rv;
    }

    /* Ok, we're looking at at least a doubly-indirect block... */
    block_num -= blks_per_ind;
    if(block_num < (blks_per_ind * blks_per_ind)) {
        if(!(iblock = (uint32_t *)ext2_block_read(fs, inode->i_block[13],
                                                  cache)))
            return NULL;

        /* Figure out what entry we want in here... */
        ibn = block_num / blks_per_ind;
        block_num %= blks_per_ind;

        if(!(iblock = (uint32_t *)ext2_block_read(fs, iblock[ibn], cache)))
            return NULL;

        /* Ok... Now we should be good to go. */
        rv = ext2_block_read(fs, iblock[block_num], cache);
        return rv;
    }

    /* Ugh... You're going to make me look at a triply-indirect block now? */
    block_num -= blks_per_ind * blks_per_ind;
    if(!(iblock = (uint32_t *)ext2_block_read(fs, inode->i_block[14], cache)))
        return NULL;

    /* Figure out what entry we want in here... */
    ibn = block_num / blks_per_ind;
    block_num %= blks_per_ind;

    if(!(iblock = (uint32_t *)ext2_block_read(fs, iblock[ibn], cache)))
        return NULL;

    /* And in this one too... */
    ibn = block_num / blks_per_ind;
    block_num %= blks_per_ind;

    if(!(iblock = (uint32_t *)ext2_block_read(fs, iblock[ibn], cache)))
        return NULL;

    /* Ok... Now we should be good to go. Finally. */
    if(block_num < blks_per_ind) {
        rv = ext2_block_read(fs, iblock[block_num], cache);
        return rv;
    }
    else {
        /* This really shouldn't happen... */
        errno = EIO;
        return NULL;
    }
}
