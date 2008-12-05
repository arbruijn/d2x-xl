#ifndef _LIBMVE_H
#define _LIBMVE_H

#define MVE_ERR_EOF 1

typedef struct{
	int screenWidth;
	int screenHeight;
	int width;
	int height;
	int truecolor;
} MVE_videoSpec;

int  MVE_rmPrepMovie(void *stream, int x, int y, int track, int bLittleEndian);
int  MVE_rmStepMovie();
void MVE_rmHoldMovie();
void MVE_rmEndMovie();

void MVE_getVideoSpec(MVE_videoSpec *vSpec);

void MVE_sndInit(int x);

typedef uint (*mve_cb_Read)(void *stream,
                                    void *buffer,
                                    uint count);

typedef void *(*mve_cb_Alloc)(uint size);
typedef void (*mve_cb_Free)(void *ptr);

typedef void (*mve_cb_ShowFrame)(ubyte *buffer,
                                 uint bufw, uint bufh,
                                 uint sx, uint sy,
                                 uint w, uint h,
                                 uint dstx, uint dsty);

typedef void (*mve_cb_SetPalette)(ubyte *p,
                                  uint start, uint count);

void MVE_ioCallbacks(mve_cb_Read io_read);
void MVE_memCallbacks(mve_cb_Alloc mem_alloc, mve_cb_Free mem_free);
void MVE_sfCallbacks(mve_cb_ShowFrame showframe);
void MVE_palCallbacks(mve_cb_SetPalette setpalette);

#endif /* _LIBMVE_H */
