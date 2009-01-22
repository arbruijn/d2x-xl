//prototypes opengl functions - Added 9/15/99 Matthew Mueller
#ifndef _OGL_BITMAP_H
#define _OGL_BITMAP_H

#ifdef _WIN32
#include <windows.h>
#include <stddef.h>
#endif

#include "ogl_defs.h"
#include "ogl_lib.h"

int OglUBitBltCopy (int w,int h,int dx,int dy, int sx, int sy, CBitmap * src, CBitmap * dest);

//------------------------------------------------------------------------------

#endif
