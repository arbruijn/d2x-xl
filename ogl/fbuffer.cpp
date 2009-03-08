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
PFNGLGENERATEMIPMAPEXTPROC glGenerateMipMapEXT;
#	endif
#endif

//------------------------------------------------------------------------------

#if RENDER2TEXTURE == 2

int CFBO::Available (void)
{
if (!gameStates.ogl.bRender2TextureOk)
	return 0;
switch (m_info.nStatus = glCheckFramebufferStatusEXT (GL_FRAMEBUFFER_EXT)) {                                          
	case GL_FRAMEBUFFER_COMPLETE_EXT:                       
		return 1;
	case GL_FRAMEBUFFER_UNSUPPORTED_EXT:                    
		return -1;
	default:                                                
		return 0;
	}
}

//------------------------------------------------------------------------------

void CFBO::Init (void)
{
memset (&m_info, 0, sizeof (m_info));
}

//------------------------------------------------------------------------------

int CFBO::Create (int nWidth, int nHeight, int nType)
{
	GLenum	nError;

if (!gameStates.ogl.bRender2TextureOk)
	return 0;
Destroy ();
glGenFramebuffersEXT (1, &m_info.hFBO);
glBindFramebufferEXT (GL_FRAMEBUFFER_EXT, m_info.hFBO);

if (nWidth > 0)
	m_info.nWidth = nWidth;
if (nHeight > 0)
	m_info.nHeight = nHeight;
OglGenTextures (1, &m_info.hRenderBuffer);
if (nType == 2) { //GPGPU
	glBindTexture (GL_TEXTURE_2D, m_info.hRenderBuffer);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE); 
	glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA32F_ARB, m_info.nWidth, m_info.nHeight, 0, GL_RGBA, GL_FLOAT, NULL);
	glFramebufferTexture2DEXT (GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, m_info.hRenderBuffer, 0);
	m_info.hDepthBuffer = 0;
	}
else {
	glBindTexture (GL_TEXTURE_2D, m_info.hRenderBuffer);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); //LINEAR /*GL_LINEAR_MIPMAP_LINEAR*/);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); //LINEAR);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
#if 0
	if (nType == 1) //depth texture
		glTexImage2D (GL_TEXTURE_2D, 0, 1, screen.Width (), screen.Height (), 0, GL_DEPTH_COMPONENT, GL_INT, NULL);
	else 
#endif
	 {
		glTexImage2D (GL_TEXTURE_2D, 0, 3, m_info.nWidth, m_info.nHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
		glGenerateMipMapEXT (GL_TEXTURE_2D);
		}
	glFramebufferTexture2DEXT (GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, m_info.hRenderBuffer, 0);
#if FBO_STENCIL_BUFFER
	if ((nType == 1) && (m_info.hDepthBuffer = OglCreateDepthTexture (0, 1))) {
		glFramebufferTexture2DEXT (GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, m_info.hDepthBuffer, 0);
		glGenRenderbuffersEXT (1, &m_info.hStencilBuffer);
		glBindRenderbufferEXT (GL_RENDERBUFFER_EXT, m_info.hStencilBuffer);
		glRenderbufferStorageEXT (GL_RENDERBUFFER_EXT, GL_STENCIL_INDEX8_EXT, m_info.nWidth, m_info.nHeight);
		glFramebufferRenderbufferEXT (GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, m_info.hStencilBuffer);
		if (Available () < 0)
			return 0;
		}
	else 
#else
	m_info.hStencilBuffer = 0;
#endif
	 {
		glGenRenderbuffersEXT (1, &m_info.hDepthBuffer);
		glBindRenderbufferEXT (GL_RENDERBUFFER_EXT, m_info.hDepthBuffer);
		glRenderbufferStorageEXT (GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, m_info.nWidth, m_info.nHeight);
		glFramebufferRenderbufferEXT (GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, m_info.hDepthBuffer);
		if ((nError = glGetError ()) == GL_OUT_OF_MEMORY)
			return 0;
		}
	if ((nError = glGetError ()) == GL_OUT_OF_MEMORY)
		return 0;
	if (Available () < 0)
		return 0;
	}
m_info.nType = nType;
glBindFramebufferEXT (GL_FRAMEBUFFER_EXT, 0);
return 1;
}

//------------------------------------------------------------------------------

void CFBO::Destroy (void)
{
if (!gameStates.ogl.bRender2TextureOk)
	return;
if (m_info.hFBO) {
	if (m_info.hRenderBuffer) {
		OglDeleteTextures (1, &m_info.hRenderBuffer);
		m_info.hRenderBuffer = 0;
		}
	if (m_info.hDepthBuffer) {
#if 1
		if (m_info.nType == 1) {
			OglDeleteTextures (1, &m_info.hDepthBuffer);
#if FBO_STENCIL_BUFFER
			glDeleteRenderbuffersEXT (1, &m_info.hStencilBuffer);
			m_info.hStencilBuffer = 0;
#endif
			}
		else
#endif
			glDeleteRenderbuffersEXT (1, &m_info.hDepthBuffer);
		m_info.hDepthBuffer = 0;
		}
	glDeleteFramebuffersEXT (1, &m_info.hFBO);
	m_info.hFBO = 0;
	}
}

//------------------------------------------------------------------------------

int CFBO::Enable (void)
{
if (!gameStates.ogl.bRender2TextureOk)
	return 0;
glDrawBuffer (GL_BACK);
glBindFramebufferEXT (GL_FRAMEBUFFER_EXT, 0);
gameStates.ogl.bDrawBufferActive = 0;
glBindTexture (GL_TEXTURE_2D, 0);
glBindFramebufferEXT (GL_FRAMEBUFFER_EXT, m_info.hFBO);
#if 1//def _DEBUG
if (!Available ())
	return 0;
#endif
OglSetDrawBuffer (GL_COLOR_ATTACHMENT0_EXT, 1);
return 1;
}

//------------------------------------------------------------------------------

int CFBO::Disable (void)
{
if (!gameStates.ogl.bRender2TextureOk)
	return 0;
glBindFramebufferEXT (GL_FRAMEBUFFER_EXT, 0);
#if 1//def _DEBUG
if (!Available ())
	return 0;
#endif
//glBindTexture (GL_TEXTURE_2D, m_info.hRenderBuffer);
OglSetDrawBuffer (GL_BACK, 1);
return 1;
}

#endif

//------------------------------------------------------------------------------

void CFBO::Setup (void)
{
PrintLog ("Checking rendering to texture ...\n");
#if RENDER2TEXTURE
if (gameStates.ogl.bUseRender2Texture) {
#	ifdef _WIN32
	gameStates.ogl.bRender2TextureOk = 0;
	if (!(glBindRenderbufferEXT = (PFNGLBINDRENDERBUFFEREXTPROC) wglGetProcAddress ("glBindRenderbufferEXT")))
		PrintLog ("   glBindRenderbufferEXT not supported by the OpenGL driver\n");
	if (!(glIsRenderbufferEXT = (PFNGLISRENDERBUFFEREXTPROC) wglGetProcAddress ("glIsRenderbufferEXT")))
		PrintLog ("   glIsRenderbufferEXT not supported by the OpenGL driver\n");
	if (!(glDeleteRenderbuffersEXT = (PFNGLDELETERENDERBUFFERSEXTPROC) wglGetProcAddress ("glDeleteRenderbuffersEXT")))
		PrintLog ("   glDeleteRenderbuffersEXT not supported by the OpenGL driver\n");
	if (!(glGenRenderbuffersEXT = (PFNGLGENRENDERBUFFERSEXTPROC) wglGetProcAddress ("glGenRenderbuffersEXT")))
		PrintLog ("   glGenRenderbuffersEXT not supported by the OpenGL driver\n");
	if (!(glRenderbufferStorageEXT = (PFNGLRENDERBUFFERSTORAGEEXTPROC) wglGetProcAddress ("glRenderbufferStorageEXT")))
		PrintLog ("   glRenderbufferStorageEXT not supported by the OpenGL driver\n");
	if (!(glGetRenderbufferParameterivEXT = (PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC) wglGetProcAddress ("glGetRenderbufferParameterivEXT")))
		PrintLog ("   glGetRenderbufferParameterivEXT not supported by the OpenGL driver\n");
	if (!(glIsFramebufferEXT = (PFNGLISFRAMEBUFFEREXTPROC) wglGetProcAddress ("glIsFramebufferEXT")))
		PrintLog ("   glIsFramebufferEXT not supported by the OpenGL driver\n");
	if (!(glBindFramebufferEXT = (PFNGLBINDFRAMEBUFFEREXTPROC) wglGetProcAddress ("glBindFramebufferEXT")))
		PrintLog ("   glBindFramebufferEXT not supported by the OpenGL driver\n");
	if (!(glDeleteFramebuffersEXT = (PFNGLDELETEFRAMEBUFFERSEXTPROC) wglGetProcAddress ("glDeleteFramebuffersEXT")))
		PrintLog ("   glDeleteFramebuffersEXT not supported by the OpenGL driver\n");
	if (!(glGenFramebuffersEXT = (PFNGLGENFRAMEBUFFERSEXTPROC) wglGetProcAddress ("glGenFramebuffersEXT")))
		PrintLog ("   glGenFramebuffersEXT not supported by the OpenGL driver\n");
	if (!(glCheckFramebufferStatusEXT = (PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC) wglGetProcAddress ("glCheckFramebufferStatusEXT")))
		PrintLog ("   glCheckFramebufferStatusEXT not supported by the OpenGL driver\n");
	if (!(glFramebufferTexture1DEXT = (PFNGLFRAMEBUFFERTEXTURE1DEXTPROC) wglGetProcAddress ("glFramebufferTexture1DEXT")))
		PrintLog ("   glFramebufferTexture1DEXT not supported by the OpenGL driver\n");
	if (!(glFramebufferTexture2DEXT = (PFNGLFRAMEBUFFERTEXTURE2DEXTPROC) wglGetProcAddress ("glFramebufferTexture2DEXT")))
		PrintLog ("   glFramebufferTexture2DEXT not supported by the OpenGL driver\n");
	if (!(glFramebufferTexture3DEXT = (PFNGLFRAMEBUFFERTEXTURE3DEXTPROC) wglGetProcAddress ("glFramebufferTexture3DEXT")))
		PrintLog ("   glFramebufferTexture3DEXT not supported by the OpenGL driver\n");
	if (!(glFramebufferRenderbufferEXT = (PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC) wglGetProcAddress ("glFramebufferRenderbufferEXT")))
		PrintLog ("   glFramebufferRenderbufferEXT not supported by the OpenGL driver\n");
	if (!(glGetFramebufferAttachmentParameterivEXT = (PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC) wglGetProcAddress ("glGetFramebufferAttachmentParameterivEXT")))
		PrintLog ("   glGetFramebufferAttachmentParameterivEXT not supported by the OpenGL driver\n");
	if (!(glGenerateMipMapEXT = (PFNGLGENERATEMIPMAPEXTPROC) wglGetProcAddress ("glGenerateMipMapEXT")))
		PrintLog ("   glGenerateMipMapEXT not supported by the OpenGL driver\n");
	else
#	endif
gameStates.ogl.bRender2TextureOk = 2;
	}
#endif
PrintLog ((gameStates.ogl.bRender2TextureOk == 2) ? "Rendering to texture is available\n" : "No rendering to texture available\n");
}

//------------------------------------------------------------------------------

