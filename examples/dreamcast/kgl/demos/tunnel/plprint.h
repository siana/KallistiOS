/* KallistiOS ##version##

   plprint.h
   (c)2002 Paul Boese

*/

#ifndef __PLPRINT_H
#define __PLPRINT_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <kos.h>

void plinit();

void plprint(int x, int y, float r, float g, float b, const char *s, int fp);

__END_DECLS

#endif /* __PLPRINT_H */
