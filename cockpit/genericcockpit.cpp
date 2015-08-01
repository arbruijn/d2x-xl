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

CStack<int32_t>	CGenericCockpit::m_save;

bool	CCockpitInfo::bWindowDrawn [2];

CGenericCockpit* cockpit = &fullCockpit;

//	-----------------------------------------------------------------------------

void DrawGuidedCrosshairs (fix xStereoSeparation);
void DrawWindowLabel (void);
bool GuidedMissileActive (void);

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

//	-----------------------------------------------------------------------------

void CGenericCockpit::ScaleUp (void)
{
m_info.xScale *= float (HUD_ASPECT);
if (ogl.IsOculusRift ()) {
	m_info.xScale *= 0.5f;
	m_info.yScale *= 0.5f;
	}
SetFontScale (sqrt (floor (float (CCanvas::Current ()->Width ()) / 640.0f)));
fontManager.PushScale ();
fontManager.SetScale (cockpit->FontScale ());
}

//	-----------------------------------------------------------------------------

void CGenericCockpit::ScaleDown (void)
{
if (ogl.IsOculusRift ()) {
	m_info.xScale *= 2.0f;
	m_info.yScale *= 2.0f;
	}
m_info.xScale /= float (HUD_ASPECT);
fontManager.PopScale ();
SetFontScale (1.0f);
}

//	---------------------------------------------------------------------------------------------------------
//draws a 3d view into one of the cockpit windows.  win is 0 for left,
//1 for right.  pViewer is CObject.  NULL CObject means give up window
//nUser is one of the WBU_ constants.  If bRearView is set, show a
//rear view.  If label is non-NULL, print the label at the top of the
//window.

void CGenericCockpit::RenderWindow (int32_t nWindow, CObject *pViewer, int32_t bRearView, int32_t nUser, const char *pszLabel)
{
ENTER (0, 0);
if (Hide ())
	RETURN
if (gameStates.app.bPlayerIsDead)
	RETURN
if (gameStates.app.bEndLevelSequence >= EL_LOOKBACK)
	RETURN
if ((gameStates.render.cockpit.nType >= CM_FULL_SCREEN) && (gameStates.zoom.nFactor > gameStates.zoom.nMinFactor))
	RETURN

	static CCanvas overlapCanv;

	int32_t	bRearViewSave = gameStates.render.bRearView;
	int32_t	bGlowSave = gameOpts->render.effects.bGlow;
	int32_t	nWindowSave = gameStates.render.nWindow [0];
	fix		xStereoSeparation = ogl.StereoSeparation ();
	float		nZoomSave;

	static int32_t bOverlapDirty [2] = {0, 0};

if (!pViewer) {								//this nUser is done
	Assert (nUser == WBU_WEAPON || nUser == WBU_STATIC);
	if ((nUser == WBU_STATIC) && (m_info.weaponBoxUser [nWindow] != WBU_STATIC))
		staticTime [nWindow] = 0;
	if (m_info.weaponBoxUser [nWindow] == WBU_WEAPON || m_info.weaponBoxUser [nWindow] == WBU_STATIC)
		RETURN		//already set
	m_info.weaponBoxUser [nWindow] = nUser;
	if (bOverlapDirty [nWindow])
		bOverlapDirty [nWindow] = 0;
	RETURN
	}
UpdateRenderedData (nWindow + 1, pViewer, bRearView, nUser);
m_info.weaponBoxUser [nWindow] = nUser;						//say who's using window
CObject* pViewerSave = gameData.SetViewer (pViewer);
gameStates.render.bRearView = -bRearView;
transformation.Push ();
SetupWindow (nWindow);
fontManager.SetCurrent (GAME_FONT);
//gameOpts->render.effects.bGlow = Min (gameOpts->render.effects.bGlow, 1);
nZoomSave = gameStates.zoom.nFactor;
gameStates.zoom.nFactor = float (I2X (gameOpts->render.cockpit.nWindowZoom + 1));					//the player's zoom factor
if (((nUser != WBU_RADAR_TOPDOWN) && (nUser != WBU_RADAR_HEADSUP)) ||
	 (IsMultiGame && !(netGameInfo.m_info.gameFlags & NETGAME_FLAG_SHOW_MAP))) {
	RenderFrame (0, nWindow + 1);
	}
else {
	automap.SetActive (-1);
	gameStates.render.SetRenderWindow (nWindow + 1);
	automap.DoFrame (0, 1 + (nUser == WBU_RADAR_TOPDOWN));
	automap.SetActive (0);
	}
gameStates.render.SetRenderWindow (nWindowSave);
gameOpts->render.effects.bGlow = bGlowSave;
gameStates.zoom.nFactor = nZoomSave;
ogl.SetStereoSeparation (xStereoSeparation);
transformation.Pop ();
if (ogl.StereoDevice () < 0)
	ogl.ChooseDrawBuffer ();
//gameData.renderData.frame.SetViewport ();
gameData.renderData.window.Activate ("GenericCockpit::RenderWindow", gameData.renderData.window.Parent ());

//	HACK!If guided missile, wake up robots as necessary.
if (pViewer->info.nType == OBJ_WEAPON) 
	WakeupRenderedObjects (pViewer, nWindow + 1);

ogl.SetDepthTest (false);
if (pszLabel) {
	SetFontColor (GREEN_RGBA);
	DrawHUDText (NULL, 0x8000, 2, pszLabel);
	}

if (nUser == WBU_GUIDED)
	DrawGuidedCrosshairs (m_info.xStereoSeparation);

if (gameStates.render.cockpit.nType >= CM_FULL_SCREEN) {
	CCanvas::Current ()->SetColorRGBi (gameStates.app.bNostalgia ? RGB_PAL (0, 0, 32) : RGB_PAL (47, 31, 0));
	glLineWidth (float (gameData.renderData.screen.Width ()) / 640.0f);
	OglDrawEmptyRect (0, 0, CCanvas::Current ()->Width () - 1, CCanvas::Current ()->Height ());
	glLineWidth (1);
#if 0
	static int32_t y, x;

	int32_t smallWindowBottom, bigWindowBottom, extraPartHeight;

	//if the window only partially overlaps the big 3d window, copy
	//the extra part to the visible screen
	bigWindowBottom = gameData.renderData.scene.Top () + gameData.renderData.scene.Height () - 1;
	if (x > bigWindowBottom) {
		//the small window is completely outside the big 3d window, so
		//copy it to the visible screen
		gameData.renderData.scene.BlitClipped (y, x);
		bOverlapDirty [nWindow] = 1;
		}
	else {
		smallWindowBottom = x + gameData.renderData.scene.Height () - 1;
		if (0 < (extraPartHeight = smallWindowBottom - bigWindowBottom)) {
			gameData.renderData.scene.SetupPane (&overlapCanv, 0, gameData.renderData.scene.Height ()-extraPartHeight, gameData.renderData.scene.Width (), extraPartHeight);
			overlapCanv.BlitClipped (y, bigWindowBottom + 1);
			bOverlapDirty [nWindow] = 1;
			}
		}
#endif
	}
//force redraw when done
m_history [0].weapon [nWindow] = m_history [0].ammo [nWindow] = -1;

gameData.SetViewer (pViewerSave);
ogl.SetDepthTest (true);
gameData.renderData.window.Deactivate ();
gameStates.render.bRearView = bRearViewSave;
RETURN
}

//------------------------------------------------------------------------------

void CGenericCockpit::RenderWindows (void)
{
ENTER (0, 0);
if (ogl.IsOculusRift ())
	RETURN

	int32_t		bDidMissileView = 0;
	int32_t		saveNewDemoState = gameData.demoData.nState;
	int32_t		w;

if (gameData.demoData.nState == ND_STATE_PLAYBACK) {
   if (nDemoDoLeft) {
      if (nDemoDoLeft == 3)
			cockpit->RenderWindow (0, gameData.objData.pConsole, 1, WBU_REAR, "REAR");
      else
			cockpit->RenderWindow (0, &demoLeftExtra, bDemoRearCheck [nDemoDoLeft], nDemoWBUType [nDemoDoLeft], szDemoExtraMessage [nDemoDoLeft]);
		}
   else
		cockpit->RenderWindow (0, NULL, 0, WBU_WEAPON, NULL);
	if (nDemoDoRight) {
      if (nDemoDoRight == 3)
			cockpit->RenderWindow (1, gameData.objData.pConsole, 1, WBU_REAR, "REAR");
      else
			cockpit->RenderWindow (1, &demoRightExtra, bDemoRearCheck [nDemoDoRight], nDemoWBUType [nDemoDoRight], szDemoExtraMessage [nDemoDoRight]);
		}
   else
		cockpit->RenderWindow (1, NULL, 0, WBU_WEAPON, NULL);
   nDemoDoLeft = nDemoDoRight = 0;
	nDemoDoingLeft = nDemoDoingRight = 0;
   RETURN
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
				cockpit->RenderWindow (w, gameData.objData.pConsole, 0, WBU_REAR, "FRONT");
				}
			else {					//show Normal rear view
				gameStates.render.nRenderingType = 3+ (w<<4);
				cockpit->RenderWindow (w, gameData.objData.pConsole, 1, WBU_REAR, "REAR");
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
				cockpit->RenderWindow (w, buddy, 0, WBU_ESCORT, gameData.escortData.szName);
				}
			break;
			}

		case CV_COOP: {
			int32_t nPlayer = gameStates.render.cockpit.nCoopPlayerView [w];
	      gameStates.render.nRenderingType = 255; // don't handle coop stuff
			if ((nPlayer != -1) && PLAYER (nPlayer).IsConnected () && SameTeam (nPlayer, N_LOCALPLAYER))
				cockpit->RenderWindow (w, PLAYEROBJECT (gameStates.render.cockpit.nCoopPlayerView [w]), 0, WBU_COOP, PLAYER (gameStates.render.cockpit.nCoopPlayerView [w]).callsign);
			else {
				cockpit->RenderWindow (w, NULL, 0, WBU_WEAPON, NULL);
				gameStates.render.cockpit.n3DView [w] = CV_NONE;
				}
			break;
			}

		case CV_MARKER: {
			char label [10];
			int16_t v = markerManager.Viewer (w);
			gameStates.render.nRenderingType = 5 + (w << 4);
			if ((v == -1) || (markerManager.Objects (v) == -1)) {
				gameStates.render.cockpit.n3DView [w] = CV_NONE;
				break;
				}
			sprintf (label, "Marker %d", markerManager.Viewer (w) + 1);
			cockpit->RenderWindow (w, OBJECT (markerManager.Objects (markerManager.Viewer (w))), 0, WBU_MARKER, label);
			break;
			}

		case CV_RADAR_TOPDOWN:
		case CV_RADAR_HEADSUP:
			if (!(gameStates.app.bNostalgia || COMPETITION) && EGI_FLAG (bRadarEnabled, 0, 1, 0))
				cockpit->RenderWindow (w, gameData.objData.pConsole, 0,
					(gameStates.render.cockpit.n3DView [w] == CV_RADAR_TOPDOWN) ? WBU_RADAR_TOPDOWN : WBU_RADAR_HEADSUP, "MINI MAP");
			else
				gameStates.render.cockpit.n3DView [w] = CV_NONE;
			break;
		default:
			Int3 ();		//invalid window nType
		}
	}
gameStates.render.nRenderingType = 0;
gameData.demoData.nState = saveNewDemoState;
RETURN
}

//	-----------------------------------------------------------------------------

void CGenericCockpit::SetupSceneCenter (CCanvas* refCanv, int32_t& w, int32_t& h)
{
ENTER (0, 0);
if (ogl.IsOculusRift ()) {
	w = refCanv->Width (false) / 2;
	h = refCanv->Height (false) * w / refCanv->Width (false);
	gameData.renderData.window.Setup (refCanv, w / 2 - CScreen::Unscaled (gameData.StereoOffset2D ()), (gameData.renderData.screen.Height (false) - h) / 2, w, h); 
	gameData.renderData.window.Activate (refCanv->Id (), refCanv);
	}
else {
	w = refCanv->Width (false) - 3 * abs (gameData.StereoOffset2D ());
	int32_t d = abs (gameData.StereoOffset2D ());
	gameData.renderData.window.Setup (refCanv, (ogl.StereoSeparation () < 0) ? 2 * d : d, 0, w, refCanv->Height (false)); 
	gameData.renderData.window.Activate (refCanv->Id (), refCanv);
	}
#if 0 //DBG
CCanvas::Current ()->SetColorRGBi (gameStates.app.bNostalgia ? RGB_PAL (0, 0, 32) : RGB_PAL (47, 31, 0));
glLineWidth (float (gameData.renderData.screen.Width ()) / 640.0f);
OglDrawEmptyRect (0, 0, CCanvas::Current ()->Width (), CCanvas::Current ()->Height ());
glLineWidth (1);
#endif
RETURN
}


//	-----------------------------------------------------------------------------
//draw all the things on the HUD

void CGenericCockpit::Render (int32_t bExtraInfo, fix xStereoSeparation)
{
ENTER (0, 0);
#if DBG
extern int32_t bHave3DCockpit;
if (bHave3DCockpit > 0)
	RETURN
#endif
if (Hide ()) 
	RETURN
if (gameStates.app.bPlayerIsDead)
	RETURN
if (gameStates.render.bChaseCam || (gameStates.render.bFreeCam > 0)) {
#if DBG
	HUDRenderMessageFrame ();
#endif
	RETURN
	}
if (!cockpit->Setup (false, false))
	RETURN

int32_t nOffsetSave;

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
m_info.xScale = gameData.renderData.scene.XScale ();
m_info.yScale = gameData.renderData.scene.YScale ();
#else
m_info.xScale = Canvas ()->XScale ();
m_info.yScale = Canvas ()->YScale ();
#endif
#if 1
m_info.nLineSpacing = int32_t (GAME_FONT->Height () + GAME_FONT->Height () * fontManager.Scale () * 0.25f);
m_info.heightPad = 0;
#else
fontManager.PushScale ();
fontManager.SetScale (floor (float (CCanvas::Current ()->Width ()) / 640.0f));
m_info.nLineSpacing = int32_t (GAME_FONT->Height () + FRound (GAME_FONT->Height () * fontManager.Scale () * 0.25f));
fontManager.PopScale ();
m_info.heightPad = (ScaleY (m_info.fontHeight) - m_info.fontHeight) / 2;
#endif
m_info.nDamage [0] = gameData.objData.pConsole->AimDamage ();
m_info.nDamage [1] = gameData.objData.pConsole->DriveDamage ();
m_info.nDamage [2] = gameData.objData.pConsole->GunDamage ();
m_info.bCloak = ((LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED) != 0);
m_info.nCockpit = (gameStates.video.nDisplayMode && !gameStates.app.bDemoData) ? gameData.modelData.nCockpits / 2 : 0;
m_info.nEnergy = LOCALPLAYER.EnergyLevel ();
if (m_info.nEnergy < 0)
	m_info.nEnergy  = 0;
m_info.nShield = LOCALPLAYER.ShieldLevel ();
if (m_info.nShield < 0)
	m_info.nShield  = 0;
m_info.bCloak = ((LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED) != 0);
m_info.tInvul = (LOCALPLAYER.invulnerableTime == 0x7fffffff) ? LOCALPLAYER.invulnerableTime : LOCALPLAYER.invulnerableTime + INVULNERABLE_TIME_MAX - gameData.timeData.xGame;
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

//gameData.renderData.scene.Activate ("Scene");
DrawReticle (ogl.StereoDevice () < 0);
//gameData.renderData.scene.Deactivate ();

if (!GuidedMissileActive () && ((gameOpts->render.cockpit.bHUD > 1) || (gameStates.render.cockpit.nType < CM_FULL_SCREEN)) && (gameStates.zoom.nFactor == float (gameStates.zoom.nMinFactor))) {
	if (ogl.IsOculusRift () && !transformation.HaveHeadAngles ()) {
		nOffsetSave = gameData.SetStereoOffsetType (STEREO_OFFSET_NONE);
		int32_t w, h;
		SetupSceneCenter (&gameData.renderData.frame, w, h);

		DrawEnergyLevels ();
		DrawModuleDamage ();
		DrawScore ();
		DrawKeys ();
		DrawFlag ();
		DrawOrbs ();
		DrawLives ();
		DrawCloak ();
		DrawInvul ();
		DrawHomingWarning ();

		gameData.SetStereoOffsetType (nOffsetSave);
		gameData.renderData.window.Deactivate ();
		h = 4 * gameData.renderData.screen.Height (false) / 9; 
		gameData.renderData.window.Setup (&gameData.renderData.frame, w / 2 - CScreen::Unscaled (gameData.StereoOffset2D ()), (gameData.renderData.screen.Height (false) - h) / 2, w, h); 
		gameData.renderData.window.Activate ("CGenericCockpit::Render (window, 1)", &gameData.renderData.frame);
		hudIcons.Render ();
		gameData.renderData.window.Deactivate ();
		}
	else {
		int32_t bStereoOffset = ogl.IsSideBySideDevice () && ((gameStates.render.cockpit.nType == CM_FULL_SCREEN) || (gameStates.render.cockpit.nType == CM_LETTERBOX));
		if (bStereoOffset) {
			nOffsetSave = gameData.SetStereoOffsetType (STEREO_OFFSET_NONE);
			int32_t w, h;
			SetupSceneCenter (&gameData.renderData.scene, w, h);
			}
		if ((gameData.demoData.nState == ND_STATE_PLAYBACK))
			gameData.appData.SetGameMode (gameData.demoData.nGameMode);

		bool bLimited = (gameStates.render.bRearView || gameStates.render.bChaseCam || (gameStates.render.bFreeCam > 0));

		if (!bLimited) {
			DrawPlayerNames ();
			//RenderWindows ();
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
		if (!bStereoOffset)
			gameData.renderData.scene.Activate ("CGenericCockpit::Render (scene, 2)");
		hudIcons.Render ();
		if (!bStereoOffset)
			gameData.renderData.scene.Deactivate ();
		if (bExtraInfo) {
			DrawCountdown ();
			DrawRecording ();
			}

		if ((gameData.demoData.nState == ND_STATE_PLAYBACK))
			gameData.appData.SetGameMode (GM_NORMAL);

		if (gameStates.app.bPlayerIsDead)
			PlayerDeadMessage ();

		if (!gameStates.render.bRearView)
			HUDRenderMessageFrame ();
		else if (gameStates.render.cockpit.nType != CM_REAR_VIEW) {
			HUDRenderMessageFrame ();
			SetFontColor (GREEN_RGBA);
			DrawHUDText (NULL, 0x8000, (gameData.demoData.nState == ND_STATE_PLAYBACK) ? -14 : -10, TXT_REAR_VIEW);
			}
		if (bStereoOffset)
			gameData.renderData.window.Deactivate ();
		}
	}
DemoRecording ();
Canvas ()->Deactivate ();
m_history [0].bCloak = m_info.bCloak;
RETURN
}

//------------------------------------------------------------------------------
//initialize the various canvases on the game screen
//called every time the screen mode or cockpit changes
bool CGenericCockpit::Setup (bool bScene, bool bRebuild)
{
ENTER (0, 0);
if (gameStates.video.nScreenMode != SCREEN_GAME)
	RETVAL (false)
if (gameData.demoData.nState == ND_STATE_RECORDING)
	NDRecordCockpitChange (gameStates.render.cockpit.nType);
if (gameStates.video.nScreenMode == SCREEN_EDITOR)
	gameStates.render.cockpit.nType = CM_FULL_SCREEN;
gameData.renderData.scene.Setup (&gameData.renderData.frame); // OpenGL viewport must be properly set here
fontManager.SetCurrent (GAME_FONT);
SetFontScale (1.0f);
SetFontColor (GREEN_RGBA);
if (bRebuild)
	gameStates.render.cockpit.nShieldFlash = 0;
RETVAL (true)
}

//------------------------------------------------------------------------------

void CGenericCockpit::Activate (int32_t nType, bool bClearMessages)
{
ENTER (0, 0);
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
	RETURN
gameStates.render.cockpit.nType = nType;
gameStates.zoom.nFactor = float (gameStates.zoom.nMinFactor);
m_info.nCockpit = (gameStates.video.nDisplayMode && !gameStates.app.bDemoData) ? gameData.modelData.nCockpits / 2 : 0;
gameStates.render.cockpit.nNextType = -1;
cockpit->Setup (false, false);
if (bClearMessages)
	HUDClearMessages ();
if (!gameStates.app.bPlayerIsDead)
	SavePlayerProfile ();
RETURN
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
