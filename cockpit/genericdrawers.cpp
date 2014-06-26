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

//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------

int CGenericCockpit::AdjustCockpitY (int y)
{
#if 1
return y;
#else
int h = CCanvas::Current ()->Height () / 2;
int fh = fontManager.Current ()->Height () + 1;
int l = (y - h) / fh - 3 * h / fh / 4;
return h + l * fh;
#endif
}

//	-----------------------------------------------------------------------------

int CGenericCockpit::AdjustCockpitXY (char* s, int& x, int& y)
{
#if 1
return gameData.render.nStereoOffsetType;
#else
	int	h;

if (m_info.bAdjustCoords && ogl.IsOculusRift ()) {
	h = CCanvas::Current ()->Width () / 2;
	if (x >= h)
		x = h + 15;
	else {
		int sw, sh, aw;
		fontManager.Current ()->StringSize (s, sw, sh, aw);
		x = h - sw - 15;
		}
	y = AdjustCockpitY (y);
	h = gameData.SetStereoOffsetType (STEREO_OFFSET_FIXED);
	}
else
	h = gameData.render.nStereoOffsetType;
m_info.bAdjustCoords = false;
return h;
#endif
}

//	-----------------------------------------------------------------------------

CBitmap* CGenericCockpit::BitBlt (int nGauge, int x, int y, bool bScalePos, bool bScaleSize, int scale, int orient, CBitmap* bmP, CBitmap* overrideP)
{
	CBitmap*	saveBmP = NULL;

if (nGauge >= 0) {
#if DBG
	if (nGauge == nDbgGauge)
		BRP;
#endif
	PageInGauge (nGauge);
	bmP = gameData.pig.tex.bitmaps [0] + GaugeIndex (nGauge);
	if (overrideP && overrideP->Buffer ())
		saveBmP = bmP->SetOverride (overrideP);
	}
if (bmP) {
#if 0
	CBitmap* bmoP = bmP->HasOverride ();
	if (bmoP)
		bmP = bmoP;
#endif
	int w = bmP->Width ();
	int h = bmP->Height ();
	if (bScalePos) {
		x = ScaleX (x);
		y = ScaleY (y);
		}
	if (bScaleSize) {
		w = ScaleX (w);
		h = ScaleY (h);
		}
	//AdjustCockpitXY (x, y);
	if (ogl.IsSideBySideDevice ()) {
		x = int (float (x) / X2F (scale)); // unscale
		w = X (x + w); // reposition left and right coordinates
		x = X (x);
		w -= x;
		x = int (float (x) * X2F (scale)); // rescale
		}
	CCanvas::Current ()->SetColorRGBi (m_info.nColor);
	bmP->RenderScaled (x, y, w * (gameStates.app.bDemoData + 1), h * (gameStates.app.bDemoData + 1), scale, orient, &CCanvas::Current ()->Color ());
	CCanvas::Current ()->SetColorRGBi (BLACK_RGBA);
	if (saveBmP)
		bmP->SetOverride (saveBmP);
	}
return bmP;
}

//	-----------------------------------------------------------------------------

int _CDECL_ CGenericCockpit::PrintF (int *idP, int x, int y, const char *pszFmt, ...)
{
	static char szBuf [1000];
	va_list args;

va_start (args, pszFmt);
vsprintf (szBuf, pszFmt, args);
AdjustCockpitXY (szBuf, x, y);
return GrString ((x < 0) ? -x : ScaleX (x), (y < 0) ? -y : ScaleY (y), szBuf, idP);
}

//------------------------------------------------------------------------------

int _CDECL_ CGenericCockpit::DrawHUDText (int *idP, int x, int y, const char * format, ...)
{
	static char buffer [1000];
	va_list args;

va_start (args, format);
vsprintf (buffer, format, args);
if (!CCanvas::Current ()->Font ())
	CCanvas::Current ()->SetFont (GAME_FONT);
CCanvasColor fontColor = CCanvas::Current ()->FontColor (0);
fontManager.SetScale (FontScale ());
fontManager.SetColorRGBi (FontColor (), 1, 0, 0);
if (x < 0)
	x = CCanvas::Current ()->Width () + x;
else if (x == 0x8000)
	x = CenteredStringPos (buffer);
if (y < 0)
	y = CCanvas::Current ()->Height () + y;
int nOffsetSave = AdjustCockpitXY (buffer, x, y);
int nId = GrString (x, y, buffer, idP);
gameData.SetStereoOffsetType (nOffsetSave);
fontManager.SetScale (1.0f);
CCanvas::Current ()->FontColor (0) = fontColor;
return nId;
}

//------------------------------------------------------------------------------

#define MAX_MARKER_MESSAGE_LEN 120

void CGenericCockpit::DrawMarkerMessage (void)
{
	char szTemp [MAX_MARKER_MESSAGE_LEN+25];

if (markerManager.DefiningMsg ()) {
	SetFontColor (GREEN_RGBA);
   sprintf (szTemp, TXT_DEF_MARKER, markerManager.Input ());
	GrString (CenteredStringPos (szTemp), CCanvas::Current ()->Height () / 2 - 16, szTemp);
   }
}

//------------------------------------------------------------------------------

void CGenericCockpit::DrawMultiMessage (void)
{
	char szMessage [MAX_MULTI_MESSAGE_LEN + 25];

if (IsMultiGame && (gameData.multigame.msg.bSending)) {
	SetFontColor (GREEN_RGBA);
	sprintf (szMessage, "%s: %s_", TXT_MESSAGE, gameData.multigame.msg.szMsg);
	GrString (CenteredStringPos (szMessage), CCanvas::Current ()->Height () / 2 - 16, szMessage);
	}
if (IsMultiGame && gameData.multigame.msg.bDefining) {
	SetFontColor (GREEN_RGBA);
	sprintf (szMessage, "%s #%d: %s_", TXT_MACRO, gameData.multigame.msg.bDefining, gameData.multigame.msg.szMsg);
	GrString (CenteredStringPos (szMessage), CCanvas::Current ()->Height () / 2 - 16, szMessage);
	}
}

//------------------------------------------------------------------------------

void CGenericCockpit::DrawCountdown (int y)
{
if (gameStates.app.bEndLevelSequence || !gameData.reactor.bDestroyed  || (gameData.reactor.countdown.nSecsLeft < 0))
	return;

if (!IS_D2_OEM && !IS_MAC_SHARE && !IS_SHAREWARE) {    // no countdown on registered only
	//	On last level, we don't want a countdown.
	if ((missionManager.nCurrentMission == missionManager.nBuiltInMission [0]) &&
		(missionManager.nCurrentLevel == missionManager.nLastLevel)) {
		if (!IsMultiGame)
			return;
		if (gameData.app.GameMode (GM_MULTI_ROBOTS))
			return;
			}
		}
fontManager.Push ("DrawCountDown");
fontManager.SetCurrent (SMALL_FONT);
SetFontColor (GREEN_RGBA);
DrawHUDText (NULL, 0x8000, y, "T-%d s", gameData.reactor.countdown.nSecsLeft);
fontManager.Pop ();
}

//------------------------------------------------------------------------------

void CGenericCockpit::DrawCruise (int x, int y)
{
if (!gameStates.render.bRearView &&
	 (gameStates.input.nCruiseSpeed > 0) &&
	 (N_LOCALPLAYER > -1) &&
	 (gameData.objs.viewerP->info.nType == OBJ_PLAYER) &&
	 (gameData.objs.viewerP->info.nId == N_LOCALPLAYER))
	DrawHUDText (NULL, x, y, "%s %2d%%", TXT_CRUISE, X2I (gameStates.input.nCruiseSpeed));
}

//------------------------------------------------------------------------------

void CGenericCockpit::DrawRecording (int y)
{
if ((gameData.demo.nState == ND_STATE_PLAYBACK) || (gameData.demo.nState == ND_STATE_RECORDING)) {
	if (gameData.demo.nState == ND_STATE_PLAYBACK) {
		CCanvas::Current ()->SetColorRGB (PAL2RGBA (27), PAL2RGBA (0), PAL2RGBA (0), 255);
		int x = gameData.render.scene.Width () / 3;
		int h = (int) FRound (8 * GLfloat (gameData.render.screen.Height ()) / 480.0f);
		y -= gameData.render.scene.Height () - y - 2;
		CCanvas::Current ()->SetColorRGB (255, 0, 0, 200);
		OglDrawFilledRect (x, y - h, x + int (NDGetPercentDone () * float (x) / 100.0f), y);
		glLineWidth (GLfloat (gameData.render.frame.Width ()) / 640.0f);
		CCanvas::Current ()->SetColorRGB (255, 0, 0, 100);
		OglDrawEmptyRect (x, y - h, 2 * x, y);
		glLineWidth (1);
		}
	else {
		char message [128];
		int h, w, aw;

		strcpy (message, TXT_DEMO_RECORDING);
		SetFontColor (RGBA_PAL2 (27, 0, 0));
		fontManager.Current ()->StringSize (message, w, h, aw);
		DrawHUDText (NULL, (CCanvas::Current ()->Width () - w) / 2, -y - h - 2, message);
		}
	}
}

//------------------------------------------------------------------------------

void CGenericCockpit::DrawPacketLoss (void)
{
	static int nIdPacketLoss = 0;

if (IsMultiGame && networkData.nTotalMissedPackets && !automap.Display ()) {
		char	szLoss [50];
		int	w, h, aw; // position measured from lower right corner
		int	nLossRate = (1000 * networkData.nTotalMissedPackets) / (networkData.nTotalPacketsGot + networkData.nTotalMissedPackets);

	if (nLossRate > 9) {
		if (nLossRate >= 300)
			SetFontColor (RED_RGBA);
		else if (nLossRate >= 200)
			SetFontColor (ORANGE_RGBA);
		else if (nLossRate >= 100)
			SetFontColor (GOLD_RGBA);
		else if (nLossRate)
			SetFontColor (GREEN_RGBA);
		sprintf (szLoss, "packet loss: %d.%d%c%c", nLossRate / 10, nLossRate % 10, '%', '%');
		fontManager.Current ()->StringSize (szLoss, w, h, aw);
		nIdPacketLoss = DrawHUDText (&nIdPacketLoss, -w - LHX (1), -8 * (GAME_FONT->Height () + GAME_FONT->Height () / 4), szLoss);
		}
	}
}

//------------------------------------------------------------------------------

#if DBG
static float nDbgFrameRate = -1.0f;
static int nFrameCount = 0;
#endif

void CGenericCockpit::DrawFrameRate (void)
{
	static fix frameTimeList [8] = {0, 0, 0, 0, 0, 0, 0, 0};
	static fix frameTimeTotal = 0;
	static int frameTimeCounter = 0;
	static int nIdFrameRate = 0;

if (gameStates.render.bShowFrameRate) {
		static time_t t, t0 = -1;

		char	szItem [50];
		int	x = 11, y; // position measured from lower right corner

	frameTimeTotal += gameData.time.xRealFrame - frameTimeList [frameTimeCounter];
	frameTimeList [frameTimeCounter] = gameData.time.xRealFrame;
	frameTimeCounter = (frameTimeCounter + 1) % 8;
	if (gameStates.render.bShowFrameRate == 1) {
		static fix xRate = 0;

		char szRate [20];
		t = SDL_GetTicks ();
		if ((t0 < 0) || (t - t0 >= 500)) {
			t0 = t;
			xRate = frameTimeTotal ? FixDiv (I2X (1) * 8, frameTimeTotal) : 0;
			}
		sprintf (szItem, "FPS: %s", ftoa (szRate, xRate));
#if DBG
		if ((nDbgFrameRate > 0) && (X2F (xRate) < nDbgFrameRate)) {
			if (++nFrameCount > 5)
				BRP;
			}
		else
			nFrameCount = 0;
#endif
		}
	else if (gameStates.render.bShowFrameRate == 2) {
		sprintf (szItem, "Faces: %d ", gameData.render.nTotalFaces);
		}
	else if (gameStates.render.bShowFrameRate == 3) {
		sprintf (szItem, "Transp: %d ", transparencyRenderer.ItemCount ());
		}
	else if (gameStates.render.bShowFrameRate == 4) {
		sprintf (szItem, "Objects: %d/%d ", gameData.render.nTotalObjects, gameData.render.nTotalSprites);
		}
	else if (gameStates.render.bShowFrameRate == 5) {
		sprintf (szItem, "States: %d/%d ", gameData.render.nStateChanges, gameData.render.nShaderChanges);
		}
	else if (gameStates.render.bShowFrameRate == 6) {
		sprintf (szItem, "Lights/Face: %1.2f (%d)",
					(float) gameData.render.nTotalLights / (float) gameData.render.nTotalFaces,
					gameData.render.nMaxLights);
		x = 19;
		}
	if (automap.Display ())
		y = 3;
	else if (IsMultiGame)
		y = 7;
	else
		y = 6;
	gameData.render.scene.Activate ("CGenericCockpit::DrawFrameRate (scene)");
	fontManager.SetCurrent (SMALL_FONT);
	SetFontColor (ORANGE_RGBA);
	nIdFrameRate = DrawHUDText (&nIdFrameRate, -x * GAME_FONT->Width (), -y * (GAME_FONT->Height () + GAME_FONT->Height () / 4), szItem);
	gameData.render.scene.Deactivate ();
	}
}

//	-----------------------------------------------------------------------------

void CGenericCockpit::DrawSlowMotion (void)
{
if (Hide ())
	return;
if (gameStates.render.bRearView)
	return;
if ((LOCALPLAYER.Energy () <= I2X (10)) || !(LOCALPLAYER.flags & PLAYER_FLAGS_SLOWMOTION))
	return;

	char	szScore [40];
	int	w, h, aw;

if ((gameData.hud.msgs [0].nMessages > 0) &&
	 (strlen (gameData.hud.msgs [0].szMsgs [gameData.hud.msgs [0].nFirst]) > 38))
	return;
szScore [0] = (char) 1;
szScore [1] = (char) (127 + 128);
szScore [2] = (char) (127 + 128);
szScore [3] = (char) (0 + 128);
#if !DBG
if (!SlowMotionActive ())
	strcpy (szScore + 4, "          ");
else
#endif
	sprintf (szScore + 4, "M%1.1f S%1.1f ",
				gameStates.gameplay.slowmo [0].fSpeed,
				gameStates.gameplay.slowmo [1].fSpeed);
szScore [14] = (char) 1;
szScore [15] = (char) (0 + 128);
szScore [16] = (char) (127 + 128);
szScore [17] = (char) (0 + 128);
szScore [18] = (char) (0);
fontManager.Current ()->StringSize (szScore, w, h, aw);
SetFontColor (GREEN_RGBA);
DrawHUDText (NULL, -2 * w - LHX (2), 3, szScore);
}

//	-----------------------------------------------------------------------------

void CGenericCockpit::DrawPlayerStats (void)
{
if (Hide ())
	return;
if (gameStates.render.bRearView)
	return;

	int		h, w, aw, y;
	double	p [3], s [3];
	char		szStats [50];

	static int nIdStats = 0;

if (!gameOpts->render.cockpit.bPlayerStats)
	return;
SetFontColor (ORANGE_RGBA);
if (gameStates.render.cockpit.nType == CM_STATUS_BAR)
	y = 6 + m_info.nLineSpacing;
else
	y = 6 + 2 * m_info.nLineSpacing;
h = (gameData.stats.nDisplayMode - 1) / 2;
if ((gameData.stats.nDisplayMode - 1) % 2 == 0) {
	sprintf (szStats, "%s%d-%d %d-%d %d-%d",
				h ? "T:" : "",
				gameData.stats.player [h].nHits [0],
				gameData.stats.player [h].nMisses [0],
				gameData.stats.player [h].nHits [1],
				gameData.stats.player [h].nMisses [1],
				gameData.stats.player [h].nHits [0] + gameData.stats.player [h].nHits [1],
				gameData.stats.player [h].nMisses [0] + gameData.stats.player [h].nMisses [1]);
	}
else {
	s [0] = gameData.stats.player [h].nHits [0] + gameData.stats.player [h].nMisses [0];
	s [1] = gameData.stats.player [h].nHits [1] + gameData.stats.player [h].nMisses [1];
	s [2] = s [0] + s [1];
	p [0] = s [0] ? (gameData.stats.player [h].nHits [0] / s [0]) * 100 : 0;
	p [1] = s [1] ? (gameData.stats.player [h].nHits [1] / s [1]) * 100 : 0;
	p [2] = s [2] ? ((gameData.stats.player [h].nHits [0] + gameData.stats.player [h].nHits [1]) / s [2]) * 100 : 0;
	sprintf (szStats, "%s%1.1f%c %1.1f%c %1.1f%c", h ? "T:" : "", p [0], '%', p [1], '%', p [2], '%');
	}
fontManager.Current ()->StringSize (szStats, w, h, aw);
int x = CCanvas::Current ()->Width () - w - LHX (2);
if ((extraGameInfo [0].nWeaponIcons >= 3) && (CCanvas::Current ()->Height () < 670))
	x -= HUD_LHX (20);
nIdStats = GrString (x, y, szStats, &nIdStats);
}

//	-----------------------------------------------------------------------------

void CGenericCockpit::DrawTime (void)
{
if (Hide ())
	return;

if (gameStates.render.bShowTime && !(IsMultiGame && gameData.multigame.score.bShowList)) {
		int secs = X2I (LOCALPLAYER.timeLevel) % 60;
		int mins = X2I (LOCALPLAYER.timeLevel) / 60;

		static int nIdTime = 0;

	SetFontColor (GREEN_RGBA);
	nIdTime = DrawHUDText (&nIdTime, -4 * m_info.fontWidth, -4 * m_info.nLineSpacing, "%d:%02d", mins, secs);
	}
}

//	-----------------------------------------------------------------------------

void CGenericCockpit::DrawTimerCount (void)
{
if (Hide ())
	return;

	char	szScore [20];
	int	w, h, aw, i;
	fix	timevar = 0;

	static int nIdTimer = 0;

if ((gameData.hud.msgs [0].nMessages > 0) && (strlen (gameData.hud.msgs [0].szMsgs [gameData.hud.msgs [0].nFirst]) > 38))
	return;

if (IsNetworkGame && (timevar = I2X (netGame.GetPlayTimeAllowed () * 5 * 60))) {
	i = X2I (timevar - gameStates.app.xThisLevelTime) + 1;
	sprintf (szScore, "T - %5d", i);
	fontManager.Current ()->StringSize (szScore, w, h, aw);
	SetFontColor (GREEN_RGBA);
	if ((i >= 0) && !gameData.reactor.bDestroyed)
		nIdTimer = DrawHUDText (&nIdTimer, -w - LHX (10), LHX (11), szScore);
	}
}

//	-----------------------------------------------------------------------------

void CGenericCockpit::DrawFlag (int x, int y)
{
if ((gameData.app.GameMode (GM_CAPTURE)) && (LOCALPLAYER.flags & PLAYER_FLAGS_FLAG))
	BitBlt ((GetTeam (N_LOCALPLAYER) == TEAM_BLUE) ? FLAG_ICON_RED : FLAG_ICON_BLUE, x, y, false, false);
}

//	-----------------------------------------------------------------------------

void CGenericCockpit::DrawOrbs (int x, int y)
{
if (Hide ())
	return;

	static int nIdOrbs = 0, nIdEntropy [2] = {0, 0};

if (gameData.app.GameMode (GM_HOARD | GM_ENTROPY)) {
	CBitmap* bmP = BitBlt (-1, x, y, false, false, I2X (1), 0, &gameData.hoard.icon [gameStates.render.fonts.bHires].bm);

	x += 3 * bmP->Width () / 2;
	y += gameStates.render.fonts.bHires + 1;
	if (IsEntropyGame) {
		char	szInfo [20];
		int	w, h, aw;
		if (LOCALPLAYER.secondaryAmmo [PROXMINE_INDEX] >= extraGameInfo [1].entropy.nCaptureVirusThreshold)
			SetFontColor (ORANGE_RGBA);
		else
			SetFontColor (GREEN_RGBA);
		sprintf (szInfo,
			"x %d [%d]",
			LOCALPLAYER.secondaryAmmo [PROXMINE_INDEX],
			LOCALPLAYER.secondaryAmmo [SMARTMINE_INDEX]);
		nIdEntropy [0] = DrawHUDText (nIdEntropy, x, y, szInfo);
		if (gameStates.entropy.bConquering) {
			int t = (extraGameInfo [1].entropy.nCaptureTimeThreshold * 1000) -
					   (gameStates.app.nSDLTicks [0] - gameStates.entropy.nTimeLastMoved);

			if (t < 0)
				t = 0;
			fontManager.Current ()->StringSize (szInfo, w, h, aw);
			x += w;
			SetFontColor (RED_RGBA);
			sprintf (szInfo, " %d.%d", t / 1000, (t % 1000) / 100);
			nIdEntropy [1] = DrawHUDText (nIdEntropy + 1, x, y, szInfo);
			}
		}
	else
		nIdOrbs = DrawHUDText (&nIdOrbs, x, y, "x %d", LOCALPLAYER.secondaryAmmo [PROXMINE_INDEX]);
	}
}

//	-----------------------------------------------------------------------------

int CGenericCockpit::BombCount (int& nBombType)
{
if (IsEntropyGame)
	return -1;
if (!(nBombType = ArmedBomb ()))
	return -1;
if (IsHoardGame && (nBombType == PROXMINE_INDEX))
	return -1;
return LOCALPLAYER.secondaryAmmo [nBombType];
}

//	-----------------------------------------------------------------------------

void CGenericCockpit::DrawBombCount (int x, int y, int bgColor, int bShowAlways)
{
	char szBombCount [5], *t;

	static int nIdBombCount = 0;

int nBombType, nBombs = BombCount (nBombType);
if ((nBombs < 0) || ((nBombs == 0) && !bShowAlways))		//no bombs, draw nothing on HUD
	return;
#if DBG
nBombs = Min (nBombs, 99);	//only have room for 2 digits - cheating give 200
#endif
sprintf (szBombCount, "B:%02d", nBombs);
for (t = szBombCount; *t; t++)
	if (*t == '1')
		*t = '\x84';	//convert to wide '1'
m_history [0].bombCount = (nBombType == PROXMINE_INDEX) ? nBombs : -nBombs;
//ClearBombCount (bgColor);
if ((gameStates.render.cockpit.nType == CM_FULL_COCKPIT) || (gameStates.render.cockpit.nType == CM_STATUS_BAR))
	fontManager.SetScale ((float) floor (float (CCanvas::Current ()->Width ()) / 640.0f));
nIdBombCount = DrawBombCount (nIdBombCount, x, y, nBombs ? (nBombType == PROXMINE_INDEX) ? RED_RGBA : GOLD_RGBA : GREEN_RGBA, szBombCount);
fontManager.SetScale (1.0f);
}

//	-----------------------------------------------------------------------------

void CGenericCockpit::DrawAmmoInfo (int x, int y, int ammoCount, int bPrimary)
{
	char	szAmmo [16];

	static int nIdAmmo [2] = {0, 0};

CCanvas::Current ()->SetColorRGBi (RGB_PAL (0, 0, 0));
SetFontColor (RED_RGBA);
sprintf (szAmmo, "%03d", ammoCount);
Convert1s (szAmmo);
nIdAmmo [bPrimary] = DrawHUDText (&nIdAmmo [bPrimary], x, y, szAmmo);
}

//	-----------------------------------------------------------------------------

void CGenericCockpit::DrawPlayerShip (int nCloakState, int nOldCloakState, int x, int y)
{
	static fix xCloakFadeTimer = 0;
	static int nCloakFadeValue = FADE_LEVELS - 1;

if (nCloakState) {
	if (nOldCloakState == -1)
		nCloakFadeValue = 0;
	else if (!nOldCloakState)
		m_info.nCloakFadeState = -1;
	}
else {
	nCloakFadeValue = FADE_LEVELS - 1;
	m_info.nCloakFadeState = 0;
	}
if (nCloakState && (LOCALPLAYER.cloakTime != 0x7fffffff) && (gameData.time.xGame > LOCALPLAYER.cloakTime + CLOAK_TIME_MAX - I2X (3)))	{	//doing "about-to-uncloak" effect
	nCloakState = 2;
	if (!m_info.nCloakFadeState)
		m_info.nCloakFadeState = 2;
	}
if (m_info.nCloakFadeState)
	xCloakFadeTimer -= gameData.time.xFrame;
while (m_info.nCloakFadeState && (xCloakFadeTimer < 0)) {
	xCloakFadeTimer += CLOAK_FADE_WAIT_TIME;
	nCloakFadeValue += m_info.nCloakFadeState;
	if (nCloakFadeValue >= FADE_LEVELS - 1) {
		nCloakFadeValue = FADE_LEVELS - 1;
		if (nCloakState && (m_info.nCloakFadeState == 2))
			m_info.nCloakFadeState = -2;
		else
			m_info.nCloakFadeState = 0;
		}
	else if (nCloakFadeValue <= 0) {
		nCloakFadeValue = 0;
		if ((nCloakState == 2) && (m_info.nCloakFadeState == -2))
			m_info.nCloakFadeState = 2;
		else
			m_info.nCloakFadeState = 0;
		}
	}

m_info.nColor = RGBA (255, 255, 255, int (float (nCloakFadeValue) / float (FADE_LEVELS) * 255));
BitBlt (GAUGE_SHIPS + (IsTeamGame ? GetTeam (N_LOCALPLAYER) : N_LOCALPLAYER % MAX_PLAYER_COLORS), x, y);
m_info.nColor = WHITE_RGBA;
gameStates.render.grAlpha = 1.0f;
}

//	-----------------------------------------------------------------------------

void CGenericCockpit::DrawWeaponInfo (int nWeaponType, int nIndex, tGaugeBox* box, int xPic, int yPic, const char *pszName, int xText, int yText, int orient)
{
	CBitmap*	bmP;
	char		szName [100], *p;
	int		i, color;

	static int nIdWeapon [2][3] = {{0, 0, 0},{0, 0, 0}}, nIdLaser [2] = {0, 0};

i = ((gameData.pig.tex.nHamFileVersion >= 3) && gameStates.video.nDisplayMode)
	 ? gameData.weapons.info [nIndex].hiresPicture.index
	 : gameData.weapons.info [nIndex].picture.index;
LoadTexture (i, 0, 0);
if (!(bmP = gameData.pig.tex.bitmaps [0] + i))
	return;
color = (m_info.weaponBoxStates [nWeaponType] == WS_SET) ? 255 : int (X2F (m_info.weaponBoxFadeValues [nWeaponType]) / float (FADE_LEVELS) * 255);
m_info.nColor = RGBA (color, color, color, 255);
BitBlt (-1, xPic, yPic, true, true, (gameStates.render.cockpit.nType == CM_FULL_SCREEN) ? I2X (2) : I2X (1), orient, bmP);
m_info.nColor = WHITE_RGBA;
CCanvas::Current ()->SetColorRGB (255, 255, 255, 255);
fontManager.SetColorRGBi (GREEN_RGBA, 1, 0, 0);
if ((p = const_cast<char*> (strchr (pszName, '\n')))) {
	memcpy (szName, pszName, i = int (p - pszName));
	szName [i] = '\0';
	nIdWeapon [nWeaponType][0] = PrintF (&nIdWeapon [nWeaponType][0], xText, yText, szName);
	nIdWeapon [nWeaponType][1] = PrintF (&nIdWeapon [nWeaponType][1], xText, yText + m_info.fontHeight + int (fontManager.Scale ()), p + 1);
	}
else {
	nIdWeapon [nWeaponType][2] = PrintF (&nIdWeapon [nWeaponType][2], xText, yText, pszName);
	}

//	For laser, show level and quadness
if (nIndex == LASER_ID || nIndex == SUPERLASER_ID) {
	sprintf (szName, "%s: 0", TXT_LVL);
	szName [5] = LOCALPLAYER.LaserLevel () + 1 + '0';
	nIdLaser [0] = PrintF (&nIdLaser [0], xText, yText + m_info.nLineSpacing, szName);
	if (LOCALPLAYER.flags & PLAYER_FLAGS_QUAD_LASERS) {
		strcpy (szName, TXT_QUAD);
		nIdLaser [nWeaponType] = PrintF (&nIdLaser [nWeaponType], xText, yText + 2 * m_info.nLineSpacing, szName);
		}
	}
}

//	-----------------------------------------------------------------------------

void CGenericCockpit::DrawWeaponInfo (int nWeaponType, int nWeaponId, int laserLevel)
{
	int nIndex;

if (nWeaponType == 0) {
	nIndex = primaryWeaponToWeaponInfo [nWeaponId];

	if (nIndex == LASER_ID && laserLevel > MAX_LASER_LEVEL)
		nIndex = SUPERLASER_ID;

	if (gameStates.render.cockpit.nType == CM_STATUS_BAR)
		DrawWeaponInfo (nWeaponType, nIndex,
			hudWindowAreas + SB_PRIMARY_BOX,
			SB_PRIMARY_W_PIC_X, SB_PRIMARY_W_PIC_Y,
			PRIMARY_WEAPON_NAMES_SHORT (nWeaponId),
			SB_PRIMARY_W_TEXT_X, SB_PRIMARY_W_TEXT_Y, 0);
	else
		DrawWeaponInfo (nWeaponType, nIndex,
			hudWindowAreas + COCKPIT_PRIMARY_BOX,
			PRIMARY_W_PIC_X, PRIMARY_W_PIC_Y,
			PRIMARY_WEAPON_NAMES_SHORT (nWeaponId),
			PRIMARY_W_TEXT_X, PRIMARY_W_TEXT_Y, 0);
		}
else {
	nIndex = secondaryWeaponToWeaponInfo [nWeaponId];

	if (gameStates.render.cockpit.nType == CM_STATUS_BAR)
		DrawWeaponInfo (nWeaponType, nIndex,
			hudWindowAreas + SB_SECONDARY_BOX,
			SB_SECONDARY_W_PIC_X, SB_SECONDARY_W_PIC_Y,
			SECONDARY_WEAPON_NAMES_SHORT (nWeaponId),
			SB_SECONDARY_W_TEXT_X, SB_SECONDARY_W_TEXT_Y, 0);
	else
		DrawWeaponInfo (nWeaponType, nIndex,
			hudWindowAreas + COCKPIT_SECONDARY_BOX,
			SECONDARY_W_PIC_X, SECONDARY_W_PIC_Y,
			SECONDARY_WEAPON_NAMES_SHORT (nWeaponId),
			SECONDARY_W_TEXT_X, SECONDARY_W_TEXT_Y, 0);
	}
}

//	-----------------------------------------------------------------------------

//returns true if drew picture
int CGenericCockpit::DrawWeaponDisplay (int nWeaponType, int nWeaponId)
{
int bLaserLevelChanged = ((nWeaponType == 0) &&
								  (nWeaponId == LASER_INDEX) &&
								  ((LOCALPLAYER.LaserLevel () != m_history [0].laserLevel)));

if ((m_info.weaponBoxStates [nWeaponType] == WS_SET) &&
	 ((nWeaponId != m_history [0].weapon [nWeaponType]) || bLaserLevelChanged)) {
	m_info.weaponBoxStates [nWeaponType] = WS_FADING_OUT;
	m_info.weaponBoxFadeValues [nWeaponType] = I2X (FADE_LEVELS - 1);
	}

if (m_info.weaponBoxStates [nWeaponType] == WS_FADING_OUT) {
	if (m_history [0].weapon [nWeaponType] < 0)
		m_info.weaponBoxFadeValues [nWeaponType] = 0;
	else {
		DrawWeaponInfo (nWeaponType,
							 m_history [0].weapon [nWeaponType],
							 m_history [0].laserLevel);
		m_history [0].ammo [nWeaponType] = -1;
		m_history [0].xOmegaCharge = -1;
		m_info.weaponBoxFadeValues [nWeaponType] -= gameData.time.xFrame * FADE_SCALE;
		}
	if (m_info.weaponBoxFadeValues [nWeaponType] <= 0) {
		m_info.weaponBoxStates [nWeaponType] = WS_FADING_IN;
		m_history [0].weapon [nWeaponType] =
		m_history [1].weapon [nWeaponType] = nWeaponId;
		m_history [0].laserLevel =
		m_history [1].laserLevel = LOCALPLAYER.LaserLevel ();
		m_info.weaponBoxFadeValues [nWeaponType] = 0;
		}
	}
else if (m_info.weaponBoxStates [nWeaponType] == WS_FADING_IN) {
	DrawWeaponInfo (nWeaponType, nWeaponId, LOCALPLAYER.LaserLevel ());
	if ((m_history [0].weapon [nWeaponType] >= 0) &&
		 (nWeaponId != m_history [0].weapon [nWeaponType])) {
		m_info.weaponBoxStates [nWeaponType] = WS_FADING_OUT;
		}
	else {
		m_history [0].weapon [nWeaponType] = nWeaponId;
		m_history [0].ammo [nWeaponType] = -1;
		m_history [0].xOmegaCharge = -1;
		m_info.weaponBoxFadeValues [nWeaponType] += gameData.time.xFrame * FADE_SCALE;
		if (m_info.weaponBoxFadeValues [nWeaponType] >= I2X (FADE_LEVELS-1)) {
			m_info.weaponBoxStates [nWeaponType] = WS_SET;
			m_history [!0].weapon [nWeaponType] = -1;		//force redraw (at full fade-in) of other page
			}
		}
	}
else {
	DrawWeaponInfo (nWeaponType, nWeaponId, LOCALPLAYER.LaserLevel ());
	m_history [0].weapon [nWeaponType] = nWeaponId;
	m_history [0].ammo [nWeaponType] = -1;
	m_history [0].xOmegaCharge = -1;
	m_history [0].laserLevel = LOCALPLAYER.LaserLevel ();
	m_info.weaponBoxStates [nWeaponType] = WS_SET;
	}

return 1;
}

//	-----------------------------------------------------------------------------

fix staticTime [2];

void CGenericCockpit::DrawStatic (int nWindow, int nIndex)
{
	tVideoClip *vc = gameData.effects.vClips [0] + VCLIP_MONITOR_STATIC;
	CBitmap *bmp;
	int framenum;
	int boxofs = (gameStates.render.cockpit.nType == CM_STATUS_BAR) ? SB_PRIMARY_BOX : COCKPIT_PRIMARY_BOX;
	int h, x, y;

staticTime [nWindow] += gameData.time.xFrame;
if (staticTime [nWindow] >= vc->xTotalTime) {
	m_info.weaponBoxUser [nWindow] = WBU_WEAPON;
	return;
	}
framenum = staticTime [nWindow] * vc->nFrameCount / vc->xTotalTime;
LoadTexture (vc->frames [framenum].index, 0, 0);
bmp = gameData.pig.tex.bitmaps [0] + vc->frames [framenum].index;
h = boxofs + nWindow;
for (x = hudWindowAreas [h].left; x < hudWindowAreas [h].right; x += bmp->Width ())
	for (y = hudWindowAreas [h].top; y < hudWindowAreas [h].bot; y += bmp->Height ())
		BitBlt (-1, x, y, true, true, I2X (1), 0, bmp);
}

//	-----------------------------------------------------------------------------

void CGenericCockpit::DrawWeapons (void)
{
if ((gameStates.render.cockpit.nType == CM_FULL_COCKPIT) || (gameStates.render.cockpit.nType == CM_STATUS_BAR))
	fontManager.SetScale ((float) floor (float (CCanvas::Current ()->Width ()) / 640.0f));

if (m_info.weaponBoxUser [0] == WBU_WEAPON) {
	if (DrawWeaponDisplay (0, gameData.weapons.nPrimary) && (m_info.weaponBoxStates [0] == WS_SET)) {
		if ((gameData.weapons.nPrimary == VULCAN_INDEX) || (gameData.weapons.nPrimary == GAUSS_INDEX)) {
			if (LOCALPLAYER.primaryAmmo [VULCAN_INDEX] != m_history [0].ammo [0]) {
				if (gameData.demo.nState == ND_STATE_RECORDING)
					NDRecordPrimaryAmmo (m_history [0].ammo [0], LOCALPLAYER.primaryAmmo [VULCAN_INDEX]);
				DrawPrimaryAmmoInfo (X2I ((unsigned) VULCAN_AMMO_SCALE * (unsigned) LOCALPLAYER.primaryAmmo [VULCAN_INDEX]));
				m_history [0].ammo [0] = LOCALPLAYER.primaryAmmo [VULCAN_INDEX];
				}
			}
		else if (gameData.weapons.nPrimary == OMEGA_INDEX) {
			if (gameData.omega.xCharge [IsMultiGame] != m_history [0].xOmegaCharge) {
				if (gameData.demo.nState == ND_STATE_RECORDING)
					NDRecordPrimaryAmmo (m_history [0].xOmegaCharge, gameData.omega.xCharge [IsMultiGame]);
				DrawPrimaryAmmoInfo (gameData.omega.xCharge [IsMultiGame] * 100 / MAX_OMEGA_CHARGE);
				m_history [0].xOmegaCharge = gameData.omega.xCharge [IsMultiGame];
				}
			}
		}
	}
else if (m_info.weaponBoxUser [0] == WBU_STATIC)
	DrawStatic (0);

if (m_info.weaponBoxUser [1] == WBU_WEAPON) {
	if (DrawWeaponDisplay (1, gameData.weapons.nSecondary) && (m_info.weaponBoxStates [1] == WS_SET) &&
		 (LOCALPLAYER.secondaryAmmo [gameData.weapons.nSecondary] != m_history [0].ammo [1])) {
		m_history [0].bombCount = 0x7fff;	//force redraw
		if (gameData.demo.nState == ND_STATE_RECORDING)
			NDRecordSecondaryAmmo (m_history [0].ammo [1], LOCALPLAYER.secondaryAmmo [gameData.weapons.nSecondary]);
		DrawSecondaryAmmoInfo (LOCALPLAYER.secondaryAmmo [gameData.weapons.nSecondary]);
		m_history [0].ammo [1] = LOCALPLAYER.secondaryAmmo [gameData.weapons.nSecondary];
		}
	}
else if (m_info.weaponBoxUser [1] == WBU_STATIC)
	DrawStatic (1);

fontManager.SetScale (1.0f);
}

//	-----------------------------------------------------------------------------

//show names of teammates & players carrying flags
void CGenericCockpit::DrawPlayerNames (void)
{
	int			bHasFlag, bShowName, bShowTeamNames, bShowAllNames, nObject, nTeam;
	int			nPlayer, nState;
	CRGBColor*	colorP;

	static int nCurColor = 1, tColorChange = 0;
	static CRGBColor typingColors [2] = {
		{63, 0, 0},
		{63, 63, 0}
	};
	char s [CALLSIGN_LEN + 10];
	int w, h, aw;
	int x0, y0, x1, y1;
	int nColor;
	static int nIdNames [2][MAX_PLAYERS] = {{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}};

bShowAllNames = ((gameData.demo.nState == ND_STATE_PLAYBACK) || (netGame.m_info.bShowAllNames && gameData.multigame.bShowReticleName));
bShowTeamNames = (gameData.multigame.bShowReticleName && (IsCoopGame || IsTeamGame));

nTeam = GetTeam (N_LOCALPLAYER);
for (nPlayer = 0; nPlayer < gameData.multiplayer.nPlayers; nPlayer++) {	//check all players
	if (gameStates.multi.bPlayerIsTyping [nPlayer])
		nState = 1;
	else if (downloadManager.Downloading (nPlayer))
		nState = 2;
	else
		nState = 0;

	bShowName = (nState ||
					 (bShowAllNames && !(gameData.multiplayer.players [nPlayer].flags & PLAYER_FLAGS_CLOAKED)) ||
					 (!IsCoopGame && (bShowTeamNames && GetTeam (nPlayer) == nTeam)));
	bHasFlag = (gameData.multiplayer.players [nPlayer].Connected () && (gameData.multiplayer.players [nPlayer].flags & PLAYER_FLAGS_FLAG));

	if (gameData.demo.nState != ND_STATE_PLAYBACK)
		nObject = gameData.multiplayer.players [nPlayer].nObject;
	else {
		//if this is a demo, the nObject in the player struct is wrong,
		//so we search the CObject list for the nObject
		CObject *objP;
		nObject = -1;
		FORALL_PLAYER_OBJS (objP, nObject)
			if (objP->info.nId == nPlayer) {
				nObject = objP->Index ();
				break;
				}
		if (IS_OBJECT (objP, nObject))		//not in list, thus not visible
			bShowName = !bHasFlag;				//..so don't show name
		}

	if ((bShowName || bHasFlag) && CanSeeObject (nObject, 1)) {
		CRenderPoint		vPlayerPos;

#if 1
		vPlayerPos.TransformAndEncode (OBJECTS [nObject].info.position.vPos);
#else
		//transformation.Push ();
		SetRenderView (0, NULL, 0);
		vPlayerPos.TransformAndEncode (OBJECTS [nObject].info.position.vPos);
		ogl.EndFrame ();
		//transformation.Pop ();
#endif
		if (vPlayerPos.Visible ()) {	//on screen
			vPlayerPos.Project ();
			if (!vPlayerPos.Overflow ()) {
				fix x = vPlayerPos.X ();
				fix y = gameData.render.scene.Height () - vPlayerPos.Y ();
				if (bShowName) {				// Draw callsign on HUD
					if (nState) {
						int t = gameStates.app.nSDLTicks [0];

						if (t - tColorChange > 333) {
							tColorChange = t;
							nCurColor = !nCurColor;
							}
						colorP = typingColors + nCurColor;
						}
					else {
						nColor = IsTeamGame ? GetTeam (nPlayer) : nPlayer % MAX_PLAYER_COLORS;
						colorP = playerColors + nColor;
						}

					sprintf (s, "%s", (nState == 1) ? TXT_TYPING : (nState == 2) ? "Downloading..." : gameData.multiplayer.players [nPlayer].callsign);
					fontManager.Current ()->StringSize (s, w, h, aw);
					SetFontColor (RGBA_PAL2 (colorP->Red (), colorP->Green (), colorP->Blue ()));
					x1 = x - w / 2;
					y1 = y - h / 2;
					fontManager.SetColorRGBi (FontColor (), 1, 0, 0);
					nIdNames [nCurColor][nPlayer] = GrString (x1, y1, s, nIdNames [nCurColor] + nPlayer);
					CCanvas::Current ()->SetColorRGBi (RGBA_PAL2 (colorP->Red (), colorP->Green (), colorP->Blue ()));
					glLineWidth ((GLfloat) 2.0f);
					OglDrawEmptyRect (x1 - 4, y1 - 3, x1 + w + 2, y1 + h + 3);
					glLineWidth (1);
					}

				if (bHasFlag && (gameStates.app.bNostalgia || !(EGI_FLAG (bTargetIndicators, 0, 1, 0) || EGI_FLAG (bTowFlags, 0, 1, 0)))) {// Draw box on HUD
					fix dy = -FixMulDiv (OBJECTS [nObject].info.xSize, I2X (CCanvas::Current ()->Height ())/2, vPlayerPos.ViewPos ().v.coord.z);
//					fix dy = -FixMulDiv (FixMul (OBJECTS [nObject].size, transformation.m_info.scale.y), I2X (CCanvas::Current ()->Height ())/2, vPlayerPos.m_z);
					fix dx = FixMul (dy, gameData.render.screen.Aspect ());
					fix w = dx / 4;
					fix h = dy / 4;
					if (gameData.app.GameMode (GM_CAPTURE | GM_ENTROPY))
						CCanvas::Current ()->SetColorRGBi ((GetTeam (nPlayer) == TEAM_BLUE) ? MEDRED_RGBA :  MEDBLUE_RGBA);
					else if (IsHoardGame) {
						if (IsTeamGame)
							CCanvas::Current ()->SetColorRGBi ((GetTeam (nPlayer) == TEAM_RED) ? MEDRED_RGBA :  MEDBLUE_RGBA);
						else
							CCanvas::Current ()->SetColorRGBi (MEDGREEN_RGBA);
						}
					x0 = x - dx;
					x1 = x + dx;
					y0 = y - dy;
					y1 = y + dy;
					// draw the edges of a rectangle around the player (not a complete rectangle)
#if 1
					// draw the complete rectangle
					OglDrawEmptyRect (x0, y0, x1, y1);
					// now partially erase its sides
					CCanvas::Current ()->SetColorRGBi (0);
					OglDrawLine (x - w, y0, x + w, y0);
					OglDrawLine (x - w, y1, x + w, y1);
					OglDrawLine (x0, y - h, x0, y + h);
					OglDrawLine (x1, y - h, x1, y + h);
#else
					OglDrawLine (x1 - w, y0, x1, y0);	//right
					OglDrawLine (x1, y0, x1, y0 + h);	//down
					OglDrawLine (x0, y0, x0 + w, y0);
					OglDrawLine (x0, y0, x0, y0 + h);
					OglDrawLine (x1 - w, y1, x1, y1);
					OglDrawLine (x1, y1, x1, y1 - h);
					OglDrawLine (x0, y1, x0 + w, y1);
					OglDrawLine (x0, y1, x0, y1 - h);
#endif
					}
				}
			}
		}
	}
}

//	-----------------------------------------------------------------------------

void CGenericCockpit::DrawKillList (int x, int y)
{
if (Hide ())
	return;
if (!(IsMultiGame && gameData.multigame.score.bShowList))
	return;

	int	nPlayers, playerList [MAX_PLAYERS];
	int	nLeft, nameLen, i, xo = 0, x0, x1, y0, fth;
	int	t = gameStates.app.nSDLTicks [0];
	int	bGetPing = gameStates.render.cockpit.bShowPingStats && (!networkData.tLastPingStat || (t - networkData.tLastPingStat >= 1000));
	int	faw = (int) FRound (float (GAME_FONT->TotalWidth ()) / float (GAME_FONT->Range ()) * 1.05f);
	float fScale;


	static int nIdKillList [2][MAX_PLAYERS] = {{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}};

if (bGetPing)
	networkData.tLastPingStat = t;
if (gameData.multigame.score.xShowListTimer > 0) {
	gameData.multigame.score.xShowListTimer -= gameData.time.xFrame;
	if (gameData.multigame.score.xShowListTimer < 0)
		gameData.multigame.score.bShowList = 0;
	}
nPlayers = (gameData.multigame.score.bShowList == 3) ? 2 : MultiGetKillList (playerList);
nLeft = /*(nPlayers <= MAXPLAYERS / 2) ? nPlayers :*/ (nPlayers + 1) / 2;
fScale = float (CCanvas::Current ()->Height ()) / 640.0f;
if (fScale < 1.0f)
	fScale = 1.0f;
//If font size changes, this code might not work right anymore
//Assert (GAME_FONT->Height ()==5 && GAME_FONT->Width ()==7);
fth = GAME_FONT->Height ();
y -= nLeft * (fth + 1);
x = CCanvas::Current ()->Width () - LHX (x);
x0 = LHX (1);
nameLen = (9 + (gameData.multigame.score.bShowList == 3)) * faw;
x1 = x0 + nameLen;
y0 = y;

if (gameStates.render.cockpit.bShowPingStats) {
	if (faw < 0) {
		if (gameData.multigame.score.bShowList == 2)
			xo = faw * 24;//was +25;
		else if (IsCoopGame)
			xo = faw * 14;//was +30;
		else
			xo = faw * 8; //was +20;
		}
	}
for (i = 0; i < nPlayers; i++) {
	int nPlayer;
	char name [80], teamInd [2] = {127, 0};
	int sh, aw, indent = 0;

	if (i >= nLeft) {
		x1 = CCanvas::Current ()->Width () - LHX (1) - 6 * faw; // (IsCoopGame ? LHX (27) : LHX (15));
		x0 = x1 - nameLen;
		if (gameStates.render.cockpit.bShowPingStats) {
			x0 -= xo + 6 * faw;
			x1 -= xo + 6 * faw;
			}
		if (i == nLeft)
			y0 = y;
#if 0
		if (netGame.GetScoreGoal () || netGame.GetPlayTimeAllowed ())
			x1 -= LHX (18);
#endif
		}
#if 0
	else if (netGame.GetScoreGoal () || netGame.GetPlayTimeAllowed ())
		 x1 = int (LHX (43) * fScale - LHX (18));
#endif
	nPlayer = (gameData.multigame.score.bShowList == 3) ? i : playerList [i];
	if (gameData.multiplayer.players [nPlayer].HasLeft ())
		continue;
	if ((gameData.multigame.score.bShowList == 1) || (gameData.multigame.score.bShowList == 2)) {
		if (!gameData.multiplayer.players [nPlayer].Connected ()) 
			SetFontColor (RGBA_PAL2 (12, 12, 12));
		else {
			int color = IsTeamGame ? GetTeam (nPlayer) : nPlayer % MAX_PLAYER_COLORS;
			SetFontColor (RGBA_PAL2 (playerColors [color].Red (), playerColors [color].Green (), playerColors [color].Blue ()));
			}
		}
	else
		SetFontColor (RGBA_PAL2 (playerColors [nPlayer].Red (), playerColors [nPlayer].Green (), playerColors [nPlayer].Blue ()));
	if (gameData.multigame.score.bShowList == 3) {
		if (N_LOCALPLAYER == i) {
#if 0//DBG
			sprintf (name, "%c%-8s %d.%d.%d.%d:%d",
						teamInd [0], netGame.m_info.szTeamName [i],
						netPlayers [0].m_info.players [i].network.Node () [0],
						netPlayers [0].m_info.players [i].network.Node () [1],
						netPlayers [0].m_info.players [i].network.Node () [2],
						netPlayers [0].m_info.players [i].network.Node () [3],
						netPlayers [0].m_info.players [i].network.Node () [5] +
						 (unsigned) netPlayers [0].m_info.players [i].network.Node () [4] * 256);
#else
			sprintf (name, "%c%s", teamInd [0], netGame.m_info.szTeamName [GetTeam (i)]);
#endif
			indent = 0;
			}
		else {
#if SHOW_PLAYER_IP
			sprintf (name, "%-8s %d.%d.%d.%d:%d",
						netGame.m_info.szTeamName [i],
						netPlayers [0].m_info.players [i].network.Node () [0],
						netPlayers [0].m_info.players [i].network.Node () [1],
						netPlayers [0].m_info.players [i].network.Node () [2],
						netPlayers [0].m_info.players [i].network.Node () [3],
						netPlayers [0].m_info.players [i].network.Node () [5] +
						 (unsigned) netPlayers [0].m_info.players [i].network.Node () [4] * 256);
#else
			strcpy (name, netGame.m_info.szTeamName [i]);
#endif
			fontManager.Current ()->StringSize (teamInd, indent, sh, aw);
			}
		}
	else
#if 0//DBG
		sprintf (name, "%-8s %d.%d.%d.%d:%d",
					gameData.multiplayer.players [nPlayer].callsign,
					netPlayers [0].m_info.players [nPlayer].network.Node () [0],
					netPlayers [0].m_info.players [nPlayer].network.Node () [1],
					netPlayers [0].m_info.players [nPlayer].network.Node () [2],
					netPlayers [0].m_info.players [nPlayer].network.Node () [3],
					netPlayers [0].m_info.players [nPlayer].network.Node () [5] +
					 (unsigned) netPlayers [0].m_info.players [nPlayer].network.Node () [4] * 256);
#else
		strcpy (name, gameData.multiplayer.players [nPlayer].callsign);	// Note link to above if!!
#endif
#if 0//DBG
	x1 += LHX (100);
	int l, sw, nWidth = x1 - x0 - LHX (2);
	for (l = (int) strlen (name); l;) {
		fontManager.Current ()->StringSize (name, sw, sh, aw);
		if (sw <= nWidth)
			break;
		name [--l] = '\0';
		}
#endif
	nIdKillList [0][i] = DrawHUDText (nIdKillList [0] + i, x0 + indent, y0, "%s", name);

	if (gameData.multigame.score.bShowList == 2) {
		if (gameData.multiplayer.players [nPlayer].netKilledTotal + gameData.multiplayer.players [nPlayer].netKillsTotal <= 0)
			nIdKillList [1][i] = DrawHUDText (nIdKillList [1] + i, x1, y0, TXT_NOT_AVAIL);
		else
			nIdKillList [1][i] = DrawHUDText (nIdKillList [1] + i, x1, y0, "%d%%",
														 (int) ((double) gameData.multiplayer.players [nPlayer].netKillsTotal /
																 ((double) gameData.multiplayer.players [nPlayer].netKilledTotal +
																  (double) gameData.multiplayer.players [nPlayer].netKillsTotal) * 100.0));
		}
	else if (gameData.multigame.score.bShowList == 3) {
		if (IsEntropyGame)
			nIdKillList [1][i] = DrawHUDText (nIdKillList [1] + i, x1, y0, "%3d [%d/%d]",
														 gameData.multigame.score.nTeam [i], gameData.entropy.nTeamRooms [i + 1],
														 gameData.entropy.nTotalRooms);
		else
			nIdKillList [1][i] = DrawHUDText (nIdKillList [1] + i, x1, y0, "%3d", gameData.multigame.score.nTeam [i]);
		}
	else if (IsCoopGame)
		nIdKillList [1][i] = DrawHUDText (nIdKillList [1] + i, x1, y0, "%-6d", gameData.multiplayer.players [nPlayer].score);
   else if (netGame.GetPlayTimeAllowed () || netGame.GetScoreGoal ())
      nIdKillList [1][i] = DrawHUDText (nIdKillList [1] + i, x1, y0, "%3d (%d)",
													 gameData.multiplayer.players [nPlayer].netKillsTotal,
													 gameData.multiplayer.players [nPlayer].nScoreGoalCount);
   else
		nIdKillList [1][i] = DrawHUDText (nIdKillList [1] + i, x1, y0, "%3d", gameData.multiplayer.players [nPlayer].netKillsTotal);
	if (gameStates.render.cockpit.bShowPingStats && (nPlayer != N_LOCALPLAYER)) {
		if (bGetPing)
			PingPlayer (nPlayer);
		if (pingStats [nPlayer].sent) {
#if 0//DBG
			nIdKillList [1][i] = DrawHUDText (nIdKillList [1] + i, x1 + xo, y0, "%lu %d %d",
						  pingStats [nPlayer].ping,
						  pingStats [nPlayer].sent,
						  pingStats [nPlayer].received);
#else
			nIdKillList [1][i] = DrawHUDText (nIdKillList [1] + i, x1 + xo, y0, "%lu %i%%", pingStats [nPlayer].ping,
														 100 - ((pingStats [nPlayer].received * 100) / pingStats [nPlayer].sent));
#endif
			}
		}
	y0 += fth + 1;
	}
}

//	-----------------------------------------------------------------------------

void CGenericCockpit::DrawModuleDamage (void)
{
if (gameStates.app.bNostalgia || !EGI_FLAG (nDamageModel, 0, 0, 0))
	return;
if (cockpit->Hide ())
	return;
if (gameStates.app.bPlayerIsDead)
	return;
#if 0
	static int		nIdDamage [3] = {0, 0, 0};
	static int		nColor [3] = {GOLD_RGBA, ORANGE_RGBA, RED_RGBA};
	static char*	szDamage [3] = {"AIM: %d%c", "DRIVES: %d%c", "GUNS: %d%c"};
	static char*	szId = "ADG";
#endif
	int				i;

	static uint		dmgColors [3][4] = {
							{RGBA (64, 16, 64, 96), RGBA (192, 0, 0, 96), RGBA (255, 208, 0, 96), RGBA (0, 192, 0, 96)},
							{RGBA (96, 32, 96, 96), RGBA (255, 0, 0, 96), RGBA (255, 255, 0, 96), RGBA (0, 255, 0, 96)},
							{RGBA (96, 32, 96, 96), RGBA (255, 0, 0, 96), RGBA (255, 255, 0, 96), RGBA (0, 255, 0, 96)}
							};

#if 1 //!DBG
if ((gameStates.app.nSDLTicks [0] - OBJECTS [LOCALPLAYER.nObject].TimeLastRepaired () > 2000) && !OBJECTS [LOCALPLAYER.nObject].CriticalDamage ())
	return;
#endif
	float fScale = float (CCanvas::Current ()->Width ()) / 640.0f;
	int	nRad = (int) FRound (16.0f * fScale);
	int	y = CCanvas::Current ()->Height () / 2 + 3 * nRad / 2;
	int	x = CCanvas::Current ()->Width () / 2;
	int	nOffsetSave = gameData.SetStereoOffsetType (STEREO_OFFSET_FIXED);

if (ShowTextGauges ()) {
	int				nColor, nDamage [3], h [4], w [4], aw [4], tw = 0;
	char				szDamage [3][10];
	CCanvasColor	dmgColor;

	dmgColor.Set (0, 0, 0, 128);
	dmgColor.index = -1;
	dmgColor.rgb = 1;

	fontManager.SetScale (Max (1.0f, (float) floor (FRound (fScale))));
	for (i = 0; i < 3; i++) {
		nDamage [i] = (int) FRound (X2F (m_info.nDamage [i]) * 200.0f);
#if 1
		sprintf (szDamage [i], "%d ", nDamage [i]);
#else
		sprintf (szDamage [i], "%c:%d ", szId [i], nDamage [i]);
#endif
		fontManager.Current ()->StringSize (szDamage [i], w [i], h [i], aw [i]);
		tw += w [i];
		}
	fontManager.Current ()->StringSize (" ", w [3], h [3], aw [3]);
	x -= (tw - w [3]) / 2;
	CCanvas::Current ()->SetFontColor (dmgColor, 1);	// black background
	for (i = 0; i < 3; i++) {
		nColor = dmgColors [1][nDamage [i] / 33];
		//dmgColor.Set (RGBA_RED (nColor), RGBA_GREEN (nColor), RGBA_BLUE (nColor));
		SetFontColor (nColor);
		if (nDamage [i] < 100)
			szDamage [i][2] = '\0'; // remove the trailing blank, it causes rendering artifacts
		DrawHUDText (NULL, x, y, szDamage [i]);
		x += w [i];
		}
	fontManager.SetScale (1.0f);
	}
else {
		static tSinCosf sinCos [32];
		static int bInitSinCos = 1;

	if (bInitSinCos) {
		ComputeSinCosTable (sizeofa (sinCos), sinCos);
		bInitSinCos = 0;
		}

	x -= nRad;
	glLineWidth (fScale);

#if 0
	SetFontColor (nColor [2], 1, 0, 0);
#	if 0 // rectangular frame
	CCanvas::Current ()->SetColorRGB (64, 32, 0, 64);
	OglDrawFilledRect (x - 7, y - 7, x + 71, y + 71);
	CCanvas::Current ()->SetColorRGB (255, 128, 0, 255);
	OglDrawEmptyRect (x - 7, y - 7, x + 71, y + 71);
#	else // round frame
	glColor4f (1.0f, 0.5f, 0.0f, 1.0f);
	OglDrawEllipse (
		sizeofa (sinCos), GL_LINE_LOOP,
		40.0f / float (gameData.render.screen.Width ()), 0.5f,
		40.0f / float (gameData.render.screen.Height ()), 1.0f - float (CCanvas::Current ()->Top () + y + 32) / float (gameData.render.screen.Height ()), sinCos);
#	endif
	glLineWidth (1);
#endif

	nRad *= 2;
	for (i = 0; i < 3; i++) {
		if (damageIcon [i].Load ()) {
			CCanvas::Current ()->SetColorRGBi (dmgColors [i][(int) FRound (X2F (m_info.nDamage [i]) * 200.0f) / 33]);
			damageIcon [i].Bitmap ()->RenderScaled (gameData.X (x), y, nRad, nRad, I2X (1), 0, &CCanvas::Current ()->Color ());
			}
		}
	}
gameData.SetStereoOffsetType (nOffsetSave);
}

//	-----------------------------------------------------------------------------

void CGenericCockpit::DrawCockpit (int nCockpit, int y, bool bAlphaTest)
{
#if 1
if (gameOpts->render.cockpit.bHUD || (gameStates.render.cockpit.nType != CM_FULL_SCREEN)) {
	int i = gameData.pig.tex.cockpitBmIndex [nCockpit].index;
	CBitmap *bmP = gameData.pig.tex.bitmaps [0] + i;
	LoadTexture (i, 0, 0, true);
	if (bmP->HasOverride ())
		bmP = bmP->Override (-1);
	ogl.m_states.nTransparencyLimit = 8;	//add transparency to black areas of palettized cockpits (namely the display windows)
	bmP->SetTranspType (3);
	bmP->SetupTexture (0, 1);
	ogl.m_states.nTransparencyLimit = 0;
	CCanvas::Current ()->SetColorRGBi (WHITE_RGBA);
	bmP->RenderScaled (0, y, -1, CCanvas::Current ()->Height () - y, I2X (1), 0, &CCanvas::Current ()->Color ());
	CCanvas::Current ()->SetColorRGBi (BLACK_RGBA);
	}
#endif
}

//------------------------------------------------------------------------------

#if DBG

fix showViewTextTimer = -1;

void DrawWindowLabel (void)
{
if (showViewTextTimer > 0) {
	const char* viewer_name, * control_name, * viewer_id;
	showViewTextTimer -= gameData.time.xFrame;

	viewer_id = const_cast<char*>("");
	switch (gameData.objs.viewerP->info.nType) {
		case OBJ_FIREBALL:
			viewer_name = const_cast<char*>("Fireball");
			break;
		case OBJ_ROBOT:
			viewer_name = const_cast<char*>("Robot");
			break;
		case OBJ_HOSTAGE:
			viewer_name = const_cast<char*>("Hostage");
			break;
		case OBJ_PLAYER:
			viewer_name = const_cast<char*>("Player");
			break;
		case OBJ_WEAPON:
			viewer_name = const_cast<char*>("Weapon");
			break;
		case OBJ_CAMERA:
			viewer_name = const_cast<char*>("Camera");
			break;
		case OBJ_POWERUP:
			viewer_name = const_cast<char*>("Powerup");
			break;
		case OBJ_DEBRIS:
			viewer_name = const_cast<char*>("Debris");
			break;
		case OBJ_REACTOR:
			viewer_name = const_cast<char*>("Reactor");
			break;
		default:
			viewer_name = const_cast<char*>("Unknown");
			break;
		}

	switch (gameData.objs.viewerP->info.controlType) {
		case CT_NONE:
			control_name = const_cast<char*>("Stopped");
			break;
		case CT_AI:
			control_name = const_cast<char*>("AI");
			break;
		case CT_FLYING:
			control_name = const_cast<char*>("Flying");
			break;
		case CT_SLEW:
			control_name = const_cast<char*>("Slew");
			break;
		case CT_FLYTHROUGH:
			control_name = const_cast<char*>("Flythrough");
			break;
		case CT_MORPH:
			control_name = const_cast<char*>("Morphing");
			break;
		default:
			control_name = const_cast<char*>("Unknown");
			break;
		}
	fontManager.SetColorRGBi (RED_RGBA, 1, 0, 0);
	GrPrintF (NULL, 0x8000, 45, "%i: %s [%s] View - %s", OBJ_IDX (gameData.objs.viewerP), viewer_name, viewer_id, control_name);
	}
}
#endif

//	-----------------------------------------------------------------------------
