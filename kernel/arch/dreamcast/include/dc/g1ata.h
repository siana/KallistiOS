/* KallistiOS ##version##

   dc/g1ata.h
   Copyright (C) 2013 Lawrence Sebald
*/

/** \file   dc/g1ata.h
    \brief  G1 bus ATA interface.

    This file provides support for accessing an ATA device on the G1 bus in the
    Dreamcast. The G1 bus usually contains a few useful pieces of the system,
    including the flashrom and the GD-ROM drive. The interesting piece here is
    that the GD-ROM drive itself is actually an ATA device.

    Luckily, Sega left everything in place to access both a master and slave
    device on this ATA port. The GD-ROM drive should always be the master device
    on the chain, but you can hook up a hard drive or some other device as a
    slave. The functions herein are for accessing just such a slave device.

    \note   The functions herein do not provide for direct access to the GD-ROM
            drive. There is not really any sort of compelling reason to access
            the GD-ROM drive directly instead of using the system calls, so you
            should continue to use the normal cdrom_* functions for accessing
            the GD-ROM drive. Also, currently there is no locking done to
            prevent you from doing "bad things" with concurrent access on the
            bus, so be careful. ;)

    \author Lawrence Sebald
*/

#ifndef __DC_G1ATA_H
#define __DC_G1ATA_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <stdint.h>
#include <kos/blockdev.h>

/** \brief  Read one or more disk sectors with Cylinder-Head-Sector addressing.

    This function reads one or more 512-byte disk blocks from the slave device
    on the G1 ATA bus using Cylinder-Head-Sector addressing. This function uses
    PIO and blocks until the data is read in.

    \param  c               The cylinder to start reading from.
    \param  h               The head to start reading from.
    \param  s               The sector to start reading from.
    \param  count           The number of disk sectors to read.
    \param  buf             Storage for the read-in disk sectors. This should be
                            at least (count * 512) bytes in length, and must be
                            at least 16-bit aligned.
    \return                 0 on success. < 0 on failure, setting errno as
                            appropriate.

    \note                   Unless you're accessing a really old hard drive, you
                            probably do not want to use this function to access
                            the disk. Use the g1_ata_read_lba() function instead
                            of this one, unless you get an error from that
                            function indicating that LBA addressing is not
                            supported.

    \par    Error Conditions:
    \em     EIO - an I/O error occurred in reading data \n
    \em     ENXIO - ATA support not initialized or no device attached \n
    \em     EOVERFLOW - one or more of the requested sectors is out of the
                        range of the disk
*/
int g1_ata_read_chs(uint16_t c, uint8_t h, uint8_t s, size_t count,
                    uint16_t *buf);

/** \brief  Write one or more disk sectors with Cylinder-Head-Sector addressing.

    This function writes one or more 512-byte disk blocks to the slave device
    on the G1 ATA bus using Cylinder-Head-Sector addressing. This function uses
    PIO and blocks until the data is written.

    \param  c               The cylinder to start writing to.
    \param  h               The head to start writing to.
    \param  s               The sector to start writing to.
    \param  count           The number of disk sectors to write.
    \param  buf             The data to write to the disk. This should be
                            (count * 512) bytes in length and must be at least
                            16-bit aligned.
    \return                 0 on success. < 0 on failure, setting errno as
                            appropriate.

    \note                   Unless you're accessing a really old hard drive, you
                            probably do not want to use this function to access
                            the disk. Use the g1_ata_write_lba() function
                            instead of this one, unless you get an error from
                            that function indicating that LBA addressing is not
                            supported.

    \par    Error Conditions:
    \em     ENXIO - ATA support not initialized or no device attached \n
    \em     EOVERFLOW - one or more of the requested sectors is out of the
                        range of the disk
*/
int g1_ata_write_chs(uint16_t c, uint8_t h, uint8_t s, size_t count,
                     const uint16_t *buf);

/** \brief  Read one or more disk sectors with Linear Block Addressing (LBA).

    This function reads one or more 512-byte disk blocks from the slave device
    on the G1 ATA bus using LBA mode (either 28 or 48 bits, as appropriate).
    This function uses PIO and blocks until the data is read.

    \param  sector          The sector to start reading from.
    \param  count           The number of disk sectors to read.
    \param  buf             Storage for the read-in disk sectors. This should be
                            at least (count * 512) bytes in length, and must be
                            at least 16-bit aligned.
    \return                 0 on success. < 0 on failure, setting errno as
                            appropriate.

    \note                   If errno is set to ENOTSUP after calling this
                            function, you must use the g1_ata_read_chs()
                            function instead.

    \par    Error Conditions:
    \em     EIO - an I/O error occurred in reading data \n
    \em     ENXIO - ATA support not initialized or no device attached \n
    \em     EOVERFLOW - one or more of the requested sectors is out of the
                        range of the disk \n
    \em     ENOTSUP - LBA mode not supported by the device
*/
int g1_ata_read_lba(uint64_t sector, size_t count, uint16_t *buf);

/** \brief  Write one or more disk sectors with Linear Block Addressing (LBA).

    This function writes one or more 512-byte disk blocks to the slave device
    on the G1 ATA bus using LBA mode (either 28 or 48 bits, as appropriate).
    This function uses PIO and blocks until the data is written.

    \param  sector          The sector to start writing to.
    \param  count           The number of disk sectors to write.
    \param  buf             The data to write to the disk. This should be
                            (count * 512) bytes in length and must be at least
                            16-bit aligned.
    \return                 0 on success. < 0 on failure, setting errno as
                            appropriate.

    \note                   If errno is set to ENOTSUP after calling this
                            function, you must use the g1_ata_write_chs()
                            function instead.

    \par    Error Conditions:
    \em     ENXIO - ATA support not initialized or no device attached \n
    \em     EOVERFLOW - one or more of the requested sectors is out of the
                        range of the disk \n
    \em     ENOTSUP - LBA mode not supported by the device
*/
int g1_ata_write_lba(uint64_t sector, size_t count, const uint16_t *buf);

/** \brief  Flush the write cache on the attached disk.

    This function flushes the write cache on the disk attached as the slave
    device on the G1 ATA bus. This ensures that all writes that have previously
    completed are fully persisted to the disk. You should do this before
    unmounting any disks or exiting your program if you have called any of the
    write functions in here.

    \return                 0 on success. <0 on error, setting errno as
                            appropriate.

    \par    Error Conditions:
    \em     ENXIO - ATA support not initialized or no device attached
*/
int g1_ata_flush(void);

/** \brief  Get a block device for a given partition on the slave ATA device.

    This function creates a block device descriptor for the given partition on
    the attached ATA device. This block device is used to interface with various
    filesystems on the device.

    \param  partition       The partition number (0-3) to use.
    \param  rv              Used to return the block device. Must be non-NULL.
    \param  partition_type  Used to return the partition type. Must be non-NULL.
    \retval 0               On success.
    \retval -1              On error, errno will be set as appropriate.

    \par    Error Conditions:
    \em     ENXIO - ATA support not initialized or no device attached \n
    \em     EIO - an I/O error occurred in reading data \n
    \em     EINVAL - invalid partition number was given \n
    \em     EFAULT - rv or partition_type was NULL \n
    \em     ENOENT - no MBR found \n
    \em     ENOENT - no partition at the specified position \n
    \em     ENOMEM - out of memory

    \note   This interface currently only supports MBR-formatted disks. There
            is currently no support for GPT partition tables.
*/
int g1_ata_blockdev_for_partition(int partition, kos_blockdev_t *rv,
                                  uint8_t *partition_type);

/** \brief  Initialize G1 ATA support.

    This function initializes the rest of this subsystem and completes a scan of
    the G1 ATA bus for devices. This function may take a while to complete with
    some devices. Currently only the slave device is scanned, as the master
    device should always be the GD-ROM drive.

    \return                 0 on success. <0 on error.
*/
int g1_ata_init(void);

/** \brief  Shut down G1 ATA support.

    This function shuts down the rest of this subsystem, and attempts to flush
    the write cache of any attached slave devices. Accessing any ATA devices
    using this subsystem after this function is called may produce undefined
    results.
*/
void g1_ata_shutdown(void);

#endif /* __DC_G1ATA_H */
