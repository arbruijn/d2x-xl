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
#include "render.h"
#include "transprender.h"
#include "gamepal.h"
#include "ogl_lib.h"
#include "ogl_render.h"
#include "ogl_bitmap.h"
#include "ogl_hudstuff.h"
#include "playsave.h"
#include "gauges.h"
#include "hud_defs.h"
#include "statusbar.h"
#include "slowmotion.h"
#include "automap.h"
#include "gr.h"
#include "key.h"

#define SHOW_PLAYER_IP		0

void DrawGuidedCrosshair (void);

double cmScaleX, cmScaleY;
int nLineSpacing;

#if 0
CCanvas *Canv_LeftEnergyGauge;
CCanvas *Canv_AfterburnerGauge;
CCanvas *Canv_RightEnergyGauge;
CCanvas *numericalGaugeCanv;
#endif

#define COCKPIT_PRIMARY_BOX		 (!gameStates.video.nDisplayMode ? 0 : 4)
#define COCKPIT_SECONDARY_BOX		 (!gameStates.video.nDisplayMode ? 1 : 5)
#define SB_PRIMARY_BOX				 (!gameStates.video.nDisplayMode ? 2 : 6)
#define SB_SECONDARY_BOX			 (!gameStates.video.nDisplayMode ? 3 : 7)

CCockpit cockpit;

//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------

void CCockpitInfo::Init (void)
{
for (int = 0; i < 2; i++) {
	old [i].score = (IsMultiGame && !IsCoopGame) ? -99 : -1;
	old [i].energy = -1;
	old [i].shields = -1;
	old [i].flags = -1;
	old [i].oldCloak = -1;
	old [i].lives = -1;
	old [i].afterburner = -1;
	old [i].bombCount = 0;
	old [i].laserLevel	old [i].= 0;
	old [i].weapon [0] = 
	old [i].weapon [1] = -1;
	old [i].ammo [0] = ammo [1] = -1;
	old [i].omegaCharge  = -1;
	}

nCloakFadeState = 0;
bLastHomingWarningShown [0] =
bLastHomingWarningShown [1] = -1;
scoreDisplay [0] =
scoreDisplay [1] = 0;
scoreTime = 0;
lastWarningBeepTime [0] = 
lastWarningBeepTime [1] = 0;
bHaveGaugeCanvases = 0;
nInvulnerableFrame = 0;
nCloakFadeState = 0;		
weaponBoxStates [0] = 
weaponBoxStates [1] = 0;
weaponBoxFadeValues [0] = 
weaponBoxFadeValues [1] = 0;
weaponBoxUser [0] = 
weaponBoxUser [1] = WBU_WEAPON;
nLineSpacing = GAME_FONT->Height () + GAME_FONT->Height () / 4;
}

//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------

void CGenericCockpit::Init (void)
{
m_info.Init ();
}

//	-----------------------------------------------------------------------------

CBitmap* CGenericCockpit::BitBlt (int nGauge, int x, int y, bool bScalePos, bool bScaleSize, int scale, int orient, CBitmap* bmP)
{
if (nGauge >= 0) {
#if DBG
	if (nGauge == nDbgGauge)
		nDbgGauge = nDbgGauge;
#endif
	PAGE_IN_GAUGE (nGauge);
	bmP = gameData.pig.tex.bitmaps [0] + GET_GAUGE_INDEX (nGauge);
	}
if (bmP) {
	if (bScalePos) {
		x = HUD_SCALE_X (x);
		y = HUD_SCALE_Y (y);
		}
	int w = bmP->Width ();
	int h = bmP->Height ();
	if (bScaleSize) {
		w = HUD_SCALE_X (w);
		h = HUD_SCALE_Y (h);
		}
	bmP->RenderScaled (x, y, w * (gameStates.app.bDemoData + 1), h * (gameStates.app.bDemoData + 1), scale, orient, NULL);
	}
return bmP;
}

//	-----------------------------------------------------------------------------

int _CDECL_ CGenericCockpit::Print (int *idP, int x, int y, const char *pszFmt, ...)
{
	static char szBuf [1000];
	va_list args;

va_start (args, pszFmt);
vsprintf (szBuf, pszFmt, args);
return GrString (HUD_SCALE_X (x), HUD_SCALE_Y (y), szBuf, idP);
}

//------------------------------------------------------------------------------

char* CGenericCockpit::ftoa (char *pszVal, fix f)
{
	int decimal, fractional;

decimal = X2I (f);
fractional = ((f & 0xffff) * 100) / 65536;
if (fractional < 0)
	fractional = -fractional;
if (fractional > 99)
	fractional = 99;
sprintf (pszVal, "%d.%02d", decimal, fractional);
return pszVal;
}

//	-----------------------------------------------------------------------------

void CGenericCockpit::DrawSlowMotion (void)
{
if (HIDE_HUD)
	return;

	char	szScore [40];
	int	w, h, aw;

if ((gameData.hud.msgs [0].nMessages > 0) &&
	 (strlen (gameData.hud.msgs [0].szMsgs [gameData.hud.msgs [0].nFirst]) > 38))
	return;
GrSetCurFont (GAME_FONT);
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
GrGetStringSize (szScore, &w, &h, &aw);
GrSetFontColorRGBi (GREEN_RGBA, 1, 0, 0);
GrPrintF (NULL, grdCurCanv->cv_w - 2 * w - HUD_LHX (2), 3, szScore);
}

//	-----------------------------------------------------------------------------

void CGenericCockpit::DrawTime (void)
{
if (gameStates.render.bShowTime) {
		int secs = X2I (LOCALPLAYER.timeLevel) % 60;
		int mins = X2I (LOCALPLAYER.timeLevel) / 60;

		static int nIdTime = 0;

	fontManager.SetColorRGBi (GREEN_RGBA, 1, 0, 0);
	nIdTime = GrPrintF (&nIdTime, CCanvas::Current ()->Width () - 4 * GAME_FONT->Width (),
							  CCanvas::Current ()->Height () - 4 * nLineSpacing,
							  "%d:%02d", mins, secs);
	}
}

//	-----------------------------------------------------------------------------

void CGenericCockpit::DrawTimerCount (void)
{
if (HIDE_HUD)
	return;

	char	szScore [20];
	int	w, h, aw, i;
	fix	timevar = 0;

	static int nIdTimer = 0;

if ((gameData.hud.msgs [0].nMessages > 0) && (strlen (gameData.hud.msgs [0].szMsgs [gameData.hud.msgs [0].nFirst]) > 38))
	return;

if ((gameData.app.nGameMode & GM_NETWORK) && netGame.xPlayTimeAllowed) {
	timevar = I2X (netGame.xPlayTimeAllowed * 5 * 60);
	i = X2I (timevar - gameStates.app.xThisLevelTime) + 1;
	sprintf (szScore, "T - %5d", i);
	fontManager.Current ()->StringSize (szScore, w, h, aw);
	fontManager.SetColorRGBi (GREEN_RGBA, 1, 0, 0);
	if ((i >= 0) && !gameData.reactor.bDestroyed)
		nIdTimer = GrPrintF (&nIdTimer, CCanvas::Current ()->Width () - w - HUD_LHX (10), HUD_LHX (11), szScore);
	}
}

//	-----------------------------------------------------------------------------

void CGenericCockpit::DrawFlag (int x, int y)
{
if ((gameData.app.nGameMode & GM_CAPTURE) && (LOCALPLAYER.flags & PLAYER_FLAGS_FLAG)) {
icon = (GetTeam (gameData.multiplayer.nLocalPlayer) == TEAM_BLUE) ? FLAG_ICON_RED : FLAG_ICON_BLUE;
HUDBitBlt (icon, x, y, false, false);
}

//	-----------------------------------------------------------------------------

void CGenericCockpit::DrawOrbs (int x, int y)
{
	static int nIdOrbs = 0, nIdEntropy [2]= {0, 0};

if (HIDE_HUD)
	return;
if (gameData.app.nGameMode & (GM_HOARD | GM_ENTROPY)) {
	int x = 0, y = 0;
	CBitmap *bmP = NULL;

	if (nMode == CM_FULL_COCKPIT) {
		y = 2*nLineSpacing;
		x = 4*GAME_FONT->Width ();
		}
	else if (nMode == CM_STATUS_BAR) {
		y = nLineSpacing;
		x = GAME_FONT->Width ();
		}
	else if ((nMode == CM_FULL_SCREEN) ||
				(nMode == CM_LETTERBOX)) {
//			y = 5*nLineSpacing;
		y = nLineSpacing;
		x = GAME_FONT->Width ();
		if (gameStates.render.fonts.bHires)
			y += nLineSpacing;
		}
	else
		Int3 ();		//what sort of cockpit?

	bmP = HUDBitBlt (-1, x, y, false, false, I2X (1), 0, &gameData.hoard.icon [gameStates.render.fonts.bHires].bm);
	//GrUBitmapM (x, y, bmP);

	x += bmP->Width () + bmP->Width ()/2;
	y+= (gameStates.render.fonts.bHires?2:1);
	if (gameData.app.nGameMode & GM_ENTROPY) {
		char	szInfo [20];
		int	w, h, aw;
		if (LOCALPLAYER.secondaryAmmo [PROXMINE_INDEX] >= extraGameInfo [1].entropy.nCaptureVirusLimit)
			fontManager.SetColorRGBi (ORANGE_RGBA, 1, 0, 0);
		else
			fontManager.SetColorRGBi (GREEN_RGBA, 1, 0, 0);
		sprintf (szInfo,
			"x %d [%d]",
			LOCALPLAYER.secondaryAmmo [PROXMINE_INDEX],
			LOCALPLAYER.secondaryAmmo [SMARTMINE_INDEX]);
		nIdEntropy [0] = GrPrintF (nIdEntropy, x, y, szInfo);
		if (gameStates.entropy.bConquering) {
			int t = (extraGameInfo [1].entropy.nCaptureTimeLimit * 1000) -
					   (gameStates.app.nSDLTicks - gameStates.entropy.nTimeLastMoved);

			if (t < 0)
				t = 0;
			fontManager.Current ()->StringSize (szInfo, w, h, aw);
			x += w;
			fontManager.SetColorRGBi (RED_RGBA, 1, 0, 0);
			sprintf (szInfo, " %d.%d", t / 1000, (t % 1000) / 100);
			nIdEntropy [1] = GrPrintF (nIdEntropy + 1, x, y, szInfo);
			}
		}
	else
		nIdOrbs = GrPrintF (&nIdOrbs, x, y, "x %d", LOCALPLAYER.secondaryAmmo [PROXMINE_INDEX]);
	}
}

//	-----------------------------------------------------------------------------

void CGenericCockpit::PlayHomingWarning (void)
{
	fix	xBeepDelay;

if (gameStates.app.bEndLevelSequence || gameStates.app.bPlayerIsDead)
	return;
if (LOCALPLAYER.homingObjectDist >= 0) {
	xBeepDelay = LOCALPLAYER.homingObjectDist/128;
	if (xBeepDelay > I2X (1))
		xBeepDelay = I2X (1);
	else if (xBeepDelay < I2X (1)/8)
		xBeepDelay = I2X (1)/8;
	if (lastWarningBeepTime [gameStates.render.vr.nCurrentPage] > gameData.time.xGame)
		lastWarningBeepTime [gameStates.render.vr.nCurrentPage] = 0;
	if (gameData.time.xGame - lastWarningBeepTime [gameStates.render.vr.nCurrentPage] > xBeepDelay/2) {
		audio.PlaySound (SOUND_HOMING_WARNING);
		lastWarningBeepTime [gameStates.render.vr.nCurrentPage] = gameData.time.xGame;
		}
	}
}

//	-----------------------------------------------------------------------------

void CGenericCockpit::DrawBombCount (int x, int y, int bgColor, int bShowAlways)
{
	int bomb, count, countx;
	char szBombCount [5], *t;

	static int nIdBombCount = 0;

if (gameData.app.nGameMode & GM_ENTROPY)
	return;
bomb = ArmedBomb ();
if (!bomb)
	return;
if ((gameData.app.nGameMode & GM_HOARD) && (bomb == PROXMINE_INDEX))
	return;
count = LOCALPLAYER.secondaryAmmo [bomb];
#if DBG
count = min (count, 99);	//only have room for 2 digits - cheating give 200
#endif
countx = (bomb == PROXMINE_INDEX) ? count : -count;
if (bShowAlways && count == 0)		//no bombs, draw nothing on HUD
	return;
if (!bShowAlways && countx == m_info.old [gameStates.render.vr.nCurrentPage].bombCount)
	return;
sprintf (szBombCount, "B:%02d", count);
while ((t = strchr (szBombCount, '1')))
	*t = '\x84';	//convert to wide '1'
m_info.old [gameStates.render.vr.nCurrentPage].bombCount = countx;

ClearBombCount ();
if (count)
	fontManager.SetColorRGBi (
		(bomb == PROXMINE_INDEX) ? RGBA_PAL2 (55, 0, 0) : RGBA_PAL2 (59, 59, 21), 1,
		bgColor, bgColor != -1);
else if (bgColor != -1)
	fontManager.SetColorRGBi (bgColor, 1, bgColor, 1);	//erase by drawing in background color
nIdBombCount = DrawBombCount (&nIdBombCount, x, y, szBombCount);
}

//	-----------------------------------------------------------------------------

void CGenericCockpit::DrawAmmoInfo (int x, int y, int ammoCount, int bPrimary)
{
	int	w;
	char	szAmmo [16];

	static int nIdAmmo [2][2] = {{0, 0},{0, 0}};

w = (m_info.fontWidth * (bPrimary ? 7 : 5)) / 2;
CCanvas::Current ()->SetColorRGBi (RGBA_PAL (0, 0, 0));
OglDrawFilledRect (HUD_SCALE_X (x), HUD_SCALE_Y (y), HUD_SCALE_X (x+w), HUD_SCALE_Y (y + m_info.fontHeight));
fontManager.SetColorRGBi (RED_RGBA, 1, 0, 0);
sprintf (szAmmo, "%03d", ammoCount);
convert_1s (szAmmo);
nIdAmmo [bPrimary][0] = HUDPrintF (&nIdAmmo [bPrimary][0], x, y, szAmmo);
OglDrawFilledRect (HUD_SCALE_X (x), HUD_SCALE_Y (y), HUD_SCALE_X (x+w), HUD_SCALE_Y (y + m_info.fontHeight));
nIdAmmo [bPrimary][1] = HUDPrintF (&nIdAmmo [bPrimary][1], x, y, szAmmo);
}

//	-----------------------------------------------------------------------------

void CGenericCockpit::AddPointsToScore (int points)
{
	int nPrevScore;

m_info.scoreTime += I2X (1) * 2;
m_info.scoreDisplay [0] += points;
m_info.scoreDisplay [1] += points;
if (m_info.scoreTime > I2X (1) * 4) 
	m_info.scoreTime = I2X (1) * 4;
if (!points || gameStates.app.cheats.bEnabled)
	return;
if (IsMultiGame && !IsCoopGame)
	return;
nPrevScore = LOCALPLAYER.score;
LOCALPLAYER.score += points;
if (gameData.demo.nState == ND_STATE_RECORDING)
	NDRecordPlayerScore (points);
if (IsCoopGame)
	MultiSendScore ();
if (!IsMultiGame)
	CheckForExtraLife (nPrevScore);
}

//	-----------------------------------------------------------------------------

void CGenericCockpit::AddBonusPointsToScore (int points)
{
	int nPrevScore;

if (!points || gameStates.app.cheats.bEnabled)
	return;
nPrevScore = LOCALPLAYER.score;
LOCALPLAYER.score += points;
if (gameData.demo.nState == ND_STATE_RECORDING)
	NDRecordPlayerScore (points);
if (!IsMultiGame)
	CheckForExtraLife (nPrevScore);
}

//	-----------------------------------------------------------------------------

void CGenericCockpit::DrawPlayerShip (int nCloakState, int nOldCloakState, int x, int y)
{
	static fix xCloakFadeTimer = 0;
	static int nCloakFadeValue = FADE_LEVELS - 1;
	static int refade = 0;

if ((nOldCloakState == -1) && nCloakState)
	nCloakFadeValue = 0;
if (!nCloakState) {
	nCloakFadeValue = FADE_LEVELS - 1;
	nCloakFadeState = 0;
	}
if ((nCloakState == 1) && !nOldCloakState)
	nCloakFadeState = -1;
if (nCloakState == nOldCloakState)		//doing "about-to-uncloak" effect
	if (!nCloakFadeState)
		nCloakFadeState = 2;
if (nCloakState && (gameData.time.xGame > LOCALPLAYER.cloakTime + CLOAK_TIME_MAX - I2X (3)))		//doing "about-to-uncloak" effect
	if (!nCloakFadeState)
		nCloakFadeState = 2;
if (nCloakFadeState)
	xCloakFadeTimer -= gameData.time.xFrame;
while (nCloakFadeState && (xCloakFadeTimer < 0)) {
	xCloakFadeTimer += CLOAK_FADE_WAIT_TIME;
	nCloakFadeValue += nCloakFadeState;
	if (nCloakFadeValue >= FADE_LEVELS - 1) {
		nCloakFadeValue = FADE_LEVELS - 1;
		if (nCloakFadeState == 2 && nCloakState)
			nCloakFadeState = -2;
		else
			nCloakFadeState = 0;
		}
	else if (nCloakFadeValue <= 0) {
		nCloakFadeValue = 0;
		if (nCloakFadeState == -2)
			nCloakFadeState = 2;
		else
			nCloakFadeState = 0;
		}
	}

//	To fade out both pages in a paged mode.
if (refade)
	refade = 0;
else if (nCloakState && nOldCloakState && !nCloakFadeState && !refade) {
	nCloakFadeState = -1;
	refade = 1;
	}
#if 0
if (gameStates.render.cockpit.nMode != CM_FULL_COCKPIT)
	CCanvas::SetCurrent (&gameStates.render.vr.buffers.render [0]);
#endif
gameStates.render.grAlpha = (float) nCloakFadeValue / (float) FADE_LEVELS;
HUDBitBlt (GAUGE_SHIPS + (IsTeamGame ? GetTeam (gameData.multiplayer.nLocalPlayer) : gameData.multiplayer.nLocalPlayer), x, y);
gameStates.render.grAlpha = FADE_LEVELS;
#if 0
if (gameStates.render.cockpit.nMode != CM_FULL_COCKPIT)
	CCanvas::SetCurrent (GetCurrentGameScreen ());
#endif
}

//	-----------------------------------------------------------------------------

void DrawWeaponInfo (int info_index, tGaugeBox *box, int xPic, int yPic, const char *pszName, int xText, int yText, int orient)
{
	CBitmap*	bmP;
	char		szName [100], *p;
	int		l;

	static int nIdWeapon [3] = {0, 0, 0}, nIdLaser [2] = {0, 0};

HUDBitBlt (((gameData.pig.tex.nHamFileVersion >= 3) && gameStates.video.nDisplayMode) 
			  ? gameData.weapons.info [info_index].hiresPicture.index
			  : gameData.weapons.info [info_index].picture.index, 
			  xPic, yPic, true, true, (gameStates.render.cockpit.nMode == CM_FULL_SCREEN) ? I2X (2) : I2X (1), orient);
if (gameStates.render.cockpit.nMode == CM_FULL_SCREEN)
	return;
fontManager.SetColorRGBi (GREEN_RGBA, 1, 0, 0);
if ((p = const_cast<char*> (strchr (pszName, '\n')))) {
	memcpy (szName, pszName, l = p - pszName);
	szName [l] = '\0';
	nIdWeapon [0] = HUDPrintF (&nIdWeapon [0], xText, yText, szName);
	nIdWeapon [1] = HUDPrintF (&nIdWeapon [1], xText, yText + m_info.fontHeight + 1, p + 1);
	}
else {
	nIdWeapon [2] = HUDPrintF (&nIdWeapon [2], xText, yText, pszName);
	}

//	For laser, show level and quadness
if (info_index == LASER_ID || info_index == SUPERLASER_ID) {
	sprintf (szName, "%s: 0", TXT_LVL);
	szName [5] = LOCALPLAYER.laserLevel + 1 + '0';
	nIdLaser [0] = HUDPrintF (&nIdLaser [0], xText, yText + nLineSpacing, szName);
	if (LOCALPLAYER.flags & PLAYER_FLAGS_QUAD_LASERS) {
		strcpy (szName, TXT_QUAD);
		nIdLaser [1] = HUDPrintF (&nIdLaser [1], xText, yText + 2 * nLineSpacing, szName);
		}
	}
}

//	-----------------------------------------------------------------------------

void DrawWeaponInfo (int nWeaponType, int nWeaponId, int laserLevel)
{
	int nIndex;

if (nWeaponType == 0) {
	nIndex = primaryWeaponToWeaponInfo [nWeaponId];

	if (nIndex == LASER_ID && laserLevel > MAX_LASER_LEVEL)
		nIndex = SUPERLASER_ID;

	if (gameStates.render.cockpit.nMode == CM_STATUS_BAR)
		DrawWeaponInfo (nIndex,
			hudWindowAreas + SB_PRIMARY_BOX,
			SB_PRIMARY_W_PIC_X, SB_PRIMARY_W_PIC_Y,
			PRIMARY_WEAPON_NAMES_SHORT (nWeaponId),
			SB_PRIMARY_W_TEXT_X, SB_PRIMARY_W_TEXT_Y, 0);
	else
		DrawWeaponInfo (nIndex,
			hudWindowAreas + COCKPIT_PRIMARY_BOX,
			PRIMARY_W_PIC_X, PRIMARY_W_PIC_Y,
			PRIMARY_WEAPON_NAMES_SHORT (nWeaponId),
			PRIMARY_W_TEXT_X, PRIMARY_W_TEXT_Y, 0);
		}
else {
	nIndex = secondaryWeaponToWeaponInfo [nWeaponId];

	if (gameStates.render.cockpit.nMode == CM_STATUS_BAR)
		DrawWeaponInfo (nIndex,
			hudWindowAreas + SB_SECONDARY_BOX,
			SB_SECONDARY_W_PIC_X, SB_SECONDARY_W_PIC_Y,
			SECONDARY_WEAPON_NAMES_SHORT (nWeaponId),
			SB_SECONDARY_W_TEXT_X, SB_SECONDARY_W_TEXT_Y, 0);
	else
		DrawWeaponInfo (nIndex,
			hudWindowAreas + COCKPIT_SECONDARY_BOX,
			SECONDARY_W_PIC_X, SECONDARY_W_PIC_Y,
			SECONDARY_WEAPON_NAMES_SHORT (nWeaponId),
			SECONDARY_W_TEXT_X, SECONDARY_W_TEXT_Y, 0);
	}
}

//	-----------------------------------------------------------------------------

//returns true if drew picture
int DrawWeaponDisplay (int nWeaponType, int nWeaponId)
{
	int bDrew = 0;
	int bLaserLevelChanged;

//CCanvas::SetCurrent (&gameStates.render.vr.buffers.render [0]);
bLaserLevelChanged = ((nWeaponType == 0) &&
								(nWeaponId == LASER_INDEX) &&
								((LOCALPLAYER.laserLevel != m_info.old [gameStates.render.vr.nCurrentPage].laserLevel)));

if ((nWeaponId != m_info.history [gameStates.render.vr.nCurrentPage].weapon [nWeaponType] || bLaserLevelChanged) && weaponBoxStates [nWeaponType] == WS_SET) {
	weaponBoxStates [nWeaponType] = WS_FADING_OUT;
	weaponBoxFadeValues [nWeaponType] = I2X (FADE_LEVELS - 1);
	}

ShowWeaponInfo (nWeaponType, nWeaponId, LOCALPLAYER.laserLevel);
m_info.history [gameStates.render.vr.nCurrentPage].weapon [nWeaponType] = nWeaponId;
m_info.history [gameStates.render.vr.nCurrentPage].ammo [nWeaponType] = -1;
xOldOmegaCharge [gameStates.render.vr.nCurrentPage] = -1;
m_info.old [gameStates.render.vr.nCurrentPage].laserLevel = LOCALPLAYER.laserLevel;
bDrew = 1;
weaponBoxStates [nWeaponType] = WS_SET;

if (weaponBoxStates [nWeaponType] == WS_FADING_OUT) {
	ShowWeaponInfo (nWeaponType, m_info.history [gameStates.render.vr.nCurrentPage].weapon [nWeaponType], m_info.old [gameStates.render.vr.nCurrentPage].laserLevel);
	m_info.history [gameStates.render.vr.nCurrentPage].ammo [nWeaponType] = -1;
	xOldOmegaCharge [gameStates.render.vr.nCurrentPage] = -1;
	bDrew = 1;
	weaponBoxFadeValues [nWeaponType] -= gameData.time.xFrame * FADE_SCALE;
	if (weaponBoxFadeValues [nWeaponType] <= 0) {
		weaponBoxStates [nWeaponType] = WS_FADING_IN;
		m_info.history [gameStates.render.vr.nCurrentPage].weapon [nWeaponType] = nWeaponId;
		oldWeapon [nWeaponType][!gameStates.render.vr.nCurrentPage] = nWeaponId;
		m_info.old [gameStates.render.vr.nCurrentPage].laserLevel = LOCALPLAYER.laserLevel;
		oldLaserLevel [!gameStates.render.vr.nCurrentPage] = LOCALPLAYER.laserLevel;
		weaponBoxFadeValues [nWeaponType] = 0;
		}
	}
else if (weaponBoxStates [nWeaponType] == WS_FADING_IN) {
	if (nWeaponId != m_info.history [gameStates.render.vr.nCurrentPage].weapon [nWeaponType]) {
		weaponBoxStates [nWeaponType] = WS_FADING_OUT;
		}
	else {
		ShowWeaponInfo (nWeaponType, nWeaponId, LOCALPLAYER.laserLevel);
		m_info.history [gameStates.render.vr.nCurrentPage].ammo [nWeaponType] = -1;
		xOldOmegaCharge [gameStates.render.vr.nCurrentPage] = -1;
		bDrew=1;
		weaponBoxFadeValues [nWeaponType] += gameData.time.xFrame * FADE_SCALE;
		if (weaponBoxFadeValues [nWeaponType] >= I2X (FADE_LEVELS-1)) {
			weaponBoxStates [nWeaponType] = WS_SET;
			oldWeapon [nWeaponType][!gameStates.render.vr.nCurrentPage] = -1;		//force redraw (at full fade-in) of other page
			}
		}
	}

if (weaponBoxStates [nWeaponType] != WS_SET) {		//fade gauge
	int fadeValue = X2I (weaponBoxFadeValues [nWeaponType]);
	int boxofs = (gameStates.render.cockpit.nMode == CM_STATUS_BAR) ? SB_PRIMARY_BOX : COCKPIT_PRIMARY_BOX;
	gameStates.render.grAlpha = (float) fadeValue;
	OglDrawFilledRect (hudWindowAreas [boxofs + nWeaponType].left,
						    hudWindowAreas [boxofs + nWeaponType].top,
						    hudWindowAreas [boxofs + nWeaponType].right,
						    hudWindowAreas [boxofs + nWeaponType].bot);
	gameStates.render.grAlpha = FADE_LEVELS;
	}
//CCanvas::SetCurrent (GetCurrentGameScreen ());
return bDrew;
}

//	-----------------------------------------------------------------------------

fix staticTime [2];

void CGenericCockpit::DrawStatic (int nWindow, int nIndex)
{
	tVideoClip *vc = gameData.eff.vClips [0] + VCLIP_MONITOR_STATIC;
	CBitmap *bmp;
	int framenum;
	int boxofs = (gameStates.render.cockpit.nMode == CM_STATUS_BAR) ? SB_PRIMARY_BOX : COCKPIT_PRIMARY_BOX;
	int h, x, y;

staticTime [nWindow] += gameData.time.xFrame;
if (staticTime [nWindow] >= vc->xTotalTime) {
	weaponBoxUser [nWindow] = WBU_WEAPON;
	return;
	}
framenum = staticTime [nWindow] * vc->nFrameCount / vc->xTotalTime;
LoadBitmap (vc->frames [framenum].index, 0);
bmp = gameData.pig.tex.bitmaps [0] + vc->frames [framenum].index;
//CCanvas::SetCurrent (&gameStates.render.vr.buffers.render [0]);
h = boxofs + nWindow;
for (x = hudWindowAreas [h].left; x < hudWindowAreas [h].right; x += bmp->Width ())
	for (y = hudWindowAreas [h].top; y < hudWindowAreas [h].bot; y += bmp->Height ())
		HUDBitBlt (-1, x, y, true, true, I2X (1), 0, bmp);
//CCanvas::SetCurrent (GetCurrentGameScreen ());
}

//	-----------------------------------------------------------------------------

void CGenericCockpit::DrawWeapons (void)
{
if (weaponBoxUser [0] == WBU_WEAPON) {
	if (DrawWeaponDisplay (0, gameData.weapons.nPrimary) && (weaponBoxStates [0] == WS_SET)) {
		 (((gameData.weapons.nPrimary == VULCAN_INDEX) || (gameData.weapons.nPrimary == GAUSS_INDEX)) {
			if (LOCALPLAYER.primaryAmmo [VULCAN_INDEX] != m_info.history [gameStates.render.vr.nCurrentPage].ammo [0]) {
				if (gameData.demo.nState == ND_STATE_RECORDING)
					NDRecordPrimaryAmmo (m_info.history [gameStates.render.vr.nCurrentPage].ammo [0], LOCALPLAYER.primaryAmmo [VULCAN_INDEX]);
				DrawPrimaryAmmoInfo (X2I ((unsigned) VULCAN_AMMO_SCALE * (unsigned) LOCALPLAYER.primaryAmmo [VULCAN_INDEX]));
				m_info.history [gameStates.render.vr.nCurrentPage].ammo [0] = LOCALPLAYER.primaryAmmo [VULCAN_INDEX];
				}
			}
		else if (gameData.weapons.nPrimary == OMEGA_INDEX) {
			if (gameData.omega.xCharge [IsMultiGame] != xOldOmegaCharge [gameStates.render.vr.nCurrentPage]) {
				if (gameData.demo.nState == ND_STATE_RECORDING)
					NDRecordPrimaryAmmo (xOldOmegaCharge [gameStates.render.vr.nCurrentPage], gameData.omega.xCharge [IsMultiGame]);
				DrawPrimaryAmmoInfo (gameData.omega.xCharge [IsMultiGame] * 100 / MAX_OMEGA_CHARGE);
				xOldOmegaCharge [gameStates.render.vr.nCurrentPage] = gameData.omega.xCharge [IsMultiGame];
				}
			}
		}
	}
else if (weaponBoxUser [0] == WBU_STATIC)
	DrawStatic (0);

if (weaponBoxUser [1] == WBU_WEAPON) {
	if (DrawWeaponDisplay (1, gameData.weapons.nSecondary) && (weaponBoxStates [1] == WS_SET) &&
		 (LOCALPLAYER.secondaryAmmo [gameData.weapons.nSecondary] != m_info.history [gameStates.render.vr.nCurrentPage].ammo [1])) {
		oldBombcount [gameStates.render.vr.nCurrentPage] = 0x7fff;	//force redraw
		if (gameData.demo.nState == ND_STATE_RECORDING)
			NDRecordSecondaryAmmo (m_info.history [gameStates.render.vr.nCurrentPage].ammo [1], LOCALPLAYER.secondaryAmmo [gameData.weapons.nSecondary]);
		DrawSecondaryAmmoInfo (LOCALPLAYER.secondaryAmmo [gameData.weapons.nSecondary]);
		m_info.history [gameStates.render.vr.nCurrentPage].ammo [1] = LOCALPLAYER.secondaryAmmo [gameData.weapons.nSecondary];
		}
	}
else if (weaponBoxUser [1] == WBU_STATIC)
	DrawStatic (1);
}

//	-----------------------------------------------------------------------------

tRgbColorb playerColors [] = {
 {15, 15, 23},
 {27, 0, 0},
 {0, 23, 0},
 {30, 11, 31},
 {31, 16, 0},
 {24, 17, 6},
 {14, 21, 12},
 {29, 29, 0}};

typedef struct {
	sbyte x, y;
} xy;

//offsets for reticle parts: high-big  high-sml  low-big  low-sml
xy cross_offsets [4] = 	 {{-8, -5},  {-4, -2},  {-4, -2}, {-2, -1}};
xy primary_offsets [4] =  {{-30, 14}, {-16, 6},  {-15, 6}, {-8, 2}};
xy secondary_offsets [4] = {{-24, 2},  {-12, 0}, {-12, 1}, {-6, -2}};

//draw the reticle
void CGenericCockpit::DrawReticle (int bForceBig)
{
	int x, y;
	int bLaserReady, bMissileReady, bLaserAmmo, bMissileAmmo;
	int nCrossBm, nPrimaryBm, nSecondaryBm;
	int bHiresReticle, bSmallReticle, ofs;

if ((gameData.demo.nState==ND_STATE_PLAYBACK) && gameData.demo.bFlyingGuided) {
	DrawGuidedCrosshair ();
	return;
	}

x = CCanvas::Current ()->Width () / 2;
y = CCanvas::Current ()->Height () / ((gameStates.render.cockpit.nMode == CM_FULL_COCKPIT) ? 2 : 2);
bLaserReady = AllowedToFireLaser ();
bMissileReady = AllowedToFireMissile (-1, 1);
bLaserAmmo = PlayerHasWeapon (gameData.weapons.nPrimary, 0, -1, 1);
bMissileAmmo = PlayerHasWeapon (gameData.weapons.nSecondary, 1, -1, 1);
nPrimaryBm = bLaserReady && (bLaserAmmo == HAS_ALL);
nSecondaryBm = bMissileReady && (bMissileAmmo == HAS_ALL);
if (nPrimaryBm && (gameData.weapons.nPrimary == LASER_INDEX) &&
	 (LOCALPLAYER.flags & PLAYER_FLAGS_QUAD_LASERS))
	nPrimaryBm++;

if (secondaryWeaponToGunNum [gameData.weapons.nSecondary] == 7)
	nSecondaryBm += 3;		//now value is 0, 1 or 3, 4
else if (nSecondaryBm && !(gameData.laser.nMissileGun & 1))
		nSecondaryBm++;

nCrossBm = ((nPrimaryBm > 0) || (nSecondaryBm > 0));

Assert (nPrimaryBm <= 2);
Assert (nSecondaryBm <= 4);
Assert (nCrossBm <= 1);
#if DBG
if (gameStates.render.bExternalView)
#else
if (gameStates.render.bExternalView && (!IsMultiGame || EGI_FLAG (bEnableCheats, 0, 0, 0)))
#endif
	return;
cmScaleX *= HUD_ASPECT;
if ((gameStates.ogl.nReticle == 2) || (gameStates.ogl.nReticle && CCanvas::Current ()->Width () > 320))
   OglDrawReticle (nCrossBm, nPrimaryBm, nSecondaryBm);
else {
	bHiresReticle = (gameStates.render.fonts.bHires != 0);
	bSmallReticle = !bForceBig && (CCanvas::Current ()->Width () * 3 <= gameData.render.window.wMax * 2);
	ofs = (bHiresReticle ? 0 : 2) + bSmallReticle;

	HUDBitBlt ((bSmallReticle ? SML_RETICLE_CROSS : RETICLE_CROSS) + nCrossBm,
				  (x + HUD_SCALE_X (cross_offsets [ofs].x)), (y + HUD_SCALE_Y (cross_offsets [ofs].y)), false, true);
	HUDBitBlt ((bSmallReticle ? SML_RETICLE_PRIMARY : RETICLE_PRIMARY) + nPrimaryBm,
					(x + HUD_SCALE_X (primary_offsets [ofs].x)), (y + HUD_SCALE_Y (primary_offsets [ofs].y)), false, true);
	HUDBitBlt ((bSmallReticle ? SML_RETICLE_SECONDARY : RETICLE_SECONDARY) + nSecondaryBm,
					(x + HUD_SCALE_X (secondary_offsets [ofs].x)), (y + HUD_SCALE_Y (secondary_offsets [ofs].y)), false, true);
  }
if (!gameStates.app.bNostalgia && gameOpts->input.mouse.bJoystick && gameOpts->render.cockpit.bMouseIndicator)
	OglDrawMouseIndicator ();
cmScaleX /= HUD_ASPECT;
}

//	-----------------------------------------------------------------------------

#if DBG
extern int bSavingMovieFrames;
#else
#define bSavingMovieFrames 0
#endif

//returns true if viewerP can see CObject
//	-----------------------------------------------------------------------------

int CGenericCockpit::CanSeeObject (int nObject, int bCheckObjs)
{
	tFVIQuery fq;
	int nHitType;
	tFVIData hit_data;

	//see if we can see this CPlayerData

fq.p0 = &gameData.objs.viewerP->info.position.vPos;
fq.p1 = &OBJECTS [nObject].info.position.vPos;
fq.radP0 =
fq.radP1 = 0;
fq.thisObjNum = gameStates.render.cameras.bActive ? -1 : OBJ_IDX (gameData.objs.viewerP);
fq.flags = bCheckObjs ? FQ_CHECK_OBJS | FQ_TRANSWALL : FQ_TRANSWALL;
fq.startSeg = gameData.objs.viewerP->info.nSegment;
fq.ignoreObjList = NULL;
nHitType = FindVectorIntersection (&fq, &hit_data);
return bCheckObjs ? (nHitType == HIT_OBJECT) && (hit_data.hit.nObject == nObject) : (nHitType != HIT_WALL);
}

//	-----------------------------------------------------------------------------

//show names of teammates & players carrying flags
void CGenericCockpit::DrawPlayerNames (void)
{
	int			bHasFlag, bShowName, bShowTeamNames, bShowAllNames, bShowFlags, nObject, nTeam;
	int			p;
	tRgbColorb*	colorP;
	
	static int nCurColor = 1, tColorChange = 0;
	static tRgbColorb typingColors [2] = {
		{63, 0, 0},
		{63, 63, 0}
	};
	char s [CALLSIGN_LEN + 10];
	int w, h, aw;
	int x0, y0, x1, y1;
	int nColor;
	static int nIdNames [2][MAX_PLAYERS] = {{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0}};

bShowAllNames = ((gameData.demo.nState == ND_STATE_PLAYBACK) || (netGame.bShowAllNames && gameData.multigame.bShowReticleName));
bShowTeamNames = (gameData.multigame.bShowReticleName && (IsCoopGame || IsTeamGame));
bShowFlags = (gameData.app.nGameMode & (GM_CAPTURE | GM_HOARD | GM_ENTROPY));

nTeam = GetTeam (gameData.multiplayer.nLocalPlayer);
for (p = 0; p < gameData.multiplayer.nPlayers; p++) {	//check all players

	bShowName = (gameStates.multi.bPlayerIsTyping [p] || 
					 (bShowAllNames && !(gameData.multiplayer.players [p].flags & PLAYER_FLAGS_CLOAKED)) || 
					 (bShowTeamNames && GetTeam (p) == nTeam));
	bHasFlag = (gameData.multiplayer.players [p].connected && gameData.multiplayer.players [p].flags & PLAYER_FLAGS_FLAG);

	if (gameData.demo.nState != ND_STATE_PLAYBACK)
		nObject = gameData.multiplayer.players [p].nObject;
	else {
		//if this is a demo, the nObject in the CPlayerData struct is wrong,
		//so we search the CObject list for the nObject
		CObject *objP;
		FORALL_PLAYER_OBJS (objP, nObject) 
			if (objP->info.nId == p) {
				nObject = objP->Index ();
				break;
				}
		if (IS_OBJECT (objP, nObject))		//not in list, thus not visible
			bShowName = !bHasFlag;				//..so don't show name
		}

	if ((bShowName || bHasFlag) && CanSeeObject (nObject, 1)) {
		g3sPoint		vPlayerPos;
		CFixVector	vPos;

		vPos = OBJECTS [nObject].info.position.vPos;
		vPos[Y] += I2X (2);
		G3TransformAndEncodePoint(&vPlayerPos, vPos);
		if (vPlayerPos.p3_codes == 0) {	//on screen
			G3ProjectPoint (&vPlayerPos);
			if (!(vPlayerPos.p3_flags & PF_OVERFLOW)) {
				fix x = vPlayerPos.p3_screen.x;
				fix y = vPlayerPos.p3_screen.y;
				if (bShowName) {				// Draw callsign on HUD
					if (gameStates.multi.bPlayerIsTyping [p]) {
						int t = gameStates.app.nSDLTicks;

						if (t - tColorChange > 333) {
							tColorChange = t;
							nCurColor = !nCurColor;
							}
						colorP = typingColors + nCurColor;
						}
					else {
						nColor = IsTeamGame ? GetTeam (p) : p;
						colorP = playerColors + nColor;
						}

					sprintf (s, "%s", gameStates.multi.bPlayerIsTyping [p] ? TXT_TYPING : gameData.multiplayer.players [p].callsign);
					fontManager.Current ()->StringSize (s, w, h, aw);
					fontManager.SetColorRGBi (RGBA_PAL2 (colorP->red, colorP->green, colorP->blue), 1, 0, 0);
					x1 = x - w / 2;
					y1 = y - h / 2;
					//glBlendFunc (GL_ONE, GL_ONE);
					nIdNames [nCurColor][p] = GrString (x1, y1, s, nIdNames [nCurColor] + p);
					CCanvas::Current ()->SetColorRGBi (RGBA_PAL2 (colorP->red, colorP->green, colorP->blue));
					glLineWidth ((GLfloat) 2.0f);
					OglDrawEmptyRect (x1 - 4, y1 - 3, x1 + w + 2, y1 + h + 3);
					glLineWidth (1);
					//glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
					}

				if (bHasFlag && (gameStates.app.bNostalgia || !(EGI_FLAG (bTargetIndicators, 0, 1, 0) || EGI_FLAG (bTowFlags, 0, 1, 0)))) {// Draw box on HUD
					fix dy = -FixMulDiv (OBJECTS [nObject].info.xSize, I2X (CCanvas::Current ()->Height ())/2, vPlayerPos.p3_vec[Z]);
//					fix dy = -FixMulDiv (FixMul (OBJECTS [nObject].size, transformation.m_info.scale.y), I2X (CCanvas::Current ()->Height ())/2, vPlayerPos.p3_z);
					fix dx = FixMul (dy, screen.Aspect ());
					fix w = dx / 4;
					fix h = dy / 4;
					if (gameData.app.nGameMode & (GM_CAPTURE | GM_ENTROPY))
						CCanvas::Current ()->SetColorRGBi ((GetTeam (p) == TEAM_BLUE) ? MEDRED_RGBA :  MEDBLUE_RGBA);
					else if (gameData.app.nGameMode & GM_HOARD) {
						if (gameData.app.nGameMode & GM_TEAM)
							CCanvas::Current ()->SetColorRGBi ((GetTeam (p) == TEAM_RED) ? MEDRED_RGBA :  MEDBLUE_RGBA);
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
	int nPlayers, playerList [MAX_NUM_NET_PLAYERS];
	int nLeft, i, xo = 0, x0, x1, y0, fth, l;
	int t = gameStates.app.nSDLTicks;
	int bGetPing = gameStates.render.cockpit.bShowPingStats && (!networkData.tLastPingStat || (t - networkData.tLastPingStat >= 1000));
	static int faw = -1;

	static int nIdKillList [2][MAX_PLAYERS] = {{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0}};

if (HIDE_HUD)
	return;

if (bGetPing)
	networkData.tLastPingStat = t;
if (gameData.multigame.kills.xShowListTimer > 0) {
	gameData.multigame.kills.xShowListTimer -= gameData.time.xFrame;
	if (gameData.multigame.kills.xShowListTimer < 0)
		gameData.multigame.kills.bShowList = 0;
	}
nPlayers = (gameData.multigame.kills.bShowList == 3) ? 2 : MultiGetKillList (playerList);
nLeft = (nPlayers <= 4) ? nPlayers : (nPlayers + 1) / 2;
//If font size changes, this code might not work right anymore
//Assert (GAME_FONT->Height ()==5 && GAME_FONT->Width ()==7);
fth = GAME_FONT->Height ();
y -= nLeft * (fth + 1);
x = CCanvas::Current ()->Width () - HUD_LHX (x);
x0 = HUD_LHX (1);
x1 = HUD_LHX (43);
y0 = y;

if (gameStates.render.cockpit.bShowPingStats) {
	if (faw < 0)
		faw = GAME_FONT->TotalWidth () / (GAME_FONT->Range ());
		if (gameData.multigame.kills.bShowList == 2)
			xo = faw * 24;//was +25;
		else if (IsCoopGame)
			xo = faw * 14;//was +30;
		else
			xo = faw * 8; //was +20;
	}
for (i = 0; i < nPlayers; i++) {
	int nPlayer;
	char name [80], teamInd [2] = {127, 0};
	int sw, sh, aw, indent =0;

	if (i >= nLeft) {
		x0 = x;
		x1 = CCanvas::Current ()->Width () - ((gameData.app.nGameMode & GM_MULTI_COOP) ? HUD_LHX (27) : HUD_LHX (15));
		if (gameStates.render.cockpit.bShowPingStats) {
			x0 -= xo + 6 * faw;
			x1 -= xo + 6 * faw;
			}
		if (i == nLeft)
			y0 = y;
		if (netGame.KillGoal || netGame.xPlayTimeAllowed)
			x1 -= HUD_LHX (18);
		}
	else if (netGame.KillGoal || netGame.xPlayTimeAllowed)
		 x1 = HUD_LHX (43) - HUD_LHX (18);
	nPlayer = (gameData.multigame.kills.bShowList == 3) ? i : playerList [i];
	if ((gameData.multigame.kills.bShowList == 1) || (gameData.multigame.kills.bShowList == 2)) {
		int color;

		if (gameData.multiplayer.players [nPlayer].connected != 1)
			fontManager.SetColorRGBi (RGBA_PAL2 (12, 12, 12), 1, 0, 0);
		else {
			if (IsTeamGame)
				color = GetTeam (nPlayer);
			else
				color = nPlayer;
			fontManager.SetColorRGBi (RGBA_PAL2 (playerColors [color].red, playerColors [color].green, playerColors [color].blue), 1, 0, 0);
			}
		}
	else
		fontManager.SetColorRGBi (RGBA_PAL2 (playerColors [nPlayer].red, playerColors [nPlayer].green, playerColors [nPlayer].blue), 1, 0, 0);
	if (gameData.multigame.kills.bShowList == 3) {
		if (GetTeam (gameData.multiplayer.nLocalPlayer) == i) {
#if 0//def _DEBUG
			sprintf (name, "%c%-8s %d.%d.%d.%d:%d",
						teamInd [0], netGame.szTeamName [i],
						netPlayers.players [i].network.ipx.node [0],
						netPlayers.players [i].network.ipx.node [1],
						netPlayers.players [i].network.ipx.node [2],
						netPlayers.players [i].network.ipx.node [3],
						netPlayers.players [i].network.ipx.node [5] +
						 (unsigned) netPlayers.players [i].network.ipx.node [4] * 256);
#else
			sprintf (name, "%c%s", teamInd [0], netGame.szTeamName [i]);
#endif
			indent = 0;
			}
		else {
#if SHOW_PLAYER_IP
			sprintf (name, "%-8s %d.%d.%d.%d:%d",
						netGame.szTeamName [i],
						netPlayers.players [i].network.ipx.node [0],
						netPlayers.players [i].network.ipx.node [1],
						netPlayers.players [i].network.ipx.node [2],
						netPlayers.players [i].network.ipx.node [3],
						netPlayers.players [i].network.ipx.node [5] +
						 (unsigned) netPlayers.players [i].network.ipx.node [4] * 256);
#else
			strcpy (name, netGame.szTeamName [i]);
#endif
			fontManager.Current ()->StringSize (teamInd, indent, sh, aw);
			}
		}
	else
#if 0//def _DEBUG
		sprintf (name, "%-8s %d.%d.%d.%d:%d",
					gameData.multiplayer.players [nPlayer].callsign,
					netPlayers.players [nPlayer].network.ipx.node [0],
					netPlayers.players [nPlayer].network.ipx.node [1],
					netPlayers.players [nPlayer].network.ipx.node [2],
					netPlayers.players [nPlayer].network.ipx.node [3],
					netPlayers.players [nPlayer].network.ipx.node [5] +
					 (unsigned) netPlayers.players [nPlayer].network.ipx.node [4] * 256);
#else
		strcpy (name, gameData.multiplayer.players [nPlayer].callsign);	// Note link to above if!!
#endif
#if 0//def _DEBUG
	x1 += HUD_LHX (100);
#endif
	for (l = (int) strlen (name); l;) {
		fontManager.Current ()->StringSize (name, sw, sh, aw);
		if (sw <= x1 - x0 - HUD_LHX (2))
			break;
		name [--l] = '\0';
		}
	nIdKillList [0][i] = GrPrintF (nIdKillList [0] + i, x0 + indent, y0, "%s", name);

	if (gameData.multigame.kills.bShowList == 2) {
		if (gameData.multiplayer.players [nPlayer].netKilledTotal + gameData.multiplayer.players [nPlayer].netKillsTotal <= 0)
			nIdKillList [1][i] = GrPrintF (nIdKillList [1] + i, x1, y0, TXT_NOT_AVAIL);
		else
			nIdKillList [1][i] = GrPrintF (nIdKillList [1] + i, x1, y0, "%d%%",
				 (int) ((double) gameData.multiplayer.players [nPlayer].netKillsTotal /
						 ((double) gameData.multiplayer.players [nPlayer].netKilledTotal +
						  (double) gameData.multiplayer.players [nPlayer].netKillsTotal) * 100.0));
		}
	else if (gameData.multigame.kills.bShowList == 3) {
		if (gameData.app.nGameMode & GM_ENTROPY)
			nIdKillList [1][i] = GrPrintF (nIdKillList [1] + i, x1, y0, "%3d [%d/%d]",
						 gameData.multigame.kills.nTeam [i], gameData.entropy.nTeamRooms [i + 1],
						 gameData.entropy.nTotalRooms);
		else
			nIdKillList [1][i] = GrPrintF (nIdKillList [1] + i, x1, y0, "%3d", gameData.multigame.kills.nTeam [i]);
		}
	else if (IsCoopGame)
		nIdKillList [1][i] = GrPrintF (nIdKillList [1] + i, x1, y0, "%-6d", gameData.multiplayer.players [nPlayer].score);
   else if (netGame.xPlayTimeAllowed || netGame.KillGoal)
      nIdKillList [1][i] = GrPrintF (nIdKillList [1] + i, x1, y0, "%3d (%d)",
					 gameData.multiplayer.players [nPlayer].netKillsTotal,
					 gameData.multiplayer.players [nPlayer].nKillGoalCount);
   else
		nIdKillList [1][i] = GrPrintF (nIdKillList [1] + i, x1, y0, "%3d", gameData.multiplayer.players [nPlayer].netKillsTotal);
	if (gameStates.render.cockpit.bShowPingStats && (nPlayer != gameData.multiplayer.nLocalPlayer)) {
		if (bGetPing)
			PingPlayer (nPlayer);
		if (pingStats [nPlayer].sent) {
#if 0//def _DEBUG
			nIdKillList [1][i] = GrPrintF (nIdKillList [1] + i, x1 + xo, y0, "%lu %d %d",
						  pingStats [nPlayer].ping,
						  pingStats [nPlayer].sent,
						  pingStats [nPlayer].received);
#else
			nIdKillList [1][i] = GrPrintF (nIdKillList [1] + i, x1 + xo, y0, "%lu %i%%", pingStats [nPlayer].ping,
													 100 - ((pingStats [nPlayer].received * 100) / pingStats [nPlayer].sent));
#endif
			}
		}
	y0 += fth + 1;
	}
}

//	-----------------------------------------------------------------------------

void DrawCockpit (int nCockpit, int y)
{
if (gameOpts->render.cockpit.bHUD || (gameStates.render.cockpit.nMode != CM_FULL_SCREEN)) {
	int i = gameData.pig.tex.cockpitBmIndex [nCockpit].index;
	CBitmap *bmP = gameData.pig.tex.bitmaps [0] + i;
	LoadBitmap (gameData.pig.tex.cockpitBmIndex [nCockpit].index, 0);
	if (bmP->Override (-1))
		bmP = bmP->Override (-1);
	bmP->SetupTexture (0, 3, 1);
   CCanvas::SetCurrent (gameStates.render.vr.buffers.screenPages + gameStates.render.vr.nCurrentPage);

	tCanvasColor color;

	color.index = 255;
	color.rgb = 0;
	bmP->RenderScaled (0, y, -1, CCanvas::Current ()->Height () - y, I2X (1), 0, &color);
	}
}

//	-----------------------------------------------------------------------------
//draw all the things on the HUD
void CGenericCockpit::Render (void)
{
CCanvas::SetCurrent (GetCurrentGameScreen ());
fontManager.SetCurrent (GAME_FONT);
m_info.fontWidth = CCanvas::Current ()->Font ()->Width ();
m_info.fontHeight = CCanvas::Current ()->Font ()->Height ();
m_info.xScale = screen.Scale (0);
m_info.xScale = screen.Scale (1);
m_info.nEnergy = X2IR (LOCALPLAYER.energy);
m_info.nShields = X2IR (LOCALPLAYER.shields);
m_info.bCloak = ((LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED) != 0);
m_info.nCockpit = (gameStates.video.nDisplayMode && !gameStates.app.bDemoData) ? gameData.models.nCockpits / 2 : 0;

if (gameStates.render.cockpit.nMode == CM_STATUS_BAR) {
	gameStates.render.cockpit.nLastDrawn [0] =
	gameStates.render.cockpit.nLastDrawn [1] = -1;
	Init ();
  }

	//	Show score so long as not in rearview
if (!(gameStates.render.bRearView || bSavingMovieFrames ||
	 (gameStates.render.cockpit.nMode == CM_REAR_VIEW) ||
	 (gameStates.render.cockpit.nMode == CM_STATUS_BAR))) {
	CHUD::ShowScore ();
	if (m_info.scoreTime)
		CHUD::ShowScoreAdded ();
	}

if (!(gameStates.render.bRearView || bSavingMovieFrames || (gameStates.render.cockpit.nMode == CM_REAR_VIEW)))
	CHUD::ShowTimerCount ();

//	Show other stuff if not in rearview or letterbox.
if (!gameStates.render.bRearView && (gameStates.render.cockpit.nMode != CM_REAR_VIEW)) {
	if ((gameStates.render.cockpit.nMode == CM_STATUS_BAR) || (gameStates.render.cockpit.nMode == CM_FULL_SCREEN))
		CHUD::ShowHomingWarning ();

	if ((gameStates.render.cockpit.nMode == CM_FULL_SCREEN) || (gameStates.render.cockpit.nMode == CM_LETTERBOX)) {
		if (!gameOpts->render.cockpit.bTextGauges) {
			if (gameOpts->render.cockpit.bScaleGauges) {
				xGaugeScale = (double) CCanvas::Current ()->Height () / 480.0;
				yGaugeScale = (double) CCanvas::Current ()->Height () / 640.0;
				}
			else
				xGaugeScale = yGaugeScale = 1;
			}
		CHUD::ShowEnergy ();
		CHUD::ShowShield ();
		CHUD::ShowAfterburner ();
		CHUD::ShowWeapons ();
		if (!bSavingMovieFrames)
			CHUD::ShowKeys ();
		CHUD::ShowCloakInvul ();

		if ((gameData.demo.nState == ND_STATE_RECORDING) && (LOCALPLAYER.flags != m_info.old [gameStates.render.vr.nCurrentPage].flags)) {
			NDRecordPlayerFlags (m_info.old [gameStates.render.vr.nCurrentPage].flags, LOCALPLAYER.flags);
			m_info.old [gameStates.render.vr.nCurrentPage].flags = LOCALPLAYER.flags;
			}
		}
#if 1//def _DEBUG
	if (!(IsMultiGame && gameData.multigame.kills.bShowList) && !bSavingMovieFrames)
		ShowTime ();
#endif
	if (gameOpts->render.cockpit.bReticle && !gameStates.app.bPlayerIsDead && !transformation.m_info.bUsePlayerHeadAngles)
		ShowReticle (0);

	DrawPlayerNames ();
	if (gameStates.render.cockpit.nMode != CM_REAR_VIEW) {
		CHUD::ShowFlag ();
		CHUD::ShowOrbs ();
		}
	if (!bSavingMovieFrames)
		HUDRenderMessageFrame ();

	if (gameStates.render.cockpit.nMode != CM_STATUS_BAR && !bSavingMovieFrames)
		CHUD::ShowLives ();

	if (gameData.app.nGameMode&GM_MULTI && gameData.multigame.kills.bShowList)
		CHUD::ShowKillList ();
#if 0
	ShowWeaponDisplays ();
#endif
	return;
	}

if (gameStates.render.bRearView && gameStates.render.cockpit.nMode!=CM_REAR_VIEW) {
	HUDRenderMessageFrame ();
	fontManager.SetColorRGBi (GREEN_RGBA, 1, 0, 0);
	GrPrintF (NULL, 0x8000, CCanvas::Current ()->Height () - ((gameData.demo.nState == ND_STATE_PLAYBACK) ? 14 : 10), TXT_REAR_VIEW);
	}
}

//	-----------------------------------------------------------------------------

//print out some CPlayerData statistics
void CGenericCockpit::RenderGauges (void)
{
	static int old_display_mode = 0;
	int nEnergy = X2IR (LOCALPLAYER.energy);
	int nShields = X2IR (LOCALPLAYER.shields);
	int bCloak = ((LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED) != 0);

if (HIDE_HUD)
	return;

Assert ((gameStates.render.cockpit.nMode == CM_FULL_COCKPIT) || (gameStates.render.cockpit.nMode == CM_STATUS_BAR));
// check to see if our display mode has changed since last render time --
// if so, then we need to make new gauge canvases.
if (old_display_mode != gameStates.video.nDisplayMode) {
	CloseGaugeCanvases ();
	InitGaugeCanvases ();
	old_display_mode = gameStates.video.nDisplayMode;
	}
if (nShields < 0)
	nShields = 0;
//CCanvas::SetCurrent (GetCurrentGameScreen ());
if (gameData.demo.nState == ND_STATE_RECORDING)
	if (LOCALPLAYER.homingObjectDist >= 0)
		NDRecordHomingDistance (LOCALPLAYER.homingObjectDist);

ShowWeaponDisplays ();
if (gameStates.render.cockpit.nMode == CM_STATUS_BAR)
	SBRenderGauges ();
else if (gameStates.render.cockpit.nMode == CM_FULL_COCKPIT) {
	if (nEnergy != oldEnergy [gameStates.render.vr.nCurrentPage]) {
		if (gameData.demo.nState == ND_STATE_RECORDING) {
			NDRecordPlayerEnergy (oldEnergy [gameStates.render.vr.nCurrentPage], nEnergy);
			}
		}
	DrawEnergyBar (nEnergy);
	DrawNumericalDisplay (nShields, nEnergy);
	oldEnergy [gameStates.render.vr.nCurrentPage] = nEnergy;

	if (gameData.physics.xAfterburnerCharge != oldAfterburner [gameStates.render.vr.nCurrentPage]) {
		if (gameData.demo.nState == ND_STATE_RECORDING) {
			NDRecordPlayerAfterburner (oldAfterburner [gameStates.render.vr.nCurrentPage], gameData.physics.xAfterburnerCharge);
			}
		}
	DrawAfterburnerBar (gameData.physics.xAfterburnerCharge);
	oldAfterburner [gameStates.render.vr.nCurrentPage] = gameData.physics.xAfterburnerCharge;

	if (LOCALPLAYER.flags & PLAYER_FLAGS_INVULNERABLE) {
		DrawNumericalDisplay (nShields, nEnergy);
		DrawInvulnerableShip ();
		oldShields [gameStates.render.vr.nCurrentPage] = nShields ^ 1;
		}
	else {
		if (nShields != oldShields [gameStates.render.vr.nCurrentPage]) {		// Draw the shield gauge
			if (gameData.demo.nState == ND_STATE_RECORDING) {
				NDRecordPlayerShields (oldShields [gameStates.render.vr.nCurrentPage], nShields);
				}
			}
		DrawShieldBar (nShields);
		DrawNumericalDisplay (nShields, nEnergy);
		oldShields [gameStates.render.vr.nCurrentPage] = nShields;
		}

	if (LOCALPLAYER.flags != m_info.old [gameStates.render.vr.nCurrentPage].flags) {
		if (gameData.demo.nState==ND_STATE_RECORDING)
			NDRecordPlayerFlags (m_info.old [gameStates.render.vr.nCurrentPage].flags, LOCALPLAYER.flags);
		m_info.old [gameStates.render.vr.nCurrentPage].flags = LOCALPLAYER.flags;
		}
	DrawKeys ();
	ShowHomingWarning ();
	ShowBombCount (BOMB_COUNT_X, BOMB_COUNT_Y, BLACK_RGBA, gameStates.render.cockpit.nMode == CM_FULL_COCKPIT);
	DrawPlayerShip (bCloak, bOldCloak [gameStates.render.vr.nCurrentPage], SHIP_GAUGE_X, SHIP_GAUGE_Y);
	}
bOldCloak [gameStates.render.vr.nCurrentPage] = bCloak;
}

//	---------------------------------------------------------------------------------------------------------
//	Call when picked up a laser powerup.
//	If laser is active, set oldWeapon [0] to -1 to force redraw.
void CGenericCockpit::UpdateLaserWeaponInfo (void)
{
if (m_info.old [gameStates.render.vr.nCurrentPage].weapon [0] == 0)
	if (!(LOCALPLAYER.laserLevel > MAX_LASER_LEVEL && m_info.old [gameStates.render.vr.nCurrentPage].laserLevel <= MAX_LASER_LEVEL))
		m_info.old [gameStates.render.vr.nCurrentPage].weapon [0] = -1;
}

//	---------------------------------------------------------------------------------------------------------
//draws a 3d view into one of the cockpit windows.  win is 0 for left,
//1 for right.  viewerP is CObject.  NULL CObject means give up window
//nUser is one of the WBU_ constants.  If bRearView is set, show a
//rear view.  If label is non-NULL, print the label at the top of the
//window.

void CGenericCockpit::RenderWindow (int nWindow, CObject *viewerP, int bRearView, int nUser, const char *pszLabel)
{
	CCanvas windowCanv;
	static CCanvas overlapCanv;

	CObject*		viewerSave = gameData.objs.viewerP;
	int			nArea;
	;
	int			bRearViewSave = gameStates.render.bRearView;
	int			w, h, dx, nZoomSave;

	static int bOverlapDirty [2] = {0, 0};
	static int y, x;

if (HIDE_HUD)
	return;
hudAreaP = NULL;
if (!viewerP) {								//this nUser is done
	Assert (nUser == WBU_WEAPON || nUser == WBU_STATIC);
	if ((nUser == WBU_STATIC) && (weaponBoxUser [nWindow] != WBU_STATIC))
		staticTime [nWindow] = 0;
	if (weaponBoxUser [nWindow] == WBU_WEAPON || weaponBoxUser [nWindow] == WBU_STATIC)
		return;		//already set
	weaponBoxUser [nWindow] = nUser;
	if (bOverlapDirty [nWindow]) {
		//CCanvas::SetCurrent (&gameStates.render.vr.buffers.screenPages [gameStates.render.vr.nCurrentPage]);
		FillBackground ();
		bOverlapDirty [nWindow] = 0;
		}
	return;
	}
UpdateRenderedData (nWindow+1, viewerP, bRearView, nUser);
weaponBoxUser [nWindow] = nUser;						//say who's using window
gameData.objs.viewerP = viewerP;
gameStates.render.bRearView = bRearView;

hudAreaP = SetupWindow (nWindow);

CCanvas::Push ();
CCanvas::SetCurrent (&windowCanv);
transformation.Push ();
nZoomSave = gameStates.render.nZoomFactor;
gameStates.render.nZoomFactor = I2X (gameOpts->render.cockpit.nWindowZoom + 1);					//the CPlayerData's zoom factor
if ((nUser == WBU_RADAR_TOPDOWN) || (nUser == WBU_RADAR_HEADSUP)) {
	if (!IsMultiGame || (netGame.gameFlags & NETGAME_FLAG_SHOW_MAP))
		automap.DoFrame (0, 1 + (nUser == WBU_RADAR_TOPDOWN));
	else
		RenderFrame (0, nWindow + 1);
	}
else
	RenderFrame (0, nWindow + 1);
gameStates.render.nZoomFactor = nZoomSave;
transformation.Pop ();
//	HACK!If guided missile, wake up robots as necessary.
if (viewerP->info.nType == OBJ_WEAPON) {
	// -- Used to require to be GUIDED -- if (viewerP->id == GUIDEDMSL_ID)
	WakeupRenderedObjects (viewerP, nWindow+1);
	}
if (pszLabel) {
	fontManager.SetColorRGBi (GREEN_RGBA, 1, 0, 0);
	GrPrintF (NULL, 0x8000, 2, pszLabel);
	}
if (nUser == WBU_GUIDED) {
	DrawGuidedCrosshair ();
	}
if (gameStates.render.cockpit.nMode == CM_FULL_SCREEN) {
	int smallWindowBottom, bigWindowBottom, extraPartHeight;

	CCanvas::Current ()->SetColorRGBi (RGBA_PAL (0, 0, 32));
	OglDrawEmptyRect (0, 0, CCanvas::Current ()->Width ()-1, CCanvas::Current ()->Height ());

	//if the window only partially overlaps the big 3d window, copy
	//the extra part to the visible screen
	bigWindowBottom = gameData.render.window.y + gameData.render.window.h - 1;
	if (x > bigWindowBottom) {
		//the small window is completely outside the big 3d window, so
		//copy it to the visible screen
#if 0
		if (gameStates.render.vr.nScreenFlags & VRF_USE_PAGING)
			CCanvas::SetCurrent (&gameStates.render.vr.buffers.screenPages [!gameStates.render.vr.nCurrentPage]);
		else
			CCanvas::SetCurrent (GetCurrentGameScreen ());
#endif
		windowCanv.BlitClipped (y, x);
		bOverlapDirty [nWindow] = 1;
		}
	else {
		smallWindowBottom = x + windowCanv.Height () - 1;
		if (0 < (extraPartHeight = smallWindowBottom - bigWindowBottom)) {
			windowCanv.SetupPane (&overlapCanv, 0, windowCanv.Height ()-extraPartHeight, windowCanv.Width (), extraPartHeight);
#if 0
			if (gameStates.render.vr.nScreenFlags & VRF_USE_PAGING)
				CCanvas::SetCurrent (&gameStates.render.vr.buffers.screenPages [!gameStates.render.vr.nCurrentPage]);
			else
				CCanvas::SetCurrent (GetCurrentGameScreen ());
#endif
			overlapCanv.BlitClipped (y, bigWindowBottom+1);
			bOverlapDirty [nWindow] = 1;
			}
		}
	}
else {
	//CCanvas::SetCurrent (GetCurrentGameScreen ());
	}
//force redraw when done
oldWeapon [nWindow][gameStates.render.vr.nCurrentPage] = oldAmmoCount [nWindow][gameStates.render.vr.nCurrentPage] = -1;

abort:;

gameData.objs.viewerP = viewerSave;
CCanvas::Pop ();
gameStates.render.bRearView = bRearViewSave;
}

//------------------------------------------------------------------------------

char *ftoa (char *pszVal, fix f)
{
	int decimal, fractional;

decimal = X2I (f);
fractional = ((f & 0xffff) * 100) / 65536;
if (fractional < 0)
	fractional = -fractional;
if (fractional > 99)
	fractional = 99;
sprintf (pszVal, "%d.%02d", decimal, fractional);
return pszVal;
}

//------------------------------------------------------------------------------

fix frameTimeList [8] = {0, 0, 0, 0, 0, 0, 0, 0};
fix frameTimeTotal = 0;
int frameTimeCounter = 0;

//	-----------------------------------------------------------------------------

void CGenericCockpit::DrawFrameRate (void)
{
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
	if (automap.m_bDisplay)
		y = 2;
	else if (IsMultiGame)
		y = 7;
	else
		y = 6;
	fontManager.SetColorRGBi (ORANGE_RGBA, 1, 0, 0);
	nIdFrameRate = GrPrintF (&nIdFrameRate,
									 CCanvas::Current ()->Width () - (x * GAME_FONT->Width ()),
									 CCanvas::Current ()->Height () - y * (GAME_FONT->Height () + GAME_FONT->Height () / 4),
									 szItem);
	}
}

//------------------------------------------------------------------------------

void CGenericCockpit::ToggleCockpit (void)
{
	int nNewMode;

switch (gameStates.render.cockpit.nMode) {
	case CM_FULL_COCKPIT:
		nNewMode = CM_STATUS_BAR;
		break;

	case CM_STATUS_BAR:
		if (gameStates.render.bRearView)
			return;
		nNewMode = (gameStates.render.cockpit.nNextMode < 0) ? CM_FULL_SCREEN : CM_FULL_COCKPIT;
		break;

	case CM_FULL_SCREEN:
		if (gameStates.render.bRearView)
			return;
		nNewMode = CM_LETTERBOX;
		break;

	case CM_LETTERBOX:
		nNewMode = CM_FULL_COCKPIT;
		break;

	case CM_REAR_VIEW:
   default:
		return;			//do nothing
		break;
	}
gameStates.render.cockpit.nNextMode = -1;
SelectCockpit (nNewMode);
HUDClearMessages ();
WritePlayerFile ();
}

//	-----------------------------------------------------------------------------
