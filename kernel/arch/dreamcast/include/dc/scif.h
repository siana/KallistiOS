/* KallistiOS ##version##

   dc/scif.h
   Copyright (C) 2000,2001,2004 Dan Potter

*/

/** \file   dc/scif.h
    \brief  Serial port functionality.

    This file deals with raw access to the serial port on the Dreamcast.

    \author Dan Potter
*/

#ifndef __DC_SCIF_H
#define __DC_SCIF_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <arch/types.h>
#include <kos/dbgio.h>

/** \brief  Set serial parameters.
    \param  baud            The bitrate to set.
    \param  fifo            1 to enable FIFO mode.
*/
void scif_set_parameters(int baud, int fifo);

// The rest of these are the standard dbgio interface.

/** \brief  Enable or disable SCIF IRQ usage.
    \param  on              1 to enable IRQ usage, 0 for polled I/O.
    \retval 0               On success (no error conditions defined).
*/
int scif_set_irq_usage(int on);

/** \brief  Is the SCIF port detected? Of course it is!
    \return                 1
*/
int scif_detected();

/** \brief  Initialize the SCIF port.

    This function initializes the SCIF port to a sane state. If dcload-serial is
    in use, this is effectively a no-op.

    \retval 0               On success (no error conditions defined).
*/
int scif_init();

/** \brief  Shutdown the SCIF port.

    This function disables SCIF IRQs, if they were enabled and cleans up.

    \retval 0               On success (no error conditions defined).
*/
int scif_shutdown();

/** \brief  Read a single character from the SCIF port.
    \return                 The character read if one is available, otherwise -1
                            and errno is set to EAGAIN.
*/
int scif_read();

/** \brief  Write a single character to the SCIF port.
    \param  c               The character to write (only the low 8-bits are
                            written).
    \retval 1               On success.
    \retval -1              If the SCIF port is disabled (errno set to EIO).
*/
int scif_write(int c);

/** \brief  Flush any FIFO'd bytes out of the buffer.

    This function sends any bytes that have been queued up for transmission but
    have not left yet in FIFO mode.

    \retval 0               On success.
    \retval -1              If the SCIF port is disabled (errno set to EIO).
*/
int scif_flush();

/** \brief  Write a whole buffer of data to the SCIF port.

    This function writes a whole buffer of data to the SCIF port, optionally
    making all newlines into carriage return + newline pairs.

    \param  data            The buffer to write.
    \param  len             The length of the buffer, in bytes.
    \param  xlat            If set to 1, all newlines will be written as CRLF.
    \return                 The number of bytes written on success, -1 on error.
*/
int scif_write_buffer(const uint8 *data, int len, int xlat);

/** \brief  Read a buffer of data from the SCIF port.

    This function reads a whole buffer of data from the SCIF port, blocking
    until it has been filled.

    \param  data            The buffer to read into.
    \param  len             The number of bytes to read.
    \return                 The number of bytes read on success, -1 on error.
*/
int scif_read_buffer(uint8 *data, int len);

/** \brief  SCIF debug I/O handler. Do not modify! */
extern dbgio_handler_t dbgio_scif;

__END_DECLS

#endif  /* __DC_SCIF_H */

