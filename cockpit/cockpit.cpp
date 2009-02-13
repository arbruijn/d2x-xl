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

void CCockpit::DrawRecording (void)
{
CGenericCockpit::DrawRecording ((CCanvas::Current ()->Height () > 240) ? 80 : 30);
}

//	-----------------------------------------------------------------------------

void CCockpit::DrawCountdown (void)
{
CGenericCockpit::DrawCountdown (SMALL_FONT->Height () * 4);
}

//	-----------------------------------------------------------------------------

void CCockpit::DrawCruise (void)
{
CGenericCockpit::DrawCruise (3, CCanvas::Current ()->Height () - m_info.nLineSpacing * (IsMultiGame ? 11 : 6));
}

//	-----------------------------------------------------------------------------

void CCockpit::DrawLives (void)
{
hudCockpit.DrawLives ();
}

//	-----------------------------------------------------------------------------

void CCockpit::DrawScore (void)
{
hudCockpit.DrawScore ();
}

//	-----------------------------------------------------------------------------

void CCockpit::DrawScoreAdded (void)
{
hudCockpit.DrawScoreAdded ();
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

void CCockpit::DrawHomingWarning (void)
{
m_info.bLastHomingWarningDrawn [gameStates.render.vr.nCurrentPage] = (LOCALPLAYER.homingObjectDist >= 0) && (gameData.time.xGame & 0x4000);
BitBlt (m_info.bLastHomingWarningDrawn [gameStates.render.vr.nCurrentPage] ? GAUGE_HOMING_WARNING_ON : GAUGE_HOMING_WARNING_OFF, 
		  HOMING_WARNING_X, HOMING_WARNING_Y);
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

int CCockpit::DrawBombCount (int& nIdBombCount, int x, int y, int nColor, char* pszBombCount)
{
CCanvas::Push ();
CCanvas::SetCurrent (CurrentGameScreen ());
fontManager.SetColorRGBi (nColor, 1, 0, 1);
int i = PrintF (&nIdBombCount, -(ScaleX (x) + WidthPad (pszBombCount)), -(ScaleY (y) + m_info.heightPad), pszBombCount, nIdBombCount);
CCanvas::Pop ();
return i;
}

//	-----------------------------------------------------------------------------

void CCockpit::DrawPrimaryAmmoInfo (int ammoCount)
{
CCanvas::Push ();
CCanvas::SetCurrent (CurrentGameScreen ());
DrawAmmoInfo (PRIMARY_AMMO_X, PRIMARY_AMMO_Y, ammoCount, 1);
CCanvas::Pop ();
}

//	-----------------------------------------------------------------------------

void CCockpit::DrawSecondaryAmmoInfo (int ammoCount)
{
CCanvas::Push ();
CCanvas::SetCurrent (CurrentGameScreen ());
DrawAmmoInfo (SECONDARY_AMMO_X, SECONDARY_AMMO_Y, ammoCount, 0);
CCanvas::Pop ();
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

	char szShield [20];

#if 0
CBitmap* bmP = BitBlt (GAUGE_NUMERICAL, NUMERICAL_GAUGE_X, NUMERICAL_GAUGE_Y);
#else
PageInGauge (GAUGE_NUMERICAL);
CBitmap* bmP = gameData.pig.tex.bitmaps [0] + GaugeIndex (GAUGE_NUMERICAL);
#endif
CCanvas::Push ();
CCanvas::SetCurrent (CurrentGameScreen ());
fontManager.SetColorRGBi (RGBA_PAL2 (14, 14, 23), 1, 0, 0);
sprintf (szShield, "%d", m_info.nShields);
int w, h, aw;
fontManager.Current ()->StringSize (szShield, w, h, aw);
nIdShield = PrintF (&nIdShield, -(ScaleX (NUMERICAL_GAUGE_X + bmP->Width () / 2) - w / 2), 
						  NUMERICAL_GAUGE_Y + (gameStates.video.nDisplayMode ? 36 : 16) + m_info.heightPad, szShield);
CCanvas::Pop ();
}

//	-----------------------------------------------------------------------------

void CCockpit::DrawEnergy (void)
{
	static int nIdEnergy = 0;

	char szEnergy [20];

#if 0
CBitmap* bmP = BitBlt (GAUGE_NUMERICAL, NUMERICAL_GAUGE_X, NUMERICAL_GAUGE_Y);
#else
PageInGauge (GAUGE_NUMERICAL);
CBitmap* bmP = gameData.pig.tex.bitmaps [0] + GaugeIndex (GAUGE_NUMERICAL);
#endif
CCanvas::Push ();
CCanvas::SetCurrent (CurrentGameScreen ());
fontManager.SetColorRGBi (RGBA_PAL2 (25, 18, 6), 1, 0, 0);
sprintf (szEnergy, "%d", m_info.nEnergy);
int w, h, aw;
fontManager.Current ()->StringSize (szEnergy, w, h, aw);
nIdEnergy = PrintF (&nIdEnergy,-(ScaleX (NUMERICAL_GAUGE_X + bmP->Width () / 2) - w / 2), 
						  NUMERICAL_GAUGE_Y + (gameStates.video.nDisplayMode ? 5 : 2), szEnergy);
CCanvas::Pop ();
}

//	-----------------------------------------------------------------------------

void CCockpit::DrawEnergyBar (void)
{
// values taken directly from the bitmap
#define ENERGY_GAUGE_TOP_LEFT		20
#define ENERGY_GAUGE_BOT_LEFT		0
#define ENERGY_GAUGE_BOT_WIDTH	126

if (m_info.nEnergy) {	
	BitBlt (GAUGE_ENERGY_LEFT, LEFT_ENERGY_GAUGE_X, LEFT_ENERGY_GAUGE_Y);
	BitBlt (GAUGE_ENERGY_RIGHT, RIGHT_ENERGY_GAUGE_X, RIGHT_ENERGY_GAUGE_Y);
	if (m_info.nEnergy < 100) {	// erase part of gauge corresponding to energy loss
		float fScale = float (100 - m_info.nEnergy) / 100.0f;
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			{
			int x [4] = {ENERGY_GAUGE_TOP_LEFT, LEFT_ENERGY_GAUGE_W, ENERGY_GAUGE_BOT_LEFT + ENERGY_GAUGE_BOT_WIDTH, ENERGY_GAUGE_BOT_LEFT};
			int y [4] = {0, 0, LEFT_ENERGY_GAUGE_H, LEFT_ENERGY_GAUGE_H};

			x [1] = x [0] + int (fScale * (x [1] - x [0]));
			x [2] = x [3] + int (fScale * (x [2] - x [3]));
			for (int i = 0; i < 4; i++) {
				x [i] = ScaleX (LEFT_ENERGY_GAUGE_X + x [i]);
				y [i] = ScaleY (LEFT_ENERGY_GAUGE_Y + y [i]);
				}
			OglDrawFilledPoly (x, y, 4, gaugeFadeColors [0], 1);
#if 0
			x [0] = x [1];
			x [3] = x [2];
			x [1] += (ScaleX (LEFT_ENERGY_GAUGE_X + LEFT_ENERGY_GAUGE_W) - x [1]) / 2;
			x [2] += (ScaleX (LEFT_ENERGY_GAUGE_X + ENERGY_GAUGE_BOT_LEFT + ENERGY_GAUGE_BOT_WIDTH) - x [2]) / 2;
			glEnable (GL_BLEND);
			OglDrawFilledPoly (x, y, 4, gaugeFadeColors [0], 4);
#endif
			}

			{
			int x [4] = {0, LEFT_ENERGY_GAUGE_W - ENERGY_GAUGE_TOP_LEFT, LEFT_ENERGY_GAUGE_W - ENERGY_GAUGE_BOT_LEFT, LEFT_ENERGY_GAUGE_W - ENERGY_GAUGE_BOT_WIDTH};
			int y [4] = {0, 0, LEFT_ENERGY_GAUGE_H, LEFT_ENERGY_GAUGE_H};

			x [0] = x [1] - int (fScale * (x [1] - x [0]));
			x [3] = x [2] - int (fScale * (x [2] - x [3]));
			for (int i = 0; i < 4; i++) {
				x [i] = ScaleX (RIGHT_ENERGY_GAUGE_X + x [i]);
				y [i] = ScaleY (RIGHT_ENERGY_GAUGE_Y + y [i]);
				}
			OglDrawFilledPoly (x, y, 4, gaugeFadeColors [0], 1);
#if 0
			x [1] = x [0];
			x [2] = x [3];
			x [0] = ScaleX (RIGHT_ENERGY_GAUGE_X);
			x [0] += (x [1] - x [0]) / 2;
			x [3] = ScaleX (RIGHT_ENERGY_GAUGE_X + LEFT_ENERGY_GAUGE_W - ENERGY_GAUGE_BOT_WIDTH);
			x [3] += (x [2] - x [3]) / 2;
			glEnable (GL_BLEND);
			OglDrawFilledPoly (x, y, 4, gaugeFadeColors [1], 4);
#endif
			}
		glDisable (GL_BLEND);
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
#if 1
if (!(LOCALPLAYER.flags & PLAYER_FLAGS_AFTERBURNER))
	return;		//don't draw if don't have
if (!gameData.physics.xAfterburnerCharge)
	return;
#endif
//CCanvas::Current ()->SetColorRGB (255, 255, 255, 255);
BitBlt (GAUGE_AFTERBURNER, AFTERBURNER_GAUGE_X, AFTERBURNER_GAUGE_Y);
int yMax = FixMul (I2X (1) - gameData.physics.xAfterburnerCharge, AFTERBURNER_GAUGE_H);
if (yMax) {
	int		x [4], y [4];
	ubyte*	tableP = gameStates.video.nDisplayMode ? afterburnerBarTableHires : afterburnerBarTable;

	y [0] = y [1] = ScaleY (AFTERBURNER_GAUGE_Y);
	y [3] = ScaleY (AFTERBURNER_GAUGE_Y + yMax) - 1;
	x [1] = ScaleX (AFTERBURNER_GAUGE_X + tableP [0]);
	x [0] = ScaleX (AFTERBURNER_GAUGE_X + tableP [1] + 1);
	x [2] = x [1];
	y [2] = 0;
	for (int i = 1; i < yMax - 1; i++)
		if (x [2] >= tableP [2 * i]) {
			x [2] = tableP [2 * i];
			y [2] = i;
			}
	x [2] = ScaleX (AFTERBURNER_GAUGE_X + x [2] + 1);
	y [2] = ScaleY (AFTERBURNER_GAUGE_Y + y [2]);
	x [3] = ScaleX (AFTERBURNER_GAUGE_X + tableP [2 * yMax - 1] + 1);
	gameStates.render.grAlpha = 1.0f;
	glEnable (GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	OglDrawFilledPoly (x, y, 4, gaugeFadeColors [0], 1);
	glDisable (GL_BLEND);
	}
}

//	-----------------------------------------------------------------------------

void CCockpit::DrawShieldBar (void)
{
if (!(LOCALPLAYER.flags & PLAYER_FLAGS_INVULNERABLE) || (m_info.tInvul <= 0))
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
int bHires = gameStates.video.nDisplayMode != 0;
for (int i = 0; i < 3; i++)
	BitBlt ((LOCALPLAYER.flags & keyGaugeInfo [i].nFlag) ? keyGaugeInfo [i].nGaugeOn : keyGaugeInfo [i].nGaugeOff, keyGaugeInfo [i].x [bHires], keyGaugeInfo [i].y [bHires]);
}

//	-----------------------------------------------------------------------------

void CCockpit::DrawWeaponInfo (int nWeaponType, int nWeaponId, int laserLevel)
{
	int nIndex;

CCanvas::Push ();
CCanvas::SetCurrent (CurrentGameScreen ());
if (nWeaponType == 0) {
	nIndex = primaryWeaponToWeaponInfo [nWeaponId];
	if (nIndex == LASER_ID && laserLevel > MAX_LASER_LEVEL)
		nIndex = SUPERLASER_ID;
	CGenericCockpit::DrawWeaponInfo (nWeaponType, nIndex,
		hudWindowAreas + COCKPIT_PRIMARY_BOX,
		PRIMARY_W_PIC_X, PRIMARY_W_PIC_Y,
		PRIMARY_WEAPON_NAMES_SHORT (nWeaponId),
		PRIMARY_W_TEXT_X, PRIMARY_W_TEXT_Y, 0);
		}
else {
	nIndex = secondaryWeaponToWeaponInfo [nWeaponId];
	CGenericCockpit::DrawWeaponInfo (nWeaponType, nIndex,
		hudWindowAreas + COCKPIT_SECONDARY_BOX,
		SECONDARY_W_PIC_X, SECONDARY_W_PIC_Y,
		SECONDARY_WEAPON_NAMES_SHORT (nWeaponId),
		SECONDARY_W_TEXT_X, SECONDARY_W_TEXT_Y, 0);
	}
CCanvas::Pop ();
}

//	-----------------------------------------------------------------------------

void CCockpit::DrawKillList (void)
{
CGenericCockpit::DrawKillList (53, CCanvas::Current ()->Height () - LHX (6));
}

//	-----------------------------------------------------------------------------

void CCockpit::DrawStatic (int nWindow)
{
CGenericCockpit::DrawStatic (nWindow, COCKPIT_PRIMARY_BOX);
}

//	-----------------------------------------------------------------------------

void CCockpit::DrawPlayerShip (void)
{
CGenericCockpit::DrawPlayerShip (m_info.bCloak, m_history [gameStates.render.vr.nCurrentPage].bCloak, SHIP_GAUGE_X, SHIP_GAUGE_Y);
}

//	-----------------------------------------------------------------------------

//	Draws invulnerable ship, or maybe the flashing ship, depending on invulnerability time left.
void CCockpit::DrawInvul (void)
{
	static fix time = 0;

if ((LOCALPLAYER.flags & PLAYER_FLAGS_INVULNERABLE) &&
	 ((m_info.tInvul > I2X (4)) || ((m_info.tInvul > 0) && (gameData.time.xGame & 0x8000)))) {
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
	ScaleX (hudAreaP->left),
	ScaleY (hudAreaP->top),
	ScaleX (hudAreaP->right - hudAreaP->left+1),
	ScaleY (hudAreaP->bot - hudAreaP->top+1));
}

//	-----------------------------------------------------------------------------

bool CCockpit::Setup (bool bRebuild)
{
if (bRebuild && !m_info.bRebuild)
	return true;
m_info.bRebuild = false;
if (!CGenericCockpit::Setup ())
	return false;
gameData.render.window.hMax = (screen.Height () * 2) / 3;
if (gameData.render.window.h > gameData.render.window.hMax)
	gameData.render.window.h = gameData.render.window.hMax;
if (gameData.render.window.w > gameData.render.window.wMax)
	gameData.render.window.w = gameData.render.window.wMax;
gameData.render.window.x = (gameData.render.window.wMax - gameData.render.window.w) / 2;
gameData.render.window.y = (gameData.render.window.hMax - gameData.render.window.h) / 2;
GameInitRenderSubBuffers (gameData.render.window.x, gameData.render.window.y, gameData.render.window.w, gameData.render.window.h);
return true;
}

//	-----------------------------------------------------------------------------

void CCockpit::Toggle (void)
{
CGenericCockpit::Activate (CM_STATUS_BAR, true);
}

//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------

bool CRearView::Setup (bool bRebuild)
{
if (bRebuild && !m_info.bRebuild)
	return true;
if (!CGenericCockpit::Setup ())
	return false;
gameData.render.window.hMax = (screen.Height () * 2) / 3;
if (gameData.render.window.h > gameData.render.window.hMax)
	gameData.render.window.h = gameData.render.window.hMax;
if (gameData.render.window.w > gameData.render.window.wMax)
	gameData.render.window.w = gameData.render.window.wMax;
gameData.render.window.x = (gameData.render.window.wMax - gameData.render.window.w) / 2;
gameData.render.window.y = (gameData.render.window.hMax - gameData.render.window.h) / 2;
GameInitRenderSubBuffers (gameData.render.window.x, gameData.render.window.y, gameData.render.window.w, gameData.render.window.h);
return true;
}

//	-----------------------------------------------------------------------------

