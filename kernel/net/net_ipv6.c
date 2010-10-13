/* KallistiOS ##version##

   kernel/net/net_ipv6.c
   Copyright (C) 2010 Lawrence Sebald

*/

#include <netinet/in.h>

/* Note, this doesn't actually implement IPv6 just yet. Its just a skeleton to
   deal with a few little things */
const struct in6_addr in6addr_any = IN6ADDR_ANY_INIT;
const struct in6_addr in6addr_loopback = IN6ADDR_LOOPBACK_INIT;
