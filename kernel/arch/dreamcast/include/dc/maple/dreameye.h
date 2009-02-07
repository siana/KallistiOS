/* KallistiOS ##version##

   dc/maple/dreameye.h
   Copyright (C) 2005, 2009 Lawrence Sebald

*/

#ifndef __DC_MAPLE_DREAMEYE_H
#define __DC_MAPLE_DREAMEYE_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <arch/types.h>

/* Dreameye Status structure. Everything in here should be considered to be
   read-only from user programs. Note that most of this is used for keeping
   track of what's going on during an image transfer. */
typedef struct dreameye_state {
    int             image_count;
    int             image_count_valid;
    int             img_transferring;
    uint8          *img_buf;
    int             img_size;
    uint8           img_number;
    uint8           img_counter;
} dreameye_state_t;

/* Attributes that can be obtained with the Get Condition command. */
#define DREAMEYE_GETCOND_NUM_IMAGES     0x81

/* Subcommands that are used with Camera Control command. */
#define DREAMEYE_SUBCOMMAND_IMAGEREQ    0x04
#define DREAMEYE_SUBCOMMAND_ERASE       0x05
#define DREAMEYE_SUBCOMMAND_ERROR       0xFF

/* Used with the image request subcommand. */
#define DREAMEYE_IMAGEREQ_CONTINUE      0x00
#define DREAMEYE_IMAGEREQ_START         0x40

/* Grab the current number of saved images on the Dreameye. This command can be
   sent to any of the sub-devices. Set block to 1 in order to wait for a
   response from the Dreameye. When a response arrives, the state image_count
   will be set and image_count_valid will be set to 1. The return value from
   this function IS NOT the number of images. Returns MAPLE_EOK on success. */
int dreameye_get_image_count(maple_device_t *dev, int block);

/* Grab a specified image from the Dreameye. This command can take some time,
   and (for the time being, anyway) will block. You can send this command to any
   of the sub-devices. You are responsible for freeing the buffer after the
   command has completed if you recieve a MAPLE_EOK response. */
int dreameye_get_image(maple_device_t *dev, uint8 image, uint8 **data,
                       int *img_sz);

/* Erase an image from the Dreameye. This command can be sent to any of the
   sub-devices. Set block to 1 in order to wait for a response from the
   Dreameye. Pass an image of 0xFF to erase all images from the Dreameye.
   Returns MAPLE_EOK on success, MAPLE_EINVALID if an invalid image number is
   passed in. */
int dreameye_erase_image(maple_device_t *dev, uint8 image, int block);

/* Init / Shutdown */
int dreameye_init();
void dreameye_shutdown();

__END_DECLS

#endif	/* __DC_MAPLE_DREAMEYE_H */

