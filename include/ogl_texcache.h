//prototypes opengl functions - Added 9/15/99 Matthew Mueller
#ifndef _OGL_TEXCACHE_H
#define _OGL_TEXCACHE_H

#ifdef _WIN32
#	pragma pack(push)
#	pragma pack(8)
#	include <windows.h>
#	pragma pack(pop)
#	include <stddef.h>
#endif

void OglCleanTextureCache (void);
void OglCachePolyModelTextures (int32_t nModel);
int32_t OglCacheLevelTextures (void);

#endif
