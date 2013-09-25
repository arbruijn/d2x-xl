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

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "descent.h"
#include "screens.h"
#include "error.h"
#include "newdemo.h"
#include "gamefont.h"
#include "text.h"
#include "network.h"
#include "network_lib.h"
#include "rendermine.h"
#include "transprender.h"
#include "gamepal.h"
#include "gr.h"
#include "ogl_lib.h"
#include "ogl_render.h"
#include "ogl_bitmap.h"
#include "ogl_hudstuff.h"
#include "playerprofile.h"
#include "hud_defs.h"
#include "cockpit.h"
#include "statusbar.h"
#include "slowmotion.h"
#include "automap.h"
#include "hudmsgs.h"
#include "hudicons.h"
#include "radar.h"
#include "visibility.h"
#include "autodl.h"
#include "key.h"
#include "addon_bitmaps.h"
#include "marker.h"

#define SHOW_PLAYER_IP		0

#if 0
CCanvas *Canv_LeftEnergyGauge;
CCanvas *Canv_AfterburnerGauge;
CCanvas *Canv_RightEnergyGauge;
CCanvas *numericalGaugeCanv;

#define COCKPIT_PRIMARY_BOX		 (!gameStates.video.nDisplayMode ? 0 : 4)
#define COCKPIT_SECONDARY_BOX		 (!gameStates.video.nDisplayMode ? 1 : 5)
#define SB_PRIMARY_BOX				 (!gameStates.video.nDisplayMode ? 2 : 6)
#define SB_SECONDARY_BOX			 (!gameStates.video.nDisplayMode ? 3 : 7)
#endif

CHUD			hudCockpit;
CWideHUD		letterboxCockpit;
CCockpit		fullCockpit;
CStatusBar	statusBarCockpit;
CRearView	rearViewCockpit;

CStack<int>	CGenericCockpit::m_save;

bool	CCockpitInfo::bWindowDrawn [2];

CGenericCockpit* cockpit = &fullCockpit;

//	-----------------------------------------------------------------------------

void DrawGuidedCrosshairs (fix xStereoSeparation);
void DrawZoomCrosshairs (void);
void DrawWindowLabel (void);

extern fix staticTime [2];

//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------

void CCockpitHistory::Init (void)
{
score = (IsMultiGame && !IsCoopGame) ? -99 : -1;
energy = -1;
shield = -1;
flags = -1;
bCloak = 0;
lives = -1;
afterburner = -1;
bombCount = 0;
laserLevel = 0;
weapon [0] =
weapon [1] = -1;
ammo [0] =
ammo [1] = -1;
xOmegaCharge  = -1;
}

//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------

void CCockpitInfo::Init (void)
{
nCloakFadeState = 0;
bLastHomingWarningDrawn [0] =
bLastHomingWarningDrawn [1] = -1;
addedScore [0] =
addedScore [1] = 0;
scoreTime = 0;
lastWarningBeepTime [0] =
lastWarningBeepTime [1] = 0;
bHaveGaugeCanvases = 0;
nInvulnerableFrame = 0;
weaponBoxStates [0] =
weaponBoxStates [1] = 0;
weaponBoxFadeValues [0] =
weaponBoxFadeValues [1] = 0;
weaponBoxUser [0] =
weaponBoxUser [1] = WBU_WEAPON;
bRebuild = false;
}

//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------

void CGenericCockpit::Init (void)
{
m_history [0].Init ();
m_history [1].Init ();
m_info.Init ();
if (!m_save.Buffer ())
	m_save.Create (3);
}

//	---------------------------------------------------------------------------------------------------------
//draws a 3d view into one of the cockpit windows.  win is 0 for left,
//1 for right.  viewerP is CObject.  NULL CObject means give up window
//nUser is one of the WBU_ constants.  If bRearView is set, show a
//rear view.  If label is non-NULL, print the label at the top of the
//window.

void CGenericCockpit::RenderWindow (int nWindow, CObject *viewerP, int bRearView, int nUser, const char *pszLabel)
{
if (Hide ())
	return;
if (gameStates.app.bPlayerIsDead)
	return;
if (gameStates.app.bEndLevelSequence >= EL_LOOKBACK)
	return;
if ((gameStates.render.cockpit.nType >= CM_FULL_SCREEN) && (gameStates.zoom.nFactor > gameStates.zoom.nMinFactor))
	return;

	static CCanvas overlapCanv;

	CObject*	viewerSave = gameData.objs.viewerP;
	int		bRearViewSave = gameStates.render.bRearView;
	int		nWindowSave = gameStates.render.nWindow [0];
	fix		xStereoSeparation = ogl.StereoSeparation ();
	float		nZoomSave;

	static int bOverlapDirty [2] = {0, 0};
	static int y, x;

if (!viewerP) {								//this nUser is done
	Assert (nUser == WBU_WEAPON || nUser == WBU_STATIC);
	if ((nUser == WBU_STATIC) && (m_info.weaponBoxUser [nWindow] != WBU_STATIC))
		staticTime [nWindow] = 0;
	if (m_info.weaponBoxUser [nWindow] == WBU_WEAPON || m_info.weaponBoxUser [nWindow] == WBU_STATIC)
		return;		//already set
	m_info.weaponBoxUser [nWindow] = nUser;
	if (bOverlapDirty [nWindow])
		bOverlapDirty [nWindow] = 0;
	return;
	}
UpdateRenderedData (nWindow + 1, viewerP, bRearView, nUser);
m_info.weaponBoxUser [nWindow] = nUser;						//say who's using window
gameData.objs.viewerP = viewerP;
gameStates.render.bRearView = -bRearView;
transformation.Push ();
SetupWindow (nWindow);
gameData.render.window.Activate (&gameData.render.scene);
fontManager.SetCurrent (GAME_FONT);
nZoomSave = gameStates.zoom.nFactor;
gameStates.zoom.nFactor = float (I2X (gameOpts->render.cockpit.nWindowZoom + 1));					//the player's zoom factor
if (((nUser != WBU_RADAR_TOPDOWN) && (nUser != WBU_RADAR_HEADSUP)) ||
	(IsMultiGame && !(netGame.m_info.gameFlags & NETGAME_FLAG_SHOW_MAP)))
	RenderFrame (0, nWindow + 1);
else {
	automap.m_bDisplay = -1;
	gameStates.render.SetRenderWindow (nWindow + 1);
	automap.DoFrame (0, 1 + (nUser == WBU_RADAR_TOPDOWN));
	automap.m_bDisplay = 0;
	}
gameStates.render.SetRenderWindow (nWindowSave);
gameStates.zoom.nFactor = nZoomSave;
ogl.SetStereoSeparation (xStereoSeparation);
transformation.Pop ();
if (ogl.StereoDevice () < 0)
	ogl.ChooseDrawBuffer ();
//gameData.render.frame.SetViewport ();
gameData.render.window.Activate (&gameData.render.scene);

//	HACK!If guided missile, wake up robots as necessary.
if (viewerP->info.nType == OBJ_WEAPON) 
	WakeupRenderedObjects (viewerP, nWindow + 1);

ogl.SetDepthTest (false);
if (pszLabel) {
	SetFontColor (GREEN_RGBA);
	DrawHUDText (NULL, 0x8000, 2, pszLabel);
	}

if (nUser == WBU_GUIDED)
	DrawGuidedCrosshairs (m_info.xStereoSeparation);

if (gameStates.render.cockpit.nType >= CM_FULL_SCREEN) {
	CCanvas::Current ()->SetColorRGBi (gameStates.app.bNostalgia ? RGB_PAL (0, 0, 32) : RGB_PAL (47, 31, 0));
	glLineWidth (float (gameData.render.screen.Width ()) / 640.0f);
	OglDrawEmptyRect (0, 0, CCanvas::Current ()->Width () - 1, CCanvas::Current ()->Height ());
	glLineWidth (1);
#if 0
	int smallWindowBottom, bigWindowBottom, extraPartHeight;

	//if the window only partially overlaps the big 3d window, copy
	//the extra part to the visible screen
	bigWindowBottom = gameData.render.scene.Top () + gameData.render.scene.Height () - 1;
	if (x > bigWindowBottom) {
		//the small window is completely outside the big 3d window, so
		//copy it to the visible screen
		gameData.render.scene.BlitClipped (y, x);
		bOverlapDirty [nWindow] = 1;
		}
	else {
		smallWindowBottom = x + gameData.render.scene.Height () - 1;
		if (0 < (extraPartHeight = smallWindowBottom - bigWindowBottom)) {
			gameData.render.scene.SetupPane (&overlapCanv, 0, gameData.render.scene.Height ()-extraPartHeight, gameData.render.scene.Width (), extraPartHeight);
			overlapCanv.BlitClipped (y, bigWindowBottom + 1);
			bOverlapDirty [nWindow] = 1;
			}
		}
#endif
	}
//force redraw when done
m_history [0].weapon [nWindow] = m_history [0].ammo [nWindow] = -1;

gameData.objs.viewerP = viewerSave;
#if 0
// draw a thicker frame with rounded edges around the cockpit displays
if (!gameStates.app.bNostalgia && (gameStates.render.cockpit.nType >= CM_FULL_SCREEN)) {
	int x0 = gameData.render.scene.Left ();
	int y0 = gameData.render.scene.Top () - CCanvas::Current ()->Top ();
	int x1 = gameData.render.scene.Right () - 1;
	int y1 = gameData.render.scene.Bottom () - CCanvas::Current ()->Top ();
	CCanvas::Current ()->SetColorRGBi (RGB_PAL (47, 31, 0));
	//CCanvas::Current ()->SetColorRGBi (RGB_PAL (0, 0, 32));
	//CCanvas::Current ()->SetColorRGBi (RGB_PAL (60, 60, 60));
	//CCanvas::Current ()->SetColorRGBi (RGB_PAL (31, 31, 31));
	OglDrawLine (x0 + 1, y0 - 1, x1 - 1, y0 - 1);
	OglDrawLine (x0 + 1, y1 + 1, x1 - 1, y1 + 1);
	OglDrawLine (x0 - 1, y0 + 1, x0 - 1, y1 - 1);
	OglDrawLine (x1 + 1, y0 + 1, x1 + 1, y1 - 1);
	//CCanvas::Current ()->SetColorRGBi (RGB_PAL (30, 30, 30));
	OglDrawLine (x0 + 3, y0 - 2, x1 - 3, y0 - 2);
	OglDrawLine (x0 + 3, y1 + 2, x1 - 3, y1 + 2);
	OglDrawLine (x0 - 2, y0 + 3, x0 - 2, y1 - 3);
	OglDrawLine (x1 + 2, y0 + 3, x1 + 2, y1 - 3);
	}
#endif
ogl.SetDepthTest (true);
gameData.render.window.Deactivate ();
gameStates.render.bRearView = bRearViewSave;
}

//------------------------------------------------------------------------------

void CGenericCockpit::RenderWindows (void)
{
if (ogl.IsOculusRift ())
	return;

	int		bDidMissileView = 0;
	int		saveNewDemoState = gameData.demo.nState;
	int		w;

if (gameData.demo.nState == ND_STATE_PLAYBACK) {
   if (nDemoDoLeft) {
      if (nDemoDoLeft == 3)
			cockpit->RenderWindow (0, gameData.objs.consoleP, 1, WBU_REAR, "REAR");
      else
			cockpit->RenderWindow (0, &demoLeftExtra, bDemoRearCheck [nDemoDoLeft], nDemoWBUType [nDemoDoLeft], szDemoExtraMessage [nDemoDoLeft]);
		}
   else
		cockpit->RenderWindow (0, NULL, 0, WBU_WEAPON, NULL);
	if (nDemoDoRight) {
      if (nDemoDoRight == 3)
			cockpit->RenderWindow (1, gameData.objs.consoleP, 1, WBU_REAR, "REAR");
      else
			cockpit->RenderWindow (1, &demoRightExtra, bDemoRearCheck [nDemoDoRight], nDemoWBUType [nDemoDoRight], szDemoExtraMessage [nDemoDoRight]);
		}
   else
		cockpit->RenderWindow (1, NULL, 0, WBU_WEAPON, NULL);
   nDemoDoLeft = nDemoDoRight = 0;
	nDemoDoingLeft = nDemoDoingRight = 0;
   return;
   }
bDidMissileView = RenderMissileView ();
for (w = 0; w < 2 - bDidMissileView; w++) {
	//show special views if selected
	switch (gameStates.render.cockpit.n3DView [w]) {
		case CV_NONE:
			gameStates.render.nRenderingType = 255;
			cockpit->RenderWindow (w, NULL, 0, WBU_WEAPON, NULL);
			break;

		case CV_REAR:
			if (gameStates.render.bRearView) {		//if big window is rear view, show front here
				gameStates.render.nRenderingType = 3+ (w<<4);
				cockpit->RenderWindow (w, gameData.objs.consoleP, 0, WBU_REAR, "FRONT");
				}
			else {					//show Normal rear view
				gameStates.render.nRenderingType = 3+ (w<<4);
				cockpit->RenderWindow (w, gameData.objs.consoleP, 1, WBU_REAR, "REAR");
				}
			break;

		case CV_ESCORT: {
			CObject *buddy = FindEscort ();
			if (!buddy) {
				cockpit->RenderWindow (w, NULL, 0, WBU_WEAPON, NULL);
				gameStates.render.cockpit.n3DView [w] = CV_NONE;
				}
			else {
				gameStates.render.nRenderingType = 4+ (w<<4);
				cockpit->RenderWindow (w, buddy, 0, WBU_ESCORT, gameData.escort.szName);
				}
			break;
			}

		case CV_COOP: {
			int nPlayer = gameStates.render.cockpit.nCoopPlayerView [w];
	      gameStates.render.nRenderingType = 255; // don't handle coop stuff
			if ((nPlayer != -1) && gameData.multiplayer.players [nPlayer].Connected () && SameTeam (nPlayer, N_LOCALPLAYER))
				cockpit->RenderWindow (w, &OBJECTS [gameData.multiplayer.players [gameStates.render.cockpit.nCoopPlayerView [w]].nObject], 0, WBU_COOP, gameData.multiplayer.players [gameStates.render.cockpit.nCoopPlayerView [w]].callsign);
			else {
				cockpit->RenderWindow (w, NULL, 0, WBU_WEAPON, NULL);
				gameStates.render.cockpit.n3DView [w] = CV_NONE;
				}
			break;
			}

		case CV_MARKER: {
			char label [10];
			short v = markerManager.Viewer (w);
			gameStates.render.nRenderingType = 5 + (w << 4);
			if ((v == -1) || (markerManager.Objects (v) == -1)) {
				gameStates.render.cockpit.n3DView [w] = CV_NONE;
				break;
				}
			sprintf (label, "Marker %d", markerManager.Viewer (w) + 1);
			cockpit->RenderWindow (w, OBJECTS + markerManager.Objects (markerManager.Viewer (w)), 0, WBU_MARKER, label);
			break;
			}

		case CV_RADAR_TOPDOWN:
		case CV_RADAR_HEADSUP:
			if (!(gameStates.app.bNostalgia || COMPETITION) && EGI_FLAG (bRadarEnabled, 0, 1, 0))
				cockpit->RenderWindow (w, gameData.objs.consoleP, 0,
					(gameStates.render.cockpit.n3DView [w] == CV_RADAR_TOPDOWN) ? WBU_RADAR_TOPDOWN : WBU_RADAR_HEADSUP, "MINI MAP");
			else
				gameStates.render.cockpit.n3DView [w] = CV_NONE;
			break;
		default:
			Int3 ();		//invalid window nType
		}
	}
gameStates.render.nRenderingType = 0;
gameData.demo.nState = saveNewDemoState;
}

//	-----------------------------------------------------------------------------
//draw all the things on the HUD

void CGenericCockpit::Render (int bExtraInfo, fix xStereoSeparation)
{
#if DBG
extern int bHave3DCockpit;
if (bHave3DCockpit > 0)
	return;
#endif
if (Hide ()) 
	return;
if (gameStates.app.bPlayerIsDead)
	return;
if (gameStates.render.bChaseCam || (gameStates.render.bFreeCam > 0)) {
#if DBG
	HUDRenderMessageFrame ();
#endif
	return;
	}
#if 1
if (!cockpit->Setup (false))
	return;
#else
if (!cockpit->Setup (true))
	return;
#endif
Canvas ()->Activate ();
ogl.SetDepthMode (GL_ALWAYS);
ogl.SetBlendMode (OGL_BLEND_ALPHA);
ogl.ColorMask (1,1,1,1,0);
CCanvas::Current ()->SetColorRGBi (BLACK_RGBA);
fontManager.SetCurrent (GAME_FONT);
cockpit->SetColor (WHITE_RGBA);
cockpit->SetLineSpacing (m_info.nLineSpacing);
m_info.fontWidth = CCanvas::Current ()->Font ()->Width ();
m_info.fontHeight = CCanvas::Current ()->Font ()->Height ();
m_info.xStereoSeparation = xStereoSeparation;
#if 0
m_info.xScale = gameData.render.scene.XScale ();
m_info.yScale = gameData.render.scene.YScale ();
#else
m_info.xScale = Canvas ()->XScale ();
m_info.yScale = Canvas ()->YScale ();
#endif
fontManager.SetScale (floor (float (CCanvas::Current ()->Width ()) / 640.0f));
m_info.nLineSpacing = int (GAME_FONT->Height () + GAME_FONT->Height () * fontManager.Scale () / 4);
fontManager.SetScale (1.0f);
m_info.heightPad = (ScaleY (m_info.fontHeight) - m_info.fontHeight) / 2;
m_info.nDamage [0] = gameData.objs.consoleP->AimDamage ();
m_info.nDamage [1] = gameData.objs.consoleP->DriveDamage ();
m_info.nDamage [2] = gameData.objs.consoleP->GunDamage ();
m_info.bCloak = ((LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED) != 0);
m_info.nCockpit = (gameStates.video.nDisplayMode && !gameStates.app.bDemoData) ? gameData.models.nCockpits / 2 : 0;
m_info.nEnergy = LOCALPLAYER.EnergyLevel ();
if (m_info.nEnergy < 0)
	m_info.nEnergy  = 0;
m_info.nShield = LOCALPLAYER.ShieldLevel ();
if (m_info.nShield < 0)
	m_info.nShield  = 0;
m_info.bCloak = ((LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED) != 0);
m_info.tInvul = (LOCALPLAYER.invulnerableTime == 0x7fffffff) ? LOCALPLAYER.invulnerableTime : LOCALPLAYER.invulnerableTime + INVULNERABLE_TIME_MAX - gameData.time.xGame;
m_info.nColor = WHITE_RGBA;

if (gameOpts->render.cockpit.bScaleGauges) {
	m_info.xGaugeScale = float (Canvas ()->Height ()) / 480.0f;
	m_info.yGaugeScale = float (Canvas ()->Height ()) / 640.0f;
	}
else
	m_info.xGaugeScale =
	m_info.yGaugeScale = 1;

CCanvas::Current ()->SetColorRGBi (BLACK_RGBA);
fontManager.SetCurrent (GAME_FONT);

DrawReticle (ogl.StereoDevice () < 0);

if ((gameOpts->render.cockpit.bHUD > 1) && (gameStates.zoom.nFactor == float (gameStates.zoom.nMinFactor))) {
	if (ogl.IsOculusRift ()) {
		int w = gameData.render.frame.Width (false) / 2;
		int h = gameData.render.screen.Height (false) * w / gameData.render.screen.Width (false);
		int nOffsetSave = gameData.SetStereoOffsetType (STEREO_OFFSET_NONE);
		gameData.render.window.Setup (&gameData.render.frame, w / 2 - CScreen::Unscaled (gameData.StereoOffset2D ()), (gameData.render.screen.Height (false) - h) / 2, w, h); 
		SetCanvas (&gameData.render.window);
		gameData.render.window.Activate (&gameData.render.frame);
		ogl.SetDepthTest (false);
#if 0
		DrawEnergyLevels ();
		DrawModuleDamage ();
		DrawScore ();
		DrawKeys ();
		DrawFlag ();
		DrawOrbs ();
		DrawLives ();
		DrawHomingWarning ();
		hudIcons.Render ();
#endif
#if DBG
		CCanvas::Current ()->SetColorRGBi (gameStates.app.bNostalgia ? RGB_PAL (0, 0, 32) : RGB_PAL (47, 31, 0));
		glLineWidth (float (gameData.render.screen.Width ()) / 640.0f);
		OglDrawEmptyRect (0, 0, CCanvas::Current ()->Width () - 1, CCanvas::Current ()->Height ());
		glLineWidth (1);
#endif
		gameData.SetStereoOffsetType (nOffsetSave);
		gameData.render.window.Deactivate ();
		SetCanvas (&gameData.render.scene);
		}
	else {
		if ((gameData.demo.nState == ND_STATE_PLAYBACK))
			gameData.app.SetGameMode (gameData.demo.nGameMode);

		bool bLimited = (gameStates.render.bRearView || gameStates.render.bChaseCam || (gameStates.render.bFreeCam > 0));

		if (!bLimited) {
			DrawPlayerNames ();
			RenderWindows ();
			}
		DrawCockpit (false);
		if (bExtraInfo) {
		#if DBG
			DrawWindowLabel ();
		#endif
			DrawMultiMessage ();
			DrawMarkerMessage ();
			DrawFrameRate ();
			DrawCruise ();
			}
		DrawPacketLoss ();
		DrawSlowMotion ();
		DrawPlayerStats ();
		DrawScore ();
		if (m_info.scoreTime)
			DrawAddedScore ();
		DrawEnergyLevels ();
		DrawModuleDamage ();
		DrawWeapons ();
		DrawTimerCount ();
		DrawCloak ();
		DrawInvul ();
		if (!bLimited) {
			DrawTime ();
			DrawKeys ();
			DrawFlag ();
			DrawOrbs ();
			DrawLives ();
			DrawBombCount ();
			DrawHomingWarning ();
			DrawKillList ();
			DrawPlayerShip ();
			}
		hudIcons.Render ();
		if (bExtraInfo) {
			DrawCountdown ();
			DrawRecording ();
			}

		if ((gameData.demo.nState == ND_STATE_PLAYBACK))
			gameData.app.SetGameMode (GM_NORMAL);

		if (gameStates.app.bPlayerIsDead)
			PlayerDeadMessage ();

		if (!gameStates.render.bRearView)
			HUDRenderMessageFrame ();
		else if (gameStates.render.cockpit.nType != CM_REAR_VIEW) {
			HUDRenderMessageFrame ();
			SetFontColor (GREEN_RGBA);
			DrawHUDText (NULL, 0x8000, (gameData.demo.nState == ND_STATE_PLAYBACK) ? -14 : -10, TXT_REAR_VIEW);
			}
		}
	}
DemoRecording ();
Canvas ()->Deactivate ();
m_history [0].bCloak = m_info.bCloak;
}

//------------------------------------------------------------------------------
//initialize the various canvases on the game screen
//called every time the screen mode or cockpit changes
bool CGenericCockpit::Setup (bool bRebuild)
{
if (gameStates.video.nScreenMode != SCREEN_GAME)
	return false;
if (gameData.demo.nState == ND_STATE_RECORDING)
	NDRecordCockpitChange (gameStates.render.cockpit.nType);
if (gameStates.video.nScreenMode == SCREEN_EDITOR)
	gameStates.render.cockpit.nType = CM_FULL_SCREEN;
gameData.render.scene.Setup (&gameData.render.frame); // OpenGL viewport must be properly set here
fontManager.SetCurrent (GAME_FONT);
SetFontScale (1.0f);
SetFontColor (GREEN_RGBA);
SetCanvas (&gameData.render.scene);
if (bRebuild)
	gameStates.render.cockpit.nShieldFlash = 0;
return true;
}

//------------------------------------------------------------------------------

void CGenericCockpit::Activate (int nType, bool bClearMessages)
{
if (ogl.IsOculusRift ())
	cockpit = &hudCockpit;
else if (nType == CM_FULL_COCKPIT)
	cockpit = &fullCockpit;
else if (nType == CM_STATUS_BAR)
	cockpit = &statusBarCockpit;
else if (nType == CM_FULL_SCREEN)
	cockpit = &hudCockpit;
else if (nType == CM_LETTERBOX)
	cockpit = &letterboxCockpit;
else if (nType == CM_REAR_VIEW) {
	cockpit = &rearViewCockpit;
	gameStates.render.bRearView = 1;
	}
else
	return;
gameStates.render.cockpit.nType = nType;
gameStates.zoom.nFactor = float (gameStates.zoom.nMinFactor);
m_info.nCockpit = (gameStates.video.nDisplayMode && !gameStates.app.bDemoData) ? gameData.models.nCockpits / 2 : 0;
gameStates.render.cockpit.nNextType = -1;
cockpit->Setup (false);
if (bClearMessages)
	HUDClearMessages ();
if (!gameStates.app.bPlayerIsDead)
	SavePlayerProfile ();
}

//	-----------------------------------------------------------------------------

bool CGenericCockpit::Save (bool bInitial)
{
if (bInitial && IsSaved ())
	return true;
return m_save.Push (gameStates.render.cockpit.nTypeSave = gameStates.render.cockpit.nType);
}

//	-----------------------------------------------------------------------------

bool CGenericCockpit::Restore (void)
{
if (!m_save.ToS ()) {
	gameStates.render.cockpit.nTypeSave = -1;
	return false;
	}
cockpit->Activate (m_save.Pop ());
gameStates.render.cockpit.nTypeSave = m_save.ToS () ? * m_save.Top () : -1;
return true;
}

//	-----------------------------------------------------------------------------

void CGenericCockpit::Rewind (bool bActivate)
{
if (bActivate)
	while (Restore ())
		;
else {
	m_save.Reset ();
	gameStates.render.cockpit.nTypeSave = -1;
	}
}

//	-----------------------------------------------------------------------------

bool CGenericCockpit::IsSaved (void)
{
return m_save.ToS () > 0;
}

//	-----------------------------------------------------------------------------
