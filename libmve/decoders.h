/*
 *
 * INTERNAL header - not to be included outside of libmve
 *
 */

#ifndef _DECODERS_H
#define _DECODERS_H

extern int g_width, g_height;
extern void *g_vBackBuf1, *g_vBackBuf2;

extern void decodeFrame8(ubyte *pFrame, ubyte *pMap, int mapRemain, ubyte *pData, int dataRemain);
extern void decodeFrame16(ubyte *pFrame, ubyte *pMap, int mapRemain, ubyte *pData, int dataRemain);

#endif // _DECODERS_H
