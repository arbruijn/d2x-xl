//prototypes opengl functions - Added 9/15/99 Matthew Mueller
#ifndef _OGL_BITMAP_H
#define _OGL_BITMAP_H

#ifdef _WIN32
#include <windows.h>
#include <stddef.h>
#endif

#include "ogl_defs.h"
#include "ogl_lib.h"

int OglUBitMapMC (int x, int y, int dw, int dh, CBitmap *bm, tCanvasColor *c, int scale, int orient);
int OglUBitBltCopy (int w,int h,int dx,int dy, int sx, int sy, CBitmap * src, CBitmap * dest);

//------------------------------------------------------------------------------

static inline int OglUBitMapM (int x, int y, CBitmap *bmP)
{
return OglUBitMapMC (x, y, 0, 0, bmP, NULL, I2X (1), 0);
}

//------------------------------------------------------------------------------

#endif
