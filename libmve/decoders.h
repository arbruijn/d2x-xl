/*
 *
 * INTERNAL header - not to be included outside of libmve
 *
 */

#ifndef _DECODERS_H
#define _DECODERS_H

#include "pstypes.h"

extern int32_t g_width, g_height, g_currBuf;
extern void *g_vBackBuf1, *g_vBackBuf2;

extern void decodeFrame8(uint8_t *pFrame, uint8_t *mapP, int32_t mapRemain, uint8_t *pData, int32_t dataRemain);
extern void decodeFrame16(uint8_t *pFrame, uint8_t *mapP, int32_t mapRemain, uint8_t *pData, int32_t dataRemain);

#endif // _DECODERS_H
