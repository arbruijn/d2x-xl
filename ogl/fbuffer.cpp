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
# include <SDL.h>
#endif

#include "ogl_defs.h"
#include "ogl_lib.h"
#include "lightmap.h"
#include "texmerge.h"
#include "error.h"

#define FBO_STENCIL_BUFFER	1

//------------------------------------------------------------------------------

#if RENDER2TEXTURE == 2
#	ifndef GL_VERSION_20
PFNGLDRAWBUFFERSPROC glDrawBuffers;
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

int CFBO::Available (void)
{
if (!ogl.m_states.bRender2TextureOk)
	return 0;
if (!Handle ())
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

int CFBO::Create (int nWidth, int nHeight, int nType, int nColorBuffers)
{
	GLenum	nError;
	GLint		nMaxBuffers;

if (!ogl.m_states.bRender2TextureOk)
	return 0;
Destroy ();
glGenFramebuffersEXT (1, &m_info.hFBO);
glBindFramebufferEXT (GL_FRAMEBUFFER_EXT, m_info.hFBO);

if (nWidth > 0)
	m_info.nWidth = nWidth;
if (nHeight > 0)
	m_info.nHeight = nHeight;

glGetIntegerv (GL_MAX_COLOR_ATTACHMENTS_EXT, &nMaxBuffers);
if (nMaxBuffers > MAX_COLOR_BUFFERS)
	nMaxBuffers = MAX_COLOR_BUFFERS;
if (nColorBuffers > nMaxBuffers)
	nColorBuffers = nMaxBuffers;
m_info.nColorBuffers = 
m_info.nBufferCount = nColorBuffers;
m_info.nFirstBuffer = 0;

ogl.GenTextures (nColorBuffers, m_info.hColorBuffer);

if (nType == 2) { //GPGPU
	for (int i = 0; i < nColorBuffers; i++) {
		ogl.BindTexture (m_info.hColorBuffer [i]);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE); 
		glTexParameteri (GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE);
		glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA32F_ARB, m_info.nWidth, m_info.nHeight, 0, GL_RGBA, GL_FLOAT, NULL);
		glFramebufferTexture2DEXT (GL_FRAMEBUFFER_EXT, m_info.bufferIds [i] = GL_COLOR_ATTACHMENT0_EXT + i, GL_TEXTURE_2D, m_info.hColorBuffer [i], 0);
		}
	m_info.hDepthBuffer = 0;
	m_info.hStencilBuffer = 0;
	}
else {
	// color buffers
	for (int i = 0; i < nColorBuffers; i++) {
		ogl.BindTexture (m_info.hColorBuffer [i]);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); //GL_LINEAR);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,  GL_NEAREST); //GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameteri (GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE);
		glTexImage2D (GL_TEXTURE_2D, 0, 3, m_info.nWidth, m_info.nHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
		glGenerateMipmapEXT (GL_TEXTURE_2D);
		glFramebufferTexture2DEXT (GL_FRAMEBUFFER_EXT, m_info.bufferIds [i] = GL_COLOR_ATTACHMENT0_EXT + i, GL_TEXTURE_2D, m_info.hColorBuffer [i], 0);
		}
#if FBO_STENCIL_BUFFER
	// depth + stencil buffer
	if ((nType == 1) && (m_info.hDepthBuffer = ogl.CreateDepthTexture (0, 1, 1))) {
		glFramebufferTexture2DEXT (GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, m_info.hDepthBuffer, 0);
		glFramebufferTexture2DEXT (GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_TEXTURE_2D, m_info.hStencilBuffer = m_info.hDepthBuffer, 0);
		if (Available () < 0)
			return 0;
		}
	else 
#endif
		{
		// depth buffer
		m_info.hStencilBuffer = 0;
		glGenRenderbuffersEXT (1, &m_info.hDepthBuffer);
		glBindRenderbufferEXT (GL_RENDERBUFFER_EXT, m_info.hDepthBuffer);
		glRenderbufferStorageEXT (GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, m_info.nWidth, m_info.nHeight);
		glFramebufferRenderbufferEXT (GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, m_info.hDepthBuffer);
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
if (!ogl.m_states.bRender2TextureOk)
	return;
if (m_info.hFBO) {
	if (m_info.nColorBuffers) {
		ogl.DeleteTextures (m_info.nColorBuffers, m_info.hColorBuffer);
		memset (m_info.hColorBuffer, 0, sizeof (m_info.hColorBuffer));
		m_info.nColorBuffers = 
		m_info.nColorBuffers = 0;
		}
	if (m_info.hDepthBuffer) {
		glDeleteRenderbuffersEXT (1, &m_info.hDepthBuffer);
		m_info.hDepthBuffer =
		m_info.hStencilBuffer = 0;
		}
	glDeleteFramebuffersEXT (1, &m_info.hFBO);
	m_info.hFBO = 0;
	}
}

//------------------------------------------------------------------------------

int CFBO::Enable (bool bFallback)
{
if (m_info.bActive)
	return 1;
if (Available () <= 0)
	return 0;
//if (bFallback) 
	{
	glBindFramebufferEXT (GL_FRAMEBUFFER_EXT, 0);
	ogl.SetDrawBuffer (GL_BACK, 0);
	}
glBindFramebufferEXT (GL_FRAMEBUFFER_EXT, m_info.hFBO);
SetDrawBuffers ();
return m_info.bActive = 1;
}

//------------------------------------------------------------------------------

int CFBO::Disable (bool bFallback)
{
if (!m_info.bActive)
	return 1;
if (Available () <= 0)
	return 0;
m_info.bActive = 0;
//if (bFallback) 
	{
	glBindFramebufferEXT (GL_FRAMEBUFFER_EXT, 0);
	ogl.SetDrawBuffer (GL_BACK, 0);
	}
return 1;
}

#endif

//------------------------------------------------------------------------------

int CFBO::IsBound (void) 
{ 
return m_info.hColorBuffer [0] && (m_info.hColorBuffer [0] == ogl.BoundTexture ()); 
}

//------------------------------------------------------------------------------

void CFBO::Setup (void)
{
PrintLog ("Checking rendering to texture ...\n");
#if RENDER2TEXTURE
if (ogl.m_states.bUseRender2Texture) {
#	ifdef _WIN32
	ogl.m_states.bRender2TextureOk = 0;
	if (!(glDrawBuffers = (PFNGLDRAWBUFFERSPROC) wglGetProcAddress ("glDrawBuffers")))
		PrintLog ("   glDrawBuffers not supported by the OpenGL driver\n");
	else if (!(glBindRenderbufferEXT = (PFNGLBINDRENDERBUFFEREXTPROC) wglGetProcAddress ("glBindRenderbufferEXT")))
		PrintLog ("   glBindRenderbufferEXT not supported by the OpenGL driver\n");
	else if (!(glIsRenderbufferEXT = (PFNGLISRENDERBUFFEREXTPROC) wglGetProcAddress ("glIsRenderbufferEXT")))
		PrintLog ("   glIsRenderbufferEXT not supported by the OpenGL driver\n");
	else if (!(glDeleteRenderbuffersEXT = (PFNGLDELETERENDERBUFFERSEXTPROC) wglGetProcAddress ("glDeleteRenderbuffersEXT")))
		PrintLog ("   glDeleteRenderbuffersEXT not supported by the OpenGL driver\n");
	else if (!(glGenRenderbuffersEXT = (PFNGLGENRENDERBUFFERSEXTPROC) wglGetProcAddress ("glGenRenderbuffersEXT")))
		PrintLog ("   glGenRenderbuffersEXT not supported by the OpenGL driver\n");
	else if (!(glRenderbufferStorageEXT = (PFNGLRENDERBUFFERSTORAGEEXTPROC) wglGetProcAddress ("glRenderbufferStorageEXT")))
		PrintLog ("   glRenderbufferStorageEXT not supported by the OpenGL driver\n");
	else if (!(glGetRenderbufferParameterivEXT = (PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC) wglGetProcAddress ("glGetRenderbufferParameterivEXT")))
		PrintLog ("   glGetRenderbufferParameterivEXT not supported by the OpenGL driver\n");
	else if (!(glIsFramebufferEXT = (PFNGLISFRAMEBUFFEREXTPROC) wglGetProcAddress ("glIsFramebufferEXT")))
		PrintLog ("   glIsFramebufferEXT not supported by the OpenGL driver\n");
	else if (!(glBindFramebufferEXT = (PFNGLBINDFRAMEBUFFEREXTPROC) wglGetProcAddress ("glBindFramebufferEXT")))
		PrintLog ("   glBindFramebufferEXT not supported by the OpenGL driver\n");
	else if (!(glDeleteFramebuffersEXT = (PFNGLDELETEFRAMEBUFFERSEXTPROC) wglGetProcAddress ("glDeleteFramebuffersEXT")))
		PrintLog ("   glDeleteFramebuffersEXT not supported by the OpenGL driver\n");
	else if (!(glGenFramebuffersEXT = (PFNGLGENFRAMEBUFFERSEXTPROC) wglGetProcAddress ("glGenFramebuffersEXT")))
		PrintLog ("   glGenFramebuffersEXT not supported by the OpenGL driver\n");
	else if (!(glCheckFramebufferStatusEXT = (PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC) wglGetProcAddress ("glCheckFramebufferStatusEXT")))
		PrintLog ("   glCheckFramebufferStatusEXT not supported by the OpenGL driver\n");
	else if (!(glFramebufferTexture1DEXT = (PFNGLFRAMEBUFFERTEXTURE1DEXTPROC) wglGetProcAddress ("glFramebufferTexture1DEXT")))
		PrintLog ("   glFramebufferTexture1DEXT not supported by the OpenGL driver\n");
	else if (!(glFramebufferTexture2DEXT = (PFNGLFRAMEBUFFERTEXTURE2DEXTPROC) wglGetProcAddress ("glFramebufferTexture2DEXT")))
		PrintLog ("   glFramebufferTexture2DEXT not supported by the OpenGL driver\n");
	else if (!(glFramebufferTexture3DEXT = (PFNGLFRAMEBUFFERTEXTURE3DEXTPROC) wglGetProcAddress ("glFramebufferTexture3DEXT")))
		PrintLog ("   glFramebufferTexture3DEXT not supported by the OpenGL driver\n");
	else if (!(glFramebufferRenderbufferEXT = (PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC) wglGetProcAddress ("glFramebufferRenderbufferEXT")))
		PrintLog ("   glFramebufferRenderbufferEXT not supported by the OpenGL driver\n");
	else if (!(glGetFramebufferAttachmentParameterivEXT = (PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC) wglGetProcAddress ("glGetFramebufferAttachmentParameterivEXT")))
		PrintLog ("   glGetFramebufferAttachmentParameterivEXT not supported by the OpenGL driver\n");
	else if (!(glGenerateMipmapEXT = (PFNGLGENERATEMIPMAPEXTPROC) wglGetProcAddress ("glGenerateMipmapEXT")))
		PrintLog ("   glGenerateMipmapEXT not supported by the OpenGL driver\n");
	else
#	endif
ogl.m_states.bRender2TextureOk = 2;
	}
#endif
PrintLog ((ogl.m_states.bRender2TextureOk == 2) ? "Rendering to texture is available\n" : "No rendering to texture available\n");
}

//------------------------------------------------------------------------------

