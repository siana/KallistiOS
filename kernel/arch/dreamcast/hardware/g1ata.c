/* KallistiOS ##version##

   hardware/g1ata.c
   Copyright (C) 2013 Lawrence Sebald
*/

#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <dc/g1ata.h>
#include <kos/dbglog.h>
#include <arch/timer.h>

/*
   This file implements support for accessing devices over the G1 bus by the
   AT Attachment (aka ATA, PATA, or IDE) protocol. See, the GD-ROM drive is
   actually just an ATA device that implements a different packet command set
   than the normal ATAPI set. Not only that, but Sega left everything in the
   hardware to actually support two devices on the bus at a time. Thus, you can
   put together a very simple passthrough adapter to get a normal 40-pin IDE
   port to work with and hook up a hard drive to. In theory, you could also hook
   up various other devices including DVD drives, CD Burners, and the whole nine
   yards, but for now this just supports hard drives (and Compact Flash cards).

   The setup here is relatively simple, because we only have one channel which
   can have a maximum of two devices attached to it at a time. Normally the
   primary device would be the GD-ROM drive itself, so we're only actually
   concerned with the secondary device (use the normal cdrom_* functions to
   access the GD-ROM drive -- there's not a particularly compelling reason to
   support its odd packet interface here). Also, at the moment, only PIO
   transfers are supported. I'll look into DMA at some point in the future.

   There are a few potentially useful outward facing functions here, but most of
   the time all you'll need here is the function to get a block device for a
   given partition. The individual block read/write functions are all public as
   well, in case you have a reason to want to use them directly. Just keep in
   mind that all block numbers in those are absolute (i.e, not offset by any
   partition boundaries or whatnot).

   If you want to learn more about ATA, look around the internet for the
   AT Attachment - 8 ATA/ATAPI Command Set document. That's where most of the
   fun stuff in here comes from. Register locations and such were derived from
   a couple of different sources, including Quzar's GDINFO program, my own SPI
   CD Player program (which I should eventually release), and the source code to
   the emulator NullDC. Also, various postings at OSDev were quite useful in 
   working some of this out.

   Anyway, that's enough for this wall of text...
*/

/* An ATA device. For the moment, we only support one of these, which happens to
   be the slave device on the only ATA bus Sega gave us. */
static struct {
    uint32_t command_sets;
    uint32_t capabilities;
    uint64_t max_lba;
    uint16_t cylinders;
    uint16_t heads;
    uint16_t sectors;
    uint16_t reserved;
} device;

/* The type of the dev_data in the block device structure */
typedef struct ata_devdata {
    uint64_t block_count;
    uint64_t start_block;
    uint64_t end_block;
} ata_devdata_t;

/* ATA-related registers. Some of these serve very different purposes when read
   than they do when written (hence why some addresses are duplicated). */
#define G1_ATA_ALTSTATUS        0xA05F7018      /* Read */
#define G1_ATA_CTL              0xA05F7018      /* Write */
#define G1_ATA_DATA             0xA05F7080      /* Read/Write */
#define G1_ATA_ERROR            0xA05F7084      /* Read */
#define G1_ATA_FEATURES         0xA05F7084      /* Write */
#define G1_ATA_IRQ_REASON       0xA05F7088      /* Read */
#define G1_ATA_SECTOR_COUNT     0xA05F7088      /* Write */
#define G1_ATA_LBA_LOW          0xA05F708C      /* Read/Write */
#define G1_ATA_LBA_MID          0xA05F7090      /* Read/Write */
#define G1_ATA_LBA_HIGH         0xA05F7094      /* Read/Write */
#define G1_ATA_CHS_SECTOR       G1_ATA_LBA_LOW
#define G1_ATA_CHS_CYL_LOW      G1_ATA_LBA_MID
#define G1_ATA_CHS_CYL_HIGH     G1_ATA_LBA_HIGH
#define G1_ATA_DEVICE_SELECT    0xA05F7098      /* Read/Write */
#define G1_ATA_STATUS_REG       0xA05F709C      /* Read */
#define G1_ATA_COMMAND_REG      0xA05F709C      /* Write */

/* Bitmasks for the STATUS_REG/ALT_STATUS registers. */
#define G1_ATA_SR_ERR   0x01
#define G1_ATA_SR_IDX   0x02
#define G1_ATA_SR_CORR  0x04
#define G1_ATA_SR_DRQ   0x08
#define G1_ATA_SR_DSC   0x10
#define G1_ATA_SR_DF    0x20
#define G1_ATA_SR_DRDY  0x40
#define G1_ATA_SR_BSY   0x80

/* ATA Commands we might like to send. */
#define ATA_CMD_READ_SECTORS        0x20
#define ATA_CMD_READ_SECTORS_EXT    0x24
#define ATA_CMD_WRITE_SECTORS       0x30
#define ATA_CMD_WRITE_SECTORS_EXT   0x34
#define ATA_CMD_FLUSH_CACHE         0xE7
#define ATA_CMD_FLUSH_CACHE_EXT     0xEA
#define ATA_CMD_IDENTIFY            0xEC

/* Macros to access the ATA registers */
#define OUT32(addr, data) *((volatile uint32_t *)addr) = data
#define OUT16(addr, data) *((volatile uint16_t *)addr) = data
#define OUT8(addr, data)  *((volatile uint8_t  *)addr) = data
#define IN32(addr)        *((volatile uint32_t *)addr)
#define IN16(addr)        *((volatile uint16_t *)addr)
#define IN8(addr)         *((volatile uint8_t  *)addr)

static int initted = 0;
static int devices = 0;

#define g1_ata_wait_nbsy() \
    do {} while((IN8(G1_ATA_ALTSTATUS) & G1_ATA_SR_BSY))

#define g1_ata_wait_drdy() \
    do {} while(!(IN8(G1_ATA_ALTSTATUS) & G1_ATA_SR_DRDY))

/* This one is an inline function since it needs to return something... */
static inline int g1_ata_wait_drq(void) {
    uint8_t val = IN8(G1_ATA_ALTSTATUS);

    while(!(val & G1_ATA_SR_DRQ) && !(val & (G1_ATA_SR_ERR | G1_ATA_SR_DF))) {
        val = IN8(G1_ATA_ALTSTATUS);
    }

    return (val & (G1_ATA_SR_ERR | G1_ATA_SR_DF)) ? -1 : 0;
}

int g1_ata_read_chs(uint16_t c, uint8_t h, uint8_t s, size_t count,
                    uint16_t *buf) {
    int rv = 0;
    unsigned int i, j;
    uint8_t nsects = (uint8_t)count;
    uint8_t dsel;

    /* Make sure that we've been initialized and there's a disk attached. */
    if(!devices) {
        errno = ENODEV;
        return -1;
    }

    /* Wait for the device to signal it is ready. */
    g1_ata_wait_nbsy();

    /* For now, just assume we're accessing the slave device. We don't care
       about the primary device, since it should always be the GD-ROM drive. */
    dsel = IN8(G1_ATA_DEVICE_SELECT);

    while(count) {
        nsects = count > 255 ? 255 : (uint8_t)count;
        count -= nsects;

        OUT8(G1_ATA_DEVICE_SELECT, 0xB0 | (h & 0x0F));

        /* Write out the number of sectors we want as well as the cylinder and
           sector. */
        OUT8(G1_ATA_SECTOR_COUNT, nsects);
        OUT8(G1_ATA_CHS_SECTOR, s);
        OUT8(G1_ATA_CHS_CYL_LOW,  (uint8_t)((c >> 0) & 0xFF));
        OUT8(G1_ATA_CHS_CYL_HIGH, (uint8_t)((c >> 8) & 0xFF));

        /* Wait until the drive is ready to accept the command. */
        g1_ata_wait_nbsy();
        g1_ata_wait_drdy();

        /* Write out the command to the device. */
        OUT8(G1_ATA_COMMAND_REG, ATA_CMD_READ_SECTORS);

        /* Now, wait for the drive to give us back each sector. */
        for(i = 0; i < nsects; ++i, ++s) {
            /* Make sure to keep track of where we are, just in case something
               errors out (or we have to deal with a second pass). */
            if(s >= device.sectors) {
                if(++h == device.heads) {
                    h = 0;
                    ++c;
                }

                s = 1;
            }

            /* Wait for data */
            if(g1_ata_wait_drq()) {
                dbglog(DBG_KDEBUG, "g1_ata_read_chs: error reading CHS "
                       "%d, %d, %d of device: %02x\n", (int)c, (int)h, (int)s,
                       IN8(G1_ATA_ALTSTATUS));
                errno = EIO;
                rv = -1;
                goto out;
            }

            for(j = 0; j < 256; ++j) {
                *buf++ = IN16(G1_ATA_DATA);
            }
        }
    }

    rv = 0;

out:
    OUT8(G1_ATA_DEVICE_SELECT, dsel);
    return rv;
}

int g1_ata_write_chs(uint16_t c, uint8_t h, uint8_t s, size_t count,
                     const uint16_t *buf) {
    int rv = 0;
    unsigned int i, j;
    uint8_t nsects = (uint8_t)count;
    uint8_t dsel;

    /* Make sure that we've been initialized and there's a disk attached. */
    if(!devices) {
        errno = ENXIO;
        return -1;
    }

    /* Wait for the device to signal it is ready. */
    g1_ata_wait_nbsy();

    /* For now, just assume we're accessing the slave device. We don't care
       about the primary device, since it should always be the GD-ROM drive. */
    dsel = IN8(G1_ATA_DEVICE_SELECT);

    while(count) {
        nsects = count > 255 ? 255 : (uint8_t)count;
        count -= nsects;

        OUT8(G1_ATA_DEVICE_SELECT, 0xB0 | (h & 0x0F));

        /* Write out the number of sectors we want as well as the cylinder and
           sector. */
        OUT8(G1_ATA_SECTOR_COUNT, nsects);
        OUT8(G1_ATA_CHS_SECTOR, s);
        OUT8(G1_ATA_CHS_CYL_LOW,  (uint8_t)((c >> 0) & 0xFF));
        OUT8(G1_ATA_CHS_CYL_HIGH, (uint8_t)((c >> 8) & 0xFF));

        /* Wait until the drive is ready to accept the command. */
        g1_ata_wait_nbsy();
        g1_ata_wait_drdy();

        /* Write out the command to the device. */
        OUT8(G1_ATA_COMMAND_REG, ATA_CMD_WRITE_SECTORS);

        /* Now, send the drive each sector. */
        for(i = 0; i < nsects; ++i, ++s) {
            /* Make sure to keep track of where we are, just in case something
               errors out (or we have to deal with a second pass). */
            if(s >= device.sectors) {
                if(++h >= device.heads) {
                    h = 0;
                    ++c;
                }

                s = 1;
            }

            /* Wait for the device to signal it is ready. */
            g1_ata_wait_nbsy();

            /* Send the data! */
            for(j = 0; j < 256; ++j) {
                OUT16(G1_ATA_DATA, *buf++);
            }
        }
    }

    rv = 0;

    OUT8(G1_ATA_DEVICE_SELECT, dsel);
    return rv;
}

int g1_ata_read_lba(uint64_t sector, size_t count, uint16_t *buf) {
    int rv = 0;
    unsigned int i, j;
    uint8_t nsects = (uint8_t)count;
    uint8_t dsel;

    /* Make sure that we've been initialized and there's a disk attached. */
    if(!devices) {
        errno = ENXIO;
        return -1;
    }

    /* Make sure the disk supports LBA mode. */
    if(!device.max_lba) {
        errno = ENOTSUP;
        return -1;
    }

    /* Make sure the range of sectors is valid. */
    if((sector + count) > device.max_lba) {
        errno = EOVERFLOW;
        return -1;
    }

    /* Wait for the device to signal it is ready. */
    g1_ata_wait_nbsy();

    /* For now, just assume we're accessing the slave device. We don't care
       about the primary device, since it should always be the GD-ROM drive. */
    dsel = IN8(G1_ATA_DEVICE_SELECT);

    while(count) {
        nsects = count > 255 ? 255 : (uint8_t)count;
        count -= nsects;

        /* Which mode are we using: LBA28 or LBA48? */
        if((sector + nsects) <= 0x0FFFFFFF) {
            OUT8(G1_ATA_DEVICE_SELECT, 0xF0 | ((sector >> 24) & 0x0F));

            /* Write out the number of sectors we want and the lower 24-bits of
               the LBA we're looking for. */
            OUT8(G1_ATA_SECTOR_COUNT, nsects);
            OUT8(G1_ATA_LBA_LOW,  (uint8_t)((sector >>  0) & 0xFF));
            OUT8(G1_ATA_LBA_MID,  (uint8_t)((sector >>  8) & 0xFF));
            OUT8(G1_ATA_LBA_HIGH, (uint8_t)((sector >> 16) & 0xFF));

            /* Wait until the drive is ready to accept the command. */
            g1_ata_wait_nbsy();
            g1_ata_wait_drdy();

            /* Write out the command to the device. */
            OUT8(G1_ATA_COMMAND_REG, ATA_CMD_READ_SECTORS);
        }
        else {
            OUT8(G1_ATA_DEVICE_SELECT, 0xF0);

            /* Write out the number of sectors we want and the LBA. */
            OUT8(G1_ATA_SECTOR_COUNT, 0);
            OUT8(G1_ATA_LBA_LOW,  (uint8_t)((sector >> 24) & 0xFF));
            OUT8(G1_ATA_LBA_MID,  (uint8_t)((sector >> 32) & 0xFF));
            OUT8(G1_ATA_LBA_HIGH, (uint8_t)((sector >> 40) & 0xFF));
            OUT8(G1_ATA_SECTOR_COUNT, nsects);
            OUT8(G1_ATA_LBA_LOW,  (uint8_t)((sector >>  0) & 0xFF));
            OUT8(G1_ATA_LBA_MID,  (uint8_t)((sector >>  8) & 0xFF));
            OUT8(G1_ATA_LBA_HIGH, (uint8_t)((sector >> 16) & 0xFF));

            /* Wait until the drive is ready to accept the command. */
            g1_ata_wait_nbsy();
            g1_ata_wait_drdy();

            /* Write out the command to the device. */
            OUT8(G1_ATA_COMMAND_REG, ATA_CMD_READ_SECTORS_EXT);
        }

        /* Now, wait for the drive to give us back each sector. */
        for(i = 0; i < nsects; ++i, ++sector) {
            /* Wait for data */
            if(g1_ata_wait_drq()) {
                dbglog(DBG_KDEBUG, "g1_ata_read_lba: error reading sector %d "
                       "of device: %02x\n", (int)sector, IN8(G1_ATA_ALTSTATUS));
                errno = EIO;
                rv = -1;
                goto out;
            }

            for(j = 0; j < 256; ++j) {
                *buf++ = IN16(G1_ATA_DATA);
            }
        }
    }

    rv = 0;
    
out:
    OUT8(G1_ATA_DEVICE_SELECT, dsel);
    return rv;
}

int g1_ata_write_lba(uint64_t sector, size_t count, const uint16_t *buf) {
    int rv = 0;
    unsigned int i, j;
    uint8_t nsects = (uint8_t)count;
    uint8_t dsel;

    /* Make sure that we've been initialized and there's a disk attached. */
    if(!devices) {
        errno = ENXIO;
        return -1;
    }

    /* Make sure the disk supports LBA mode. */
    if(!device.max_lba) {
        errno = ENOTSUP;
        return -1;
    }

    /* Make sure the range of sectors is valid. */
    if((sector + count) > device.max_lba) {
        errno = EOVERFLOW;
        return -1;
    }

    /* Wait for the device to signal it is ready. */
    g1_ata_wait_nbsy();

    /* For now, just assume we're accessing the slave device. We don't care
       about the primary device, since it should always be the GD-ROM drive. */
    dsel = IN8(G1_ATA_DEVICE_SELECT);

    while(count) {
        nsects = count > 255 ? 255 : (uint8_t)count;
        count -= nsects;

        /* Which mode are we using: LBA28 or LBA48? */
        if((sector + nsects) <= 0x0FFFFFFF) {
            OUT8(G1_ATA_DEVICE_SELECT, 0xF0 | ((sector >> 24) & 0x0F));

            /* Write out the number of sectors we want and the lower 24-bits of
               the LBA we're looking for. */
            OUT8(G1_ATA_SECTOR_COUNT, nsects);
            OUT8(G1_ATA_LBA_LOW,  (uint8_t)((sector >>  0) & 0xFF));
            OUT8(G1_ATA_LBA_MID,  (uint8_t)((sector >>  8) & 0xFF));
            OUT8(G1_ATA_LBA_HIGH, (uint8_t)((sector >> 16) & 0xFF));

            /* Write out the command to the device. */
            OUT8(G1_ATA_COMMAND_REG, ATA_CMD_WRITE_SECTORS);
        }
        else {
            OUT8(G1_ATA_DEVICE_SELECT, 0xF0);

            /* Write out the number of sectors we want and the LBA. */
            OUT8(G1_ATA_SECTOR_COUNT, 0);
            OUT8(G1_ATA_LBA_LOW,  (uint8_t)((sector >> 24) & 0xFF));
            OUT8(G1_ATA_LBA_MID,  (uint8_t)((sector >> 32) & 0xFF));
            OUT8(G1_ATA_LBA_HIGH, (uint8_t)((sector >> 40) & 0xFF));
            OUT8(G1_ATA_SECTOR_COUNT, nsects);
            OUT8(G1_ATA_LBA_LOW,  (uint8_t)((sector >>  0) & 0xFF));
            OUT8(G1_ATA_LBA_MID,  (uint8_t)((sector >>  8) & 0xFF));
            OUT8(G1_ATA_LBA_HIGH, (uint8_t)((sector >> 16) & 0xFF));

            /* Write out the command to the device. */
            OUT8(G1_ATA_COMMAND_REG, ATA_CMD_WRITE_SECTORS_EXT);
        }

        /* Now, send the drive each sector. */
        for(i = 0; i < nsects; ++i, ++sector) {
            /* Wait for the device to signal it is ready. */
            g1_ata_wait_nbsy();

            /* Send the data! */
            for(j = 0; j < 256; ++j) {
                OUT16(G1_ATA_DATA, *buf++);
            }
        }
    }

    rv = 0;

    OUT8(G1_ATA_DEVICE_SELECT, dsel);
    return rv;
}

int g1_ata_flush(void) {
    uint8_t dsel;

    /* Make sure that we've been initialized and there's a disk attached. */
    if(!devices) {
        errno = ENXIO;
        return -1;
    }

    /* Select the slave device. */
    dsel = IN8(G1_ATA_DEVICE_SELECT);
    OUT8(G1_ATA_DEVICE_SELECT, 0xF0);
    timer_spin_sleep(1);

    /* Flush the disk's write cache to make sure everything gets written out. */
    if(device.max_lba > 0x0FFFFFFF)
        OUT8(G1_ATA_COMMAND_REG, ATA_CMD_FLUSH_CACHE_EXT);
    else
        OUT8(G1_ATA_COMMAND_REG, ATA_CMD_FLUSH_CACHE);

    timer_spin_sleep(1);
    g1_ata_wait_nbsy();

    /* Restore the old selected device and return. */
    OUT8(G1_ATA_DEVICE_SELECT, dsel);
    return 0;
}

static int g1_ata_scan(void) {
    uint8_t dsel = IN8(G1_ATA_DEVICE_SELECT), st;
    int rv, i;
    uint16_t data[256];

    /* For now, just check if there's a slave device. We don't care about the
       primary device, since it should always be the GD-ROM drive. */
    OUT8(G1_ATA_DEVICE_SELECT, 0xF0);
    timer_spin_sleep(1);

    OUT8(G1_ATA_SECTOR_COUNT, 0);
    OUT8(G1_ATA_LBA_LOW, 0);
    OUT8(G1_ATA_LBA_MID, 0);
    OUT8(G1_ATA_LBA_HIGH, 0);

    /* Send the IDENTIFY command. */
    OUT8(G1_ATA_COMMAND_REG, ATA_CMD_IDENTIFY);
    timer_spin_sleep(1);
    st = IN8(G1_ATA_STATUS_REG);

    /* Check if there's anything on the bus. */
    if(!st || st == 0xFF) {
        rv = 0;
        goto out;
    }

    /* Wait for the device to finish. */
    g1_ata_wait_nbsy();

    /* Wait for data. */
    if(g1_ata_wait_drq()) {
        dbglog(DBG_KDEBUG, "g1_ata_scan: error while identifying device\n"
                           "             possibly ATAPI? %02x %02x\n",
               IN8(G1_ATA_LBA_MID), IN8(G1_ATA_LBA_HIGH));
        rv = 0;
        goto out;
    }

    /* Read out the data from the device. There will always be 256 words of
       data, according to the spec. */
    for(i = 0; i < 256; ++i)
        data[i] = IN16(G1_ATA_DATA);

    /* Read off some information we might need. */
    device.command_sets = (uint32_t)(data[82]) | ((uint32_t)(data[83]) << 16);
    device.capabilities = (uint32_t)(data[49]) | ((uint32_t)(data[50]) << 16);

    /* See if we support LBA mode or not... */
    if(!(device.capabilities & (1 << 9))) {
        /* Nope. We have to use CHS addressing... >_< */
        device.max_lba = 0;
        device.cylinders = data[1];
        device.heads = data[3];
        device.sectors = data[6];
        dbglog(DBG_KDEBUG, "g1_ata_scan: found device with CHS: %d %d %d\n",
               device.cylinders, device.heads, device.sectors);
    }
    /* Do we support LBA48? */
    else if(!(device.command_sets & (1 << 26))) {
        /* Nope, use LBA28 */
        device.max_lba = (uint64_t)(data[60]) | ((uint64_t)(data[61]) << 16);
        device.cylinders = device.heads = device.sectors = 0;
        dbglog(DBG_KDEBUG, "g1_ata_scan: found device with LBA28: %llu\n",
               device.max_lba);
    }
    else {
        /* Yep, we support LBA48 */
        device.max_lba = (uint64_t)(data[100]) | ((uint64_t)(data[101]) << 16) |
            ((uint64_t)(data[102]) << 32) | ((uint64_t)(data[103]) << 48);
        device.cylinders = device.heads = device.sectors = 0;
        dbglog(DBG_KDEBUG, "g1_ata_scan: found device with LBA48: %llu\n",
               device.max_lba);
    }

    rv = 1;

out:
    OUT8(G1_ATA_DEVICE_SELECT, dsel);
    return rv;
}

/* Block device interface. */
static int atab_init(kos_blockdev_t *d) {
    (void)d;

    if(!initted) {
        errno = ENXIO;
        return -1;
    }

    return 0;
}

static int atab_shutdown(kos_blockdev_t *d) {
    free(d->dev_data);
    return 0;
}

static int atab_read_blocks(kos_blockdev_t *d, uint64_t block, size_t count,
                           void *buf) {
    ata_devdata_t *data = (ata_devdata_t *)d->dev_data;

    if(block + count > data->end_block) {
        errno = EOVERFLOW;
        return -1;
    }

    return g1_ata_read_lba(block + data->start_block, count, (uint16_t *)buf);
}

static int atab_write_blocks(kos_blockdev_t *d, uint64_t block, size_t count,
                            const void *buf) {
    ata_devdata_t *data = (ata_devdata_t *)d->dev_data;

    if(block + count > data->end_block) {
        errno = EOVERFLOW;
        return -1;
    }

    return g1_ata_write_lba(block + data->start_block, count,
                            (const uint16_t *)buf);
}

static int atab_read_blocks_chs(kos_blockdev_t *d, uint64_t block, size_t count,
                                void *buf) {
    ata_devdata_t *data = (ata_devdata_t *)d->dev_data;
    uint8_t h, s;
    uint16_t c;

    if(block + count > data->end_block) {
        errno = EOVERFLOW;
        return -1;
    }

    /* Convert the LBA address to CHS */
    block += data->start_block;
    c = (uint16_t)(block / (device.sectors * device.heads));
    h = (uint8_t)((block / device.sectors) % device.heads);
    s = (uint8_t)((block % device.sectors) + 1);

    return g1_ata_read_chs(c, h, s, count, (uint16_t *)buf);
}

static int atab_write_blocks_chs(kos_blockdev_t *d, uint64_t block,
                                 size_t count, const void *buf) {
    ata_devdata_t *data = (ata_devdata_t *)d->dev_data;
    uint8_t h, s;
    uint16_t c;

    if(block + count > data->end_block) {
        errno = EOVERFLOW;
        return -1;
    }

    /* Convert the LBA address to CHS */
    block += data->start_block;
    c = (uint16_t)(block / (device.sectors * device.heads));
    h = (uint8_t)((block / device.sectors) % device.heads);
    s = (uint8_t)((block % device.sectors) + 1);

    return g1_ata_write_chs(c, h, s, count, (const uint16_t *)buf);
}

static uint64_t atab_count_blocks(kos_blockdev_t *d) {
    ata_devdata_t *data = (ata_devdata_t *)d->dev_data;

    return (uint32_t)data->block_count;
}

static int atab_flush(kos_blockdev_t *d) {
    (void)d;
    return g1_ata_flush();
}

static kos_blockdev_t ata_blockdev = {
    NULL,                   /* dev_data */
    9,                      /* l_block_size (block size of 512 bytes) */
    &atab_init,             /* init */
    &atab_shutdown,         /* shutdown */
    &atab_read_blocks,      /* read_blocks */
    &atab_write_blocks,     /* write_blocks */
    &atab_count_blocks,     /* count_blocks */
    &atab_flush             /* flush */
};

static kos_blockdev_t ata_blockdev_chs = {
    NULL,                   /* dev_data */
    9,                      /* l_block_size (block size of 512 bytes) */
    &atab_init,             /* init */
    &atab_shutdown,         /* shutdown */
    &atab_read_blocks_chs,  /* read_blocks */
    &atab_write_blocks_chs, /* write_blocks */
    &atab_count_blocks,     /* count_blocks */
    &atab_flush             /* flush */
};

int g1_ata_blockdev_for_partition(int partition, kos_blockdev_t *rv,
                                  uint8_t *partition_type) {
    uint8_t buf[512];
    int pval;
    ata_devdata_t *ddata;

    if(!initted) {
        errno = ENXIO;
        return -1;
    }

    if(!rv || !partition_type) {
        errno = EFAULT;
        return -1;
    }

    /* Make sure the partition asked for is sane */
    if(partition < 0 || partition > 3) {
        dbglog(DBG_DEBUG, "Invalid partition number given: %d\n", partition);
        errno = EINVAL;
        return -1;
    }

    /* Read the MBR from the disk */
    if(device.max_lba) {
        if(g1_ata_read_lba(0, 1, (uint16_t *)buf) < 0)
            return -1;
    }
    else {
        if(g1_ata_read_chs(0, 0, 1, 1, (uint16_t *)buf) < 0)
            return -1;
    }

    /* Make sure the ATA disk uses MBR partitions.
       TODO: Support GPT partitioning at some point. */
    if(buf[0x01FE] != 0x55 || buf[0x1FF] != 0xAA) {
        dbglog(DBG_DEBUG, "ATA device doesn't appear to have a MBR %02x %02x\n",
               buf[0x01fe], buf[0x01ff]);
        errno = ENOENT;
        return -1;
    }

    /* Figure out where the partition record we're concerned with is, and make
       sure that the partition actually exists. */
    pval = 16 * partition + 0x01BE;

    if(buf[pval + 4] == 0) {
        dbglog(DBG_DEBUG, "Partition %d appears to be empty\n", partition);
        errno = ENOENT;
        return -1;
    }

    /* Allocate the device data */
    if(!(ddata = (ata_devdata_t *)malloc(sizeof(ata_devdata_t)))) {
        errno = ENOMEM;
        return -1;
    }

    /* Copy in the template block device and fill it in */
    if(device.max_lba)
        memcpy(rv, &ata_blockdev, sizeof(kos_blockdev_t));
    else
        memcpy(rv, &ata_blockdev_chs, sizeof(kos_blockdev_t));

    /* It doesn't matter whether we're using CHS or LBA... We only bother to
       parse out the LBA information from the MBR. Should be valid either
       way (whereas the CHS stuff may very well be invalid). */
    ddata->block_count = buf[pval + 0x0C] | (buf[pval + 0x0D] << 8) |
        (buf[pval + 0x0E] << 16) | (buf[pval + 0x0F] << 24);
    ddata->start_block = buf[pval + 0x08] | (buf[pval + 0x09] << 8) |
        (buf[pval + 0x0A] << 16) | (buf[pval + 0x0B] << 24);
    ddata->end_block = ddata->start_block + ddata->block_count - 1;
    rv->dev_data = ddata;
    *partition_type = buf[pval + 4];

    return 0;
}

int g1_ata_init(void) {
    if(initted)
        return 0;

    /* Scan for devices. */
    if((devices = g1_ata_scan()) < 0) {
        devices = 0;
        return -1;
    }

    if(!devices) {
        dbglog(DBG_KDEBUG, "g1_ata_init: no adapter or device present\n");
        return -1;
    }

    initted = 1;
    
    return 0;
}

void g1_ata_shutdown(void) {
    /* Make sure to flush any cached data out. */
    g1_ata_flush();

    devices = 0;
    initted = 0;

    memset(&device, 0, sizeof(device));
}
