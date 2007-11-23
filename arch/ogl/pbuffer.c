/* $Id: ogl.c,v 1.14 2004/05/11 23:15:55 btb Exp $ */
/*
 *
 * Graphics support functions for OpenGL.
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef _WIN32
#	include <windows.h>
#	include <stddef.h>
#	include <io.h>
#endif
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <stdio.h>
#ifdef __macosx__
# include <stdlib.h>
# include <SDL/SDL.h>
#else
# include <malloc.h>
# include <SDL.h>
#endif

#include "3d.h"
#include "piggy.h"
#include "../../3d/globvars.h"
#include "error.h"
#include "texmap.h"
#include "palette.h"
#include "rle.h"
#include "mono.h"

#include "inferno.h"
#include "textures.h"
#include "texmerge.h"
#include "effects.h"
#include "weapon.h"
#include "powerup.h"
#include "polyobj.h"
#include "gamefont.h"
#include "byteswap.h"
#include "cameras.h"
#include "render.h"
#include "grdef.h"
#include "ogl_defs.h"
#include "lightmap.h"

//------------------------------------------------------------------------------

#if RENDER2TEXTURE == 1

HGLDC		hGlDC = 0;
HGLRC		hGlRC = 0;
#ifndef _WIN32
GLXDrawable hGlWindow = 0;
#endif


int gameStates.ogl.bUseRender2Texture = 1;
int gameStates.ogl.bRender2TextureOk = 0;

#	ifdef _WIN32
PFNWGLCREATEPBUFFERARBPROC				wglCreatePbufferARB = NULL;
PFNWGLGETPBUFFERDCARBPROC				wglGetPbufferDCARB = NULL;
PFNWGLRELEASEPBUFFERDCARBPROC			wglReleasePbufferDCARB = NULL;
PFNWGLDESTROYPBUFFERARBPROC			wglDestroyPbufferARB = NULL;
PFNWGLQUERYPBUFFERARBPROC				wglQueryPbufferARB = NULL;
PFNWGLGETPIXELFORMATATTRIBIVARBPROC	wglGetPixelFormatAttribivARB = NULL;
PFNWGLGETPIXELFORMATATTRIBFVARBPROC wglGetPixelFormatAttribfvARB = NULL;
PFNWGLCHOOSEPIXELFORMATARBPROC		wglChoosePixelFormatARB = NULL;
PFNWGLBINDTEXIMAGEARBPROC				wglBindTexImageARB = NULL;
PFNWGLRELEASETEXIMAGEARBPROC			wglReleaseTexImageARB = NULL;
PFNWGLSETPBUFFERATTRIBARBPROC			wglSetPbufferAttribARB = NULL;
PFNWGLMAKECONTEXTCURRENTARBPROC		wglMakeContextCurrentARB = NULL;
PFNWGLGETCURRENTREADDCARBPROC			wglGetCurrentReadDCARB = NULL;
#	endif
#endif

//------------------------------------------------------------------------------

#if RENDER2TEXTURE == 1

int OglCreatePBuffer (tPixelBuffer *pb, int nWidth, int nHeight, int nDepth)
{
#ifdef _WIN32
	int	pf;
	UINT	nPf;
	
	static int pfAttribs [] = {
		WGL_SUPPORT_OPENGL_ARB, TRUE,
		WGL_DRAW_TO_PBUFFER_ARB, TRUE,
		WGL_BIND_TO_TEXTURE_RGBA_ARB, TRUE,
		WGL_RED_BITS_ARB, 8,
		WGL_GREEN_BITS_ARB, 8,
		WGL_BLUE_BITS_ARB, 8,
		WGL_ALPHA_BITS_ARB, 8,
		WGL_DEPTH_BITS_ARB, 24,
		//WGL_STENCIL_BITS_ARB, 1,
		WGL_DOUBLE_BUFFER_ARB, FALSE,
		0};

	static int pbAttribs [] = {
		WGL_TEXTURE_FORMAT_ARB,
		WGL_TEXTURE_RGBA_ARB,
		WGL_TEXTURE_TARGET_ARB,
		WGL_TEXTURE_2D_ARB,
		0};

if (!gameStates.ogl.bRender2TextureOk)
	return 0;
OglDestroyPBuffer (pb);
hGlDC = wglGetCurrentDC ();
hGlRC = wglGetCurrentContext ();
if (nWidth > 0)
	pb->nWidth = nWidth;
if (nHeight > 0)
	pb->nHeight = nHeight;
wglChoosePixelFormatARB (hGlDC, (const int *) pfAttribs, NULL, 1, &pf, &nPf);
if (!nPf)
	return 0;
if (!(pb->hBuf = wglCreatePbufferARB (hGlDC, pf, pb->nWidth, pb->nHeight, pbAttribs)))
	return 0;
if (!(pb->hDC = wglGetPbufferDCARB (pb->hBuf))) {
	wglDestroyPbufferARB (pb->hBuf);
	pb->hBuf = NULL;
	return 0;
	}
if (!(pb->hRC = wglCreateContext (pb->hDC))) {
	wglReleasePbufferDCARB (pb->hBuf, pb->hDC);
	wglDestroyPbufferARB (pb->hBuf);
	pb->hDC = NULL;
	pb->hBuf = NULL;
	return 0;
	}
wglShareLists (hGlRC, pb->hRC);
#else //!_WIN32
	XVisualInfo *vi;
	GLXFBConfig *pfd;	// pixel format descriptor
	int			nScreen, nFd;

	static int pfAttribs [] = {
		GLX_DOUBLEBUFFER, 0,
		GLX_RED_SIZE, 8,
		GLX_GREEN_SIZE, 8,
		GLX_BLUE_SIZE, 8,
		GLX_DEPTH_SIZE, 24,
		GLX_RENDER_TYPE, GLX_RGBA_BIT,
		GLX_DRAWABLE_TYPE, GLX_PBUFFER_BIT | GLX_WINDOW_BIT,
		0
		};

    int pbAttribs [] = {
        GLX_PBUFFER_WIDTH, 0,
        GLX_PBUFFER_HEIGHT, 0,
        GLX_LARGEST_PBUFFER, 0,
        None
    };

if (!gameStates.ogl.bRender2TextureOk)
	return 0;
hGlWindow = glXGetCurrentDrawable ();
hGlDC = glXGetCurrentDisplay ();
hGlRC = glXGetCurrentContext ();
nScreen = DefaultScreen (hGlDC);
pfd = glXChooseFBConfig (hGlDC, nScreen, pfAttribs, &nFd);
if (nWidth > 0)
	pb->nWidth = nWidth;
if (nHeight > 0)
	pb->nHeight = nHeight;
pbAttribs [1] = nWidth;
pbAttribs [3] = nHeight;
if (!(pb->hBuf = glXCreatePbuffer (hGlDC, pfd [0], pbAttribs))) {
	XFree (pfd);
	return 0;
	}
if (!(vi = glXGetVisualFromFBConfig (hGlDC, pfd [0]))) {
	glXDestroyPbuffer (pb->hDC, pb->hBuf);
	XFree (pfd);
	return 0;
	}
if (!(pb->hRC = glXCreateContext (hGlDC, vi, hGlRC, GL_TRUE))) {// Share display lists and textures with the regular window
	glXDestroyPbuffer (pb->hDC, pb->hBuf);
	XFree (pfd);
	XFree (vi);
	return 0;
	}
XFree (pfd);
#endif //!_WIN32
glGenTextures (1, &pb->texId);
OGL_BINDTEX (pb->texId);
glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR /*GL_LINEAR_MIPMAP_LINEAR*/);
glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
return 1;
}

//------------------------------------------------------------------------------

void OglDestroyPBuffer (tPixelBuffer *pb)
{
if (pb->hBuf) {
#ifdef _WIN32
	wglReleaseTexImageARB (pb->hBuf, WGL_FRONT_LEFT_ARB);
	if (pb->hRC)
		wglDeleteContext (pb->hRC);
	if (pb->hDC)
		wglReleasePbufferDCARB (pb->hBuf, pb->hDC);
	wglDestroyPbufferARB (pb->hBuf);
#else //!_WIN32
	if (pb->hRC)
		glXDestroyContext (pb->hDC, pb->hRC);
	if (pb->hBuf)
		glXDestroyPbuffer (pb->hDC, pb->hBuf);
#endif //!_WIN32
	glDeleteTextures (1, &pb->texId);
	pb->hRC = NULL;
	pb->hDC = NULL;
	pb->hBuf = NULL;
	}
}

//------------------------------------------------------------------------------

int OglPBufferAvail (tPixelBuffer *pb)
{
#ifdef _WIN32
	int	bLost;

if (!wglQueryPbufferARB)
	return 0;	// pixel buffer extension not available
if (!(pb->hDC && pb->hDC && pb->hRC))
	return 0;	// pixel buffer not available
wglQueryPbufferARB (pb->hBuf, WGL_PBUFFER_LOST_ARB, &bLost);
return (bLost && !OglCreatePBuffer (pb, 0, 0)) ? -1 : 1;
#else
return (pb->hBuf && pb->hRC);
#endif
}

//------------------------------------------------------------------------------

int OglEnablePBuffer (tPixelBuffer *pb)
{
if (OglPBufferAvail (pb) < 1)
	return 0;
#ifdef _WIN32
if (!wglMakeContextCurrentARB (pb->hDC, pb->hDC, pb->hRC))
#else
if (!glXMakeCurrent (pb->hDC, pb->hBuf, pb->hRC))
#endif
	return 0;
OglDrawBuffer (GL_FRONT, 1);
OglReadBuffer (GL_FRONT, 1);
return 1;
}

//------------------------------------------------------------------------------

int OglDisablePBuffer (tPixelBuffer *pb)
{
if (OglPBufferAvail (pb) < 1)
	return 0;
#ifdef _WIN32
if (!wglMakeContextCurrentARB (hGlDC, hGlDC, hGlRC))
#else
if (!glXMakeCurrent (hGlDC, hGlWindow, hGlRC))
#endif
	return 0;
OglDrawBuffer (GL_BACK, 1);
OglReadBuffer (GL_FRONT, 1);
return 1;
}

#endif

//------------------------------------------------------------------------------

void OglInitPBuffer (void)
{
#if RENDER2TEXTURE == 1
if (gameStates.ogl.bUseRender2Texture) {
#	ifdef _WIN32
	wglCreatePbufferARB = (PFNWGLCREATEPBUFFERARBPROC) wglGetProcAddress ("wglCreatePbufferARB");
	wglGetPbufferDCARB = (PFNWGLGETPBUFFERDCARBPROC) wglGetProcAddress ("wglGetPbufferDCARB");
	wglReleasePbufferDCARB = (PFNWGLRELEASEPBUFFERDCARBPROC) wglGetProcAddress ("wglReleasePbufferDCARB");
	wglDestroyPbufferARB = (PFNWGLDESTROYPBUFFERARBPROC) wglGetProcAddress ("wglDestroyPbufferARB");
	wglQueryPbufferARB = (PFNWGLQUERYPBUFFERARBPROC) wglGetProcAddress ("wglQueryPbufferARB");
	wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC) wglGetProcAddress ("wglChoosePixelFormatARB");
	wglMakeContextCurrentARB = (PFNWGLMAKECONTEXTCURRENTARBPROC) wglGetProcAddress ("wglMakeContextCurrentARB");
	wglBindTexImageARB = (PFNWGLBINDTEXIMAGEARBPROC) wglGetProcAddress ("wglBindTexImageARB");
	wglReleaseTexImageARB = (PFNWGLRELEASETEXIMAGEARBPROC) wglGetProcAddress ("wglReleaseTexImageARB");
	gameStates.ogl.bRender2TextureOk =
		wglCreatePbufferARB && wglGetPbufferDCARB && wglReleasePbufferDCARB && wglDestroyPbufferARB && 
		wglQueryPbufferARB && wglChoosePixelFormatARB && wglMakeContextCurrentARB &&
		wglBindTexImageARB && wglReleaseTexImageARB;
#	else
	gameStates.ogl.bRender2TextureOk = 1;
#endif
	}

#ifdef _WIN32
  hGlDC = wglGetCurrentDC ();
  hGlRC = wglGetCurrentContext ();
#else
  hGlWindow = glXGetCurrentDrawable ();
  hGlDC = glXGetCurrentDisplay ();
  hGlRC = glXGetCurrentContext ();
#endif
  
#endif
LogErr ((gameStates.ogl.bRender2TextureOk == 1) ? "Rendering to pixel buffers is available\n" : "No rendering to pixel buffers available\n");

}

//------------------------------------------------------------------------------

