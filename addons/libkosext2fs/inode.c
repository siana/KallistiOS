/* KallistiOS ##version##

   inode.c
   Copyright (C) 2012, 2013 Lawrence Sebald
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include <limits.h>
#include <assert.h>
#include <sys/queue.h>
#include <inttypes.h>

#include "inode.h"
#include "utils.h"
#include "ext2fs.h"
#include "ext2internal.h"
#include "directory.h"

#define MAX_INODES      (1 << EXT2_LOG_MAX_INODES)
#define INODE_HASH_SZ   (1 << EXT2_LOG_INODE_HASH)

/* Internal inode storage structure. This is used for cacheing used inodes. */
static struct int_inode {
    /* Start with the on-disk inode itself to make the put() function easier. */
    ext2_inode_t inode;

    /* Hash table entry -- used at all points after the first time an inode is
       placed into this entry. */
    LIST_ENTRY(int_inode) entry;

    /* Unused list entry -- used initially by all inodes, and later when the
       inode has had its reference counter decremented to 0. Note that when the
       second case there happens, the inode will still be in the inode hash
       table. That way, if we happen to pull it back up again later, we still
       can use the cached version and not have to re-read it from the block
       device. */
    TAILQ_ENTRY(int_inode) qentry;

    /* Flags for this inode. */
    uint32_t flags;

    /* Reference count for the inode. */
    uint32_t refcnt;

    /* What filesystem is this inode on? */
    ext2_fs_t *fs;

    /* What inode number is this? */
    uint32_t inode_num;
} inodes[MAX_INODES];

/* Head types */
LIST_HEAD(inode_list, int_inode);
TAILQ_HEAD(inode_queue, int_inode);

/* Tail queue of free/unused inodes. */
static struct inode_queue free_inodes;

/* Hash table of inodes in use. */
static struct inode_list inode_hash[INODE_HASH_SZ];

/* Forward declaration... */
static ext2_inode_t *ext2_inode_read(ext2_fs_t *fs, uint32_t inode_num);

void ext2_inode_init(void) {
    int i;

    /* Initialize the hash table to its starting state */
    for(i = 0; i < INODE_HASH_SZ; ++i) {
        LIST_INIT(&inode_hash[i]);
    }

    /* Put all the inodes in the unused list */
    TAILQ_INIT(&free_inodes);

    for(i = 0; i < MAX_INODES; ++i) {
        inodes[i].flags = 0;
        inodes[i].inode_num = 0;
        inodes[i].refcnt = 0;
        TAILQ_INSERT_TAIL(&free_inodes, inodes + i, qentry);
    }
}

ext2_inode_t *ext2_inode_get(ext2_fs_t *fs, uint32_t inode_num, int *err) {
    int entry = inode_num & (INODE_HASH_SZ - 1);
    struct int_inode *i;
    ext2_inode_t *rinode;

    /* Figure out if this inode is already in the hash table. */
    LIST_FOREACH(i, &inode_hash[entry], entry) {
        if(i->fs == fs && i->inode_num == inode_num) {
            /* Increase the reference count, and see if it was free before. */
            if(!i->refcnt++) {
                /* It is in the free list. Remove it from the free list. */
                TAILQ_REMOVE(&free_inodes, i, qentry);
            }

#ifdef EXT2FS_DEBUG
            dbglog(DBG_KDEBUG, "ext2_inode_get: %" PRIu32 " (%" PRIu32
                   " refs)\n", inode_num, i->refcnt);
#endif
            return (ext2_inode_t *)i;
        }
    }

    /* Didn't find it... */
    if(!(i = TAILQ_FIRST(&free_inodes))) {
        /* Uh oh... No more free inodes... */
        *err = -ENFILE;
        return NULL;
    }

    /* Ok, at this point, we have a free inode, remove it from the free pool. */
    TAILQ_REMOVE(&free_inodes, i, qentry);
    i->refcnt = 1;
    i->inode_num = inode_num;
    i->fs = fs;

    /* Read the inode in from the block device. */
    if(!(rinode = ext2_inode_read(fs, inode_num))) {
        /* Hrm... what to do about that... */
        i->refcnt = 0;
        i->inode_num = 0;
        i->fs = NULL;
        *err = -EIO;
        return NULL;
    }

    /* Add it to the hash table. */
    i->inode = *rinode;
    LIST_INSERT_HEAD(&inode_hash[entry], i, entry);

#ifdef EXT2FS_DEBUG
    dbglog(DBG_KDEBUG, "ext2_inode_get: %" PRIu32 " (%" PRIu32 " refs)\n",
           inode_num, i->refcnt);
#endif

    /* Ok... That should do it. */
    return &i->inode;
}

void ext2_inode_put(ext2_inode_t *inode) {
    struct int_inode *iinode = (struct int_inode *)inode;

    /* Make sure we're not trying anything really mean. */
    assert(iinode->refcnt != 0);

    /* Decrement the reference counter, and see if we've got the last one. */
    if(!--iinode->refcnt) {
        /* Yep, we've gone and consumed the last reference, so put it on the
           free list at the end (in case we want to bring it back from the dead
           later on).
           XXXX: We should write it back out to the disk if it is dirty, but
           that is for another day. */
        TAILQ_INSERT_TAIL(&free_inodes, iinode, qentry);
    }

#ifdef EXT2FS_DEBUG
    /* Technically not thread-safe, but then again, there's many bigger issues
       with thread-safety in this library than this. */
    dbglog(DBG_KDEBUG, "ext2_inode_put: %" PRIu32 " (%" PRIu32 " refs)\n",
           iinode->inode_num, iinode->refcnt);
#endif
}

static ext2_inode_t *ext2_inode_read(ext2_fs_t *fs, uint32_t inode_num) {
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

int ext2_inode_by_path(ext2_fs_t *fs, const char *path, ext2_inode_t **rv,
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
    if(!(inode = ext2_inode_get(fs, EXT2_ROOT_INO, &err)))
        return err;

    /* We're going to tokenize the string into its component parts, so make a
       copy of the path to go from here. */
    if(!(ipath = strdup(path))) {
        ext2_inode_put(inode);
        return -ENOMEM;
    }

    token = strtok_r(ipath, "/", &cxt);

    /* If we get nothing back here, they gave us /. Give them back the root
       directory inode. */
    if(!token) {
        free(ipath);
        *rv = inode;
        *inode_num = EXT2_ROOT_INO;

        if(rdent)
            *rdent = NULL;
        return 0;
    }

    block_size = fs->block_size;

    while(token) {
        last = inode;

        /* If this isn't a directory, give up now. */
        if(!(inode->i_mode & EXT2_S_IFDIR)) {
            free(ipath);
            ext2_inode_put(inode);
            return -ENOTDIR;
        }

        blocks = inode->i_blocks / (2 << fs->sb.s_log_block_size);

        /* Run through any direct blocks in the inode. */
        for(i = 0; i < blocks && inode->i_block[i] && i < 12; ++i) {
            /* Grab the block, looking in the directory cache. */
            if(!(buf = ext2_block_read(fs, inode->i_block[i],
                                       EXT2_CACHE_DIR))) {
                free(ipath);
                ext2_inode_put(inode);
                return -EIO;
            }

            /* Search through the directory block */
            if((dent = search_dir(buf, block_size, token, &err))) {
                goto next_token;
            }
            else if(err) {
                free(ipath);
                ext2_inode_put(inode);
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
            ext2_inode_put(inode);
            return -EIO;
        }

        if((dent = search_indir(fs, iblock, block_size, token, &err))) {
            goto next_token;
        }
        else if(err) {
            free(ipath);
            ext2_inode_put(inode);
            return err;
        }

        /* Next, look through the doubly-indirect block. */
        if(inode->i_block[13]) {
            /* Grab the block, looking in the directory cache. */
            if(!(iblock = (uint32_t *)ext2_block_read(fs, inode->i_block[13],
                                                      EXT2_CACHE_DIR))) {
                free(ipath);
                ext2_inode_put(inode);
                return -EIO;
            }

            if((dent = search_indir_23(fs, iblock, block_size, token, &err,
                                       0))) {
                goto next_token;
            }
            else if(err) {
                free(ipath);
                ext2_inode_put(inode);
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
                ext2_inode_put(inode);
                return -EIO;
            }

            if((dent = search_indir_23(fs, iblock, block_size, token, &err,
                                       1))) {
                goto next_token;
            }
            else if(err) {
                free(ipath);
                ext2_inode_put(inode);
                return err;
            }
        }

out:
        /* If we get here, we didn't find the next entry. Return that error. */
        ext2_inode_put(inode);

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

        if(!(inode = ext2_inode_get(fs, dent->inode, &err))) {
            free(ipath);
            ext2_inode_put(last);
            return err;
        }

        /* Are we supposed to resolve symbolic links? If we have one and we're
           supposed to resolve them, do it. */
        if((inode->i_mode & 0xF000) == EXT2_S_IFLNK &&
           (rlink == 1 || (rlink == 2 && token))) {
            /* Make sure we don't fall into an infinite loop... */
            if(links_derefed++ > SYMLOOP_MAX) {
                free(ipath);
                ext2_inode_put(inode);
                return -ELOOP;
            }

            tmp_sz = PATH_MAX;

            if(!(symbuf = (char *)malloc(PATH_MAX))) {
                free(ipath);
                ext2_inode_put(inode);
                return -ENOMEM;
            }

            if(ext2_resolve_symlink(fs, inode, symbuf, &tmp_sz)) {
                free(symbuf);
                free(ipath);
                ext2_inode_put(inode);
                return -EIO;
            }

            /* Make sure we got it all */
            if(tmp_sz >= PATH_MAX) {
                free(symbuf);
                free(ipath);
                ext2_inode_put(inode);
                return -ENAMETOOLONG;
            }

            /* For now, drop any absolute pathnames that we might encounter. At
               some point, I'll probably revisit this decision, but for now
               that's just how it is. */
            if(symbuf[0] == '/') {
                free(symbuf);
                free(ipath);
                ext2_inode_put(inode);
                return -EXDEV;
            }

            /* Tack on the rest of the path to the symbolic link. This is
               horribly inefficient, but it should work fine. */
            while(token) {
                if((tmp_sz += strlen(token) + 1) >= PATH_MAX) {
                    free(symbuf);
                    free(ipath);
                    ext2_inode_put(inode);
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
            ext2_inode_put(inode);
            inode = last;
        }
        else {
            ext2_inode_put(last);
        }
    }

    /* Well, looks like we have it, return the inode. */
    *rv = inode;
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
