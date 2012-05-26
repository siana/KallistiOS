/* KallistiOS ##version##

   kernel/net/net_udp.c
   Copyright (C) 2005, 2006, 2007, 2008, 2009, 2012 Lawrence Sebald

*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>
#include <kos/net.h>
#include <kos/mutex.h>
#include <kos/genwait.h>
#include <sys/queue.h>
#include <kos/fs_socket.h>
#include <arch/irq.h>
#include <sys/socket.h>

#include "net_ipv4.h"
#include "net_ipv6.h"

#define packed __attribute__((packed))
typedef struct {
    uint16 src_port    packed;
    uint16 dst_port    packed;
    uint16 length      packed;
    uint16 checksum    packed;
} udp_hdr_t;
#undef packed

struct udp_pkt {
    TAILQ_ENTRY(udp_pkt) pkt_queue;
    struct sockaddr_in6 from;
    uint8 *data;
    uint16 datasize;
};

TAILQ_HEAD(udp_pkt_queue, udp_pkt);

struct udp_sock {
    LIST_ENTRY(udp_sock) sock_list;
    struct sockaddr_in6 local_addr;
    struct sockaddr_in6 remote_addr;

    uint32 flags;
    int domain;

    struct udp_pkt_queue packets;
};

LIST_HEAD(udp_sock_list, udp_sock);

static struct udp_sock_list net_udp_sockets = LIST_HEAD_INITIALIZER(0);
static mutex_t *udp_mutex = NULL;
static net_udp_stats_t udp_stats = { 0 };

static int net_udp_send_raw(netif_t *net, const struct sockaddr_in6 *src,
                            const struct sockaddr_in6 *dst, const uint8 *data,
                            int size, int flags);

static int net_udp_accept(net_socket_t *hnd, struct sockaddr *addr,
                          socklen_t *addr_len) {
    errno = EOPNOTSUPP;
    return -1;
}

static int net_udp_bind(net_socket_t *hnd, const struct sockaddr *addr,
                        socklen_t addr_len) {
    struct udp_sock *udpsock, *iter;
    struct sockaddr_in *realaddr4;
    struct sockaddr_in6 realaddr6;

    /* Verify the parameters sent in first */
    if(addr == NULL) {
        errno = EDESTADDRREQ;
        return -1;
    }

    switch(addr->sa_family) {
        case AF_INET:
            if(addr_len != sizeof(struct sockaddr_in)) {
                errno = EINVAL;
                return -1;
            }

            /* Grab the IPv4 address struct and convert it to IPv6 */
            realaddr4 = (struct sockaddr_in *)addr;
            memset(&realaddr6, 0, sizeof(struct sockaddr_in6));
            realaddr6.sin6_family = AF_INET6;
            realaddr6.sin6_port = realaddr4->sin_port;

            if(realaddr4->sin_addr.s_addr != INADDR_ANY) {
                realaddr6.sin6_addr.__s6_addr.__s6_addr16[5] = 0xFFFF;
                realaddr6.sin6_addr.__s6_addr.__s6_addr32[3] =
                    realaddr4->sin_addr.s_addr;
            }
            else {
                realaddr6.sin6_addr = in6addr_any;
            }
            break;

        case AF_INET6:
            if(addr_len != sizeof(struct sockaddr_in6)) {
                errno = EINVAL;
                return -1;
            }

            realaddr6 = *((struct sockaddr_in6 *)addr);
            break;

        default:
            errno = EAFNOSUPPORT;
            return -1;
    }

    if(irq_inside_int()) {
        if(mutex_trylock(udp_mutex) == -1) {
            errno = EWOULDBLOCK;
            return -1;
        }
    }
    else {
        mutex_lock(udp_mutex);
    }

    udpsock = (struct udp_sock *)hnd->data;
    if(udpsock == NULL) {
        mutex_unlock(udp_mutex);
        errno = EBADF;
        return -1;
    }

    /* Make sure the address family we're binding to matches that which is set
       on the socket itself */
    if(addr->sa_family != udpsock->domain) {
        mutex_unlock(udp_mutex);
        errno = EINVAL;
        return -1;
    }

    /* See if we requested a specific port or not */
    if(realaddr6.sin6_port != 0) {
        /* Make sure we don't already have a socket bound to the
           port specified */
        LIST_FOREACH(iter, &net_udp_sockets, sock_list) {
            if(iter == udpsock)
                continue;

            if(iter->local_addr.sin6_port == realaddr6.sin6_port) {
                mutex_unlock(udp_mutex);
                errno = EADDRINUSE;
                return -1;
            }
        }

        udpsock->local_addr = realaddr6;
    }
    else {
        uint16 port = 1024, tmp = 0;

        /* Grab the first unused port >= 1024. This is, unfortunately, O(n^2) */
        while(tmp != port) {
            tmp = port;

            LIST_FOREACH(iter, &net_udp_sockets, sock_list) {
                if(iter->local_addr.sin6_port == port) {
                    ++port;
                    break;
                }
            }
        }

        udpsock->local_addr = realaddr6;
        udpsock->local_addr.sin6_port = htons(port);
    }

    mutex_unlock(udp_mutex);

    return 0;
}

static int net_udp_connect(net_socket_t *hnd, const struct sockaddr *addr,
                           socklen_t addr_len) {
    struct udp_sock *udpsock;
    struct sockaddr_in *realaddr4;
    struct sockaddr_in6 realaddr6;

    if(addr == NULL) {
        errno = EDESTADDRREQ;
        return -1;
    }

    switch(addr->sa_family) {
        case AF_INET:
            if(addr_len != sizeof(struct sockaddr_in)) {
                errno = EINVAL;
                return -1;
            }

            /* Grab the IPv4 address struct and convert it to IPv6 */
            realaddr4 = (struct sockaddr_in *)addr;

            if(realaddr4->sin_addr.s_addr == INADDR_ANY) {
                errno = EADDRNOTAVAIL;
                return -1;
            }

            memset(&realaddr6, 0, sizeof(struct sockaddr_in6));
            realaddr6.sin6_family = AF_INET6;
            realaddr6.sin6_port = realaddr4->sin_port;
            realaddr6.sin6_addr.__s6_addr.__s6_addr16[5] = 0xFFFF;
            realaddr6.sin6_addr.__s6_addr.__s6_addr32[3] =
                realaddr4->sin_addr.s_addr;
            break;

        case AF_INET6:
            if(addr_len != sizeof(struct sockaddr_in6)) {
                errno = EINVAL;
                return -1;
            }

            realaddr6 = *((struct sockaddr_in6 *)addr);
            break;

        default:
            errno = EAFNOSUPPORT;
            return -1;
    }

    if(irq_inside_int()) {
        if(mutex_trylock(udp_mutex) == -1) {
            errno = EWOULDBLOCK;
            return -1;
        }
    }
    else {
        mutex_lock(udp_mutex);
    }

    udpsock = (struct udp_sock *)hnd->data;
    if(udpsock == NULL) {
        mutex_unlock(udp_mutex);
        errno = EBADF;
        return -1;
    }

    /* Make sure the address family we're binding to matches that which is set
       on the socket itself */
    if(addr->sa_family != udpsock->domain) {
        mutex_unlock(udp_mutex);
        errno = EINVAL;
        return -1;
    }

    /* Make sure the socket isn't already connected */
    if(!IN6_IS_ADDR_UNSPECIFIED(&udpsock->remote_addr.sin6_addr)) {
        mutex_unlock(udp_mutex);
        errno = EISCONN;
        return -1;
    }

    /* Make sure we have a valid address to connect to */
    if(IN6_IS_ADDR_UNSPECIFIED(&realaddr6.sin6_addr) ||
       realaddr6.sin6_port == 0) {
        mutex_unlock(udp_mutex);
        errno = EADDRNOTAVAIL;
        return -1;
    }

    /* "Connect" to the specified address */
    udpsock->remote_addr = realaddr6;

    mutex_unlock(udp_mutex);

    return 0;
}

static int net_udp_listen(net_socket_t *hnd, int backlog) {
    errno = EOPNOTSUPP;
    return -1;
}

static ssize_t net_udp_recvfrom(net_socket_t *hnd, void *buffer, size_t length,
                               int flags, struct sockaddr *addr,
                               socklen_t *addr_len) {
    struct udp_sock *udpsock;
    struct udp_pkt *pkt;

    if(irq_inside_int()) {
        if(mutex_trylock(udp_mutex) == -1) {
            errno = EWOULDBLOCK;
            return -1;
        }
    }
    else {
        mutex_lock(udp_mutex);
    }

    udpsock = (struct udp_sock *)hnd->data;
    if(udpsock == NULL) {
        mutex_unlock(udp_mutex);
        errno = EBADF;
        return -1;
    }

    if(udpsock->flags & (SHUT_RD << 24)) {
        mutex_unlock(udp_mutex);
        return 0;
    }

    if(buffer == NULL || (addr != NULL && addr_len == NULL)) {
        mutex_unlock(udp_mutex);
        errno = EFAULT;
        return -1;
    }

    if(TAILQ_EMPTY(&udpsock->packets) &&
       ((udpsock->flags & FS_SOCKET_NONBLOCK) || irq_inside_int())) {
        mutex_unlock(udp_mutex);
        errno = EWOULDBLOCK;
        return -1;
    }

    while(TAILQ_EMPTY(&udpsock->packets)) {
        mutex_unlock(udp_mutex);
        genwait_wait(udpsock, "net_udp_recvfrom", 0, NULL);
        mutex_lock(udp_mutex);
    }

    pkt = TAILQ_FIRST(&udpsock->packets);

    if(pkt->datasize > length) {
        memcpy(buffer, pkt->data, length);
    }
    else {
        memcpy(buffer, pkt->data, pkt->datasize);
        length = pkt->datasize;
    }

    if(addr != NULL) {
        if(udpsock->domain == AF_INET) {
            struct sockaddr_in realaddr;

            memset(&realaddr, 0, sizeof(struct sockaddr_in));
            realaddr.sin_family = AF_INET;
            realaddr.sin_addr.s_addr =
                pkt->from.sin6_addr.__s6_addr.__s6_addr32[3];
            realaddr.sin_port = pkt->from.sin6_port;

            if(*addr_len < sizeof(struct sockaddr_in)) {
                memcpy(addr, &realaddr, *addr_len);
            }
            else {
                memcpy(addr, &realaddr, sizeof(struct sockaddr_in));
                *addr_len = sizeof(struct sockaddr_in);
            }
        }
        else if(udpsock->domain == AF_INET6) {
            struct sockaddr_in6 realaddr6;

            memset(&realaddr6, 0, sizeof(struct sockaddr_in6));
            realaddr6.sin6_family = AF_INET6;
            realaddr6.sin6_addr = pkt->from.sin6_addr;
            realaddr6.sin6_port = pkt->from.sin6_port;

            if(*addr_len < sizeof(struct sockaddr_in6)) {
                memcpy(addr, &realaddr6, *addr_len);
            }
            else {
                memcpy(addr, &realaddr6, sizeof(struct sockaddr_in6));
                *addr_len = sizeof(struct sockaddr_in6);
            }
        }
    }

    TAILQ_REMOVE(&udpsock->packets, pkt, pkt_queue);
    free(pkt->data);
    free(pkt);

    mutex_unlock(udp_mutex);

    return length;
}

static ssize_t net_udp_sendto(net_socket_t *hnd, const void *message,
                              size_t length, int flags,
                              const struct sockaddr *addr, socklen_t addr_len) {
    struct udp_sock *udpsock;
    struct sockaddr_in *realaddr;
    struct sockaddr_in6 realaddr6;
    uint32_t sflags;
    struct sockaddr_in6 local_addr;

    if(irq_inside_int()) {
        if(mutex_trylock(udp_mutex) == -1) {
            errno = EWOULDBLOCK;
            return -1;
        }
    }
    else {
        mutex_lock(udp_mutex);
    }

    udpsock = (struct udp_sock *)hnd->data;
    if(udpsock == NULL) {
        errno = EBADF;
        goto err;
    }

    if(udpsock->flags & (SHUT_WR << 24)) {
        errno = EPIPE;
        goto err;
    }

    if(!IN6_IS_ADDR_UNSPECIFIED(&udpsock->remote_addr.sin6_addr) &&
       udpsock->remote_addr.sin6_port != 0) {
        if(addr) {
            errno = EISCONN;
            goto err;
        }

        realaddr6 = udpsock->remote_addr;
    }
    else if(addr == NULL) {
        errno = EDESTADDRREQ;
        goto err;
    }
    else if(addr->sa_family != udpsock->domain) {
        errno = EAFNOSUPPORT;
        goto err;
    }
    else if(udpsock->domain == AF_INET6) {
        if(addr_len != sizeof(struct sockaddr_in6)) {
            errno = EINVAL;
            goto err;
        }

        realaddr6 = *((struct sockaddr_in6 *)addr);
    }
    else if(udpsock->domain == AF_INET) {
        if(addr_len != sizeof(struct sockaddr_in)) {
            errno = EINVAL;
            goto err;
        }

        realaddr = (struct sockaddr_in *)addr;
        memset(&realaddr6, 0, sizeof(struct sockaddr_in6));
        realaddr6.sin6_family = AF_INET6;
        realaddr6.sin6_addr.__s6_addr.__s6_addr16[5] = 0xFFFF;
        realaddr6.sin6_addr.__s6_addr.__s6_addr32[3] =
            realaddr->sin_addr.s_addr;
        realaddr6.sin6_port = realaddr->sin_port;
    }
    else {
        /* Shouldn't be able to get here... */
        errno = EBADF;
        goto err;
    }

    if(message == NULL) {
        errno = EFAULT;
        goto err;
    }

    if(udpsock->local_addr.sin6_port == 0) {
        uint16 port = 1024, tmp = 0;
        struct udp_sock *iter;

        /* Grab the first unused port >= 1024. This is, unfortunately, O(n^2) */
        while(tmp != port) {
            tmp = port;

            LIST_FOREACH(iter, &net_udp_sockets, sock_list) {
                if(iter->local_addr.sin6_port == port) {
                    ++port;
                    break;
                }
            }
        }

        udpsock->local_addr.sin6_port = htons(port);
    }

    local_addr = udpsock->local_addr;
    sflags = udpsock->flags;
    mutex_unlock(udp_mutex);

    return net_udp_send_raw(NULL, &local_addr, &realaddr6,
                            (const uint8 *)message, length, sflags);
err:
    mutex_unlock(udp_mutex);
    return -1;
}

static int net_udp_shutdownsock(net_socket_t *hnd, int how) {
    struct udp_sock *udpsock;

    if(irq_inside_int()) {
        if(mutex_trylock(udp_mutex) == -1) {
            errno = EWOULDBLOCK;
            return -1;
        }
    }
    else {
        mutex_lock(udp_mutex);
    }

    udpsock = (struct udp_sock *)hnd->data;
    if(udpsock == NULL) {
        mutex_unlock(udp_mutex);
        errno = EBADF;
        return -1;
    }

    if(how & 0xFFFFFFFC) {
        mutex_unlock(udp_mutex);
        errno = EINVAL;
        return -1;
    }

    udpsock->flags |= (how << 24);

    mutex_unlock(udp_mutex);

    return 0;
}

static int net_udp_socket(net_socket_t *hnd, int domain, int type, int proto) {
    struct udp_sock *udpsock;

    udpsock = (struct udp_sock *)malloc(sizeof(struct udp_sock));
    if(udpsock == NULL) {
        errno = ENOMEM;
        return -1;
    }

    memset(udpsock, 0, sizeof(struct udp_sock));
    TAILQ_INIT(&udpsock->packets);
    udpsock->domain = domain;

    if(irq_inside_int()) {
        if(mutex_trylock(udp_mutex) == -1) {
            free(udpsock);
            errno = EWOULDBLOCK;
            return -1;
        }
    }
    else {
        mutex_lock(udp_mutex);
    }

    LIST_INSERT_HEAD(&net_udp_sockets, udpsock, sock_list);
    hnd->data = udpsock;
    mutex_unlock(udp_mutex);

    return 0;
}

static void net_udp_close(net_socket_t *hnd) {
    struct udp_sock *udpsock;
    struct udp_pkt *pkt;

    if(irq_inside_int()) {
        if(mutex_trylock(udp_mutex) == -1) {
            errno = EWOULDBLOCK;
            return;
        }
    }
    else {
        mutex_lock(udp_mutex);
    }

    udpsock = (struct udp_sock *)hnd->data;
    if(udpsock == NULL) {
        mutex_unlock(udp_mutex);
        errno = EBADF;
        return;
    }

    TAILQ_FOREACH(pkt, &udpsock->packets, pkt_queue) {
        free(pkt->data);
        TAILQ_REMOVE(&udpsock->packets, pkt, pkt_queue);
        free(pkt);
    }

    LIST_REMOVE(udpsock, sock_list);

    free(udpsock);
    mutex_unlock(udp_mutex);
}

static int net_udp_setflags(net_socket_t *hnd, uint32_t flags) {
    struct udp_sock *udpsock;

    if(irq_inside_int()) {
        if(mutex_trylock(udp_mutex) == -1) {
            errno = EWOULDBLOCK;
            return -1;
        }
    }
    else {
        mutex_lock(udp_mutex);
    }

    udpsock = (struct udp_sock *) hnd->data;
    if(udpsock == NULL) {
        mutex_unlock(udp_mutex);
        errno = EBADF;
        return -1;
    }

    if(flags & (~FS_SOCKET_NONBLOCK)) {
        mutex_unlock(udp_mutex);
        errno = EINVAL;
        return -1;
    }

    udpsock->flags |= flags;
    mutex_unlock(udp_mutex);

    return 0;
}

static int net_udp_input4(netif_t *src, const ip_hdr_t *ip, const uint8 *data,
                          int size) {
    udp_hdr_t *hdr = (udp_hdr_t *)data;
    uint16 cs;
    struct udp_sock *sock;
    struct udp_pkt *pkt;

    if(!udp_mutex)
        return -1;

    if(size <= sizeof(udp_hdr_t)) {
        /* Discard the packet, since it is too short to be of any interest. */
        ++udp_stats.pkt_recv_bad_size;
        return -1;
    }

    if(hdr->checksum != 0) {
        cs = net_ipv4_checksum_pseudo(ip->src, ip->dest, ip->protocol, size);

        /* If the checksum is right, we'll get zero back from the checksum
           function */
        if(net_ipv4_checksum(data, size, cs)) {
            /* The checksum was wrong, bail out */
            ++udp_stats.pkt_recv_bad_chksum;
            return -1;
        }
    }

    if(mutex_trylock(udp_mutex))
        /* Considering this function is usually called in an IRQ, if the
           mutex is locked, there isn't much that can be done. */
        return -1;

    LIST_FOREACH(sock, &net_udp_sockets, sock_list) {
        /* Don't even bother looking at IPv6-only sockets */
        if(sock->domain == AF_INET6 && (sock->flags & FS_SOCKET_V6ONLY))
            continue;

        /* If the ports don't match, don't look any further */
        if(sock->local_addr.sin6_port != hdr->dst_port)
            continue;

        /* If the socket has a remote port set and it isn't the one that this
           packet came from, bail */
        if(sock->remote_addr.sin6_port != 0 &&
           sock->remote_addr.sin6_port != hdr->src_port)
            continue;

        /* If we have a address specified, and its not v4-mapped or its not the
           address this packet came from, bail out */
        if(!IN6_IS_ADDR_UNSPECIFIED(&sock->remote_addr.sin6_addr) &&
           (!IN6_IS_ADDR_V4MAPPED(&sock->remote_addr.sin6_addr) ||
            sock->remote_addr.sin6_addr.__s6_addr.__s6_addr32[3] != ip->src))
            continue;

        if(!(pkt = (struct udp_pkt *)malloc(sizeof(struct udp_pkt)))) {
            mutex_unlock(udp_mutex);
            return -1;
        }

        memset(pkt, 0, sizeof(struct udp_pkt));

        pkt->datasize = size - sizeof(udp_hdr_t);
        if(!(pkt->data = (uint8 *)malloc(pkt->datasize))) {
            free(pkt);
            mutex_unlock(udp_mutex);
            return -1;
        }

        pkt->from.sin6_family = AF_INET6;
        pkt->from.sin6_addr.__s6_addr.__s6_addr16[5] = 0xFFFF;
        pkt->from.sin6_addr.__s6_addr.__s6_addr32[3] = ip->src;
        pkt->from.sin6_port = hdr->src_port;

        memcpy(pkt->data, data + sizeof(udp_hdr_t), pkt->datasize);

        TAILQ_INSERT_TAIL(&sock->packets, pkt, pkt_queue);

        ++udp_stats.pkt_recv;
        genwait_wake_one(sock);
        mutex_unlock(udp_mutex);

        return 0;
    }

    ++udp_stats.pkt_recv_no_sock;
    mutex_unlock(udp_mutex);

    return -1;
}

static int net_udp_input6(netif_t *src, const ipv6_hdr_t *ip, const uint8 *data,
                          int size) {
    udp_hdr_t *hdr = (udp_hdr_t *)data;
    uint16 cs;
    struct udp_sock *sock;
    struct udp_pkt *pkt;

    if(!udp_mutex)
        return -1;

    if(size <= sizeof(udp_hdr_t)) {
        /* Discard the packet, since it is too short to be of any interest. */
        ++udp_stats.pkt_recv_bad_size;
        return -1;
    }

    if(hdr->checksum != 0) {
        cs = net_ipv6_checksum_pseudo(&ip->src_addr, &ip->dst_addr,
                                      IPPROTO_UDP, size);

        /* If the checksum is right, we'll get zero back from the checksum
           function */
        if(net_ipv4_checksum(data, size, cs)) {
            /* The checksum was wrong, bail out */
            ++udp_stats.pkt_recv_bad_chksum;
            return -1;
        }
    }

    if(mutex_trylock(udp_mutex))
    /* Considering this function is usually called in an IRQ, if the
       mutex is locked, there isn't much that can be done. */
        return -1;

    LIST_FOREACH(sock, &net_udp_sockets, sock_list) {
        /* Don't even bother looking at IPv4 sockets */
        if(sock->domain == AF_INET)
            continue;

        /* If the ports don't match, don't look any further */
        if(sock->local_addr.sin6_port != hdr->dst_port)
            continue;

        /* If the socket has a remote port set and it isn't the one that this
           packet came from, bail */
        if(sock->remote_addr.sin6_port != 0 &&
           sock->remote_addr.sin6_port != hdr->src_port)
            continue;

        /* If we have a address specified and it is not the address this packet
           came from, bail out */
        if(!IN6_IS_ADDR_UNSPECIFIED(&sock->remote_addr.sin6_addr) &&
           memcmp(&sock->remote_addr.sin6_addr, &ip->src_addr,
                  sizeof(struct in6_addr)))
            continue;

        if(!(pkt = (struct udp_pkt *)malloc(sizeof(struct udp_pkt)))) {
            mutex_unlock(udp_mutex);
            return -1;
        }

        memset(pkt, 0, sizeof(struct udp_pkt));

        pkt->datasize = size - sizeof(udp_hdr_t);
        if(!(pkt->data = (uint8 *)malloc(pkt->datasize))) {
            free(pkt);
            mutex_unlock(udp_mutex);
            return -1;
        }

        pkt->from.sin6_family = AF_INET6;
        pkt->from.sin6_addr = ip->src_addr;
        pkt->from.sin6_port = hdr->src_port;

        memcpy(pkt->data, data + sizeof(udp_hdr_t), pkt->datasize);

        TAILQ_INSERT_TAIL(&sock->packets, pkt, pkt_queue);

        ++udp_stats.pkt_recv;
        genwait_wake_one(sock);
        mutex_unlock(udp_mutex);

        return 0;
    }

    ++udp_stats.pkt_recv_no_sock;
    mutex_unlock(udp_mutex);

    return -1;
}

static int net_udp_input(netif_t *src, int domain, const void *hdr,
                         const uint8 *data, int size) {
    if(!udp_mutex)
        return -1;

    switch(domain) {
        case AF_INET:
            return net_udp_input4(src, (const ip_hdr_t *)hdr, data, size);

        case AF_INET6:
            return net_udp_input6(src, (const ipv6_hdr_t *)hdr, data, size);
    }

    return -1;
}

static int net_udp_send_raw(netif_t *net, const struct sockaddr_in6 *src,
                            const struct sockaddr_in6 *dst, const uint8 *data,
                            int size, int flags) {
    uint8 buf[size + sizeof(udp_hdr_t)];
    udp_hdr_t *hdr = (udp_hdr_t *)buf;
    uint16 cs;
    int err;
    struct in6_addr srcaddr = src->sin6_addr;

    if(!net) {
        net = net_default_dev;

        if(!net) {
            errno = ENETDOWN;
            ++udp_stats.pkt_send_failed;
            return -1;
        }
    }

    if(IN6_IS_ADDR_UNSPECIFIED(&src->sin6_addr)) {
        if(IN6_IS_ADDR_V4MAPPED(&dst->sin6_addr)) {
            srcaddr.__s6_addr.__s6_addr16[5] = 0xFFFF;
            srcaddr.__s6_addr.__s6_addr32[3] =
                htonl(net_ipv4_address(net->ip_addr));

            if(srcaddr.__s6_addr.__s6_addr32[3] == INADDR_ANY) {
                errno = ENETDOWN;
                ++udp_stats.pkt_send_failed;
                return -1;
            }
        }
        else {
            if(IN6_IS_ADDR_LOOPBACK(&dst->sin6_addr)) {
                srcaddr = in6addr_loopback;
            }
            else if(IN6_IS_ADDR_LINKLOCAL(&dst->sin6_addr) ||
                    IN6_IS_ADDR_MC_LINKLOCAL(&dst->sin6_addr)) {
                srcaddr = net->ip6_lladdr;
            }
            else if(net->ip6_addr_count) {
                /* Punt and pick the first non-link-local address */
                srcaddr = net->ip6_addrs[0];
            }
            else {
                errno = ENETDOWN;
                ++udp_stats.pkt_send_failed;
                return -1;
            }
        }
    }

    memcpy(buf + sizeof(udp_hdr_t), data, size);
    size += sizeof(udp_hdr_t);
    cs = net_ipv6_checksum_pseudo(&srcaddr, &dst->sin6_addr, IPPROTO_UDP, size);

    hdr->src_port = src->sin6_port;
    hdr->dst_port = dst->sin6_port;
    hdr->length = htons(size);
    hdr->checksum = 0;
    hdr->checksum = net_ipv4_checksum(buf, size, cs);

    /* Pass everything off to the network layer to do the rest. */
    err = net_ipv6_send(net, buf, size, 0, IPPROTO_UDP, &srcaddr,
                        &dst->sin6_addr);
    if(err < 0) {
        ++udp_stats.pkt_send_failed;
        return -1;
    }
    else {
        ++udp_stats.pkt_sent;
        return size - sizeof(udp_hdr_t);
    }
}

net_udp_stats_t net_udp_get_stats() {
    return udp_stats;
}

/* Protocol handler for fs_socket. */
static fs_socket_proto_t proto = {
    FS_SOCKET_PROTO_ENTRY,
    PF_INET6,                           /* domain */
    SOCK_DGRAM,                         /* type */
    IPPROTO_UDP,                        /* protocol */
    net_udp_socket,
    net_udp_close,
    net_udp_setflags,
    net_udp_accept,
    net_udp_bind,
    net_udp_connect,
    net_udp_listen,
    net_udp_recvfrom,
    net_udp_sendto,
    net_udp_shutdownsock,
    net_udp_input
};

int net_udp_init() {
    udp_mutex = mutex_create();

    if(udp_mutex == NULL) {
        return -1;
    }

    return fs_socket_proto_add(&proto);
}

void net_udp_shutdown() {
    fs_socket_proto_remove(&proto);

    if(udp_mutex != NULL)
        mutex_destroy(udp_mutex);
}
