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

#include "descent.h"
#include "ogl_defs.h"
#include "ogl_lib.h"
#include "ogl_shader.h"

//------------------------------------------------------------------------------


const char *pszOglExtensions = NULL;

//------------------------------------------------------------------------------

void COGL::SetupOcclusionQuery (void)
{
m_available.bOcclusionQuery = (pszOglExtensions && strstr (pszOglExtensions, "GL_ARB_occlusion_query"));
PrintLog (m_available.bOcclusionQuery ? (char *) "Occlusion query is available\n" : (char *) "No occlusion query available\n");
}

//------------------------------------------------------------------------------

void COGL::SetupPointSprites (void)
{
}

//------------------------------------------------------------------------------

void COGL::SetupStencilOps (void)
{
ogl.SetStencilTest (true);
ogl.m_available.bSeparateStencilOps = 0;
if ((ogl.m_available.bStencilBuffer = glIsEnabled (GL_STENCIL_TEST))) {
	SetStencilTest (false);
	if (pszOglExtensions) {
		if (strstr (pszOglExtensions, "GL_ATI_separate_stencil"))
			ogl.m_available.bSeparateStencilOps = 1;
		else if (strstr (pszOglExtensions, "GL_EXT_stencil_two_side"))
			ogl.m_available.bSeparateStencilOps = 2;
		}
	}
}

//------------------------------------------------------------------------------

void COGL::SetupVBOs (void)
{
m_available.bVertexBufferObjects = (pszOglExtensions && strstr (pszOglExtensions, "GL_ARB_vertex_buffer_object"));
PrintLog (m_available.bVertexBufferObjects ? (char *) "VBOs are available\n" : (char *) "No VBOs available\n");
}

//------------------------------------------------------------------------------

void COGL::SetupTextureArrays (void)
{
m_available.bTextureArrays = (pszOglExtensions && strstr (pszOglExtensions, "GL_EXT_texture_array"));
PrintLog (m_available.bTextureArrays ? (char *) "Multi-texturing is available\n" : (char *) "No multi-texturing available\n");
}

//------------------------------------------------------------------------------

void COGL::SetupTextureCompression (void)
{
#if TEXTURE_COMPRESSION
m_available.bTextureCompression = 1;
#else
m_available.bTextureCompression = 0;
#endif
}

//------------------------------------------------------------------------------

void COGL::SetupMultiTexturing (void)
{
m_available.bMultiTexturing = (pszOglExtensions && strstr (pszOglExtensions, "GL_ARB_multitexture"));
PrintLog (m_available.bMultiTexturing ? (char *) "Multi-texturing is available\n" : (char *) "No multi-texturing available\n");
}

//------------------------------------------------------------------------------

void COGL::SetupAntiAliasing (void)
{
m_available.bAntiAliasing = (pszOglExtensions && strstr (pszOglExtensions, "GL_ARB_multisample"));
}

//------------------------------------------------------------------------------

void COGL::SetupMRT (void)
{
m_available.bMultipleRenderTargets = (pszOglExtensions && strstr (pszOglExtensions, "GL_ARB_draw_buffers"));
}

//------------------------------------------------------------------------------

void COGL::SetupRefreshSync (void)
{
#ifdef _WIN32
gameStates.render.bVSyncOk = 1;
#else
gameStates.render.bVSyncOk = 0;
#endif
}

//------------------------------------------------------------------------------

void COGL::SetupExtensions (void)
{
pszOglExtensions = reinterpret_cast<const char*> (glGetString (GL_EXTENSIONS));
glewInit ();
SetupMultiTexturing ();
SetupShaders ();
SetupOcclusionQuery ();
SetupPointSprites ();
SetupTextureCompression ();
SetupStencilOps ();
SetupRefreshSync ();
SetupAntiAliasing ();
SetupMRT ();
SetupVBOs ();
#if RENDER2TEXTURE == 1
SetupPBuffer ();
#elif RENDER2TEXTURE == 2
CFBO::Setup ();
#endif
if (!(gameOpts->render.bUseShaders && m_available.bShaders)) {
	gameOpts->ogl.bGlTexMerge = 0;
	m_states.bLowMemory = 0;
	m_available.bTextureCompression = 0;
	}
}

//------------------------------------------------------------------------------

