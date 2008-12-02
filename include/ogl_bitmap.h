//prototypes opengl functions - Added 9/15/99 Matthew Mueller
#ifndef _OGL_BITMAP_H
#define _OGL_BITMAP_H

#ifdef _WIN32
#include <windows.h>
#include <stddef.h>
#endif

#include "ogl_defs.h"
#include "ogl_lib.h"

int OglUBitMapMC (int x, int y, int dw, int dh, CBitmap *bm, grsColor *c, int scale, int orient);
int OglUBitBltI (int dw,int dh,int dx,int dy, int sw, int sh, int sx, int sy, 
					  CBitmap * src, CBitmap * dest, int bMipMaps, int bTransp, float fAlpha);
int OglUBitBltToLinear (int w,int h,int dx,int dy, int sx, int sy, CBitmap * src, CBitmap * dest);
int OglUBitBltCopy (int w,int h,int dx,int dy, int sx, int sy, CBitmap * src, CBitmap * dest);

//------------------------------------------------------------------------------

static inline int OglUBitBlt (int w,int h,int dx,int dy, int sx, int sy, 
										CBitmap *src, CBitmap *dest, int bTransp)
{
return OglUBitBltI (w, h, dx, dy, w, h, sx, sy, src, dest, 0, bTransp, 1.0f);
}

static inline int OglUBitMapM (int x, int y, CBitmap *bmP)
{
return OglUBitMapMC (x, y, 0, 0, bmP, NULL, F1_0, 0);
}

//------------------------------------------------------------------------------

#endif
