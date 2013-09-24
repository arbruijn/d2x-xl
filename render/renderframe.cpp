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
bool GuidedMissileActive (void);

uint	nClearWindowColor = 0;

extern tTexCoord2f quadTexCoord [3][4];
extern float quadVerts [3][4][2];

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

ogl.ColorMask (1,1,1,1,0);
if (ogl.StereoDevice () < 0) 
	ogl.ChooseDrawBuffer ();
else {
	ogl.SetDrawBuffer (GL_BACK, 0);
	ogl.SetStereoSeparation (0);
	}

#if 1
//SetBlendMode (OGL_BLEND_ALPHA);
if (gameStates.app.bGameRunning) {
	if (automap.Display ()) 
		automap.RenderInfo ();
	else {
		PROF_START
		cockpit->Render (!(gameOpts->render.cockpit.bGuidedInMainView && GuidedMissileActive ()), 0);
		PROF_END(ptCockpit)
		}
	}
paletteManager.RenderEffect ();
FlashMine ();
#endif

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

#if 0
if (ogl.IsSideBySideDevice ()) {
	SetupCanvasses ();
	//ogl.StartFrame (0, 0, xStereoSeparation);
	ogl.ChooseDrawBuffer ();
	SetRenderView (xStereoSeparation, NULL, 1);
	transformation.ComputeFrustum ();

	CFloatMatrix	view;
	view.Assign (*gameData.objs.viewerP->View ());
	CFloatVector3	vPos;
	vPos.Assign (gameData.objs.viewerP->Position ());

	CFloatVector3 quadVerts [4];

	for (int i = 0; i < 4; i++) {
		CFloatVector3 v;
		v.Assign (transformation.Frustum ().m_corners [7 - i]);
		v.v.coord.x *= 7.0f / ZFAR;
		v.v.coord.y *= 7.0f / ZFAR;
		v.v.coord.z *= 10.0f / ZFAR;
		quadVerts [(i + 1) % 4] = view * v;
		quadVerts [(i + 1) % 4] += vPos;
		}

	ogl.EnableClientStates (1, 0, 0, GL_TEXTURE0);
	ogl.BindTexture (ogl.BlurBuffer (0)->ColorBuffer ()); // set source for subsequent rendering step
	OglTexCoordPointer (2, GL_FLOAT, 0, quadTexCoord [1]);
	OglVertexPointer (3, GL_FLOAT, 0, quadVerts);
	ogl.SetupTransform (1);
	ogl.SetBlending (true);
	ogl.SetBlendMode (OGL_BLEND_ALPHA);
	//ogl.SetDepthMode (GL_ALWAYS); 
	ogl.SetDepthTest (false);
	ogl.SetFaceCulling (false);
	ogl.SetAlphaTest (true);
	glAlphaFunc (GL_GEQUAL, (float) 0.005);
	//ogl.SetTexturing (false);
	//glColor4f (0.0f, 0.5f, 1.0f, 0.5f);
	OglDrawArrays (GL_QUADS, 0, 4);
	ogl.SetFaceCulling (true);
	ogl.ResetTransform (1);
	ogl.EndFrame (0);
	}
#endif
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
		gameData.render.scene.SetViewport (&screen);
		if (i < 0)
			Draw2DFrameElements ();
		if (xStereoSeparation > 0) {
			screen.SetViewport ();
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
	fix	nEyeOffsetSave = gameStates.render.xStereoSeparation [0];

gameStates.render.nType = -1;
gameStates.render.SetRenderWindow (nWindow);
gameStates.render.SetStereoSeparation (xStereoSeparation);
if (gameStates.app.bEndLevelSequence) {
	RenderEndLevelFrame (xStereoSeparation, nWindow);
	gameData.app.nFrameCount++;
	gameStates.render.xStereoSeparation [0] = nEyeOffsetSave;
	return;
	}
if ((gameData.demo.nState == ND_STATE_RECORDING) && (xStereoSeparation >= 0)) {
   if (!gameStates.render.nRenderingType)
   	NDRecordStartFrame (gameData.app.nFrameCount, gameData.time.xFrame);
   if (gameStates.render.nRenderingType != 255)
   	NDRecordViewerObject (gameData.objs.viewerP);
	}

StartLightingFrame (gameData.objs.viewerP);		//this is for ugly light-smoothing hack
//ogl.m_states.bEnableScissor = !gameStates.render.cameras.bActive && nWindow;

if (!nWindow) {
	cockpit->Setup ();
	cockpit->Canvas ()->Activate ();
	}

{
PROF_START
G3StartFrame (transformation, 0, (nWindow || gameStates.render.cameras.bActive) ? 0 : 1, nWindow ? nEyeOffsetSave : xStereoSeparation);
ogl.SetStereoSeparation (xStereoSeparation);
if (!nWindow) {
	//gameData.render.scene.Activate ();
	fontManager.SetCurrent (GAME_FONT);
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
	gameStates.render.xStereoSeparation [0] = nEyeOffsetSave;
	return;
	}
#endif

if (0 > (gameStates.render.nStartSeg = nStartSeg)) {
	G3EndFrame (transformation, nWindow);
	gameStates.render.xStereoSeparation [0] = nEyeOffsetSave;
	return;
	}


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
		else 
			RenderNeatShadows (xStereoSeparation, nWindow, nStartSeg);
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
	ogl.SetStereoSeparation (gameStates.render.xStereoSeparation [0] = nEyeOffsetSave);
if (!ShowGameMessage (gameData.messages, -1, -1))
	ShowGameMessage (gameData.messages + 1, -1, -1);
}


//------------------------------------------------------------------------------

void RenderMonoFrame (fix xStereoSeparation = 0)
{
#if MAX_SHADOWMAPS
RenderShadowMaps (xStereoSeparation);
#endif
SetupCanvasses ();
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
		CCanvas::Current ()->SetViewport ();
	RenderFrame (xStereoSeparation, 0);
	if (xStereoSeparation <= 0)
  		WakeupRenderedObjects (gameData.objs.viewerP, 0);
	gameData.objs.viewerP = viewerSave;
	fontManager.SetCurrent (GAME_FONT);    //GAME_FONT);
	fontManager.SetColorRGBi (RED_RGBA, 1, 0, 0);
	fontManager.Current ()->StringSize (msg, w, h, aw);
	GrPrintF (NULL, (CCanvas::Current ()->Width () - w) / 2, 3, msg);
	//DrawGuidedCrosshairs (xStereoSeparation);
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
		CCanvas::Current ()->SetViewport ();
	RenderFrame (xStereoSeparation, 0);
	}
FlushFrame (xStereoSeparation);
//CCanvas::SetCurrent (&screen);
}

//------------------------------------------------------------------------------

#define WINDOW_W_DELTA	 ((gameData.render.frame.Width () / 16) & ~1)	//24	//20
#define WINDOW_H_DELTA	 ((gameData.render.frame.Height () / 16) & ~1)	//12	//10

#define WINDOW_MIN_W		 ((gameData.render.frame.Width () * 10) / 22)	//160
#define WINDOW_MIN_H		 ((gameData.render.frame.Height () * 10) / 22)

void GrowWindow (void)
{
#if 0
StopTime ();
if (gameStates.render.cockpit.nType == CM_FULL_COCKPIT) {
	screen.SetHeight (screen.Height ());
	screen.SetWidth (screen.Width ());
	cockpit->Toggle ();
	HUDInitMessage (TXT_COCKPIT_F3);
	StartTime (0);
	return;
	}

if (gameStates.render.cockpit.nType != CM_STATUS_BAR) {
	StartTime (0);
	return;
	}

if ((screen.Height () >= screen.Height ()) || (screen.Width () >= screen.Width ())) {
	//screen.Width () = screen.Width ();
	//screen[HA] = screen.Height ();
	cockpit->Activate (CM_FULL_SCREEN);
	}
else {
	//int x, y;
	screen.SetWidth (screen.Width () + WINDOW_W_DELTA);
	screen.SetHeight (screen.Height () + WINDOW_H_DELTA);
	if (screen.Height () > screen.Height ())
		screen.SetHeight (screen.Height ());
	if (screen.Width () > screen.Width ())
		screen.SetWidth (screen.Width ());
	screen.SetLeft ((screen.Width () - screen.Width ()) / 2);
	screen.SetTop ((screen.Height () - screen.Height ()) / 2);
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

#if DBG
static bool bFixSBSBug = false;
#endif

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
	transparencyRenderer.Reset ();
	}
if (!ogl.StereoDevice ())
	RenderMonoFrame ();
else {
	if (gameOpts->render.stereo.xSeparation [ogl.IsOculusRift ()] == 0)
		gameOpts->render.stereo.xSeparation [ogl.IsOculusRift ()] = I2X (1);
	fix xStereoSeparation = (automap.Display () && !ogl.IsSideBySideDevice ()) 
									? 2 * gameOpts->render.stereo.xSeparation [0] 
									: gameOpts->render.stereo.xSeparation [ogl.IsOculusRift ()];
	SetupCanvasses (ogl.IsOculusRift () ? 1.25f : 1.0f);
	RenderMonoFrame (-xStereoSeparation);
	RenderMonoFrame (xStereoSeparation);
	SetupCanvasses (1.0f);
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

