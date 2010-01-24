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
#include "cockpit.h"
#include "gamefont.h"
#include "newdemo.h"
#include "text.h"
#include "gr.h"
#include "ogl_render.h"
#include "endlevel.h"
#include "playsave.h"
#include "automap.h"
#include "gamepal.h"
#include "visibility.h"
#include "lightning.h"
#include "transprender.h"
#include "menubackground.h"

#define bSavingMovieFrames 0

//------------------------------------------------------------------------------

static inline bool GuidedMissileActive (void)
{
CObject *gmObjP = gameData.objs.guidedMissile [gameData.multiplayer.nLocalPlayer].objP;
return gmObjP &&
		 (gmObjP->info.nType == OBJ_WEAPON) &&
		 (gmObjP->info.nId == GUIDEDMSL_ID) &&
		 (gmObjP->info.nSignature == gameData.objs.guidedMissile [gameData.multiplayer.nLocalPlayer].nSignature);
}

//------------------------------------------------------------------------------

//draw a crosshair for the guided missile
void DrawGuidedCrosshair (void)
{
CCanvas::Current ()->SetColorRGBi (RGB_PAL (0, 31, 0));
int w = CCanvas::Current ()->Width () >> 5;
if (w < 5)
	w = 5;
int h = I2X (w) / screen.Aspect ();
int x = CCanvas::Current ()->Width () / 2;
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
}

//------------------------------------------------------------------------------

void DrawScope (void)
{
if (LoadScope ()) {
	float sh = float (screen.Height ());
	float ch = float (CCanvas::Current ()->Height ());
	float w = 0.25f * float (CCanvas::Current ()->Width ()) / ch;
	float y = 1.0f - float (CCanvas::Current ()->Top ()) / sh;
	float h = ch / sh;

	glEnable (GL_TEXTURE_2D);
	glEnable (GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable (GL_DEPTH_TEST);
	if (bmpScope->Bind (1))
		return;
	bmpScope->Texture ()->Wrap (GL_REPEAT);
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
	OglBindTexture (0);
	glEnable (GL_DEPTH_TEST);
	glDisable (GL_TEXTURE_2D);
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
glEnable (GL_LINE_SMOOTH);
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
glEnable (GL_LINE_SMOOTH);
if (bHaveTarget)
	glColor4f (1.0f, 0.0f, 0.0f, 0.25f);
else
	glColor4f (0.0f, 1.0f, 0.0f, 0.25f);
glTranslatef (0.5f, 0.5f, 0.5f);
glScalef (float (w << 5) / float (cw), float (h << 5) / float (ch), 0.1f);
OglDrawEllipse (sizeofa (sinCos), GL_LINE_LOOP, 1.0f, 0, 1.0f, 0, sinCos);
glDisable (GL_LINE_SMOOTH);
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
		gameData.objs.guidedMissile [gameData.multiplayer.nLocalPlayer].objP = NULL;
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
if (gameOpts->app.bEpilepticFriendly ||
	 !(/*extraGameInfo [0].bFlickerLights &&*/ gameStates.render.nFlashScale && (gameStates.render.nFlashScale != I2X (1))))
	return;

	int	bDepthTest, bBlend;
	GLint	blendSrc, blendDest;

if ((bBlend = glIsEnabled (GL_BLEND))) {
	glGetIntegerv (GL_BLEND_SRC, &blendSrc);
	glGetIntegerv (GL_BLEND_DST, &blendDest);
	}
else
	glEnable (GL_BLEND);
glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
glColor4f (0, 0, 0, /*1.0f -*/ 3 * X2F (gameStates.render.nFlashScale) / 4);
if ((bDepthTest = glIsEnabled (GL_DEPTH_TEST)))
	glDisable (GL_DEPTH_TEST);
glDisable (GL_TEXTURE_2D);
glBegin (GL_QUADS);
glVertex2f (0,0);
glVertex2f (0,1);
glVertex2f (1,1);
glVertex2f (1,0);
glEnd ();
if (bDepthTest)
	glEnable (GL_DEPTH_TEST);
if (bBlend)
	glBlendFunc (blendSrc, blendDest);
else
	glDisable (GL_BLEND);
}

//------------------------------------------------------------------------------

void FlushFrame (fix xStereoSeparation)
{
if (!xStereoSeparation || (gameOpts->render.n3DGlasses == GLASSES_SHUTTER)) {	//no stereo or shutter glasses
	FlashMine ();
	ogl.SwapBuffers (0, 0);
	}
else if (ogl.ColorCode3D ()) {	// ColorCode 3-D
	FlashMine ();
	if (xStereoSeparation > 0)
		ogl.SwapBuffers (0, 0);
	}
else {
	if (xStereoSeparation < 0) {
		glFlush ();
		ogl.ColorMask (1,1,1,1,0);
		glAccum (GL_LOAD, 1.0); 
		}
	else {
		glFlush ();
		ogl.ColorMask (1,1,1,1,0);
		FlashMine ();
		glAccum (GL_ACCUM, 1.0); 
	   glAccum (GL_RETURN, 1.0);
		ogl.SwapBuffers (0, 0);
		}
	}
}

//------------------------------------------------------------------------------

void RenderMonoFrame (fix xStereoSeparation = 0)
{
	CCanvas		frameWindow;
	int			bExtraInfo = 1;

gameStates.render.vr.buffers.screenPages [0].SetupPane (
	&frameWindow,
	gameStates.render.vr.buffers.subRender [0].Left (),
	gameStates.render.vr.buffers.subRender [0].Top (),
	gameStates.render.vr.buffers.subRender [0].Width (),
	gameStates.render.vr.buffers.subRender [0].Height ());
CCanvas::SetCurrent (&gameStates.render.vr.buffers.subRender [0]);

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
  	gameData.objs.viewerP = gameData.objs.guidedMissile [gameData.multiplayer.nLocalPlayer].objP;
	UpdateRenderedData (0, gameData.objs.viewerP, 0, 0);
	if (!xStereoSeparation && cameraManager.Render ())
		CCanvas::SetCurrent (&gameStates.render.vr.buffers.subRender [0]);
	RenderFrame (xStereoSeparation, 0);
	if (xStereoSeparation <= 0)
  		WakeupRenderedObjects (gameData.objs.viewerP, 0);
	gameData.objs.viewerP = viewerSave;
	fontManager.SetCurrent (GAME_FONT);    //GAME_FONT);
	fontManager.SetColorRGBi (RED_RGBA, 1, 0, 0);
	fontManager.Current ()->StringSize (msg, w, h, aw);
	GrPrintF (NULL, (CCanvas::Current ()->Width () - w) / 2, 3, msg);
	//DrawGuidedCrosshair ();
	HUDRenderMessageFrame ();
	bExtraInfo = 0;
	}
else {
	if (gameStates.render.cockpit.bBigWindowSwitch) {
		gameStates.render.cockpit.bRedraw = 1;
		cockpit->Activate (CM_FULL_COCKPIT);
		gameStates.render.cockpit.bBigWindowSwitch = 0;
		return;
		}
	UpdateRenderedData (0, gameData.objs.viewerP, gameStates.render.bRearView, 0);
	if (!xStereoSeparation && cameraManager.Render ())
		CCanvas::SetCurrent (&gameStates.render.vr.buffers.subRender [0]);
	RenderFrame (xStereoSeparation, 0);
	}
CCanvas::SetCurrent (&gameStates.render.vr.buffers.subRender [0]);

if ((gameOpts->render.n3DGlasses < GLASSES_BLUE_RED) || (gameOpts->render.n3DGlasses > GLASSES_RED_CYAN) || (xStereoSeparation >= 0)) {
	PROF_START
	cockpit->Render (bExtraInfo);
	PROF_END(ptCockpit)
	}

paletteManager.RenderEffect ();
console.Draw ();
FlushFrame (xStereoSeparation);

if (gameStates.app.bSaveScreenshot && !xStereoSeparation)
	SaveScreenShot (NULL, 0);
}

//------------------------------------------------------------------------------

#define WINDOW_W_DELTA	 ((gameData.render.window.wMax / 16) & ~1)	//24	//20
#define WINDOW_H_DELTA	 ((gameData.render.window.hMax / 16) & ~1)	//12	//10

#define WINDOW_MIN_W		 ((gameData.render.window.wMax * 10) / 22)	//160
#define WINDOW_MIN_H		 ((gameData.render.window.hMax * 10) / 22)

void GrowWindow (void)
{
StopTime ();
if (gameStates.render.cockpit.nType == CM_FULL_COCKPIT) {
	gameData.render.window.h = gameData.render.window.hMax;
	gameData.render.window.w = gameData.render.window.wMax;
	cockpit->Toggle ();
	HUDInitMessage (TXT_COCKPIT_F3);
	StartTime (0);
	return;
	}

if (gameStates.render.cockpit.nType != CM_STATUS_BAR && (gameStates.render.vr.nScreenFlags & VRF_ALLOW_COCKPIT)) {
	StartTime (0);
	return;
	}

if (gameData.render.window.h>=gameData.render.window.hMax || gameData.render.window.w>=gameData.render.window.wMax) {
	//gameData.render.window.w = gameData.render.window.wMax;
	//gameData.render.window[HA] = gameData.render.window.hMax;
	cockpit->Activate (CM_FULL_SCREEN);
	}
else {
	//int x, y;
	gameData.render.window.w += WINDOW_W_DELTA;
	gameData.render.window.h += WINDOW_H_DELTA;
	if (gameData.render.window.h > gameData.render.window.hMax)
		gameData.render.window.h = gameData.render.window.hMax;
	if (gameData.render.window.w > gameData.render.window.wMax)
		gameData.render.window.w = gameData.render.window.wMax;
	gameData.render.window.x = (gameData.render.window.wMax - gameData.render.window.w)/2;
	gameData.render.window.y = (gameData.render.window.hMax - gameData.render.window.h)/2;
	GameInitRenderSubBuffers (gameData.render.window.x, gameData.render.window.y, gameData.render.window.w, gameData.render.window.h);
	}
HUDClearMessages ();	//	@mk, 11/11/94
SavePlayerProfile ();
StartTime (0);
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
if (gameData.render.window.x || gameData.render.window.y) {
	CCanvas::Push ();
	CCanvas::SetCurrent (CurrentGameScreen ());
	ogl.m_states.nLastW = CCanvas::Current ()->Width ();
	ogl.m_states.nLastH = CCanvas::Current ()->Height ();
	glDepthMask (0);
	bmBackground.Render (CCanvas::Current (), 0, 0, CCanvas::Current ()->Width (), CCanvas::Current ()->Height (), 0, 0, -bmBackground.Width (), -bmBackground.Height ());
	glDepthMask (1);
	CCanvas::Pop ();
	ogl.m_states.nLastW = CCanvas::Current ()->Width ();
	ogl.m_states.nLastH = CCanvas::Current ()->Height ();
	}
}

//------------------------------------------------------------------------------

void ShrinkWindow (void)
{
StopTime ();
if (gameStates.render.cockpit.nType == CM_FULL_COCKPIT && (gameStates.render.vr.nScreenFlags & VRF_ALLOW_COCKPIT)) {
	gameData.render.window.h = gameData.render.window.hMax;
	gameData.render.window.w = gameData.render.window.wMax;
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

if (gameStates.render.cockpit.nType == CM_FULL_SCREEN && (gameStates.render.vr.nScreenFlags & VRF_ALLOW_COCKPIT)) {
	//gameData.render.window.w = gameData.render.window.wMax;
	//gameData.render.window[HA] = gameData.render.window.hMax;
	cockpit->Activate (CM_STATUS_BAR);
	SavePlayerProfile ();
	StartTime (0);
	return;
	}

if (gameStates.render.cockpit.nType != CM_STATUS_BAR && (gameStates.render.vr.nScreenFlags & VRF_ALLOW_COCKPIT)) {
	StartTime (0);
	return;
	}

#if TRACE
console.printf (CON_DBG, "Cockpit mode=%d\n", gameStates.render.cockpit.nType);
#endif
if (gameData.render.window.w > WINDOW_MIN_W) {
	//int x, y;
   gameData.render.window.w -= WINDOW_W_DELTA;
	gameData.render.window.h -= WINDOW_H_DELTA;
#if TRACE
  console.printf (CON_DBG, "NewW=%d NewH=%d VW=%d maxH=%d\n", gameData.render.window.w, gameData.render.window.h, gameData.render.window.wMax, gameData.render.window.hMax);
#endif
	if (gameData.render.window.w < WINDOW_MIN_W)
		gameData.render.window.w = WINDOW_MIN_W;
	if (gameData.render.window.h < WINDOW_MIN_H)
		gameData.render.window.h = WINDOW_MIN_H;
	gameData.render.window.x = (gameData.render.window.wMax - gameData.render.window.w) / 2;
	gameData.render.window.y = (gameData.render.window.hMax - gameData.render.window.h) / 2;
	GameInitRenderSubBuffers (gameData.render.window.x, gameData.render.window.y, gameData.render.window.w, gameData.render.window.h);
	HUDClearMessages ();
	SavePlayerProfile ();
	}
StartTime (0);
}

//------------------------------------------------------------------------------

void GameRenderFrame (void)
{
PROF_START
SetScreenMode (SCREEN_GAME);
if (!ogl.ColorCode3D () || !(gameData.app.nFrameCount & 1)) {
	cockpit->PlayHomingWarning ();
	FillBackground ();
	transparencyRenderer.Reset ();
	}
if (!gameOpts->render.xStereoSeparation || gameStates.app.bSaveScreenshot)
	RenderMonoFrame ();
else if (gameStates.menus.nInMenu && (ogl.ColorCode3D ())) {
	RenderMonoFrame ((gameData.app.nFrameCount & 1) ? gameOpts->render.xStereoSeparation : -gameOpts->render.xStereoSeparation);
	}
else {
	RenderMonoFrame (-gameOpts->render.xStereoSeparation);
	RenderMonoFrame (gameOpts->render.xStereoSeparation);
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

