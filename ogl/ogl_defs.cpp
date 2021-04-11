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
#	pragma pack(push)
#	pragma pack(8)
#	include <windows.h>
#	pragma pack(pop)
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
m_features.bOcclusionQuery = (pszOglExtensions && strstr (pszOglExtensions, "GL_ARB_occlusion_query"));
PrintLog (0, m_features.bOcclusionQuery ? (char *) "Occlusion query is available\n" : (char *) "No occlusion query available\n");
}

//------------------------------------------------------------------------------

void COGL::SetupPointSprites (void)
{
}

//------------------------------------------------------------------------------

void COGL::SetupStencilOps (void)
{
ogl.SetStencilTest (true);
ogl.m_features.bSeparateStencilOps = 0;
#if 1
if ((ogl.m_features.bStencilBuffer.Available (glIsEnabled (GL_STENCIL_TEST)))) {
	SetStencilTest (false);
	if (pszOglExtensions) {
		if (strstr (pszOglExtensions, "GL_ATI_separate_stencil"))
			ogl.m_features.bSeparateStencilOps = 1;
		else if (strstr (pszOglExtensions, "GL_EXT_stencil_two_side"))
			ogl.m_features.bSeparateStencilOps = 2;
		}
	}
#endif
}

//------------------------------------------------------------------------------

void COGL::SetupVBOs (void)
{
m_features.bVertexBufferObjects = (pszOglExtensions && strstr (pszOglExtensions, "GL_ARB_vertex_buffer_object"));
PrintLog (0, m_features.bVertexBufferObjects ? (char *) "VBOs are available\n" : (char *) "No VBOs available\n");
}

//------------------------------------------------------------------------------

void COGL::SetupTextureArrays (void)
{
m_features.bTextureArrays = (pszOglExtensions && strstr (pszOglExtensions, "GL_EXT_texture_array"));
PrintLog (0, m_features.bTextureArrays ? (char *) "Multi-texturing is available\n" : (char *) "No multi-texturing available\n");
}

//------------------------------------------------------------------------------

void COGL::SetupTextureCompression (void)
{
#if TEXTURE_COMPRESSION
m_features.bTextureCompression = 1;
#else
m_features.bTextureCompression = 0;
#endif
}

//------------------------------------------------------------------------------

void COGL::SetupMultiTexturing (void)
{
m_features.bMultiTexturing = (pszOglExtensions && strstr (pszOglExtensions, "GL_ARB_multitexture"));
PrintLog (0, m_features.bMultiTexturing ? (char *) "Multi-texturing is available\n" : (char *) "No multi-texturing available\n");
}

//------------------------------------------------------------------------------

void COGL::SetupAntiAliasing (void)
{
m_features.bAntiAliasing = (pszOglExtensions && strstr (pszOglExtensions, "GL_ARB_multisample"));
}

//------------------------------------------------------------------------------

void COGL::SetupMRT (void)
{
m_features.bMultipleRenderTargets = (pszOglExtensions && strstr (pszOglExtensions, "GL_ARB_draw_buffers"));
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
if (!(gameOpts->render.bUseShaders && m_features.bShaders)) {
	gameOpts->ogl.bGlTexMerge = 0;
	m_states.bLowMemory = 0;
	m_features.bTextureCompression = 0;
	}
}

//------------------------------------------------------------------------------

