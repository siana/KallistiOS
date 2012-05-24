/* KallistiOS ##version##

   kernel/net/net_tcp.c
   Copyright (C) 2012 Lawrence Sebald

*/

#include <errno.h>
#include <stdint.h>
#include <sys/socket.h>

#include <kos/net.h>
#include <kos/fs_socket.h>

#include "net_ipv4.h"
#include "net_ipv6.h"

typedef struct tcp_hdr {
    uint16_t src_port;
    uint16_t dst_port;
    uint32_t seq;
    uint32_t ack;
    uint16_t off_flags;
    uint16_t wnd;
    uint16_t checksum;
    uint16_t urg;
} __attribute__((packed)) tcp_hdr_t;

/* Flags that can be set in the off_flags field of the above struct */
#define TCP_FLAG_FIN    0x01
#define TCP_FLAG_SYN    0x02
#define TCP_FLAG_RST    0x04
#define TCP_FLAG_PSH    0x08
#define TCP_FLAG_ACK    0x10
#define TCP_FLAG_URG    0x20

#define TCP_GET_OFFSET(x)   (((x) & 0xF000) >> 10)
#define TCP_OFFSET(y)       (((y) & 0x0F) << 12)

/* Sockets interface. These are all stubs at the moment. */
static int net_tcp_socket(net_socket_t *hnd, int domain, int type, int proto) {
    errno = EPROTONOSUPPORT;
    return -1;
}

static void net_tcp_close(net_socket_t *hnd) {
    errno = EBADF;
}

static int net_tcp_setflags(net_socket_t *hnd, uint32_t flags) {
    errno = EBADF;
    return -1;
}

static int net_tcp_accept(net_socket_t *hnd, struct sockaddr *addr,
                          socklen_t *addr_len) {
    errno = EBADF;
    return -1;
}

static int net_tcp_bind(net_socket_t *hnd, const struct sockaddr *addr,
                        socklen_t addr_len) {
    errno = EBADF;
    return -1;
}

static int net_tcp_connect(net_socket_t *hnd, const struct sockaddr *addr,
                           socklen_t addr_len) {
    errno = EBADF;
    return -1;
}

static int net_tcp_listen(net_socket_t *hnd, int backlog) {
    errno = EBADF;
    return -1;
}

static ssize_t net_tcp_recvfrom(net_socket_t *hnd, void *buffer, size_t length,
                                int flags, struct sockaddr *addr,
                                socklen_t *addr_len) {
    errno = EBADF;
    return -1;
}

static ssize_t net_tcp_sendto(net_socket_t *hnd, const void *message,
                              size_t length, int flags,
                              const struct sockaddr *addr, socklen_t addr_len) {
    errno = EBADF;
    return -1;
}

static int net_tcp_shutdownsock(net_socket_t *hnd, int how) {
    errno = EBADF;
    return -1;
}

static void tcp_send_rst(netif_t *net, const struct in6_addr *src,
                         const struct in6_addr *dst, const tcp_hdr_t *ohdr,
                         int size) {
    tcp_hdr_t pkt;
    uint16 cs;
    uint16 flags = ntohs(ohdr->off_flags);

    /* Fill in the packet */
    pkt.src_port = ohdr->dst_port;
    pkt.dst_port = ohdr->src_port;

    if(flags & TCP_FLAG_SYN) {
        size += 1;
    }

    if(flags & TCP_FLAG_ACK) {
        pkt.seq = ohdr->ack;
        pkt.ack = 0;
        pkt.off_flags = TCP_FLAG_RST;
    }
    else {
        pkt.seq = 0;
        pkt.ack = htonl(ntohl(ohdr->seq) + size);
        pkt.off_flags = TCP_FLAG_ACK | TCP_FLAG_RST;
    }

    pkt.off_flags = htons(pkt.off_flags | TCP_OFFSET(5));
    pkt.wnd = 0;
    pkt.checksum = 0;
    pkt.urg = 0;

    cs = net_ipv6_checksum_pseudo(dst, src, IPPROTO_TCP, sizeof(tcp_hdr_t));
    pkt.checksum = net_ipv4_checksum((const uint8 *)&pkt, sizeof(tcp_hdr_t),
                                     cs);

    net_ipv6_send(net, (const uint8 *)&pkt, sizeof(tcp_hdr_t), 0, IPPROTO_TCP,
                  dst, src);
}

static int net_tcp_input(netif_t *src, int domain, const void *hdr,
                         const uint8 *data, int size) {
    struct in6_addr srca, dsta;
    const ip_hdr_t *ip4;
    const ipv6_hdr_t *ip6;
    const tcp_hdr_t *tcp;
    uint16 flags;

    switch(domain) {
        case AF_INET:
            ip4 = (const ip_hdr_t *)hdr;
            srca.__s6_addr.__s6_addr32[0] = srca.__s6_addr.__s6_addr32[1] = 0;
            srca.__s6_addr.__s6_addr16[4] = 0;
            srca.__s6_addr.__s6_addr16[5] = 0xFFFF;
            srca.__s6_addr.__s6_addr32[3] = ip4->src;
            dsta.__s6_addr.__s6_addr32[0] = dsta.__s6_addr.__s6_addr32[1] = 0;
            dsta.__s6_addr.__s6_addr16[4] = 0;
            dsta.__s6_addr.__s6_addr16[5] = 0xFFFF;
            dsta.__s6_addr.__s6_addr32[3] = ip4->dest;
            break;

        case AF_INET6:
            ip6 = (const ipv6_hdr_t *)hdr;
            srca = ip6->src_addr;
            dsta = ip6->dst_addr;
            break;
    }

    /* Send a RST, since we don't actually support TCP just yet... */
    tcp = (const tcp_hdr_t *)data;
    flags = ntohs(tcp->off_flags);

    if(!(flags & TCP_FLAG_RST)) {
        tcp_send_rst(src, &srca, &dsta, tcp, size - TCP_GET_OFFSET(flags));
    }

    return 0;
}

/* Protocol handler for fs_socket. */
static fs_socket_proto_t proto = {
    FS_SOCKET_PROTO_ENTRY,
    PF_INET6,                           /* domain */
    SOCK_STREAM,                        /* type */
    IPPROTO_TCP,                        /* protocol */
    net_tcp_socket,                     /* socket */
    net_tcp_close,                      /* close */
    net_tcp_setflags,                   /* setflags */
    net_tcp_accept,                     /* accept */
    net_tcp_bind,                       /* bind */
    net_tcp_connect,                    /* connect */
    net_tcp_listen,                     /* listen */
    net_tcp_recvfrom,                   /* recvfrom */
    net_tcp_sendto,                     /* sendto */
    net_tcp_shutdownsock,               /* shutdown */
    net_tcp_input                       /* input */
};

int net_tcp_init() {
    return fs_socket_proto_add(&proto);
}

void net_tcp_shutdown() {
    fs_socket_proto_remove(&proto);
}
