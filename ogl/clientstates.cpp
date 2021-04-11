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
#include "error.h"
#include "u_mem.h"
#include "config.h"
#include "maths.h"
#include "crypt.h"
#include "strutil.h"
#include "segmath.h"
#include "light.h"
#include "dynlight.h"
#include "lightmap.h"
#include "network.h"
#include "gr.h"
#include "gamefont.h"
#include "screens.h"
#include "interp.h"
#include "ogl_defs.h"
#include "ogl_lib.h"
#include "ogl_texcache.h"
#include "ogl_color.h"
#include "ogl_shader.h"
#include "ogl_render.h"
#include "findfile.h"
#include "rendermine.h"
#include "sphere.h"
#include "glare.h"
#include "menu.h"
#include "menubackground.h"
#include "cockpit.h"
#include "renderframe.h"
#include "automap.h"
#include "text.h"
#include "postprocessing.h"
#include "gpgpu_lighting.h"

//#define _WIN32_WINNT		0x0600

#define TRACK_STATES		0

//------------------------------------------------------------------------------

#if DBG_OGL

void COGL::VertexPointer (GLint size, GLenum type, GLsizei stride, const GLvoid* pointer, const char* pszFile, int32_t nLine)
{
if (Locked ())
	return;
if ((m_data.clientBuffers [m_data.nTMU [0]][0].buffer) && (m_data.clientBuffers [m_data.nTMU [0]][0].buffer != pointer))
	return;
m_data.clientBuffers [m_data.nTMU [0]][0].buffer = pointer;
m_data.clientBuffers [m_data.nTMU [0]][0].pszFile = pszFile;
m_data.clientBuffers [m_data.nTMU [0]][0].nLine = nLine;
glVertexPointer (size, type, stride, pointer);
}

//------------------------------------------------------------------------------

void COGL::NormalPointer (GLenum type, GLsizei stride, const GLvoid* pointer, const char* pszFile, int32_t nLine)
{
if (Locked ())
	return;
if ((m_data.clientBuffers [m_data.nTMU [0]][1].buffer) && (m_data.clientBuffers [m_data.nTMU [0]][1].buffer != pointer))
	return;
m_data.clientBuffers [m_data.nTMU [0]][1].buffer = pointer;
m_data.clientBuffers [m_data.nTMU [0]][1].pszFile = pszFile;
m_data.clientBuffers [m_data.nTMU [0]][1].nLine = nLine;
glNormalPointer (type, stride, pointer);
}

//------------------------------------------------------------------------------

void COGL::ColorPointer (GLint size, GLenum type, GLsizei stride, const GLvoid* pointer, const char* pszFile, int32_t nLine)
{
if (Locked ())
	return;
if ((m_data.clientBuffers [m_data.nTMU [0]][2].buffer) && (m_data.clientBuffers [m_data.nTMU [0]][2].buffer != pointer))
	return;
m_data.clientBuffers [m_data.nTMU [0]][2].buffer = pointer;
m_data.clientBuffers [m_data.nTMU [0]][2].pszFile = pszFile;
m_data.clientBuffers [m_data.nTMU [0]][2].nLine = nLine;
glColorPointer (size, type, stride, pointer);
}

//------------------------------------------------------------------------------

void COGL::TexCoordPointer (GLint size, GLenum type, GLsizei stride, const GLvoid* pointer, const char* pszFile, int32_t nLine)
{
if (Locked ())
	return;
if ((m_data.clientBuffers [m_data.nTMU [0]][3].buffer) && (m_data.clientBuffers [m_data.nTMU [0]][3].buffer != pointer))
	return;
m_data.clientBuffers [m_data.nTMU [0]][4].buffer = pointer;
m_data.clientBuffers [m_data.nTMU [0]][4].pszFile = pszFile;
m_data.clientBuffers [m_data.nTMU [0]][4].nLine = nLine;
glTexCoordPointer (size, type, stride, pointer);
}

#endif

//------------------------------------------------------------------------------

void COGL::SelectTMU (int32_t nTMU, bool bClient)
{
#if 0 //DBG
	int32_t	i = nTMU - GL_TEXTURE0;

if ((i < 0) || (i > 3))
	return;
#endif
//SDL_mutexP (semaphore);
//if (m_data.nTMU [0] != nTMU)
	{
	glActiveTexture (nTMU);
	m_data.nTMU [0] = nTMU - GL_TEXTURE0;
	}
if (bClient) {
//	if (m_data.nTMU [1] != nTMU)
		glClientActiveTexture (nTMU);
		m_data.nTMU [1] = nTMU - GL_TEXTURE0;
	}
ClearError (0);
//SDL_mutexV (semaphore);
}

//------------------------------------------------------------------------------

int32_t COGL::EnableClientState (GLuint nState, int32_t nTMU)
{
#if DBG_OGL
if (Locked ())
	return -1;
#endif
if (nTMU >= GL_TEXTURE0)
	SelectTMU (nTMU, true);
#if TRACK_STATES
if (m_data.clientStates [m_data.nTMU [0]][nState - GL_VERTEX_ARRAY])
	return 1;
#endif
glEnableClientState (nState);
#if DBG_OGL
if (Locked ())
	BRP;
memset (&m_data.clientBuffers [m_data.nTMU [0]][nState - GL_VERTEX_ARRAY], 0, sizeof (m_data.clientBuffers [m_data.nTMU [0]][nState - GL_VERTEX_ARRAY]));
#endif
GLenum nError = glGetError ();
if (!nError) {
#if TRACK_STATES || DBG_OGL
	m_data.clientStates [m_data.nTMU [0]][nState - GL_VERTEX_ARRAY] = 1;
#endif
	return 1;
	}
DisableClientState (nState, -1);
return glGetError () == 0;
}

//------------------------------------------------------------------------------

int32_t COGL::DisableClientState (GLuint nState, int32_t nTMU)
{
#if DBG_OGL
if (Locked ())
	return -1;
#endif
if (nTMU >= GL_TEXTURE0)
	SelectTMU (nTMU, true);
#if TRACK_STATES
if (!m_data.clientStates [m_data.nTMU [0]][nState - GL_VERTEX_ARRAY])
#endif
	{
	glDisableClientState (nState);
#if TRACK_STATES || DBG_OGL
	m_data.clientStates [m_data.nTMU [0]][nState - GL_VERTEX_ARRAY] = 0;
#endif
	}
#if DBG_OGL
memset (&m_data.clientBuffers [m_data.nTMU [0]][nState - GL_VERTEX_ARRAY], 0, sizeof (m_data.clientBuffers [m_data.nTMU [0]][nState - GL_VERTEX_ARRAY]));
#endif
return glGetError () == 0;
}

//------------------------------------------------------------------------------

int32_t COGL::EnableClientStates (int32_t bTexCoord, int32_t bColor, int32_t bNormals, int32_t nTMU)
{
if (nTMU >= GL_TEXTURE0) {
	SelectTMU (nTMU, true);
	SetTexturing (true);
	}
if (!bNormals)
	DisableClientState (GL_NORMAL_ARRAY);
else if (!EnableClientState (GL_NORMAL_ARRAY, -1)) {
	DisableClientStates (0, 0, 0, -1);
	return 0;
	}
if (!bTexCoord)
	DisableClientState (GL_TEXTURE_COORD_ARRAY);
else if (!EnableClientState (GL_TEXTURE_COORD_ARRAY, -1)) {
	DisableClientStates (0, 0, 0, -1);
	return 0;
	}
if (!bColor)
	DisableClientState (GL_COLOR_ARRAY);
else if (!EnableClientState (GL_COLOR_ARRAY, -1)) {
	DisableClientStates (bTexCoord, 0, 0, -1);
	return 0;
	}
return EnableClientState (GL_VERTEX_ARRAY, -1);
}

//------------------------------------------------------------------------------

void COGL::DisableClientStates (int32_t bTexCoord, int32_t bColor, int32_t bNormals, int32_t nTMU)
{
if (nTMU >= GL_TEXTURE0)
	SelectTMU (nTMU, true);
if (bNormals)
	DisableClientState (GL_NORMAL_ARRAY);
if (bTexCoord)
	DisableClientState (GL_TEXTURE_COORD_ARRAY);
if (bColor)
	DisableClientState (GL_COLOR_ARRAY);
DisableClientState (GL_VERTEX_ARRAY);
SetTexturing (false);
}

//------------------------------------------------------------------------------

void COGL::ResetClientStates (int32_t nFirst)
{
for (int32_t i = 3; i >= nFirst; i--) {
	DisableClientStates (1, 1, 1, GL_TEXTURE0 + i);
	SetTexturing (true);
	ogl.BindTexture (0);
	if (i)
		SetTexturing (false);
	}
memset (m_data.clientStates + nFirst, 0, (4 - nFirst) * sizeof (m_data.clientStates [0]));
}

//------------------------------------------------------------------------------
//eof
