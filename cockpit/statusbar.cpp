/* $Id: gauges.c, v 1.10 2003/10/11 09:28:38 btb Exp $& /
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
#include "physics.h"
#include "error.h"
#include "newdemo.h"
#include "gamefont.h"
#include "text.h"
#include "network.h"
#include "input.h"
#include "ogl_bitmap.h"
#include "ogl_hudstuff.h"
#include "ogl_render.h"
#include "cockpit.h"
#include "hud_defs.h"
#include "hudmsgs.h"
#include "statusbar.h"

//	-----------------------------------------------------------------------------

CBitmap* CStatusBar::StretchBlt (int nGauge, int x, int y, double xScale, double yScale, int scale, int orient)
{
	CBitmap* bmP = NULL;

if (nGauge >= 0) {
	PageInGauge (nGauge);
	CBitmap* bmP = gameData.pig.tex.bitmaps [0] + GaugeIndex (nGauge);
	if (bmP)
		bmP->RenderScaled (ScaleX (x), ScaleY (y), 
								 ScaleX ((int) (bmP->Width () * xScale + 0.5)), ScaleY ((int) (bmP->Height () * yScale + 0.5)), 
								 scale, orient, NULL);
	}
return bmP;
}

//	-----------------------------------------------------------------------------
//fills in the coords of the hostage video window
void CStatusBar::GetHostageWindowCoords (int& x, int& y, int& w, int& h)
{
x = SB_SECONDARY_W_BOX_LEFT;
y = SB_SECONDARY_W_BOX_TOP;
w = SB_SECONDARY_W_BOX_RIGHT - SB_SECONDARY_W_BOX_LEFT + 1;
h = SB_SECONDARY_W_BOX_BOT - SB_SECONDARY_W_BOX_TOP + 1;
}

//	-----------------------------------------------------------------------------

void CStatusBar::DrawRecording (void)
{
CGenericCockpit::DrawRecording (0);
}

//	-----------------------------------------------------------------------------

void CStatusBar::DrawCountdown (void)
{
CGenericCockpit::DrawCountdown (SMALL_FONT->Height () * 6);
}

//	-----------------------------------------------------------------------------

void CStatusBar::DrawCruise (void)
{
CGenericCockpit::DrawCruise (22, m_info.nLineSpacing * 22);
}

//	-----------------------------------------------------------------------------

void CStatusBar::DrawScore (void)
{	                                                                                                                                                                                                                                                             
	char	szScore [20];
	int 	x, y;
	int	w, h, aw;

	static int lastX [4] = {SB_SCORE_RIGHT_L, SB_SCORE_RIGHT_L, SB_SCORE_RIGHT_H, SB_SCORE_RIGHT_H};
	static int nIdLabel = 0, nIdScore = 0;

CCanvas::Push ();
CCanvas::SetCurrent (CurrentGameScreen ());
strcpy (szScore, (IsMultiGame && !IsCoopGame) ? TXT_KILLS : TXT_SCORE);
strcat (szScore, ":");
fontManager.Current ()->StringSize (szScore, w, h, aw);
fontManager.SetColorRGBi (MEDGREEN_RGBA, 1, 0, 0);
nIdLabel = PrintF (&nIdLabel, -(ScaleX (SB_SCORE_LABEL_X + w) - w), SB_SCORE_Y, szScore);

sprintf (szScore, "%5d", (IsMultiGame && !IsCoopGame) ? LOCALPLAYER.netKillsTotal : LOCALPLAYER.score);
fontManager.Current ()->StringSize (szScore, w, h, aw);
x = SB_SCORE_RIGHT - w - LHY (2);
y = SB_SCORE_Y;
//erase old score
CCanvas::Current ()->SetColorRGBi (RGBA_PAL (0, 0, 0));
Rect (lastX [(gameStates.video.nDisplayMode ? 2 : 0) + gameStates.render.vr.nCurrentPage], y, SB_SCORE_RIGHT, y + GAME_FONT->Height ());
fontManager.SetColorRGBi ((IsMultiGame && !IsCoopGame) ? MEDGREEN_RGBA : GREEN_RGBA, 1, 0, 0);
nIdScore = PrintF (&nIdScore, -(ScaleX (x) - w), y, szScore);
lastX [(gameStates.video.nDisplayMode ? 2 : 0) + gameStates.render.vr.nCurrentPage] = x;
CCanvas::Pop ();
}

//	-----------------------------------------------------------------------------

void CStatusBar::DrawScoreAdded (void)
{
if (IsMultiGame && !IsCoopGame) 
	return;
if (m_info.scoreDisplay [gameStates.render.vr.nCurrentPage] == 0)
	return;

	int	x, w, h, aw, color, frc = 0;
	char	szScore [32];

	static int lastX [4]={SB_SCORE_RIGHT_L, SB_SCORE_RIGHT_L, SB_SCORE_RIGHT_H, SB_SCORE_RIGHT_H};
	static int last_score_display [2] = {-1, -1};
	static int nIdTotalScore = 0;

m_info.scoreTime -= gameData.time.xFrame;
if (m_info.scoreTime > 0) {
	if (m_info.scoreDisplay [gameStates.render.vr.nCurrentPage] != last_score_display [gameStates.render.vr.nCurrentPage] || frc) {
		CCanvas::Current ()->SetColorRGBi (RGBA_PAL (0, 0, 0));
		OglDrawFilledRect (lastX [(gameStates.video.nDisplayMode?2:0)+gameStates.render.vr.nCurrentPage], SB_SCORE_ADDED_Y, SB_SCORE_ADDED_RIGHT, SB_SCORE_ADDED_Y+GAME_FONT->Height ());
		last_score_display [gameStates.render.vr.nCurrentPage] = m_info.scoreDisplay [gameStates.render.vr.nCurrentPage];
		}
	color = X2I (m_info.scoreTime * 20) + 10;
	if (color < 10) 
		color = 10;
	else if (color > 31) 
		color = 31;
	if (gameStates.app.cheats.bEnabled)
		sprintf (szScore, "%s", TXT_CHEATER);
	else
		sprintf (szScore, "%5d", m_info.scoreDisplay [gameStates.render.vr.nCurrentPage]);
	fontManager.Current ()->StringSize (szScore, w, h, aw);
	x = SB_SCORE_ADDED_RIGHT-w-LHY (2);
	fontManager.SetColorRGBi (RGBA_PAL2 (0, color, 0), 1, 0, 0);
	nIdTotalScore = GrPrintF (&nIdTotalScore, x, SB_SCORE_ADDED_Y, szScore);
	lastX [(gameStates.video.nDisplayMode?2:0)+gameStates.render.vr.nCurrentPage] = x;
	} 
else {
	//erase old score
	CCanvas::Current ()->SetColorRGBi (RGBA_PAL (0, 0, 0));
	OglDrawFilledRect (lastX [(gameStates.video.nDisplayMode?2:0)+gameStates.render.vr.nCurrentPage], SB_SCORE_ADDED_Y, SB_SCORE_ADDED_RIGHT, SB_SCORE_ADDED_Y+GAME_FONT->Height ());
	m_info.scoreTime = 0;
	m_info.scoreDisplay [gameStates.render.vr.nCurrentPage] = 0;
	}
}

//	-----------------------------------------------------------------------------

void CStatusBar::DrawOrbs (void)
{
CGenericCockpit::DrawOrbs (m_info.fontWidth, m_info.nLineSpacing);
}

//	-----------------------------------------------------------------------------

void CStatusBar::DrawFlag (void)
{
CGenericCockpit::DrawFlag (5 * m_info.nLineSpacing, m_info.nLineSpacing * (gameStates.render.fonts.bHires + 1));
}

//	-----------------------------------------------------------------------------

void CStatusBar::DrawHomingWarning (void)
{
hudCockpit.DrawHomingWarning ();
}

//	-----------------------------------------------------------------------------

void CStatusBar::DrawPrimaryAmmoInfo (int ammoCount)
{
DrawAmmoInfo (SB_PRIMARY_AMMO_X, SB_PRIMARY_AMMO_Y, ammoCount, 1);
}

//	-----------------------------------------------------------------------------

void CStatusBar::DrawSecondaryAmmoInfo (int ammoCount)
{
DrawAmmoInfo (SB_SECONDARY_AMMO_X, SB_SECONDARY_AMMO_Y, ammoCount, 0);
}

//	-----------------------------------------------------------------------------

void CStatusBar::DrawLives (void)
{
	static int nIdLives [2] = {0, 0}, nIdKilled = 0;
  
	CBitmap* bmP = gameData.pig.tex.bitmaps [0] + GaugeIndex (GAUGE_LIVES);
	char		szLives [20];
	int		w, h, aw;

CCanvas::Push ();
CCanvas::SetCurrent (CurrentGameScreen ());
fontManager.SetColorRGBi (MEDGREEN_RGBA, 1, 0, 0);
strcpy (szLives, IsMultiGame ? TXT_DEATHS : TXT_LIVES);
fontManager.Current ()->StringSize (szLives, w, h, aw);
nIdLives [0] = PrintF (&nIdLives [0], -(ScaleX (SB_LIVES_LABEL_X + w) - w), -ScaleY (SB_LIVES_LABEL_Y + m_info.heightPad), szLives);

if (IsMultiGame) {
	static int lastX [4] = {SB_SCORE_RIGHT_L, SB_SCORE_RIGHT_L, SB_SCORE_RIGHT_H, SB_SCORE_RIGHT_H};

	char	szKilled [20];
	int	x = SB_LIVES_X, 
			y = -(ScaleY (SB_LIVES_Y + 1) + m_info.heightPad);

	sprintf (szKilled, "%5d", LOCALPLAYER.netKilledTotal);
	fontManager.Current ()->StringSize (szKilled, w, h, aw);
	CCanvas::Current ()->SetColorRGBi (RGBA_PAL (0, 0, 0));
	Rect (lastX [(gameStates.video.nDisplayMode ? 2 : 0) + gameStates.render.vr.nCurrentPage], 
			y + 1, SB_SCORE_RIGHT, y + GAME_FONT->Height ());
	fontManager.SetColorRGBi (MEDGREEN_RGBA, 1, 0, 0);
	x = SB_SCORE_RIGHT - w - 2;	
	nIdKilled = PrintF (&nIdKilled, x, y + 1, szKilled);
	lastX [(gameStates.video.nDisplayMode ? 2 : 0) + gameStates.render.vr.nCurrentPage] = x;
	}
else if (LOCALPLAYER.lives != m_history [gameStates.render.vr.nCurrentPage].lives) {

	int	y = -ScaleY (SB_LIVES_Y + m_info.heightPad);

	CCanvas::Current ()->SetColorRGBi (RGBA_PAL (0, 0, 0));
	Rect (SB_LIVES_X, y, SB_SCORE_RIGHT, y + bmP->Height ());
	if (LOCALPLAYER.lives - 1 > 0) {
		fontManager.SetColorRGBi (MEDGREEN_RGBA, 1, 0, 0);
		BitBlt (GAUGE_LIVES, SB_LIVES_X, SB_LIVES_Y);
		nIdLives [1] = PrintF (&nIdLives [1], SB_LIVES_X + bmP->Width () + m_info.fontWidth, y, "x %d", LOCALPLAYER.lives - 1);
		}
	}
CCanvas::Pop ();
}

//	-----------------------------------------------------------------------------

void CStatusBar::DrawEnergy (void)
{
	static int nIdEnergy = 0;

	int w, h, aw;
	char szEnergy [20];

CCanvas::Push ();
CCanvas::SetCurrent (CurrentGameScreen ());
sprintf (szEnergy, "%d", m_info.nEnergy);
fontManager.Current ()->StringSize (szEnergy, w, h, aw);
fontManager.SetColorRGBi (RGBA_PAL2 (25, 18, 6), 1, 0, 0);
nIdEnergy = PrintF (&nIdEnergy, 
						  -(ScaleX (SB_ENERGY_GAUGE_X) + (ScaleX (SB_ENERGY_GAUGE_W) - w) / 2), 
						  -(ScaleY (SB_ENERGY_GAUGE_Y + SB_ENERGY_GAUGE_H - m_info.nLineSpacing) + m_info.heightPad), 
						  "%d", m_info.nEnergy);
CCanvas::Pop ();
}

//	-----------------------------------------------------------------------------

void CStatusBar::DrawEnergyBar (void)
{
if (gameStates.app.bD1Mission)
	StretchBlt (SB_GAUGE_ENERGY, SB_ENERGY_GAUGE_X, SB_ENERGY_GAUGE_Y, 1.0, 
					double (SB_ENERGY_GAUGE_H) / double (SB_ENERGY_GAUGE_H - SB_AFTERBURNER_GAUGE_H));
else
	BitBlt (SB_GAUGE_ENERGY, SB_ENERGY_GAUGE_X, SB_ENERGY_GAUGE_Y);
int nEraseHeight = (100 - m_info.nEnergy) * SB_ENERGY_GAUGE_H / 100;
if (nEraseHeight > 0) {
	CCanvas::Current ()->SetColorRGBi (BLACK_RGBA);
	glDisable (GL_BLEND);
	Rect (
		SB_ENERGY_GAUGE_X, 
		SB_ENERGY_GAUGE_Y, 
		SB_ENERGY_GAUGE_X + SB_ENERGY_GAUGE_W, 
		SB_ENERGY_GAUGE_Y + nEraseHeight);
	glEnable (GL_BLEND);
	}
}

//	-----------------------------------------------------------------------------

void CStatusBar::DrawAfterburner (void)
{
if (gameStates.app.bD1Mission)
	return;

	static int nIdAfterBurner = 0;

	char szAB [3] = "AB";

CCanvas::Push ();
CCanvas::SetCurrent (CurrentGameScreen ());
if (LOCALPLAYER.flags & PLAYER_FLAGS_AFTERBURNER)
	fontManager.SetColorRGBi (RGBA_PAL2 (45, 21, 0), 1, 0, 0);
else 
	fontManager.SetColorRGBi (RGBA_PAL2 (12, 12, 12), 1, 0, 0);

int w, h, aw;
fontManager.Current ()->StringSize (szAB, w, h, aw);
nIdAfterBurner = PrintF (&nIdAfterBurner, 
								 -(ScaleX (SB_AFTERBURNER_GAUGE_X) + (ScaleX (SB_AFTERBURNER_GAUGE_W) - w) / 2), 
								 -(ScaleY (SB_AFTERBURNER_GAUGE_Y + SB_AFTERBURNER_GAUGE_H - m_info.nLineSpacing) + m_info.heightPad), 
								 "AB");
CCanvas::Pop ();
}

//	-----------------------------------------------------------------------------

void CStatusBar::DrawAfterburnerBar (void)
{
if (gameStates.app.bD1Mission)
	return;

	int nEraseHeight;
	char szAB [3] = "AB";

	static int nIdAfterBurner = 0;

BitBlt (SB_GAUGE_AFTERBURNER, SB_AFTERBURNER_GAUGE_X, SB_AFTERBURNER_GAUGE_Y);
nEraseHeight = FixMul ((I2X (1) - gameData.physics.xAfterburnerCharge), SB_AFTERBURNER_GAUGE_H);

if (nEraseHeight > 0) {
	CCanvas::Current ()->SetColorRGBi (BLACK_RGBA);
	glDisable (GL_BLEND);
	Rect (
		SB_AFTERBURNER_GAUGE_X, 
		SB_AFTERBURNER_GAUGE_Y, 
		SB_AFTERBURNER_GAUGE_X + SB_AFTERBURNER_GAUGE_W - 1, 
		SB_AFTERBURNER_GAUGE_Y + nEraseHeight - 1);
	glEnable (GL_BLEND);
	}
}

//	-----------------------------------------------------------------------------

void CStatusBar::DrawShield (void)
{
	static int nIdShield = 0;

	int w, h, aw;
	char szShield [20];

CCanvas::Push ();
CCanvas::SetCurrent (CurrentGameScreen ());
//LoadBitmap (gameData.pig.tex.cockpitBmIndex [gameStates.render.cockpit.nType + (gameStates.video.nDisplayMode ? gameData.models.nCockpits / 2 : 0)].index, 0);
fontManager.SetColorRGBi (BLACK_RGBA, 1, 0, 0);
Rect (SB_SHIELD_NUM_X, SB_SHIELD_NUM_Y, SB_SHIELD_NUM_X + (gameStates.video.nDisplayMode ? 27 : 13), SB_SHIELD_NUM_Y + m_info.fontHeight);
sprintf (szShield, "%d", m_info.nShields);
fontManager.Current ()->StringSize (szShield, w, h, aw);
fontManager.SetColorRGBi (RGBA_PAL2 (14, 14, 23), 1, 0, 0);
nIdShield = PrintF (&nIdShield, 
						  -(ScaleX (SB_SHIELD_NUM_X + (gameStates.video.nDisplayMode ? 13 : 6)) - w / 2), 
						  -(ScaleY (SB_SHIELD_NUM_Y) + m_info.heightPad), 
						  "%d", m_info.nShields);
CCanvas::Pop ();
}

//	-----------------------------------------------------------------------------

void CStatusBar::DrawShieldBar (void)
{
if (m_info.tInvul <= 0)
	BitBlt (GAUGE_SHIELDS + 9 - ((m_info.nShields >= 100) ? 9 : (m_info.nShields / 10)), SB_SHIELD_GAUGE_X, SB_SHIELD_GAUGE_Y);
}

//	-----------------------------------------------------------------------------

typedef struct tKeyGaugeInfo {
	int	nFlag, nGaugeOn, nGaugeOff, x [2], y [2];
} tKeyGaugeInfo;

static tKeyGaugeInfo keyGaugeInfo [] = {
	{PLAYER_FLAGS_BLUE_KEY, SB_GAUGE_BLUE_KEY, SB_GAUGE_BLUE_KEY_OFF, {SB_GAUGE_KEYS_X_L, SB_GAUGE_KEYS_X_H}, {SB_GAUGE_BLUE_KEY_Y_L, SB_GAUGE_BLUE_KEY_Y_H}},
	{PLAYER_FLAGS_GOLD_KEY, SB_GAUGE_GOLD_KEY, SB_GAUGE_GOLD_KEY_OFF, {SB_GAUGE_KEYS_X_L, SB_GAUGE_KEYS_X_H}, {SB_GAUGE_GOLD_KEY_Y_L, SB_GAUGE_GOLD_KEY_Y_H}},
	{PLAYER_FLAGS_RED_KEY, SB_GAUGE_RED_KEY, SB_GAUGE_RED_KEY_OFF, {SB_GAUGE_KEYS_X_L, SB_GAUGE_KEYS_X_H}, {SB_GAUGE_RED_KEY_Y_L, SB_GAUGE_RED_KEY_Y_H}},
	};

void CStatusBar::DrawKeys (void)
{
int bHires = gameStates.video.nDisplayMode != 0;
for (int i = 0; i < 3; i++)
	BitBlt ((LOCALPLAYER.flags & keyGaugeInfo [i].nFlag) ? keyGaugeInfo [i].nGaugeOn : keyGaugeInfo [i].nGaugeOff, keyGaugeInfo [i].x [bHires], keyGaugeInfo [i].y [bHires]);
}

//	-----------------------------------------------------------------------------

void CStatusBar::DrawPlayerShip (void)
{
CGenericCockpit::DrawPlayerShip (m_info.bCloak, m_history [gameStates.render.vr.nCurrentPage].bCloak, SB_SHIP_GAUGE_X, SB_SHIP_GAUGE_Y);
}

//	-----------------------------------------------------------------------------

//	Draws invulnerable ship, or maybe the flashing ship, depending on invulnerability time left.
void CStatusBar::DrawInvul (void)
{
	static fix time = 0;

if ((LOCALPLAYER.flags & PLAYER_FLAGS_INVULNERABLE) &&
	 (m_info.tInvul > I2X (4)) || ((m_info.tInvul > 0) && (gameData.time.xGame & 0x8000))) {
	BitBlt (GAUGE_INVULNERABLE + m_info.nInvulnerableFrame, SB_SHIELD_GAUGE_X, SB_SHIELD_GAUGE_Y);
	time += gameData.time.xFrame;
	while (time > INV_FRAME_TIME) {
		time -= INV_FRAME_TIME;
		if (++m_info.nInvulnerableFrame == N_INVULNERABLE_FRAMES)
			m_info.nInvulnerableFrame = 0;
		}
	}
}

//	-----------------------------------------------------------------------------

void CStatusBar::ClearBombCount (int bgColor)
{
CCanvas::Push ();
CCanvas::SetCurrent (CurrentGameScreen ());
CCanvas::Current ()->SetColorRGBi (bgColor);
if (!gameStates.video.nDisplayMode) {
	Rect (169, 189, 189, 196);
	CCanvas::Current ()->SetColorRGBi (RGBA_PAL (128, 128, 128));
	OglDrawLine (ScaleX (168), ScaleY (189), ScaleX (189), ScaleY (189));
	}
else {
	OglDrawFilledRect (ScaleX (338), ScaleY (453), ScaleX (378), ScaleY (470));
	CCanvas::Current ()->SetColorRGBi (RGBA_PAL (128, 128, 128));
	OglDrawLine (ScaleX (336), ScaleY (453), ScaleX (378), ScaleY (453));
	}
CCanvas::Pop ();
}

//	-----------------------------------------------------------------------------

void CStatusBar::DrawBombCount (void)
{
CGenericCockpit::DrawBombCount (SB_BOMB_COUNT_X, SB_BOMB_COUNT_Y, BLACK_RGBA, 0);
}

//	-----------------------------------------------------------------------------

int CStatusBar::DrawBombCount (int& nIdBombCount, int x, int y, char* pszBombCount)
{
CCanvas::Push ();
CCanvas::SetCurrent (CurrentGameScreen ());
return PrintF (&nIdBombCount, x, y, pszBombCount, nIdBombCount);
CCanvas::Pop ();
}

//	-----------------------------------------------------------------------------

void CStatusBar::DrawStatic (int nWindow)
{
CGenericCockpit::DrawStatic (nWindow, SB_PRIMARY_BOX);
}

//	-----------------------------------------------------------------------------

void CStatusBar::DrawWeaponInfo (int nWeaponType, int nWeaponId, int laserLevel)
{
	int nIndex;

CCanvas::Push ();
CCanvas::SetCurrent (CurrentGameScreen ());
if (nWeaponType == 0) {
	nIndex = primaryWeaponToWeaponInfo [nWeaponId];
	if (nIndex == LASER_ID && laserLevel > MAX_LASER_LEVEL)
		nIndex = SUPERLASER_ID;
	CGenericCockpit::DrawWeaponInfo (nIndex,
		hudWindowAreas + SB_PRIMARY_BOX,
		SB_PRIMARY_W_PIC_X, SB_PRIMARY_W_PIC_Y,
		PRIMARY_WEAPON_NAMES_SHORT (nWeaponId),
		SB_PRIMARY_W_TEXT_X, SB_PRIMARY_W_TEXT_Y, 0);
		}
else {
	nIndex = secondaryWeaponToWeaponInfo [nWeaponId];
	CGenericCockpit::DrawWeaponInfo (nIndex,
		hudWindowAreas + SB_SECONDARY_BOX,
		SB_SECONDARY_W_PIC_X, SB_SECONDARY_W_PIC_Y,
		SECONDARY_WEAPON_NAMES_SHORT (nWeaponId),
		SB_SECONDARY_W_TEXT_X, SB_SECONDARY_W_TEXT_Y, 0);
	}
CCanvas::Pop ();
}

//	-----------------------------------------------------------------------------

void CStatusBar::DrawKillList (void)
{
CGenericCockpit::DrawKillList (60, CCanvas::Current ()->Height ());
}

//	-----------------------------------------------------------------------------

void CStatusBar::DrawCockpit (bool bAlphaTest)
{
CGenericCockpit::DrawCockpit (CM_STATUS_BAR + m_info.nCockpit, gameData.render.window.hMax, bAlphaTest);
gameData.render.window.x = (gameData.render.window.wMax - gameData.render.window.w) / 2;
gameData.render.window.y = (gameData.render.window.hMax - gameData.render.window.h) / 2;
//FillBackground ();
}

//	-----------------------------------------------------------------------------

bool CStatusBar::Setup (void)
{
if (!CGenericCockpit::Setup ())
	return false;
int h = gameData.pig.tex.bitmaps [0][gameData.pig.tex.cockpitBmIndex [CM_STATUS_BAR + (gameStates.video.nDisplayMode ? (gameData.models.nCockpits / 2) : 0)].index].Height ();
if (gameStates.app.bDemoData)
	h *= 2;
if (screen.Height () > 480)
	h = (int) ((double) h * (double) screen.Height () / 480.0);
gameData.render.window.hMax = screen.Height () - h;
gameData.render.window.h = gameData.render.window.hMax;
gameData.render.window.w = gameData.render.window.wMax;
gameData.render.window.x = (gameData.render.window.wMax - gameData.render.window.w) / 2;
gameData.render.window.y = (gameData.render.window.hMax - gameData.render.window.h) / 2;
GameInitRenderSubBuffers (gameData.render.window.x, gameData.render.window.y, gameData.render.window.w, gameData.render.window.h);
return true;
}

//	-----------------------------------------------------------------------------

void CStatusBar::SetupWindow (int nWindow, CCanvas* canvP)
{
tGaugeBox* hudAreaP = hudWindowAreas + SB_PRIMARY_BOX + nWindow;
gameStates.render.vr.buffers.render->SetupPane (
	canvP,
	ScaleX (hudAreaP->left),
	ScaleY (hudAreaP->top),
	ScaleX (hudAreaP->right - hudAreaP->left + 1),
	ScaleY (hudAreaP->bot - hudAreaP->top + 1));
}

//	-----------------------------------------------------------------------------

void CStatusBar::Toggle (void)
{
CGenericCockpit::Activate ((gameStates.render.cockpit.nNextType < 0) ? CM_FULL_SCREEN : CM_FULL_COCKPIT);
}

//	-----------------------------------------------------------------------------
