/* KallistiOS ##version##

   sip.c
   Copyright (C) 2005, 2008 Lawrence Sebald
*/

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arch/irq.h>
#include <kos/genwait.h>
#include <dc/maple.h>
#include <dc/maple/sip.h>

#define SIP_START_SAMPLING 0x80

static void sip_start_sampling_cb(maple_frame_t *frame) {
    sip_state_t *sip;
    maple_response_t *resp;

    /* Unlock the frame */
    maple_frame_unlock(frame);

    /* Make sure we got a valid response */
    resp = (maple_response_t *)frame->recv_buf;

    if(resp->response != MAPLE_RESPONSE_OK)
        return;

    /* Set the is_sampling flag. */
    sip = (sip_state_t *)frame->dev->status;
    sip->is_sampling = 1;

    /* Wake up! */
    genwait_wake_all(frame);
}

static void sip_stop_sampling_cb(maple_frame_t *frame) {
    sip_state_t *sip;
    maple_response_t *resp;

    /* Unlock the frame */
    maple_frame_unlock(frame);

    /* Make sure we got a valid response */
    resp = (maple_response_t *)frame->recv_buf;

    if(resp->response != MAPLE_RESPONSE_OK)
        return;

    /* Clear the is_sampling flag. */
    sip = (sip_state_t *)frame->dev->status;
    sip->is_sampling = 0;

    /* Wake up! */
    genwait_wake_all(frame);
}

int sip_set_gain(maple_device_t *dev, unsigned int g) {
    sip_state_t *sip;

    assert( dev != NULL );

    /* Check the gain value for validity */
    if(g > SIP_MAX_GAIN)
        return MAPLE_EINVALID;

    sip = (sip_state_t *)dev->status;
    sip->amp_gain = g;

    return MAPLE_EOK;
}

int sip_set_sample_type(maple_device_t *dev, unsigned int type) {
    sip_state_t *sip;

    assert( dev != NULL );

    /* Check the sample type value for validity. */
    if(type > SIP_SAMPLE_8BIT_ULAW)
        return MAPLE_EINVALID;

    sip = (sip_state_t *)dev->status;

    /* Make sure we aren't sampling already. */
    if(sip->is_sampling)
        return MAPLE_EFAIL;

    sip->sample_type = type;

    return MAPLE_EOK;
}

int sip_set_frequency(maple_device_t *dev, unsigned int freq) {
    sip_state_t *sip;

    assert( dev != NULL );

    /* Check the frequency value for validity. */
    if(freq > SIP_SAMPLE_8KHZ)
        return MAPLE_EINVALID;

    sip = (sip_state_t *)dev->status;

    /* Make sure we aren't sampling already. */
    if(sip->is_sampling)
        return MAPLE_EFAIL;

    sip->frequency = freq;

    return MAPLE_EOK;
}

int sip_start_sampling(maple_device_t *dev, int block) {
    sip_state_t *sip;
    uint32 *send_buf;

    assert( dev != NULL );

    sip = (sip_state_t *)dev->status;

    /* Make sure we aren't yet sampling */
    if(sip->is_sampling)
        return MAPLE_EFAIL;

    /* Lock the frame */
    if(maple_frame_lock(&dev->frame) < 0)
        return MAPLE_EAGAIN;

    /* Reset the frame */
    maple_frame_init(&dev->frame);
    send_buf = (uint32 *)dev->frame.recv_buf;
    send_buf[0] = MAPLE_FUNC_MICROPHONE;
    send_buf[1] = SIP_SUBCOMMAND_BASIC_CTRL |
                  (((sip->sample_type) | (sip->frequency << 2) |
                    SIP_START_SAMPLING) << 8);
    dev->frame.cmd = MAPLE_COMMAND_MICCONTROL;
    dev->frame.dst_port = dev->port;
    dev->frame.dst_unit = dev->unit;
    dev->frame.length = 2;
    dev->frame.callback = sip_start_sampling_cb;
    dev->frame.send_buf = send_buf;
    maple_queue_frame(&dev->frame);

    if(block) {
        /* Wait for the SIP to accept it */
        if(genwait_wait(&dev->frame, "sip_start_sampling", 500, NULL) < 0) {
            if(dev->frame.state != MAPLE_FRAME_VACANT) {
                /* Something went wrong.... */
                dev->frame.state = MAPLE_FRAME_VACANT;
                dbglog(DBG_ERROR, "sip_start_sampling: timeout to unit %c%c\n",
                       dev->port + 'A', dev->unit + '0');
                return MAPLE_ETIMEOUT;
            }
        }
    }

    return MAPLE_EOK;
}

int sip_stop_sampling(maple_device_t *dev, int block) {
    sip_state_t *sip;
    uint32 *send_buf;

    assert( dev != NULL );

    sip = (sip_state_t *)dev->status;

    /* Make sure we actually are sampling */
    if(!sip->is_sampling)
        return MAPLE_EFAIL;

    /* Lock the frame */
    if(maple_frame_lock(&dev->frame) < 0)
        return MAPLE_EAGAIN;

    /* Reset the frame */
    maple_frame_init(&dev->frame);
    send_buf = (uint32 *)dev->frame.recv_buf;
    send_buf[0] = MAPLE_FUNC_MICROPHONE;
    send_buf[1] = SIP_SUBCOMMAND_BASIC_CTRL;
    dev->frame.cmd = MAPLE_COMMAND_MICCONTROL;
    dev->frame.dst_port = dev->port;
    dev->frame.dst_unit = dev->unit;
    dev->frame.length = 2;
    dev->frame.callback = sip_stop_sampling_cb;
    dev->frame.send_buf = send_buf;
    maple_queue_frame(&dev->frame);

    if(block) {
        /* Wait for the SIP to accept it */
        if(genwait_wait(&dev->frame, "sip_stop_sampling", 500, NULL) < 0) {
            if(dev->frame.state != MAPLE_FRAME_VACANT) {
                /* Something went wrong.... */
                dev->frame.state = MAPLE_FRAME_VACANT;
                dbglog(DBG_ERROR, "sip_stop_sampling: timeout to unit %c%c\n",
                       dev->port + 'A', dev->unit + '0');
                return MAPLE_ETIMEOUT;
            }
        }
    }

    return MAPLE_EOK;
}

uint8 *sip_get_samples(maple_device_t *dev, size_t *sz) {
    sip_state_t *sip;
    uint8 *rv;
    uint32 old;

    assert( dev != NULL );
    assert( sz != NULL );

    /* Disable interrupts so that nothing changes underneath us. */
    old = irq_disable();

    sip = (sip_state_t *)dev->status;

    /* Make sure that we're not currently sampling. */
    if(sip->is_sampling) {
        irq_restore(old);
        *sz = (size_t)-1;
        return NULL;
    }

    /* Grab the values to return. */
    *sz = sip->buf_pos;
    rv = sip->samples_buf;

    /* Allocate us a new buffer. */
    sip->buf_pos = 0;
    sip->samples_buf = (uint8 *)malloc(11025 * 2 * 10);

    if(sip->samples_buf == NULL) {
        sip->buf_len = 0;
        dev->status_valid = 0;
    }
    else {
        sip->buf_len = 11025 * 2 * 10;
        dev->status_valid = 1;
    }

    irq_restore(old);
    return rv;
}

int sip_clear_samples(maple_device_t *dev) {
    sip_state_t *sip;
    uint32 old;

    assert( dev != NULL );

    /* Disable IRQs so that nothing changes under us */
    old = irq_disable();

    sip = (sip_state_t *)dev->status;

    if(sip->is_sampling) {
        irq_restore(old);
        return MAPLE_EFAIL;
    }

    sip->buf_pos = 0;

    irq_restore(old);

    return MAPLE_EOK;
}

static void sip_reply(maple_frame_t *frm) {
    maple_response_t *resp;
    uint32 *respbuf;
    size_t sz;
    sip_state_t *sip;
    void *tmp;

    /* Unlock the frame now (it's ok, we're in an IRQ) */
    maple_frame_unlock(frm);

    /* Make sure we got a valid response */
    resp = (maple_response_t *)frm->recv_buf;

    if(resp->response != MAPLE_RESPONSE_DATATRF)
        return;

    respbuf = (uint32 *)resp->data;

    if(respbuf[0] != MAPLE_FUNC_MICROPHONE)
        return;

    if(frm->dev) {
        sip = (sip_state_t *)frm->dev->status;
        frm->dev->status_valid = 1;

        if(sip->is_sampling) {
            sz = resp->data_len * 4 - 8;

            /* Resize the buffer, if it is needed. */
            if(sz + sip->buf_pos > sip->buf_len) {
                /* Attempt to double the buffer size. */
                tmp = realloc(sip->samples_buf, sip->buf_len << 1);

                if(!tmp) {
                    return;
                }

                sip->samples_buf = tmp;
                sip->buf_len <<= 1;
            }

            memcpy(sip->samples_buf + sip->buf_pos, resp->data + 8, sz);
            sip->buf_pos += sz;
        }
    }
}

static int sip_poll(maple_device_t *dev) {
    sip_state_t *sip;
    uint32 *send_buf;

    sip = (sip_state_t *)dev->status;

    /* Test to make sure that the particular mic is enabled */
    if(!sip->is_sampling) {
        return 0;
    }

    /* Lock the frame, or die trying */
    if(maple_frame_lock(&dev->frame) < 0)
        return 0;

    maple_frame_init(&dev->frame);
    send_buf = (uint32 *)dev->frame.recv_buf;
    send_buf[0] = MAPLE_FUNC_MICROPHONE;
    send_buf[1] = SIP_SUBCOMMAND_GET_SAMPLES |
                  (sip->amp_gain << 8);
    dev->frame.cmd = MAPLE_COMMAND_MICCONTROL;
    dev->frame.dst_port = dev->port;
    dev->frame.dst_unit = dev->unit;
    dev->frame.length = 2;
    dev->frame.callback = sip_reply;
    dev->frame.send_buf = send_buf;
    maple_queue_frame(&dev->frame);

    return 0;
}

static void sip_periodic(maple_driver_t *drv) {
    maple_driver_foreach(drv, sip_poll);
}

static int sip_attach(maple_driver_t *drv, maple_device_t *dev) {
    sip_state_t *sip;

    /* Allocate the sample buffer for 10 seconds worth of samples (11.025kHz,
       16-bit signed samples). */
    sip = (sip_state_t *)dev->status;
    sip->is_sampling = 0;
    sip->amp_gain = SIP_DEFAULT_GAIN;
    sip->buf_pos = 0;
    sip->samples_buf = (uint8 *)malloc(11025 * 2 * 10);

    if(sip->samples_buf == NULL) {
        dev->status_valid = 0;
        sip->buf_len = 0;
        return -1;
    }
    else {
        dev->status_valid = 1;
        sip->buf_len = 11025 * 2 * 10;
        return 0;
    }
}

static void sip_detach(maple_driver_t *drv, maple_device_t *dev) {
    sip_state_t *sip;

    sip = (sip_state_t *)dev->status;

    if(sip->samples_buf) {
        free(sip->samples_buf);
    }
}

/* Device Driver Struct */
static maple_driver_t sip_drv = {
    functions:	MAPLE_FUNC_MICROPHONE,
    name:		"Sound Input Peripheral",
    periodic:	sip_periodic,
    attach:		sip_attach,
    detach:		sip_detach
};

/* Add the SIP to the driver chain */
int sip_init() {
    return maple_driver_reg(&sip_drv);
}

void sip_shutdown() {
    maple_driver_unreg(&sip_drv);
}
