/* KallistiOS ##version##

   utils.h
   Copyright (C) 2012 Lawrence Sebald
*/

#ifndef __EXT2_UTILS_H
#define __EXT2_UTILS_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <stdint.h>

/* Added in for my initial debugging. */
#ifdef NOT_IN_KOS
#define DBG_KDEBUG 0
#define dbglog(lvl, ...) printf(__VA_ARGS__)
#endif /* NOT_IN_KOS */

uint32_t ext2_bit_find_nonzero(const uint32_t *btbl, uint32_t start,
                               uint32_t end);
uint32_t ext2_bit_find_zero(const uint32_t *btbl, uint32_t start, uint32_t end);

/* This was a macro, originally. However, that relies on a GCC extension, so I
   made it an inline function instead. */
static inline int ext2_bit_is_set(const uint32_t *btbl, uint32_t bit_num) {
    int byte = (bit_num >> 5);
    int bit = (bit_num & 0x1F);
    return btbl[byte] & (1 << bit);
}

__END_DECLS

#endif /* !__EXT2_UTILS_H */
