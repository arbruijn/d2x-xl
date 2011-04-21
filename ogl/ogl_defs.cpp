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
m_states.bOcclusionQuery = 1;
PrintLog (m_states.bOcclusionQuery ? (char *) "Occlusion query is available\n" : (char *) "No occlusion query available\n");
}

//------------------------------------------------------------------------------

void COGL::SetupPointSprites (void)
{
}

//------------------------------------------------------------------------------

void COGL::SetupStencilOps (void)
{
ogl.SetStencilTest (true);
if ((gameStates.render.bHaveStencilBuffer = glIsEnabled (GL_STENCIL_TEST)))
	SetStencilTest (false);
}

//------------------------------------------------------------------------------

void COGL::SetupVBOs (void)
{
m_states.bHaveVBOs = 1;
PrintLog (m_states.bHaveVBOs ? (char *) "VBOs are available\n" : (char *) "No VBOs available\n");
}

//------------------------------------------------------------------------------

void COGL::SetupTextureCompression (void)
{
#if TEXTURE_COMPRESSION
m_states.bHaveTexCompression = 1;
#else
m_states.bHaveTexCompression = 0;
#endif
}

//------------------------------------------------------------------------------

void COGL::SetupMultiTexturing (void)
{
m_states.bMultiTexturingOk = 1;
PrintLog (m_states.bMultiTexturingOk ? (char *) "Multi-texturing is available\n" : (char *) "No multi-texturing available\n");
}

//------------------------------------------------------------------------------

void COGL::SetupAntiAliasing (void)
{
m_states.bAntiAliasingOk = (pszOglExtensions && strstr (pszOglExtensions, "GL_ARB_multisample"));
}

//------------------------------------------------------------------------------

void COGL::SetupMRT (void)
{
m_states.bMRTOk = (pszOglExtensions && strstr (pszOglExtensions, "GL_ARB_draw_buffers"));
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
if (!(gameOpts->render.bUseShaders && m_states.bShadersOk)) {
	gameOpts->ogl.bGlTexMerge = 0;
	m_states.bLowMemory = 0;
	m_states.bHaveTexCompression = 0;
	}
}

//------------------------------------------------------------------------------

