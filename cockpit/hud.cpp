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

void CHUD::ShowSlowMotion (void)
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
//x offset between lines on HUD

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
		int y = 0x8000, x = CCanvas::Current ()->Height ()-m_info.nLineSpacing;
		if ((weaponBoxUser [0] != WBU_WEAPON) || (weaponBoxUser [1] != WBU_WEAPON)) {
			int wy = (weaponBoxUser [0] != WBU_WEAPON) ? SW_y [0] : SW_y [1];
			x = min (x, (wy - m_info.nLineSpacing - gameData.render.window.x));
			}
		fontManager.SetCurrent (GAME_FONT);
		fontManager.SetColorRGBi (GREEN_RGBA, 1, 0, 0);
		nIdLock = GrPrintF (&nIdLock, y, x, TXT_LOCK);
		}
	}
}

//	-----------------------------------------------------------------------------

void CHUD::DrawKeys (void)
{
if (HIDE_HUD)
	return;

	int x = 3 * m_info.nLineSpacing;
	int dx = GAME_FONT->Width () + GAME_FONT->Width () / 2;

if (IsMultiGame && !IsCoopGame)
	return;
if (LOCALPLAYER.flags & PLAYER_FLAGS_BLUE_KEY)
	HUDBitBlt (KEY_ICON_BLUE, 2, x, false, false);
if (LOCALPLAYER.flags & PLAYER_FLAGS_GOLD_KEY) 
	HUDBitBlt (KEY_ICON_YELLOW, 2 + dx, x, false, false);
if (LOCALPLAYER.flags & PLAYER_FLAGS_RED_KEY)
	HUDBitBlt (KEY_ICON_RED, 2 + (2 * dx), x, false, false);
}

//	-----------------------------------------------------------------------------

void CHUD::DrawOrbs (void)
{
if (HIDE_HUD)
	return;

	static int nIdOrbs = 0, nIdEntropy [2]= {0, 0};

if (gameData.app.nGameMode & (GM_HOARD | GM_ENTROPY)) {
	int y = 0, x = 0;
	CBitmap *bmP = NULL;

	if (gameStates.render.cockpit.nMode == CM_FULL_COCKPIT) {
		x = 2*m_info.nLineSpacing;
		y = 4*GAME_FONT->Width ();
		}
	else if (gameStates.render.cockpit.nMode == CM_STATUS_BAR) {
		x = m_info.nLineSpacing;
		y = GAME_FONT->Width ();
		}
	else if ((gameStates.render.cockpit.nMode == CM_FULL_SCREEN) ||
				(gameStates.render.cockpit.nMode == CM_LETTERBOX)) {
//			x = 5*m_info.nLineSpacing;
		x = m_info.nLineSpacing;
		y = GAME_FONT->Width ();
		if (gameStates.render.fonts.bHires)
			x += m_info.nLineSpacing;
		}
	else
		Int3 ();		//what sort of cockpit?

	bmP = HUDBitBlt (-1, y, x, false, false, I2X (1), 0, &gameData.hoard.icon [gameStates.render.fonts.bHires].bm);
	//GrUBitmapM (y, x, bmP);

	y += bmP->Width () + bmP->Width ()/2;
	x+= (gameStates.render.fonts.bHires?2:1);
	if (gameData.app.nGameMode & GM_ENTROPY) {
		char	szInfo [20];
		int	w, h, aw;
		if (LOCALPLAYER.secondaryAmmo [PROXMINE_INDEX] >= extraGameInfo [1].entropy.nCaptureVirusLimit)
			fontManager.SetColorRGBi (ORANGE_RGBA, 1, 0, 0);
		else
			fontManager.SetColorRGBi (GREEN_RGBA, 1, 0, 0);
		sprintf (szInfo,
			"y %d [%d]",
			LOCALPLAYER.secondaryAmmo [PROXMINE_INDEX],
			LOCALPLAYER.secondaryAmmo [SMARTMINE_INDEX]);
		nIdEntropy [0] = GrPrintF (nIdEntropy, y, x, szInfo);
		if (gameStates.entropy.bConquering) {
			int t = (extraGameInfo [1].entropy.nCaptureTimeLimit * 1000) -
					   (gameStates.app.nSDLTicks - gameStates.entropy.nTimeLastMoved);

			if (t < 0)
				t = 0;
			fontManager.Current ()->StringSize (szInfo, w, h, aw);
			y += w;
			fontManager.SetColorRGBi (RED_RGBA, 1, 0, 0);
			sprintf (szInfo, " %d.%d", t / 1000, (t % 1000) / 100);
			nIdEntropy [1] = GrPrintF (nIdEntropy + 1, y, x, szInfo);
			}
		}
	else
		nIdOrbs = GrPrintF (&nIdOrbs, y, x, "y %d", LOCALPLAYER.secondaryAmmo [PROXMINE_INDEX]);
	}
}

//	-----------------------------------------------------------------------------

void CHUD::DrawFlag (void)
{
if (HIDE_HUD)
	return;

if ((gameData.app.nGameMode & GM_CAPTURE) && (LOCALPLAYER.flags & PLAYER_FLAGS_FLAG)) {
	int y = 0, x = 0, icon;

	if (gameStates.render.cockpit.nMode == CM_FULL_COCKPIT) {
		x = 2 * m_info.nLineSpacing;
		y = 4 * GAME_FONT->Width ();
		}
	else if (gameStates.render.cockpit.nMode == CM_STATUS_BAR) {
		x = m_info.nLineSpacing;
		y = GAME_FONT->Width ();
		}
	else if ((gameStates.render.cockpit.nMode == CM_FULL_SCREEN) ||
				 (gameStates.render.cockpit.nMode == CM_LETTERBOX)) {
		x = 5 * m_info.nLineSpacing;
		y = GAME_FONT->Width ();
		if (gameStates.render.fonts.bHires)
			x += m_info.nLineSpacing;
		}
	else
		Int3 ();		//what sort of cockpit?
	icon = (GetTeam (gameData.multiplayer.nLocalPlayer) == TEAM_BLUE) ? FLAG_ICON_RED : FLAG_ICON_BLUE;
	HUDBitBlt (icon, y, x, false, false);
	}
}

//	-----------------------------------------------------------------------------

inline int CHUD::FlashGauge (int h, int *bFlash, int tToggle)
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

	int h, x;
	static int nIdEnergy = 0;

h = LOCALPLAYER.energy ? X2IR (LOCALPLAYER.energy) : 0;
if (gameOpts->render.cockpit.bTextGauges) {
	x = CCanvas::Current ()->Height () - (IsMultiGame ? 5 : 1) * m_info.nLineSpacing;
	fontManager.SetCurrent (GAME_FONT);
	fontManager.SetColorRGBi (GREEN_RGBA, 1, 0, 0);
	nIdEnergy = GrPrintF (&nIdEnergy, 2, x, "%s: %i", TXT_ENERGY, h);
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

	int h, x;
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
	x = CCanvas::Current ()->Height () - (int) (((IsMultiGame ? 5 : 1) * m_info.nLineSpacing - 1) * yGaugeScale);
	CCanvas::Current ()->SetColorRGB (255, 255, (ubyte) ((h > 100) ? 255 : 0), 255);
	OglDrawEmptyRect (6, x, 6 + (int) (100 * xGaugeScale), x + (int) (9 * yGaugeScale));
	if (bFlash) {
		if (!bShow)
			return;
		h = 100;
		}
	else
		bShow = 1;
	c = (h > 100) ? 224 : 224;
	CCanvas::Current ()->SetColorRGB (c, c, (ubyte) ((h > 100) ? c : 0), 128);
	OglDrawFilledRect (6, x, 6 + (int) (((h > 100) ? h - 100 : h) * xGaugeScale), x + (int) (9 * yGaugeScale));
	}
}

//	-----------------------------------------------------------------------------

void CHUD::DrawAfterburner (void)
{
if (HIDE_HUD)
	return;
	
	int h, x;
	static int nIdAfterBurner = 0;

if (!(LOCALPLAYER.flags & PLAYER_FLAGS_AFTERBURNER))
	return;		//don't draw if don't have
h = FixMul (gameData.physics.xAfterburnerCharge, 100);
if (gameOpts->render.cockpit.bTextGauges) {
	x = CCanvas::Current ()->Height () - ((gameData.app.nGameMode & GM_MULTI) ? 8 : 3) * m_info.nLineSpacing;
	fontManager.SetCurrent (GAME_FONT);
	fontManager.SetColorRGBi (GREEN_RGBA, 1, 0, 0);
	nIdAfterBurner = GrPrintF (&nIdAfterBurner, 2, x, TXT_HUD_BURN, h);
	}
}

//	-----------------------------------------------------------------------------

void CHUD::DrawAfterburnerBar (void)
{
if (HIDE_HUD)
	return;
	
	int h, x;
	static int nIdAfterBurner = 0;

if (!(LOCALPLAYER.flags & PLAYER_FLAGS_AFTERBURNER))
	return;		//don't draw if don't have
h = FixMul (gameData.physics.xAfterburnerCharge, 100);
if (!gameOpts->render.cockpit.bTextGauges) {
	x = CCanvas::Current ()->Height () - (int) ((((gameData.app.nGameMode & GM_MULTI) ? 8 : 3) * m_info.nLineSpacing - 1) * yGaugeScale);
	CCanvas::Current ()->SetColorRGB (255, 0, 0, 255);
	OglDrawEmptyRect (6, x, 6 + (int) (100 * xGaugeScale), x + (int) (9 * yGaugeScale));
	CCanvas::Current ()->SetColorRGB (224, 0, 0, 128);
	OglDrawFilledRect (6, x, 6 + (int) (h * xGaugeScale), x + (int) (9 * yGaugeScale));
	}
if (gameData.demo.nState == ND_STATE_RECORDING) {
	if (gameData.physics.xAfterburnerCharge != oldAfterburner [gameStates.render.vr.nCurrentPage]) {
		NDRecordPlayerAfterburner (oldAfterburner [gameStates.render.vr.nCurrentPage], gameData.physics.xAfterburnerCharge);
		oldAfterburner [gameStates.render.vr.nCurrentPage] = gameData.physics.xAfterburnerCharge;
	 	}
	}
}

//	-----------------------------------------------------------------------------

#define SECONDARY_WEAPON_NAMES_VERY_SHORT(nWeaponId) 				\
	 ((nWeaponId <= MEGA_INDEX)?GAMETEXT (541 + nWeaponId):	\
	 GT (636 + nWeaponId - FLASHMSL_INDEX))

//return which bomb will be dropped next time the bomb key is pressed
extern int ArmedBomb ();

//	-----------------------------------------------------------------------------

void CHUD::DrawBombCount (int& nIdBombCount, int y, int x, char* pszBombCount)
{
GrString (y, x, szBombCount, &nIdBombCount);
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
	int	x;
	const char	*pszWeapon;
	char	szWeapon [32];

	static int nIdWeapons [2] = {0, 0};

#if 0
CHUD::DrawIcons ();
#endif
//	CCanvas::SetCurrent (&gameStates.render.vr.buffers.subRender [0]);	//render off-screen
fontManager.SetCurrent (GAME_FONT);
fontManager.SetColorRGBi (GREEN_RGBA, 1, 0, 0);
x = CCanvas::Current ()->Height ();
if (IsMultiGame)
	x -= 4 * m_info.nLineSpacing;
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
nIdWeapons [0] = GrPrintF (nIdWeapons + 0, CCanvas::Current ()->Width () - 5 - w, x-2*m_info.nLineSpacing, szWeapon);

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
nIdWeapons [1] = GrPrintF (nIdWeapons + 1, CCanvas::Current ()->Width ()-5-w, x-m_info.nLineSpacing, szWeapon);

if (LOCALPLAYER.secondaryAmmo [gameData.weapons.nSecondary] != oldAmmoCount [1][gameStates.render.vr.nCurrentPage]) {
	if (gameData.demo.nState == ND_STATE_RECORDING)
		NDRecordSecondaryAmmo (oldAmmoCount [1][gameStates.render.vr.nCurrentPage], LOCALPLAYER.secondaryAmmo [gameData.weapons.nSecondary]);
	oldAmmoCount [1][gameStates.render.vr.nCurrentPage] = LOCALPLAYER.secondaryAmmo [gameData.weapons.nSecondary];
	}
ShowBombCount (CCanvas::Current ()->Width ()- (3 * GAME_FONT->Width () + (gameStates.render.fonts.bHires?0:2)), x - 3 * m_info.nLineSpacing, -1, 1);
}

//	-----------------------------------------------------------------------------

void CHUD::DrawCloakInvul (void)
{

	static int nIdCloak = 0, nIdInvul = 0;

if (HIDE_HUD)
	return;
fontManager.SetColorRGBi (GREEN_RGBA, 1, 0, 0);

if (LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED) {
	int	x = CCanvas::Current ()->Height ();

	if (IsMultiGame)
		x -= 7 * m_info.nLineSpacing;
	else
		x -= 4 * m_info.nLineSpacing;
	if ((LOCALPLAYER.cloakTime+CLOAK_TIME_MAX - gameData.time.xGame > I2X (3)) || (gameData.time.xGame & 0x8000))
		nIdCloak = GrPrintF (&nIdCloak, 2, x, "%s", TXT_CLOAKED);
	}
if (LOCALPLAYER.flags & PLAYER_FLAGS_INVULNERABLE) {
	int	x = CCanvas::Current ()->Height ();

	if (IsMultiGame)
		x -= 10*m_info.nLineSpacing;
	else
		x -= 5*m_info.nLineSpacing;
	if (((LOCALPLAYER.invulnerableTime + INVULNERABLE_TIME_MAX - gameData.time.xGame) > I2X (4)) || (gameData.time.xGame & 0x8000))
		nIdInvul = GrPrintF (&nIdInvul, 2, x, "%s", TXT_INVULNERABLE);
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
	int x = CCanvas::Current ()->Height () - (IsMultiGame ? 6 : 2) * m_info.nLineSpacing;
	fontManager.SetCurrent (GAME_FONT);
	fontManager.SetColorRGBi (GREEN_RGBA, 1, 0, 0);
	nIdShield = GrPrintF (&nIdShield, 2, x, "%s: %i", TXT_SHIELD, h);
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
	int				h, x;

	static int nIdShield = 0;

//	CCanvas::SetCurrent (&gameStates.render.vr.buffers.subRender [0]);	//render off-screen
if (!gameOpts->render.cockpit.bTextGauges) {
	h = (LOCALPLAYER.shields >= 0) ? X2IR (LOCALPLAYER.shields) : 0;
	if ((t = FlashGauge (h, &gameStates.render.cockpit.nShieldFlash, (int) tToggle))) {
		tToggle = t;
		bShow = !bShow;
		}

	x = CCanvas::Current ()->Height () - (int) (((IsMultiGame ? 6 : 2) * m_info.nLineSpacing - 1) * yGaugeScale);
	CCanvas::Current ()->SetColorRGB (0, (ubyte) ((h > 100) ? 255 : 64), 255, 255);
	OglDrawEmptyRect (6, x, 6 + (int) (100 * xGaugeScale), x + (int) (9 * yGaugeScale));
	if (bShow) {
		CCanvas::Current ()->SetColorRGB (0, (ubyte) ((h > 100) ? 224 : 64), 224, 128);
		OglDrawFilledRect (6, x, 6 + (int) (((h > 100) ? h - 100 : h) * xGaugeScale), x + (int) (9 * yGaugeScale));
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
	bmP = HUDBitBlt (GAUGE_LIVES, 10, 3, false, false);
	nIdLives = GrPrintF (&nIdLives, 10 + bmP->Width () + bmP->Width () / 2, 4, "y %d", LOCALPLAYER.lives - 1);
	}
}

//	-----------------------------------------------------------------------------

void CHUD::DrawTime (void)
{
if (gameStates.render.bShowTime) {
		int secs = X2I (LOCALPLAYER.timeLevel) % 60;
		int mins = X2I (LOCALPLAYER.timeLevel) / 60;

		static int nIdTime = 0;

	fontManager.SetCurrent (GAME_FONT);
	fontManager.SetColorRGBi (GREEN_RGBA, 1, 0, 0);
	nIdTime = GrPrintF (&nIdTime, CCanvas::Current ()->Width () - 4 * GAME_FONT->Width (),
							  CCanvas::Current ()->Height () - 4 * m_info.nLineSpacing,
							  "%d:%02d", mins, secs);
	}
}

//	-----------------------------------------------------------------------------

void CHud::DrawPlayerShip (int nCloakState, int nOldCloakState, int y, int x)
{
}

//	-----------------------------------------------------------------------------

inline int NumDispX (int val)
{
int y = ((val > 99) ? 7 : (val > 9) ? 11 : 15);
if (!gameStates.video.nDisplayMode)
	y /= 2;
return y + NUMERICAL_GAUGE_X;
}

void DrawNumericalDisplay (int shield, int energy)
{
	int dx = NUMERICAL_GAUGE_X,
		 dy = NUMERICAL_GAUGE_Y;

	static int nIdShields = 0, nIdEnergy = 0;

if (gameStates.render.cockpit.nMode != CM_FULL_COCKPIT)
	CCanvas::SetCurrent (numericalGaugeCanv);
HUDBitBlt (GAUGE_NUMERICAL, dx, dy);
fontManager.SetCurrent (GAME_FONT);
fontManager.SetColorRGBi (RGBA_PAL2 (14, 14, 23), 1, 0, 0);
nIdShields = HUDPrintF (&nIdShields, NumDispX (shield), dy + (gameStates.video.nDisplayMode ? 36 : 16), "%d", shield);
fontManager.SetColorRGBi (RGBA_PAL2 (25, 18, 6), 1, 0, 0);
nIdEnergy = HUDPrintF (&nIdEnergy, NumDispX (energy), dy + (gameStates.video.nDisplayMode ? 5 : 2), "%d", energy);

if (gameStates.render.cockpit.nMode != CM_FULL_COCKPIT) {
	CCanvas::SetCurrent (GetCurrentGameScreen ());
	HUDBitBlt (-1, NUMERICAL_GAUGE_X, NUMERICAL_GAUGE_Y, true, true, I2X (1), 0, numericalGaugeCanv);
	}
}


//	-----------------------------------------------------------------------------

typedef struct tKeyGaugeInfo {
	int	nFlag, nGaugeOn, nGaugeOff, y [2], x [2];
} tKeyGaugeInfo;

static tKeyGaugeInfo keyGaugeInfo [] = {
	{PLAYER_FLAGS_BLUE_KEY, GAUGE_BLUE_KEY, GAUGE_BLUE_KEY_OFF, {GAUGE_BLUE_KEY_X_L, GAUGE_BLUE_KEY_X_H}, {GAUGE_BLUE_KEY_Y_L, GAUGE_BLUE_KEY_Y_H}},
	{PLAYER_FLAGS_GOLD_KEY, GAUGE_GOLD_KEY, GAUGE_GOLD_KEY_OFF, {GAUGE_GOLD_KEY_X_L, GAUGE_GOLD_KEY_X_H}, {GAUGE_GOLD_KEY_Y_L, GAUGE_GOLD_KEY_Y_H}},
	{PLAYER_FLAGS_RED_KEY, GAUGE_RED_KEY, GAUGE_RED_KEY_OFF, {GAUGE_RED_KEY_X_L, GAUGE_RED_KEY_X_H}, {GAUGE_RED_KEY_Y_L, GAUGE_RED_KEY_Y_H}},
	};

void DrawKeys (void)
{
CCanvas::SetCurrent (GetCurrentGameScreen ());
int bHires = gameStates.video.nDisplayMode != 0;
for (int i = 0; i < 3; i++)
	HUDBitBlt ((LOCALPLAYER.flags & keyGaugeInfo [i].nFlag) ? keyGaugeInfo [i].nGaugeOn : keyGaugeInfo [i].nGaugeOff, keyGaugeInfo [i].y [bHires], keyGaugeInfo [i].x [bHires]);
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
	nIdWeapon [1] = HUDPrintF (&nIdWeapon [1], xText, yText + CCanvas::Current ()->Font ()->Height () + 1, p + 1);
	}
else {
	nIdWeapon [2] = HUDPrintF (&nIdWeapon [2], xText, yText, pszName);
	}

//	For laser, show level and quadness
if (info_index == LASER_ID || info_index == SUPERLASER_ID) {
	sprintf (szName, "%s: 0", TXT_LVL);
	szName [5] = LOCALPLAYER.laserLevel + 1 + '0';
	nIdLaser [0] = HUDPrintF (&nIdLaser [0], xText, yText + m_info.nLineSpacing, szName);
	if (LOCALPLAYER.flags & PLAYER_FLAGS_QUAD_LASERS) {
		strcpy (szName, TXT_QUAD);
		nIdLaser [1] = HUDPrintF (&nIdLaser [1], xText, yText + 2 * m_info.nLineSpacing, szName);
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
	int h, y, x;

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
for (y = hudWindowAreas [h].left; y < hudWindowAreas [h].right; y += bmp->Width ())
	for (x = hudWindowAreas [h].top; x < hudWindowAreas [h].bot; x += bmp->Height ())
		HUDBitBlt (-1, y, x, true, true, I2X (1), 0, bmp);
CCanvas::SetCurrent (GetCurrentGameScreen ());
CopyGaugeBox (&hudWindowAreas [h], &gameStates.render.vr.buffers.render [0]);
}

//	-----------------------------------------------------------------------------

void DrawWeaponDisplays (void)
{
	int boxofs = (gameStates.render.cockpit.nMode == CM_STATUS_BAR) ? SB_PRIMARY_BOX : COCKPIT_PRIMARY_BOX;
	int bDrawn;

if (weaponBoxUser [0] == WBU_WEAPON) {
	bDrawn = DrawWeaponDisplay (0, gameData.weapons.nPrimary);
	if (bDrawn)
		CopyGaugeBox (hudWindowAreas+boxofs, &gameStates.render.vr.buffers.render [0]);

	if (weaponBoxStates [0] == WS_SET) {
		if ((gameData.weapons.nPrimary == VULCAN_INDEX) || (gameData.weapons.nPrimary == GAUSS_INDEX)) {
			if (LOCALPLAYER.primaryAmmo [VULCAN_INDEX] != oldAmmoCount [0][gameStates.render.vr.nCurrentPage]) {
				if (gameData.demo.nState == ND_STATE_RECORDING)
					NDRecordPrimaryAmmo (oldAmmoCount [0][gameStates.render.vr.nCurrentPage], LOCALPLAYER.primaryAmmo [VULCAN_INDEX]);
				DrawPrimaryAmmoInfo (X2I ((unsigned) VULCAN_AMMO_SCALE * (unsigned) LOCALPLAYER.primaryAmmo [VULCAN_INDEX]));
				oldAmmoCount [0][gameStates.render.vr.nCurrentPage] = LOCALPLAYER.primaryAmmo [VULCAN_INDEX];
				}
			}
		if (gameData.weapons.nPrimary == OMEGA_INDEX) {
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
	bDrawn = DrawWeaponDisplay (1, gameData.weapons.nSecondary);
	if (bDrawn)
		CopyGaugeBox (&hudWindowAreas [boxofs+1], &gameStates.render.vr.buffers.render [0]);

	if (weaponBoxStates [1] == WS_SET)
		if (LOCALPLAYER.secondaryAmmo [gameData.weapons.nSecondary] != oldAmmoCount [1][gameStates.render.vr.nCurrentPage]) {
			oldBombcount [gameStates.render.vr.nCurrentPage] = 0x7fff;	//force redraw
			if (gameData.demo.nState == ND_STATE_RECORDING)
				NDRecordSecondaryAmmo (oldAmmoCount [1][gameStates.render.vr.nCurrentPage], LOCALPLAYER.secondaryAmmo [gameData.weapons.nSecondary]);
			DrawSecondaryAmmoInfo (LOCALPLAYER.secondaryAmmo [gameData.weapons.nSecondary]);
			oldAmmoCount [1][gameStates.render.vr.nCurrentPage] = LOCALPLAYER.secondaryAmmo [gameData.weapons.nSecondary];
			}
	}
else if (weaponBoxUser [1] == WBU_STATIC)
	DrawStatic (1);
}

//	-----------------------------------------------------------------------------

//	Draws invulnerable ship, or maybe the flashing ship, depending on invulnerability time left.
void DrawInvulnerableShip (void)
{
	static fix time = 0;
	fix tInvul = LOCALPLAYER.invulnerableTime + INVULNERABLE_TIME_MAX - gameData.time.xGame;

CCanvas::SetCurrent (GetCurrentGameScreen ());
if (tInvul > 0) {
	if ((tInvul > I2X (4)) || (gameData.time.xGame & 0x8000)) {
		if (gameStates.render.cockpit.nMode == CM_STATUS_BAR)
			HUDBitBlt (GAUGE_INVULNERABLE + nInvulnerableFrame, SB_SHIELD_GAUGE_X, SB_SHIELD_GAUGE_Y);
		else
			HUDBitBlt (GAUGE_INVULNERABLE + nInvulnerableFrame, SHIELD_GAUGE_X, SHIELD_GAUGE_Y);
		time += gameData.time.xFrame;
		while (time > INV_FRAME_TIME) {
			time -= INV_FRAME_TIME;
			if (++nInvulnerableFrame == N_INVULNERABLE_FRAMES)
				nInvulnerableFrame = 0;
			}
		}
	}
else if (gameStates.render.cockpit.nMode == CM_STATUS_BAR)
	SBDrawShieldBar (X2IR (LOCALPLAYER.shields));
else
	DrawShieldBar (X2IR (LOCALPLAYER.shields));
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
	sbyte y, x;
} xy;

//offsets for reticle parts: high-big  high-sml  low-big  low-sml
xy cross_offsets [4] = 	 {{-8, -5},  {-4, -2},  {-4, -2}, {-2, -1}};
xy primary_offsets [4] =  {{-30, 14}, {-16, 6},  {-15, 6}, {-8, 2}};
xy secondary_offsets [4] = {{-24, 2},  {-12, 0}, {-12, 1}, {-6, -2}};

void OglDrawMouseIndicator (void);

//draw the reticle
void DrawReticle (int bForceBig)
{
	int y, x;
	int bLaserReady, bMissileReady, bLaserAmmo, bMissileAmmo;
	int nCrossBm, nPrimaryBm, nSecondaryBm;
	int bHiresReticle, bSmallReticle, ofs;

if ((gameData.demo.nState==ND_STATE_PLAYBACK) && gameData.demo.bFlyingGuided) {
	DrawGuidedCrosshair ();
	return;
	}

y = CCanvas::Current ()->Width () / 2;
x = CCanvas::Current ()->Height () / ((gameStates.render.cockpit.nMode == CM_FULL_COCKPIT) ? 2 : 2);
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
				  (y + HUD_SCALE_X (cross_offsets [ofs].y)), (x + HUD_SCALE_Y (cross_offsets [ofs].x)), false, true);
	HUDBitBlt ((bSmallReticle ? SML_RETICLE_PRIMARY : RETICLE_PRIMARY) + nPrimaryBm,
					(y + HUD_SCALE_X (primary_offsets [ofs].y)), (x + HUD_SCALE_Y (primary_offsets [ofs].x)), false, true);
	HUDBitBlt ((bSmallReticle ? SML_RETICLE_SECONDARY : RETICLE_SECONDARY) + nSecondaryBm,
					(y + HUD_SCALE_X (secondary_offsets [ofs].y)), (x + HUD_SCALE_Y (secondary_offsets [ofs].x)), false, true);
  }
if (!gameStates.app.bNostalgia && gameOpts->input.mouse.bJoystick && gameOpts->render.cockpit.bMouseIndicator)
	OglDrawMouseIndicator ();
cmScaleX /= HUD_ASPECT;
}

//	-----------------------------------------------------------------------------

void CHUD::DrawKillList (void)
{
	int n_players, player_list [MAX_NUM_NET_PLAYERS];
	int n_left, i, xo = 0, x0, x1, x, save_y, fth, l;
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
save_y = x = CCanvas::Current ()->Height () - n_left* (fth+1);
if (gameStates.render.cockpit.nMode == CM_FULL_COCKPIT) {
	save_y = x -= HUD_LHX (6);
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
			x = save_y;
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
	nIdKillList [0][i] = GrPrintF (nIdKillList [0] + i, x0 + indent, x, "%s", name);

	if (gameData.multigame.kills.bShowList == 2) {
		if (gameData.multiplayer.players [nPlayer].netKilledTotal + gameData.multiplayer.players [nPlayer].netKillsTotal <= 0)
			nIdKillList [1][i] = GrPrintF (nIdKillList [1] + i, x1, x, TXT_NOT_AVAIL);
		else
			nIdKillList [1][i] = GrPrintF (nIdKillList [1] + i, x1, x, "%d%%",
				 (int) ((double) gameData.multiplayer.players [nPlayer].netKillsTotal /
						 ((double) gameData.multiplayer.players [nPlayer].netKilledTotal +
						  (double) gameData.multiplayer.players [nPlayer].netKillsTotal) * 100.0));
		}
	else if (gameData.multigame.kills.bShowList == 3) {
		if (gameData.app.nGameMode & GM_ENTROPY)
			nIdKillList [1][i] = GrPrintF (nIdKillList [1] + i, x1, x, "%3d [%d/%d]",
						 gameData.multigame.kills.nTeam [i], gameData.entropy.nTeamRooms [i + 1],
						 gameData.entropy.nTotalRooms);
		else
			nIdKillList [1][i] = GrPrintF (nIdKillList [1] + i, x1, x, "%3d", gameData.multigame.kills.nTeam [i]);
		}
	else if (IsCoopGame)
		nIdKillList [1][i] = GrPrintF (nIdKillList [1] + i, x1, x, "%-6d", gameData.multiplayer.players [nPlayer].score);
   else if (netGame.xPlayTimeAllowed || netGame.KillGoal)
      nIdKillList [1][i] = GrPrintF (nIdKillList [1] + i, x1, x, "%3d (%d)",
					 gameData.multiplayer.players [nPlayer].netKillsTotal,
					 gameData.multiplayer.players [nPlayer].nKillGoalCount);
   else
		nIdKillList [1][i] = GrPrintF (nIdKillList [1] + i, x1, x, "%3d", gameData.multiplayer.players [nPlayer].netKillsTotal);
	if (gameStates.render.cockpit.bShowPingStats && (nPlayer != gameData.multiplayer.nLocalPlayer)) {
		if (bGetPing)
			PingPlayer (nPlayer);
		if (pingStats [nPlayer].sent) {
#if 0//def _DEBUG
			nIdKillList [1][i] = GrPrintF (nIdKillList [1] + i, x1 + xo, x, "%lu %d %d",
						  pingStats [nPlayer].ping,
						  pingStats [nPlayer].sent,
						  pingStats [nPlayer].received);
#else
			nIdKillList [1][i] = GrPrintF (nIdKillList [1] + i, x1 + xo, x, "%lu %i%%", pingStats [nPlayer].ping,
						 100 - ((pingStats [nPlayer].received * 100) / pingStats [nPlayer].sent));
#endif
			}
		}
	x += fth+1;
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
				fix y = vPlayerPos.p3_screen.y;
				fix x = vPlayerPos.p3_screen.x;
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
					x1 = y - w / 2;
					y1 = x - h / 2;
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
//					fix dy = -FixMulDiv (FixMul (OBJECTS [nObject].size, transformation.m_info.scale.x), I2X (CCanvas::Current ()->Height ())/2, vPlayerPos.p3_z);
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
					x0 = y - dx;
					x1 = y + dx;
					y0 = x - dy;
					y1 = x + dy;
					// draw the edges of a rectangle around the player (not a complete rectangle)
#if 1
					// draw the complete rectangle
					OglDrawEmptyRect (x0, y0, x1, y1);
					// now partially erase its sides
					CCanvas::Current ()->SetColorRGBi (0);
					OglDrawLine (y - w, y0, y + w, y0);
					OglDrawLine (y - w, y1, y + w, y1);
					OglDrawLine (x0, x - h, x0, x + h);
					OglDrawLine (x1, x - h, x1, x + h);
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
void DrawHUD (void)
{
if (HIDE_HUD)
	return;
if (gameStates.render.cockpit.nMode == CM_STATUS_BAR) {
	gameStates.render.cockpit.nLastDrawn [0] =
	gameStates.render.cockpit.nLastDrawn [1] = -1;
	InitGauges ();
//	vr_reset_display ();
  }
SetCMScales ();
m_info.nLineSpacing = GAME_FONT->Height () + GAME_FONT->Height ()/4;

	//	Show score so long as not in rearview
if (!(gameStates.render.bRearView || bSavingMovieFrames ||
	 (gameStates.render.cockpit.nMode == CM_REAR_VIEW) ||
	 (gameStates.render.cockpit.nMode == CM_STATUS_BAR))) {
	CHUD::DrawScore ();
	if (scoreTime)
		CHUD::DrawScoreAdded ();
	}

if (!(gameStates.render.bRearView || bSavingMovieFrames || (gameStates.render.cockpit.nMode == CM_REAR_VIEW)))
	CHUD::DrawTimerCount ();

//	Show other stuff if not in rearview or letterbox.
if (!gameStates.render.bRearView && (gameStates.render.cockpit.nMode != CM_REAR_VIEW)) {
	if ((gameStates.render.cockpit.nMode == CM_STATUS_BAR) || (gameStates.render.cockpit.nMode == CM_FULL_SCREEN))
		CHUD::DrawHomingWarning ();

	if ((gameStates.render.cockpit.nMode == CM_FULL_SCREEN) || (gameStates.render.cockpit.nMode == CM_LETTERBOX)) {
		if (!gameOpts->render.cockpit.bTextGauges) {
			if (gameOpts->render.cockpit.bScaleGauges) {
				xGaugeScale = (double) CCanvas::Current ()->Height () / 480.0;
				yGaugeScale = (double) CCanvas::Current ()->Height () / 640.0;
				}
			else
				xGaugeScale = yGaugeScale = 1;
			}
		CHUD::DrawEnergy ();
		CHUD::DrawShield ();
		CHUD::DrawAfterburner ();
		CHUD::DrawWeapons ();
		if (!bSavingMovieFrames)
			CHUD::DrawKeys ();
		CHUD::DrawCloakInvul ();

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

	ShowHUDNames ();
	if (gameStates.render.cockpit.nMode != CM_REAR_VIEW) {
		CHUD::DrawFlag ();
		CHUD::DrawOrbs ();
		}
	if (!bSavingMovieFrames)
		HUDRenderMessageFrame ();

	if (gameStates.render.cockpit.nMode!=CM_STATUS_BAR && !bSavingMovieFrames)
		CHUD::DrawLives ();

	if (gameData.app.nGameMode&GM_MULTI && gameData.multigame.kills.bShowList)
		CHUD::DrawKillList ();
#if 0
	ShowWeaponDisplayes ();
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
void RenderGauges (void)
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

ShowWeaponDisplayes ();
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

static int cockpitWindowScale [4] = {6, 5, 4, 3};

void CHUD::SetupWindow (int nWindow)
{
int x, y;
int w = (int) (gameStates.render.vr.buffers.render [0].Width () / cockpitWindowScale [gameOpts->render.cockpit.nWindowSize] * HUD_ASPECT);	
int h = I2X (w) / screen.Aspect ();
int dx = (nWindow == 0) ? -w - w / 10 : w / 10;
switch (gameOpts->render.cockpit.nWindowPos) {
	case 0:
		x = nWindow ?
			gameStates.render.vr.buffers.render [0].Width () - w - h / 10 :
			h / 10;
		y = gameStates.render.vr.buffers.render [0].Height () - h - h / 10;
		break;
	case 1:
		x = nWindow ?
			gameStates.render.vr.buffers.render [0].Width () / 3 * 2 - w / 3 :
			gameStates.render.vr.buffers.render [0].Width () / 3 - 2 * w / 3;
		y = gameStates.render.vr.buffers.render [0].Height () - h - h / 10;
		break;
	case 2:	// only makes sense if there's only one cockpit window
		x = gameStates.render.vr.buffers.render [0].Width () / 2 - w / 2;
		y = gameStates.render.vr.buffers.render [0].Height () - h - h / 10;
		break;
	case 3:
		x = nWindow ?
			gameStates.render.vr.buffers.render [0].Width () - w - h / 10 :
			h / 10;
		y = h / 10;
		break;
	case 4:
		x = nWindow ?
			gameStates.render.vr.buffers.render [0].Width () / 3 * 2 - w / 3 :
			gameStates.render.vr.buffers.render [0].Width () / 3 - 2 * w / 3;
		y = h / 10;
		break;
	case 5:	// only makes sense if there's only one cockpit window
		x = gameStates.render.vr.buffers.render [0].Width () / 2 - w / 2;
		y = h / 10;
		break;
	}
if ((gameOpts->render.cockpit.nWindowPos < 3) &&
		extraGameInfo [0].nWeaponIcons &&
		(extraGameInfo [0].nWeaponIcons - gameOpts->render.weaponIcons.bEquipment < 3))
		y -= (int) ((gameOpts->render.weaponIcons.bSmall ? 20.0 : 30.0) * (double) CCanvas::Current ()->Height () / 480.0);

//copy these vars so stereo code can get at them
SW_drawn [nWindow] = 1;
SW_x [nWindow] = y;
SW_y [nWindow] = x;
SW_w [nWindow] = w;
SW_h [nWindow] = h;

gameStates.render.vr.buffers.render [0].SetupPane (&windowCanv, x, y, w, h);
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
