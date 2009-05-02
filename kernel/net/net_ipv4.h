/* KallistiOS ##version##

   kernel/net/net_ipv4.h
   Copyright (C) 2005, 2007, 2008 Lawrence Sebald

*/

#ifndef __LOCAL_NET_IPV4_H
#define __LOCAL_NET_IPV4_H

/* These structs are from AndrewK's dcload-ip. */
#define packed __attribute__((packed))
typedef struct {
	uint8	dest[6];
	uint8	src[6];
	uint8	type[2];
} packed eth_hdr_t;

typedef struct {
	uint8	version_ihl;
	uint8	tos;
	uint16	length;
	uint16	packet_id;
	uint16	flags_frag_offs;
	uint8	ttl;
	uint8	protocol;
	uint16	checksum;
	uint32	src;
	uint32	dest;
} packed ip_hdr_t;

typedef struct {
	uint32 src_addr;
	uint32 dst_addr;
	uint8 zero;
	uint8 proto;
	uint16 length;
	uint16 src_port;
	uint16 dst_port;
	uint16 hdrlength;
	uint16 checksum;
	uint8 data[1];
} packed ip_pseudo_hdr_t;
#undef packed

uint16 net_ipv4_checksum(const uint8 *data, int bytes);
int net_ipv4_send_packet(netif_t *net, ip_hdr_t *hdr, const uint8 *data,
                         int size);
int net_ipv4_send(netif_t *net, const uint8 *data, int size, int id, int ttl,
                  int proto, uint32 src, uint32 dst);
int net_ipv4_input(netif_t *src, const uint8 *pkt, int pktsize);

#endif /* __LOCAL_NET_IPV4_H */
