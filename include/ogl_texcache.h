//prototypes opengl functions - Added 9/15/99 Matthew Mueller
#ifndef _OGL_TEXCACHE_H
#define _OGL_TEXCACHE_H

#ifdef _WIN32
#include <windows.h>
#include <stddef.h>
#endif

void OglCleanTextureCache (void);
void OglCachePolyModelTextures (int nModel);
int OglCacheLevelTextures (void);

#endif
