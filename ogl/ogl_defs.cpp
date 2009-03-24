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
PFNWGLSWAPINTERVALEXTPROC			wglSwapIntervalEXT = NULL;
PFNGLACTIVESTENCILFACEEXTPROC		glActiveStencilFaceEXT = NULL;
#endif

const char *pszOglExtensions = NULL;

//------------------------------------------------------------------------------

void OglInitOcclusionQuery (void)
{
#if OGL_QUERY
gameStates.ogl.bOcclusionQuery = 0;
PrintLog ("Checking occlusion query ...\n");
if (!(pszOglExtensions && strstr (pszOglExtensions, "GL_ARB_occlusion_query")))
	PrintLog ("   Occlusion query not supported by the OpenGL driver\n");
else {
#	ifndef GL_VERSION_20
	if (!(glGenQueries = (PFNGLGENQUERIESPROC) wglGetProcAddress ("glGenQueries")))
		PrintLog ("   glGenQueries not supported by the OpenGL driver\n");
	else if (!(glDeleteQueries = (PFNGLDELETEQUERIESPROC) wglGetProcAddress ("glDeleteQueries")))
		PrintLog ("   glDeleteQueries not supported by the OpenGL driver\n");
	else if (!(glIsQuery = (PFNGLISQUERYPROC) wglGetProcAddress ("glIsQuery")))
		PrintLog ("   glIsQuery not supported by the OpenGL driver\n");
	else if (!(glBeginQuery = (PFNGLBEGINQUERYPROC) wglGetProcAddress ("glBeginQuery")))
		PrintLog ("   glBeginQuery not supported by the OpenGL driver\n");
	else if (!(glEndQuery = (PFNGLENDQUERYPROC) wglGetProcAddress ("glEndQuery")))
		PrintLog ("   glEndQuery not supported by the OpenGL driver\n");
	else if (!(glGetQueryiv = (PFNGLGETQUERYIVPROC) wglGetProcAddress ("glGetQueryiv")))
		PrintLog ("   glGetQueryiv not supported by the OpenGL driver\n");
	else if (!(glGetQueryObjectiv = (PFNGLGETQUERYOBJECTIVPROC) wglGetProcAddress ("glGetQueryObjectiv")))
		PrintLog ("   glGetQueryObjectiv not supported by the OpenGL driver\n");
	else if (!(glGetQueryObjectuiv = (PFNGLGETQUERYOBJECTUIVPROC) wglGetProcAddress ("glGetQueryObjectuiv")))
		PrintLog ("   glGetQueryObjectuiv not supported by the OpenGL driver\n");
	else {
		GLint nBits;
      glGetQueryiv (GL_SAMPLES_PASSED, GL_QUERY_COUNTER_BITS_ARB, &nBits);
		gameStates.ogl.bOcclusionQuery = nBits > 0;
		}
#	else
	gameStates.ogl.bOcclusionQuery = 1;
#	endif
	}
#endif
PrintLog (gameStates.ogl.bOcclusionQuery ? (char *) "Occlusion query is available\n" : (char *) "No occlusion query available\n");
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
PrintLog ("Checking VBOs ...\n");
gameStates.ogl.bHaveVBOs = 0;
if (!(glGenBuffersARB = (PFNGLGENBUFFERSPROC) wglGetProcAddress ("glGenBuffersARB")))
	PrintLog ("   glGenBuffersARB not supported by the OpenGL driver\n");
else if (!(glBindBufferARB = (PFNGLBINDBUFFERPROC) wglGetProcAddress ("glBindBufferARB")))
	PrintLog ("   glBindBufferARB not supported by the OpenGL driver\n");
else if (!(glBufferDataARB = (PFNGLBUFFERDATAPROC) wglGetProcAddress ("glBufferDataARB")))
	PrintLog ("   glBufferDataARB not supported by the OpenGL driver\n");
else if (!(glMapBufferARB = (PFNGLMAPBUFFERPROC) wglGetProcAddress ("glMapBufferARB")))
	PrintLog ("   glMapBufferARB not supported by the OpenGL driver\n");
else if (!(glUnmapBufferARB = (PFNGLUNMAPBUFFERPROC) wglGetProcAddress ("glUnmapBufferARB")))
	PrintLog ("   glUnmapBufferARB not supported by the OpenGL driver\n");
else if (!(glDeleteBuffersARB = (PFNGLDELETEBUFFERSPROC) wglGetProcAddress ("glDeleteBuffersARB")))
	PrintLog ("   glDeleteBuffersARB not supported by the OpenGL driver\n");
else if (!(glDrawRangeElements = (PFNGLDRAWRANGEELEMENTSPROC) wglGetProcAddress ("glDrawRangeElements")))
	PrintLog ("   glDrawRangeElements not supported by the OpenGL driver\n");
else
#	endif
#endif
gameStates.ogl.bHaveVBOs = 1;
PrintLog (gameStates.ogl.bHaveVBOs ? (char *) "VBOs are available\n" : (char *) "No VBOs available\n");
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
#ifndef GL_VERSION_20
gameStates.ogl.bMultiTexturingOk = 0;
PrintLog ("Checking multi-texturing ...\n");
#	ifdef _WIN32
if (!(glMultiTexCoord2d	= (PFNGLMULTITEXCOORD2DARBPROC) wglGetProcAddress ("glMultiTexCoord2dARB")))
	PrintLog ("   glMultiTexCoord2d not supported by the OpenGL driver\n");
else if (!(glMultiTexCoord2f = (PFNGLMULTITEXCOORD2FARBPROC) wglGetProcAddress ("glMultiTexCoord2fARB")))
	PrintLog ("   glMultiTexCoord2f not supported by the OpenGL driver\n");
else if (!(glMultiTexCoord2fv = (PFNGLMULTITEXCOORD2FVARBPROC) wglGetProcAddress ("glMultiTexCoord2fvARB")))
	PrintLog ("   glMultiTexCoord2fv not supported by the OpenGL driver\n");
else
#	endif
if (!(glActiveTexture = (PFNGLACTIVETEXTUREARBPROC) wglGetProcAddress ("glActiveTextureARB")))
	PrintLog ("   glActiveTexture not supported by the OpenGL driver\n");
else if (!(glClientActiveTexture = (PFNGLCLIENTACTIVETEXTUREARBPROC) wglGetProcAddress ("glClientActiveTextureARB")))
	PrintLog ("   glClientActiveTexture not supported by the OpenGL driver\n");
else
#endif
gameStates.ogl.bMultiTexturingOk = 1;
PrintLog (gameStates.ogl.bMultiTexturingOk ? (char *) "Multi-texturing is available\n" : (char *) "No multi-texturing available\n");
}

//------------------------------------------------------------------------------

void OglInitAntiAliasing (void)
{
gameStates.ogl.bAntiAliasingOk = (pszOglExtensions && strstr (pszOglExtensions, "GL_ARB_multisample"));
}

//------------------------------------------------------------------------------

void OglInitRefreshSync (void)
{
gameStates.render.bVSyncOk = 0;
#ifdef _WIN32
if (!(wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC) wglGetProcAddress ("wglSwapIntervalEXT")))
	 PrintLog ("V-Sync not supported by the OpenGL driver\n");
else
	gameStates.render.bVSyncOk = 1;
#endif
}

//------------------------------------------------------------------------------

void OglInitExtensions (void)
{
pszOglExtensions = reinterpret_cast<const char*> (glGetString (GL_EXTENSIONS));
OglInitMultiTexturing ();
OglInitShaders ();
OglInitOcclusionQuery ();
OglInitPointSprites ();
OglInitTextureCompression ();
OglInitStencilOps ();
OglInitRefreshSync ();
OglInitAntiAliasing ();
OglInitVBOs ();
#if RENDER2TEXTURE == 1
OglInitPBuffer ();
#elif RENDER2TEXTURE == 2
CFBO::Setup ();
#endif
if (!(gameOpts->render.bUseShaders && gameStates.ogl.bShadersOk)) {
	gameOpts->ogl.bGlTexMerge = 0;
	gameStates.ogl.bLowMemory = 0;
	gameStates.ogl.bHaveTexCompression = 0;
	}
}

//------------------------------------------------------------------------------

