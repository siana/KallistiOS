/* KallistiOS ##version##

   kernel/net/net_ipv4_frag.c
   Copyright (C) 2009 Lawrence Sebald

*/

#include <string.h>
#include <errno.h>
#include <arpa/inet.h>

#include <kos/net.h>

#include "net_ipv4.h"

int net_ipv4_frag_send(netif_t *net, ip_hdr_t *hdr, const uint8 *data,
                       int size) {
    int ihl = (hdr->version_ihl & 0x0f) << 2;
    int total = size + ihl;
    uint16 flags = ntohs(hdr->flags_frag_offs);
    ip_hdr_t newhdr;
    int nfb, ds;

    if(net == NULL)
        net = net_default_dev;

    /* If the packet doesn't need to be fragmented, send it away as is. */
    if(total < net->mtu) {
        return net_ipv4_send_packet(net, hdr, data, size);
    }
    /* If it needs to be fragmented and the DF flag is set, return error. */
    else if(flags & 0x4000) {
        errno = EMSGSIZE;
        return -1;
    }

    /* Copy over the old header, and set up things for fragment processing. */
    memcpy(&newhdr, hdr, ihl);
    nfb = ((net->mtu - ihl) >> 3);
    ds = nfb << 3;
    newhdr.flags_frag_offs = htons(flags | 0x2000);
    newhdr.length = htons(ihl + (nfb << 3));

    /* Recompute the checksum. */
    newhdr.checksum = 0;
    newhdr.checksum = net_ipv4_checksum((uint8 *)&newhdr, sizeof(ip_hdr_t));

    if(net_ipv4_send_packet(net, &newhdr, data, ds)) {
        return -1;
    }

    /* We don't deal with options right now, so dealing with the rest of the
       fragments is pretty easy. Fix the header, and recursively call this
       function to finish things off. */
    hdr->length = htons(ihl + size);
    hdr->flags_frag_offs = htons((flags & 0xE000) | ((flags & 0x1FFF) + nfb));
    hdr->checksum = 0;
    hdr->checksum = net_ipv4_checksum((uint8 *)hdr, sizeof(ip_hdr_t));

    return net_ipv4_frag_send(net, hdr, data + ds, size - ds);
}

int net_ipv4_reassemble(netif_t *src, ip_hdr_t *hdr, const uint8 *data,
                        int size) {
    uint16 flags = ntohs(hdr->flags_frag_offs);

    /* If the fragment offset is zero and the MF flag is 0, this is the whole
       packet. Treat it as such. */
    if(!(flags & 0x2000) && (flags & 0x1FFF) == 0) {
        return net_ipv4_input_proto(src, hdr, data);
    }

    return -1;
}
