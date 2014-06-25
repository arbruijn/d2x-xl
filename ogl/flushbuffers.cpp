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

#if PROFILING

static tProfilerData p [2];

typedef struct tProfilerTagLabels {
	char	szLabel [30];
	char	szPad [200];
	char*	pszPad;
	int	nPadOffset;
} tProfilerTagLabels;

static tProfilerTagLabels szTags [ptTagCount] = {
	{"  frame", "", NULL, 0},
	{"  scene", "", NULL, 0},
	{"    mine", "", NULL, 0},
	{"      seg list", "", NULL, 0},
	{"      obj list", "", NULL, 0},
	{"      light", "", NULL, 0},
	{"      render", "", NULL, 0},
	{"        face list", "", NULL, 0},
	{"        faces", "", NULL, 0},
	{"          vertex color", "", NULL, 0},
	{"          per pixel light", "", NULL, 0},
	{"        objects", "", NULL, 0},
	{"          object meshes", "", NULL, 0},
	{"        states", "", NULL, 0},
	{"        shaders", "", NULL, 0},
	{"        transparency", "", NULL, 0},
	{"      effects", "", NULL, 0},
	{"        particles", "", NULL, 0},
	{"      cockpit", "", NULL, 0},
	{"      transform", "", NULL, 0},
	{"      other", "", NULL, 0},
	{"    game states", "", NULL, 0},
	{"      object states",  "", NULL, 0},
	{"        physics", "", NULL, 0}
};

static int profilerValueOffset = 0;

//------------------------------------------------------------------------------

static void PrepareTagLabels (void)
{
if (profilerValueOffset)
	return;

int	i, w, h, aw, wMax = 0;
CFont* fontP = fontManager.Current ();

for (i = (int) ptFrame; i < (int) ptTagCount; i++) {
	fontP->StringSize (szTags [i].szLabel, w, h, aw);
	if (profilerValueOffset < w)
		profilerValueOffset = w;
	}
fontP->StringSize (" .", w, h, aw);
profilerValueOffset += w + 5;

for (i = (int) ptFrame; i < (int) ptTagCount; i++) {
	fontP->PadString (szTags [i].szPad, szTags [i].szLabel, " .", profilerValueOffset);
	szTags [i].pszPad = strstr (szTags [i].szPad, " .");
	*szTags [i].pszPad++ = '\0';
	fontP->StringSize (szTags [i].pszPad, w, h, aw);
	szTags [i].nPadOffset = profilerValueOffset - w;
	}
}

//------------------------------------------------------------------------------

static float PrintTime (tProfilerTags tag, int nLine, int bStyle = 0)
{
	char szValue [200];
	int h = fontManager.Current ()->Height () + 3;
	float t = 0.0f;

#if 0

GrPrintF (NULL, 5, h * nLine, fontManager.Current ()->PadString (szValue, pszLabel, " .", 190));
#else
PrepareTagLabels ();
GrString (5, h * nLine, szTags [tag].szLabel);
GrString (szTags [tag].nPadOffset, h * nLine, szTags [tag].pszPad);
#endif

if (bStyle == 0)
	sprintf (szValue, ": %05.2f %c (%05.2f)", t = 100.0f * float (p [0].t [tag]) / float (p [0].t [ptFrame]), '%', float (p [0].t [tag]) / p [0].nFrameCount);
else if (bStyle == 1)
	sprintf (szValue, ": %05.2f", float (p [0].t [tag]) / p [0].nFrameCount);
else
	sprintf (szValue, ": %05.2f %c", t = 100.0f * float (p [0].t [tag]) / float (p [0].t [ptFrame]), '%');


int x = profilerValueOffset + 1;
char c [2] = {'\0', '\0'};

for (char* ps = szValue; *ps; ps++) {
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

#endif //PROFILING

//------------------------------------------------------------------------------

static void PrintStatistics (void)
{
#if PROFILING
if (gameStates.render.bShowProfiler && !gameStates.menus.nInMenu && fontManager.Current () && SMALL_FONT) {
	static time_t t0 = -1000;
	time_t t1 = clock ();

	if (gameStates.render.bShowProfiler == 1)
		gameData.profiler.nFrameCount = 1;
	else
		gameData.profiler.nFrameCount++;
	if (((gameStates.render.bShowProfiler == 1) && (t1 - t0 >= 10)) || ((gameStates.render.bShowProfiler == 2) && (t1 - t0 >= 1000))) {
		memcpy (&p [1], &p [0], sizeof (tProfilerData));
		memcpy (&p [0], &gameData.profiler, sizeof (tProfilerData));
		t0 = t1;
		}
	fontManager.SetCurrent (SMALL_FONT);
	int h = fontManager.Current ()->Height () + 3;
	fontManager.SetColorRGBi (ORANGE_RGBA, 1, 0, 0);
	fontManager.SetColorRGBi (GOLD_RGBA, 1, 0, 0);
	CFont* fontP = fontManager.Current ();

	int nLine = 3;
	float s = 0;

	PrintTime (ptFrame, nLine++, 1);
	if (p [0].t [ptRenderFrame]) {
		PrintTime (ptGameStates, nLine++);
		fontManager.SetColorRGBi (ORANGE_RGBA, 1, 0, 0);
		PrintTime (ptObjectStates, nLine++);
		PrintTime (ptPhysics, nLine++);
		fontManager.SetColorRGBi (GOLD_RGBA, 1, 0, 0);
		PrintTime (ptRenderFrame, nLine++);
		PrintTime (ptRenderMine, nLine++);
		fontManager.SetColorRGBi (ORANGE_RGBA, 1, 0, 0);
		s += PrintTime (ptLighting, nLine++);
		s += PrintTime (ptRenderPass, nLine++);
		s += PrintTime (ptFaceList, nLine++);
		s += PrintTime (ptRenderFaces, nLine++);
		s += PrintTime (ptRenderObjects, nLine++);
		s += PrintTime (ptTranspPolys, nLine++);
		s += PrintTime (ptEffects, nLine++);
		s += PrintTime (ptParticles, nLine++);
		s += PrintTime (ptCockpit, nLine++);
		s += PrintTime (ptRenderStates, nLine++);
		s += PrintTime (ptShaderStates, nLine++);
		s += PrintTime (ptAux, nLine++);
		s += PrintTime (ptTransform, nLine++);
		s += PrintTime (ptBuildSegList, nLine++);
		s += PrintTime (ptBuildObjList, nLine++);

		fontManager.SetColorRGBi (GOLD_RGBA, 1, 0, 0);
		char szPad [200];
		GrPrintF (NULL, 5, h * nLine, fontManager.Current ()->PadString (szPad, "      total", " .", profilerValueOffset));
		GrPrintF (NULL, profilerValueOffset + 1, h * nLine++, ": %05.2f %c", s, '%');
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
		ogl.DisableClientStates (1, 1, 1, GL_TEXTURE1);
#if 0
		if (StereoDevice () == -GLASSES_SHUTTER_HDMI)
			SetDrawBuffer (GL_BACK, 0);
		else
#endif
			SelectBlurBuffer (0); 
#if DBG
		glClearDepth (1.0);
		glClearColor (0.0f, 0.0f, 0.0f, 1.0f);
		glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif
		SetBlendMode (OGL_BLEND_REPLACE);
		SetDepthMode (GL_ALWAYS);
		for (int i = 0; i < 2; i++) {
			gameData.SetStereoSeparation (i ? STEREO_RIGHT_FRAME : STEREO_LEFT_FRAME);
			SetupCanvasses ();
			gameData.render.screen.Activate ("COGL::FlushEffects (frame)");
			ogl.EnableClientStates (1, 0, 0, GL_TEXTURE0);
			ogl.BindTexture (DrawBuffer (0)->ColorBuffer ());
			OglTexCoordPointer (2, GL_FLOAT, 0, quadTexCoord [i + 1]);
			OglVertexPointer (2, GL_FLOAT, 0, quadVerts [/*0*/i + 1]);
			glColor3f (1, 1, 1);
			if (nEffects & 1) {
				postProcessManager.Setup ();
				postProcessManager.Render ();
				}
			else
				OglDrawArrays (GL_QUADS, 0, 4);
			gameData.render.screen.Deactivate ();
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

