//prototypes opengl functions - Added 9/15/99 Matthew Mueller
#ifndef _OGL_TEXTURE_H
#define _OGL_TEXTURE_H

#ifdef _WIN32
#include <windows.h>
#include <stddef.h>
#endif
#include "ogl_defs.h"

//------------------------------------------------------------------------------

#define OGL_TEXTURE_LIST_SIZE 5000

#define OglActiveTexture(i,b)	if (b) glClientActiveTexture (i); else glActiveTexture (i)

#define OGL_BINDTEX(_handle)	glBindTexture (GL_TEXTURE_2D, _handle)

//------------------------------------------------------------------------------

typedef struct tTextureInfo {
	GLuint	 		handle;
	GLint				internalformat;
	GLenum			format;
	int 				w, h, tw, th, lw;
	int 				bytesu;
	int 				bytes;
	GLfloat			u, v;
	GLfloat 			prio;
	int 				wrapstate;
	fix 				lastrend;
	unsigned	long	numrend;
	ubyte				bMipMaps;
	ubyte				bFrameBuffer;
#if RENDER2TEXTURE == 1
	CPBO				pbo;
#elif RENDER2TEXTURE == 2
	CFBO				fbo;
	tBitmap			*bmP;
#endif
} tTextureInfo;

//------------------------------------------------------------------------------

tTextureInfo *OglGetFreeTexture (CBitmap *bmP);
void OglInitTexture (tTextureInfo *t, int bMask, CBitmap *bmP);
void OglResetTextureStatsInternal (void);
void OglInitTextureListInternal (void);
void OglSmashTextureListInternal (void);
void OglVivifyTextureListInternal (void);

int OglTextureStats (void);

int OglBindBmTex (CBitmap *bmP, int bMipMaps, int nTransp);
#if RENDER2TEXTURE == 1
int OglLoadBmTextureM (CBitmap *bm, int bMipMap, int nTransp, int bMask, CPBO *pbo);
#elif RENDER2TEXTURE == 2
int OglLoadBmTextureM (CBitmap *bm, int bMipMap, int nTransp, int bMask, CFBO  *fbo);
#else
int OglLoadBmTextureM (CBitmap *bm, int bMipMap, int nTransp, int bMask, void *pb);
#endif
int OglLoadBmTexture (CBitmap *bm, int bMipMap, int nTransp, int bLoad);
int OglLoadTexture (CBitmap *bmP, int dxo,int dyo, tTextureInfo *tex, int nTransp, int bSuperTransp);
void OglFreeTexture (tTextureInfo *glTexture);
void OglFreeBmTexture (CBitmap *bm);
int OglSetupBmFrames (CBitmap *bmP, int bDoMipMap, int nTransp, int bLoad);

tRgbColorf *BitmapColor (CBitmap *bmP, ubyte *bufP);

void OglTexWrap (tTextureInfo *tex, int state);

GLuint EmptyTexture (int Xsize, int Ysize);

//------------------------------------------------------------------------------

extern int nOglMemTarget;
extern tTextureInfo oglTextureList [OGL_TEXTURE_LIST_SIZE];

//------------------------------------------------------------------------------

#endif
