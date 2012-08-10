/* KallistiOS ##version##

   keyboard.c
   Copyright (C) 2002 Dan Potter
   Copyright (C) 2012 Lawrence Sebald
*/

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <dc/maple.h>
#include <dc/maple/keyboard.h>

/*

This module is an (almost) complete keyboard system. It handles key
debouncing and queueing so you don't miss any pressed keys as long
as you poll often enough. The only thing missing currently is key
repeat handling.

*/

/* Built-in keymaps. */
#define KBD_NUM_KEYMAPS 2
static kbd_keymap_t keymaps[KBD_NUM_KEYMAPS] = {
    {
        /* Japanese keyboard */
        {
            /* Base values */
            0, 0, 0, 0, 'a', 'b', 'c', 'd',                 /* 0x00 - 0x07 */
            'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l',         /* 0x08 - 0x0F */
            'm', 'n', 'o', 'p', 'q', 'r', 's', 't',         /* 0x10 - 0x17 */
            'u', 'v', 'w', 'x', 'y', 'z', '1', '2',         /* 0x18 - 0x1F */
            '3', '4', '5', '6', '7', '8', '9', '0',         /* 0x20 - 0x27 */
            10, 27, 8, 9, ' ', '-', '^', '@',               /* 0x28 - 0x2F */
            '[', 0, ']', ';', ':', 0, ',', '.',             /* 0x30 - 0x37 */
            '/', 0, 0, 0, 0, 0, 0, 0,                       /* 0x38 - 0x3F */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x40 - 0x47 */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x48 - 0x4F */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x50 - 0x57 */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x58 - 0x5F */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x60 - 0x67 */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x68 - 0x6F */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x70 - 0x77 */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x78 - 0x7F */
            0, 0, 0, 0, 0, 0, 0, '\\',                      /* 0x80 - 0x87 */
            0, 165, 0, 0                                    /* 0x88 - 0x8A */
            /* All the rest are unused, and will be 0. */
        },
        {
            /* Shifted values */
            0, 0, 0, 0, 'A', 'B', 'C', 'D',                 /* 0x00 - 0x07 */
            'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',         /* 0x08 - 0x0F */
            'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',         /* 0x10 - 0x17 */
            'U', 'V', 'W', 'X', 'Y', 'Z', '!', '"',         /* 0x18 - 0x1F */
            '#', '$', '%', '&', '\'', '(', ')', '~',        /* 0x20 - 0x27 */
            10, 27, 8, 9, ' ', '=', 175, '`',               /* 0x28 - 0x2F */
            '{', 0, '}', '+', '*', 0, '<', '>',             /* 0x30 - 0x37 */
            '?', 0, 0, 0, 0, 0, 0, 0,                       /* 0x38 - 0x3F */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x40 - 0x47 */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x48 - 0x4F */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x50 - 0x57 */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x58 - 0x5F */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x60 - 0x67 */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x68 - 0x6F */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x70 - 0x77 */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x78 - 0x7F */
            0, 0, 0, 0, 0, 0, 0, '_',                       /* 0x80 - 0x87 */
            0, '|', 0, 0                                    /* 0x88 - 0x8A */
            /* All the rest are unused, and will be 0. */
        }
    },
    {
        /* US/QWERTY keyboard */
        {
            0, 0, 0, 0, 'a', 'b', 'c', 'd',                 /* 0x00 - 0x07 */
            'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l',         /* 0x08 - 0x0F */
            'm', 'n', 'o', 'p', 'q', 'r', 's', 't',         /* 0x10 - 0x17 */
            'u', 'v', 'w', 'x', 'y', 'z', '1', '2',         /* 0x18 - 0x1F */
            '3', '4', '5', '6', '7', '8', '9', '0',         /* 0x20 - 0x27 */
            10, 27, 8, 9, ' ', '-', '=', '[',               /* 0x28 - 0x2F */
            ']', '\\', 0, ';', '\'', '`', ',', '.',         /* 0x30 - 0x37 */
            '/', 0, 0, 0, 0, 0, 0, 0,                       /* 0x38 - 0x3F */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x40 - 0x47 */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x48 - 0x4F */
            0, 0, 0, 0, '/', '*', '-', '+',                 /* 0x50 - 0x57 */
            13, '1', '2', '3', '4', '5', '6', '7',          /* 0x58 - 0x5F */
            '8', '9', '0', '.', 0, 0                        /* 0x60 - 0x65 */
            /* All the rest are unused, and will be 0. */
        },
        {
            0, 0, 0, 0, 'A', 'B', 'C', 'D',                 /* 0x00 - 0x07 */
            'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',         /* 0x08 - 0x0F */
            'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',         /* 0x10 - 0x17 */
            'U', 'V', 'W', 'X', 'Y', 'Z', '!', '@',         /* 0x18 - 0x1F */
            '#', '$', '%', '^', '&', '*', '(', ')',         /* 0x20 - 0x27 */
            10, 27, 8, 9, ' ', '_', '+', '{',               /* 0x28 - 0x2F */
            '}', '|', 0, ':', '"', '~', '<', '>',           /* 0x30 - 0x37 */
            '?', 0, 0, 0, 0, 0, 0, 0,                       /* 0x38 - 0x3F */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x40 - 0x47 */
            0, 0, 0, 0, 0, 0, 0, 0,                         /* 0x48 - 0x4F */
            0, 0, 0, 0, '/', '*', '-', '+',                 /* 0x50 - 0x57 */
            13, '1', '2', '3', '4', '5', '6', '7',          /* 0x58 - 0x5F */
            '8', '9', '0', '.', 0, 0                        /* 0x60 - 0x65 */
            /* All the rest are unused, and will be 0. */
        }
    }
};


/* The keyboard queue (global for now) */
static volatile int kbd_queue_active = 1;
static volatile int kbd_queue_tail = 0, kbd_queue_head = 0;
static volatile uint16  kbd_queue[KBD_QUEUE_SIZE];

/* Turn keyboard queueing on or off. This is mainly useful if you want
   to use the keys for a game where individual keypresses don't mean
   as much as having the keys up or down. Setting this state to
   a new value will clear the queue. */
void kbd_set_queue(int active) {
    if(kbd_queue_active != active) {
        kbd_queue_head = kbd_queue_tail = 0;
    }

    kbd_queue_active = active;
}

/* Take a key scancode, encode it appropriately, and place it on the
   keyboard queue. At the moment we assume no key overflows. */
static int kbd_enqueue(kbd_state_t *state, uint8 keycode, int mods) {
    static char keymap_noshift[] = {
        /*0*/   0, 0, 0, 0, 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i',
        'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
        'u', 'v', 'w', 'x', 'y', 'z',
        /*1e*/  '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
        /*28*/  13, 27, 8, 9, 32, '-', '=', '[', ']', '\\', 0, ';', '\'',
        /*35*/  '`', ',', '.', '/', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        /*46*/  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        /*53*/  0, '/', '*', '-', '+', 13, '1', '2', '3', '4', '5', '6',
        /*5f*/  '7', '8', '9', '0', '.', 0
    };
    static char keymap_shift[] = {
        /*0*/   0, 0, 0, 0, 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I',
        'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
        'U', 'V', 'W', 'X', 'Y', 'Z',
        /*1e*/  '!', '@', '#', '$', '%', '^', '&', '*', '(', ')',
        /*28*/  13, 27, 8, 9, 32, '_', '+', '{', '}', '|', 0, ':', '"',
        /*35*/  '~', '<', '>', '?', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        /*46*/  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        /*53*/  0, '/', '*', '-', '+', 13, '1', '2', '3', '4', '5', '6',
        /*5f*/  '7', '8', '9', '0', '.', 0
    };
    uint16 ascii = 0;

    /* Don't bother with bad keycodes. */
    if(keycode <= 1)
        return 0;

    /* Queue the key up on the device-specific queue. */
    if(state->queue_len < KBD_QUEUE_SIZE) {
        state->key_queue[state->queue_head] = keycode | (mods << 8);
        state->queue_head = (state->queue_head + 1) & (KBD_QUEUE_SIZE - 1);
        ++state->queue_len;
    }

    /* If queueing is turned off, don't bother with the global queue. */
    if(!kbd_queue_active)
        return 0;

    /* Figure out its key queue value */
    if(keycode <= 0x64) {
        if(state->shift_keys & (KBD_MOD_LSHIFT | KBD_MOD_RSHIFT))
            ascii = keymap_shift[keycode];
        else
            ascii = keymap_noshift[keycode];
    }

    if(ascii == 0)
        ascii = ((uint16)keycode) << 8;

    /* Ok... now do the enqueue to the global queue */
    kbd_queue[kbd_queue_head] = ascii;
    kbd_queue_head = (kbd_queue_head + 1) & (KBD_QUEUE_SIZE - 1);


    return 0;
}

/* Take a key off the key queue, or return -1 if there is none waiting */
int kbd_get_key() {
    int rv;

    /* If queueing isn't active, there won't be anything to get */
    if(!kbd_queue_active)
        return -1;

    /* Check available */
    if(kbd_queue_head == kbd_queue_tail)
        return -1;

    rv = kbd_queue[kbd_queue_tail];
    kbd_queue_tail = (kbd_queue_tail + 1) & (KBD_QUEUE_SIZE - 1);

    return rv;
}

/* Take a key off of a specific key queue. */
int kbd_queue_pop(maple_device_t *dev, int xlat) {
    kbd_state_t *state = (kbd_state_t *)dev->status;
    uint32 rv, mods;
    uint8 ascii;

    if(!state->queue_len)
        return -1;

    rv = state->key_queue[state->queue_tail];
    state->queue_tail = (state->queue_tail + 1) & (KBD_QUEUE_SIZE - 1);
    --state->queue_len;

    if(!xlat)
        return (int)rv;

    if(state->region < KBD_REGION_JP || state->region > KBD_NUM_KEYMAPS)
        return (int)(rv & 0xFF) << 8;

    mods = rv >> 8;

    if(mods & (KBD_MOD_LSHIFT | KBD_MOD_RSHIFT | (1 << 9)))
        ascii = keymaps[state->region - 1].shifted[(uint8)rv];
    else
        ascii = keymaps[state->region - 1].base[(uint8)rv];

    if(ascii)
        return (int)ascii;
    else
        return (int)((rv & 0xFF) << 8);
}

/* Update the keyboard status; this will handle debounce handling as well as
   queueing keypresses for later usage. The key press queue uses 16-bit
   words so that we can store "special" keys as such. This needs to be called
   fairly periodically if you're expecting keyboard input. */
static void kbd_check_poll(maple_frame_t *frm) {
    kbd_state_t *state;
    kbd_cond_t *cond;
    int i, p;
    int mods;

    state = (kbd_state_t *)frm->dev->status;
    cond = (kbd_cond_t *)&state->cond;

    /* Check the shift state */
    state->shift_keys = cond->modifiers;
    mods = cond->modifiers | (cond->leds << 8);

    /* Process all pressed keys */
    for(i = 0; i < 6; i++) {
        if(cond->keys[i] > 1) {
            p = state->matrix[cond->keys[i]];
            state->matrix[cond->keys[i]] = 2;   /* 2 == currently pressed */

            if(p == 0)
                kbd_enqueue(state, cond->keys[i], mods);
        }
    }

    /* Now normalize the key matrix */
    for(i = 0; i < 256; i++) {
        if(state->matrix[i] == 1)
            state->matrix[i] = 0;
        else if(state->matrix[i] == 2)
            state->matrix[i] = 1;
        else if(state->matrix[i] != 0) {
            assert_msg(0, "invalid key matrix array detected");
        }
    }
}

static void kbd_reply(maple_frame_t *frm) {
    maple_response_t *resp;
    uint32 *respbuf;
    kbd_state_t *state;
    kbd_cond_t *cond;

    /* Unlock the frame (it's ok, we're in an IRQ) */
    maple_frame_unlock(frm);

    /* Make sure we got a valid response */
    resp = (maple_response_t *)frm->recv_buf;

    if(resp->response != MAPLE_RESPONSE_DATATRF)
        return;

    respbuf = (uint32 *)resp->data;

    if(respbuf[0] != MAPLE_FUNC_KEYBOARD)
        return;

    /* Update the status area from the response */
    if(frm->dev) {
        state = (kbd_state_t *)frm->dev->status;
        cond = (kbd_cond_t *)&state->cond;
        memcpy(cond, respbuf + 1, (resp->data_len - 1) * 4);
        frm->dev->status_valid = 1;
        kbd_check_poll(frm);
    }
}

static int kbd_poll_intern(maple_device_t *dev) {
    uint32 * send_buf;

    if(maple_frame_lock(&dev->frame) < 0)
        return 0;

    maple_frame_init(&dev->frame);
    send_buf = (uint32 *)dev->frame.recv_buf;
    send_buf[0] = MAPLE_FUNC_KEYBOARD;
    dev->frame.cmd = MAPLE_COMMAND_GETCOND;
    dev->frame.dst_port = dev->port;
    dev->frame.dst_unit = dev->unit;
    dev->frame.length = 1;
    dev->frame.callback = kbd_reply;
    dev->frame.send_buf = send_buf;
    maple_queue_frame(&dev->frame);

    return 0;
}

static void kbd_periodic(maple_driver_t *drv) {
    maple_driver_foreach(drv, kbd_poll_intern);
}

static int kbd_attach(maple_driver_t *drv, maple_device_t *dev) {
    kbd_state_t *state = (kbd_state_t *)dev->status;
    uint32 f = dev->info.functions, tmp = MAPLE_FUNC_KEYBOARD;
    int d = 0;

    /* Figure out which function data we want to look at. This is borrowed from
       the maple_enum_type_ex function and isn't really pretty... */
    while(tmp != 0x80000000) {
        if(f & 0x80000000) {
            ++d;
        }
        
        f <<= 1;
        tmp <<= 1;
    }

    if(d > 2)
        /* Punt. */
        state->region = KBD_REGION_US;
    else
        state->region = dev->info.function_data[d] & 0xFF;

    /* Make sure all the queue variables are set up properly... */
    state->queue_tail = state->queue_head = state->queue_len = 0;

    return 0;
}

/* Device driver struct */
static maple_driver_t kbd_drv = {
    functions:  MAPLE_FUNC_KEYBOARD,
    name:       "Keyboard Driver",
    periodic:   kbd_periodic,
    attach:     kbd_attach,
    detach:     NULL
};

/* Add the keyboard to the driver chain */
int kbd_init() {
    return maple_driver_reg(&kbd_drv);
}

void kbd_shutdown() {
    maple_driver_unreg(&kbd_drv);
}
