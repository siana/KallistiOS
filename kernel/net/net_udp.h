/* KallistiOS ##version##

   kernel/net/net_udp.h
   Copyright (C) 2005, 2006, 2007, 2008 Lawrence Sebald

*/

#ifndef __LOCAL_NET_UDP_H
#define __LOCAL_NET_UDP_H

#include <sys/socket.h>
#include <kos/fs_socket.h>

int net_udp_input(netif_t *src, ip_hdr_t *ih, const uint8 *data, int size);

#endif /* __LOCAL_NET_UDP_H */
