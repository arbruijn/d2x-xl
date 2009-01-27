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
#include "cockpit.h"
#include "hud_defs.h"
#include "statusbar.h"
#include "slowmotion.h"
#include "automap.h"
#include "gr.h"

//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------

void CCockpit::GetHostageWindowCoords (int& x, int& y, int& w, int& h)
{
x = SECONDARY_W_BOX_LEFT;
y = SECONDARY_W_BOX_TOP;
w = SECONDARY_W_BOX_RIGHT - SECONDARY_W_BOX_LEFT + 1;
h = SECONDARY_W_BOX_BOT - SECONDARY_W_BOX_TOP + 1;
}

//	-----------------------------------------------------------------------------

void CCockpit::DrawCountdown (void)
{
CGenericCockpit::DrawCountdown (SMALL_FONT->Height () * 4);
}

//	-----------------------------------------------------------------------------

void CHUD::DrawCruise (void)
{
CGenericCockpit::DrawCruise (3, CCanvas::Current ()->Height () - m_info.nLineSpacing * (IsMultiGame ? 11 : 6));
}

//	-----------------------------------------------------------------------------

void CCockpit::DrawOrbs (void)
{
CGenericCockpit::DrawOrbs (4 * m_info.fontWidth, 2 * m_info.nLineSpacing);
}

//	-----------------------------------------------------------------------------

void CCockpit::DrawFlag (void)
{
CGenericCockpit::DrawFlag (4 * m_info.fontWidth, 2 * m_info.nLineSpacing);
}

//	-----------------------------------------------------------------------------

void CCockpit::ClearBombCount (int bgColor)
{
}

//	-----------------------------------------------------------------------------

void CCockpit::DrawBombCount (void)
{
CGenericCockpit::DrawBombCount (BOMB_COUNT_X, BOMB_COUNT_Y, BLACK_RGBA, 1);
}

//	-----------------------------------------------------------------------------

int CCockpit::DrawBombCount (int& nIdBombCount, int x, int y, char* pszBombCount)
{
return PrintF (&nIdBombCount, x, y, pszBombCount, nIdBombCount);
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
nIdShield = PrintF (&nIdShield, NumDispX (m_info.nShields), NUMERICAL_GAUGE_Y + (gameStates.video.nDisplayMode ? 36 : 16), "%d", m_info.nShields);
}

//	-----------------------------------------------------------------------------

void CCockpit::DrawEnergy (void)
{
	static int nIdEnergy = 0;

BitBlt (GAUGE_NUMERICAL, NUMERICAL_GAUGE_X, NUMERICAL_GAUGE_Y);
fontManager.SetColorRGBi (RGBA_PAL2 (25, 18, 6), 1, 0, 0);
nIdEnergy = PrintF (&nIdEnergy, NumDispX (m_info.nEnergy), NUMERICAL_GAUGE_Y + (gameStates.video.nDisplayMode ? 5 : 2), "%d", m_info.nEnergy);
}

//	-----------------------------------------------------------------------------

void CCockpit::DrawEnergyBar (void)
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
if (m_info.nEnergy < 100) {	// erase part of gauge corresponding to energy loss
	gameStates.render.grAlpha = FADE_LEVELS;
	float fScale = float (100 - m_info.nEnergy) / 100.0f;

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
}

//	-----------------------------------------------------------------------------

ubyte afterburnerBarTable [AFTERBURNER_GAUGE_H_L * 2] = {
			3, 11,
			3, 11,
			3, 11,
			3, 11,
			3, 11,
			3, 11,
			2, 11,
			2, 10,
			2, 10,
			2, 10,
			2, 10,
			2, 10,
			2, 10,
			1, 10,
			1, 10,
			1, 10,
			1, 9,
			1, 9,
			1, 9,
			1, 9,
			0, 9,
			0, 9,
			0, 8,
			0, 8,
			0, 8,
			0, 8,
			1, 8,
			2, 8,
			3, 8,
			4, 8,
			5, 8,
			6, 7,
};

ubyte afterburnerBarTableHires [AFTERBURNER_GAUGE_H_H*2] = {
	5, 20,
	5, 20,
	5, 19,
	5, 19,
	5, 19,
	5, 19,
	4, 19,
	4, 19,
	4, 19,
	4, 19,

	4, 19,
	4, 18,
	4, 18,
	4, 18,
	4, 18,
	3, 18,
	3, 18,
	3, 18,
	3, 18,
	3, 18,

	3, 18,
	3, 17,
	3, 17,
	2, 17,
	2, 17,
	2, 17,
	2, 17,
	2, 17,
	2, 17,
	2, 17,

	2, 17,
	2, 16,
	2, 16,
	1, 16,
	1, 16,
	1, 16,
	1, 16,
	1, 16,
	1, 16,
	1, 16,

	1, 16,
	1, 15,
	1, 15,
	1, 15,
	0, 15,
	0, 15,
	0, 15,
	0, 15,
	0, 15,
	0, 15,

	0, 14,
	0, 14,
	0, 14,
	1, 14,
	2, 14,
	3, 14,
	4, 14,
	5, 14,
	6, 13,
	7, 13,

	8, 13,
	9, 13,
	10, 13,
	11, 13,
	12, 13
};

//	-----------------------------------------------------------------------------

void CCockpit::DrawAfterburnerBar (void)
{
	int		x [4], y [4], yMax;
	ubyte*	tableP = gameStates.video.nDisplayMode ? afterburnerBarTableHires : afterburnerBarTable;

BitBlt (GAUGE_AFTERBURNER, AFTERBURNER_GAUGE_X, AFTERBURNER_GAUGE_Y);
CCanvas::Current ()->SetColorRGB (0, 0, 0, 255);
if ((yMax = FixMul (I2X (1) - gameData.physics.xAfterburnerCharge, AFTERBURNER_GAUGE_H))) {
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

void CCockpit::DrawShieldBar (void)
{
if (m_info.tInvul <= 0)
	BitBlt (GAUGE_SHIELDS + 9 - ((m_info.nShields >= 100) ? 9 : (m_info.nShields / 10)), SHIELD_GAUGE_X, SHIELD_GAUGE_Y);
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
	CGenericCockpit::DrawWeaponInfo (nIndex,
		hudWindowAreas + COCKPIT_PRIMARY_BOX,
		PRIMARY_W_PIC_X, PRIMARY_W_PIC_Y,
		PRIMARY_WEAPON_NAMES_SHORT (nWeaponId),
		PRIMARY_W_TEXT_X, PRIMARY_W_TEXT_Y, 0);
		}
else {
	nIndex = secondaryWeaponToWeaponInfo [nWeaponId];
	CGenericCockpit::DrawWeaponInfo (nIndex,
		hudWindowAreas + COCKPIT_SECONDARY_BOX,
		SECONDARY_W_PIC_X, SECONDARY_W_PIC_Y,
		SECONDARY_WEAPON_NAMES_SHORT (nWeaponId),
		SECONDARY_W_TEXT_X, SECONDARY_W_TEXT_Y, 0);
	}
}

//	-----------------------------------------------------------------------------

void CCockpit::DrawKillList (void)
{
CGenericCockpit::DrawKillList (53, CCanvas::Current ()->Height () - HUD_LHX (6));
}

//	-----------------------------------------------------------------------------

void CStatusBar::DrawStatic (int nWindow)
{
CGenericCockpit::DrawStatic (nWindow, COCKPIT_PRIMARY_BOX);
}

//	-----------------------------------------------------------------------------

void CStatusBar::DrawPlayerShip (void)
{
CGenericCockpit::DrawPlayerShip (m_info.bCloak, m_history [gameStates.render.vr.nCurrentPage].bCloak, SHIP_GAUGE_X, SHIP_GAUGE_Y);
}

//	-----------------------------------------------------------------------------

//	Draws invulnerable ship, or maybe the flashing ship, depending on invulnerability time left.
void CCockpit::DrawInvul (void)
{
	static fix time = 0;

if ((m_info.tInvul > I2X (4)) || ((m_info.tInvul > 0) && (gameData.time.xGame & 0x8000))) {
	BitBlt (GAUGE_INVULNERABLE + m_info.nInvulnerableFrame, SHIELD_GAUGE_X, SHIELD_GAUGE_Y);
	time += gameData.time.xFrame;
	while (time > INV_FRAME_TIME) {
		time -= INV_FRAME_TIME;
		if (++m_info.nInvulnerableFrame == N_INVULNERABLE_FRAMES)
			m_info.nInvulnerableFrame = 0;
		}
	}
}

//	-----------------------------------------------------------------------------

void CCockpit::DrawCockpit (bool bAlphaTest)
{
CGenericCockpit::DrawCockpit (m_info.nCockpit, 0, bAlphaTest);
}

//	-----------------------------------------------------------------------------

void CCockpit::SetupWindow (int nWindow, CCanvas* canvP)
{
tGaugeBox* hudAreaP = hudWindowAreas + COCKPIT_PRIMARY_BOX + nWindow;
gameStates.render.vr.buffers.render->SetupPane (
	canvP,
	HUD_SCALE_X (hudAreaP->left),
	HUD_SCALE_Y (hudAreaP->top),
	HUD_SCALE_X (hudAreaP->right - hudAreaP->left+1),
	HUD_SCALE_Y (hudAreaP->bot - hudAreaP->top+1));
}

//	-----------------------------------------------------------------------------

bool CCockpit::Setup (void)
{
if (!CGenericCockpit::Setup ())
	return false;
gameData.render.window.hMax = (screen.Height () * 2) / 3;
if (gameData.render.window.h > gameData.render.window.hMax)
	gameData.render.window.h = gameData.render.window.hMax;
if (gameData.render.window.w > gameData.render.window.wMax)
	gameData.render.window.w = gameData.render.window.wMax;
gameData.render.window.x = (gameData.render.window.wMax - gameData.render.window.w)/2;
gameData.render.window.y = (gameData.render.window.hMax - gameData.render.window.h)/2;
GameInitRenderSubBuffers (gameData.render.window.x, gameData.render.window.y, gameData.render.window.w, gameData.render.window.h);
return true;
}

//	-----------------------------------------------------------------------------

void CCockpit::Toggle (void)
{
CGenericCockpit::Toggle (CM_STATUS_BAR);
}

//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------

bool CRearView::Setup (void)
{
if (!CGenericCockpit::Setup ())
	return false;
gameData.render.window.hMax = (screen.Height () * 2) / 3;
if (gameData.render.window.h > gameData.render.window.hMax)
	gameData.render.window.h = gameData.render.window.hMax;
if (gameData.render.window.w > gameData.render.window.wMax)
	gameData.render.window.w = gameData.render.window.wMax;
gameData.render.window.x = (gameData.render.window.wMax - gameData.render.window.w)/2;
gameData.render.window.y = (gameData.render.window.hMax - gameData.render.window.h)/2;
GameInitRenderSubBuffers (gameData.render.window.x, gameData.render.window.y, gameData.render.window.w, gameData.render.window.h);
return true;
}

//	-----------------------------------------------------------------------------

