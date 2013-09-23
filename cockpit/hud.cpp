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
#include "playerprofile.h"
#include "cockpit.h"
#include "hud_defs.h"
#include "statusbar.h"
#include "slowmotion.h"
#include "automap.h"
#include "hudicons.h"
#include "gr.h"

//	-----------------------------------------------------------------------------

void CHUD::GetHostageWindowCoords (int& x, int& y, int& w, int& h)
{
x = SECONDARY_W_BOX_LEFT;
y = SECONDARY_W_BOX_TOP;
w = SECONDARY_W_BOX_RIGHT - SECONDARY_W_BOX_LEFT + 1;
h = SECONDARY_W_BOX_BOT - SECONDARY_W_BOX_TOP + 1;
}

//	-----------------------------------------------------------------------------

void CHUD::DrawRecording (void)
{
CGenericCockpit::DrawRecording (0);
}

//	-----------------------------------------------------------------------------

void CHUD::DrawCountdown (void)
{
CGenericCockpit::DrawCountdown (SMALL_FONT->Height () * 4);
}

//	-----------------------------------------------------------------------------

void CHUD::DrawCruise (void)
{
CGenericCockpit::DrawCruise (3, -(IsMultiGame ? 11 : 6) *  LineSpacing ());
}

//	-----------------------------------------------------------------------------

void CHUD::DrawScore (void)
{
if (cockpit->Hide ())
	return;

	char	szScore [40];
	int	w, h, aw;

if ((gameData.hud.msgs [0].nMessages > 0) &&
	 (strlen (gameData.hud.msgs [0].szMsgs [gameData.hud.msgs [0].nFirst]) > 38))
	return;
if ((IsMultiGame && !IsCoopGame))
	sprintf (szScore, "   %s: %5d", TXT_KILLS, LOCALPLAYER.netKillsTotal);
else
	sprintf (szScore, "   %s: %5d", TXT_SCORE, LOCALPLAYER.score);
fontManager.Current ()->StringSize (szScore, w, h, aw);
SetFontColor (GREEN_RGBA);
DrawHUDText (NULL, -w - LHX (2), 3, szScore);
}

//	-----------------------------------------------------------------------------

 void CHUD::DrawAddedScore (void)
{
if (cockpit->Hide ())
	return;

	int	color;
	int	w, h, aw, nScore, nTime;
	char	szScore [20];

	static int nIdTotalScore = 0;

if (IsMultiGame && !IsCoopGame)
	return;
if (!(nScore = cockpit->AddedScore ()))
	return;
cockpit->SetScoreTime (nTime = cockpit->ScoreTime () - gameData.time.xFrame);
if (nTime > 0) {
	color = X2I (nTime * 20) + 10;
	if (color < 10)
		color = 10;
	else if (color > 31)
		color = 30;
	color = color - (color % 4);	//	Only allowing colors 12, 16, 20, 24, 28 speeds up gr_getcolor, improves caching
	if (gameStates.app.cheats.bEnabled)
		sprintf (szScore, "%s", TXT_CHEATER);
	else
		sprintf (szScore, "%5d", nScore);
	fontManager.Current ()->StringSize (szScore, w, h, aw);
	SetFontColor (RGBA_PAL2 (0, color, 0));
	nIdTotalScore = DrawHUDText (&nIdTotalScore, -w - LHX (12), LineSpacing () + 4, szScore);
	}
else {
	cockpit->SetScoreTime (0);
	cockpit->SetAddedScore (0, 0);
	}
}

//	-----------------------------------------------------------------------------

void CHUD::DrawHomingWarning (void)
{
if (cockpit->Hide ())
	return;

	static int nIdLock = 0;

if ((LOCALPLAYER.homingObjectDist >= 0) && (gameData.time.xGame & 0x4000)) {
	int	x, y, nOffsetSave = -1;

	if (ogl.IsOculusRift ()) {
		int w, h, aw;
		fontManager.Current ()->StringSize (TXT_LOCK, w, h, aw);
		x = CCanvas::Current ()->Width () / 2 - w / 2;
		y = AdjustCockpitY (-2 * LineSpacing ());
		//y = CCanvas::Current ()->Height () / 2 + 4 * h;
		nOffsetSave = gameData.SetStereoOffsetType (STEREO_OFFSET_FIXED);
		}
	else {
		x = 0x8000;
		y = CCanvas::Current ()->Height () - LineSpacing ();
		if ((hudIcons.Visible () && (extraGameInfo [0].nWeaponIcons == 2)) ||
			(hudIcons.Inventory () && (extraGameInfo [0].nWeaponIcons & 1)))
			y -= LHY (20);
		m_info.bAdjustCoords = true;
		}
	if ((m_info.weaponBoxUser [0] != WBU_WEAPON) || (m_info.weaponBoxUser [1] != WBU_WEAPON)) {
		int wy = (m_info.weaponBoxUser [0] != WBU_WEAPON) ? SW_y [0] : SW_y [1];
		y = min (y, (wy - LineSpacing () - gameData.render.frame.Top ()));
		}
	SetFontColor (RED_RGBA);
	nIdLock = DrawHUDText (&nIdLock, x, y, TXT_LOCK);
	gameData.SetStereoOffsetType (nOffsetSave);
	}
}

//	-----------------------------------------------------------------------------

void CHUD::DrawKeys (void)
{
if (cockpit->Hide ())
	return;
if (IsMultiGame && !IsCoopGame)
	return;

	int	x, y, dx = GAME_FONT->Width () + GAME_FONT->Width () / 2;

if (ogl.IsOculusRift ()) {
	x = 3 * CCanvas::Current ()->Width () / 4 - 4 * dx;
	y = CCanvas::Current ()->Height () + AdjustCockpitY (-2 * LineSpacing ());
	}
else {
	x = 2;
	y = 3 * LineSpacing ();
	}

int nOffsetSave = gameData.SetStereoOffsetType (STEREO_OFFSET_FIXED);
if (LOCALPLAYER.flags & PLAYER_FLAGS_BLUE_KEY)
	BitBlt (KEY_ICON_BLUE, x, y, false, false);
if (LOCALPLAYER.flags & PLAYER_FLAGS_GOLD_KEY) 
	BitBlt (KEY_ICON_YELLOW, x + dx, y, false, false);
if (LOCALPLAYER.flags & PLAYER_FLAGS_RED_KEY)
	BitBlt (KEY_ICON_RED, x + 2 * dx, y, false, false);
gameData.SetStereoOffsetType (nOffsetSave);
}

//	-----------------------------------------------------------------------------

void CHUD::DrawOrbs (void)
{
if (cockpit->Hide ())
	return;
CGenericCockpit::DrawOrbs (m_info.fontWidth, LineSpacing () * (gameStates.render.fonts.bHires + 1));
}

//	-----------------------------------------------------------------------------

void CHUD::DrawFlag (void)
{
if (cockpit->Hide ())
	return;
CGenericCockpit::DrawFlag (5 * LineSpacing (), LineSpacing () * (gameStates.render.fonts.bHires + 1));
}

//	-----------------------------------------------------------------------------

int CHUD::FlashGauge (int h, int *bFlash, int tToggle)
{
	time_t t = gameStates.app.nSDLTicks [0];
	int b = *bFlash;

if (gameOpts->app.bEpilepticFriendly || gameStates.app.bPlayerIsDead || gameData.multiplayer.players [N_LOCALPLAYER].m_bExploded)
	b = 0;
else {
	if (!b)
		tToggle = -1;
	if (h > 20)
		b = 0;
	else if (h > 10)
		b = 1;
	else
		b = 2;
	}
*bFlash = b;
return (int) ((b && (tToggle <= t)) ? t + 300 / b : 0);
}

//	-----------------------------------------------------------------------------

void CHUD::DrawShieldText (void)
{
if (cockpit->Hide ())
	return;

	static int nIdShield = 0;

if (ShowTextGauges ()) {
	int y = IsMultiGame ? -6 * LineSpacing () : -2 * LineSpacing ();
	SetFontColor (GREEN_RGBA);
	m_info.bAdjustCoords = true;
	nIdShield = DrawHUDText (&nIdShield, 2, y, "%s: %i", TXT_SHIELD, (int) FRound (m_info.nShield * LOCALPLAYER.ShieldScale ()));
	}
}

//	-----------------------------------------------------------------------------

void CHUD::DrawShieldBar (void)
{
if (cockpit->Hide ())
	return;

	static int		bShow = 1;
	static time_t	tToggle = 0, nBeep = -1;
	//static int		nIdLevel = 0;

	time_t			t = gameStates.app.nSDLTicks [0];
	int				bLastFlash = gameStates.render.cockpit.nShieldFlash;

if (!ShowTextGauges ()) {

	int nLevel = m_info.nShield;
	if ((t = FlashGauge (nLevel, &gameStates.render.cockpit.nShieldFlash, (int) tToggle))) {
		tToggle = t;
		bShow = !bShow;
		}

	int nLineSpacing = 5 * GAME_FONT->Height () / 4;
	int h = int (9 * m_info.yGaugeScale), 
		 w = int (9 * m_info.xGaugeScale), 
		 y = -(int) (((IsMultiGame ? 6 : 2) * nLineSpacing - 1) * m_info.yGaugeScale);
	if (hudIcons.LoadGaugeIcons () > 0)
		hudIcons.GaugeIcon (0).RenderScaled (6, y, w, h);
	int x = 6 + int (10 * m_info.xGaugeScale);
	w = (nLevel > 100) ? 100 : 50;
	CCanvas::Current ()->SetColorRGB (0, 64, 224, 255);
	glLineWidth (1);
	OglDrawEmptyRect (x, y, x + int (w * m_info.xGaugeScale), y + h);
	if (bShow) {
		CCanvas::Current ()->SetColorRGB (0, 64, 224, 128);
		if (nLevel <= 100)
			OglDrawFilledRect (x, y, x + int (nLevel * m_info.xGaugeScale / 2.0f), y + h);
		else {
			w = int (50 * m_info.xGaugeScale);
			OglDrawFilledRect (x, y, x + w, y + h);
			while (nLevel > 100)
				nLevel -= 100;
			CCanvas::Current ()->SetColorRGB (0, 224, 224, 128);
			OglDrawFilledRect (x + w, y, x + w + int (nLevel * m_info.xGaugeScale / 2.0f), y + h);
			}
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

void CHUD::DrawEnergyText (void)
{
if (cockpit->Hide ())
	return;

	int h, y;
	static int nIdEnergy = 0;

h = LOCALPLAYER.Energy () ? X2IR (LOCALPLAYER.Energy ()) : 0;
if (ShowTextGauges ()) {
	y = IsMultiGame ? -5 * LineSpacing () : -LineSpacing ();
	SetFontColor (GREEN_RGBA);
	m_info.bAdjustCoords = true;
	nIdEnergy = DrawHUDText (&nIdEnergy, 2, y, "%s: %i", TXT_ENERGY, h);
	}
if (gameData.demo.nState == ND_STATE_RECORDING) {
	int energy = X2IR (LOCALPLAYER.Energy ());

	if (energy != m_history [0].energy) {
		NDRecordPlayerEnergy (m_history [0].energy, energy);
		m_history [0].energy = energy;
	 	}
	}
}

//	-----------------------------------------------------------------------------

void CHUD::DrawEnergyBar (void)
{
if (cockpit->Hide ())
	return;

if (!ShowTextGauges ()) {
	static int		bFlash = 0, bShow = 1;
	static time_t	tToggle;
	//static int		nIdLevel = 0;
	time_t			t;

	int nLevel = m_info.nEnergy;
	int nLineSpacing = 5 * GAME_FONT->Height () / 4;
	if ((t = FlashGauge (nLevel, &bFlash, (int) tToggle))) {
		tToggle = t;
		bShow = !bShow;
		}
	int h = int (9 * m_info.yGaugeScale), 
		 w = int (9 * m_info.xGaugeScale), 
		 y = - (int) (((IsMultiGame ? 5 : 1) * nLineSpacing - 1) * m_info.yGaugeScale);
	if (hudIcons.LoadGaugeIcons () > 0)
		hudIcons.GaugeIcon (1).RenderScaled (6, y, w, h);
	CCanvas::Current ()->SetColorRGB (255, 224, 0, 255);
	glLineWidth (1);
	int x = 6 + int (10 * m_info.xGaugeScale);
	w = (nLevel > 100) ? 100 : 50;
	OglDrawEmptyRect (x, y, x + int (w * m_info.xGaugeScale), y + h);
	if (bFlash) {
		if (!bShow)
			return;
		nLevel = 100;
		}
	else
		bShow = 1;
	CCanvas::Current ()->SetColorRGB (255, 224, 0, 128);
	if (nLevel <= 100)
		OglDrawFilledRect (x, y, x + int (nLevel * m_info.xGaugeScale / 2.0f), y + h);
	else {
		w = int (50 * m_info.xGaugeScale);
		OglDrawFilledRect (x, y, x + w, y + h);
		while (nLevel > 100)
			nLevel -= 100;
		CCanvas::Current ()->SetColorRGB (255, 240, 240, 128);
		OglDrawFilledRect (x + w, y, x + w + int (nLevel * m_info.xGaugeScale / 2.0f), y + h);
		}
	}
}

//	-----------------------------------------------------------------------------

void CHUD::DrawAfterburnerText (void)
{
if (cockpit->Hide ())
	return;
	
	int h, y;
	static int nIdAfterBurner = 0;

if (!(LOCALPLAYER.flags & PLAYER_FLAGS_AFTERBURNER))
	return;		//don't draw if don't have
h = FixMul (gameData.physics.xAfterburnerCharge, 100);
if (ShowTextGauges ()) {
	y = -(IsMultiGame ? 8 : 3) * LineSpacing ();
	SetFontColor (GREEN_RGBA);
	m_info.bAdjustCoords = true;
	nIdAfterBurner = DrawHUDText (&nIdAfterBurner, 2, y, TXT_HUD_BURN, h);
	}
}

//	-----------------------------------------------------------------------------

void CHUD::DrawAfterburnerBar (void)
{
if (cockpit->Hide ())
	return;

	//static int nIdLevel = 0;
	
if (!(LOCALPLAYER.flags & PLAYER_FLAGS_AFTERBURNER))
	return;		//don't draw if don't have
int nLevel = FixMul (gameData.physics.xAfterburnerCharge, 100);
int nLineSpacing = 5 * GAME_FONT->Height () / 4;
if (!ShowTextGauges ()) {
	int h = int (9 * m_info.yGaugeScale), 
		 w = int (9 * m_info.xGaugeScale), 
		 y = -(int) (((IsMultiGame ? 8 : 3) * nLineSpacing - 1) * m_info.yGaugeScale);
	if (hudIcons.LoadGaugeIcons () > 0)
		hudIcons.GaugeIcon (2).RenderScaled (6, y, w = int (9 * m_info.xGaugeScale), h = int (9 * m_info.yGaugeScale));
	CCanvas::Current ()->SetColorRGB (255, 0, 0, 255);
	glLineWidth (1);
	int x = 6 + int (10 * m_info.xGaugeScale);
	OglDrawEmptyRect (x, y, x + (int) (50 * m_info.xGaugeScale), y + h);
	CCanvas::Current ()->SetColorRGB (224, 0, 0, 128);
	OglDrawFilledRect (x, y, x + (int) (nLevel * m_info.xGaugeScale / 2.0f), y + h);
	}
if (gameData.demo.nState == ND_STATE_RECORDING) {
	if (gameData.physics.xAfterburnerCharge != m_history [0].afterburner) {
		NDRecordPlayerAfterburner (m_history [0].afterburner, gameData.physics.xAfterburnerCharge);
		m_history [0].afterburner = gameData.physics.xAfterburnerCharge;
	 	}
	}
}

//	-----------------------------------------------------------------------------

void CHUD::DrawEnergyLevelsCombined (void)
{
	int	y = AdjustCockpitY (-2 * LineSpacing ());
	int	x = CCanvas::Current ()->Width () / 4;
	int	nOffsetSave = gameData.SetStereoOffsetType (STEREO_OFFSET_FIXED);

	static uint	energyColors [3] = { GOLD_RGBA, BLUE_RGBA, RED_RGBA };

int				i, nColor, h [4], w [4], aw [4], tw = 0;
int				nEnergy [3] = {LOCALPLAYER.Energy () ? X2IR (LOCALPLAYER.Energy ()) : 0, 
										(int) FRound (m_info.nShield * LOCALPLAYER.ShieldScale ()), 
										(LOCALPLAYER.flags & PLAYER_FLAGS_AFTERBURNER) ? FixMul (gameData.physics.xAfterburnerCharge, 100) : -1
									  };
char				szEnergy [3][10];
CCanvasColor	color;

color.Set (0, 0, 0, 128);
color.index = -1;
color.rgb = 1;

CCanvas::Current ()->SetFont (GAME_FONT);
for (i = 0; i < 3; i++) {
	if (szEnergy [i] < 0)
		strcpy (szEnergy [i], " ---");
	else
		sprintf (szEnergy [i], "%4d", nEnergy [i]);
	fontManager.Current ()->StringSize (szEnergy [i], w [i], h [i], aw [i]);
	tw += w [i];
	}
fontManager.Current ()->StringSize (" ", w [3], h [3], aw [3]);
CCanvas::Current ()->SetFontColor (color, 1);	// black background
for (i = 0; i < 3; i++) {
	if (nEnergy [i] >= 0) {
		nColor = energyColors [i];
		//color.Set (RGBA_RED (nColor), RGBA_GREEN (nColor), RGBA_BLUE (nColor));
		//SetCanvas (&gameData.render.frame);
		SetFontColor (nColor);
		szEnergy [i][4] = '\0'; 
		DrawHUDText (NULL, x, y, szEnergy [i]);
		x += w [i];
		}
	}
fontManager.SetScale (1.0f);
gameData.SetStereoOffsetType (nOffsetSave);
}

//	-----------------------------------------------------------------------------

void CHUD::DrawEnergyLevels (void)
{
#if DBG
if (ogl.IsSideBySideDevice ())
#else
if (ogl.IsOculusRift ())
#endif
	DrawEnergyLevelsCombined ();
else
	CGenericCockpit::DrawEnergyLevels ();
}

//	-----------------------------------------------------------------------------

#define SECONDARY_WEAPON_NAMES_VERY_SHORT(nWeaponId) 			\
	 ((nWeaponId <= MEGA_INDEX)?GAMETEXT (541 + nWeaponId):	\
	 GT (636 + nWeaponId - FLASHMSL_INDEX))

//return which bomb will be dropped next time the bomb key is pressed
int ArmedBomb ();

//	-----------------------------------------------------------------------------

void CHUD::ClearBombCount (int bgColor)
{
}

//	-----------------------------------------------------------------------------

void CHUD::DrawBombCount (void)
{
CGenericCockpit::DrawBombCount (0, 0, BLACK_RGBA, 0);
}

//	-----------------------------------------------------------------------------

int CHUD::DrawBombCount (int& nIdBombCount, int x, int y, int nColor, char* pszBombCount)
{
SetFontColor (nColor);
x = gameData.render.scene.Width () - 3 * GAME_FONT->Width () - gameStates.render.fonts.bHires - 1;
y = gameData.render.scene.Height () - 3 * LineSpacing ();
if ((extraGameInfo [0].nWeaponIcons >= 3) && (gameData.render.scene.Height () < 670))
	x -= LHX (20);
int nOffsetSave = AdjustCockpitXY (pszBombCount, x, y);
return GrString (x, y, pszBombCount, &nIdBombCount);
gameData.SetStereoOffsetType (nOffsetSave);
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
void CHUD::DrawWeapons (void)
{
if (cockpit->Hide ())
	return;
if (ogl.IsOculusRift () && EGI_FLAG (nWeaponIcons, 1, 1, 0))
	return;

	int	w, h, aw;
	int	y;
	const char	*pszWeapon;
	char	szWeapon [32];

	static int nIdWeapons [2] = {0, 0};

SetFontColor (GREEN_RGBA);
y = IsMultiGame ? -4 * LineSpacing () : 0;
pszWeapon = PRIMARY_WEAPON_NAMES_SHORT (gameData.weapons.nPrimary);
switch (gameData.weapons.nPrimary) {
	case LASER_INDEX:
		if (LOCALPLAYER.flags & PLAYER_FLAGS_QUAD_LASERS)
			sprintf (szWeapon, "%s %s %i", TXT_QUAD, pszWeapon, LOCALPLAYER.LaserLevel () + 1);
		else
			sprintf (szWeapon, "%s %i", pszWeapon, LOCALPLAYER.LaserLevel () + 1);
		break;

	case SUPER_LASER_INDEX:
		Int3 ();
		break;	//no such thing as super laser

	case VULCAN_INDEX:
	case GAUSS_INDEX:
		sprintf (szWeapon, "%s: %i", pszWeapon, X2I ((unsigned) LOCALPLAYER.primaryAmmo [VULCAN_INDEX] * (unsigned) VULCAN_AMMO_SCALE));
		Convert1s (szWeapon);
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
		Convert1s (szWeapon);
		break;

	default:
		Int3 ();
		szWeapon [0] = 0;
		break;
	}

fontManager.Current ()->StringSize (szWeapon, w, h, aw);
m_info.bAdjustCoords = true;
nIdWeapons [0] = DrawHUDText (nIdWeapons + 0, -5 - w, y - 2 * LineSpacing (), szWeapon);

if (gameData.weapons.nPrimary == VULCAN_INDEX) {
	if (LOCALPLAYER.primaryAmmo [gameData.weapons.nPrimary] != m_history [0].ammo [0]) {
		if (gameData.demo.nState == ND_STATE_RECORDING)
			NDRecordPrimaryAmmo (m_history [0].ammo [0], LOCALPLAYER.primaryAmmo [gameData.weapons.nPrimary]);
		m_history [0].ammo [0] = LOCALPLAYER.primaryAmmo [gameData.weapons.nPrimary];
		}
	}

if (gameData.weapons.nPrimary == OMEGA_INDEX) {
	if (gameData.omega.xCharge [IsMultiGame] != m_history [0].xOmegaCharge) {
		if (gameData.demo.nState == ND_STATE_RECORDING)
			NDRecordPrimaryAmmo (m_history [0].xOmegaCharge, gameData.omega.xCharge [IsMultiGame]);
		m_history [0].xOmegaCharge = gameData.omega.xCharge [IsMultiGame];
		}
	}

pszWeapon = SECONDARY_WEAPON_NAMES_VERY_SHORT (gameData.weapons.nSecondary);
sprintf (szWeapon, "%s %d", pszWeapon, LOCALPLAYER.secondaryAmmo [gameData.weapons.nSecondary]);
fontManager.Current ()->StringSize (szWeapon, w, h, aw);
m_info.bAdjustCoords = true;
nIdWeapons [1] = DrawHUDText (nIdWeapons + 1, -5 - w, y - LineSpacing (), szWeapon);

if (LOCALPLAYER.secondaryAmmo [gameData.weapons.nSecondary] != m_history [0].ammo [1]) {
	if (gameData.demo.nState == ND_STATE_RECORDING)
		NDRecordSecondaryAmmo (m_history [0].ammo [1], LOCALPLAYER.secondaryAmmo [gameData.weapons.nSecondary]);
	m_history [0].ammo [1] = LOCALPLAYER.secondaryAmmo [gameData.weapons.nSecondary];
	}
}

//	-----------------------------------------------------------------------------

void CHUD::DrawInvul (void)
{
if (cockpit->Hide ())
	return;

	static int nIdInvul = 0;

if ((LOCALPLAYER.flags & PLAYER_FLAGS_INVULNERABLE) &&
	 (((LOCALPLAYER.invulnerableTime + INVULNERABLE_TIME_MAX - gameData.time.xGame) > I2X (4)) || (gameData.time.xGame & 0x8000))) {
	 int y = IsMultiGame ? -10 * LineSpacing () : -5 * LineSpacing ();
	if ((LOCALPLAYER.flags & PLAYER_FLAGS_AFTERBURNER) && !ShowTextGauges ())
		y -= LineSpacing () + gameStates.render.fonts.bHires + 1;
	SetFontColor (GREEN_RGBA);
	nIdInvul = DrawHUDText (&nIdInvul, 2, y, "%s", TXT_INVULNERABLE);
	}
}

//	-----------------------------------------------------------------------------

void CHUD::DrawCloak (void)
{
if (cockpit->Hide ())
	return;

	static int nIdCloak = 0;


if ((LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED) &&
	 ((LOCALPLAYER.cloakTime + CLOAK_TIME_MAX - gameData.time.xGame > I2X (3)) || (gameData.time.xGame & 0x8000))) {
	int y = IsMultiGame ? -7 * LineSpacing () : -4 * LineSpacing ();
	if ((LOCALPLAYER.flags & PLAYER_FLAGS_AFTERBURNER) && !ShowTextGauges ())
		y -= LineSpacing () + gameStates.render.fonts.bHires + 1;
	SetFontColor (GREEN_RGBA);
	nIdCloak = DrawHUDText (&nIdCloak, 2, y, "%s", TXT_CLOAKED);
	}
}

//	-----------------------------------------------------------------------------

//draw the icons for number of lives
void CHUD::DrawLives (void)
{
	static int nIdLives = 0;

if (cockpit->Hide ())
	return;
if ((gameData.hud.msgs [0].nMessages > 0) && (strlen (gameData.hud.msgs [0].szMsgs [gameData.hud.msgs [0].nFirst]) > 38))
	return;
if (IsMultiGame) {
	SetFontColor (GREEN_RGBA);
	nIdLives = DrawHUDText (&nIdLives, 10, 3, "%s: %d", TXT_DEATHS, LOCALPLAYER.netKilledTotal);
	}
else if (LOCALPLAYER.lives > 1)  {
	//m_info.nColor = WHITE_RGBA;
	//CCanvas::Push ();
	//CCanvas::SetCurrent (&gameData.render.scene);
	CBitmap* bmP = BitBlt (GAUGE_LIVES, 10, 3, false, false);
	//CCanvas::Pop ();
	SetFontColor (MEDGREEN_RGBA);
	nIdLives = DrawHUDText (&nIdLives, 10 + bmP->Width () + bmP->Width () / 2, 4, "x %d", LOCALPLAYER.lives - 1);
	}
}

//	-----------------------------------------------------------------------------

void CHUD::DrawStatic (int nWindow)
{
}

//	-----------------------------------------------------------------------------

void CHUD::DrawKillList (void)
{
CGenericCockpit::DrawKillList (60, CCanvas::Current ()->Height ());
}

//	-----------------------------------------------------------------------------

void CHUD::DrawCockpit (bool bAlphaTest)
{
//gameData.render.frame.SetLeft ((screen.Width () - gameData.render.frame.Width ()) / 2);
//gameData.render.frame.SetTop ((screen.Height () - gameData.render.frame.Height ()) / 2);
}

//	-----------------------------------------------------------------------------

bool CHUD::Setup (bool bRebuild)
{
if (bRebuild && !m_info.bRebuild)
	return true;
m_info.bRebuild = false;
if (!CGenericCockpit::Setup ())
	return false;
Canvas ()->Activate ();
//GameInitRenderSubBuffers (gameData.render.frame.Left (), gameData.render.frame.Top (), gameData.render.frame.Width (), gameData.render.frame.Height ());
return true;
}

//	-----------------------------------------------------------------------------

void CHUD::SetupWindow (int nWindow)
{
	static int cockpitWindowScale [4] = {8, 6, 4, 3};

	int x, y, nWindowPos;

int w = (int) (gameData.render.frame.Width () / cockpitWindowScale [gameOpts->render.cockpit.nWindowSize] * HUD_ASPECT);	
int h = I2X (w) / screen.Aspect ();
if (!gameStates.app.bNostalgia) //(gameOpts->render.cockpit.bWideDisplays)
	w = int (float (w) * float (screen.Width ()) / float (screen.Height ()) * 0.75f);
	//w = int (w / HUD_ASPECT);
nWindowPos = gameOpts->render.cockpit.nWindowPos % 3;
if ((nWindowPos == 2) && gameStates.render.cockpit.n3DView [0] && gameStates.render.cockpit.n3DView [1])
	nWindowPos = 1;
if (nWindowPos == 0)	// near corners
	x = nWindow
		 ? Canvas ()->Width () - w - h / 10
		 : h / 10;
else if (nWindowPos == 1)	// near middle
	x = nWindow
		 ? Canvas ()->Width () / 3 * 2 - w / 3
		 : Canvas ()->Width () / 3 - 2 * w / 3;
else 	// middle
	x = Canvas ()->Width () / 2 - w / 2;
x -= gameData.FloatingStereoOffset2D (x);

if (gameOpts->render.cockpit.nWindowPos > 2) 
	y = h / 10;
else {
	y = gameData.render.frame.Height () - h - h / 10;
	if (extraGameInfo [0].nWeaponIcons &&
		 (extraGameInfo [0].nWeaponIcons - gameOpts->render.weaponIcons.bEquipment < 3))
		y -= (int) ((gameOpts->render.weaponIcons.bSmall ? 20.0 : 30.0) * (double) CCanvas::Current ()->Height () / 480.0);
	}
//copy these vars so stereo code can get at them
CCockpitInfo::bWindowDrawn [nWindow] = 1;
gameData.render.window = gameData.render.scene;
gameData.render.window += CViewport (x, y, w, h);
}

//	-----------------------------------------------------------------------------

void CHUD::Toggle (void)
{
CGenericCockpit::Activate (CM_LETTERBOX, true);
}

//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------

bool CWideHUD::Setup (bool bRebuild)
{
if (bRebuild && !m_info.bRebuild)
	return true;
m_info.bRebuild = false;
if (!CGenericCockpit::Setup ())
	return false;
int h = (int) ((gameData.render.frame.Height () * 7) / 10 / ((double) screen.Height () / (double) screen.Width () / 0.75));
Canvas () += CViewport (0, (gameData.render.frame.Height () - h) / 2, gameData.render.frame.Width (), h);
Canvas ()->Activate ();
//GameInitRenderSubBuffers (x, y, w, h);
return true;
}

//	-----------------------------------------------------------------------------

void CWideHUD::DrawRecording (void)
{
CGenericCockpit::DrawRecording (7);
}

//	-----------------------------------------------------------------------------

void CWideHUD::Toggle (void)
{
CGenericCockpit::Activate (CM_FULL_COCKPIT, true);
}

//	-----------------------------------------------------------------------------

void CWideHUD::SetupWindow (int nWindow, CCanvas* canvP)
{
CHUD::SetupWindow (nWindow);
#if 0
if (SW_y [nWindow] + SW_h [nWindow] > gameData.render.frame.Bottom ())
	SW_y [nWindow] -= (screen.Height () - gameData.render.frame.Height ());
gameData.render.frame.SetupPane (canvP, SW_x [nWindow], SW_y [nWindow], SW_w [nWindow], SW_h [nWindow]);
#endif
}

//	-----------------------------------------------------------------------------
