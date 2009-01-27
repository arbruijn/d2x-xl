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

CCockpit cockpit;

//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------
//fills in the coords of the hostage video window
void GetHostageWindowCoords (int& x, int& y, int& w, int& h)
{
x = SECONDARY_W_BOX_LEFT;
y = SECONDARY_W_BOX_TOP;
w = SECONDARY_W_BOX_RIGHT - SECONDARY_W_BOX_LEFT + 1;
h = SECONDARY_W_BOX_BOT - SECONDARY_W_BOX_TOP + 1;
}

//	-----------------------------------------------------------------------------

void CFullScreenCockpit::ShowScore (void)
{
	char	szScore [40];
	int	w, h, aw;

if (HIDE_HUD)
	return;
if ((gameData.hud.msgs [0].nMessages > 0) &&
	 (strlen (gameData.hud.msgs [0].szMsgs [gameData.hud.msgs [0].nFirst]) > 38))
	return;
fontManager.SetCurrent (GAME_FONT);
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
if ((IsMultiGame && !IsCoopGame))
	sprintf (szScore + 18, "   %s: %5d", TXT_KILLS, LOCALPLAYER.netKillsTotal);
else
	sprintf (szScore + 18, "   %s: %5d", TXT_SCORE, LOCALPLAYER.score);
fontManager.Current ()->StringSize (szScore, w, h, aw);
fontManager.SetColorRGBi (GREEN_RGBA, 1, 0, 0);
GrPrintF (NULL, CCanvas::Current ()->Width ()-w-HUD_LHX (2), 3, szScore);
}

//	-----------------------------------------------------------------------------

void CFullScreenCockpit::ShowTimerCount (void)
 {
	char	szScore [20];
	int	w, h, aw, i;
	fix timevar = 0;

	static int nIdTimer = 0;

if (HIDE_HUD)
	return;
if ((gameData.hud.msgs [0].nMessages > 0) && (strlen (gameData.hud.msgs [0].szMsgs [gameData.hud.msgs [0].nFirst]) > 38))
	return;

if ((gameData.app.nGameMode & GM_NETWORK) && netGame.xPlayTimeAllowed) {
	timevar = I2X (netGame.xPlayTimeAllowed*5*60);
	i = X2I (timevar-gameStates.app.xThisLevelTime);
	i++;
	sprintf (szScore, "T - %5d", i);
	fontManager.Current ()->StringSize (szScore, w, h, aw);
	fontManager.SetColorRGBi (GREEN_RGBA, 1, 0, 0);
	if ((i >= 0) && !gameData.reactor.bDestroyed)
		nIdTimer = GrPrintF (&nIdTimer, CCanvas::Current ()->Width ()-w-HUD_LHX (10), HUD_LHX (11), szScore);
	}
}

//	-----------------------------------------------------------------------------
//y offset between lines on HUD

 void CFullScreenCockpit::ShowScoreAdded (void)
{
	int	color;
	int	w, h, aw;
	char	szScore [20];

	static int nIdTotalScore = 0;

if (HIDE_HUD)
	return;
if (IsMultiGame && !IsCoopGame)
	return;
if (scoreDisplay [0] == 0)
	return;
fontManager.SetCurrent (GAME_FONT);
scoreTime -= gameData.time.xFrame;
if (scoreTime > 0) {
	color = X2I (scoreTime * 20) + 12;
	if (color < 10)
		color = 12;
	else if (color > 31)
		color = 30;
	color = color - (color % 4);	//	Only allowing colors 12, 16, 20, 24, 28 speeds up gr_getcolor, improves caching
	if (gameStates.app.cheats.bEnabled)
		sprintf (szScore, "%s", TXT_CHEATER);
	else
		sprintf (szScore, "%5d", scoreDisplay [0]);
	fontManager.Current ()->StringSize (szScore, w, h, aw);
	fontManager.SetColorRGBi (RGBA_PAL2 (0, color, 0), 1, 0, 0);
	nIdTotalScore = GrPrintF (&nIdTotalScore, CCanvas::Current ()->Width ()-w-HUD_LHX (2+10), nLineSpacing+4, szScore);
	}
else {
	scoreTime = 0;
	scoreDisplay [0] = 0;
	}
}

//	-----------------------------------------------------------------------------

void DrawHomingWarning (void)
{
if ((gameStates.render.cockpit.nMode == CM_STATUS_BAR) || gameStates.app.bEndLevelSequence)
	SBShowHomingWarning ();
else {
	CCanvas::SetCurrent (GetCurrentGameScreen ());
	if (LOCALPLAYER.homingObjectDist >= 0) {
		if (gameData.time.xGame & 0x4000) {
			BitBlt (GAUGE_HOMING_WARNING_ON, HOMING_WARNING_X, HOMING_WARNING_Y);
			bLastHomingWarningShown [gameStates.render.vr.nCurrentPage] = 1;
			}
		else {
			BitBlt (GAUGE_HOMING_WARNING_OFF, HOMING_WARNING_X, HOMING_WARNING_Y);
			bLastHomingWarningShown [gameStates.render.vr.nCurrentPage] = 0;
			}
		}
	else {
		BitBlt (GAUGE_HOMING_WARNING_OFF, HOMING_WARNING_X, HOMING_WARNING_Y);
		bLastHomingWarningShown [gameStates.render.vr.nCurrentPage] = 0;
		}
	}
}

//	-----------------------------------------------------------------------------

extern int SW_y [2];

void CFullScreenCockpit::DrawHomingWarning (void)
{
	static int nIdLock = 0;

if (HIDE_HUD)
	return;
if (LOCALPLAYER.homingObjectDist >= 0) {
	if (gameData.time.xGame & 0x4000) {
		int x = 0x8000, y = CCanvas::Current ()->Height ()-nLineSpacing;
		if ((weaponBoxUser [0] != WBU_WEAPON) || (weaponBoxUser [1] != WBU_WEAPON)) {
			int wy = (weaponBoxUser [0] != WBU_WEAPON) ? SW_y [0] : SW_y [1];
			y = min (y, (wy - nLineSpacing - gameData.render.window.y));
			}
		fontManager.SetCurrent (GAME_FONT);
		fontManager.SetColorRGBi (GREEN_RGBA, 1, 0, 0);
		nIdLock = GrPrintF (&nIdLock, x, y, TXT_LOCK);
		}
	}
}

//	-----------------------------------------------------------------------------

void CCockpit::DrawOrbs (void)
{
DrawOrbs (4 * m_info.fontWidth, 2 * m_info.nLineSpacing);
}

//	-----------------------------------------------------------------------------

void CFullScreenCockpit::DrawFlag (void)
{
DrawFlag (4 * m_info.fontWidth, 2 * m_info.nLineSpacing);
}

//	-----------------------------------------------------------------------------

void CCockpit::ClearBombCount (void)
{
}

//	-----------------------------------------------------------------------------

int CCockpit::DrawBombCount (int* nId, int x, int y, char* pszBombCount)
{
CCanvas::Current ()->SetColorRGBi (bgColor);
return Print (&nIdBombCount, x, y, szBombCount, nIdBombCount);
}

//	-----------------------------------------------------------------------------

void CCockpit::DrawPrimaryAmmoInfo (int ammoCount)
{
DrawAmmoInfo (PRIMARY_AMMO_X, PRIMARY_AMMO_Y, ammoCount, 1);
}

//	-----------------------------------------------------------------------------

void CCockpit::DrawSecondaryAmmoInfo (int ammoCount)
{
DrawAmmoInfo (SECONDARY_AMMO_X, SECONDARY_AMMO_Y, ammoCount, 0);
}

//	-----------------------------------------------------------------------------

static inline int NumDispX (int val)
{
int x = ((val > 99) ? 7 : (val > 9) ? 11 : 15);
if (!gameStates.video.nDisplayMode)
	x /= 2;
return x + NUMERICAL_GAUGE_X;
}

//	-----------------------------------------------------------------------------

void CCockpit::DrawShield (void)
{
	static int nIdShield = 0;

BitBlt (GAUGE_NUMERICAL, NUMERICAL_GAUGE_X, NUMERICAL_GAUGE_Y);
fontManager.SetColorRGBi (RGBA_PAL2 (14, 14, 23), 1, 0, 0);
nIdShields = PrintF (&nIdShields, NumDispX (shield), NUMERICAL_GAUGE_Y + (gameStates.video.nDisplayMode ? 36 : 16), "%d", shield);
}

//	-----------------------------------------------------------------------------

void CCockpit::DrawEnergy (void)
{
	static int nIdEnergy = 0;

BitBlt (GAUGE_NUMERICAL, NUMERICAL_GAUGE_X, NUMERICAL_GAUGE_Y);
fontManager.SetColorRGBi (RGBA_PAL2 (25, 18, 6), 1, 0, 0);
nIdEnergy = PrintF (&nIdEnergy, NumDispX (energy), NUMERICAL_GAUGE_Y + (gameStates.video.nDisplayMode ? 5 : 2), "%d", energy);
}

//	-----------------------------------------------------------------------------

void CCockpit::DrawEnergyBar (int nEnergy)
{
// values taken directly from the bitmap
#define ENERGY_GAUGE_TOP_LEFT		20
#define ENERGY_GAUGE_BOT_LEFT		0
#define ENERGY_GAUGE_BOT_WIDTH	126

BitBlt (GAUGE_ENERGY_LEFT, LEFT_ENERGY_GAUGE_X, LEFT_ENERGY_GAUGE_Y);
BitBlt (GAUGE_ENERGY_RIGHT, RIGHT_ENERGY_GAUGE_X, RIGHT_ENERGY_GAUGE_Y);
#if DBG
CCanvas::Current ()->SetColorRGBi (RGBA_PAL (255, 255, 255));
#else
CCanvas::Current ()->SetColorRGBi (RGBA_PAL (0, 0, 0));
#endif
if (nEnergy < 100) {	// erase part of gauge corresponding to energy loss
	gameStates.render.grAlpha = FADE_LEVELS;
	float fScale = float (100 - nEnergy) / 100.0f;

	{
	int x [4] = {ENERGY_GAUGE_TOP_LEFT, LEFT_ENERGY_GAUGE_W, ENERGY_GAUGE_BOT_LEFT + ENERGY_GAUGE_BOT_WIDTH, ENERGY_GAUGE_BOT_LEFT};
	int y [4] = {0, 0, LEFT_ENERGY_GAUGE_H, LEFT_ENERGY_GAUGE_H};

	x [1] = x [0] + int (fScale * (x [1] - x [0]));
	x [2] = x [3] + int (fScale * (x [2] - x [3]));
	for (int i = 0; i < 4; i++) {
		x [i] = HUD_SCALE_X (LEFT_ENERGY_GAUGE_X + x [i]);
		y [i] = HUD_SCALE_Y (LEFT_ENERGY_GAUGE_Y + y [i]);
		}
	OglDrawFilledPoly (x, y, 4);
	}

	{
	int x [4] = {0, LEFT_ENERGY_GAUGE_W - ENERGY_GAUGE_TOP_LEFT, LEFT_ENERGY_GAUGE_W - ENERGY_GAUGE_BOT_LEFT, LEFT_ENERGY_GAUGE_W - ENERGY_GAUGE_BOT_WIDTH};
	int y [4] = {0, 0, LEFT_ENERGY_GAUGE_H, LEFT_ENERGY_GAUGE_H};

	x [0] = x [1] - int (fScale * (x [1] - x [0]));
	x [3] = x [2] - int (fScale * (x [2] - x [3]));
	for (int i = 0; i < 4; i++) {
		x [i] = HUD_SCALE_X (RIGHT_ENERGY_GAUGE_X + x [i]);
		y [i] = HUD_SCALE_Y (RIGHT_ENERGY_GAUGE_Y + y [i]);
		}
	OglDrawFilledPoly (x, y, 4);
	}

	}
CCanvas::SetCurrent (GetCurrentGameScreen ());
}

//	-----------------------------------------------------------------------------

void CCockpit::DrawAfterburnerBar (int nEnergy)
{
	int		x [4], y [4], yMax;
	ubyte*	tableP = gameStates.video.nDisplayMode ? afterburnerBarTableHires : afterburnerBarTable;

BitBlt (GAUGE_AFTERBURNER, AFTERBURNER_GAUGE_X, AFTERBURNER_GAUGE_Y);
CCanvas::Current ()->SetColorRGB (0, 0, 0, 255);
if ((yMax = FixMul (I2X (1) - nEnergy, AFTERBURNER_GAUGE_H))) {
	y [0] = y [1] = HUD_SCALE_Y (AFTERBURNER_GAUGE_Y);
	y [3] = HUD_SCALE_Y (AFTERBURNER_GAUGE_Y + yMax) - 1;
	x [1] = HUD_SCALE_X (AFTERBURNER_GAUGE_X + tableP [0]);
	x [0] = HUD_SCALE_X (AFTERBURNER_GAUGE_X + tableP [1] + 1);
	x [2] = x [1];
	y [2] = 0;
	for (int i = 1; i < yMax - 1; i++)
		if (x [2] >= tableP [2 * i]) {
			x [2] = tableP [2 * i];
			y [2] = i;
			}
	x [2] = HUD_SCALE_X (AFTERBURNER_GAUGE_X + x [2] + 1);
	y [2] = HUD_SCALE_Y (AFTERBURNER_GAUGE_Y + y [2]);
	x [3] = HUD_SCALE_X (AFTERBURNER_GAUGE_X + tableP [2 * yMax - 1] + 1);
	gameStates.render.grAlpha = FADE_LEVELS;
	OglDrawFilledPoly (x, y, 4);
	}
//CCanvas::SetCurrent (GetCurrentGameScreen ());
}

//	-----------------------------------------------------------------------------

void CCockpit::DrawShieldBar (int shield)
{
BitBlt (GAUGE_SHIELDS + 9 - ((shield >= 100) ? 9 : (shield / 10)), SHIELD_GAUGE_X, SHIELD_GAUGE_Y);
}

//	-----------------------------------------------------------------------------

typedef struct tKeyGaugeInfo {
	int	nFlag, nGaugeOn, nGaugeOff, x [2], y [2];
} tKeyGaugeInfo;

static tKeyGaugeInfo keyGaugeInfo [] = {
	{PLAYER_FLAGS_BLUE_KEY, GAUGE_BLUE_KEY, GAUGE_BLUE_KEY_OFF, {GAUGE_BLUE_KEY_X_L, GAUGE_BLUE_KEY_X_H}, {GAUGE_BLUE_KEY_Y_L, GAUGE_BLUE_KEY_Y_H}},
	{PLAYER_FLAGS_GOLD_KEY, GAUGE_GOLD_KEY, GAUGE_GOLD_KEY_OFF, {GAUGE_GOLD_KEY_X_L, GAUGE_GOLD_KEY_X_H}, {GAUGE_GOLD_KEY_Y_L, GAUGE_GOLD_KEY_Y_H}},
	{PLAYER_FLAGS_RED_KEY, GAUGE_RED_KEY, GAUGE_RED_KEY_OFF, {GAUGE_RED_KEY_X_L, GAUGE_RED_KEY_X_H}, {GAUGE_RED_KEY_Y_L, GAUGE_RED_KEY_Y_H}},
	};

void CCockpit::DrawKeys (void)
{
CCanvas::SetCurrent (GetCurrentGameScreen ());
int bHires = gameStates.video.nDisplayMode != 0;
for (int i = 0; i < 3; i++)
	BitBlt ((LOCALPLAYER.flags & keyGaugeInfo [i].nFlag) ? keyGaugeInfo [i].nGaugeOn : keyGaugeInfo [i].nGaugeOff, keyGaugeInfo [i].x [bHires], keyGaugeInfo [i].y [bHires]);
}

//	-----------------------------------------------------------------------------

void CCockpit::DrawWeaponInfo (int nWeaponType, int nWeaponId, int laserLevel)
{
	int nIndex;

if (nWeaponType == 0) {
	nIndex = primaryWeaponToWeaponInfo [nWeaponId];
	if (nIndex == LASER_ID && laserLevel > MAX_LASER_LEVEL)
		nIndex = SUPERLASER_ID;
	DrawWeaponInfo (nIndex,
		hudWindowAreas + COCKPIT_PRIMARY_BOX,
		PRIMARY_W_PIC_X, PRIMARY_W_PIC_Y,
		PRIMARY_WEAPON_NAMES_SHORT (nWeaponId),
		PRIMARY_W_TEXT_X, PRIMARY_W_TEXT_Y, 0);
		}
else {
	nIndex = secondaryWeaponToWeaponInfo [nWeaponId];
	DrawWeaponInfo (nIndex,
		hudWindowAreas + COCKPIT_SECONDARY_BOX,
		SECONDARY_W_PIC_X, SECONDARY_W_PIC_Y,
		SECONDARY_WEAPON_NAMES_SHORT (nWeaponId),
		SECONDARY_W_TEXT_X, SECONDARY_W_TEXT_Y, 0);
	}
}

//	-----------------------------------------------------------------------------

void CStatusBar::DrawKillList (void)
{
DrawKillList (53, CCanvas::Current ()->Height () - HUD_LHX (6));
}

//	-----------------------------------------------------------------------------

void CStatusBar::DrawStatic (int nWindow)
{
DrawStatic (nWindow, COCKPIT_PRIMARY_BOX);
}

//	-----------------------------------------------------------------------------

//	Draws invulnerable ship, or maybe the flashing ship, depending on invulnerability time left.
void CCockpit::DrawInvulnerableShip (void)
{
	static fix time = 0;
	fix tInvul = LOCALPLAYER.invulnerableTime + INVULNERABLE_TIME_MAX - gameData.time.xGame;

if (tInvul <= 0) 
	DrawShieldBar (X2IR (LOCALPLAYER.shields));
else if ((tInvul > I2X (4)) || (gameData.time.xGame & 0x8000)) {
	BitBlt (GAUGE_INVULNERABLE + nInvulnerableFrame, SHIELD_GAUGE_X, SHIELD_GAUGE_Y);
	time += gameData.time.xFrame;
	while (time > INV_FRAME_TIME) {
		time -= INV_FRAME_TIME;
		if (++nInvulnerableFrame == N_INVULNERABLE_FRAMES)
			nInvulnerableFrame = 0;
		}
	}
}

//	---------------------------------------------------------------------------------------------------------

void DrawCockpit (bool bAlphaTest)
{
DrawCockpit (nCockpit, 0, bAlphaTest);
}

//	---------------------------------------------------------------------------------------------------------
//	Call when picked up a laser powerup.
//	If laser is active, set oldWeapon [0] to -1 to force redraw.
void UpdateLaserWeaponInfo (void)
{
	if (oldWeapon [0][gameStates.render.vr.nCurrentPage] == 0)
		if (!(LOCALPLAYER.laserLevel > MAX_LASER_LEVEL && oldLaserLevel [gameStates.render.vr.nCurrentPage] <= MAX_LASER_LEVEL))
			oldWeapon [0][gameStates.render.vr.nCurrentPage] = -1;
}

void FillBackground (void);

int SW_drawn [2], SW_x [2], SW_y [2], SW_w [2], SW_h [2];

//	---------------------------------------------------------------------------------------------------------

void CCockpit::SetupWindow (int nWindow)
{
nArea = COCKPIT_PRIMARY_BOX + nWindow;
	tGaugeBox* hudAreaP = hudWindowAreas + nArea;
	gameStates.render.vr.buffers.render->SetupPane (
		&windowCanv,
		HUD_SCALE_X (hudAreaP->left),
		HUD_SCALE_Y (hudAreaP->top),
		HUD_SCALE_X (hudAreaP->right - hudAreaP->left+1),
		HUD_SCALE_Y (hudAreaP->bot - hudAreaP->top+1));
	}
}

//	-----------------------------------------------------------------------------
