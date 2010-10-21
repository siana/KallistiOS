/* KallistiOS ##version##

   kernel/net/net_ipv4.c

   Copyright (C) 2005, 2006, 2007, 2008, 2009, 2010 Lawrence Sebald

   Portions adapted from KOS' old net_icmp.c file:
   Copyright (c) 2002 Dan Potter

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <kos/net.h>
#include "net_ipv4.h"
#include "net_icmp.h"
#include "net_udp.h"

static net_ipv4_stats_t ipv4_stats = { 0 };

/* Perform an IP-style checksum on a block of data */
uint16 net_ipv4_checksum(const uint8 *data, int bytes, uint16 start) {
    uint32 sum = start;
    int i;

    /* Make sure we don't do any unaligned memory accesses */
    if(((uint32)data) & 0x01) {
        for(i = 0; i < bytes; i += 2) {
            sum += (data[i]) | (data[i + 1] << 8);

            if(sum & 0xFFFF0000) {
                sum &= 0xFFFF;
                ++sum;
            }
        }
    }
    else {
        uint16 *ptr = (uint16 *)data;

        for(i = 0; i < (bytes >> 1); ++i) {
            sum += ptr[i];

            if(sum & 0xFFFF0000) {
                sum &= 0xFFFF;
                ++sum;
            }
        }
    }

    /* Handle the last byte, if we have an odd byte count */
    if(bytes & 0x01) {
        sum += data[bytes - 1];

        if(sum & 0xFFFF0000) {
            sum &= 0xFFFF;
            ++sum;
        }
    }

    return sum ^ 0xFFFF;
}

/* Determine if a given IP is in the current network */
static int is_in_network(const uint8 src[4], const uint8 dest[4],
                         const uint8 netmask[4]) {
    int i;

    for(i = 0; i < 4; i++) {
        if((dest[i] & netmask[i]) != (src[i] & netmask[i]))
            return 0;
    }

    return 1;
}

/* Determine if a given IP is the adapter's broadcast address. */
static int is_broadcast(const uint8 dest[4], const uint8 bc[4]) {
    int i;

    for(i = 0; i < 4; ++i) {
        if(dest[i] != bc[i])
            return 0;
    }

    return 1;
}

/* Send a packet on the specified network adapter */
int net_ipv4_send_packet(netif_t *net, ip_hdr_t *hdr, const uint8 *data,
                         int size) {
    uint8 dest_ip[4];
    uint8 dest_mac[6];
    uint8 pkt[size + sizeof(ip_hdr_t) + sizeof(eth_hdr_t)];
    eth_hdr_t *ehdr;
    int err;

    if(net == NULL) {
        net = net_default_dev;
    }

    net_ipv4_parse_address(ntohl(hdr->dest), dest_ip);

    /* Is this a loopback address (127/8)? */
    if((dest_ip[0] & 0xFF) == 0x7F) {
        /* Put the IP header / data into our packet */
        memcpy(pkt, hdr, 4 * (hdr->version_ihl & 0x0f));
        memcpy(pkt + 4 * (hdr->version_ihl & 0x0f), data, size);

        ++ipv4_stats.pkt_sent;
        
        /* Send it "away" */
        net_ipv4_input(NULL, pkt, 4 * (hdr->version_ihl & 0x0f) + size);

        return 0;
    }

    /* Are we sending a broadcast packet? */
    if(hdr->dest == 0xFFFFFFFF || is_broadcast(dest_ip, net->broadcast)) {
        /* Set the destination to the datalink layer broadcast address. */
        memset(dest_mac, 0xFF, 6);
    }
    else {
        /* Is it in our network? */
        if(!is_in_network(net->ip_addr, dest_ip, net->netmask)) {
            memcpy(dest_ip, net->gateway, 4);
        }

        /* Get our destination's MAC address. If we do not have the MAC address
           cached, return a distinguished error to the upper-level protocol so
           that it can decide what to do. */
        err = net_arp_lookup(net, dest_ip, dest_mac, hdr, data, size);
        if(err == -1) {
            errno = ENETUNREACH;
            ++ipv4_stats.pkt_send_failed;
            return -1;
        }
        else if(err == -2) {
            /* It'll send when the ARP reply comes in (assuming one does), so
               return success. */
            return 0;
        }
    }

    /* Fill in the ethernet header */
    ehdr = (eth_hdr_t *)pkt;
    memcpy(ehdr->dest, dest_mac, 6);
    memcpy(ehdr->src, net->mac_addr, 6);
    ehdr->type[0] = 0x08;
    ehdr->type[1] = 0x00;

    /* Put the IP header / data into our ethernet packet */
    memcpy(pkt + sizeof(eth_hdr_t), hdr, 4 * (hdr->version_ihl & 0x0f));
    memcpy(pkt + sizeof(eth_hdr_t) + 4 * (hdr->version_ihl & 0x0f), data,
           size);

    ++ipv4_stats.pkt_sent;

    /* Send it away */
    net->if_tx(net, pkt, sizeof(ip_hdr_t) + size + sizeof(eth_hdr_t),
           NETIF_BLOCK);

    return 0;
}

int net_ipv4_send(netif_t *net, const uint8 *data, int size, int id, int ttl,
                  int proto, uint32 src, uint32 dst) {
    ip_hdr_t hdr;

    /* If the ID is -1, generate a random ID value that can be used in case the
       packet gets fragmented. */
    if(id == -1) {
        id = rand() & 0xFFFF;
    }

    /* Fill in the IPv4 Header */
    hdr.version_ihl = 0x45;
    hdr.tos = 0;
    hdr.length = htons(size + 20);
    hdr.packet_id = id;
    hdr.flags_frag_offs = 0;
    hdr.ttl = ttl;
    hdr.protocol = proto;
    hdr.checksum = 0;
    hdr.src = src;
    hdr.dest = dst;

    hdr.checksum = net_ipv4_checksum((uint8 *)&hdr, sizeof(ip_hdr_t), 0);

    return net_ipv4_frag_send(net, &hdr, data, size);
}

int net_ipv4_input(netif_t *src, const uint8 *pkt, int pktsize) {
    ip_hdr_t *ip;
    int i;
    uint8 *data;
    int hdrlen;

    if(pktsize < sizeof(ip_hdr_t)) {
        /* This is obviously a bad packet, drop it */
        ++ipv4_stats.pkt_recv_bad_size;
        return -1;
    }

    ip = (ip_hdr_t*) pkt;
    hdrlen = (ip->version_ihl & 0x0F) << 2;

    if(pktsize < hdrlen) {
        /* The packet is smaller than the listed header length, bail */
        ++ipv4_stats.pkt_recv_bad_size;
        return -1;
    }

    /* Check ip header checksum */
    i = ip->checksum;
    ip->checksum = 0;
    ip->checksum = net_ipv4_checksum((uint8 *)ip, hdrlen, 0);

    if(i != ip->checksum) {
        /* The checksums don't match, bail */
        ++ipv4_stats.pkt_recv_bad_chksum;
        return -1;
    }

    data = (uint8 *)(pkt + hdrlen);

    /* Submit the packet for possible reassembly. */
    return net_ipv4_reassemble(src, ip, data, ntohs(ip->length) - hdrlen);
}

int net_ipv4_input_proto(netif_t *src, ip_hdr_t *ip, const uint8 *data) {
    int hdrlen = (ip->version_ihl & 0x0F) << 2;

    /* Send the packet along to the appropriate protocol. */
    switch(ip->protocol) {
        case IPPROTO_ICMP:
            ++ipv4_stats.pkt_recv;
            return net_icmp_input(src, ip, data, ntohs(ip->length) - hdrlen);           

        case IPPROTO_UDP:
            ++ipv4_stats.pkt_recv;
            return net_udp_input(src, ip, data, ntohs(ip->length) - hdrlen);
    }

    /* There's no handler for this packet type, send an ICMP Destination
       Unreachable, and log the unknown protocol. */
    ++ipv4_stats.pkt_recv_bad_proto;
    net_icmp_send_dest_unreach(src, ICMP_PROTOCOL_UNREACHABLE, (uint8 *)ip);

    return -1;
}

uint32 net_ipv4_address(const uint8 addr[4]) {
    return (addr[0] << 24) | (addr[1] << 16) | (addr[2] << 8) | (addr[3]);
}

void net_ipv4_parse_address(uint32 addr, uint8 out[4]) {
    out[0] = (uint8) ((addr >> 24) & 0xFF);
    out[1] = (uint8) ((addr >> 16) & 0xFF);
    out[2] = (uint8) ((addr >> 8) & 0xFF);
    out[3] = (uint8) (addr & 0xFF);
}

uint16 net_ipv4_checksum_pseudo(in_addr_t src, in_addr_t dst, uint8 proto,
                                uint16 len) {
    ipv4_pseudo_hdr_t ps = { src, dst, 0, proto, htons(len) };

    return ~net_ipv4_checksum((uint8 *)&ps, sizeof(ipv4_pseudo_hdr_t), 0);
}

net_ipv4_stats_t net_ipv4_get_stats() {
    return ipv4_stats;
}
