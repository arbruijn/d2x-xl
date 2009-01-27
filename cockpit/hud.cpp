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


// these macros refer to arrays above

int oldScore [2]			= {-1, -1};
int oldEnergy [2]			= {-1, -1};
int oldShields [2]		= {-1, -1};
uint oldFlags [2]			= {(uint) -1, (uint) -1};
int oldWeapon [2][2]		= {{-1, -1}, {-1, -1}};
int oldAmmoCount [2][2]	= {{-1, -1}, {-1, -1}};
int xOldOmegaCharge [2]	= {-1, -1};
int oldLaserLevel [2]	= {-1, -1};
int bOldCloak [2]			= {0, 0};
int oldLives [2]			= {-1, -1};
fix oldAfterburner [2]	= {-1, -1};
int oldBombcount [2]		= {0, 0};

#define COCKPIT_PRIMARY_BOX		 (!gameStates.video.nDisplayMode ? 0 : 4)
#define COCKPIT_SECONDARY_BOX		 (!gameStates.video.nDisplayMode ? 1 : 5)
#define SB_PRIMARY_BOX				 (!gameStates.video.nDisplayMode ? 2 : 6)
#define SB_SECONDARY_BOX			 (!gameStates.video.nDisplayMode ? 3 : 7)

CHUD hud;

//	-----------------------------------------------------------------------------

void CHUD::DrawScore (void)
{
if (HIDE_HUD)
	return;

	char	szScore [40];
	int	w, h, aw;

if ((gameData.hud.msgs [0].nMessages > 0) &&
	 (strlen (gameData.hud.msgs [0].szMsgs [gameData.hud.msgs [0].nFirst]) > 38))
	return;
GrSetCurFont (GAME_FONT);
if ((IsMultiGame && !IsCoopGame))
	sprintf (szScore, "   %s: %5d", TXT_KILLS, LOCALPLAYER.netKillsTotal);
else
	sprintf (szScore, "   %s: %5d", TXT_SCORE, LOCALPLAYER.score);
GrGetStringSize (szScore, &w, &h, &aw);
GrSetFontColorRGBi (GREEN_RGBA, 1, 0, 0);
GrPrintF (NULL, grdCurCanv->cv_w - w - HUD_LHX (2), 3, szScore);
}

//	-----------------------------------------------------------------------------
//y offset between lines on HUD

 void CHUD::DrawScoreAdded (void)
{
if (HIDE_HUD)
	return;

	int	color;
	int	w, h, aw;
	char	szScore [20];

	static int nIdTotalScore = 0;

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
	nIdTotalScore = GrPrintF (&nIdTotalScore, CCanvas::Current ()->Width ()-w-HUD_LHX (2+10), m_info.nLineSpacing+4, szScore);
	}
else {
	scoreTime = 0;
	scoreDisplay [0] = 0;
	}
}

//	-----------------------------------------------------------------------------

void CHUD::DrawHomingWarning (void)
{
if (HIDE_HUD)
	return;

	static int nIdLock = 0;

if (LOCALPLAYER.homingObjectDist >= 0) {
	if (gameData.time.xGame & 0x4000) {
		int x = 0x8000, y = CCanvas::Current ()->Height ()-m_info.nLineSpacing;
		if ((weaponBoxUser [0] != WBU_WEAPON) || (weaponBoxUser [1] != WBU_WEAPON)) {
			int wy = (weaponBoxUser [0] != WBU_WEAPON) ? SW_y [0] : SW_y [1];
			y = min (y, (wy - m_info.nLineSpacing - gameData.render.window.y));
			}
		fontManager.SetCurrent (GAME_FONT);
		fontManager.SetColorRGBi (GREEN_RGBA, 1, 0, 0);
		nIdLock = GrPrintF (&nIdLock, x, y, TXT_LOCK);
		}
	}
}

//	-----------------------------------------------------------------------------

void CHUD::DrawKeys (void)
{
if (HIDE_HUD)
	return;

	int y = 3 * m_info.nLineSpacing;
	int dx = GAME_FONT->Width () + GAME_FONT->Width () / 2;

if (IsMultiGame && !IsCoopGame)
	return;
if (LOCALPLAYER.flags & PLAYER_FLAGS_BLUE_KEY)
	BitBlt (KEY_ICON_BLUE, 2, y, false, false);
if (LOCALPLAYER.flags & PLAYER_FLAGS_GOLD_KEY) 
	BitBlt (KEY_ICON_YELLOW, 2 + dx, y, false, false);
if (LOCALPLAYER.flags & PLAYER_FLAGS_RED_KEY)
	BitBlt (KEY_ICON_RED, 2 + (2 * dx), y, false, false);
}

//	-----------------------------------------------------------------------------

void CHUD::DrawOrbs (void)
{
if (HIDE_HUD)
	return;

//	-----------------------------------------------------------------------------

void CCockpit::DrawOrbs (void)
{
DrawOrbs (m_info.fontWidth, m_info.nLineSpacing * (gameStates.render.fonts.bHires + 1));
}
}

//	-----------------------------------------------------------------------------

void CHUD::DrawFlag (void)
{
if (HIDE_HUD)
	return;
DrawFlag (5 * m_info.nLineSpacing, m_info.nLineSpacing * (gameStates.render.fonts.bHires + 1));
}

//	-----------------------------------------------------------------------------

int CHUD::FlashGauge (int h, int *bFlash, int tToggle)
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

void CHUD::DrawEnergy (void)
{
if (HIDE_HUD)
	return;

	int h, y;
	static int nIdEnergy = 0;

h = LOCALPLAYER.energy ? X2IR (LOCALPLAYER.energy) : 0;
if (gameOpts->render.cockpit.bTextGauges) {
	y = CCanvas::Current ()->Height () - (IsMultiGame ? 5 : 1) * m_info.nLineSpacing;
	fontManager.SetCurrent (GAME_FONT);
	fontManager.SetColorRGBi (GREEN_RGBA, 1, 0, 0);
	nIdEnergy = GrPrintF (&nIdEnergy, 2, y, "%s: %i", TXT_ENERGY, h);
	}
if (gameData.demo.nState == ND_STATE_RECORDING) {
	int energy = X2IR (LOCALPLAYER.energy);

	if (energy != oldEnergy [gameStates.render.vr.nCurrentPage]) {
		NDRecordPlayerEnergy (oldEnergy [gameStates.render.vr.nCurrentPage], energy);
		oldEnergy [gameStates.render.vr.nCurrentPage] = energy;
	 	}
	}
}

//	-----------------------------------------------------------------------------

void CHUD::DrawEnergyBar (void)
{
if (HIDE_HUD)
	return;

	int h, y;
	static int nIdEnergy = 0;

if (!gameOpts->render.cockpit.bTextGauges) {
	static int		bFlash = 0, bShow = 1;
	static time_t	tToggle;
	time_t			t;
	ubyte				c;

	if ((t = FlashGauge (h, &bFlash, (int) tToggle))) {
		tToggle = t;
		bShow = !bShow;
		}
	y = CCanvas::Current ()->Height () - (int) (((IsMultiGame ? 5 : 1) * m_info.nLineSpacing - 1) * yGaugeScale);
	CCanvas::Current ()->SetColorRGB (255, 255, (ubyte) ((h > 100) ? 255 : 0), 255);
	OglDrawEmptyRect (6, y, 6 + (int) (100 * xGaugeScale), y + (int) (9 * yGaugeScale));
	if (bFlash) {
		if (!bShow)
			return;
		h = 100;
		}
	else
		bShow = 1;
	c = (h > 100) ? 224 : 224;
	CCanvas::Current ()->SetColorRGB (c, c, (ubyte) ((h > 100) ? c : 0), 128);
	OglDrawFilledRect (6, y, 6 + (int) (((h > 100) ? h - 100 : h) * xGaugeScale), y + (int) (9 * yGaugeScale));
	}
}

//	-----------------------------------------------------------------------------

void CHUD::DrawAfterburner (void)
{
if (HIDE_HUD)
	return;
	
	int h, y;
	static int nIdAfterBurner = 0;

if (!(LOCALPLAYER.flags & PLAYER_FLAGS_AFTERBURNER))
	return;		//don't draw if don't have
h = FixMul (gameData.physics.xAfterburnerCharge, 100);
if (gameOpts->render.cockpit.bTextGauges) {
	y = CCanvas::Current ()->Height () - ((gameData.app.nGameMode & GM_MULTI) ? 8 : 3) * m_info.nLineSpacing;
	fontManager.SetCurrent (GAME_FONT);
	fontManager.SetColorRGBi (GREEN_RGBA, 1, 0, 0);
	nIdAfterBurner = GrPrintF (&nIdAfterBurner, 2, y, TXT_HUD_BURN, h);
	}
}

//	-----------------------------------------------------------------------------

void CHUD::DrawAfterburnerBar (void)
{
if (HIDE_HUD)
	return;
	
	int h, y;
	static int nIdAfterBurner = 0;

if (!(LOCALPLAYER.flags & PLAYER_FLAGS_AFTERBURNER))
	return;		//don't draw if don't have
h = FixMul (gameData.physics.xAfterburnerCharge, 100);
if (!gameOpts->render.cockpit.bTextGauges) {
	y = CCanvas::Current ()->Height () - (int) ((((gameData.app.nGameMode & GM_MULTI) ? 8 : 3) * m_info.nLineSpacing - 1) * yGaugeScale);
	CCanvas::Current ()->SetColorRGB (255, 0, 0, 255);
	OglDrawEmptyRect (6, y, 6 + (int) (100 * xGaugeScale), y + (int) (9 * yGaugeScale));
	CCanvas::Current ()->SetColorRGB (224, 0, 0, 128);
	OglDrawFilledRect (6, y, 6 + (int) (h * xGaugeScale), y + (int) (9 * yGaugeScale));
	}
if (gameData.demo.nState == ND_STATE_RECORDING) {
	if (gameData.physics.xAfterburnerCharge != oldAfterburner [gameStates.render.vr.nCurrentPage]) {
		NDRecordPlayerAfterburner (oldAfterburner [gameStates.render.vr.nCurrentPage], gameData.physics.xAfterburnerCharge);
		oldAfterburner [gameStates.render.vr.nCurrentPage] = gameData.physics.xAfterburnerCharge;
	 	}
	}
}

//	-----------------------------------------------------------------------------

#define SECONDARY_WEAPON_NAMES_VERY_SHORT(nWeaponId) 			\
	 ((nWeaponId <= MEGA_INDEX)?GAMETEXT (541 + nWeaponId):	\
	 GT (636 + nWeaponId - FLASHMSL_INDEX))

//return which bomb will be dropped next time the bomb key is pressed
int ArmedBomb ();

//	-----------------------------------------------------------------------------

void CHUD::ClearBombCount (void)
{
}

//	-----------------------------------------------------------------------------

void CHUD::DrawBombCount (int& nIdBombCount, int x, int y, char* pszBombCount)
{
GrString (x, y, szBombCount, &nIdBombCount);
}

//	-----------------------------------------------------------------------------

void CHUD::DrawPrimaryAmmoInfo (int ammoCount)
{
DrawAmmoInfo (PRIMARY_AMMO_X, PRIMARY_AMMO_Y, ammoCount, 1);
}

//	-----------------------------------------------------------------------------

void CHUD::DrawSecondaryAmmoInfo (int ammoCount)
{
DrawAmmoInfo (SECONDARY_AMMO_X, SECONDARY_AMMO_Y, ammoCount, 0);
}

//	-----------------------------------------------------------------------------

//convert '1' characters to special wide ones
#define convert_1s(s) {char *p=s; while ((p = strchr (p, '1'))) *p = (char)132;}

void CHUD::DrawWeapons (void)
{
if (HIDE_HUD)
	return;

	int	w, h, aw;
	int	y;
	const char	*pszWeapon;
	char	szWeapon [32];

	static int nIdWeapons [2] = {0, 0};

//	CCanvas::SetCurrent (&gameStates.render.vr.buffers.subRender [0]);	//render off-screen
fontManager.SetCurrent (GAME_FONT);
fontManager.SetColorRGBi (GREEN_RGBA, 1, 0, 0);
y = CCanvas::Current ()->Height ();
if (IsMultiGame)
	y -= 4 * m_info.nLineSpacing;
pszWeapon = PRIMARY_WEAPON_NAMES_SHORT (gameData.weapons.nPrimary);
switch (gameData.weapons.nPrimary) {
	case LASER_INDEX:
		if (LOCALPLAYER.flags & PLAYER_FLAGS_QUAD_LASERS)
			sprintf (szWeapon, "%s %s %i", TXT_QUAD, pszWeapon, LOCALPLAYER.laserLevel+1);
		else
			sprintf (szWeapon, "%s %i", pszWeapon, LOCALPLAYER.laserLevel+1);
		break;

	case SUPER_LASER_INDEX:
		Int3 ();
		break;	//no such thing as super laser

	case VULCAN_INDEX:
	case GAUSS_INDEX:
		sprintf (szWeapon, "%s: %i", pszWeapon, X2I ((unsigned) LOCALPLAYER.primaryAmmo [VULCAN_INDEX] * (unsigned) VULCAN_AMMO_SCALE));
		convert_1s (szWeapon);
		break;

	case SPREADFIRE_INDEX:
	case PLASMA_INDEX:
	case FUSION_INDEX:
	case HELIX_INDEX:
	case PHOENIX_INDEX:
		strcpy (szWeapon, pszWeapon);
		break;

	case OMEGA_INDEX:
		sprintf (szWeapon, "%s: %03i", pszWeapon, gameData.omega.xCharge [IsMultiGame] * 100 / MAX_OMEGA_CHARGE);
		convert_1s (szWeapon);
		break;

	default:
		Int3 ();
		szWeapon [0] = 0;
		break;
	}

fontManager.Current ()->StringSize (szWeapon, w, h, aw);
nIdWeapons [0] = GrPrintF (nIdWeapons + 0, CCanvas::Current ()->Width () - 5 - w, y-2*m_info.nLineSpacing, szWeapon);

if (gameData.weapons.nPrimary == VULCAN_INDEX) {
	if (LOCALPLAYER.primaryAmmo [gameData.weapons.nPrimary] != oldAmmoCount [0][gameStates.render.vr.nCurrentPage]) {
		if (gameData.demo.nState == ND_STATE_RECORDING)
			NDRecordPrimaryAmmo (oldAmmoCount [0][gameStates.render.vr.nCurrentPage], LOCALPLAYER.primaryAmmo [gameData.weapons.nPrimary]);
		oldAmmoCount [0][gameStates.render.vr.nCurrentPage] = LOCALPLAYER.primaryAmmo [gameData.weapons.nPrimary];
		}
	}

if (gameData.weapons.nPrimary == OMEGA_INDEX) {
	if (gameData.omega.xCharge [IsMultiGame] != xOldOmegaCharge [gameStates.render.vr.nCurrentPage]) {
		if (gameData.demo.nState == ND_STATE_RECORDING)
			NDRecordPrimaryAmmo (xOldOmegaCharge [gameStates.render.vr.nCurrentPage], gameData.omega.xCharge [IsMultiGame]);
		xOldOmegaCharge [gameStates.render.vr.nCurrentPage] = gameData.omega.xCharge [IsMultiGame];
		}
	}

pszWeapon = SECONDARY_WEAPON_NAMES_VERY_SHORT (gameData.weapons.nSecondary);
sprintf (szWeapon, "%s %d", pszWeapon, LOCALPLAYER.secondaryAmmo [gameData.weapons.nSecondary]);
fontManager.Current ()->StringSize (szWeapon, w, h, aw);
nIdWeapons [1] = GrPrintF (nIdWeapons + 1, CCanvas::Current ()->Width ()-5-w, y-m_info.nLineSpacing, szWeapon);

if (LOCALPLAYER.secondaryAmmo [gameData.weapons.nSecondary] != oldAmmoCount [1][gameStates.render.vr.nCurrentPage]) {
	if (gameData.demo.nState == ND_STATE_RECORDING)
		NDRecordSecondaryAmmo (oldAmmoCount [1][gameStates.render.vr.nCurrentPage], LOCALPLAYER.secondaryAmmo [gameData.weapons.nSecondary]);
	oldAmmoCount [1][gameStates.render.vr.nCurrentPage] = LOCALPLAYER.secondaryAmmo [gameData.weapons.nSecondary];
	}
ShowBombCount (CCanvas::Current ()->Width ()- (3 * GAME_FONT->Width () + (gameStates.render.fonts.bHires?0:2)), y - 3 * m_info.nLineSpacing, -1, 1);
}

//	-----------------------------------------------------------------------------

void CHUD::DrawInvul (void)
{
if (HIDE_HUD)
	return;

	static int nIdInvul = 0;

if ((LOCALPLAYER.flags & PLAYER_FLAGS_INVULNERABLE) &&
	 (((LOCALPLAYER.invulnerableTime + INVULNERABLE_TIME_MAX - gameData.time.xGame) > I2X (4)) || (gameData.time.xGame & 0x8000))) {
	int	y = CCanvas::Current ()->Height ();

	if (IsMultiGame)
		y -= 10 * m_info.nLineSpacing;
	else
		y -= 5 * m_info.nLineSpacing;
	fontManager.SetColorRGBi (GREEN_RGBA, 1, 0, 0);
	nIdInvul = GrPrintF (&nIdInvul, 2, y, "%s", TXT_INVULNERABLE);
	}
}

//	-----------------------------------------------------------------------------

void CHUD::DrawCloak (void)
{
if (HIDE_HUD)
	return;

	static int nIdCloak = 0;


if ((LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED) &&
	 ((LOCALPLAYER.cloakTime + CLOAK_TIME_MAX - gameData.time.xGame > I2X (3)) || (gameData.time.xGame & 0x8000))) {
	int	y = CCanvas::Current ()->Height ();

	if (IsMultiGame)
		y -= 7 * m_info.nLineSpacing;
	else
		y -= 4 * m_info.nLineSpacing;
	fontManager.SetColorRGBi (GREEN_RGBA, 1, 0, 0);
	nIdCloak = GrPrintF (&nIdCloak, 2, y, "%s", TXT_CLOAKED);
	}
}

//	-----------------------------------------------------------------------------

void CHUD::DrawShield (void)
{
if (HIDE_HUD)
	return;

	static int nIdShield = 0;

//	CCanvas::SetCurrent (&gameStates.render.vr.buffers.subRender [0]);	//render off-screen
h = (LOCALPLAYER.shields >= 0) ? X2IR (LOCALPLAYER.shields) : 0;
if (gameOpts->render.cockpit.bTextGauges) {
	int y = CCanvas::Current ()->Height () - (IsMultiGame ? 6 : 2) * m_info.nLineSpacing;
	fontManager.SetCurrent (GAME_FONT);
	fontManager.SetColorRGBi (GREEN_RGBA, 1, 0, 0);
	nIdShield = GrPrintF (&nIdShield, 2, y, "%s: %i", TXT_SHIELD, h);
	}
if (gameData.demo.nState == ND_STATE_RECORDING) {
	if (h != oldShields [gameStates.render.vr.nCurrentPage]) {		// Draw the shield gauge
		NDRecordPlayerShields (oldShields [gameStates.render.vr.nCurrentPage], h);
		m_info.old [gameStates.render.vr.nCurrentPage].shields = h;
		}
	}
}

//	-----------------------------------------------------------------------------

void CHUD::DrawShieldBar (void)
{
if (HIDE_HUD)
	return;

	static int		bShow = 1;
	static time_t	tToggle = 0, nBeep = -1;
	time_t			t = gameStates.app.nSDLTicks;
	int				bLastFlash = gameStates.render.cockpit.nShieldFlash;
	int				h, y;

	static int nIdShield = 0;

//	CCanvas::SetCurrent (&gameStates.render.vr.buffers.subRender [0]);	//render off-screen
if (!gameOpts->render.cockpit.bTextGauges) {
	h = (LOCALPLAYER.shields >= 0) ? X2IR (LOCALPLAYER.shields) : 0;
	if ((t = FlashGauge (h, &gameStates.render.cockpit.nShieldFlash, (int) tToggle))) {
		tToggle = t;
		bShow = !bShow;
		}

	y = CCanvas::Current ()->Height () - (int) (((IsMultiGame ? 6 : 2) * m_info.nLineSpacing - 1) * yGaugeScale);
	CCanvas::Current ()->SetColorRGB (0, (ubyte) ((h > 100) ? 255 : 64), 255, 255);
	OglDrawEmptyRect (6, y, 6 + (int) (100 * xGaugeScale), y + (int) (9 * yGaugeScale));
	if (bShow) {
		CCanvas::Current ()->SetColorRGB (0, (ubyte) ((h > 100) ? 224 : 64), 224, 128);
		OglDrawFilledRect (6, y, 6 + (int) (((h > 100) ? h - 100 : h) * xGaugeScale), y + (int) (9 * yGaugeScale));
		}
	}
if (gameStates.render.cockpit.nShieldFlash) {
	if (gameOpts->gameplay.bShieldWarning && gameOpts->sound.bUseSDLMixer) {
		if ((nBeep < 0) || (bLastFlash != gameStates.render.cockpit.nShieldFlash)) {
			if (nBeep >= 0)
				audio.StopSound ((int) nBeep);
			nBeep = audio.StartSound (-1, SOUNDCLASS_GENERIC, I2X (2) / 3, 0xFFFF / 2, -1, -1, -1, -1, I2X (1),
											  AddonSoundName ((gameStates.render.cockpit.nShieldFlash == 1) ? SND_ADDON_LOW_SHIELDS1 : SND_ADDON_LOW_SHIELDS2));
			}
		}
	else if (nBeep >= 0) {
		audio.StopSound ((int) nBeep);
		nBeep = -1;
		}
	if (!bShow)
		return;
	h = 100;
	}
else {
	bShow = 1;
	if (nBeep >= 0) {
		audio.StopSound ((int) nBeep);
		nBeep = -1;
		}
	}
}

//	-----------------------------------------------------------------------------

//draw the icons for number of lives
void CHUD::DrawLives (void)
{
	static int nIdLives = 0;

if (HIDE_HUD)
	return;
if ((gameData.hud.msgs [0].nMessages > 0) && (strlen (gameData.hud.msgs [0].szMsgs [gameData.hud.msgs [0].nFirst]) > 38))
	return;
if (IsMultiGame) {
	fontManager.SetCurrent (GAME_FONT);
	fontManager.SetColorRGBi (GREEN_RGBA, 1, 0, 0);
	nIdLives = GrPrintF (&nIdLives, 10, 3, "%s: %d", TXT_DEATHS, LOCALPLAYER.netKilledTotal);
	}
else if (LOCALPLAYER.lives > 1)  {
	CBitmap *bmP;
	fontManager.SetCurrent (GAME_FONT);
	fontManager.SetColorRGBi (MEDGREEN_RGBA, 1, 0, 0);
	bmP = BitBlt (GAUGE_LIVES, 10, 3, false, false);
	nIdLives = GrPrintF (&nIdLives, 10 + bmP->Width () + bmP->Width () / 2, 4, "x %d", LOCALPLAYER.lives - 1);
	}
}

//	-----------------------------------------------------------------------------

void CHUD::DrawStatic (int nWindow)
{
}

//	-----------------------------------------------------------------------------

void CStatusBar::DrawKillList (void)
{
DrawKillList (60, CCanvas::Current ()->Height ());
}

//	-----------------------------------------------------------------------------

void CHud::DrawPlayerShip (int nCloakState, int nOldCloakState, int x, int y)
{
}

//	-----------------------------------------------------------------------------

void DrawWeaponInfo (int info_index, tGaugeBox *box, int xPic, int yPic, const char *pszName, int xText, int yText, int orient)
{
	CBitmap*	bmP;
	char		szName [100], *p;
	int		l;

	static int nIdWeapon [3] = {0, 0, 0}, nIdLaser [2] = {0, 0};

BitBlt (((gameData.pig.tex.nHamFileVersion >= 3) && gameStates.video.nDisplayMode) 
			  ? gameData.weapons.info [info_index].hiresPicture.index
			  : gameData.weapons.info [info_index].picture.index, 
			  xPic, yPic, true, true, (gameStates.render.cockpit.nMode == CM_FULL_SCREEN) ? I2X (2) : I2X (1), orient);
if (gameStates.render.cockpit.nMode == CM_FULL_SCREEN)
	return;
fontManager.SetColorRGBi (GREEN_RGBA, 1, 0, 0);
if ((p = const_cast<char*> (strchr (pszName, '\n')))) {
	memcpy (szName, pszName, l = p - pszName);
	szName [l] = '\0';
	nIdWeapon [0] = PrintF (&nIdWeapon [0], xText, yText, szName);
	nIdWeapon [1] = PrintF (&nIdWeapon [1], xText, yText + CCanvas::Current ()->Font ()->Height () + 1, p + 1);
	}
else {
	nIdWeapon [2] = PrintF (&nIdWeapon [2], xText, yText, pszName);
	}

//	For laser, show level and quadness
if (info_index == LASER_ID || info_index == SUPERLASER_ID) {
	sprintf (szName, "%s: 0", TXT_LVL);
	szName [5] = LOCALPLAYER.laserLevel + 1 + '0';
	nIdLaser [0] = PrintF (&nIdLaser [0], xText, yText + m_info.nLineSpacing, szName);
	if (LOCALPLAYER.flags & PLAYER_FLAGS_QUAD_LASERS) {
		strcpy (szName, TXT_QUAD);
		nIdLaser [1] = PrintF (&nIdLaser [1], xText, yText + 2 * m_info.nLineSpacing, szName);
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

CCanvas::SetCurrent (&gameStates.render.vr.buffers.render [0]);
fontManager.SetCurrent (GAME_FONT);
bLaserLevelChanged = ((nWeaponType == 0) &&
							  (nWeaponId == LASER_INDEX) &&
							  ((LOCALPLAYER.laserLevel != oldLaserLevel [gameStates.render.vr.nCurrentPage])));

if ((nWeaponId != oldWeapon [nWeaponType][gameStates.render.vr.nCurrentPage] || bLaserLevelChanged) && weaponBoxStates [nWeaponType] == WS_SET) {
	weaponBoxStates [nWeaponType] = WS_FADING_OUT;
	weaponBoxFadeValues [nWeaponType] = I2X (FADE_LEVELS - 1);
	}

if ((oldWeapon [nWeaponType][gameStates.render.vr.nCurrentPage] == -1) || 1/* (gameStates.render.cockpit.nMode == CM_FULL_COCKPIT)*/) {
	ShowWeaponInfo (nWeaponType, nWeaponId, LOCALPLAYER.laserLevel);
	oldWeapon [nWeaponType][gameStates.render.vr.nCurrentPage] = nWeaponId;
	oldAmmoCount [nWeaponType][gameStates.render.vr.nCurrentPage] = -1;
	xOldOmegaCharge [gameStates.render.vr.nCurrentPage] = -1;
	oldLaserLevel [gameStates.render.vr.nCurrentPage] = LOCALPLAYER.laserLevel;
	bDrew = 1;
	weaponBoxStates [nWeaponType] = WS_SET;
	}

if (weaponBoxStates [nWeaponType] == WS_FADING_OUT) {
	ShowWeaponInfo (nWeaponType, oldWeapon [nWeaponType][gameStates.render.vr.nCurrentPage], oldLaserLevel [gameStates.render.vr.nCurrentPage]);
	oldAmmoCount [nWeaponType][gameStates.render.vr.nCurrentPage] = -1;
	xOldOmegaCharge [gameStates.render.vr.nCurrentPage] = -1;
	bDrew = 1;
	weaponBoxFadeValues [nWeaponType] -= gameData.time.xFrame * FADE_SCALE;
	if (weaponBoxFadeValues [nWeaponType] <= 0) {
		weaponBoxStates [nWeaponType] = WS_FADING_IN;
		oldWeapon [nWeaponType][gameStates.render.vr.nCurrentPage] = nWeaponId;
		oldWeapon [nWeaponType][!gameStates.render.vr.nCurrentPage] = nWeaponId;
		oldLaserLevel [gameStates.render.vr.nCurrentPage] = LOCALPLAYER.laserLevel;
		oldLaserLevel [!gameStates.render.vr.nCurrentPage] = LOCALPLAYER.laserLevel;
		weaponBoxFadeValues [nWeaponType] = 0;
		}
	}
else if (weaponBoxStates [nWeaponType] == WS_FADING_IN) {
	if (nWeaponId != oldWeapon [nWeaponType][gameStates.render.vr.nCurrentPage]) {
		weaponBoxStates [nWeaponType] = WS_FADING_OUT;
		}
	else {
		ShowWeaponInfo (nWeaponType, nWeaponId, LOCALPLAYER.laserLevel);
		oldAmmoCount [nWeaponType][gameStates.render.vr.nCurrentPage] = -1;
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
CCanvas::SetCurrent (GetCurrentGameScreen ());
return bDrew;
}

//	-----------------------------------------------------------------------------

fix staticTime [2];

void DrawStatic (int nWindow)
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
CCanvas::SetCurrent (&gameStates.render.vr.buffers.render [0]);
h = boxofs + nWindow;
for (x = hudWindowAreas [h].left; x < hudWindowAreas [h].right; x += bmp->Width ())
	for (y = hudWindowAreas [h].top; y < hudWindowAreas [h].bot; y += bmp->Height ())
		BitBlt (-1, x, y, true, true, I2X (1), 0, bmp);
CCanvas::SetCurrent (GetCurrentGameScreen ());
CopyGaugeBox (&hudWindowAreas [h], &gameStates.render.vr.buffers.render [0]);
}

//	-----------------------------------------------------------------------------

//	Draws invulnerable ship, or maybe the flashing ship, depending on invulnerability time left.
void CHUD::DrawInvulnerableShip (void)
{
}

//	---------------------------------------------------------------------------------------------------------

void DrawCockpit (bool bAlphaTest)
{
gameData.render.window.x = (gameData.render.window.wMax - gameData.render.window.w) / 2;
gameData.render.window.y = (gameData.render.window.hMax - gameData.render.window.h) / 2;
//FillBackground ();
}

//	---------------------------------------------------------------------------------------------------------

static int cockpitWindowScale [4] = {6, 5, 4, 3};

void CHUD::SetupWindow (int nWindow)
{
int y, x;
int w = (int) (gameStates.render.vr.buffers.render [0].Width () / cockpitWindowScale [gameOpts->render.cockpit.nWindowSize] * HUD_ASPECT);	
int h = I2X (w) / screen.Aspect ();
int dx = (nWindow == 0) ? -w - w / 10 : w / 10;
switch (gameOpts->render.cockpit.nWindowPos) {
	case 0:
		y = nWindow ?
			gameStates.render.vr.buffers.render [0].Width () - w - h / 10 :
			h / 10;
		x = gameStates.render.vr.buffers.render [0].Height () - h - h / 10;
		break;
	case 1:
		y = nWindow ?
			gameStates.render.vr.buffers.render [0].Width () / 3 * 2 - w / 3 :
			gameStates.render.vr.buffers.render [0].Width () / 3 - 2 * w / 3;
		x = gameStates.render.vr.buffers.render [0].Height () - h - h / 10;
		break;
	case 2:	// only makes sense if there's only one cockpit window
		y = gameStates.render.vr.buffers.render [0].Width () / 2 - w / 2;
		x = gameStates.render.vr.buffers.render [0].Height () - h - h / 10;
		break;
	case 3:
		y = nWindow ?
			gameStates.render.vr.buffers.render [0].Width () - w - h / 10 :
			h / 10;
		x = h / 10;
		break;
	case 4:
		y = nWindow ?
			gameStates.render.vr.buffers.render [0].Width () / 3 * 2 - w / 3 :
			gameStates.render.vr.buffers.render [0].Width () / 3 - 2 * w / 3;
		x = h / 10;
		break;
	case 5:	// only makes sense if there's only one cockpit window
		y = gameStates.render.vr.buffers.render [0].Width () / 2 - w / 2;
		x = h / 10;
		break;
	}
if ((gameOpts->render.cockpit.nWindowPos < 3) &&
		extraGameInfo [0].nWeaponIcons &&
		(extraGameInfo [0].nWeaponIcons - gameOpts->render.weaponIcons.bEquipment < 3))
		x -= (int) ((gameOpts->render.weaponIcons.bSmall ? 20.0 : 30.0) * (double) CCanvas::Current ()->Height () / 480.0);

//copy these vars so stereo code can get at them
SW_drawn [nWindow] = 1;
SW_x [nWindow] = x;
SW_y [nWindow] = y;
SW_w [nWindow] = w;
SW_h [nWindow] = h;

gameStates.render.vr.buffers.render [0].SetupPane (&windowCanv, y, x, w, h);
}

//	-----------------------------------------------------------------------------
