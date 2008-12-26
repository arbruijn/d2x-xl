/*
 *
 * INTERNAL header - not to be included outside of libmve
 *
 */

#ifndef _DECODERS_H
#define _DECODERS_H

#include "pstypes.h"

extern int g_width, g_height, g_currBuf;
extern void *g_vBackBuf1, *g_vBackBuf2;

extern void decodeFrame8(ubyte *frameP, ubyte *mapP, int mapRemain, ubyte *dataP, int dataRemain);
extern void decodeFrame16(ubyte *frameP, ubyte *mapP, int mapRemain, ubyte *dataP, int dataRemain);

#endif // _DECODERS_H
