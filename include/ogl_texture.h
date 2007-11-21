//prototypes opengl functions - Added 9/15/99 Matthew Mueller
#ifndef _OGL_TEXTURE_H
#define _OGL_TEXTURE_H

#ifdef _MSC_VER
#include <windows.h>
#include <stddef.h>
#endif

//------------------------------------------------------------------------------

#define OGL_TEXTURE_LIST_SIZE 5000

#define OglActiveTexture(i,b)	if (b) glClientActiveTexture (i); else glActiveTexture (i)

#define OGL_BINDTEX(_handle)	glBindTexture (GL_TEXTURE_2D, _handle)

//------------------------------------------------------------------------------

typedef struct tOglTexture {
	GLuint	 	handle;
	GLint			internalformat;
	GLenum		format;
	int 			w,h,tw,th,lw;
	int 			bytesu;
	int 			bytes;
	GLfloat		u,v;
	GLfloat 		prio;
	int 			wrapstate;
	fix 			lastrend;
	unsigned		long numrend;
	ubyte			bMipMaps;
	ubyte			bFrameBuf;
#if RENDER2TEXTURE == 1
	tPixelBuffer	pbuffer;
#elif RENDER2TEXTURE == 2
	tFrameBuffer	fbuffer;
#endif
} tOglTexture;

//------------------------------------------------------------------------------

tOglTexture *OglGetFreeTexture (void);
void OglInitTexture (tOglTexture *t, int bMask);
void OglResetTextureStatsInternal (void);
void OglInitTextureListInternal (void);
void OglSmashTextureListInternal (void);
void OglVivifyTextureListInternal (void);

int OglTextureStats (void);

int OglBindBmTex (grsBitmap *bmP, int bMipMaps, int nTransp);
#if RENDER2TEXTURE == 1
int OglLoadBmTextureM (grsBitmap *bm, int bMipMap, int nTransp, int bMask, tPixelBuffer *pb);
#elif RENDER2TEXTURE == 2
int OglLoadBmTextureM (grsBitmap *bm, int bMipMap, int nTransp, int bMask, tFrameBuffer *pb);
#else
int OglLoadBmTextureM (grsBitmap *bm, int bMipMap, int nTransp, int bMask, void *pb);
#endif
int OglLoadBmTexture (grsBitmap *bm, int bMipMap, int nTransp, int bLoad);
int OglLoadTexture (grsBitmap *bmP, int dxo,int dyo, tOglTexture *tex, int nTransp, int bSuperTransp);
void OglFreeTexture (tOglTexture *glTexture);
void OglFreeBmTexture (grsBitmap *bm);
int OglSetupBmFrames (grsBitmap *bmP, int bDoMipMap, int nTransp, int bLoad);

tRgbColorf *BitmapColor (grsBitmap *bmP, ubyte *bufP);

void OglTexWrap (tOglTexture *tex, int state);

GLuint EmptyTexture (int Xsize, int Ysize);

//------------------------------------------------------------------------------

extern int nOglMemTarget;
extern tOglTexture oglTextureList [OGL_TEXTURE_LIST_SIZE];

//------------------------------------------------------------------------------

#endif
