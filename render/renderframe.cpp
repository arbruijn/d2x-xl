/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * Stuff for rendering the HUD
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "descent.h"
#include "error.h"
#include "rendermine.h"
#include "screens.h"
#include "console.h"
#include "cockpit.h"
#include "gamefont.h"
#include "newdemo.h"
#include "text.h"
#include "textdata.h"
#include "gr.h"
#include "ogl_render.h"
#include "endlevel.h"
#include "playerprofile.h"
#include "automap.h"
#include "gamepal.h"
#include "visibility.h"
#include "lightning.h"
#include "rendershadows.h"
#include "transprender.h"
#include "radar.h"
#include "menubackground.h"
#include "addon_bitmaps.h"
#include "createmesh.h"
#include "renderthreads.h"

#define bSavingMovieFrames 0

void StartLightingFrame (CObject *viewer);

uint	nClearWindowColor = 0;

//------------------------------------------------------------------------------

static inline bool GuidedMissileActive (void)
{
CObject *gmObjP = gameData.objs.guidedMissile [N_LOCALPLAYER].objP;
return gmObjP &&
		 (gmObjP->info.nType == OBJ_WEAPON) &&
		 (gmObjP->info.nId == GUIDEDMSL_ID) &&
		 (gmObjP->info.nSignature == gameData.objs.guidedMissile [N_LOCALPLAYER].nSignature);
}

//------------------------------------------------------------------------------

//draw a crosshair for the guided missile
void DrawGuidedCrosshair (fix xStereoSeparation)
{
CCanvas::Current ()->SetColorRGBi (RGB_PAL (0, 31, 0));
int w = CCanvas::Current ()->Width () >> 5;
if (w < 5)
	w = 5;
int h = I2X (w) / screen.Aspect ();
int x = CCanvas::Current ()->Width () / 2;
if (xStereoSeparation) {
	ogl.ColorMask (1,1,1,1,1);
	x -= int (float (x / 32) * X2F (xStereoSeparation));
	}
int y = CCanvas::Current ()->Height () / 2;
glLineWidth (float (screen.Width ()) / 640.0f);
#if 1
	w /= 2;
	h /= 2;
	OglDrawLine (x - w, y, x + w, y);
	OglDrawLine (x, y - h, x, y + h);
#else
	OglDrawLine (I2X (x-w/2), I2X (y), I2X (x+w/2), I2X (y));
	OglDrawLine (I2X (x), I2X (y-h/2), I2X (x), I2X (y+h/2));
#endif
glLineWidth (1.0f);
if (xStereoSeparation) {
	ogl.ColorMask (1,1,1,1,0);
	}
}

//------------------------------------------------------------------------------

void DrawScope (void)
{
if (scope.Load ()) {
	float sh = float (screen.Height ());
	float ch = float (CCanvas::Current ()->Height ());
	float w = 0.25f * float (CCanvas::Current ()->Width ()) / ch;
	float y = 1.0f - float (CCanvas::Current ()->Top ()) / sh;
	float h = ch / sh;

	ogl.SetTexturing (true);
	ogl.SetBlending (true);
	ogl.SetBlendMode (OGL_BLEND_ALPHA);
	ogl.SetDepthTest (false);
	if (scope.Bind (1))
		return;
	scope.Texture ()->Wrap (GL_REPEAT);
	glColor3f (1.0f, 1.0f, 1.0f);
	glBegin (GL_QUADS);
	glTexCoord2f (0.5f - w, 0.25f);
	glVertex2f (0, y);
	glTexCoord2f (0.5f + w, 0.25f);
	glVertex2f (1, y);
	glTexCoord2f (0.5f + w, 0.75f);
	glVertex2f (1, y - h);
	glTexCoord2f (0.5f - w, 0.75f);
	glVertex2f (0, y - h);
	glEnd ();
	ogl.BindTexture (0);
	ogl.SetDepthTest (true);
	ogl.SetTexturing (false);
	glPopMatrix ();
	}
}

//------------------------------------------------------------------------------

//draw a crosshair for the zoom
void DrawZoomCrosshair (void)
{
DrawScope ();

	static tSinCosf sinCos [124];
	static int bInitSinCos = 1;

if (bInitSinCos) {
	ComputeSinCosTable (sizeofa (sinCos), sinCos);
	bInitSinCos = 0;
	}

	int bHaveTarget = TargetInLineOfFire ();
	int sh = screen.Height ();
	int ch = CCanvas::Current ()->Height ();
	int cw = CCanvas::Current ()->Width ();
	int h = ch >> 2;
	int w = X2I (h * screen.Aspect ());
	int x = cw / 2;
	int y = ch / 2;
	int left = x - w, right = x + w, top = y - h, bottom = y + h;
	float xStep = float (2 * w + 1) / 12.0f;
	//float yStep = float (2 * h + 1) / 12.0f;
	float xScale = float (w + (w >> 1)) / float (cw);
	float yScale = float (h + (h >> 1)) / float (sh);
	float	x1, y1;
	int	i;

w >>= 4;
h >>= 4;
//w += w >> 1;
//h += h >> 1;

glLineWidth (float (cw) / 640.0f);

if (bHaveTarget)
	CCanvas::Current ()->SetColorRGBi (RGBA_PAL (39, 0, 0, 128));
else
	CCanvas::Current ()->SetColorRGBi (RGBA_PAL (0, 39, 0, 128));
for (i = 0, x1 = float (left); i < 11; i++) {
	x1 += xStep;
	if (i != 5)
		OglDrawLine (int (x1 + 0.5f), y - h, int (x1 + 0.5f), y + h);
	}

for (i = 0, y1 = float (top); i < 11; i++) {
	y1 += xStep;
	if (i != 5)
		OglDrawLine (x - w, int (y1 + 0.5f), x + w, int (y1 + 0.5f));
	}

if (bHaveTarget)
	CCanvas::Current ()->SetColorRGBi (RGBA_PAL (63, 0, 0, 160));
else
	CCanvas::Current ()->SetColorRGBi (RGBA_PAL (0, 63, 0, 160));

glLineWidth (float (floor (2 * float (cw) / 640.0f)));

glPushMatrix ();
ogl.SetLineSmooth (true);
glTranslatef (0.5f, 1.0f - float (CCanvas::Current ()->Top () + y) / float (screen.Height ()), 0.0f);
glScalef (xScale, yScale, 1.0f);
#if 0
float fh = 2.0f * float (h) / float (sh);
float fw = 2.0f * float (w) / float (cw);
#endif
glBegin (GL_LINES);
glVertex2f (0.0f, -1.0f);
glVertex2f (0.0f, -1.125f);
glVertex2f (sinCos [15].fCos, sinCos [15].fSin);
glVertex2f (sinCos [15].fCos * 1.0625f, sinCos [15].fSin * 1.0625f);
glVertex2f (0.0f, 1.0f);
glVertex2f (0.0f, 1.125f);
glVertex2f (sinCos [46].fCos, sinCos [46].fSin);
glVertex2f (sinCos [46].fCos * 1.0625f, sinCos [46].fSin * 1.0625f);
glVertex2f (-1.125f, 0.0f);
glVertex2f (-1.0f, 0.0f);
glVertex2f (sinCos [77].fCos, sinCos [77].fSin);
glVertex2f (sinCos [77].fCos * 1.0625f, sinCos [77].fSin * 1.0625f);
glVertex2f (1.125f, 0.0f);
glVertex2f (1.0f, 0.0f);
glVertex2f (sinCos [108].fCos, sinCos [108].fSin);
glVertex2f (sinCos [108].fCos * 1.0625f, sinCos [108].fSin * 1.0625f);
glEnd ();
glPopMatrix ();

w += w >> 1;
h += h >> 1;
w <<= 1;
h <<= 1;

glLineWidth (float (cw) / 640.0f);

OglDrawLine (left, y, right, y);
OglDrawLine (x, top, x, bottom);
OglDrawLine (left, y - h, left, y + h);
OglDrawLine (right, y - h, right, y + h);
OglDrawLine (x - w, top, x + w, top);
OglDrawLine (x - w, bottom, x + w, bottom);
glLineWidth (1.0f);

w >>= 1;
h >>= 1;
w -= w >> 1;
h -= h >> 1;
glLineWidth (float (floor (2 * float (cw) / 640.0f)));
#if 1
//float xScale = float (w << 5) / float (cw);
//float yScale = float (h << 5) / float (screen.Height ());
OglDrawEllipse (sizeofa (sinCos), GL_LINE_LOOP, xScale, 0.5f, yScale, 1.0f - float (CCanvas::Current ()->Top () + y) / float (screen.Height ()), sinCos);
#else
glPushMatrix ();
ogl.SetLineSmooth (true);
if (bHaveTarget)
	glColor4f (1.0f, 0.0f, 0.0f, 0.25f);
else
	glColor4f (0.0f, 1.0f, 0.0f, 0.25f);
glTranslatef (0.5f, 0.5f, 0.5f);
glScalef (float (w << 5) / float (cw), float (h << 5) / float (ch), 0.1f);
OglDrawEllipse (sizeofa (sinCos), GL_LINE_LOOP, 1.0f, 0, 1.0f, 0, sinCos);
ogl.SetLineSmooth (false);
glPopMatrix ();
#endif

char	szZoom [20];
int	r, aw;
if (extraGameInfo [IsMultiGame].nZoomMode == 2)
	r = int (100.0f * gameStates.zoom.nFactor / float (gameStates.zoom.nMinFactor));
else {
	float s = float (pow (float (gameStates.zoom.nMaxFactor) / float (gameStates.zoom.nMinFactor), 0.25f));
	fix f = gameStates.zoom.nMinFactor;
	for (r = 1; f < fix (gameStates.zoom.nFactor); r++)
		f = fix (float (f) * s + 0.5f);
	r *= 100;
	}
sprintf (szZoom, "X %d.%02d", r / 100, r % 100);
fontManager.Current ()->StringSize (szZoom, w, h, aw);
if (bHaveTarget)
	fontManager.SetColorRGBi (RED_RGBA, 1, 0, 0);
else
	fontManager.SetColorRGBi (GREEN_RGBA, 1, 0, 0);
GrPrintF (NULL, x - w  / 2, bottom + h, szZoom);
}

//------------------------------------------------------------------------------

int RenderMissileView (void)
{
	CObject	*objP = NULL;

if (GuidedMslView (&objP)) {
	if (gameOpts->render.cockpit.bGuidedInMainView) {
		gameStates.render.nRenderingType = 6 + (1 << 4);
		cockpit->RenderWindow (1, gameData.objs.viewerP, 0, WBUMSL, "SHIP");
		}
	else {
		gameStates.render.nRenderingType = 1+ (1 << 4);
		cockpit->RenderWindow (1, objP, 0, WBU_GUIDED, "GUIDED");
	   }
	return 1;
	}
else {
	if (objP) {		//used to be active
		if (!gameOpts->render.cockpit.bGuidedInMainView)
			cockpit->RenderWindow (1, NULL, 0, WBU_STATIC, NULL);
		gameData.objs.guidedMissile [N_LOCALPLAYER].objP = NULL;
		}
	if (gameData.objs.missileViewerP && !gameStates.render.bChaseCam) {		//do missile view
		static int mslViewerSig = -1;
		if (mslViewerSig == -1)
			mslViewerSig = gameData.objs.missileViewerP->info.nSignature;
		if (gameOpts->render.cockpit.bMissileView &&
			 (gameData.objs.missileViewerP->info.nType != OBJ_NONE) &&
			 (gameData.objs.missileViewerP->info.nSignature == mslViewerSig)) {
			//HUDMessage (0, "missile view");
  			gameStates.render.nRenderingType = 2 + (1 << 4);
			cockpit->RenderWindow (1, gameData.objs.missileViewerP, 0, WBUMSL, "MISSILE");
			return 1;
			}
		else {
			gameData.objs.missileViewerP = NULL;
			mslViewerSig = -1;
			gameStates.render.nRenderingType = 255;
			cockpit->RenderWindow (1, NULL, 0, WBU_STATIC, NULL);
			}
		}
	}
return 0;
}

//------------------------------------------------------------------------------

void FlashMine (void)
{
if (gameOpts->app.bEpilepticFriendly || !gameStates.render.nFlashScale)
	return;

#if 0
ogl.SetBlendMode (OGL_BLEND_ALPHA);
glColor4f (0, 0, 0, /*1.0f -*/ X2F (gameStates.render.nFlashScale) * 0.75f);
#else
ogl.SetBlendMode (OGL_BLEND_MULTIPLY);
float color = 1.0f - X2F (gameStates.render.nFlashScale) * 0.8f;
glColor4f (color, color, color, 1.0f);
#endif
ogl.ResetClientStates (1);
ogl.SetTexturing (false);
ogl.SetDepthTest (false);
ogl.RenderScreenQuad ();
ogl.SetDepthTest (true);
ogl.SetBlendMode (OGL_BLEND_ALPHA);
}

//------------------------------------------------------------------------------

void Draw2DFrameElements (void)
{
	fix xStereoSeparation = ogl.StereoSeparation ();

if (ogl.StereoDevice () >= 0) {
	ogl.SetDrawBuffer (GL_BACK, 0);
	ogl.SetStereoSeparation (0);
	}
ogl.ColorMask (1,1,1,1,0);
//SetBlendMode (OGL_BLEND_ALPHA);
if (gameStates.app.bGameRunning && !automap.Display ()) {
	PROF_START
	cockpit->Render (!(gameOpts->render.cockpit.bGuidedInMainView && GuidedMissileActive ()), 0);
	PROF_END(ptCockpit)
	}
paletteManager.RenderEffect ();
FlashMine ();
console.Draw ();
if (gameStates.app.bShowVersionInfo || gameStates.app.bSaveScreenShot || (gameData.demo.nState == ND_STATE_PLAYBACK))
	PrintVersionInfo ();
if (CMenu::Active ()) {
#if DBG
	if (!gameStates.menus.nInMenu)
		PrintLog (0, "invalid menu handle in renderer\n");
	else
#endif
		CMenu::Active ()->Render ();
	}
ogl.SetStereoSeparation (xStereoSeparation);
}

//------------------------------------------------------------------------------

void FlushFrame (fix xStereoSeparation)
{
	int i = ogl.StereoDevice ();

if (!(i && xStereoSeparation)) {	//no stereo or shutter glasses or Oculus Rift
	//if (gameStates.render.bRenderIndirect <= 0)
	//	Draw2DFrameElements ();
	ogl.SwapBuffers (0, 0);
	}
else {
	if ((i < 0) || (xStereoSeparation > 0)) {
		if (i < 0)
			Draw2DFrameElements ();
		if (xStereoSeparation > 0) {
			ogl.SetViewport (0, 0, gameData.render.screen.Width (), gameData.render.screen.Height ());
			ogl.SwapBuffers (0, 0);
			}
		}
	else {
		ogl.ColorMask (1,1,1,1,0);
		}
	}
}

//------------------------------------------------------------------------------

#if MAX_SHADOWMAPS

static int RenderShadowMap (CDynLight* lightP, int nLight, fix xStereoSeparation)
{
if (!lightP)
	return 0;

	CCamera* cameraP = cameraManager.ShadowMap (nLight);

if (!(cameraP || (cameraP = cameraManager.AddShadowMap (nLight, lightP)))) 
	return 0;

if (cameraP->HaveBuffer (0))
	cameraP->Setup (cameraP->Id (), lightP->info.nSegment, lightP->info.nSide, -1, -1, (lightP->info.nObject < 0) ? NULL : OBJECTS + lightP->info.nObject, 0);
else if (!cameraP->Create (cameraManager.Count () - 1, lightP->info.nSegment, lightP->info.nSide, -1, -1, (lightP->info.nObject < 0) ? NULL : OBJECTS + lightP->info.nObject, 1, 0)) {
	cameraManager.DestroyShadowMap (nLight);
	return 0;
	}
CCamera* current = cameraManager [cameraManager.Current ()];
cameraP = cameraManager.ShadowMap (nLight);
cameraManager.SetCurrent (cameraP);
cameraP->Render ();
cameraManager.SetCurrent (current);
return 1;
}

//------------------------------------------------------------------------------

static void RenderShadowMaps (fix xStereoSeparation)
{
if (EGI_FLAG (bShadows, 0, 1, 0)) {
	lightManager.ResetActive (1, 0);
	short nSegment = OBJSEG (gameData.objs.viewerP);
	lightManager.ResetNearestStatic (nSegment, 1);
	lightManager.SetNearestStatic (nSegment, 1, 1);
	CDynLightIndex* sliP = &lightManager.Index (0,1);
	CActiveDynLight* activeLightsP = lightManager.Active (1) + sliP->nFirst;
	int nLights = 0, h = (sliP->nActive < abs (MAX_SHADOWMAPS)) ? sliP->nActive : abs (MAX_SHADOWMAPS);
	for (gameStates.render.nShadowMap = 1; gameStates.render.nShadowMap <= h; gameStates.render.nShadowMap++) 
		nLights += RenderShadowMap (lightManager.GetActive (activeLightsP, 1), nLights, xStereoSeparation);
	lightManager.SetLightCount (nLights, 2);
	gameStates.render.nShadowMap = 0;
	}
}

#endif

//------------------------------------------------------------------------------

extern CBitmap bmBackground;

void RenderFrame (fix xStereoSeparation, int nWindow)
{
	short nStartSeg;
	fix	nEyeOffsetSave = gameStates.render.xStereoSeparation;

gameStates.render.nType = -1;
gameStates.render.nWindow = nWindow;
gameStates.render.xStereoSeparation = xStereoSeparation;
if (gameStates.app.bEndLevelSequence) {
	RenderEndLevelFrame (xStereoSeparation, nWindow);
	gameData.app.nFrameCount++;
	gameStates.render.xStereoSeparation = nEyeOffsetSave;
	return;
	}
if ((gameData.demo.nState == ND_STATE_RECORDING) && (xStereoSeparation >= 0)) {
   if (!gameStates.render.nRenderingType)
   	NDRecordStartFrame (gameData.app.nFrameCount, gameData.time.xFrame);
   if (gameStates.render.nRenderingType != 255)
   	NDRecordViewerObject (gameData.objs.viewerP);
	}

StartLightingFrame (gameData.objs.viewerP);		//this is for ugly light-smoothing hack
ogl.m_states.bEnableScissor = !gameStates.render.cameras.bActive && nWindow;

if (nWindow)
	CCanvas::SetCurrent (&gameData.render.scene);
else
	cockpit->Setup ();

{
PROF_START
G3StartFrame (transformation, 0, ((ogl.StereoDevice () == -2) && (xStereoSeparation > 0)) ? -1 : (nWindow || gameStates.render.cameras.bActive) ? 0 : 1, xStereoSeparation);
if (!nWindow) {
	CCanvas::SetCurrent (&gameData.render.scene);
	ogl.SetViewport (gameData.render.frame.Left () + gameData.render.scene.Left (), 
						  gameData.render.frame.Top () + gameData.render.scene.Top (), 
						  gameData.render.scene.Width (), 
						  gameData.render.scene.Height ());
	fontManager.SetCurrent (GAME_FONT);
	//transformation.ComputeAspect (gameData.render.frame.Width (), gameData.render.frame.Height ());
	gameData.render.dAspect = (double) CCanvas::Current ()->Width () / (double) CCanvas::Current ()->Height ();
	}
SetRenderView (xStereoSeparation, &nStartSeg, 1);
transformation.ComputeFrustum ();
#if MAX_SHADOWMAPS
if (!(nWindow || gameStates.render.cameras.bActive)) {
	ogl.SetupTransform (1);
	transformation.SystemMatrix (-1).Get (GL_MODELVIEW_MATRIX, true); // inverse
	transformation.SystemMatrix (-2).Get (GL_PROJECTION_MATRIX, true); 
	transformation.SystemMatrix (-3).Get (GL_PROJECTION_MATRIX, false);
	ogl.ResetTransform (1);
	glPushMatrix ();
	transformation.SystemMatrix (-1).Set ();
	transformation.SystemMatrix (-2).Mul ();
	transformation.SystemMatrix (-3).Get (GL_MODELVIEW_MATRIX, false); // inverse (modelview * projection)
	glPopMatrix ();
	}
#endif
PROF_END(ptAux)
}

#if 0 //DBG
if (gameStates.render.nShadowMap) {
	G3EndFrame (transformation, nWindow);
	gameStates.render.xStereoSeparation = nEyeOffsetSave;
	return;
	}
#endif

if (0 > (gameStates.render.nStartSeg = nStartSeg)) {
	G3EndFrame (transformation, nWindow);
	gameStates.render.xStereoSeparation = nEyeOffsetSave;
	return;
	}

#if DBG
if (bShowOnlyCurSide)
	CCanvas::Current ()->Clear (nClearWindowColor);
#endif

#if MAX_SHADOWMAPS
RenderMine (nStartSeg, xStereoSeparation, nWindow);
#else
if (!ogl.m_features.bStencilBuffer.Available ())
	extraGameInfo [0].bShadows =
	extraGameInfo [1].bShadows = 0;
if (SHOW_SHADOWS && !(nWindow || gameStates.render.cameras.bActive || automap.Display ())) {
	if (!gameStates.render.nShadowMap) {
		gameStates.render.nShadowPass = 1;
#if SOFT_SHADOWS
		if (gameOpts->render.shadows.bSoft = 1)
			gameStates.render.nShadowBlurPass = 1;
#endif
		ogl.StartFrame (0, 0, xStereoSeparation);
#if SOFT_SHADOWS
		ogl.SetViewport (CCanvas::Current ()->props.x, CCanvas::Current ()->props.y, 128, 128);
#endif
		RenderMine (nStartSeg, xStereoSeparation, nWindow);
		PROF_START
		if (FAST_SHADOWS)
			RenderFastShadows (xStereoSeparation, nWindow, nStartSeg);
		else {
			RenderNeatShadows (xStereoSeparation, nWindow, nStartSeg);
			}
#if SOFT_SHADOWS
		if (gameOpts->render.shadows.bSoft) {
			PROF_START
			CreateShadowTexture ();
			PROF_END(ptEffects)
			gameStates.render.nShadowBlurPass = 2;
			gameStates.render.nShadowPass = 0;
			ogl.StartFrame (0, 1, xStereoSeparation);
			SetRenderView (xStereoSeparation, &nStartSeg, 1);
			RenderMine (nStartSeg, xStereoSeparation, nWindow);
			RenderShadowTexture ();
			}
#endif
		PROF_END(ptEffects)
		nWindow = 0;
		}
	}
else {
	if (gameStates.render.nRenderPass < 0)
		RenderMine (nStartSeg, xStereoSeparation, nWindow);
	else {
		for (gameStates.render.nRenderPass = 0; gameStates.render.nRenderPass < 2; gameStates.render.nRenderPass++) {
			ogl.StartFrame (0, 1, xStereoSeparation);
			RenderMine (nStartSeg, xStereoSeparation, nWindow);
			}
		}
	}
ogl.StencilOff ();
#endif
RenderSkyBox (nWindow);
RenderEffects (nWindow);

if (!(nWindow || gameStates.render.cameras.bActive || gameStates.app.bEndLevelSequence || GuidedInMainView ())) {
	radar.Render ();
	}
#if 0
if (transformation.m_info.bUsePlayerHeadAngles)
	Draw3DReticle (xStereoSeparation);
#endif
gameStates.render.nShadowPass = 0;
G3EndFrame (transformation, nWindow);
if (nWindow)
	ogl.SetStereoSeparation (gameStates.render.xStereoSeparation = nEyeOffsetSave);
if (!ShowGameMessage (gameData.messages, -1, -1))
	ShowGameMessage (gameData.messages + 1, -1, -1);
}

//------------------------------------------------------------------------------

void RenderMonoFrame (fix xStereoSeparation = 0)
{
#if MAX_SHADOWMAPS
RenderShadowMaps (xStereoSeparation);
#endif
if (ogl.StereoDevice () == -2)
	screen.Canvas ()->SetupPane (&gameData.render.frame, (xStereoSeparation < 0) ? 0 : screen.Width () / 2, 0, screen.Width () / 2, screen.Height ());
else
	screen.Canvas ()->SetupPane (&gameData.render.frame, gameData.render.frame.Left (), gameData.render.frame.Top (), gameData.render.frame.Width (), gameData.render.frame.Height ());
screen.Canvas ()->SetupPane (&gameData.render.viewport, 0, 0, gameData.render.frame.Width (), gameData.render.frame.Height ());
screen.Canvas ()->SetupPane (&gameData.render.scene, 0, 0, gameData.render.frame.Width (), gameData.render.frame.Height ());
CCanvas::SetCurrent (&gameData.render.frame);

if (xStereoSeparation <= 0) {
	PROF_START
	SEM_ENTER (SEM_LIGHTNING)
	bool bRetry;
	do {
		bRetry = false;
		try {
			lightningManager.SetLights ();
			}
		catch(...) {
			bRetry = true;
			}
		} while (bRetry);
	SEM_LEAVE (SEM_LIGHTNING)
	PROF_END(ptLighting)
	}

if (gameOpts->render.cockpit.bGuidedInMainView && GuidedMissileActive ()) {
	int w, h, aw;
	const char *msg = "Guided Missile View";
	CObject *viewerSave = gameData.objs.viewerP;

   if (gameStates.render.cockpit.nType == CM_FULL_COCKPIT) {
		gameStates.render.cockpit.bBigWindowSwitch = 1;
		gameStates.render.cockpit.bRedraw = 1;
		cockpit->Activate (CM_STATUS_BAR);
		return;
		}
  	gameData.objs.viewerP = gameData.objs.guidedMissile [N_LOCALPLAYER].objP;
	UpdateRenderedData (0, gameData.objs.viewerP, 0, 0);
	if ((xStereoSeparation <= 0) && cameraManager.Render ())
		ogl.SetViewport (CCanvas::Current ()->Left (), CCanvas::Current ()->Top (), CCanvas::Current ()->Width (), CCanvas::Current ()->Height ());
	RenderFrame (xStereoSeparation, 0);
	if (xStereoSeparation <= 0)
  		WakeupRenderedObjects (gameData.objs.viewerP, 0);
	gameData.objs.viewerP = viewerSave;
	fontManager.SetCurrent (GAME_FONT);    //GAME_FONT);
	fontManager.SetColorRGBi (RED_RGBA, 1, 0, 0);
	fontManager.Current ()->StringSize (msg, w, h, aw);
	GrPrintF (NULL, (CCanvas::Current ()->Width () - w) / 2, 3, msg);
	//DrawGuidedCrosshair (xStereoSeparation);
	HUDRenderMessageFrame ();
	}
else {
	if (gameStates.render.cockpit.bBigWindowSwitch) {
		gameStates.render.cockpit.bRedraw = 1;
		cockpit->Activate (CM_FULL_COCKPIT);
		gameStates.render.cockpit.bBigWindowSwitch = 0;
		return;
		}
	UpdateRenderedData (0, gameData.objs.viewerP, gameStates.render.bRearView, 0);
	if ((xStereoSeparation <= 0) && cameraManager.Render ())
		ogl.SetViewport (CCanvas::Current ()->Left (), CCanvas::Current ()->Top (), CCanvas::Current ()->Width (), CCanvas::Current ()->Height ());
	RenderFrame (xStereoSeparation, 0);
	}
FlushFrame (xStereoSeparation);
CCanvas::SetCurrent (&gameData.render.frame);
}

//------------------------------------------------------------------------------

#define WINDOW_W_DELTA	 ((screen.Width () / 16) & ~1)	//24	//20
#define WINDOW_H_DELTA	 ((screen.Height () / 16) & ~1)	//12	//10

#define WINDOW_MIN_W		 ((screen.Width () * 10) / 22)	//160
#define WINDOW_MIN_H		 ((screen.Height () * 10) / 22)

void GrowWindow (void)
{
#if 0
StopTime ();
if (gameStates.render.cockpit.nType == CM_FULL_COCKPIT) {
	gameData.render.frame.SetHeight (screen.Height ());
	gameData.render.frame.SetWidth (screen.Width ());
	cockpit->Toggle ();
	HUDInitMessage (TXT_COCKPIT_F3);
	StartTime (0);
	return;
	}

if (gameStates.render.cockpit.nType != CM_STATUS_BAR) {
	StartTime (0);
	return;
	}

if ((gameData.render.frame.Height () >= screen.Height ()) || (gameData.render.frame.Width () >= screen.Width ())) {
	//gameData.render.frame.Width () = screen.Width ();
	//gameData.render.frame[HA] = screen.Height ();
	cockpit->Activate (CM_FULL_SCREEN);
	}
else {
	//int x, y;
	gameData.render.frame.SetWidth (gameData.render.frame.Width () + WINDOW_W_DELTA);
	gameData.render.frame.SetHeight (gameData.render.frame.Height () + WINDOW_H_DELTA);
	if (gameData.render.frame.Height () > screen.Height ())
		gameData.render.frame.SetHeight (screen.Height ());
	if (gameData.render.frame.Width () > screen.Width ())
		gameData.render.frame.SetWidth (screen.Width ());
	gameData.render.frame.SetLeft ((screen.Width () - gameData.render.frame.Width ()) / 2);
	gameData.render.frame.SetTop ((screen.Height () - gameData.render.frame.Height ()) / 2);
	//GameInitRenderSubBuffers (gameData.render.frame.Left (), gameData.render.frame.Top (), gameData.render.frame.Width (), gameData.render.frame.Height ());
	}
HUDClearMessages ();	//	@mk, 11/11/94
SavePlayerProfile ();
StartTime (0);
#endif
}

//------------------------------------------------------------------------------

extern CBitmap bmBackground;

void CopyBackgroundRect (int left, int top, int right, int bot)
{
if (right < left || bot < top)
	return;

	int x, y;
	int tileLeft, tileRight, tileTop, tileBot;
	int xOffs, yOffs;
	int xDest, yDest;

tileLeft = left / bmBackground.Width ();
tileRight = right / bmBackground.Width ();
tileTop = top / bmBackground.Height ();
tileBot = bot / bmBackground.Height ();

yOffs = top % bmBackground.Height ();
yDest = top;

for (y = tileTop;y <= tileBot; y++) {
	xOffs = left % bmBackground.Width ();
	xDest = left;
	int h = min (bot - yDest + 1, bmBackground.Height () - yOffs);
	for (x = tileLeft; x <= tileRight; x++) {
		int w = min (right - xDest + 1, bmBackground.Width () - xOffs);
		bmBackground.Blit (CCanvas::Current (), xDest, yDest, w, h, xOffs, yOffs, 1);
		xOffs = 0;
		xDest += w;
		}
	yOffs = 0;
	yDest += h;
	}
}

//------------------------------------------------------------------------------
//fills int the background surrounding the 3d window
void FillBackground (void)
{
#if 0
if (gameData.render.frame.Left () || gameData.render.frame.Top ()) {
	CCanvas::Push ();
	CCanvas::SetCurrent (CurrentGameScreen ());
	CViewport viewport = ogl.m_states.viewport [0];
	ogl.m_states.viewport [0] = CViewport (0, 0, CCanvas::Current ()->Width (), CCanvas::Current ()->Height ());
	ogl.SetDepthWrite (false);
	bmBackground.Render (CCanvas::Current (), 0, 0, CCanvas::Current ()->Width (), CCanvas::Current ()->Height (), 0, 0, -bmBackground.Width (), -bmBackground.Height ());
	ogl.SetDepthWrite (true);
	CCanvas::Pop ();
	ogl.m_states.viewport [0] = viewport;
	}
#endif
}

//------------------------------------------------------------------------------

void ShrinkWindow (void)
{
#if 0
StopTime ();
if (gameStates.render.cockpit.nType == CM_FULL_COCKPIT) {
	gameData.render.frame.SetHeight (screen.Height ());
	gameData.render.frame.SetWidth (screen.Width ());
	//!!ToggleCockpit ();
	gameStates.render.cockpit.nNextType = CM_FULL_COCKPIT;
	cockpit->Activate (CM_STATUS_BAR);
//		ShrinkWindow ();
//		ShrinkWindow ();
	HUDInitMessage (TXT_COCKPIT_F3);
	SavePlayerProfile ();
	StartTime (0);
	return;
	}

if (gameStates.render.cockpit.nType == CM_FULL_SCREEN) {
	//gameData.render.frame.Width () = screen.Width ();
	//gameData.render.frame[HA] = screen.Height ();
	cockpit->Activate (CM_STATUS_BAR);
	SavePlayerProfile ();
	StartTime (0);
	return;
	}

if (gameStates.render.cockpit.nType != CM_STATUS_BAR) {
	StartTime (0);
	return;
	}

#if TRACE
console.printf (CON_DBG, "Cockpit mode=%d\n", gameStates.render.cockpit.nType);
#endif
if (gameData.render.frame.Width () > WINDOW_MIN_W) {
	//int x, y;
   gameData.render.frame.SetWidth (gameData.render.frame.Width () - WINDOW_W_DELTA);
	gameData.render.frame.SetHeight (gameData.render.frame.Height () - WINDOW_H_DELTA);
#if TRACE
  console.printf (CON_DBG, "NewW=%d NewH=%d VW=%d maxH=%d\n", gameData.render.frame.Width (), gameData.render.frame.Height (), screen.Width (), screen.Height ());
#endif
	if (gameData.render.frame.Width () < WINDOW_MIN_W)
		gameData.render.frame.SetWidth (WINDOW_MIN_W);
	if (gameData.render.frame.Height () < WINDOW_MIN_H)
		gameData.render.frame.SetHeight (WINDOW_MIN_H);
	gameData.render.frame.SetLeft ((screen.Width () - gameData.render.frame.Width ()) / 2);
	gameData.render.frame.SetTop ((screen.Height () - gameData.render.frame.Height ()) / 2);
	//GameInitRenderSubBuffers (gameData.render.frame.Left (), gameData.render.frame.Top (), gameData.render.frame.Width (), gameData.render.frame.Height ());
	HUDClearMessages ();
	SavePlayerProfile ();
	}
StartTime (0);
#endif
}

//------------------------------------------------------------------------------

void UpdateSlidingFaces (void)
{
	CSegFace*		faceP;
	short				nOffset;
	tTexCoord2f*	texCoordP, *ovlTexCoordP;
	tUVL*				uvlP;

for (faceP = FACES.slidingFaces; faceP; faceP = faceP->nextSlidingFace) {
#if DBG
	if ((faceP->m_info.nSegment == nDbgSeg) && ((nDbgSide < 0) || (faceP->m_info.nSide == nDbgSide)))
		faceP = faceP;
#endif
	texCoordP = FACES.texCoord + faceP->m_info.nIndex;
	ovlTexCoordP = FACES.ovlTexCoord + faceP->m_info.nIndex;
	uvlP = SEGMENTS [faceP->m_info.nSegment].m_sides [faceP->m_info.nSide].m_uvls;
	nOffset = faceP->m_info.nType == SIDE_IS_TRI_13;
	if (gameStates.render.bTriangleMesh) {
		static short nTriVerts [2][6] = {{0,1,2,0,2,3},{0,1,3,1,2,3}};
		int j = faceP->m_info.nTriangles * 3;
		for (int i = 0; i < j; i++) {
			short k = nTriVerts [nOffset][i];
			texCoordP [i].v.u = X2F (uvlP [k].u);
			texCoordP [i].v.v = X2F (uvlP [k].v);
			RotateTexCoord2f (ovlTexCoordP [i], texCoordP [i], faceP->m_info.nOvlOrient);
			}
		}
	else {
		int j = 2 * faceP->m_info.nTriangles;
		for (int i = 0; i < j; i++) {
			texCoordP [i].v.u = X2F (uvlP [(i + nOffset) % 4].u);
			texCoordP [i].v.v = X2F (uvlP [(i + nOffset) % 4].v);
			RotateTexCoord2f (ovlTexCoordP [i], texCoordP [i], faceP->m_info.nOvlOrient);
			}
		}
	}
}

//------------------------------------------------------------------------------

void GameRenderFrame (void)
{
if (gameData.segs.nFaceKeys < 0)
	meshBuilder.ComputeFaceKeys ();
PROF_START
UpdateSlidingFaces ();
PROF_END(ptAux);
SetScreenMode (SCREEN_GAME);
if (!ogl.StereoDevice () || !(gameData.app.nFrameCount & 1)) {
	cockpit->PlayHomingWarning ();
	FillBackground ();
	transparencyRenderer.Reset ();
	}
if (!gameOpts->render.stereo.nGlasses)
	RenderMonoFrame ();
else {
	if (gameOpts->render.stereo.xSeparation == 0)
		gameOpts->render.stereo.xSeparation = 3 * I2X (1) / 4;
	fix xStereoSeparation = automap.Display () ? 2 * gameOpts->render.stereo.xSeparation : gameOpts->render.stereo.xSeparation;
	if (gameStates.menus.nInMenu && ogl.StereoDevice ()) {
		RenderMonoFrame ((gameData.app.nFrameCount & 1) ? xStereoSeparation : -xStereoSeparation);
		}
	else {
		RenderMonoFrame (-xStereoSeparation);
		//RenderMonoFrame (xStereoSeparation);
		FlushFrame (xStereoSeparation);
		CCanvas::SetCurrent (&gameData.render.frame);
		}
	}
//StopTime ();
//if (!gameStates.menus.nInMenu)
//	paletteManager.EnableEffect ();
//StartTime (0);
gameData.app.nFrameCount++;
PROF_END (ptRenderFrame)
}

//------------------------------------------------------------------------------
//eof

