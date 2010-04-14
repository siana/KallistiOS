/* KallistiOS ##version##

   netinet/in.h
   Copyright (C) 2006, 2007, 2010 Lawrence Sebald

*/

/** \file   netinet/in.h
    \brief  Definitions for the Internet address family.

    This file contains the standard definitions (as directed by the POSIX 2008
    standard) for internet-related functionality in the AF_INET address family.
    This does not include anything related to AF_INET6 (as IPv6 is not currently
    implemented in KOS), and is not guaranteed to have everything that one might
    have in a fully-standard compliant implementation of the POSIX standard.

    \author Lawrence Sebald
*/

#ifndef __NETINET_IN_H
#define __NETINET_IN_H

#include <sys/cdefs.h>

__BEGIN_DECLS

/* Bring in <inttypes.h> to grab the uint16_t and uint32_t types (for in_port_t
   and in_addr_t below), along with uint8_t. Bring in <sys/socket.h> for the
   sa_family_t type, as required. IEEE Std 1003.1-2008 specifically states that
   <netinet/in.h> can make visible all symbols from both <inttypes.h> and
   <sys/socket.h>. */
#include <inttypes.h>
#include <sys/socket.h>

/** \brief  16-bit type used to store a value for an internet port. */
typedef uint16_t in_port_t;

/** \brief  32-bit value used to store an IPv4 address. */
typedef uint32_t in_addr_t;

/** \brief  Structure used to store an IPv4 address.
    \headerfile netinet/in.h
*/
struct in_addr {
    in_addr_t s_addr;
};

/* Bring in <arpa/inet.h> to make ntohl/ntohs/htonl/htons visible, as per IEEE
   Std 1003.1-2008 (the standard specifically states that <netinet/in.h> may
   make all symbols from <arpa/inet.h> visible. The <arpa/inet.h> header
   actually needs the stuff above, so that's why we include it here. */
#include <arpa/inet.h>

/** \brief  Structure used to store an address for a socket.

    This structure is the standard way to set up addresses for sockets in the
    AF_INET address family. Generally you will not send one of these directly
    to a function, but rather will cast it to a struct sockaddr. Also, this
    structure contains the old sin_zero member which is no longer required by
    the standard (for compatibility with applications that expect it).

    \headerfile netinet/in.h
*/
struct sockaddr_in {
    /** \brief  Family for the socket. Must be AF_INET. */
    sa_family_t    sin_family;

    /** \brief  Port for the socket. Must be in network byte order. */
    in_port_t      sin_port;

    /** \brief  Address for the socket. Must be in network byte order. */
    struct in_addr sin_addr;

    /** \brief  Empty space, ignored for all intents and purposes. */
    unsigned char  sin_zero[8];
};

/** \brief  Local IPv4 host address.

    This address can be used by many things if you prefer to not specify the
    local address, and would rather it be detected automatically.
*/
#define INADDR_ANY       0x00000000

/** \brief  IPv4 broadcast address.

    This address is the normal IPv4 broadcast address (255.255.255.255).
*/
#define INADDR_BROADCAST 0xFFFFFFFF

/** \brief  IPv4 error address.

    This address is non-standard, but is available on many systems. It is used
    to detect failure from some functions that normally return addresses (such
    as the inet_addr function).
*/
#define INADDR_NONE      0xFFFFFFFF

/** \brief  Length of a string form of a maximal IPv4 address. */
#define INET_ADDRSTRLEN 16

/** \brief  Internet Protocol. */
#define IPPROTO_IP      0

/** \brief  Internet Control Message Protocol. */
#define IPPROTO_ICMP    1

/** \brief  Transmission Control Protocol. */
#define IPPROTO_TCP     6

/** \brief  User Datagram Protocol. */
#define IPPROTO_UDP     17

__END_DECLS

#endif /* __NETINET_IN_H */
