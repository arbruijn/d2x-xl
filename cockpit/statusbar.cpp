/* $Id: gauges.c, v 1.10 2003/10/11 09:28:38 btb Exp $ */
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

#include "inferno.h"
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
#include "gauges.h"
#include "hud_defs.h"
#include "hudmsg.h"
#include "statusbar.h"

//	-----------------------------------------------------------------------------

CCanvas *Canv_SBEnergyGauge;
CCanvas *Canv_SBAfterburnerGauge;

//	-----------------------------------------------------------------------------

void SBInitGaugeCanvases (void)
{
if (!bHaveGaugeCanvases && paletteManager.Game ()) {
	PAGE_IN_GAUGE (SB_GAUGE_ENERGY);
	Canv_SBEnergyGauge = CCanvas::Create (SB_ENERGY_GAUGE_W, SB_ENERGY_GAUGE_H);
	Canv_SBAfterburnerGauge = CCanvas::Create (SB_AFTERBURNER_GAUGE_W, SB_AFTERBURNER_GAUGE_H);
	}
}

//	-----------------------------------------------------------------------------

void SBCloseGaugeCanvases (void)
{
if (bHaveGaugeCanvases) {
	Canv_SBEnergyGauge->Destroy ();
	Canv_SBAfterburnerGauge->Destroy ();
	}
}

//	-----------------------------------------------------------------------------
//fills in the coords of the hostage video window
void SBGetHostageWindowCoords (int *x, int *y, int *w, int *h)
{
*x = SB_SECONDARY_W_BOX_LEFT;
*y = SB_SECONDARY_W_BOX_TOP;
*w = SB_SECONDARY_W_BOX_RIGHT - SB_SECONDARY_W_BOX_LEFT + 1;
*h = SB_SECONDARY_W_BOX_BOT - SB_SECONDARY_W_BOX_TOP + 1;
}

//	-----------------------------------------------------------------------------

void SBShowScore (void)
{	                                                                                                                                                                                                                                                             
	char	szScore [20];
	int 	x, y;
	int	w, h, aw;
	int 	bRedrawScore;

	static int lastX [4] = {SB_SCORE_RIGHT_L, SB_SCORE_RIGHT_L, SB_SCORE_RIGHT_H, SB_SCORE_RIGHT_H};
	static int nIdLabel = 0, nIdScore = 0;

bRedrawScore = -99 ? (IsMultiGame && !IsCoopGame) : -1;
if (oldScore [gameStates.render.vr.nCurrentPage] == bRedrawScore) {
	fontManager.SetCurrent (GAME_FONT);
	fontManager.SetColorRGBi (MEDGREEN_RGBA, 1, 0, 0);
	nIdLabel = HUDPrintF (&nIdLabel, SB_SCORE_LABEL_X, SB_SCORE_Y, "%s:", (IsMultiGame && !IsCoopGame) ? TXT_KILLS : TXT_SCORE);
	}
fontManager.SetCurrent (GAME_FONT);
sprintf (szScore, "%5d", (IsMultiGame && !IsCoopGame) ? LOCALPLAYER.netKillsTotal : LOCALPLAYER.score);
FONT->StringSize (szScore, w, h, aw);
x = SB_SCORE_RIGHT-w-HUD_LHX (2);
y = SB_SCORE_Y;
//erase old score
CCanvas::Current ()->SetColorRGBi (RGBA_PAL (0, 0, 0));
HUDRect (lastX [(gameStates.video.nDisplayMode ? 2 : 0) + gameStates.render.vr.nCurrentPage], y, SB_SCORE_RIGHT, y + GAME_FONT->Height ());
fontManager.SetColorRGBi ((IsMultiGame && !IsCoopGame) ? MEDGREEN_RGBA : GREEN_RGBA, 1, 0, 0);
nIdScore = HUDPrintF (&nIdScore, x, y, szScore);
lastX [(gameStates.video.nDisplayMode?2:0)+gameStates.render.vr.nCurrentPage] = x;
}

//	-----------------------------------------------------------------------------

void SBShowScoreAdded (void)
{
	int	x, w, h, aw, color, frc = 0;
	char	szScore [32];

	static int lastX [4]={SB_SCORE_RIGHT_L, SB_SCORE_RIGHT_L, SB_SCORE_RIGHT_H, SB_SCORE_RIGHT_H};
	static int last_score_display [2] = {-1, -1};
	static int nIdTotalScore = 0;

if (IsMultiGame && !IsCoopGame) 
	return;
if (scoreDisplay [gameStates.render.vr.nCurrentPage] == 0)
	return;
fontManager.SetCurrent (GAME_FONT);
scoreTime -= gameData.time.xFrame;
if (scoreTime > 0) {
	if (scoreDisplay [gameStates.render.vr.nCurrentPage] != last_score_display [gameStates.render.vr.nCurrentPage] || frc) {
		CCanvas::Current ()->SetColorRGBi (RGBA_PAL (0, 0, 0));
		GrRect (lastX [(gameStates.video.nDisplayMode?2:0)+gameStates.render.vr.nCurrentPage], SB_SCORE_ADDED_Y, SB_SCORE_ADDED_RIGHT, SB_SCORE_ADDED_Y+GAME_FONT->Height ());
		last_score_display [gameStates.render.vr.nCurrentPage] = scoreDisplay [gameStates.render.vr.nCurrentPage];
		}
	color = X2I (scoreTime * 20) + 10;
	if (color < 10) 
		color = 10;
	else if (color > 31) 
		color = 31;
	if (gameStates.app.cheats.bEnabled)
		sprintf (szScore, "%s", TXT_CHEATER);
	else
		sprintf (szScore, "%5d", scoreDisplay [gameStates.render.vr.nCurrentPage]);
	FONT->StringSize (szScore, w, h, aw);
	x = SB_SCORE_ADDED_RIGHT-w-HUD_LHX (2);
	fontManager.SetColorRGBi (RGBA_PAL2 (0, color, 0), 1, 0, 0);
	nIdTotalScore = GrPrintF (&nIdTotalScore, x, SB_SCORE_ADDED_Y, szScore);
	lastX [(gameStates.video.nDisplayMode?2:0)+gameStates.render.vr.nCurrentPage] = x;
	} 
else {
	//erase old score
	CCanvas::Current ()->SetColorRGBi (RGBA_PAL (0, 0, 0));
	GrRect (lastX [(gameStates.video.nDisplayMode?2:0)+gameStates.render.vr.nCurrentPage], SB_SCORE_ADDED_Y, SB_SCORE_ADDED_RIGHT, SB_SCORE_ADDED_Y+GAME_FONT->Height ());
	scoreTime = 0;
	scoreDisplay [gameStates.render.vr.nCurrentPage] = 0;
	}
}

//	-----------------------------------------------------------------------------

void SBShowHomingWarning (void)
{
if (bLastHomingWarningShown [gameStates.render.vr.nCurrentPage] == 1) {
	PAGE_IN_GAUGE (GAUGE_HOMING_WARNING_OFF);
	HUDBitBlt (
		HOMING_WARNING_X, HOMING_WARNING_Y, 
		gameData.pig.tex.bitmaps [0] + GET_GAUGE_INDEX (GAUGE_HOMING_WARNING_OFF), F1_0, 0);
	bLastHomingWarningShown [gameStates.render.vr.nCurrentPage] = 0;
	}
}

//	-----------------------------------------------------------------------------

void SBDrawPrimaryAmmoInfo (int ammoCount)
{
DrawAmmoInfo (SB_PRIMARY_AMMO_X, SB_PRIMARY_AMMO_Y, ammoCount, 1);
}

//	-----------------------------------------------------------------------------

void SBDrawSecondaryAmmoInfo (int ammoCount)
{
DrawAmmoInfo (SB_SECONDARY_AMMO_X, SB_SECONDARY_AMMO_Y, ammoCount, 0);
}

//	-----------------------------------------------------------------------------

void SBShowLives (void)
{
	int x = SB_LIVES_X, y = SB_LIVES_Y;

	CBitmap *bmP = gameData.pig.tex.bitmaps [0] + GET_GAUGE_INDEX (GAUGE_LIVES);

	static int nIdLives [2] = {0, 0}, nIdKilled = 0;
  
if (oldLives [gameStates.render.vr.nCurrentPage] == -1) {
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
		FONT->StringSize (szKilled, w, h, aw);
		CCanvas::Current ()->SetColorRGBi (RGBA_PAL (0, 0, 0));
		HUDRect (lastX [(gameStates.video.nDisplayMode ? 2 : 0) + gameStates.render.vr.nCurrentPage], 
					y + 1, SB_SCORE_RIGHT, y + GAME_FONT->Height ());
		fontManager.SetColorRGBi (MEDGREEN_RGBA, 1, 0, 0);
		x = SB_SCORE_RIGHT - w - 2;	
		nIdKilled = HUDPrintF (&nIdKilled, x, y + 1, szKilled);
		lastX [(gameStates.video.nDisplayMode?2:0)+gameStates.render.vr.nCurrentPage] = x;
		return;
	}
if ((oldLives [gameStates.render.vr.nCurrentPage] == -1) || 
	 (LOCALPLAYER.lives != oldLives [gameStates.render.vr.nCurrentPage])) {
//erase old icons
	CCanvas::Current ()->SetColorRGBi (RGBA_PAL (0, 0, 0));
   
	HUDRect (x, y, SB_SCORE_RIGHT, y + bmP->Height ());
	if (LOCALPLAYER.lives - 1 > 0) {
		fontManager.SetCurrent (GAME_FONT);
		fontManager.SetColorRGBi (MEDGREEN_RGBA, 1, 0, 0);
		PAGE_IN_GAUGE (GAUGE_LIVES);
		HUDBitBlt (x, y, bmP, F1_0, 0);
		nIdLives [1] = HUDPrintF (&nIdLives [1], x + bmP->Width () + GAME_FONT->Width (), y, "x %d", LOCALPLAYER.lives - 1);
		}
	}
}

//	-----------------------------------------------------------------------------

void SBDrawEnergyBar (int nEnergy)
{
	int erase_height, w, h, aw;
	char energy_str [20];

	static int nIdEnergyBar = 0;

//	CCanvas::SetCurrent (Canv_SBEnergyGauge);

	PAGE_IN_GAUGE (SB_GAUGE_ENERGY);
	if (gameStates.app.bD1Mission)
	HUDStretchBlt (SB_ENERGY_GAUGE_X, SB_ENERGY_GAUGE_Y, 
			         gameData.pig.tex.bitmaps [0] + GET_GAUGE_INDEX (SB_GAUGE_ENERGY), F1_0, 0, 
						1.0, (double) SB_ENERGY_GAUGE_H / (double) (SB_ENERGY_GAUGE_H - SB_AFTERBURNER_GAUGE_H));
	else
	HUDBitBlt (SB_ENERGY_GAUGE_X, SB_ENERGY_GAUGE_Y, 
									  gameData.pig.tex.bitmaps [0] + GET_GAUGE_INDEX (SB_GAUGE_ENERGY), F1_0, 0);
	erase_height = (100 - nEnergy) * SB_ENERGY_GAUGE_H / 100;
	if (erase_height > 0) {
		CCanvas::Current ()->SetColorRGBi (BLACK_RGBA);
		glDisable (GL_BLEND);
		HUDRect (
			SB_ENERGY_GAUGE_X, 
			SB_ENERGY_GAUGE_Y, 
			SB_ENERGY_GAUGE_X+SB_ENERGY_GAUGE_W, 
			SB_ENERGY_GAUGE_Y+erase_height);
		glEnable (GL_BLEND);
	}
CCanvas::SetCurrent (GetCurrentGameScreen ());
//draw numbers
sprintf (energy_str, "%d", nEnergy);
FONT->StringSize (energy_str, w, h, aw);
fontManager.SetColorRGBi (RGBA_PAL2 (25, 18, 6), 1, 0, 0);
nIdEnergyBar = HUDPrintF (&nIdEnergyBar, 
								  SB_ENERGY_GAUGE_X + ((SB_ENERGY_GAUGE_W - w)/2), 
								  SB_ENERGY_GAUGE_Y + SB_ENERGY_GAUGE_H - GAME_FONT->Height () - (GAME_FONT->Height () / 4), 
							     "%d", nEnergy);
Canv_SBEnergyGauge->FreeTexture ();
}

//	-----------------------------------------------------------------------------

void SBDrawAfterburner (void)
{
	int erase_height, w, h, aw;
	char ab_str [3] = "AB";

	static int nIdAfterBurner = 0;

//	static int b = 1;

	if (gameStates.app.bD1Mission)
		return;
//	CCanvas::SetCurrent (Canv_SBAfterburnerGauge);
	PAGE_IN_GAUGE (SB_GAUGE_AFTERBURNER);
	HUDBitBlt (SB_AFTERBURNER_GAUGE_X, SB_AFTERBURNER_GAUGE_Y, gameData.pig.tex.bitmaps [0] + GET_GAUGE_INDEX (SB_GAUGE_AFTERBURNER), F1_0, 0);
	erase_height = FixMul ((f1_0 - gameData.physics.xAfterburnerCharge), SB_AFTERBURNER_GAUGE_H);
//	HUDMessage (0, "AB: %d", erase_height);

	if (erase_height > 0) {
		CCanvas::Current ()->SetColorRGBi (BLACK_RGBA);
		glDisable (GL_BLEND);
		HUDRect (
			SB_AFTERBURNER_GAUGE_X, 
			SB_AFTERBURNER_GAUGE_Y, 
			SB_AFTERBURNER_GAUGE_X+SB_AFTERBURNER_GAUGE_W-1, 
			SB_AFTERBURNER_GAUGE_Y+erase_height-1);
		glEnable (GL_BLEND);
	}
CCanvas::SetCurrent (GetCurrentGameScreen ());
//draw legend
if (LOCALPLAYER.flags & PLAYER_FLAGS_AFTERBURNER)
	fontManager.SetColorRGBi (RGBA_PAL2 (45, 0, 0), 1, 0, 0);
else 
	fontManager.SetColorRGBi (RGBA_PAL2 (12, 12, 12), 1, 0, 0);

FONT->StringSize (ab_str, w, h, aw);
nIdAfterBurner = HUDPrintF (&nIdAfterBurner, 
									 SB_AFTERBURNER_GAUGE_X + ((SB_AFTERBURNER_GAUGE_W - w)/2), 
									 SB_AFTERBURNER_GAUGE_Y+SB_AFTERBURNER_GAUGE_H-GAME_FONT->Height () - (GAME_FONT->Height () / 4), 
									 "AB");
Canv_SBAfterburnerGauge->FreeTexture ();
}

//	-----------------------------------------------------------------------------

void SBDrawShieldNum (int shield)
{
	static int nIdShieldNum = 0;

//draw numbers
fontManager.SetCurrent (GAME_FONT);
fontManager.SetColorRGBi (RGBA_PAL2 (14, 14, 23), 1, 0, 0);
//erase old one
PIGGY_PAGE_IN (gameData.pig.tex.cockpitBmIndex [gameStates.render.cockpit.nMode + (gameStates.video.nDisplayMode? (gameData.models.nCockpits/2):0)].index, 0);
HUDRect (SB_SHIELD_NUM_X, SB_SHIELD_NUM_Y, SB_SHIELD_NUM_X+ (gameStates.video.nDisplayMode?27:13), SB_SHIELD_NUM_Y+GAME_FONT->Height ());
nIdShieldNum = HUDPrintF (&nIdShieldNum, (shield>99)?SB_SHIELD_NUM_X: ((shield>9)?SB_SHIELD_NUM_X+2:SB_SHIELD_NUM_X+4), SB_SHIELD_NUM_Y, "%d", shield);
}

//	-----------------------------------------------------------------------------

void SBDrawShieldBar (int shield)
{
	int bm_num = shield>=100?9: (shield / 10);

CCanvas::SetCurrent (GetCurrentGameScreen ());
PAGE_IN_GAUGE (GAUGE_SHIELDS + 9 - bm_num);
HUDBitBlt (SB_SHIELD_GAUGE_X, SB_SHIELD_GAUGE_Y, &gameData.pig.tex.bitmaps [0][GET_GAUGE_INDEX (GAUGE_SHIELDS+9-bm_num)], F1_0, 0);
}

//	-----------------------------------------------------------------------------

void SBDrawKeys (void)
{
	CBitmap * bmP;
	int flags = LOCALPLAYER.flags;

CCanvas::SetCurrent (GetCurrentGameScreen ());
bmP = &gameData.pig.tex.bitmaps [0][GET_GAUGE_INDEX ((flags&PLAYER_FLAGS_BLUE_KEY)?SB_GAUGE_BLUE_KEY:SB_GAUGE_BLUE_KEY_OFF)];
PAGE_IN_GAUGE ((flags&PLAYER_FLAGS_BLUE_KEY)?SB_GAUGE_BLUE_KEY:SB_GAUGE_BLUE_KEY_OFF);
HUDBitBlt (SB_GAUGE_KEYS_X, SB_GAUGE_BLUE_KEY_Y, bmP, F1_0, 0);
bmP = &gameData.pig.tex.bitmaps [0][GET_GAUGE_INDEX ((flags&PLAYER_FLAGS_GOLD_KEY)?SB_GAUGE_GOLD_KEY:SB_GAUGE_GOLD_KEY_OFF)];
PAGE_IN_GAUGE ((flags&PLAYER_FLAGS_GOLD_KEY)?SB_GAUGE_GOLD_KEY:SB_GAUGE_GOLD_KEY_OFF);
HUDBitBlt (SB_GAUGE_KEYS_X, SB_GAUGE_GOLD_KEY_Y, bmP, F1_0, 0);
bmP = &gameData.pig.tex.bitmaps [0][GET_GAUGE_INDEX ((flags&PLAYER_FLAGS_RED_KEY)?SB_GAUGE_RED_KEY:SB_GAUGE_RED_KEY_OFF)];
PAGE_IN_GAUGE ((flags&PLAYER_FLAGS_RED_KEY)?SB_GAUGE_RED_KEY:SB_GAUGE_RED_KEY_OFF);
HUDBitBlt (SB_GAUGE_KEYS_X, SB_GAUGE_RED_KEY_Y, bmP , F1_0, 0);
}

//	-----------------------------------------------------------------------------

//	Draws invulnerable ship, or maybe the flashing ship, depending on invulnerability time left.
void SBDrawInvulnerableShip (void)
{
	fix tInvul = LOCALPLAYER.invulnerableTime + INVULNERABLE_TIME_MAX - gameData.time.xGame;

CCanvas::SetCurrent (GetCurrentGameScreen ());
if (tInvul <= 0) 
	SBDrawShieldBar (X2IR (LOCALPLAYER.shields));
else if ((tInvul > F1_0 * 4) || (gameData.time.xGame & 0x8000)) {
	PAGE_IN_GAUGE (GAUGE_INVULNERABLE + nInvulnerableFrame);
	HUDBitBlt (SB_SHIELD_GAUGE_X, SB_SHIELD_GAUGE_Y, &gameData.pig.tex.bitmaps [0][GET_GAUGE_INDEX (GAUGE_INVULNERABLE + nInvulnerableFrame)], F1_0, 0);
	}
}

//	-----------------------------------------------------------------------------

//print out some CPlayerData statistics
void SBRenderGauges (void)
{
	int nEnergy = X2IR (LOCALPLAYER.energy);
	int nShields = X2IR (LOCALPLAYER.shields);
	int bCloak = ((LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED) != 0);

if (nEnergy != oldEnergy [gameStates.render.vr.nCurrentPage])  {
	if (gameData.demo.nState==ND_STATE_RECORDING) {
		NDRecordPlayerEnergy (oldEnergy [gameStates.render.vr.nCurrentPage], nEnergy);
		}
	SBDrawEnergyBar (nEnergy);
	oldEnergy [gameStates.render.vr.nCurrentPage] = nEnergy;
	}
if (gameData.physics.xAfterburnerCharge != oldAfterburner [gameStates.render.vr.nCurrentPage]) {
	if (gameData.demo.nState == ND_STATE_RECORDING) {
		NDRecordPlayerAfterburner (oldAfterburner [gameStates.render.vr.nCurrentPage], gameData.physics.xAfterburnerCharge);
		}
	SBDrawAfterburner ();
	oldAfterburner [gameStates.render.vr.nCurrentPage] = gameData.physics.xAfterburnerCharge;
	}
if (LOCALPLAYER.flags & PLAYER_FLAGS_INVULNERABLE) {
	DrawInvulnerableShip ();
	oldShields [gameStates.render.vr.nCurrentPage] = nShields ^ 1;
	SBDrawShieldNum (nShields);
	} 
else {
	if (nShields != oldShields [gameStates.render.vr.nCurrentPage]) {		// Draw the shield gauge
		if (gameData.demo.nState == ND_STATE_RECORDING) {
			NDRecordPlayerShields (oldShields [gameStates.render.vr.nCurrentPage], nShields);
			}
		SBDrawShieldBar (nShields);
		oldShields [gameStates.render.vr.nCurrentPage] = nShields;
		SBDrawShieldNum (nShields);
		}
	}
if (LOCALPLAYER.flags != oldFlags [gameStates.render.vr.nCurrentPage]) {
	if (gameData.demo.nState == ND_STATE_RECORDING)
		NDRecordPlayerFlags (oldFlags [gameStates.render.vr.nCurrentPage], LOCALPLAYER.flags);
	SBDrawKeys ();
	oldFlags [gameStates.render.vr.nCurrentPage] = LOCALPLAYER.flags;
	}
if (IsMultiGame && !IsCoopGame) {
	if (LOCALPLAYER.netKilledTotal != oldLives [gameStates.render.vr.nCurrentPage]) {
		SBShowLives ();
		oldLives [gameStates.render.vr.nCurrentPage] = LOCALPLAYER.netKilledTotal;
		}
	}
else {
	if (LOCALPLAYER.lives != oldLives [gameStates.render.vr.nCurrentPage]) {
		SBShowLives ();
		oldLives [gameStates.render.vr.nCurrentPage] = LOCALPLAYER.lives;
		}
	}
if (IsMultiGame && !IsCoopGame) {
	if (LOCALPLAYER.netKillsTotal != oldScore [gameStates.render.vr.nCurrentPage]) {
		SBShowScore ();
		oldScore [gameStates.render.vr.nCurrentPage] = LOCALPLAYER.netKillsTotal;
		}
	}
else {
	if (LOCALPLAYER.score != oldScore [gameStates.render.vr.nCurrentPage]) {
		SBShowScore ();
		oldScore [gameStates.render.vr.nCurrentPage] = LOCALPLAYER.score;
		}
	//if (scoreTime)
	SBShowScoreAdded ();
	}
ShowBombCount (SB_BOMB_COUNT_X, SB_BOMB_COUNT_Y, BLACK_RGBA, 0);
DrawPlayerShip (bCloak, bOldCloak [gameStates.render.vr.nCurrentPage], SB_SHIP_GAUGE_X, SB_SHIP_GAUGE_Y);
}

//	-----------------------------------------------------------------------------
