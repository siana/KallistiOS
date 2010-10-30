/* KallistiOS ##version##

   kos/md5.h
   Copyright (C) 2010 Lawrence Sebald
*/

#ifndef __KOS_MD5_H
#define __KOS_MD5_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <arch/types.h>

typedef struct kos_md5_cxt {
    uint64 size;
    uint32 hash[4];
    uint8  buf[64];
} kos_md5_cxt_t;

void kos_md5_start(kos_md5_cxt_t *cxt);
void kos_md5_hash_block(kos_md5_cxt_t *cxt, const uint8 *input, uint32 size);
void kos_md5_finish(kos_md5_cxt_t *cxt, uint8 output[16]);

__END_DECLS

#endif /* !__KOS_MD5_H */
