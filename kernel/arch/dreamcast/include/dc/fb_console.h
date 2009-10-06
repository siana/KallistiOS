/* KallistiOS ##version##

   dc/fb_console.h
   Copyright (C) 2009 Lawrence Sebald

*/

#ifndef __DC_FB_CONSOLE_H
#define __DC_FB_CONSOLE_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <kos/dbgio.h>

extern dbgio_handler_t dbgio_fb;

/* Set up the target for the "framebuffer". This allows you to move it around
   so that, for example, you could have it render to a texture rather than
   to the full framebuffer. If you don't call this, then by default this
   dbgio interface prints to the full 640x480 framebuffer with a 32-pixel
   border. To restore this functionality after changing it, call this function
   with a NULL for t, and the appropriate parameters in w, h, borderx, and
   bordery. */
void dbgio_fb_set_target(uint16 *t, int w, int h, int borderx, int bordery);

__END_DECLS

#endif /* __DC_FB_CONSOLE_H */
