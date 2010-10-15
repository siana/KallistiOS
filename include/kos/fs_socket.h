/* KallistiOS ##version##

   kos/fs_socket.h
   Copyright (C) 2006, 2009, 2010 Lawrence Sebald

*/

/** \file   kos/fs_socket.h
    \brief  Definitions for a sockets "filesystem".

    This file provides definitions to support the BSD-sockets-like filesystem
    in KallistiOS. Technically, this filesystem mounts itself on /sock, but it
    doesn't export any files there, so that point is largely irrelevant. The
    filesystem is designed to be extensible, making it possible to add
    additional socket family handlers at runtime. Currently, the kernel only
    implements UDP sockets over IPv4, but as mentioned, this can be extended in
    a fairly straightforward manner. In general, as a user of KallistiOS
    (someone not interested in adding additional socket family drivers), there's
    very little in this file that will be of interest.

    Also, note, that currently there is no way to get input into a network
    protocol added with this functionality only. At some point, I will add
    protocol registration with net_ipv4 for that, however, I haven't had the
    time to do so just yet.

    \author Lawrence Sebald
*/

#ifndef __KOS_FS_SOCKET_H
#define __KOS_FS_SOCKET_H

#include <sys/cdefs.h>

__BEGIN_DECLS

#include <arch/types.h>
#include <kos/limits.h>
#include <kos/fs.h>
#include <sys/queue.h>
#include <sys/socket.h>

struct fs_socket_proto;

/** \brief  Internal representation of a socket for fs_socket.

    This structure is the internal representation of a socket "file" that is
    used within fs_socket. A normal user will never deal with this structure
    directly (only protocol handlers and fs_socket itself ever sees this
    structure directly).

    \headerfile kos/fs_socket.h
*/
typedef struct net_socket {
    /** \cond */
    /* List handle */
    LIST_ENTRY(net_socket) sock_list;
    /** \endcond */

    /** \brief   File handle from the VFS layer. */
    file_t fd;

    /** \brief  The protocol handler for this socket. */
    struct fs_socket_proto *protocol;

    /** \brief  Protocol-specific data. */
    void *data;
} net_socket_t;

/** \brief  Internal sockets protocol handler.

    This structure is a protocol handler used within fs_socket. Each protocol
    that is supported has one of these registered for it within the kernel.
    Generally, users will not come in contact with this structure (unless you're
    planning on writing a protocol handler), and it can generally be ignored.

    For a complete list of appropriate errno values to return from any functions
    that are in here, take a look at the Single Unix Specification (aka, the
    POSIX spec), specifically the page about sys/socket.h (and all the functions
    that it defines, which is available at
    http://www.opengroup.org/onlinepubs/9699919799/basedefs/sys_socket.h.html .

    \headerfile kos/fs_socket.h
*/
typedef struct fs_socket_proto {
    /** \brief  Entry into the global list of protocols.

        Contrary to what Doxygen might think, this is <b>NOT</b> a function.
        This should be initialized with the FS_SOCKET_PROTO_ENTRY macro before
        adding the protocol to the kernel with fs_socket_proto_add().
    */
    TAILQ_ENTRY(fs_socket_proto) entry;

    /** \brief  Domain of support for this protocol handler.

        This field determines which sockets domain this protocol handler
        actually supports. This corresponds with the domain argument of the
        ::socket() function.
    */
    int domain;

    /** \brief  Type of support for this protocol handler.

        This field determines which types of sockets that this protocol handler
        pays attention to. This corresponds with the type argument of the
        ::socket() function.
    */
    int type;

    /** \brief  Protocol of support for this protocol handler.

        This field determines the protocol that this protocol handler actually
        pays attention to. This corresponds with the protocol argument of the
        ::socket() function.
    */
    int protocol;

    /** \brief  Create a new socket for the protocol.

        This function must create a new socket, initializing any data that the
        protocol might need for the socket, based on the parameters passed in.
        The socket passed in is already initialized prior to the handler being
        called, and will be cleaned up by fs_socket if an error is returned from
        the handler (a return value of -1).

        \param  s           The socket structure to initialize
        \param  domain      Domain of the socket
        \param  type        Type of the socket
        \param protocol     Protocol of the socket
        \retval -1          On error (errno should be set appropriately)
        \retval 0           On success
    */
    int (*socket)(net_socket_t *s, int domain, int type, int protocol);

    /** \brief  Close a socket that was created with the protocol.

        This function must do any work required to close a socket and destroy
        it. This function will be called when a socket requests to be closed
        with the close system call. There are no errors defined for this
        function.

        \param  s           The socket to close
    */
    void (*close)(net_socket_t *hnd);

    /** \brief  Set flags on a socket created with the protocol.

        This function will be called when the user calls fs_socket_setflags() on
        a socket created with this protocol. The semantics are the same as
        described in the documentation for that function.

        \param  s           The socket to set flags on
        \param  flags       The flags to set
        \retval -1          On error (set errno appropriately)
        \retval 0           On success
        \see    fs_socket_setflags
    */
    int (*setflags)(net_socket_t *s, int flags);

    /** \brief  Accept a connection on a socket created with the protocol.

        This function should implement the ::accept() system call for the
        protocol. The semantics are exactly as expected for that function.

        \param  s           The socket to accept a connection on
        \param  addr        The address of the incoming connection
        \param  alen        The length of the address
        \return             A newly created socket for the incoming connection
                            or -1 on error (with errno set appropriately)
    */
    int (*accept)(net_socket_t *s, struct sockaddr *addr, socklen_t *alen);

    /** \brief  Bind a socket created with the protocol to an address.

        This function should implement the ::bind() system call for the
        protocol. The semantics are exactly as expected for that function.

        \param  s           The socket to bind to the address
        \param  addr        The address to bind to
        \param  alen        The length of the address
        \retval -1          On error (set errno appropriately)
        \retval 0           On success
    */
    int (*bind)(net_socket_t *s, const struct sockaddr *addr, socklen_t alen);

    /** \brief  Connect a socket created with the protocol to a remote system.

        This function should implement the ::connect() system call for the
        protocol. The semantics are exactly as expected for that function.

        \param  s           The socket to connect with
        \param  addr        The address to connect to
        \param  alen        The length of the address
        \retval -1          On error (with errno set appropriately)
        \retval 0           On success
    */
    int (*connect)(net_socket_t *s, const struct sockaddr *addr,
                   socklen_t alen);

    /** \brief  Listen for incoming connections on a socket created with the
                protocol.

        This function should implement the ::listen() system call for the
        protocol. The semantics are exactly as expected for that function.

        \param  s           The socket to listen on
        \param  backlog     The number of connections to queue
        \retval -1          On error (with errno set appropriately)
        \retval 0           On success
    */
    int (*listen)(net_socket_t *s, int backlog);

    /** \brief  Recieve data on a socket created with the protocol.

        This function should implement the ::recvfrom() system call for the
        protocol. The semantics are exactly as expected for that function. Also,
        this function should implement the ::recv() system call, which will
        call this function with NULL for addr and alen.

        \param  s           The socket to receive data on
        \param  buffer      The buffer to save data in
        \param  len         The length of the buffer
        \param  flags       Flags to the function
        \param  addr        Space to store the address that data came from (NULL
                            if this was called by ::recv())
        \param  alen        Space to store the length of the address (NULL if
                            this was called by ::recv())
        \retval -1          On error (set errno appropriately)
        \retval 0           No outstanding data and the peer has disconnected
                            cleanly
        \retval n           The number of bytes received (may be less than len)
    */
    ssize_t (*recvfrom)(net_socket_t *s, void *buffer, size_t len, int flags,
                        struct sockaddr *addr, socklen_t *alen);

    /** \brief  Send data on a socket created with the protocol.

        This function should implement the ::sendto() system call for the
        protocol. The semantics are exactly as expected for that function. Also,
        this function should implement the ::send() system call, which will
        call this function with NULL for addr and 0 for alen.

        \param  s           The socket to send data on
        \param  msg         The data to send
        \param  len         The length of data to send
        \param  flags       Flags to the function
        \param  addr        The address to send data to (NULL if this was called
                            by ::send())
        \param  alen        The length of the address (0 if this was called by
                            ::send())
        \retval -1          On error (set errno appropriately)
        \retval n           The number of bytes actually sent (may be less than
                            len)
    */
    ssize_t (*sendto)(net_socket_t *s, const void *msg, size_t len, int flags,
                      const struct sockaddr *addr, socklen_t alen);

    /** \brief  Shut down a socket created with the protocol.

        This function should implement the ::shutdown() system call for the
        protocol. The semantics are exactly as expected for that function.

        \param  s           The socket to shut down
        \param  how         What should be shut down on the socket
        \retval -1          On error (set errno appropriately)
        \retval 0           On success
    */
    int (*shutdownsock)(net_socket_t *s, int how);
} fs_socket_proto_t;

/** \brief  Initializer for the entry field in the fs_socket_proto_t struct. */
#define FS_SOCKET_PROTO_ENTRY { NULL, NULL }

/* \cond */
/* Init/shutdown */
int fs_socket_init();
int fs_socket_shutdown();
/* \endcond */

/** \brief  Set flags on a socket file descriptor.

    This function can be used to set various flags on a socket file descriptor,
    similar to what one would use fcntl or ioctl on a normal system for. The
    flags available for use here are largely protocol dependent, and for UDP
    the only flag available is O_NONBLOCK.

    \param  sock        The socket to operate on (returned from a call to the
                        function socket())
    \param  flags       The flags to set on the socket.
    \retval -1          On error, and sets errno as appropriate
    \retval 0           On success

    \par    Error Conditions:
    \em     EWOULDBLOCK - if the function would block while inappropriate to \n
    \em     EBADF - if passed an invalid file descriptor \n
    \em     ENOTSOCK - if passed a file descriptor other than a socket \n
    \em     EINVAL - if an invalid flag was passed in
*/
int fs_socket_setflags(int sock, int flags);

/** \brief  Add a new protocol for use with fs_socket.

    This function registers a protocol handler with fs_socket for use when
    creating and using sockets. This protocol handler must implement all of the
    functions in the fs_socket_proto_t structure. See the code in
    kos/kernel/net/net_udp.c for an example of how to do this.

    This function is NOT safe to call inside an interrupt.

    \param  proto       The new protocol handler to register
    \retval 0           On success (no error conditions are currently defined)
*/
int fs_socket_proto_add(fs_socket_proto_t *proto);

/** \brief  Unregister a protocol from fs_socket.

    This function does the exact opposite of fs_socket_proto_add, and removes
    a protocol from use with fs_socket. It is the programmer's responsibility to
    make sure that no sockets are still around that are registered with the
    protocol to be removed (as they will not work properly once the handler has
    been removed).

    \param  proto       The protocol handler to remove
    \retval -1          On error (This function does not directly change errno)
    \retval 0           On success
*/
int fs_socket_proto_remove(fs_socket_proto_t *proto);

__END_DECLS

#endif	/* __KOS_FS_SOCKET_H */

