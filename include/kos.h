/* KallistiOS ##version##

   kos.h
   (c)2001 Dan Potter

*/

/** \file   kos.h
    \brief  Include everything KOS has to offer!

    This file includes pretty much every KOS-related header file, so you don't
    have to figure out what you actually need. The ultimate for the truly lazy!

    You may want to include individual header files yourself if you need more
    fine-grained control, as may be more appropriate for some projects.

    \author Dan Potter
*/

#ifndef __KOS_H
#define __KOS_H

/* The ultimate for the truly lazy: include and go! No more figuring out
   which headers to include for your project. */

#include <kos/cdefs.h>
__BEGIN_DECLS

#include <ctype.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <kos/fs.h>
#include <kos/fs_romdisk.h>
#include <kos/fs_ramdisk.h>
#include <kos/fs_pty.h>
#include <kos/limits.h>
#include <kos/thread.h>
#include <kos/sem.h>
#include <kos/rwsem.h>
#include <kos/once.h>
#include <kos/tls.h>
#include <kos/mutex.h>
#include <kos/cond.h>
#include <kos/genwait.h>
#include <kos/library.h>
#include <kos/net.h>
#include <kos/nmmgr.h>
#include <kos/exports.h>
#include <kos/dbgio.h>

#include <arch/arch.h>
#include <arch/cache.h>
#include <arch/irq.h>
#include <arch/spinlock.h>
#include <arch/timer.h>
#include <arch/types.h>
#include <arch/exec.h>
#include <arch/stack.h>

#ifdef _arch_dreamcast
#   include <arch/mmu.h>

#   include <dc/biosfont.h>
#   include <dc/cdrom.h>
#   include <dc/fs_dcload.h>
#   include <dc/fs_iso9660.h>
#   include <dc/fs_vmu.h>
#   include <dc/asic.h>
#   include <dc/maple.h>
#   include <dc/maple/controller.h>
#   include <dc/maple/keyboard.h>
#   include <dc/maple/mouse.h>
#   include <dc/maple/vmu.h>
#   include <dc/maple/sip.h>
#   include <dc/maple/purupuru.h>
#   include <dc/vmu_pkg.h>
#   include <dc/spu.h>
#   include <dc/pvr.h>
#   include <dc/video.h>
#   include <dc/fmath.h>
#   include <dc/matrix.h>
#   include <dc/sound/stream.h>
#   include <dc/sound/sfxmgr.h>
#   include <dc/net/broadband_adapter.h>
#   include <dc/net/lan_adapter.h>
#   include <dc/modem/modem.h>
#   include <dc/sq.h>
#   include <dc/ubc.h>
#   include <dc/flashrom.h>
#   include <dc/vblank.h>
#   include <dc/vmufs.h>
#   include <dc/scif.h>
#else   /* _arch_dreamcast */
#   error Invalid architecture or no architecture specified
#endif

__END_DECLS

#endif
