/* KallistiOS ##version##

   sys/socket.h
   Copyright (C)2006, 2010 Lawrence Sebald

*/

/** \file   sys/socket.h
    \brief  Main sockets header.

    This file contains the standard definitions (as directed by the POSIX 2008
    standard) for socket-related functionality in the AF_INET address family.
    This does not include anything related to AF_INET6 (as IPv6 is not currently
    implemented in KOS) nor UNIX domain sockets, and is not guaranteed to have
    everything that one might have in a fully-standard compliant implementation
    of the POSIX standard.

    \author Lawrence Sebald
*/

#ifndef __SYS_SOCKET_H
#define __SYS_SOCKET_H

#include <sys/cdefs.h>
#include <sys/types.h>

__BEGIN_DECLS

/** \brief  Socket length type. */
typedef __uint32_t socklen_t;

/** \brief  Socket address family type. */
typedef __uint8_t sa_family_t;

/** \brief  Socket address structure.
    \headerfile sys/socket.h
*/
struct sockaddr {
    /** \brief  Address family. */
    sa_family_t sa_family;
    /** \brief  Address data. */
    char        sa_data[];
};

/** \brief  Datagram socket type.

    This socket type specifies that the socket in question transmits datagrams
    that may or may not be reliably transmitted. With IPv4, this implies using
    UDP as the underlying protocol.
*/
#define SOCK_DGRAM 1

/** \brief  Internet domain sockets for use with IPv4 addresses. */
#define AF_INET     1

/** \brief  Internet domain sockets for use with IPv6 addresses.

    Note, these are NOT supported just yet.
*/
#define AF_INET6    2

/** \brief  Protocol family for Internet domain sockets (IPv4). */
#define PF_INET     AF_INET

/** \brief  Protocol family for Internet domain sockets (IPv6). */
#define PF_INET6    AF_INET6

/** \brief  Disable furhter receive operations. */
#define SHUT_RD   0x00000001

/** \brief  Disable further send operations. */
#define SHUT_WR   0x00000002

/** \brief  Disable further send and receive operations. */
#define SHUT_RDWR (SHUT_RD | SHUT_WR)

/** \brief  Accept a new connection on a socket.

    This function extracts the first connection on the queue of connections of
    the specified socket, creating a new socket with the same protocol and
    address family as that socket for communication with the extracted
    connection.

    \param  socket      A socket created with socket() that has been bound to an
                        address with bind() and is listening for connections
                        after a call to listen().
    \param  address     A pointer to a sockaddr structure where the address of
                        the connecting socket will be returned (can be NULL).
    \param  address_len A pointer to a socklen_t which specifies the amount of
                        space in address on input, and the amount used of the
                        space on output.
    \return             On success, the non-negative file descriptor of the
                        new connection, otherwise -1 and errno will be set to
                        the appropriate error value.
*/
int accept(int socket, struct sockaddr *address, socklen_t *address_len);

/** \brief  Bind a name to a socket.

    This function assigns the socket to a unique name (address).

    \param  socket      A socket that is to be bound.
    \param  address     A pointer to a sockaddr structure where the name to be
                        assigned to the socket resides.
    \param  address_len The length of the address structure.
    \retval 0           On success.
    \retval -1          On error, sets errno as appropriate.
*/
int bind(int socket, const struct sockaddr *address, socklen_t address_len);

/** \brief  Connect a socket.

    This function attempts to make a connection to a resource on a connection-
    mode socket, or sets/resets the peer address on a connectionless one.

    \param  socket      A socket that is to be connected.
    \param  address     A pointer to a sockaddr structure where the name of the
                        peer resides.
    \param  address_len The length of the address structure.
    \retval 0           On success.
    \retval -1          On error, sets errno as appropriate.
*/
int connect(int socket, const struct sockaddr *address, socklen_t address_len);

/** \brief  Listen for socket connections and set the queue length.

    This function marks a connection-mode socket for incoming connections.

    \param  socket      A connection-mode socket to listen on.
    \param  backlog     The number of queue entries.
    \retval 0           On success.
    \retval -1          On error, sets errno as appropriate.
*/
int listen(int socket, int backlog);

/** \brief  Receive a message on a connected socket.

    This function receives messages from the peer on a connected socket.

    \param  socket      The socket to receive on.
    \param  buffer      A pointer to a buffer to store the message in.
    \param  length      The length of the buffer.
    \param  flags       The type of message reception. Set to 0 for now.
    \return             On success, the length of the message in bytes. If no
                        messages are available, and the socket has been shut
                        down, 0. On error, -1, and sets errno as appropriate.
*/
ssize_t recv(int socket, void *buffer, size_t length, int flags);

/** \brief  Receive a message on a socket.

    This function receives messages from a peer on a (usually connectionless)
    socket.

    \param  socket      The socket to receive on.
    \param  buffer      A pointer to a buffer to store the message in.
    \param  length      The length of the buffer.
    \param  flags       The type of message reception. Set to 0 for now.
    \param  address     A pointer to a sockaddr structure to store the peer's
                        name in.
    \param  address_len A pointer to the length of the address structure on
                        input, the number of bytes used on output.
    \return             On success, the length of the message in bytes. If no
                        messages are available, and the socket has been shut
                        down, 0. On error, -1, and sets errno as appropriate.
*/
ssize_t recvfrom(int socket, void *buffer, size_t length, int flags,
                 struct sockaddr *address, socklen_t *address_len);

/** \brief  Send a message on a connected socket.

    This function sends messages to the peer on a connected socket.

    \param  socket      The socket to send on.
    \param  message     A pointer to a buffer with the message to send.
    \param  length      The length of the message.
    \param  flags       The type of message transmission. Set to 0 for now.
    \return             On success, the number of bytes sent. On error, -1,
                        and sets errno as appropriate.
*/
ssize_t send(int socket, const void *message, size_t length, int flags);

/** \brief  Send a message on a socket.

    This function sends messages to the peer on a (usually connectionless)
    socket. If used on a connection-mode socket, this function may change the
    peer that the socket is connected to, or it may simply return error.

    \param  socket      The socket to send on.
    \param  message     A pointer to a buffer with the message to send.
    \param  length      The length of the message.
    \param  flags       The type of message transmission. Set to 0 for now.
    \param  dest_addr   A pointer to a sockaddr structure with the peer's name.
    \param  dest_len    The length of dest_addr, in bytes.
    \return             On success, the number of bytes sent. On error, -1,
                        and sets errno as appropriate.
*/
ssize_t sendto(int socket, const void *message, size_t length, int flags,
               const struct sockaddr *dest_addr, socklen_t dest_len);

/** \brief  Shutdown socket send and receive operations.

    This function closes a specific socket for the set of specified operations.

    \param  socket      The socket to shutdown.
    \param  how         The type of shutdown.
    \retval 0           On success.
    \retval -1          On error, sets errno as appropriate.
    \see                SHUT_RD
    \see                SHUT_WR
    \see                SHUT_RDWR
*/
int shutdown(int socket, int how);

/** \brief  Create an endpoint for communications.

    This function creates an unbound socket for communications with the
    specified parameters.

    \param  domain      The domain to create the socket in (i.e, AF_INET).
    \param  type        The type of socket to be created (i.e, SOCK_DGRAM).
    \param  protocol    The protocol to use with the socket. May be 0 to allow
                        a default to be used.
    \return             A non-negative file descriptor on success. -1 on error,
                        and sets errno as appropriate.
*/
int socket(int domain, int type, int protocol);

__END_DECLS

#endif /* __SYS_SOCKET_H */
