/* KallistiOS ##version##

   dc/maple/sip.h
   Copyright (C) 2005, 2008 Lawrence Sebald

*/

#ifndef __DC_MAPLE_SIP_H
#define __DC_MAPLE_SIP_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <sys/types.h>

/* This driver controls the Sound Input Peripheral for the maple bus (aka, the
   microphone that came with Seaman and the Dreameye). Many thanks go out to
   ZeZu for pointing me towards what some commands actually do in the original
   version of this driver.
*/

/* SIP Status structure. Everything in here should be considered to be read-only
   from user programs. */
typedef struct	sip_state {
    int             amp_gain;
    int             sample_type;
    int             frequency;
    int             is_sampling;
    size_t          buf_len;
    off_t           buf_pos;
    uint8          *samples_buf;
} sip_state_t;

/* Subcommands of the Microphone command. */
#define SIP_SUBCOMMAND_GET_SAMPLES 0x01
#define SIP_SUBCOMMAND_BASIC_CTRL  0x02

/* The minimum, maximum, and default gain values that can be passed to the
   sip_set_gain function. */
#define SIP_MIN_GAIN     0x00
#define SIP_DEFAULT_GAIN 0x0F
#define SIP_MAX_GAIN     0x1F

/* Set the amplifier's gain. This should only be called prior to sampling, to 
   ensure that the sound returned is of the same volume (unlike the other
   functions below, this is not strictly a requirement, as this gets sent
   with all of the get samples packets). */
int sip_set_gain(maple_device_t *dev, unsigned int g);

/* Sample types. These two values are the only defined types of samples that
   the SIP can output. 16-bit signed is your standard 16-bit signed samples,
   where 8-bit ulaw is obvously encoded as ulaw. */
#define SIP_SAMPLE_16BIT_SIGNED 0x00
#define SIP_SAMPLE_8BIT_ULAW    0x01

/* Set the sample type to be returned by the microphone. This must be called
   prior to a sip_start_sampling if you want to change it from the default
   (16-bit signed is the default). */
int sip_set_sample_type(maple_device_t *dev, unsigned int type);

/* Sampling frequencies. The SIP supports sampling at either 8kHz or 11.025 kHz.
   One of these values should be passed to the sip_set_frequency function. */
#define SIP_SAMPLE_11KHZ 0x00
#define SIP_SAMPLE_8KHZ  0x01

/* Set the sampling frequency to be returned by the microphone. This must be
   called prior to a sip_start_sampling if you want to change it from the
   default (11kHz is the default). */
int sip_set_frequency(maple_device_t *dev, unsigned int freq);

/* Start sampling. If you want the function to block until sampling has started,
   set the block argument to something other than zero. Otherwise, check the
   is_sampling member of the device status to know when sampling has started. */
int sip_start_sampling(maple_device_t *dev, int block);

/* Stop sampling. Same comment about blocking above applies here too. */
int sip_stop_sampling(maple_device_t *dev, int block);

/* Grab the samples buffer from the device. This function can only be called
   when the microphone is not sampling. Once you call this function, you are
   responsible for the buffer, so you must free it when you're done. The sz
   pointer is used to return how long the sample data is. */
uint8 *sip_get_samples(maple_device_t *dev, size_t *sz);

/* Clear the internal sample buffer. */
int sip_clear_samples(maple_device_t *dev);

/* Init / Shutdown */
int sip_init();
void sip_shutdown();

__END_DECLS

#endif	/* __DC_MAPLE_SIP_H */
