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

#include "ogl_init.h"
#include "lightmap.h"
#include "texmerge.h"
#include "error.h"

//------------------------------------------------------------------------------

#if RENDER2TEXTURE == 2

int bUseRender2Texture = 1;
int bRender2TextureOk = 0;

#ifdef _WIN32
PFNGLBINDRENDERBUFFEREXTPROC glBindRenderbufferEXT;
PFNGLISRENDERBUFFEREXTPROC glIsRenderbufferEXT;
PFNGLDELETERENDERBUFFERSEXTPROC glDeleteRenderbuffersEXT;
PFNGLGENRENDERBUFFERSEXTPROC glGenRenderbuffersEXT;
PFNGLRENDERBUFFERSTORAGEEXTPROC glRenderbufferStorageEXT;
PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC glGetRenderbufferParameterivEXT;
PFNGLISFRAMEBUFFEREXTPROC glIsFramebufferEXT;
PFNGLBINDFRAMEBUFFEREXTPROC glBindFramebufferEXT;
PFNGLDELETEFRAMEBUFFERSEXTPROC glDeleteFramebuffersEXT;
PFNGLGENFRAMEBUFFERSEXTPROC glGenFramebuffersEXT;
PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC glCheckFramebufferStatusEXT;
PFNGLFRAMEBUFFERTEXTURE1DEXTPROC glFramebufferTexture1DEXT;
PFNGLFRAMEBUFFERTEXTURE2DEXTPROC glFramebufferTexture2DEXT;
PFNGLFRAMEBUFFERTEXTURE3DEXTPROC glFramebufferTexture3DEXT;
PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC glFramebufferRenderbufferEXT;
PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC glGetFramebufferAttachmentParameterivEXT;
PFNGLGENERATEMIPMAPEXTPROC glGenerateMipmapEXT;
#	endif
#endif

//------------------------------------------------------------------------------

#if RENDER2TEXTURE == 2

int OglFBufferAvail (ogl_fbuffer *fb)
{
if (!bRender2TextureOk)
	return 0;
switch (fb->nStatus = glCheckFramebufferStatusEXT (GL_FRAMEBUFFER_EXT)) {                                          
	case GL_FRAMEBUFFER_COMPLETE_EXT:                       
		return 1;
	case GL_FRAMEBUFFER_UNSUPPORTED_EXT:                    
		return -1;
	default:                                                
		return 0;
	}
}

//------------------------------------------------------------------------------

int OglCreateFBuffer (ogl_fbuffer *fb, int nWidth, int nHeight, int bDepth)
{
if (!bRender2TextureOk)
	return 0;
glGenFramebuffersEXT (1, &fb->hBuf);
glGenTextures (1, &fb->texId);
glGenRenderbuffersEXT (1, &fb->hDepthRb);
glBindFramebufferEXT (GL_FRAMEBUFFER_EXT, fb->hBuf);
OGL_BINDTEX (fb->texId);
glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR /*GL_LINEAR_MIPMAP_LINEAR*/);
glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
if (nWidth > 0)
	fb->nWidth = nWidth;
if (nHeight > 0)
	fb->nHeight = nHeight;
#if 1
if (bDepth)
	glTexImage2D (GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, fb->nWidth, fb->nHeight, 0, GL_DEPTH_COMPONENT, GL_INT, NULL);
else
	glTexImage2D (GL_TEXTURE_2D, 0, GL_RGB8, fb->nWidth, fb->nHeight, 0, GL_RGB, GL_INT, NULL);
#else
glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA8, fb->nWidth, fb->nHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
#endif
glGenerateMipmapEXT (GL_TEXTURE_2D);
glFramebufferTexture2DEXT (GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, fb->texId, 0);
glBindRenderbufferEXT (GL_RENDERBUFFER_EXT, fb->hDepthRb);
glRenderbufferStorageEXT (GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, fb->nWidth, fb->nHeight);
glFramebufferRenderbufferEXT (GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, fb->hDepthRb);
if (OglFBufferAvail (fb) < 0)
	return fb->nStatus;
glFramebufferTexture2DEXT (GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, fb->texId, 0);
glFramebufferRenderbufferEXT (GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, fb->hDepthRb);
glBindFramebufferEXT (GL_FRAMEBUFFER_EXT, 0);
return 1;
}

//------------------------------------------------------------------------------

void OglDestroyFBuffer (ogl_fbuffer *fb)
{
if (!bRender2TextureOk)
	return;
if (fb->hBuf) {
	glDeleteFramebuffersEXT (1, &fb->hBuf);
	if (fb->texId)
		glDeleteTextures (1, &fb->texId);
	glDeleteRenderbuffersEXT (1, &fb->hDepthRb);
	fb->texId = 0;
	fb->hDepthRb = 0;
	fb->hBuf = 0;
	}
}

//------------------------------------------------------------------------------

int OglEnableFBuffer (ogl_fbuffer *fb)
{
if (!bRender2TextureOk)
	return 0;
OGL_BINDTEX (0);
glBindFramebufferEXT (GL_FRAMEBUFFER_EXT, fb->hBuf);
//glDrawBuffer (GL_FRONT);
//glReadBuffer (GL_FRONT);
return 1;
}

//------------------------------------------------------------------------------

int OglDisableFBuffer (ogl_fbuffer *fb)
{
if (!bRender2TextureOk)
	return 0;
glBindFramebufferEXT (GL_FRAMEBUFFER_EXT, 0);
OGL_BINDTEX (fb->texId);
//glDrawBuffer (GL_BACK);
//glReadBuffer (GL_FRONT);
return 1;
}

#endif

//------------------------------------------------------------------------------

void OglInitFBuffer (void)
{
#if RENDER2TEXTURE
if (bUseRender2Texture) {
#	ifdef _WIN32
	glBindRenderbufferEXT = (PFNGLBINDRENDERBUFFEREXTPROC) wglGetProcAddress ("glBindRenderbufferEXT");
	glIsRenderbufferEXT = (PFNGLISRENDERBUFFEREXTPROC) wglGetProcAddress ("glIsRenderbufferEXT");
	glDeleteRenderbuffersEXT = (PFNGLDELETERENDERBUFFERSEXTPROC) wglGetProcAddress ("glDeleteRenderbuffersEXT");
	glGenRenderbuffersEXT = (PFNGLGENRENDERBUFFERSEXTPROC) wglGetProcAddress ("glGenRenderbuffersEXT");
	glRenderbufferStorageEXT = (PFNGLRENDERBUFFERSTORAGEEXTPROC) wglGetProcAddress ("glRenderbufferStorageEXT");
	glGetRenderbufferParameterivEXT = (PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC) wglGetProcAddress ("glGetRenderbufferParameterivEXT");
	glIsFramebufferEXT = (PFNGLISFRAMEBUFFEREXTPROC) wglGetProcAddress ("glIsFramebufferEXT");
	glBindFramebufferEXT = (PFNGLBINDFRAMEBUFFEREXTPROC) wglGetProcAddress ("glBindFramebufferEXT");
	glDeleteFramebuffersEXT = (PFNGLDELETEFRAMEBUFFERSEXTPROC) wglGetProcAddress ("glDeleteFramebuffersEXT");
	glGenFramebuffersEXT = (PFNGLGENFRAMEBUFFERSEXTPROC) wglGetProcAddress ("glGenFramebuffersEXT");
	glCheckFramebufferStatusEXT = (PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC) wglGetProcAddress ("glCheckFramebufferStatusEXT");
	glFramebufferTexture1DEXT = (PFNGLFRAMEBUFFERTEXTURE1DEXTPROC) wglGetProcAddress ("glFramebufferTexture1DEXT");
	glFramebufferTexture2DEXT = (PFNGLFRAMEBUFFERTEXTURE2DEXTPROC) wglGetProcAddress ("glFramebufferTexture2DEXT");
	glFramebufferTexture3DEXT = (PFNGLFRAMEBUFFERTEXTURE3DEXTPROC) wglGetProcAddress ("glFramebufferTexture3DEXT");
	glFramebufferRenderbufferEXT = (PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC) wglGetProcAddress ("glFramebufferRenderbufferEXT");
	glGetFramebufferAttachmentParameterivEXT = (PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC) wglGetProcAddress ("glGetFramebufferAttachmentParameterivEXT");
	glGenerateMipmapEXT = (PFNGLGENERATEMIPMAPEXTPROC) wglGetProcAddress ("glGenerateMipmapEXT");

	bRender2TextureOk =
		(glBindRenderbufferEXT && glIsRenderbufferEXT && glDeleteRenderbuffersEXT &&
		 glGenRenderbuffersEXT && glRenderbufferStorageEXT && glGetRenderbufferParameterivEXT &&
		 glIsFramebufferEXT && glBindFramebufferEXT && glDeleteFramebuffersEXT &&
		 glGenFramebuffersEXT && glCheckFramebufferStatusEXT && glFramebufferTexture1DEXT &&
		 glFramebufferTexture2DEXT && glFramebufferTexture3DEXT && glFramebufferRenderbufferEXT &&
		 glGetFramebufferAttachmentParameterivEXT && glGenerateMipmapEXT) ?
		 2 : 0;
#	else
bRender2TextureOk = 2;
#	endif
	}
#endif
LogErr ((bRender2TextureOk == 2) ? "Rendering to texture is available\n" : "No rendering to texture available\n");
}

//------------------------------------------------------------------------------

