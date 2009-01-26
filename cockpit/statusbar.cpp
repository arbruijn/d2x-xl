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
#include "gauges.h"
#include "hud_defs.h"
#include "hudmsg.h"
#include "statusbar.h"

//	-----------------------------------------------------------------------------

CBitmap* CStatusBar::StretchBlt (int nGauge, int x, int y, double xScale, double yScale, int scale, int orient, CBitmap)
{
if (nGauge >= 0) {
	PAGE_IN_GAUGE (nGauge);
	bmP = gameData.pig.tex.bitmaps [0] + GET_GAUGE_INDEX (nGauge);
	}
if (bmP)
	bmP->RenderScaled (HUD_SCALE_X (x), HUD_SCALE_Y (y), 
							 HUD_SCALE_X ((int) (bmP->Width ()&  xScale + 0.5)), HUD_SCALE_Y ((int) (bmP->Height ()&  yScale + 0.5)), 
							 scale, orient, NULL);
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

void CStatusBar::DrawScore (void)
{	                                                                                                                                                                                                                                                             
	char	szScore [20];
	int 	x, y;
	int	w, h, aw;
	int 	bRedrawScore;

	static int lastX [4] = {SB_SCORE_RIGHT_L, SB_SCORE_RIGHT_L, SB_SCORE_RIGHT_H, SB_SCORE_RIGHT_H};
	static int nIdLabel = 0, nIdScore = 0;

bRedrawScore = -99 ? (IsMultiGame && !IsCoopGame) : -1;
if (m_info.old [gameStates.render.vr.nCurrentPage].score == bRedrawScore) {
	fontManager.SetCurrent (GAME_FONT);
	fontManager.SetColorRGBi (MEDGREEN_RGBA, 1, 0, 0);
	nIdLabel = HUDPrintF (&nIdLabel, SB_SCORE_LABEL_X, SB_SCORE_Y, "%s:", (IsMultiGame && !IsCoopGame) ? TXT_KILLS : TXT_SCORE);
	}
fontManager.SetCurrent (GAME_FONT);
sprintf (szScore, "%5d", (IsMultiGame && !IsCoopGame) ? LOCALPLAYER.netKillsTotal : LOCALPLAYER.score);
fontManager.Current ()->StringSize (szScore, w, h, aw);
x = SB_SCORE_RIGHT - w - HUD_LHX (2);
y = SB_SCORE_Y;
//erase old score
CCanvas::Current ()->SetColorRGBi (RGBA_PAL (0, 0, 0));
HUDRect (lastX [(gameStates.video.nDisplayMode ? 2 : 0) + gameStates.render.vr.nCurrentPage], y, SB_SCORE_RIGHT, y + GAME_FONT->Height ());
fontManager.SetColorRGBi ((IsMultiGame && !IsCoopGame) ? MEDGREEN_RGBA : GREEN_RGBA, 1, 0, 0);
nIdScore = HUDPrintF (&nIdScore, x, y, szScore);
lastX [(gameStates.video.nDisplayMode?2:0)+gameStates.render.vr.nCurrentPage] = x;
}

//	-----------------------------------------------------------------------------

void CStatusBar::DrawScoreAdded (void)
{
if (IsMultiGame && !IsCoopGame) 
	return;
if (scoreDisplay [gameStates.render.vr.nCurrentPage] == 0)
	return;

	int	x, w, h, aw, color, frc = 0;
	char	szScore [32];

	static int lastX [4]={SB_SCORE_RIGHT_L, SB_SCORE_RIGHT_L, SB_SCORE_RIGHT_H, SB_SCORE_RIGHT_H};
	static int last_score_display [2] = {-1, -1};
	static int nIdTotalScore = 0;

fontManager.SetCurrent (GAME_FONT);
scoreTime -= gameData.time.xFrame;
if (scoreTime > 0) {
	if (scoreDisplay [gameStates.render.vr.nCurrentPage] != last_score_display [gameStates.render.vr.nCurrentPage] || frc) {
		CCanvas::Current ()->SetColorRGBi (RGBA_PAL (0, 0, 0));
		OglDrawFilledRect (lastX [(gameStates.video.nDisplayMode?2:0)+gameStates.render.vr.nCurrentPage], SB_SCORE_ADDED_Y, SB_SCORE_ADDED_RIGHT, SB_SCORE_ADDED_Y+GAME_FONT->Height ());
		last_score_display [gameStates.render.vr.nCurrentPage] = scoreDisplay [gameStates.render.vr.nCurrentPage];
		}
	color = X2I (scoreTime&  20) + 10;
	if (color < 10) 
		color = 10;
	else if (color > 31) 
		color = 31;
	if (gameStates.app.cheats.bEnabled)
		sprintf (szScore, "%s", TXT_CHEATER);
	else
		sprintf (szScore, "%5d", scoreDisplay [gameStates.render.vr.nCurrentPage]);
	fontManager.Current ()->StringSize (szScore, w, h, aw);
	x = SB_SCORE_ADDED_RIGHT-w-HUD_LHX (2);
	fontManager.SetColorRGBi (RGBA_PAL2 (0, color, 0), 1, 0, 0);
	nIdTotalScore = GrPrintF (&nIdTotalScore, x, SB_SCORE_ADDED_Y, szScore);
	lastX [(gameStates.video.nDisplayMode?2:0)+gameStates.render.vr.nCurrentPage] = x;
	} 
else {
	//erase old score
	CCanvas::Current ()->SetColorRGBi (RGBA_PAL (0, 0, 0));
	OglDrawFilledRect (lastX [(gameStates.video.nDisplayMode?2:0)+gameStates.render.vr.nCurrentPage], SB_SCORE_ADDED_Y, SB_SCORE_ADDED_RIGHT, SB_SCORE_ADDED_Y+GAME_FONT->Height ());
	scoreTime = 0;
	scoreDisplay [gameStates.render.vr.nCurrentPage] = 0;
	}
}

//	-----------------------------------------------------------------------------

void CCockpit::DrawOrbs (void)
{
DrawOrbs (m_info.fontWidth, m_info.nLineSpacing);
}

//	-----------------------------------------------------------------------------

void CStatusBar::DrawHomingWarning (void)
{
if (bLastHomingWarningDrawn [gameStates.render.vr.nCurrentPage] == 1) {
	HUDBitBlt (GAUGE_HOMING_WARNING_OFF, HOMING_WARNING_X, HOMING_WARNING_Y);
	bLastHomingWarningDrawn [gameStates.render.vr.nCurrentPage] = 0;
	}
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
	int x = SB_LIVES_X, y = SB_LIVES_Y;

	CBitmap& bmP = gameData.pig.tex.bitmaps [0] + GET_GAUGE_INDEX (GAUGE_LIVES);

	static int nIdLives [2] = {0, 0}, nIdKilled = 0;
  
if (m_info.old [gameStates.render.vr.nCurrentPage].lives == -1) {
	fontManager.SetCurrent (GAME_FONT);
	fontManager.SetColorRGBi (MEDGREEN_RGBA, 1, 0, 0);
	nIdLives [0] = IsMultiGame ? 
						HUDPrintF (&nIdLives [0], SB_LIVES_LABEL_X, SB_LIVES_LABEL_Y, "%s:", TXT_DEATHS) : 
						HUDPrintF (&nIdLives [0], SB_LIVES_LABEL_X, SB_LIVES_LABEL_Y, "%s:", TXT_LIVES);
	}
if (IsMultiGame) {
	char szKilled [20];
	int w, h, aw;
	static int lastX [4] = {SB_SCORE_RIGHT_L, SB_SCORE_RIGHT_L, SB_SCORE_RIGHT_H, SB_SCORE_RIGHT_H};
	int x;

		sprintf (szKilled, "%5d", LOCALPLAYER.netKilledTotal);
		fontManager.Current ()->StringSize (szKilled, w, h, aw);
		CCanvas::Current ()->SetColorRGBi (RGBA_PAL (0, 0, 0));
		HUDRect (lastX [(gameStates.video.nDisplayMode ? 2 : 0) + gameStates.render.vr.nCurrentPage], 
					y + 1, SB_SCORE_RIGHT, y + GAME_FONT->Height ());
		fontManager.SetColorRGBi (MEDGREEN_RGBA, 1, 0, 0);
		x = SB_SCORE_RIGHT - w - 2;	
		nIdKilled = HUDPrintF (&nIdKilled, x, y + 1, szKilled);
		lastX [(gameStates.video.nDisplayMode?2:0)+gameStates.render.vr.nCurrentPage] = x;
		return;
	}
if ((m_info.old [gameStates.render.vr.nCurrentPage].lives == -1) || 
	 (LOCALPLAYER.lives != m_info.old [gameStates.render.vr.nCurrentPage].lives)) {
//erase old icons
	CCanvas::Current ()->SetColorRGBi (RGBA_PAL (0, 0, 0));
   
	HUDRect (x, y, SB_SCORE_RIGHT, y + bmP->Height ());
	if (LOCALPLAYER.lives - 1 > 0) {
		fontManager.SetCurrent (GAME_FONT);
		fontManager.SetColorRGBi (MEDGREEN_RGBA, 1, 0, 0);
		HUDBitBlt (GAUGE_LIVES, x, y);
		nIdLives [1] = HUDPrintF (&nIdLives [1], x + bmP->Width () + GAME_FONT->Width (), y, "x %d", LOCALPLAYER.lives - 1);
		}
	}
}

//	-----------------------------------------------------------------------------

void CStatusBar::DrawEnergy (int nEnergy)
{
	int w, h, aw;
	char szEnergy [20];

	static int nIdEnergyBar = 0;

sprintf (szEnergy, "%d", nEnergy);
fontManager.Current ()->StringSize (szEnergy, w, h, aw);
fontManager.SetColorRGBi (RGBA_PAL2 (25, 18, 6), 1, 0, 0);
nIdEnergyBar = HUDPrintF (&nIdEnergyBar, 
								  SB_ENERGY_GAUGE_X + ((SB_ENERGY_GAUGE_W - w)/2), 
								  SB_ENERGY_GAUGE_Y + SB_ENERGY_GAUGE_H - GAME_FONT->Height () - (GAME_FONT->Height () / 4), 
							     "%d", nEnergy);
}

//	-----------------------------------------------------------------------------

void CStatusBar::DrawEnergyBar (int nEnergy)
{
	int nEraseHeight;

	static int nIdEnergyBar = 0;

	if (gameStates.app.bD1Mission)
		StretchBlt (SB_GAUGE_ENERGY, SB_ENERGY_GAUGE_X, SB_ENERGY_GAUGE_Y, 1.0, (double) SB_ENERGY_GAUGE_H / (double) (SB_ENERGY_GAUGE_H - SB_AFTERBURNER_GAUGE_H));
	else
		BitBlt (SB_GAUGE_ENERGY, SB_ENERGY_GAUGE_X, SB_ENERGY_GAUGE_Y);
	nEraseHeight = (100 - nEnergy)&  SB_ENERGY_GAUGE_H / 100;
	if (nEraseHeight > 0) {
		CCanvas::Current ()->SetColorRGBi (BLACK_RGBA);
		glDisable (GL_BLEND);
		HUDRect (
			SB_ENERGY_GAUGE_X, 
			SB_ENERGY_GAUGE_Y, 
			SB_ENERGY_GAUGE_X+SB_ENERGY_GAUGE_W, 
			SB_ENERGY_GAUGE_Y+nEraseHeight);
		glEnable (GL_BLEND);
	}
}

//	-----------------------------------------------------------------------------

void CStatusBar::DrawAfterburner (void)
{
if (gameStates.app.bD1Mission)
	return;

	char szAB [3] = "AB";

	static int nIdAfterBurner = 0;

if (LOCALPLAYER.flags & PLAYER_FLAGS_AFTERBURNER)
	fontManager.SetColorRGBi (RGBA_PAL2 (45, 0, 0), 1, 0, 0);
else 
	fontManager.SetColorRGBi (RGBA_PAL2 (12, 12, 12), 1, 0, 0);

fontManager.Current ()->StringSize (szAB, w, h, aw);
nIdAfterBurner = HUDPrintF (&nIdAfterBurner, 
									 SB_AFTERBURNER_GAUGE_X + ((SB_AFTERBURNER_GAUGE_W - w)/2), 
									 SB_AFTERBURNER_GAUGE_Y+SB_AFTERBURNER_GAUGE_H-GAME_FONT->Height () - (GAME_FONT->Height () / 4), 
									 "AB");
}

//	-----------------------------------------------------------------------------

void CStatusBar::DrawAfterburnerBar (void)
{
if (gameStates.app.bD1Mission)
	return;

	int nEraseHeight, w, h, aw;
	char szAB [3] = "AB";

	static int nIdAfterBurner = 0;

HUDBitBlt (SB_GAUGE_AFTERBURNER, SB_AFTERBURNER_GAUGE_X, SB_AFTERBURNER_GAUGE_Y);
nEraseHeight = FixMul ((I2X (1) - gameData.physics.xAfterburnerCharge), SB_AFTERBURNER_GAUGE_H);
//	HUDMessage (0, "AB: %d", nEraseHeight);

if (nEraseHeight > 0) {
	CCanvas::Current ()->SetColorRGBi (BLACK_RGBA);
	glDisable (GL_BLEND);
	HUDRect (
		SB_AFTERBURNER_GAUGE_X, 
		SB_AFTERBURNER_GAUGE_Y, 
		SB_AFTERBURNER_GAUGE_X+SB_AFTERBURNER_GAUGE_W-1, 
		SB_AFTERBURNER_GAUGE_Y+nEraseHeight-1);
	glEnable (GL_BLEND);
	}
}

//	-----------------------------------------------------------------------------

void CStatusBar::DrawShield (int shield)
{
	static int nIdShieldNum = 0;

//draw numbers
fontManager.SetCurrent (GAME_FONT);
fontManager.SetColorRGBi (RGBA_PAL2 (14, 14, 23), 1, 0, 0);
//erase old one
LoadBitmap (gameData.pig.tex.cockpitBmIndex [gameStates.render.cockpit.nMode + (gameStates.video.nDisplayMode? (gameData.models.nCockpits/2):0)].index, 0);
HUDRect (SB_SHIELD_NUM_X, SB_SHIELD_NUM_Y, SB_SHIELD_NUM_X+ (gameStates.video.nDisplayMode?27:13), SB_SHIELD_NUM_Y+GAME_FONT->Height ());
nIdShieldNum = HUDPrintF (&nIdShieldNum, (shield>99)?SB_SHIELD_NUM_X: ((shield>9)?SB_SHIELD_NUM_X+2:SB_SHIELD_NUM_X+4), SB_SHIELD_NUM_Y, "%d", shield);
}

//	-----------------------------------------------------------------------------

void CStatusBar::DrawShieldBar (int shield)
{
CCanvas::SetCurrent (GetCurrentGameScreen ());
HUDBitBlt (GAUGE_SHIELDS + 9 - ((shield >= 100) ? 9 : (shield / 10)), SB_SHIELD_GAUGE_X, SB_SHIELD_GAUGE_Y);
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
	HUDBitBlt ((LOCALPLAYER.flags & keyGaugeInfo [i].nFlag) ? keyGaugeInfo [i].nGaugeOn : keyGaugeInfo [i].nGaugeOff, keyGaugeInfo [i].x [bHires], keyGaugeInfo [i].y [bHires]);
}

//	-----------------------------------------------------------------------------

//	Draws invulnerable ship, or maybe the flashing ship, depending on invulnerability time left.
void CStatusBar::DrawInvulnerableShip (void)
{
	static fix time = 0;
	fix tInvul = LOCALPLAYER.invulnerableTime + INVULNERABLE_TIME_MAX - gameData.time.xGame;

if (tInvul <= 0) 
	DrawShieldBar (X2IR (LOCALPLAYER.shields));
else if ((tInvul > I2X (4)) || (gameData.time.xGame & 0x8000)) {
		HUDBitBlt (GAUGE_INVULNERABLE + nInvulnerableFrame, SB_SHIELD_GAUGE_X, SB_SHIELD_GAUGE_Y);
		time += gameData.time.xFrame;
		while (time > INV_FRAME_TIME) {
			time -= INV_FRAME_TIME;
			if (++nInvulnerableFrame == N_INVULNERABLE_FRAMES)
				nInvulnerableFrame = 0;
			}
		}
	}
}

//	-----------------------------------------------------------------------------

void CStatusBar::ClearBombCount (void)
{
CCanvas::Current ()->SetColorRGBi (bgColor);
if (!gameStates.video.nDisplayMode) {
	HUDRect (169, 189, 189, 196);
	CCanvas::Current ()->SetColorRGBi (RGBA_PAL (128, 128, 128));
	OglDrawLine (HUD_SCALE_X (168), HUD_SCALE_Y (189), HUD_SCALE_X (189), HUD_SCALE_Y (189));
	}
else {
	OglDrawFilledRect (HUD_SCALE_X (338), HUD_SCALE_Y (453), HUD_SCALE_X (378), HUD_SCALE_Y (470));
	CCanvas::Current ()->SetColorRGBi (RGBA_PAL (128, 128, 128));
	OglDrawLine (HUD_SCALE_X (336), HUD_SCALE_Y (453), HUD_SCALE_X (378), HUD_SCALE_Y (453));
	}
}

//	-----------------------------------------------------------------------------

int CStatusBar::DrawBombCount (int* nId, int x, int y, char* pszBombCount)
{
CCanvas::Current ()->SetColorRGBi (bgColor);
return Print (&nIdBombCount, x, y, szBombCount, nIdBombCount);
}

//	-----------------------------------------------------------------------------

void CStatusBar::DrawStatic (int nWindow)
{
DrawStatic (nWindow, SB_PRIMARY_BOX);
}

//	-----------------------------------------------------------------------------

void CStatusBar::DrawKillList (void)
{
DrawKillList (60, CCanvas::Current ()->Height ());
}

//	-----------------------------------------------------------------------------

//print out some CPlayerData statistics
void SBRenderGauges (void)
{
	int nEnergy = X2IR (LOCALPLAYER.energy);
	int nShields = X2IR (LOCALPLAYER.shields);
	int bCloak = ((LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED) != 0);

if (nEnergy != m_info.old  [gameStates.render.vr.nCurrentPage].energy)  {
	if (gameData.demo.nState == ND_STATE_RECORDING) {
		NDRecordPlayerEnergy (m_info.old  [gameStates.render.vr.nCurrentPage].energy, nEnergy);
		}
	DrawEnergyBar (nEnergy);
	m_info.old  [gameStates.render.vr.nCurrentPage].energy = nEnergy;
	}
if (gameData.physics.xAfterburnerCharge != m_info.old [gameStates.render.vr.nCurrentPage].afterburner) {
	if (gameData.demo.nState == ND_STATE_RECORDING) {
		NDRecordPlayerAfterburner (m_info.old [gameStates.render.vr.nCurrentPage].afterburner, gameData.physics.xAfterburnerCharge);
		}
	DrawAfterburnerBar ();
	DrawAfterburner ();
	m_info.old [gameStates.render.vr.nCurrentPage].afterburner = gameData.physics.xAfterburnerCharge;
	}
if (LOCALPLAYER.flags & PLAYER_FLAGS_INVULNERABLE) {
	DrawInvulnerableShip ();
	DrawShield (nShields);
	m_info.old [gameStates.render.vr.nCurrentPage].shields = nShields ^ 1;
	} 
else {
	if (nShields != m_info.old [gameStates.render.vr.nCurrentPage].shields) {		// Draw the shield gauge
		if (gameData.demo.nState == ND_STATE_RECORDING) {
			NDRecordPlayerShields (m_info.old [gameStates.render.vr.nCurrentPage].shields, nShields);
			}
		DrawShieldBar (nShields);
		DrawShield (nShields);
		m_info.old [gameStates.render.vr.nCurrentPage].shields = nShields;
		}
	}
if (LOCALPLAYER.flags != oldFlags [gameStates.render.vr.nCurrentPage]) {
	if (gameData.demo.nState == ND_STATE_RECORDING)
		NDRecordPlayerFlags (oldFlags [gameStates.render.vr.nCurrentPage], LOCALPLAYER.flags);
	DrawKeys ();
	oldFlags [gameStates.render.vr.nCurrentPage] = LOCALPLAYER.flags;
	}
if ((IsMultiGame && !IsCoopGame) || (LOCALPLAYER.lives != m_info.old [gameStates.render.vr.nCurrentPage].lives)) {
	if (LOCALPLAYER.netKilledTotal != m_info.old [gameStates.render.vr.nCurrentPage].lives) {
		DrawLives ();
		m_info.old [gameStates.render.vr.nCurrentPage].lives = LOCALPLAYER.netKilledTotal;
		}
	}
if ((IsMultiGame && !IsCoopGame) || (LOCALPLAYER.score != m_info.old [gameStates.render.vr.nCurrentPage].score)) {
	if (LOCALPLAYER.netKillsTotal != m_info.old [gameStates.render.vr.nCurrentPage].score) {
		DrawScore ();
		m_info.old [gameStates.render.vr.nCurrentPage].score = LOCALPLAYER.netKillsTotal;
		}
	CStatusBar::DrawScoreAdded ();
	}
DrawBombCount (SB_BOMB_COUNT_X, SB_BOMB_COUNT_Y, BLACK_RGBA, 0);
DrawPlayerShip (bCloak, m_info.old [gameStates.render.vr.nCurrentPage].bCloak, SB_SHIP_GAUGE_X, SB_SHIP_GAUGE_Y);
}

//	---------------------------------------------------------------------------------------------------------

void CStatusBar::SetupWindow (int nWindow)
{
nArea = SB_PRIMARY_BOX + nWindow;
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
