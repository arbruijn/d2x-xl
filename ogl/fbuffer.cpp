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

#define FBO_STENCIL_BUFFER	0

//------------------------------------------------------------------------------

#if RENDER2TEXTURE == 2

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

int OglFBufferAvail (tFrameBuffer *fb)
{
if (!gameStates.ogl.bRender2TextureOk)
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

int OglCreateFBuffer (tFrameBuffer *fb, int nWidth, int nHeight, int nType)
{
	GLenum	nError;

if (!gameStates.ogl.bRender2TextureOk)
	return 0;
OglDestroyFBuffer (fb);
glGenFramebuffersEXT (1, &fb->hFBO);
glBindFramebufferEXT (GL_FRAMEBUFFER_EXT, fb->hFBO);

if (nWidth > 0)
	fb->nWidth = nWidth;
if (nHeight > 0)
	fb->nHeight = nHeight;
OglGenTextures (1, &fb->hRenderBuffer);
if (nType == 2) { //GPGPU
	glBindTexture (GL_TEXTURE_2D, fb->hRenderBuffer);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE); 
	glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA32F_ARB, fb->nWidth, fb->nHeight, 0, GL_RGBA, GL_FLOAT, NULL);
	glFramebufferTexture2DEXT (GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, fb->hRenderBuffer, 0);
	fb->hDepthBuffer = 0;
	}
else {
	glBindTexture (GL_TEXTURE_2D, fb->hRenderBuffer);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); //LINEAR /*GL_LINEAR_MIPMAP_LINEAR*/);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); //LINEAR);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
#if 0
	if (nType == 1) //depth texture
		glTexImage2D (GL_TEXTURE_2D, 0, 1, grdCurScreen->scWidth, grdCurScreen->scHeight, 0, GL_DEPTH_COMPONENT, GL_INT, NULL);
	else 
#endif
		{
		glTexImage2D (GL_TEXTURE_2D, 0, 3, fb->nWidth, fb->nHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
		glGenerateMipmapEXT (GL_TEXTURE_2D);
		}
	glFramebufferTexture2DEXT (GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, fb->hRenderBuffer, 0);
#if FBO_STENCIL_BUFFER
	if ((nType == 1) && (fb->hDepthBuffer = OglCreateDepthTexture (0, 1))) {
		glFramebufferTexture2DEXT (GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, fb->hDepthBuffer, 0);
		glGenRenderbuffersEXT (1, &fb->hStencilBuffer);
		glBindRenderbufferEXT (GL_RENDERBUFFER_EXT, fb->hStencilBuffer);
		glRenderbufferStorageEXT (GL_RENDERBUFFER_EXT, GL_STENCIL_INDEX8_EXT, fb->nWidth, fb->nHeight);
		glFramebufferRenderbufferEXT (GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, fb->hStencilBuffer);
		if (OglFBufferAvail (fb) < 0)
			return 0;
		}
	else 
#else
	fb->hStencilBuffer = 0;
#endif
		{
		glGenRenderbuffersEXT (1, &fb->hDepthBuffer);
		glBindRenderbufferEXT (GL_RENDERBUFFER_EXT, fb->hDepthBuffer);
		glRenderbufferStorageEXT (GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, fb->nWidth, fb->nHeight);
		glFramebufferRenderbufferEXT (GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, fb->hDepthBuffer);
		if ((nError = glGetError ()) == GL_OUT_OF_MEMORY)
			return 0;
		}
	if ((nError = glGetError ()) == GL_OUT_OF_MEMORY)
		return 0;
	if (OglFBufferAvail (fb) < 0)
		return 0;
	}
fb->nType = nType;
glBindFramebufferEXT (GL_FRAMEBUFFER_EXT, 0);
return 1;
}

//------------------------------------------------------------------------------

void OglDestroyFBuffer (tFrameBuffer *fb)
{
if (!gameStates.ogl.bRender2TextureOk)
	return;
if (fb->hFBO) {
	if (fb->hRenderBuffer) {
		OglDeleteTextures (1, &fb->hRenderBuffer);
		fb->hRenderBuffer = 0;
		}
	if (fb->hDepthBuffer) {
#if 1
		if (fb->nType == 1) {
			OglDeleteTextures (1, &fb->hDepthBuffer);
#if FBO_STENCIL_BUFFER
			glDeleteRenderbuffersEXT (1, &fb->hStencilBuffer);
			fb->hStencilBuffer = 0;
#endif
			}
		else
#endif
			glDeleteRenderbuffersEXT (1, &fb->hDepthBuffer);
		fb->hDepthBuffer = 0;
		}
	glDeleteFramebuffersEXT (1, &fb->hFBO);
	fb->hFBO = 0;
	}
}

//------------------------------------------------------------------------------

int OglEnableFBuffer (tFrameBuffer *fb)
{
if (!gameStates.ogl.bRender2TextureOk)
	return 0;
glDrawBuffer (GL_BACK);
glBindFramebufferEXT (GL_FRAMEBUFFER_EXT, 0);
gameStates.ogl.bDrawBufferActive = 0;
glBindTexture (GL_TEXTURE_2D, 0);
glBindFramebufferEXT (GL_FRAMEBUFFER_EXT, fb->hFBO);
#if 1//def _DEBUG
if (!OglFBufferAvail (fb))
	return 0;
#endif
OglDrawBuffer (GL_COLOR_ATTACHMENT0_EXT, 1);
return 1;
}

//------------------------------------------------------------------------------

int OglDisableFBuffer (tFrameBuffer *fb)
{
if (!gameStates.ogl.bRender2TextureOk)
	return 0;
glBindFramebufferEXT (GL_FRAMEBUFFER_EXT, 0);
#if 1//def _DEBUG
if (!OglFBufferAvail (fb))
	return 0;
#endif
//glBindTexture (GL_TEXTURE_2D, fb->hRenderBuffer);
OglDrawBuffer (GL_BACK, 1);
return 1;
}

#endif

//------------------------------------------------------------------------------

void OglInitFBuffer (void)
{
#if RENDER2TEXTURE
if (gameStates.ogl.bUseRender2Texture) {
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

	gameStates.ogl.bRender2TextureOk =
		(glBindRenderbufferEXT && glIsRenderbufferEXT && glDeleteRenderbuffersEXT &&
		 glGenRenderbuffersEXT && glRenderbufferStorageEXT && glGetRenderbufferParameterivEXT &&
		 glIsFramebufferEXT && glBindFramebufferEXT && glDeleteFramebuffersEXT &&
		 glGenFramebuffersEXT && glCheckFramebufferStatusEXT && glFramebufferTexture1DEXT &&
		 glFramebufferTexture2DEXT && glFramebufferTexture3DEXT && glFramebufferRenderbufferEXT &&
		 glGetFramebufferAttachmentParameterivEXT && glGenerateMipmapEXT) ?
		 2 : 0;
#	else
gameStates.ogl.bRender2TextureOk = 2;
#	endif
	}
#endif
PrintLog ((gameStates.ogl.bRender2TextureOk == 2) ? (char *) "Rendering to texture is available\n" : (char *) "No rendering to texture available\n");
}

//------------------------------------------------------------------------------

