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

tTexCoord2f quadTexCoord [3][4] = {
	{{{0,0}},{{1,0}},{{1,1}},{{0,1}}},
	{{{0,0}},{{0.5f,0}},{{0.5f,1}},{{0,1}}},
	{{{0.5f,0}},{{1,0}},{{1,1}},{{0.5f,1}}}
	};

float quadVerts [5][4][2] = {
	{{0,0},{1,0},{1,1},{0,1}},
	{{0,0},{0.5f,0},{0.5f,1},{0,1}},
	{{0.5f,0},{1,0},{1,1},{0.5f,1}},
	{{0,0},{0.25f,0},{0.25f,0.5f},{0,0.5f}},
	{{0.25f,0},{0.5f,0},{0.5f,0.5f},{0.25f,0.5f}}
	};

//------------------------------------------------------------------------------

static tProfilerData p;
static float nFrameCount = 1;

static float PrintTime (char* pszLabel, tProfilerTags tag, int nLine)
{
	char szPad [200];

int h = fontManager.Current ()->Height () + 3;
float t;
GrPrintF (NULL, 5, h * nLine, fontManager.Current ()->PadString (szPad, pszLabel, " .", 190));
sprintf (szPad, ": %05.2f %c (%05.2f)", t = 100.0f * float (p.t [tag]) / float (p.t [ptFrame]), '%', float (p.t [tag]) / nFrameCount);

int x = 196;
char c [2] = {'\0', '\0'};

for (char* ps = szPad; *ps; ps++) {
	int sw, sh, aw;
	c [0] = *ps;
	fontManager.Current ()->StringSize (c, sw, sh, aw);
	if (isdigit (*ps)) {
		GrString (x + 10 - sw, h * nLine, c, NULL);
		x += 10;
		}
	else {
		GrString (x, h * nLine, c, NULL);
		x += sw;
		}
	}
return t;
}

//------------------------------------------------------------------------------

static void PrintStatistics (void)
{
#if PROFILING
if (gameStates.render.bShowProfiler && !gameStates.menus.nInMenu && fontManager.Current () && SMALL_FONT) {
	static time_t t0 = -1000;
	time_t t1 = clock ();

	if (((gameStates.render.bShowProfiler == 1) && (t1 - t0 >= 10)) || ((gameStates.render.bShowProfiler == 2) && (t1 - t0 >= 1000))) {
		memcpy (&p, &gameData.profiler, sizeof (p));
		nFrameCount = float (gameData.app.nFrameCount);
		t0 = t1;
		}
	fontManager.SetCurrent (SMALL_FONT);
	int h = fontManager.Current ()->Height () + 3;
	fontManager.SetColorRGBi (ORANGE_RGBA, 1, 0, 0);
	fontManager.SetColorRGBi (GOLD_RGBA, 1, 0, 0);
	CFont* fontP = fontManager.Current ();

	int nLine = 3;
	float s = 0;
	char szPad [200];

	GrPrintF (NULL, 5, h * nLine, fontManager.Current ()->PadString (szPad, "  frame ", " .", 190));
	GrPrintF (NULL, 196, h * nLine++, ": %05.2f", float (p.t [ptFrame]) / nFrameCount);
	if (p.t [ptRenderFrame]) {
		PrintTime ("  game states", ptGameStates, nLine++);
		fontManager.SetColorRGBi (ORANGE_RGBA, 1, 0, 0);
		PrintTime ("     object states", ptUpdateObjects, nLine++);
		PrintTime ("        physics", ptPhysics, nLine++);
		fontManager.SetColorRGBi (GOLD_RGBA, 1, 0, 0);
		PrintTime ("  scene", ptRenderFrame, nLine++);
		PrintTime ("    mine", ptRenderMine, nLine++);
		fontManager.SetColorRGBi (ORANGE_RGBA, 1, 0, 0);
		s += PrintTime ("      light", ptLighting, nLine++);
		s += PrintTime ("      render", ptRenderPass, nLine++);
		s += PrintTime ("      face list", ptFaceList, nLine++);
		s += PrintTime ("      faces", ptRenderFaces, nLine++);
		s += PrintTime ("      objects", ptRenderObjects, nLine++);
		s += PrintTime ("      transparency", ptTranspPolys, nLine++);
		s += PrintTime ("      effects", ptEffects, nLine++);
		s += PrintTime ("        particles", ptParticles, nLine++);
		s += PrintTime ("      cockpit", ptCockpit, nLine++);
		s += PrintTime ("      states", ptRenderStates, nLine++);
		s += PrintTime ("      shaders", ptShaderStates, nLine++);
		s += PrintTime ("      other", ptAux, nLine++);
		s += PrintTime ("      transform", ptTransform, nLine++);
		s += PrintTime ("      seg list", ptBuildSegList, nLine++);
		s += PrintTime ("      obj list", ptBuildObjList, nLine++);

		fontManager.SetColorRGBi (GOLD_RGBA, 1, 0, 0);
		GrPrintF (NULL, 5, h * nLine, fontManager.Current ()->PadString (szPad, "      total", " .", 190));
		GrPrintF (NULL, 196, h * nLine++, ": %05.2f %c", s, '%');
		}
	if (gameStates.render.bShowProfiler == 1)
		memset (&gameData.profiler, 0, sizeof (gameData.profiler));
	fontManager.SetCurrent (GAME_FONT);
	}
#endif
}

//------------------------------------------------------------------------------

void COGL::Update (int bClear)
{
if (m_states.bInitialized) {
	if (m_states.nDrawBuffer == GL_FRONT)
		glFlush ();
	else
		SwapBuffers (1, bClear);
	}
}

//------------------------------------------------------------------------------

void COGL::SwapBuffers (int bForce, int bClear)
{
if (gameStates.app.bGameRunning)	{
	PrintStatistics ();
	glowRenderer.End ();
	if (gameStates.render.bRenderIndirect > 0) 
		FlushDrawBuffer ();
	if (StereoDevice () >= 0) {
		int bRenderIndirect = gameStates.render.bRenderIndirect = 0;
		Draw2DFrameElements ();
		gameStates.render.bRenderIndirect = bRenderIndirect;
		}
	}
else if (IsSideBySideDevice () || (gameStates.render.bRenderIndirect > 0))
	FlushDrawBuffer ();
SDL_GL_SwapBuffers ();
if (gameStates.app.bSaveScreenShot)
	SaveScreenShot (NULL, 0);
SetupCanvasses (-1.0f);
SetDrawBuffer (GL_BACK, gameStates.render.bRenderIndirect > 0);
#if 0
if (gameStates.menus.nInMenu || bClear)
	glClear (GL_COLOR_BUFFER_BIT);
#endif
}

//------------------------------------------------------------------------------
// render post processed effects
// if no or hardware (shutter glasses / Oculus Rift) based stereo rendering,
// use render buffer 0 as source, otherwise (anaglyph stereo) use render buffer 2 as source
// if hardware (shutter glasses / Oculus Rift) based stereo rendering,
// use render buffer 1 as destination, otherwise use hardware draw buffer as destination

void COGL::FlushEffects (int nEffects)
{
//ogl.EnableClientStates (1, 0, 0, GL_TEXTURE1);
if (nEffects & 5) {
	if (StereoDevice () > -DEVICE_STEREO_SIDEBYSIDE) {
		ogl.EnableClientStates (1, 0, 0, GL_TEXTURE0);
		OglTexCoordPointer (2, GL_FLOAT, 0, quadTexCoord [0]);
		OglVertexPointer (2, GL_FLOAT, 0, quadVerts [0]);
		if (nEffects & 4) // shutter glasses or Oculus Rift
			SelectDrawBuffer (1); // render effects to texture 1
		else
			SetDrawBuffer (GL_BACK, 0);
		ogl.BindTexture (DrawBuffer ((nEffects & 2) ? 2 : 0)->ColorBuffer ());
		gameData.render.screen.SetScale (1.0f);
		gameData.render.screen.Activate ("COGL::FlushEffects (screen)");
		postProcessManager.Setup ();
		postProcessManager.Render ();
		if (nEffects & 4) // shutter glasses
			ogl.BindTexture (DrawBuffer (1)->ColorBuffer ());
		gameData.render.screen.Deactivate ();
		}
	else {
		SelectBlurBuffer (0); 
		SetBlendMode (OGL_BLEND_REPLACE);
		SetDepthMode (GL_ALWAYS);
		for (int i = 0; i < 2; i++) {
			gameData.SetStereoSeparation (i ? STEREO_RIGHT_FRAME : STEREO_LEFT_FRAME);
			SetupCanvasses ();
			gameData.render.frame.Activate ("COGL::FlushEffects (frame)");
			ogl.EnableClientStates (1, 0, 0, GL_TEXTURE0);
			OglTexCoordPointer (2, GL_FLOAT, 0, quadTexCoord [i + 1]);
			OglVertexPointer (2, GL_FLOAT, 0, quadVerts [0]);
			ogl.BindTexture (DrawBuffer (0)->ColorBuffer ());
			if (nEffects & 1) {
				postProcessManager.Setup ();
				postProcessManager.Render ();
				}
			else
				OglDrawArrays (GL_QUADS, 0, 4);
			gameData.render.frame.Deactivate ();
			}
		gameData.render.screen.SetScale (1.0f);
		}
	}
}

//------------------------------------------------------------------------------

void COGL::FlushDrawBuffer (bool bAdditive)
{
if (HaveDrawBuffer ()) {
	int nEffects = postProcessManager.HaveEffects () 
						+ (int (StereoDevice () > 0) << 1)
						+ (int (StereoDevice () < 0) << 2)
#if MAX_SHADOWMAPS
						+ (int (EGI_FLAG (bShadows, 0, 1, 0) != 0) << 3)
#endif
						;

	glColor3f (1,1,1);

	EnableClientStates (1, 0, 0, GL_TEXTURE0);
	BindTexture (DrawBuffer (0)->ColorBuffer ()); // set source for subsequent rendering step
	OglTexCoordPointer (2, GL_FLOAT, 0, quadTexCoord [0]);
	OglVertexPointer (2, GL_FLOAT, 0, quadVerts [0]);

	if (nEffects & 5) {
		FlushEffects (nEffects);
		FlushStereoBuffers (nEffects);
		}
	else if (nEffects & 3) {
		FlushStereoBuffers (nEffects);
		FlushEffects (nEffects);
#if MAX_SHADOWMAPS
		FlushShadowMaps (nEffects);
#endif
		}
	else {
		SetDrawBuffer (GL_BACK, 0);
		gameData.render.screen.Activate ("COGL::FlushDrawBuffer (screen)");
		OglDrawArrays (GL_QUADS, 0, 4);
		gameData.render.screen.Deactivate ();
		}
	ResetClientStates (0);
	SelectDrawBuffer (0);
	//if (bStereo)
		shaderManager.Deploy (-1);
	}
}

//------------------------------------------------------------------------------

