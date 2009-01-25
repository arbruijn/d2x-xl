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

#include "descent.h"
#include "ogl_shader.h"

//------------------------------------------------------------------------------

#if TEXTURE_COMPRESSION
#	ifndef GL_VERSION_20
#		ifdef _WIN32
PFNGLGETCOMPRESSEDTEXIMAGEPROC	glGetCompressedTexImage = NULL;
PFNGLCOMPRESSEDTEXIMAGE2DPROC		glCompressedTexImage2D = NULL;
#		endif
#	endif
#endif

#if OGL_MULTI_TEXTURING
#	ifndef GL_VERSION_20
PFNGLACTIVETEXTUREARBPROC			glActiveTexture = NULL;
#		ifdef _WIN32
PFNGLMULTITEXCOORD2DARBPROC		glMultiTexCoord2d = NULL;
PFNGLMULTITEXCOORD2FARBPROC		glMultiTexCoord2f = NULL;
PFNGLMULTITEXCOORD2FVARBPROC		glMultiTexCoord2fv = NULL;
#		endif
PFNGLCLIENTACTIVETEXTUREARBPROC	glClientActiveTexture = NULL;
#	endif
#endif

#if OGL_QUERY
#	ifndef GL_VERSION_20
PFNGLGENQUERIESARBPROC				glGenQueries = NULL;
PFNGLDELETEQUERIESARBPROC			glDeleteQueries = NULL;
PFNGLISQUERYARBPROC					glIsQuery = NULL;
PFNGLBEGINQUERYARBPROC				glBeginQuery = NULL;
PFNGLENDQUERYARBPROC					glEndQuery = NULL;
PFNGLGETQUERYIVARBPROC				glGetQueryiv = NULL;
PFNGLGETQUERYOBJECTIVARBPROC		glGetQueryObjectiv = NULL;
PFNGLGETQUERYOBJECTUIVARBPROC		glGetQueryObjectuiv = NULL;
#	endif
#endif

#if OGL_POINT_SPRITES
#	ifdef _WIN32
PFNGLPOINTPARAMETERFVARBPROC		glPointParameterfvARB = NULL;
PFNGLPOINTPARAMETERFARBPROC		glPointParameterfARB = NULL;
#	endif
#endif

#ifndef GL_VERSION_20
#	ifdef _WIN32
PFNGLGENBUFFERSPROC					glGenBuffersARB = NULL;
PFNGLBINDBUFFERPROC					glBindBufferARB = NULL;
PFNGLBUFFERDATAPROC					glBufferDataARB = NULL;
PFNGLMAPBUFFERPROC					glMapBufferARB = NULL;
PFNGLUNMAPBUFFERPROC					glUnmapBufferARB = NULL;
PFNGLDELETEBUFFERSPROC				glDeleteBuffersARB = NULL;
PFNGLDRAWRANGEELEMENTSPROC			glDrawRangeElements = NULL;
#	endif
#endif

#ifdef _WIN32
PFNGLACTIVESTENCILFACEEXTPROC		glActiveStencilFaceEXT = NULL;
#endif

const char *pszOglExtensions = NULL;

//------------------------------------------------------------------------------

void OglInitOcclusionQuery (void)
{
#if OGL_QUERY
if (!(pszOglExtensions && strstr (pszOglExtensions, "GL_ARB_occlusion_query")))
	gameStates.ogl.bOcclusionQuery = 0;
else {
#	ifndef GL_VERSION_20
	glGenQueries        = (PFNGLGENQUERIESPROC) wglGetProcAddress ("glGenQueries");
	glDeleteQueries     = (PFNGLDELETEQUERIESPROC) wglGetProcAddress ("glDeleteQueries");
	glIsQuery           = (PFNGLISQUERYPROC) wglGetProcAddress ("glIsQuery");
	glBeginQuery        = (PFNGLBEGINQUERYPROC) wglGetProcAddress ("glBeginQuery");
	glEndQuery          = (PFNGLENDQUERYPROC) wglGetProcAddress ("glEndQuery");
	glGetQueryiv        = (PFNGLGETQUERYIVPROC) wglGetProcAddress ("glGetQueryiv");
	glGetQueryObjectiv  = (PFNGLGETQUERYOBJECTIVPROC) wglGetProcAddress ("glGetQueryObjectiv");
	glGetQueryObjectuiv = (PFNGLGETQUERYOBJECTUIVPROC) wglGetProcAddress ("glGetQueryObjectuiv");
	if (glGenQueries && glDeleteQueries && glIsQuery && 
		 glBeginQuery && glEndQuery && glGetQueryiv && 
		 glGetQueryObjectiv && glGetQueryObjectuiv) {
			GLint nBits;

      glGetQueryiv (GL_SAMPLES_PASSED, GL_QUERY_COUNTER_BITS_ARB, &nBits);
		gameStates.ogl.bOcclusionQuery = nBits > 0;
		}
	else
		gameStates.ogl.bOcclusionQuery = 0;
#	else
	gameStates.ogl.bOcclusionQuery = 1;
#	endif
	}
#endif
}

//------------------------------------------------------------------------------

void OglInitPointSprites (void)
{
#if OGL_POINT_SPRITES
#	ifdef _WIN32
glPointParameterfvARB	= (PFNGLPOINTPARAMETERFVARBPROC) wglGetProcAddress ("glPointParameterfvARB");
glPointParameterfARB		= (PFNGLPOINTPARAMETERFARBPROC) wglGetProcAddress ("glPointParameterfARB");
#	endif
#endif
}

//------------------------------------------------------------------------------

void OglInitStencilOps (void)
{
glEnable (GL_STENCIL_TEST);
if ((gameStates.render.bHaveStencilBuffer = glIsEnabled (GL_STENCIL_TEST)))
	glDisable (GL_STENCIL_TEST);
#if !DBG
#	ifdef _WIN32
glActiveStencilFaceEXT	= (PFNGLACTIVESTENCILFACEEXTPROC) wglGetProcAddress ("glActiveStencilFaceEXT");
#	endif
#endif
}

//------------------------------------------------------------------------------

void OglInitVBOs (void)
{
#ifndef GL_VERSION_20
#	ifdef _WIN32
glGenBuffersARB = (PFNGLGENBUFFERSPROC) wglGetProcAddress ("glGenBuffersARB");
glBindBufferARB = (PFNGLBINDBUFFERPROC) wglGetProcAddress ("glBindBufferARB");
glBufferDataARB = (PFNGLBUFFERDATAPROC) wglGetProcAddress ("glBufferDataARB");
glMapBufferARB = (PFNGLMAPBUFFERPROC) wglGetProcAddress ("glMapBufferARB");
glUnmapBufferARB = (PFNGLUNMAPBUFFERPROC) wglGetProcAddress ("glUnmapBufferARB");
glDeleteBuffersARB = (PFNGLDELETEBUFFERSPROC) wglGetProcAddress ("glDeleteBuffersARB");
glDrawRangeElements = (PFNGLDRAWRANGEELEMENTSPROC) wglGetProcAddress ("glDrawRangeElements");
gameStates.ogl.bHaveVBOs = glGenBuffersARB && glBindBufferARB && glBufferDataARB && glMapBufferARB && glUnmapBufferARB;
#	endif
#else
gameStates.ogl.bHaveVBOs = 1;
#endif
}

//------------------------------------------------------------------------------

void OglInitTextureCompression (void)
{
#if TEXTURE_COMPRESSION
gameStates.ogl.bHaveTexCompression = 1;
#	ifndef GL_VERSION_20
#		ifdef _WIN32
	glGetCompressedTexImage = (PFNGLGETCOMPRESSEDTEXIMAGEPROC) wglGetProcAddress ("glGetCompressedTexImage");
	glCompressedTexImage2D = (PFNGLCOMPRESSEDTEXIMAGE2DPROC) wglGetProcAddress ("glCompressedTexImage2D");
	gameStates.ogl.bHaveTexCompression = glGetCompressedTexImage && glCompressedTexImage2D;
#		endif
#	endif
#else
gameStates.ogl.bHaveTexCompression = 0;
#endif
}

//------------------------------------------------------------------------------

void OglInitMultiTexturing (void)
{
#if OGL_MULTI_TEXTURING
#	ifndef GL_VERSION_20
#		ifdef _WIN32
glMultiTexCoord2d			= (PFNGLMULTITEXCOORD2DARBPROC) wglGetProcAddress ("glMultiTexCoord2dARB");
glMultiTexCoord2f			= (PFNGLMULTITEXCOORD2FARBPROC) wglGetProcAddress ("glMultiTexCoord2fARB");
glMultiTexCoord2fv		= (PFNGLMULTITEXCOORD2FVARBPROC) wglGetProcAddress ("glMultiTexCoord2fvARB");
#		endif
glActiveTexture			= (PFNGLACTIVETEXTUREARBPROC) wglGetProcAddress ("glActiveTextureARB");	
glClientActiveTexture	= (PFNGLCLIENTACTIVETEXTUREARBPROC) wglGetProcAddress ("glClientActiveTextureARB");
gameStates.ogl.bMultiTexturingOk =
#ifdef _WIN32
	glMultiTexCoord2d && glMultiTexCoord2f	&& glMultiTexCoord2fv &&
#endif 
	glActiveTexture && glClientActiveTexture;
#	else
gameStates.ogl.bMultiTexturingOk = 1;
#	endif
#endif
}

//------------------------------------------------------------------------------

void OglInitAntiAliasing (void)
{
gameStates.ogl.bAntiAliasingOk = (pszOglExtensions && strstr (pszOglExtensions, "GL_ARB_multisample"));
}

//------------------------------------------------------------------------------

void OglInitExtensions (void)
{
pszOglExtensions = reinterpret_cast<const char*> (glGetString (GL_EXTENSIONS));
OglInitMultiTexturing ();
OglInitOcclusionQuery ();
OglInitPointSprites ();
OglInitTextureCompression ();
OglInitStencilOps ();
OglInitAntiAliasing ();
OglInitVBOs ();
OglInitShaders ();
#if RENDER2TEXTURE == 1
OglInitPBuffer ();
#elif RENDER2TEXTURE == 2
CFBO::Setup ();
#endif
//InitShaders ();
}

//------------------------------------------------------------------------------

