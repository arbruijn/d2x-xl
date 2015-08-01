//prototypes opengl functions - Added 9/15/99 Matthew Mueller
#ifndef _OGL_HUDSTUFF_H
#define _OGL_HUDSTUFF_H

#ifdef _WIN32
#include <windows.h>
#include <stddef.h>
#endif

#include "descent.h"

void OglDrawMouseIndicator (void);
void OglDrawReticle (int32_t cross, int32_t primary, int32_t secondary);

#endif
