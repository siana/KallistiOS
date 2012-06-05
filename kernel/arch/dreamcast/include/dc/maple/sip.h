/* KallistiOS ##version##

   dc/maple/sip.h
   Copyright (C) 2005, 2008, 2010 Lawrence Sebald

*/

/** \file   dc/maple/sip.h
    \brief  Definitions for using the Sound Input Peripheral.

    This file contains the definitions needed to access the Maple microphone
    type device (the Seaman mic). Many thanks go out to ZeZu who pointed me
    toward what some of the commands actually do in the original version of this
    driver.

    As a note, the device itself is actually referred to by the system as the
    Sound Input Peripheral, so hence why this driver is named as it is.

    \author Lawrence Sebald
*/

#ifndef __DC_MAPLE_SIP_H
#define __DC_MAPLE_SIP_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <sys/types.h>

/** \brief  SIP status structure.

    This structure contains information about the status of the microphone
    device and can be fetched with maple_dev_status(). You should not modify
    any of the values in here, it is all "read-only" to your programs. Modifying
    any of this, especially while the microphone is sampling could really screw
    things up.

    \headerfile dc/maple/sip.h
*/
typedef struct  sip_state {
    /** \brief  The gain value for the microphone amp. */
    int             amp_gain;

    /** \brief  The type of samples that are being recorded. */
    int             sample_type;

    /** \brief  What frequency are we sampling at? */
    int             frequency;

    /** \brief  Is the mic currently sampling? */
    int             is_sampling;

    /** \brief  How long is the samples buffer? */
    size_t          buf_len;

    /** \brief  What is the last place written to in the buffer? */
    off_t           buf_pos;

    /** \brief  Buffer for storing samples in (automatically allocated). */
    uint8          *samples_buf;
} sip_state_t;

/** \brief  Get recorded samples from the microphone device.

    This subcommand is used with the MAPLE_COMMAND_MICCONTROL command to fetch
    samples from the microphone.
*/
#define SIP_SUBCOMMAND_GET_SAMPLES 0x01

/** \brief  Start and stop sampling.

    This subcommand is used with the MAPLE_COMMAND_MICCONTROL command to start
    and stop sampling on the microphone.
*/
#define SIP_SUBCOMMAND_BASIC_CTRL  0x02

/** \brief  Minimum microphone gain. */
#define SIP_MIN_GAIN     0x00

/** \brief  Default microphone gain. */
#define SIP_DEFAULT_GAIN 0x0F

/** \brief  Maximum microphone gain. */
#define SIP_MAX_GAIN     0x1F

/** \brief  Set the microphone's gain value.

    This function sets the gain value of the specified microphone device to
    the value given. This should only be called prior to sampling so as to keep
    the amplification constant throughout the sampling process, but can be
    changed on the fly if you really want to.

    \param  dev             The microphone device to set gain on.
    \param  g               The value to set as the gain.
    \retval MAPLE_EOK       On success.
    \retval MAPLE_EINVALID  If g is out of range.
    \see    SIP_MIN_GAIN
    \see    SIP_DEFAULT_GAIN
    \see    SIP_MAX_GAIN
*/
int sip_set_gain(maple_device_t *dev, unsigned int g);

/* Sample types. These two values are the only defined types of samples that
   the SIP can output. 16-bit signed is your standard 16-bit signed samples,
   where 8-bit ulaw is obvously encoded as ulaw. */

/** \brief  Record 16-bit signed integer samples. */
#define SIP_SAMPLE_16BIT_SIGNED 0x00

/** \brief  Record 8-bit ulaw samples. */
#define SIP_SAMPLE_8BIT_ULAW    0x01

/** \brief  Set the sample type to be recorded by the microphone.

    This function sets the sample type that the microphone will return. The
    default value for this is 16-bit signed integer samples. You must call this
    prior to sip_start_sampling() if you wish to change it from the default.

    \param  dev             The microphone device to set sample type on.
    \param  type            The type of samples requested.
    \retval MAPLE_EOK       On success.
    \retval MAPLE_EINVALID  If type is invalid.
    \retval MAPLE_EFAIL     If the microphone is sampling.
    \see    SIP_SAMPLE_16BIT_SIGNED
    \see    SIP_SAMPLE_8BIT_ULAW
*/
int sip_set_sample_type(maple_device_t *dev, unsigned int type);

/* Sampling frequencies. The SIP supports sampling at either 8kHz or 11.025 kHz.
   One of these values should be passed to the sip_set_frequency function. */
/** \brief  Record samples at 11.025kHz. */
#define SIP_SAMPLE_11KHZ 0x00

/** \brief  Record samples at 8kHz. */
#define SIP_SAMPLE_8KHZ  0x01

/** \brief  Set the sample frequency to be recorded by the microphone.

    This function sets the sample frequency that the microphone will record. The
    default value for this is about 11.025kHz samples. You must call this prior
    to sip_start_sampling() if you wish to change it from the default.

    \param  dev             The microphone device to set sample type on.
    \param  freq            The type of samples requested.
    \retval MAPLE_EOK       On success.
    \retval MAPLE_EINVALID  If freq is invalid.
    \retval MAPLE_EFAIL     If the microphone is sampling.
    \see    SIP_SAMPLE_11KHZ
    \see    SIP_SAMPLE_8KHZ
*/
int sip_set_frequency(maple_device_t *dev, unsigned int freq);

/** \brief  Start sampling on a microphone.

    This function informs a microphone it should start recording samples.

    \param  dev             The device to start sampling on.
    \param  block           Set to 1 to wait for the SIP to start sampling.
                            Otherwise check the is_sampling member of the status
                            for dev to know when it has started.
    \retval MAPLE_EOK       On success.
    \retval MAPLE_EAGAIN    If the command couldn't be sent, try again later.
    \retval MAPLE_EFAIL     If the microphone is already sampling.
    \retval MAPLE_ETIMEOUT  If the command timed out while blocking.
*/
int sip_start_sampling(maple_device_t *dev, int block);

/** \brief  Stop sampling on a microphone.

    This function informs a microphone it should stop recording samples.

    \param  dev             The device to stop sampling on.
    \param  block           Set to 1 to wait for the SIP to stop sampling.
                            Otherwise check the is_sampling member of the status
                            for dev to know when it has finished.
    \retval MAPLE_EOK       On success.
    \retval MAPLE_EAGAIN    If the command couldn't be sent, try again later.
    \retval MAPLE_EFAIL     If the microphone is not sampling.
    \retval MAPLE_ETIMEOUT  If the command timed out while blocking.
*/
int sip_stop_sampling(maple_device_t *dev, int block);

/** \brief  Retrieve the sample buffer from the microphone.

    This function retrieves the sample buffer from the microphone and allocates
    a new buffer for the microphone to record into. This function cannot be
    called while the microphone is sampling. The caller is responsible for the
    buffer returned, and must free the buffer when it is done with it.

    \param  dev             The device to fetch samples for.
    \param  sz              On return, the size of the sample buffer in bytes.
                            This must not be NULL.
    \return                 The sample buffer on success, NULL on failure.
*/
uint8 *sip_get_samples(maple_device_t *dev, size_t *sz);

/** \brief  Clear the sample buffer of a microphone.

    This function clears out any old samples on the microphone buffer so that
    recording will start from the beginning of the buffer again. This does not
    resize the buffer in any way. This function will not work if called while
    the microphone is sampling.

    \param  dev             The device to clear the buffer on.
    \retval MAPLE_EOK       On success.
    \retval MAPLE_EFAIL     If the device is currently sampling.
*/
int sip_clear_samples(maple_device_t *dev);

/* \cond */
/* Init / Shutdown */
int sip_init();
void sip_shutdown();
/* \endcond */

__END_DECLS

#endif  /* __DC_MAPLE_SIP_H */
