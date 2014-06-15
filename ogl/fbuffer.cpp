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

int CFBO::Available (void)
{
if (!ogl.m_features.bRenderToTexture)
	return 0;
if (!Handle ())
	return 0;
switch (m_info.nStatus = glCheckFramebufferStatusEXT (GL_FRAMEBUFFER_EXT)) {                                          
	case GL_FRAMEBUFFER_COMPLETE_EXT:                       
		return 1;
	case GL_FRAMEBUFFER_UNSUPPORTED_EXT:                    
		return -1;
	default:                                                
		return -1;
	}
}

//------------------------------------------------------------------------------

void CFBO::Init (void)
{
memset (&m_info, 0, sizeof (m_info));
m_info.nBuffer = 0x7FFFFFFF;
}

//------------------------------------------------------------------------------

int CFBO::CreateColorBuffers (int nBuffers)
{
if (!nBuffers)
	return 1;

	GLint	nMaxBuffers;

PrintLog (1, "Creating %d color buffers\n", nBuffers);
ogl.ClearError (false);
glGetIntegerv (GL_MAX_COLOR_ATTACHMENTS_EXT, &nMaxBuffers);
if (nMaxBuffers > MAX_COLOR_BUFFERS)
	nMaxBuffers = MAX_COLOR_BUFFERS;
else if (nBuffers > nMaxBuffers)
	nBuffers = nMaxBuffers;
m_info.nColorBuffers = 
m_info.nBufferCount = nBuffers;
m_info.nFirstBuffer = 0;

#if DBG
GLenum nError = glGetError ();
if (nError)
	nError = 0;
#endif

ogl.GenTextures (nBuffers, m_info.hColorBuffer);
for (int i = 0; i < nBuffers; i++) {
	PrintLog (0, "color buffer #%d handle = %d\n", i + 1, m_info.hColorBuffer [i]);
	ogl.BindTexture (m_info.hColorBuffer [i]);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE); 
	glTexParameteri (GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE);
	if (m_info.nType == 2) // GPGPU
		glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA32F_ARB, m_info.nWidth, m_info.nHeight, 0, GL_RGBA, GL_FLOAT, NULL);
	else {
#if 0
		if (i > 0)
			glTexImage2D (GL_TEXTURE_2D, 0, GL_RGB, m_info.nWidth, m_info.nHeight, 0, GL_RGB, GL_FLOAT, NULL);
		else 
#endif
			{
			glTexImage2D (GL_TEXTURE_2D, 0, GL_RGB, m_info.nWidth, m_info.nHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
			glGenerateMipmapEXT (GL_TEXTURE_2D);
			}
		}
	m_info.bufferIds [i] = GL_COLOR_ATTACHMENT0_EXT + i;
	//glFramebufferTexture2DEXT (GL_FRAMEBUFFER_EXT, m_info.bufferIds [i], GL_TEXTURE_2D, m_info.hColorBuffer [i], 0);
	}
PrintLog (-1);
return ogl.ClearError (false) ? 0 : 1;
}

//------------------------------------------------------------------------------

int CFBO::CreateDepthBuffer (void)
{
ogl.ClearError (false);
PrintLog (0, "Creating depth buffer (type = %d)\n", m_info.nType);
if (m_info.nType == 3) { // depth buffer for shadow map
	if (!(m_info.hDepthBuffer = ogl.CreateDepthTexture (GL_TEXTURE0, 1, 2, m_info.nWidth, m_info.nHeight)))
		return 0;
	glFramebufferTexture2DEXT (GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, m_info.hDepthBuffer, 0);
	}
else if (m_info.nType != 2) { // 2 -> GPGPU
	// depth buffer
	if (abs (m_info.nType) != 1) // -> no stencil buffer
		glGenRenderbuffersEXT (1, &m_info.hDepthBuffer);
	else { // depth + stencil buffer
		m_info.hDepthBuffer = (m_info.nType == 1)
									  ? ogl.CreateDepthTexture (GL_TEXTURE0, 1, 1, m_info.nWidth, m_info.nHeight)
									  : ogl.CopyDepthTexture (1); //ogl.m_states.hDepthBuffer [1];
		if (!m_info.hDepthBuffer)
			return 0;
		glFramebufferTexture2DEXT (GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, m_info.hDepthBuffer, 0);
		glFramebufferTexture2DEXT (GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_TEXTURE_2D, m_info.hStencilBuffer = m_info.hDepthBuffer, 0);
		}
	}
return ogl.ClearError (false) ? 0 : 1;
}

//------------------------------------------------------------------------------

void CFBO::AttachBuffers (void)
{
if (m_info.nType == 3) { // depth buffer for shadow map
	glFramebufferTexture2DEXT (GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, m_info.hDepthBuffer, 0);
	}
else if (m_info.nType != 2) { // 2 -> GPGPU
	for (int i = 0; i < m_info.nColorBuffers; i++)
		glFramebufferTexture2DEXT (GL_FRAMEBUFFER_EXT, m_info.bufferIds [i], GL_TEXTURE_2D, m_info.hColorBuffer [i], 0);
	// depth + stencil buffer
	if (abs (m_info.nType) == 1) {
		glFramebufferTexture2DEXT (GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, m_info.hDepthBuffer, 0);
		glFramebufferTexture2DEXT (GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_TEXTURE_2D, m_info.hStencilBuffer = m_info.hDepthBuffer, 0);
		}
	else {
		// depth buffer
		glBindRenderbufferEXT (GL_RENDERBUFFER_EXT, m_info.hDepthBuffer);
		glRenderbufferStorageEXT (GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, m_info.nWidth, m_info.nHeight);
		glFramebufferRenderbufferEXT (GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, m_info.hDepthBuffer);
		}
	}
}

//------------------------------------------------------------------------------

int CFBO::Create (int nWidth, int nHeight, int nType, int nColorBuffers)
{
if (!ogl.m_features.bRenderToTexture)
	return 0;

Destroy ();
if (nWidth > 0)
	m_info.nWidth = nWidth;
if (nHeight > 0)
	m_info.nHeight = nHeight;

if (nType == 3) { // no color buffer needed for rendering shadow maps
#if MAX_SHADOWMAPS < 0
	nType = 1;
#else
	nColorBuffers = 0;
#endif
	}

m_info.nType = nType;
m_info.hDepthBuffer = 0;
m_info.hStencilBuffer = 0;

glGenFramebuffersEXT (1, &m_info.hFBO);
PrintLog (0, "draw buffer handle = %d\n", m_info.hFBO);
glBindFramebufferEXT (GL_FRAMEBUFFER_EXT, m_info.hFBO);

if (!(CreateColorBuffers (nColorBuffers) && CreateDepthBuffer ())) {
	Destroy ();
	return 0;
	}
AttachBuffers ();
glBindFramebufferEXT (GL_FRAMEBUFFER_EXT, 0);
return Available ();
}

//------------------------------------------------------------------------------

void CFBO::Destroy (void)
{
if (!ogl.m_features.bRenderToTexture)
	return;
if (m_info.hFBO) {
	if (m_info.nColorBuffers) {
		ogl.DeleteTextures (m_info.nColorBuffers, m_info.hColorBuffer);
		memset (m_info.hColorBuffer, 0, sizeof (m_info.hColorBuffer));
		m_info.nColorBuffers = 0;
		}
	if (m_info.hDepthBuffer) {
		if (m_info.nType == 1)
			ogl.DeleteTextures (1, &m_info.hDepthBuffer);
		if (m_info.nType >= 0)
			glDeleteRenderbuffersEXT (1, &m_info.hDepthBuffer);
		m_info.hDepthBuffer =
		m_info.hStencilBuffer = 0;
		}
	glDeleteFramebuffersEXT (1, &m_info.hFBO);
	m_info.hFBO = 0;
	}
}

//------------------------------------------------------------------------------

int CFBO::Enable (int nColorBuffers)
{
if (!m_info.bActive) {
	if (Available () <= 0)
		return 0;
	if (m_info.nType == 3) {
		glDrawBuffer (GL_NONE);
		glReadBuffer (GL_NONE);
		}
	glBindFramebufferEXT (GL_FRAMEBUFFER_EXT, m_info.hFBO);
	}
if (m_info.nType != 3)
	SelectColorBuffers (nColorBuffers);
return m_info.bActive = 1;
}

//------------------------------------------------------------------------------

int CFBO::Disable (void)
{
if (!m_info.bActive)
	return 1;
if (Available () <= 0)
	return 0;
m_info.bActive = 0;
m_info.nBuffer = 0x7FFFFFFF;
glBindFramebufferEXT (GL_FRAMEBUFFER_EXT, 0);
ogl.SetDrawBuffer (GL_BACK, 0);
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
PrintLog (1, "Checking rendering to texture ...\n");
#if RENDER2TEXTURE
if (ogl.m_features.bRenderToTexture.Available ()) {
	PrintLog (1);
#	ifdef _WIN32
	ogl.m_features.bRenderToTexture = 0;
	if (!(glDrawBuffers = (PFNGLDRAWBUFFERSPROC) wglGetProcAddress ("glDrawBuffers")))
		PrintLog (0, "glDrawBuffers not supported by the OpenGL driver\n");
	else if (!(glBindRenderbufferEXT = (PFNGLBINDRENDERBUFFEREXTPROC) wglGetProcAddress ("glBindRenderbufferEXT")))
		PrintLog (0, "glBindRenderbufferEXT not supported by the OpenGL driver\n");
	else if (!(glIsRenderbufferEXT = (PFNGLISRENDERBUFFEREXTPROC) wglGetProcAddress ("glIsRenderbufferEXT")))
		PrintLog (0, "glIsRenderbufferEXT not supported by the OpenGL driver\n");
	else if (!(glDeleteRenderbuffersEXT = (PFNGLDELETERENDERBUFFERSEXTPROC) wglGetProcAddress ("glDeleteRenderbuffersEXT")))
		PrintLog (0, "glDeleteRenderbuffersEXT not supported by the OpenGL driver\n");
	else if (!(glGenRenderbuffersEXT = (PFNGLGENRENDERBUFFERSEXTPROC) wglGetProcAddress ("glGenRenderbuffersEXT")))
		PrintLog (0, "glGenRenderbuffersEXT not supported by the OpenGL driver\n");
	else if (!(glRenderbufferStorageEXT = (PFNGLRENDERBUFFERSTORAGEEXTPROC) wglGetProcAddress ("glRenderbufferStorageEXT")))
		PrintLog (0, "glRenderbufferStorageEXT not supported by the OpenGL driver\n");
	else if (!(glGetRenderbufferParameterivEXT = (PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC) wglGetProcAddress ("glGetRenderbufferParameterivEXT")))
		PrintLog (0, "glGetRenderbufferParameterivEXT not supported by the OpenGL driver\n");
	else if (!(glIsFramebufferEXT = (PFNGLISFRAMEBUFFEREXTPROC) wglGetProcAddress ("glIsFramebufferEXT")))
		PrintLog (0, "glIsFramebufferEXT not supported by the OpenGL driver\n");
	else if (!(glBindFramebufferEXT = (PFNGLBINDFRAMEBUFFEREXTPROC) wglGetProcAddress ("glBindFramebufferEXT")))
		PrintLog (0, "glBindFramebufferEXT not supported by the OpenGL driver\n");
	else if (!(glDeleteFramebuffersEXT = (PFNGLDELETEFRAMEBUFFERSEXTPROC) wglGetProcAddress ("glDeleteFramebuffersEXT")))
		PrintLog (0, "glDeleteFramebuffersEXT not supported by the OpenGL driver\n");
	else if (!(glGenFramebuffersEXT = (PFNGLGENFRAMEBUFFERSEXTPROC) wglGetProcAddress ("glGenFramebuffersEXT")))
		PrintLog (0, "glGenFramebuffersEXT not supported by the OpenGL driver\n");
	else if (!(glCheckFramebufferStatusEXT = (PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC) wglGetProcAddress ("glCheckFramebufferStatusEXT")))
		PrintLog (0, "glCheckFramebufferStatusEXT not supported by the OpenGL driver\n");
	else if (!(glFramebufferTexture1DEXT = (PFNGLFRAMEBUFFERTEXTURE1DEXTPROC) wglGetProcAddress ("glFramebufferTexture1DEXT")))
		PrintLog (0, "glFramebufferTexture1DEXT not supported by the OpenGL driver\n");
	else if (!(glFramebufferTexture2DEXT = (PFNGLFRAMEBUFFERTEXTURE2DEXTPROC) wglGetProcAddress ("glFramebufferTexture2DEXT")))
		PrintLog (0, "glFramebufferTexture2DEXT not supported by the OpenGL driver\n");
	else if (!(glFramebufferTexture3DEXT = (PFNGLFRAMEBUFFERTEXTURE3DEXTPROC) wglGetProcAddress ("glFramebufferTexture3DEXT")))
		PrintLog (0, "glFramebufferTexture3DEXT not supported by the OpenGL driver\n");
	else if (!(glFramebufferRenderbufferEXT = (PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC) wglGetProcAddress ("glFramebufferRenderbufferEXT")))
		PrintLog (0, "glFramebufferRenderbufferEXT not supported by the OpenGL driver\n");
	else if (!(glGetFramebufferAttachmentParameterivEXT = (PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC) wglGetProcAddress ("glGetFramebufferAttachmentParameterivEXT")))
		PrintLog (0, "glGetFramebufferAttachmentParameterivEXT not supported by the OpenGL driver\n");
	else if (!(glGenerateMipmapEXT = (PFNGLGENERATEMIPMAPEXTPROC) wglGetProcAddress ("glGenerateMipmapEXT")))
		PrintLog (0, "glGenerateMipmapEXT not supported by the OpenGL driver\n");
	else
#	endif
	ogl.m_features.bRenderToTexture = 2;
	PrintLog (-1);
	}
#endif
PrintLog (-1, (ogl.m_features.bRenderToTexture == 2) ? "Rendering to texture is available\n" : "No rendering to texture available\n");
}

//------------------------------------------------------------------------------

