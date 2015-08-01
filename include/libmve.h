#ifndef _LIBMVE_H
#define _LIBMVE_H

#include "pstypes.h"

#define MVE_ERR_EOF 1

typedef struct{
	int32_t screenWidth;
	int32_t screenHeight;
	int32_t width;
	int32_t height;
	int32_t truecolor;
} MVE_videoSpec;

int32_t  MVE_rmPrepMovie(void *stream, int32_t x, int32_t y, int32_t track, int32_t bLittleEndian);
int32_t  MVE_rmStepMovie();
void MVE_rmHoldMovie();
void MVE_rmEndMovie();

void MVE_getVideoSpec(MVE_videoSpec *vSpec);

void MVE_sndInit(int32_t x);

typedef uint32_t (*mve_cb_Read)(void *stream,
                                    void *buffer,
                                    uint32_t count);

typedef void *(*mve_cb_Alloc)(uint32_t size);
typedef void (*mve_cb_Free)(void *ptr);

typedef void (*mve_cb_ShowFrame)(uint8_t *buffer,
                                 uint32_t bufw, uint32_t bufh,
                                 uint32_t sx, uint32_t sy,
                                 uint32_t w, uint32_t h,
                                 uint32_t dstx, uint32_t dsty);

typedef void (*mve_cb_SetPalette)(uint8_t *p,
                                  uint32_t start, uint32_t count);

void MVE_ioCallbacks(mve_cb_Read io_read);
void MVE_memCallbacks(mve_cb_Alloc mem_alloc, mve_cb_Free mem_free);
void MVE_sfCallbacks(mve_cb_ShowFrame showframe);
void MVE_palCallbacks(mve_cb_SetPalette setpalette);

#endif /* _LIBMVE_H */
