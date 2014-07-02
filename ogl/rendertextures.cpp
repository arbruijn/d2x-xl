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
#include "glow.h"
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
#include "gpgpu_lighting.h"
#include "postprocessing.h"

// -----------------------------------------------------------------------------------

#define GL_DEPTH_STENCIL_EXT			0x84F9
#define GL_UNSIGNED_INT_24_8_EXT		0x84FA
#define GL_DEPTH24_STENCIL8_EXT		0x88F0
#define GL_TEXTURE_STENCIL_SIZE_EXT 0x88F1

GLuint COGL::CreateDepthTexture (int32_t nTMU, int32_t bFBO, int32_t nType, int32_t nWidth, int32_t nHeight)
{
	GLuint	hBuffer;

if (nTMU >= GL_TEXTURE0)
	SelectTMU (nTMU);
SetTexturing (true);
GenTextures (1, &hBuffer);
if (glGetError ())
	return hBuffer = 0;
BindTexture (hBuffer);
glTexParameteri (GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
glTexParameteri (GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY); 	
//glTexParameteri (GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE);
if (nType == 1) // depth + stencil
	glTexImage2D (GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8_EXT, (nWidth > 0) ? nWidth : DrawBufferWidth (), (nHeight > 0) ? nHeight : DrawBufferHeight (), 
					  0, GL_DEPTH_STENCIL_EXT, GL_UNSIGNED_INT_24_8_EXT, NULL);
else
	glTexImage2D (GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, (nWidth > 0) ? nWidth : DrawBufferWidth (), (nHeight > 0) ? nHeight : DrawBufferHeight (), 
					  0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
if (glGetError ()) {
	DeleteTextures (1, &hBuffer);
	return hBuffer = 0;
	}
ogl.BindTexture (0);
return hBuffer;
}

// -----------------------------------------------------------------------------------

void COGL::DestroyDepthTexture (int32_t bFBO)
{
if (m_states.hDepthBuffer [bFBO]) {
	DeleteTextures (1, &m_states.hDepthBuffer [bFBO]);
	m_states.hDepthBuffer [bFBO] = 0;
	}
}

// -----------------------------------------------------------------------------------

GLuint COGL::CopyDepthTexture (int32_t nId, int32_t nTMU)
{
	static int32_t nSamples = -1;
	static int32_t nDuration = 0;

	GLenum nError = glGetError ();

#if DBG
if (nError)
	nError = nError;
#endif
if (!gameOpts->render.bUseShaders)
	return m_states.hDepthBuffer [nId] = 0;
if (ogl.m_features.bDepthBlending < 0) // too slow on the current hardware
	return m_states.hDepthBuffer [nId] = 0;
int32_t t = (nSamples >= 5) ? -1 : SDL_GetTicks ();
SelectTMU (nTMU);
SetTexturing (true);
if (!m_states.hDepthBuffer [nId])
	m_states.bDepthBuffer [nId] = 0;
if (m_states.hDepthBuffer [nId] || (m_states.hDepthBuffer [nId] = CreateDepthTexture (-1, nId, nId))) {
	BindTexture (m_states.hDepthBuffer [nId]);
	if (!m_states.bDepthBuffer [nId]) {
#if 0
		if (ogl.StereoDevice () == -GLASSES_SHUTTER_NVIDIA)
			ogl.SetReadBuffer ((ogl.StereoSeparation () < 0) ? GL_BACK_LEFT : GL_BACK_RIGHT, 0);
		else
#endif
			ogl.SetReadBuffer (GL_BACK, (gameStates.render.bRenderIndirect > 0) || gameStates.render.cameras.bActive); // side effect: If the current render target is an FBO, it will be activated!
		SaveViewport ();
		SetViewport (0, 0, gameData.render.screen.Width (), gameData.render.screen.Height ());
		glCopyTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, 0, 0, gameData.render.screen.Width (), gameData.render.screen.Height ());
		RestoreViewport ();
		if ((nError = glGetError ())) {
			DestroyDepthTexture (nId);
			return m_states.hDepthBuffer [nId] = 0;
			}
		m_states.bDepthBuffer [nId] = 1;
		gameData.render.nStateChanges++;
		}
	}
if (t > 0) {
	if (++nSamples > 0) {
		nDuration += SDL_GetTicks () - t;
		if ((nSamples >= 5) && (nDuration / nSamples > 10)) {
			PrintLog (0, "Disabling depth buffer reads (average read time: %d ms)\n", nDuration / nSamples);
			ogl.m_features.bDepthBlending = -1;
			DestroyDepthTexture (nId);
			m_states.hDepthBuffer [nId] = 0;
			}
		}
	}
return m_states.hDepthBuffer [nId];
}

// -----------------------------------------------------------------------------------

GLuint COGL::CreateColorTexture (int32_t nTMU, int32_t bFBO)
{
	GLuint	hBuffer;

if (nTMU > GL_TEXTURE0)
	SelectTMU (nTMU);
SetTexturing (true);
GenTextures (1, &hBuffer);
if (glGetError ())
	return hBuffer = 0;
BindTexture (hBuffer);
glTexParameteri (GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE);
glTexImage2D (GL_TEXTURE_2D, 0, GL_RGB, DrawBufferWidth (), DrawBufferHeight (), 0, GL_RGB, GL_UNSIGNED_SHORT, NULL);
if (glGetError ()) {
	DeleteTextures (1, &hBuffer);
	return hBuffer = 0;
	}
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
if (!bFBO) {
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
	glTexParameteri (GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE);
	}
if (glGetError ()) {
	DeleteTextures (1, &hBuffer);
	return hBuffer = 0;
	}
return hBuffer;
}

// -----------------------------------------------------------------------------------

void COGL::DestroyColorTexture (void)
{
if (ogl.m_states.hColorBuffer) {
	ogl.DeleteTextures (1, &ogl.m_states.hColorBuffer);
	ogl.m_states.hColorBuffer = 0;
	}
}

// -----------------------------------------------------------------------------------

GLuint COGL::CopyColorTexture (void)
{
	GLenum nError = glGetError ();

#if DBG
if (nError)
	nError = nError;
#endif
SelectTMU (GL_TEXTURE1);
SetTexturing (true);
if (!m_states.hColorBuffer)
	m_states.bColorBuffer = 0;
if (m_states.hColorBuffer || (m_states.hColorBuffer = CreateColorTexture (-1, 0))) {
	BindTexture (m_states.hColorBuffer);
	if (!m_states.bColorBuffer) {
		glCopyTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, 0, 0, gameData.render.screen.Width (), gameData.render.screen.Height ());
		if ((nError = glGetError ())) {
			glCopyTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, 0, 0, gameData.render.screen.Width (), gameData.render.screen.Height ());
			if ((nError = glGetError ())) {
				DestroyColorTexture ();
				return m_states.hColorBuffer = 0;
				}
			}
		m_states.bColorBuffer = 1;
		gameData.render.nStateChanges++;
		}
	}
return m_states.hColorBuffer;
}

// -----------------------------------------------------------------------------------

#if 0

GLuint COGL::CreateStencilTexture (int32_t nTMU, int32_t bFBO)
{
	GLuint	hBuffer;

if (nTMU > 0)
	SelectTMU (nTMU);
ogl.SetTexturing (true);
GenTextures (1, &hBuffer);
if (glGetError ())
	return hDepthBuffer = 0;
ogl.BindTexture (hBuffer);
glTexParameteri (GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE);
glTexImage2D (GL_TEXTURE_2D, 0, GL_STENCIL_COMPONENT8, DrawBufferWidth (), DrawBufferHeight (),
				  0, GL_STENCIL_COMPONENT, GL_UNSIGNED_BYTE, NULL);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
if (glGetError ()) {
	DeleteTextures (1, &hBuffer);
	return hBuffer = 0;
	}
return hBuffer;
}

#endif

//------------------------------------------------------------------------------

