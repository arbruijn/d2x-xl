//prototypes opengl functions - Added 9/15/99 Matthew Mueller
#ifndef _OGL_BITMAP_H
#define _OGL_BITMAP_H

#ifdef _WIN32
#	pragma pack(push)
#	pragma pack(8)
#	include <windows.h>
#	pragma pack(pop)
#	include <stddef.h>
#endif

#include "ogl_defs.h"
#include "ogl_lib.h"

int32_t OglUBitBltCopy (int32_t w,int32_t h,int32_t dx,int32_t dy, int32_t sx, int32_t sy, CBitmap * src, CBitmap * dest);

//------------------------------------------------------------------------------

#endif
