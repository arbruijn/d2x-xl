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

void PlayHomingWarning (void)
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

int	bLastHomingWarningShown [2]={-1, -1};

//	-----------------------------------------------------------------------------

void DrawHomingWarning (void)
{
if ((gameStates.render.cockpit.nMode == CM_STATUS_BAR) || gameStates.app.bEndLevelSequence)
	SBShowHomingWarning ();
else {
	CCanvas::SetCurrent (GetCurrentGameScreen ());
	if (LOCALPLAYER.homingObjectDist >= 0) {
		if (gameData.time.xGame & 0x4000) {
			HUDBitBlt (GAUGE_HOMING_WARNING_ON, HOMING_WARNING_X, HOMING_WARNING_Y);
			bLastHomingWarningShown [gameStates.render.vr.nCurrentPage] = 1;
			}
		else {
			HUDBitBlt (GAUGE_HOMING_WARNING_OFF, HOMING_WARNING_X, HOMING_WARNING_Y);
			bLastHomingWarningShown [gameStates.render.vr.nCurrentPage] = 0;
			}
		}
	else {
		HUDBitBlt (GAUGE_HOMING_WARNING_OFF, HOMING_WARNING_X, HOMING_WARNING_Y);
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

void CFullScreenCockput::DrawKeys (void)
{
	int y = 3 * nLineSpacing;
	int dx = GAME_FONT->Width () + GAME_FONT->Width () / 2;

if (HIDE_HUD)
	return;
if (IsMultiGame && !IsCoopGame)
	return;
if (LOCALPLAYER.flags & PLAYER_FLAGS_BLUE_KEY)
	HUDBitBlt (KEY_ICON_BLUE, 2, y, false, false);
if (LOCALPLAYER.flags & PLAYER_FLAGS_GOLD_KEY) 
	HUDBitBlt (KEY_ICON_YELLOW, 2 + dx, y, false, false);
if (LOCALPLAYER.flags & PLAYER_FLAGS_RED_KEY)
	HUDBitBlt (KEY_ICON_RED, 2 + (2 * dx), y, false, false);
}

//	-----------------------------------------------------------------------------

void CFullScreenCockpit::ShowOrbs (void)
{
	static int nIdOrbs = 0, nIdEntropy [2]= {0, 0};

if (HIDE_HUD)
	return;
if (gameData.app.nGameMode & (GM_HOARD | GM_ENTROPY)) {
	int x = 0, y = 0;
	CBitmap *bmP = NULL;

	if (gameStates.render.cockpit.nMode == CM_FULL_COCKPIT) {
		y = 2*nLineSpacing;
		x = 4*GAME_FONT->Width ();
		}
	else if (gameStates.render.cockpit.nMode == CM_STATUS_BAR) {
		y = nLineSpacing;
		x = GAME_FONT->Width ();
		}
	else if ((gameStates.render.cockpit.nMode == CM_FULL_SCREEN) ||
				(gameStates.render.cockpit.nMode == CM_LETTERBOX)) {
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

void CFullScreenCockpit::ShowFlag (void)
{
if (HIDE_HUD)
	return;
if ((gameData.app.nGameMode & GM_CAPTURE) && (LOCALPLAYER.flags & PLAYER_FLAGS_FLAG)) {
	int x = 0, y = 0, icon;

	if (gameStates.render.cockpit.nMode == CM_FULL_COCKPIT) {
		y = 2*nLineSpacing;
		x = 4*GAME_FONT->Width ();
		}
	else if (gameStates.render.cockpit.nMode == CM_STATUS_BAR) {
		y = nLineSpacing;
		x = GAME_FONT->Width ();
		}
	else if ((gameStates.render.cockpit.nMode == CM_FULL_SCREEN) ||
				 (gameStates.render.cockpit.nMode == CM_LETTERBOX)) {
		y = 5*nLineSpacing;
		x = GAME_FONT->Width ();
		if (gameStates.render.fonts.bHires)
			y += nLineSpacing;
		}
	else
		Int3 ();		//what sort of cockpit?
	icon = (GetTeam (gameData.multiplayer.nLocalPlayer) == TEAM_BLUE) ? FLAG_ICON_RED : FLAG_ICON_BLUE;
	HUDBitBlt (icon, x, y, false, false);
	}
}

//	-----------------------------------------------------------------------------

inline int HUDFlashGauge (int h, int *bFlash, int tToggle)
{
	time_t t = gameStates.app.nSDLTicks;
	int b = *bFlash;

if (gameStates.app.bPlayerIsDead || gameStates.app.bPlayerExploded)
	b = 0;
else if (b == 2) {
	if (h > 20)
		b = 0;
	else if (h > 10)
		b = 1;
	}
else if (b == 1) {
	if (h > 20)
		b = 0;
	else if (h <= 10)
		b = 2;
	}
else {
	if (h <= 10)
		b = 2;
	else if (h <= 20)
		b = 1;
	else
		b = 0;
	tToggle = -1;
	}
*bFlash = b;
return (int) ((b && (tToggle <= t)) ? t + 300 / b : 0);
}

//	-----------------------------------------------------------------------------

void CFullScreenCockpit::ShowEnergy (void)
{
	int h, y;

	static int nIdEnergy = 0;

	//CCanvas::SetCurrent (&gameStates.render.vr.buffers.subRender [0]);	//render off-screen
if (HIDE_HUD)
	return;
h = LOCALPLAYER.energy ? X2IR (LOCALPLAYER.energy) : 0;
if (gameOpts->render.cockpit.bTextGauges) {
	y = CCanvas::Current ()->Height () - (IsMultiGame ? 5 : 1) * nLineSpacing;
	fontManager.SetCurrent (GAME_FONT);
	fontManager.SetColorRGBi (GREEN_RGBA, 1, 0, 0);
	nIdEnergy = GrPrintF (&nIdEnergy, 2, y, "%s: %i", TXT_ENERGY, h);
	}
else {
	static int		bFlash = 0, bShow = 1;
	static time_t	tToggle;
	time_t			t;
	ubyte				c;

	if ((t = HUDFlashGauge (h, &bFlash, (int) tToggle))) {
		tToggle = t;
		bShow = !bShow;
		}
	y = CCanvas::Current ()->Height () - (int) (((IsMultiGame ? 5 : 1) * nLineSpacing - 1) * yScale);
	CCanvas::Current ()->SetColorRGB (255, 255, (ubyte) ((h > 100) ? 255 : 0), 255);
	OglDrawEmptyRect (6, y, 6 + (int) (100 * xScale), y + (int) (9 * yScale));
	if (bFlash) {
		if (!bShow)
			goto skipGauge;
		h = 100;
		}
	else
		bShow = 1;
	c = (h > 100) ? 224 : 224;
	CCanvas::Current ()->SetColorRGB (c, c, (ubyte) ((h > 100) ? c : 0), 128);
	OglDrawFilledRect (6, y, 6 + (int) (((h > 100) ? h - 100 : h) * xScale), y + (int) (9 * yScale));
	}

skipGauge:

if (gameData.demo.nState == ND_STATE_RECORDING) {
	int energy = X2IR (LOCALPLAYER.energy);

	if (energy != oldEnergy [gameStates.render.vr.nCurrentPage]) {
		NDRecordPlayerEnergy (oldEnergy [gameStates.render.vr.nCurrentPage], energy);
		oldEnergy [gameStates.render.vr.nCurrentPage] = energy;
	 	}
	}
}

//	-----------------------------------------------------------------------------

void CFullScreenCockpit::ShowAfterburner (void)
{
	int h, y;

	static int nIdAfterBurner = 0;
if (HIDE_HUD)
	return;
if (!(LOCALPLAYER.flags & PLAYER_FLAGS_AFTERBURNER))
	return;		//don't draw if don't have
h = FixMul (gameData.physics.xAfterburnerCharge, 100);
if (gameOpts->render.cockpit.bTextGauges) {
	y = CCanvas::Current ()->Height () - ((gameData.app.nGameMode & GM_MULTI) ? 8 : 3) * nLineSpacing;
	fontManager.SetCurrent (GAME_FONT);
	fontManager.SetColorRGBi (GREEN_RGBA, 1, 0, 0);
	nIdAfterBurner = GrPrintF (&nIdAfterBurner, 2, y, TXT_HUD_BURN, h);
	}
else {
	y = CCanvas::Current ()->Height () - (int) ((((gameData.app.nGameMode & GM_MULTI) ? 8 : 3) * nLineSpacing - 1) * yScale);
	CCanvas::Current ()->SetColorRGB (255, 0, 0, 255);
	OglDrawEmptyRect (6, y, 6 + (int) (100 * xScale), y + (int) (9 * yScale));
	CCanvas::Current ()->SetColorRGB (224, 0, 0, 128);
	OglDrawFilledRect (6, y, 6 + (int) (h * xScale), y + (int) (9 * yScale));
	}
if (gameData.demo.nState==ND_STATE_RECORDING) {
	if (gameData.physics.xAfterburnerCharge != oldAfterburner [gameStates.render.vr.nCurrentPage]) {
		NDRecordPlayerAfterburner (oldAfterburner [gameStates.render.vr.nCurrentPage], gameData.physics.xAfterburnerCharge);
		oldAfterburner [gameStates.render.vr.nCurrentPage] = gameData.physics.xAfterburnerCharge;
	 	}
	}
}
#if 0
char *d2_very_shortSecondary_weapon_names [] =
	 {"Flash", "Guided", "SmrtMine", "Mercury", "Shaker"};
#endif
#define SECONDARY_WEAPON_NAMES_VERY_SHORT(nWeaponId) 				\
	 ((nWeaponId <= MEGA_INDEX)?GAMETEXT (541 + nWeaponId):	\
	 GT (636+nWeaponId-FLASHMSL_INDEX))

//return which bomb will be dropped next time the bomb key is pressed
extern int ArmedBomb ();

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

inline int NumDispX (int val)
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

HUDBitBlt (GAUGE_NUMERICAL, NUMERICAL_GAUGE_X, NUMERICAL_GAUGE_Y);
fontManager.SetColorRGBi (RGBA_PAL2 (14, 14, 23), 1, 0, 0);
nIdShields = HUDPrintF (&nIdShields, NumDispX (shield), NUMERICAL_GAUGE_Y + (gameStates.video.nDisplayMode ? 36 : 16), "%d", shield);
}

//	-----------------------------------------------------------------------------

void CCockpit::DrawEnergy (void)
{
	static int nIdEnergy = 0;

HUDBitBlt (GAUGE_NUMERICAL, NUMERICAL_GAUGE_X, NUMERICAL_GAUGE_Y);
fontManager.SetColorRGBi (RGBA_PAL2 (25, 18, 6), 1, 0, 0);
nIdEnergy = HUDPrintF (&nIdEnergy, NumDispX (energy), NUMERICAL_GAUGE_Y + (gameStates.video.nDisplayMode ? 5 : 2), "%d", energy);
}

//	-----------------------------------------------------------------------------

void CCockpit::DrawEnergyBar (int nEnergy)
{
// values taken directly from the bitmap
#define ENERGY_GAUGE_TOP_LEFT		20
#define ENERGY_GAUGE_BOT_LEFT		0
#define ENERGY_GAUGE_BOT_WIDTH	126

HUDBitBlt (GAUGE_ENERGY_LEFT, LEFT_ENERGY_GAUGE_X, LEFT_ENERGY_GAUGE_Y);
HUDBitBlt (GAUGE_ENERGY_RIGHT, RIGHT_ENERGY_GAUGE_X, RIGHT_ENERGY_GAUGE_Y);
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

HUDBitBlt (GAUGE_AFTERBURNER, AFTERBURNER_GAUGE_X, AFTERBURNER_GAUGE_Y);
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
HUDBitBlt (GAUGE_SHIELDS + 9 - ((shield >= 100) ? 9 : (shield / 10)), SHIELD_GAUGE_X, SHIELD_GAUGE_Y);
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
	HUDBitBlt ((LOCALPLAYER.flags & keyGaugeInfo [i].nFlag) ? keyGaugeInfo [i].nGaugeOn : keyGaugeInfo [i].nGaugeOff, keyGaugeInfo [i].x [bHires], keyGaugeInfo [i].y [bHires]);
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

//	Draws invulnerable ship, or maybe the flashing ship, depending on invulnerability time left.
void CCockpit::DrawInvulnerableShip (void)
{
	static fix time = 0;
	fix tInvul = LOCALPLAYER.invulnerableTime + INVULNERABLE_TIME_MAX - gameData.time.xGame;

if (tInvul <= 0) 
	DrawShieldBar (X2IR (LOCALPLAYER.shields));
else if ((tInvul > I2X (4)) || (gameData.time.xGame & 0x8000)) {
	HUDBitBlt (GAUGE_INVULNERABLE + nInvulnerableFrame, SHIELD_GAUGE_X, SHIELD_GAUGE_Y);
	time += gameData.time.xFrame;
	while (time > INV_FRAME_TIME) {
		time -= INV_FRAME_TIME;
		if (++nInvulnerableFrame == N_INVULNERABLE_FRAMES)
			nInvulnerableFrame = 0;
		}
	}
}

//	-----------------------------------------------------------------------------

void CFullScreenCockpit::ShowKillList (void)
{
	int n_players, player_list [MAX_NUM_NET_PLAYERS];
	int n_left, i, xo = 0, x0, x1, y, save_y, fth, l;
	int t = gameStates.app.nSDLTicks;
	int bGetPing = gameStates.render.cockpit.bShowPingStats && (!networkData.tLastPingStat || (t - networkData.tLastPingStat >= 1000));
	static int faw = -1;

	static int nIdKillList [2][MAX_PLAYERS] = {{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0}};

if (HIDE_HUD)
	return;
// ugly hack since placement of netgame players and kills is based off of
// menuhires (which is always 1 for mac).  This throws off placement of
// players in pixel double mode.
if (bGetPing)
	networkData.tLastPingStat = t;
if (gameData.multigame.kills.xShowListTimer > 0) {
	gameData.multigame.kills.xShowListTimer -= gameData.time.xFrame;
	if (gameData.multigame.kills.xShowListTimer < 0)
		gameData.multigame.kills.bShowList = 0;
	}
fontManager.SetCurrent (GAME_FONT);
n_players = (gameData.multigame.kills.bShowList == 3) ? 2 : MultiGetKillList (player_list);
n_left = (n_players <= 4) ? n_players : (n_players+1)/2;
//If font size changes, this code might not work right anymore
//Assert (GAME_FONT->Height ()==5 && GAME_FONT->Width ()==7);
fth = GAME_FONT->Height ();
x0 = HUD_LHX (1);
x1 = (IsCoopGame) ? HUD_LHX (43) : HUD_LHX (43);
save_y = y = CCanvas::Current ()->Height () - n_left* (fth+1);
if (gameStates.render.cockpit.nMode == CM_FULL_COCKPIT) {
	save_y = y -= HUD_LHX (6);
	x1 = (IsCoopGame) ? HUD_LHX (43) : HUD_LHX (43);
	}
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
for (i = 0; i < n_players; i++) {
	int nPlayer;
	char name [80], teamInd [2] = {127, 0};
	int sw, sh, aw, indent =0;

	if (i>=n_left) {
		x0 = CCanvas::Current ()->Width () - ((gameStates.render.cockpit.nMode == CM_FULL_COCKPIT) ? HUD_LHX (53) : HUD_LHX (60));
		x1 = CCanvas::Current ()->Width () - ((gameData.app.nGameMode & GM_MULTI_COOP) ? HUD_LHX (27) : HUD_LHX (15));
		if (gameStates.render.cockpit.bShowPingStats) {
			x0 -= xo + 6 * faw;
			x1 -= xo + 6 * faw;
			}
		if (i==n_left)
			y = save_y;
		if (netGame.KillGoal || netGame.xPlayTimeAllowed)
			x1-=HUD_LHX (18);
		}
	else if (netGame.KillGoal || netGame.xPlayTimeAllowed)
		 x1 = HUD_LHX (43) - HUD_LHX (18);
	nPlayer = (gameData.multigame.kills.bShowList == 3) ? i : player_list [i];
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
	nIdKillList [0][i] = GrPrintF (nIdKillList [0] + i, x0 + indent, y, "%s", name);

	if (gameData.multigame.kills.bShowList == 2) {
		if (gameData.multiplayer.players [nPlayer].netKilledTotal + gameData.multiplayer.players [nPlayer].netKillsTotal <= 0)
			nIdKillList [1][i] = GrPrintF (nIdKillList [1] + i, x1, y, TXT_NOT_AVAIL);
		else
			nIdKillList [1][i] = GrPrintF (nIdKillList [1] + i, x1, y, "%d%%",
				 (int) ((double) gameData.multiplayer.players [nPlayer].netKillsTotal /
						 ((double) gameData.multiplayer.players [nPlayer].netKilledTotal +
						  (double) gameData.multiplayer.players [nPlayer].netKillsTotal) * 100.0));
		}
	else if (gameData.multigame.kills.bShowList == 3) {
		if (gameData.app.nGameMode & GM_ENTROPY)
			nIdKillList [1][i] = GrPrintF (nIdKillList [1] + i, x1, y, "%3d [%d/%d]",
						 gameData.multigame.kills.nTeam [i], gameData.entropy.nTeamRooms [i + 1],
						 gameData.entropy.nTotalRooms);
		else
			nIdKillList [1][i] = GrPrintF (nIdKillList [1] + i, x1, y, "%3d", gameData.multigame.kills.nTeam [i]);
		}
	else if (IsCoopGame)
		nIdKillList [1][i] = GrPrintF (nIdKillList [1] + i, x1, y, "%-6d", gameData.multiplayer.players [nPlayer].score);
   else if (netGame.xPlayTimeAllowed || netGame.KillGoal)
      nIdKillList [1][i] = GrPrintF (nIdKillList [1] + i, x1, y, "%3d (%d)",
					 gameData.multiplayer.players [nPlayer].netKillsTotal,
					 gameData.multiplayer.players [nPlayer].nKillGoalCount);
   else
		nIdKillList [1][i] = GrPrintF (nIdKillList [1] + i, x1, y, "%3d", gameData.multiplayer.players [nPlayer].netKillsTotal);
	if (gameStates.render.cockpit.bShowPingStats && (nPlayer != gameData.multiplayer.nLocalPlayer)) {
		if (bGetPing)
			PingPlayer (nPlayer);
		if (pingStats [nPlayer].sent) {
#if 0//def _DEBUG
			nIdKillList [1][i] = GrPrintF (nIdKillList [1] + i, x1 + xo, y, "%lu %d %d",
						  pingStats [nPlayer].ping,
						  pingStats [nPlayer].sent,
						  pingStats [nPlayer].received);
#else
			nIdKillList [1][i] = GrPrintF (nIdKillList [1] + i, x1 + xo, y, "%lu %i%%", pingStats [nPlayer].ping,
						 100 - ((pingStats [nPlayer].received * 100) / pingStats [nPlayer].sent));
#endif
			}
		}
	y += fth+1;
	}
}

//	-----------------------------------------------------------------------------

#if DBG
extern int bSavingMovieFrames;
#else
#define bSavingMovieFrames 0
#endif

//returns true if viewerP can see CObject
//	-----------------------------------------------------------------------------

int CanSeeObject (int nObject, int bCheckObjs)
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
void DrawHUDNames (void)
{
	int			bHasFlag, bShowName, bShowTeamNames, bShowAllNames, bShowFlags, nObject, nTeam;
	int			p;
	tRgbColorb*	colorP;
	
	static int nCurColor = 1, tColorChange = 0;
	static tRgbColorb typingColors [2] = {
	 {63, 0, 0},
	 {63, 63, 0}
	};
	char s [CALLSIGN_LEN+10];
	int w, h, aw;
	int x0, y0, x1, y1;
	int nColor;
	static int nIdNames [2][MAX_PLAYERS] = {{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0}};

bShowAllNames = ((gameData.demo.nState == ND_STATE_PLAYBACK) ||
						 (netGame.bShowAllNames && gameData.multigame.bShowReticleName));
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
//draw all the things on the HUD
void CGenericCockpit::Render (void)
{
xScale = screen.Scale (0);
xScale = screen.Scale (1);
if (gameStates.render.cockpit.nMode == CM_STATUS_BAR) {
	gameStates.render.cockpit.nLastDrawn [0] =
	gameStates.render.cockpit.nLastDrawn [1] = -1;
	InitGauges ();
//	vr_reset_display ();
  }
SetCMScales ();
nLineSpacing = GAME_FONT->Height () + GAME_FONT->Height ()/4;

	//	Show score so long as not in rearview
if (!(gameStates.render.bRearView || bSavingMovieFrames ||
	 (gameStates.render.cockpit.nMode == CM_REAR_VIEW) ||
	 (gameStates.render.cockpit.nMode == CM_STATUS_BAR))) {
	CFullScreenCockpit::ShowScore ();
	if (scoreTime)
		CFullScreenCockpit::ShowScoreAdded ();
	}

if (!(gameStates.render.bRearView || bSavingMovieFrames || (gameStates.render.cockpit.nMode == CM_REAR_VIEW)))
	CFullScreenCockpit::ShowTimerCount ();

//	Show other stuff if not in rearview or letterbox.
if (!gameStates.render.bRearView && (gameStates.render.cockpit.nMode != CM_REAR_VIEW)) {
	if ((gameStates.render.cockpit.nMode == CM_STATUS_BAR) || (gameStates.render.cockpit.nMode == CM_FULL_SCREEN))
		CFullScreenCockpit::ShowHomingWarning ();

	if ((gameStates.render.cockpit.nMode == CM_FULL_SCREEN) || (gameStates.render.cockpit.nMode == CM_LETTERBOX)) {
		if (!gameOpts->render.cockpit.bTextGauges) {
			if (gameOpts->render.cockpit.bScaleGauges) {
				xScale = (double) CCanvas::Current ()->Height () / 480.0;
				yScale = (double) CCanvas::Current ()->Height () / 640.0;
				}
			else
				xScale = yScale = 1;
			}
		CFullScreenCockpit::ShowEnergy ();
		CFullScreenCockpit::ShowShield ();
		CFullScreenCockpit::ShowAfterburner ();
		CFullScreenCockpit::ShowWeapons ();
		if (!bSavingMovieFrames)
			CFullScreenCockpit::ShowKeys ();
		CFullScreenCockpit::ShowCloakInvul ();

		if ((gameData.demo.nState==ND_STATE_RECORDING) && (LOCALPLAYER.flags != oldFlags [gameStates.render.vr.nCurrentPage])) {
			NDRecordPlayerFlags (oldFlags [gameStates.render.vr.nCurrentPage], LOCALPLAYER.flags);
			oldFlags [gameStates.render.vr.nCurrentPage] = LOCALPLAYER.flags;
			}
		}
#if 1//def _DEBUG
	if (!(IsMultiGame && gameData.multigame.kills.bShowList) && !bSavingMovieFrames)
		ShowTime ();
#endif
	if (gameOpts->render.cockpit.bReticle && !gameStates.app.bPlayerIsDead && !transformation.m_info.bUsePlayerHeadAngles)
		ShowReticle (0);

	ShowNames ();
	if (gameStates.render.cockpit.nMode != CM_REAR_VIEW) {
		CFullScreenCockpit::ShowFlag ();
		CFullScreenCockpit::ShowOrbs ();
		}
	if (!bSavingMovieFrames)
		HUDRenderMessageFrame ();

	if (gameStates.render.cockpit.nMode != CM_STATUS_BAR && !bSavingMovieFrames)
		CFullScreenCockpit::ShowLives ();

	if (gameData.app.nGameMode&GM_MULTI && gameData.multigame.kills.bShowList)
		CFullScreenCockpit::ShowKillList ();
#if 0
	ShowWeaponDisplays ();
#endif
	return;
	}

if (gameStates.render.bRearView && gameStates.render.cockpit.nMode!=CM_REAR_VIEW) {
	HUDRenderMessageFrame ();
	fontManager.SetCurrent (GAME_FONT);
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

SetCMScales ();
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
CCanvas::SetCurrent (GetCurrentGameScreen ());
fontManager.SetCurrent (GAME_FONT);
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

	if (LOCALPLAYER.flags != oldFlags [gameStates.render.vr.nCurrentPage]) {
		if (gameData.demo.nState==ND_STATE_RECORDING)
			NDRecordPlayerFlags (oldFlags [gameStates.render.vr.nCurrentPage], LOCALPLAYER.flags);
		oldFlags [gameStates.render.vr.nCurrentPage] = LOCALPLAYER.flags;
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
