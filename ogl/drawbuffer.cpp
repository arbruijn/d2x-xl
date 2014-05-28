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

//#define _WIN32_WINNT		0x0600

//------------------------------------------------------------------------------

int COGL::DrawBufferWidth (void)
{
return int (gameData.render.screen.Scaled (m_states.nCurWidth));
}

int COGL::DrawBufferHeight (void)
{
return int (gameData.render.screen.Scaled (m_states.nCurHeight));
}

//------------------------------------------------------------------------------

void COGL::CreateDrawBuffer (int nType)
{
if (!m_features.bRenderToTexture)
	return;
if ((gameStates.render.bRenderIndirect <= 0) && (nType >= 0))
	return;
if (DrawBuffer ()->Handle () && !DrawBuffer ()->Resize (DrawBufferWidth (), DrawBufferHeight ()))
	return;
PrintLog (1, "creating draw buffer\n");
DrawBuffer ()->Create (DrawBufferWidth (), DrawBufferHeight (), nType, (nType != 1) ? 1 : 1 + m_features.bMultipleRenderTargets);
PrintLog (-1);
}

//------------------------------------------------------------------------------

void COGL::DestroyDrawBuffer (void)
{
#	if 1
	static int bSemaphore = 0;

if (bSemaphore)
	return;
bSemaphore++;
#	endif
if (m_features.bRenderToTexture && DrawBuffer () && DrawBuffer ()->Handle ()) {
	SetDrawBuffer (GL_BACK, 0);
	DrawBuffer ()->Destroy ();
	}
#	if 1
bSemaphore--;
#	endif
}

//------------------------------------------------------------------------------

void COGL::DestroyDrawBuffers (void)
{
for (int i = m_data.drawBuffers.Length () - 1; i >= 0; i--) {
	if (m_data.drawBuffers [i].Handle ()) {
		SelectDrawBuffer (i);
		DestroyDrawBuffer ();
		}
	}
}

//------------------------------------------------------------------------------

void COGL::SetDrawBuffer (int nBuffer, int bFBO)
{
#if 1
	static int bSemaphore = 0;

if (bSemaphore)
	return;
bSemaphore++;
#endif

if (bFBO && (nBuffer == GL_BACK) && m_features.bRenderToTexture && DrawBuffer ()->Handle ()) {
	if (DrawBuffer ()->Active ()) 
		DrawBuffer ()->SelectColorBuffers ();
	else if (!DrawBuffer ()->Enable ()) {
		DestroyDrawBuffers ();
		SelectDrawBuffer (0);
		glDrawBuffer (GL_BACK);
		}
	}
else {
	if (DrawBuffer ()->Active ())
		DrawBuffer ()->Disable ();
	glDrawBuffer (m_states.nDrawBuffer = nBuffer);
	}

#if 1
bSemaphore--;
#endif
}

//------------------------------------------------------------------------------

void COGL::SetReadBuffer (int nBuffer, int bFBO)
{
if (bFBO && (nBuffer == GL_BACK) && m_features.bRenderToTexture && DrawBuffer ()->Handle ()) {
	if (DrawBuffer ()->Active () || DrawBuffer ()->Enable ())
		glReadBuffer (GL_COLOR_ATTACHMENT0_EXT);
	else
		glReadBuffer (GL_BACK);
	}
else {
	if (DrawBuffer ()->Active ())
		DrawBuffer ()->Disable ();
	glReadBuffer (nBuffer);
	}
}

//------------------------------------------------------------------------------

int COGL::SelectDrawBuffer (int nBuffer, int nColorBuffers) 
{ 
//if (gameStates.render.nShadowMap > 0)
//	nBuffer = gameStates.render.nShadowMap + 5;
int nPrevBuffer = (m_states.nCamera < 0) 
						? m_states.nCamera 
						: (m_data.drawBufferP && m_data.drawBufferP->Active () && !m_data.drawBufferP->Resize (DrawBufferWidth (), DrawBufferHeight ())) 
							? int (m_data.drawBufferP - m_data.drawBuffers.Buffer ()) 
							: 0x7FFFFFFF;

CCamera* cameraP;

if (nBuffer != nPrevBuffer) {
	if (m_data.drawBufferP)
		m_data.drawBufferP->Disable ();
	if (nBuffer >= 0) {
		m_states.nCamera = 0;
		m_data.drawBufferP = m_data.GetDrawBuffer (nBuffer); 
		CreateDrawBuffer ((nBuffer < 2) ? 1 : (nBuffer < 3) ? -1 : -2);
		}
	else if ((cameraP = cameraManager [-nBuffer - 1])) {
		m_states.nCamera = nBuffer;
		m_data.drawBufferP = &cameraP->FrameBuffer ();
		}
	}
return m_data.drawBufferP->Enable (nColorBuffers) ? nPrevBuffer : -1;
}

//------------------------------------------------------------------------------

void COGL::ChooseDrawBuffer (void)
{
ogl.ClearError (0);
if (/*gameStates.render.bBriefing ||*/ (gameStates.render.bRenderIndirect < 0) || (gameStates.render.nWindow [0] && !IsSideBySideDevice ())) {
	gameStates.render.bRenderIndirect = 0;
	SetDrawBuffer (GL_BACK, 0);
	}
else if (gameStates.render.cameras.bActive) {
	SelectDrawBuffer (-cameraManager.Current () - 1);
	gameStates.render.bRenderIndirect = 0;
	}
else {
	gameStates.render.bRenderIndirect = 
#if 1
		!gameStates.app.bNostalgia && m_features.bShaders && (m_features.bRenderToTexture > 0); 
#else
		(postProcessManager.Effects () != NULL) 
		|| (m_data.xStereoSeparation && (i > 0)) 
		|| (glowRenderer.Available (BLUR_SHADOW) && (EGI_FLAG (bShadows, 0, 1, 0) != 0));
#endif
	if (gameStates.render.bRenderIndirect <= 0) {
		gameOpts->render.stereo.nGlasses = 0;
		m_data.xStereoSeparation = 0;
		SetDrawBuffer (GL_BACK, 0);
		}
	else {
		ogl.ClearError (0);
		if (m_data.drawBufferP)
			m_data.drawBufferP->Disable ();
		SelectDrawBuffer (!IsSideBySideDevice () && (m_data.xStereoSeparation > 0));
		if (ogl.ClearError (0))
			gameOpts->render.stereo.nGlasses = 0;
		}
	}
}

//------------------------------------------------------------------------------

int COGL::SelectGlowBuffer (void) 
{ 
return SelectDrawBuffer (int (!IsSideBySideDevice () && (m_data.xStereoSeparation > 0)), 1) > -1;
}

//------------------------------------------------------------------------------

int COGL::SelectBlurBuffer (int nBuffer) 
{ 
return SelectDrawBuffer (nBuffer + 3) > -1;
}

//------------------------------------------------------------------------------

