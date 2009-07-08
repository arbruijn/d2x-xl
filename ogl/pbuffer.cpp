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

#include "ogl_defs.h"
#include "ogl_lib.h"
#include "lightmap.h"
#include "texmerge.h"
#include "error.h"

//------------------------------------------------------------------------------

#if RENDER2TEXTURE == 1

HGLDC		hGlDC = 0;
HGLRC		hGlRC = 0;
#ifndef _WIN32
GLXDrawable hGlWindow = 0;
#endif

#ifdef _WIN32
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
#endif

//------------------------------------------------------------------------------

void CPBO::Init (void)
{
memset (&m_info, 0, sizeof (m_info));
}

//------------------------------------------------------------------------------

int CPBO::Create (int nWidth, int nHeight)
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

if (!ogl.m_states.bRender2TextureOk)
	return 0;
Destroy ();
hGlDC = wglGetCurrentDC ();
hGlRC = wglGetCurrentContext ();
if (nWidth > 0)
	m_info.nWidth = nWidth;
if (nHeight > 0)
	m_info.nHeight = nHeight;
wglChoosePixelFormatARB (hGlDC, reinterpret_cast<const int*> (pfAttribs), NULL, 1, &pf, &nPf);
if (!nPf)
	return 0;
if (!(m_info.hBuf = wglCreatePbufferARB (hGlDC, pf, m_info.nWidth, m_info.nHeight, pbAttribs)))
	return 0;
if (!(m_info.hDC = wglGetPbufferDCARB (m_info.hBuf))) {
	wglDestroyPbufferARB (m_info.hBuf);
	m_info.hBuf = NULL;
	return 0;
	}
if (!(m_info.hRC = wglCreateContext (m_info.hDC))) {
	wglReleasePbufferDCARB (m_info.hBuf, m_info.hDC);
	wglDestroyPbufferARB (m_info.hBuf);
	m_info.hDC = NULL;
	m_info.hBuf = NULL;
	return 0;
	}
wglShareLists (hGlRC, m_info.hRC);
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

if (!ogl.m_states.bRender2TextureOk)
	return 0;
hGlWindow = glXGetCurrentDrawable ();
hGlDC = glXGetCurrentDisplay ();
hGlRC = glXGetCurrentContext ();
nScreen = DefaultScreen (hGlDC);
pfd = glXChooseFBConfig (hGlDC, nScreen, pfAttribs, &nFd);
if (nWidth > 0)
	m_info.nWidth = nWidth;
if (nHeight > 0)
	m_info.nHeight = nHeight;
pbAttribs [1] = nWidth;
pbAttribs [3] = nHeight;
if (!(m_info.hBuf = glXCreatePbuffer (hGlDC, pfd [0], pbAttribs))) {
	XFree (pfd);
	return 0;
	}
if (!(vi = glXGetVisualFromFBConfig (hGlDC, pfd [0]))) {
	glXDestroyPbuffer (m_info.hDC, m_info.hBuf);
	XFree (pfd);
	return 0;
	}
if (!(m_info.hRC = glXCreateContext (hGlDC, vi, hGlRC, GL_TRUE))) {// Share display lists and textures with the regular window
	glXDestroyPbuffer (m_info.hDC, m_info.hBuf);
	XFree (pfd);
	XFree (vi);
	return 0;
	}
XFree (pfd);
#endif //!_WIN32
glGenTextures (1, &m_info.texId);
OGL_BINDTEX (m_info.texId);
glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR /*GL_LINEAR_MIPMAP_LINEAR*/);
glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
return 1;
}

//------------------------------------------------------------------------------

void CPBO::Destroy (void)
{
if (m_info.hBuf) {
#ifdef _WIN32
	Release ();
	if (m_info.hRC)
		wglDeleteContext (m_info.hRC);
	if (m_info.hDC)
		wglReleasePbufferDCARB (m_info.hBuf, m_info.hDC);
	wglDestroyPbufferARB (m_info.hBuf);
#else //!_WIN32
	if (m_info.hRC)
		glXDestroyContext (m_info.hDC, m_info.hRC);
	if (m_info.hBuf)
		glXDestroyPbuffer (m_info.hDC, m_info.hBuf);
#endif //!_WIN32
	glDeleteTextures (1, &m_info.texId);
	m_info.hRC = NULL;
	m_info.hDC = NULL;
	m_info.hBuf = NULL;
	}
}

//------------------------------------------------------------------------------

int CPBO::Available (void)
{
#ifdef _WIN32
	int	bLost;

if (!wglQueryPbufferARB)
	return 0;	// pixel buffer extension not available
if (!(m_info.hDC && m_info.hDC && m_info.hRC))
	return 0;	// pixel buffer not available
wglQueryPbufferARB (m_info.hBuf, WGL_PBUFFER_LOST_ARB, &bLost);
return (bLost && !Create (0, 0)) ? -1 : 1;
#else
return (m_info.hBuf && m_info.hRC);
#endif
}

//------------------------------------------------------------------------------

int CPBO::Enable (void)
{
if (Available () < 1)
	return 0;
#ifdef _WIN32
if (!wglMakeContextCurrentARB (m_info.hDC, m_info.hDC, m_info.hRC))
#else
if (!glXMakeCurrent (m_info.hDC, m_info.hBuf, m_info.hRC))
#endif
	return 0;
OglSetDrawBuffer (GL_FRONT, 1);
OglSetReadBuffer (GL_FRONT, 1);
return 1;
}

//------------------------------------------------------------------------------

int CPBO::Disable (void)
{
if (Available () < 1)
	return 0;
#ifdef _WIN32
if (!wglMakeContextCurrentARB (hGlDC, hGlDC, hGlRC))
#else
if (!glXMakeCurrent (hGlDC, hGlWindow, hGlRC))
#endif
	return 0;
OglSetDrawBuffer (GL_BACK, 1);
OglSetReadBuffer (GL_FRONT, 1);
return 1;
}

//------------------------------------------------------------------------------

bool CPBO::Bind (void)
{
#ifdef _WIN32
if (!Handle ())
	return false;
if (m_info.bBound)
	return true;
return m_info.bBound = (Handle () > 0) && wglBindTexImageARB (Handle (), WGL_FRONT_LEFT_ARB);
#else
return false;
#endif
}

//------------------------------------------------------------------------------

void CPBO::Release (void)
{
#ifdef _WIN32
if (Handle () > 0) 
	wglReleaseTexImageARB (Handle (), WGL_FRONT_LEFT_ARB);
m_info.bBound = false;
#endif
}

//------------------------------------------------------------------------------

void CPBO::Setup (void)
{
ogl.m_states.bUseRender2Texture = 1;
ogl.m_states.bRender2TextureOk = 0;
if (ogl.m_states.bUseRender2Texture) {
#ifdef _WIN32
	wglCreatePbufferARB = (PFNWGLCREATEPBUFFERARBPROC) wglGetProcAddress ("wglCreatePbufferARB");
	wglGetPbufferDCARB = (PFNWGLGETPBUFFERDCARBPROC) wglGetProcAddress ("wglGetPbufferDCARB");
	wglReleasePbufferDCARB = (PFNWGLRELEASEPBUFFERDCARBPROC) wglGetProcAddress ("wglReleasePbufferDCARB");
	wglDestroyPbufferARB = (PFNWGLDESTROYPBUFFERARBPROC) wglGetProcAddress ("wglDestroyPbufferARB");
	wglQueryPbufferARB = (PFNWGLQUERYPBUFFERARBPROC) wglGetProcAddress ("wglQueryPbufferARB");
	wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC) wglGetProcAddress ("wglChoosePixelFormatARB");
	wglMakeContextCurrentARB = (PFNWGLMAKECONTEXTCURRENTARBPROC) wglGetProcAddress ("wglMakeContextCurrentARB");
	wglBindTexImageARB = (PFNWGLBINDTEXIMAGEARBPROC) wglGetProcAddress ("wglBindTexImageARB");
	wglReleaseTexImageARB = (PFNWGLRELEASETEXIMAGEARBPROC) wglGetProcAddress ("wglReleaseTexImageARB");
	ogl.m_states.bRender2TextureOk =
		wglCreatePbufferARB && wglGetPbufferDCARB && wglReleasePbufferDCARB && wglDestroyPbufferARB && 
		wglQueryPbufferARB && wglChoosePixelFormatARB && wglMakeContextCurrentARB &&
		wglBindTexImageARB && wglReleaseTexImageARB;
#else
	ogl.m_states.bRender2TextureOk = 1;
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
  
PrintLog ((ogl.m_states.bRender2TextureOk == 1)  
		  ? "Rendering to pixel buffers is available\n" 
		  : "No rendering to pixel buffers available\n");

}

#endif

//------------------------------------------------------------------------------

