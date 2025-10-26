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
#include "font.h"

//	-----------------------------------------------------------------------------

static void DrawSeparator (int32_t l, int32_t t, int32_t r, int32_t b)
{
int32_t nLayout = gameStates.menus.nInMenu ? 0 : gameOpts->render.cockpit.nShipStateLayout;
if (gameOpts->render.cockpit.bSeparators && (nLayout == 2)) {
	CCanvas::Current ()->Color ().Set (128, 128, 128, 192);
	glLineWidth (2);
	OglDrawLine (l, t, r, b);
	glLineWidth (1);
	}
}

//	-----------------------------------------------------------------------------

void CHUD::GetHostageWindowCoords (int32_t& x, int32_t& y, int32_t& w, int32_t& h)
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
	int32_t	w, h, aw;

if ((gameData.hudData.msgs [0].nMessages > 0) &&
	 (strlen (gameData.hudData.msgs [0].szMsgs [gameData.hudData.msgs [0].nFirst]) > 38))
	return;
if ((IsMultiGame && !IsCoopGame))
	sprintf (szScore, "   %s: %5d", TXT_KILLS, LOCALPLAYER.netKillsTotal);
else
	sprintf (szScore, "   %s: %5d", TXT_SCORE, LOCALPLAYER.score);
fontManager.Current ()->StringSize (szScore, w, h, aw);
SetFontColor (GREEN_RGBA);
SetFontScale (1.0f);
DrawHUDText (NULL, -w - LHX (2), 3, szScore);
}

//	-----------------------------------------------------------------------------

 void CHUD::DrawAddedScore (void)
{
if (cockpit->Hide ())
	return;

	int32_t	color;
	int32_t	w, h, aw, nScore, nTime;
	char	szScore [20];

	static int32_t nIdTotalScore = 0;

if (IsMultiGame && !IsCoopGame)
	return;
if (!(nScore = cockpit->AddedScore ()))
	return;
cockpit->SetScoreTime (nTime = cockpit->ScoreTime () - gameData.timeData.xFrame);
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

	static int32_t nIdLock = 0;

#if 0 //DBG
if (gameData.timeData.xGame & 0x4000) {
#else
if ((LOCALPLAYER.homingObjectDist >= 0) && (gameData.timeData.xGame & 0x4000)) {
#endif
	int32_t	x, y, nOffsetSave = -1;
	int32_t nLayout = ogl.VRActive () ? -1 : gameStates.menus.nInMenu ? 0 : gameOpts->render.cockpit.nShipStateLayout + 1;
	int32_t w, h, aw;

	if (nLayout) {
		ScaleUp ();
		fontManager.Current ()->StringSize (TXT_LOCK, w, h, aw);
		x = gameData.renderData.scene.Width () / 2 - w / 2;
		y = gameData.renderData.scene.Height () / 2 + ScaleY (Y_GAUGE_OFFSET (0)) + (nLayout ? LineSpacing () : 0);
		if (nLayout == 1) {
			SetFontColor (RED_RGBA);
			if ((EGI_FLAG (nDamageModel, 0, 0, 0) != 0) && !ShowTextGauges ())
				y -= 2 * LineSpacing ();
			}
		else {
			CCanvas::Current ()->SetColorRGB (255, 0, 0, 255);
			OglDrawFilledRect (x - 5, y - 4, x + w + 3, y + h + 3);
			SetFontColor (BLACK_RGBA);
			}
		nIdLock = DrawHUDText (&nIdLock, x, y, TXT_LOCK);
		ScaleDown ();
		}
	else {
		fontManager.Current ()->StringSize (TXT_LOCK, w, h, aw);
		if (nLayout < 0) {
			x = CCanvas::Current ()->Width () / 2 - w / 2;
			y = AdjustCockpitY (-4 * LineSpacing ());
			//y = CCanvas::Current ()->Height () / 2 + 4 * h;
			nOffsetSave = gameData.SetStereoOffsetType (STEREO_OFFSET_NONE);
			}
		else {
			x = 0x8000;
			y = CCanvas::Current ()->Height () - LineSpacing ();
			if ((hudIcons.Visible () && (extraGameInfo [0].nWeaponIcons == 2)) ||
				(hudIcons.Inventory () && (extraGameInfo [0].nWeaponIcons & 1)))
				y -= LHY (30);
			m_info.bAdjustCoords = true;
			}
		if ((m_info.weaponBoxUser [0] != WBU_WEAPON) || (m_info.weaponBoxUser [1] != WBU_WEAPON)) {
			int32_t wy = (m_info.weaponBoxUser [0] != WBU_WEAPON) ? SW_y [0] : SW_y [1];
			y = Min (y, (wy - LineSpacing () - gameData.renderData.frame.Top ()));
			}
		SetFontColor (RED_RGBA);
		nIdLock = DrawHUDText (&nIdLock, x, y, TXT_LOCK);
		}
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

	int32_t	x, y, dx = GAME_FONT->Width () + GAME_FONT->Width () / 2;

#if 0
if (ogl.VRActive ()) {
	x = 3 * CCanvas::Current ()->Width () / 4 - 4 * dx;
	y = CCanvas::Current ()->Height () + AdjustCockpitY (-2 * LineSpacing ());
	}
else 
#endif
	{
	x = 2;
	y = 3 * LineSpacing ();
	}

int32_t nOffsetSave = gameData.SetStereoOffsetType (STEREO_OFFSET_NONE);
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

int32_t CHUD::FlashGauge (int32_t h, int32_t *bFlash, int32_t tToggle)
{
	time_t t = gameStates.app.nSDLTicks [0];
	int32_t b = *bFlash;

if (gameOpts->app.bEpilepticFriendly || gameStates.app.bPlayerIsDead || LOCALPLAYER.m_bExploded)
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
return (int32_t) ((b && (tToggle <= t)) ? t + 300 / b : 0);
}

//	-----------------------------------------------------------------------------

void CHUD::DrawShieldText (void)
{
if (cockpit->Hide ())
	return;

	static int32_t nIdShield = 0;

if (ShowTextGauges ()) {
	char szGauge [20];

	int32_t	x, y;
	int32_t	nLayout = gameStates.menus.nInMenu ? 0 : gameOpts->render.cockpit.nShipStateLayout;

	if (nLayout) {
		ScaleUp ();
		sprintf (szGauge, "%i", (int32_t) FRound (m_info.nShield * LOCALPLAYER.ShieldScale ()));
		x = gameData.renderData.scene.Width () / 2 + ScaleX (X_GAUGE_OFFSET (0));
		y = gameData.renderData.scene.Height () / 2 + ScaleY (Y_GAUGE_OFFSET (0));
		SetFontColor (RGBA (0,128,255,255));
		//gameStates.render.grAlpha = 0.5f;
		}
	else {
		sprintf (szGauge, "%s: %i", TXT_SHIELD, (int32_t) FRound (m_info.nShield * LOCALPLAYER.ShieldScale ()));
		x = 2;
		y = IsMultiGame ? -6 * LineSpacing () : -2 * LineSpacing ();
		SetFontColor (GREEN_RGBA);
		}
	m_info.bAdjustCoords = true;
	nIdShield = DrawHUDText (&nIdShield, x, y, szGauge);
	if (nLayout)
		ScaleDown ();
	gameStates.render.grAlpha = 1.0f;
	}
}

//	-----------------------------------------------------------------------------

void CHUD::DrawShieldBar (void)
{
if (cockpit->Hide ())
	return;

	static int32_t		bShow = 1;
	static time_t		tToggle = 0, nBeep = -1;
	//static int32_t		nIdLevel = 0;

	time_t				t = gameStates.app.nSDLTicks [0];
	int32_t				bLastFlash = gameStates.render.cockpit.nShieldFlash;

if (!ShowTextGauges ()) {

	int32_t nLevel = m_info.nShield;
	if ((t = FlashGauge (nLevel, &gameStates.render.cockpit.nShieldFlash, (int32_t) tToggle))) {
		tToggle = t;
		bShow = !bShow;
		}

	int32_t nLineSpacing = 5 * GAME_FONT->Height () / 4;
	int32_t h = int32_t (9 * m_info.yGaugeScale), 
		 w = int32_t (9 * m_info.xGaugeScale), 
		 y = CCanvas::Current ()->Height () - (int32_t) (((IsMultiGame ? 6 : 2) * nLineSpacing - 1) * m_info.yGaugeScale);
	if (hudIcons.LoadGaugeIcons () > 0)
		hudIcons.GaugeIcon (0).RenderScaled (6, y, w, h);
	int32_t x = 6 + int32_t (10 * m_info.xGaugeScale);
	w = (nLevel > 100) ? 100 : 50;
	CCanvas::Current ()->SetColorRGB (0, 64, 224, 255);
	glLineWidth (1);
	OglDrawEmptyRect (x, y, x + int32_t (w * m_info.xGaugeScale), y + h);
	if (bShow) {
		CCanvas::Current ()->SetColorRGB (0, 64, 224, 128);
		if (nLevel <= 100)
			OglDrawFilledRect (x, y, x + int32_t (nLevel * m_info.xGaugeScale / 2.0f), y + h);
		else {
			w = int32_t (50 * m_info.xGaugeScale);
			OglDrawFilledRect (x, y, x + w, y + h);
			while (nLevel > 100)
				nLevel -= 100;
			CCanvas::Current ()->SetColorRGB (0, 224, 224, 128);
			OglDrawFilledRect (x + w, y, x + w + int32_t (nLevel * m_info.xGaugeScale / 2.0f), y + h);
			}
		}
	}
if (gameStates.render.cockpit.nShieldFlash) {
	if (gameOpts->gameplay.bShieldWarning && gameOpts->sound.bUseSDLMixer) {
		if ((nBeep < 0) || (bLastFlash != gameStates.render.cockpit.nShieldFlash)) {
			if (nBeep >= 0)
				audio.StopSound ((int32_t) nBeep);
			nBeep = audio.StartSound (-1, SOUNDCLASS_GENERIC, I2X (2) / 3, 0xFFFF / 2, -1, -1, -1, -1, I2X (1),
											  AddonSoundName ((gameStates.render.cockpit.nShieldFlash == 1) ? SND_ADDON_LOW_SHIELDS1 : SND_ADDON_LOW_SHIELDS2));
			}
		}
	else if (nBeep >= 0) {
		audio.StopSound ((int32_t) nBeep);
		nBeep = -1;
		}
	}
else {
	bShow = 1;
	if (nBeep >= 0) {
		audio.StopSound ((int32_t) nBeep);
		nBeep = -1;
		}
	}
}

//	-----------------------------------------------------------------------------

void CHUD::DrawEnergyText (void)
{
if (cockpit->Hide ())
	return;

	int32_t h, x, y;
	static int32_t nIdEnergy = 0;

h = LOCALPLAYER.Energy () ? X2IR (LOCALPLAYER.Energy ()) : 0;
if (ShowTextGauges ()) {
	char szGauge [20];

	int32_t	nLayout = gameStates.menus.nInMenu ? 0 : gameOpts->render.cockpit.nShipStateLayout;

	if (nLayout) {
		ScaleUp ();
		sprintf (szGauge, "%i", h);
		x = gameData.renderData.scene.Width () / 2 - ScaleX (X_GAUGE_OFFSET (0)) - StringWidth (szGauge);
		y = gameData.renderData.scene.Height () / 2 + ScaleY (Y_GAUGE_OFFSET (0));
		SetFontColor (GOLD_RGBA);
		//gameStates.render.grAlpha = 0.5f;
		}
	else {
		sprintf (szGauge, "%s: %i", TXT_ENERGY, h);
		x = 2;
		y = IsMultiGame ? -5 * LineSpacing () : -LineSpacing ();
		SetFontColor (GREEN_RGBA);
		}
	m_info.bAdjustCoords = true;
	nIdEnergy = DrawHUDText (&nIdEnergy, x, y, szGauge);
	if (nLayout)
		ScaleDown ();
	gameStates.render.grAlpha = 1.0f;
	}
if (gameData.demoData.nState == ND_STATE_RECORDING) {
	int32_t energy = X2IR (LOCALPLAYER.Energy ());

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
	static int32_t	bFlash = 0, bShow = 1;
	static time_t	tToggle;
	//static int32_t		nIdLevel = 0;
	time_t			t;

	int32_t nLevel = m_info.nEnergy;
	int32_t nLineSpacing = 5 * GAME_FONT->Height () / 4;
	if ((t = FlashGauge (nLevel, &bFlash, (int32_t) tToggle))) {
		tToggle = t;
		bShow = !bShow;
		}
	int32_t h = int32_t (9 * m_info.yGaugeScale), 
		 w = int32_t (9 * m_info.xGaugeScale), 
		 y = CCanvas::Current ()->Height () - (int32_t) (((IsMultiGame ? 5 : 1) * nLineSpacing - 1) * m_info.yGaugeScale);
	if (hudIcons.LoadGaugeIcons () > 0)
		hudIcons.GaugeIcon (1).RenderScaled (6, y, w, h);
	CCanvas::Current ()->SetColorRGB (255, 224, 0, 255);
	glLineWidth (1);
	int32_t x = 6 + int32_t (10 * m_info.xGaugeScale);
	w = (nLevel > 100) ? 100 : 50;
	OglDrawEmptyRect (x, y, x + int32_t (w * m_info.xGaugeScale), y + h);
	if (bFlash) {
		if (!bShow)
			return;
		nLevel = 100;
		}
	else
		bShow = 1;
	CCanvas::Current ()->SetColorRGB (255, 224, 0, 128);
	if (nLevel <= 100)
		OglDrawFilledRect (x, y, x + int32_t (nLevel * m_info.xGaugeScale / 2.0f), y + h);
	else {
		w = int32_t (50 * m_info.xGaugeScale);
		OglDrawFilledRect (x, y, x + w, y + h);
		while (nLevel > 100)
			nLevel -= 100;
		CCanvas::Current ()->SetColorRGB (255, 240, 240, 128);
		OglDrawFilledRect (x + w, y, x + w + int32_t (nLevel * m_info.xGaugeScale / 2.0f), y + h);
		}
	}
}

//	-----------------------------------------------------------------------------

void CHUD::DrawAfterburnerText (void)
{
if (cockpit->Hide ())
	return;
	
	int32_t h, x, y;
	static int32_t nIdAfterBurner = 0;

if (!(LOCALPLAYER.flags & PLAYER_FLAGS_AFTERBURNER))
	return;		//don't draw if don't have
h = FixMul (gameData.physicsData.xAfterburnerCharge, 100);
if (ShowTextGauges ()) {
	char szGauge [20];
	int32_t	nLayout = gameStates.menus.nInMenu ? 0 : gameOpts->render.cockpit.nShipStateLayout;

	if (nLayout) {
		ScaleUp ();
		sprintf (szGauge, "%i", h);
		x = gameData.renderData.scene.Width () / 2 - StringWidth (szGauge) / 2;
		y = gameData.renderData.scene.Height () / 2 + ScaleY (Y_GAUGE_OFFSET (0));
		SetFontColor (RED_RGBA);
		//gameStates.render.grAlpha = 0.5f;
		}
	else {
		x = 2;
		y = -(IsMultiGame ? 8 : 3) * LineSpacing ();
		strcpy (szGauge, TXT_HUD_BURN);
		SetFontColor (GREEN_RGBA);
		}
	m_info.bAdjustCoords = true;
	nIdAfterBurner = DrawHUDText (&nIdAfterBurner, x, y, szGauge, h);
	if (nLayout)
		ScaleDown ();
	gameStates.render.grAlpha = 1.0f;
	}
}

//	-----------------------------------------------------------------------------

void CHUD::DrawAfterburnerBar (void)
{
if (cockpit->Hide ())
	return;

	//static int32_t nIdLevel = 0;
	
if (!(LOCALPLAYER.flags & PLAYER_FLAGS_AFTERBURNER))
	return;		//don't draw if don't have
int32_t nLevel = FixMul (gameData.physicsData.xAfterburnerCharge, 100);
int32_t nLineSpacing = 5 * GAME_FONT->Height () / 4;
if (!ShowTextGauges ()) {
	int32_t h = int32_t (9 * m_info.yGaugeScale), 
		 w = int32_t (9 * m_info.xGaugeScale), 
		 y = CCanvas::Current ()->Height () - (int32_t) (((IsMultiGame ? 8 : 3) * nLineSpacing - 1) * m_info.yGaugeScale);
	if (hudIcons.LoadGaugeIcons () > 0)
		hudIcons.GaugeIcon (2).RenderScaled (6, y, w = int32_t (9 * m_info.xGaugeScale), h = int32_t (9 * m_info.yGaugeScale));
	CCanvas::Current ()->SetColorRGB (255, 0, 0, 255);
	glLineWidth (1);
	int32_t x = 6 + int32_t (10 * m_info.xGaugeScale);
	OglDrawEmptyRect (x, y, x + (int32_t) (50 * m_info.xGaugeScale), y + h);
	CCanvas::Current ()->SetColorRGB (224, 0, 0, 128);
	OglDrawFilledRect (x, y, x + (int32_t) (nLevel * m_info.xGaugeScale / 2.0f), y + h);
	}
if (gameData.demoData.nState == ND_STATE_RECORDING) {
	if (gameData.physicsData.xAfterburnerCharge != m_history [0].afterburner) {
		NDRecordPlayerAfterburner (m_history [0].afterburner, gameData.physicsData.xAfterburnerCharge);
		m_history [0].afterburner = gameData.physicsData.xAfterburnerCharge;
	 	}
	}
}

//	-----------------------------------------------------------------------------

void CHUD::DrawEnergyLevelsCombined (void)
{
	int32_t	y = AdjustCockpitY (-2 * LineSpacing ());
	int32_t	x = 2; //CCanvas::Current ()->Width () / 4;
	int32_t	nOffsetSave = gameData.SetStereoOffsetType (STEREO_OFFSET_NONE);

	static uint32_t	energyColors [3] = { GOLD_RGBA, BLUE_RGBA, RED_RGBA };

int32_t				i, nColor, h [4], w [4], aw [4], tw = 0;
int32_t				nEnergy [3] = {LOCALPLAYER.Energy () ? X2IR (LOCALPLAYER.Energy ()) : 0, 
											(int32_t) FRound (m_info.nShield * LOCALPLAYER.ShieldScale ()), 
											(LOCALPLAYER.flags & PLAYER_FLAGS_AFTERBURNER) ? FixMul (gameData.physicsData.xAfterburnerCharge, 100) : -1
										  };
char				szEnergy [3][10];
CCanvasColor	color;

color.Set (0, 0, 0, 128);
color.index = -1;
color.rgb = 1;

CCanvas::Current ()->SetFont (GAME_FONT);
for (i = 0; i < 3; i++) {
	if (nEnergy [i] < 0)
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
if (ogl.VRActive ())
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

//	-----------------------------------------------------------------------------

void CHUD::ClearBombCount (int32_t bgColor)
{
}

//	-----------------------------------------------------------------------------

void CHUD::DrawBombCount (void)
{
CGenericCockpit::DrawBombCount (0, 0, BLACK_RGBA, 0);
}

//	-----------------------------------------------------------------------------

int32_t CHUD::DrawBombCount (int32_t& nIdBombCount, int32_t x, int32_t y, int32_t nColor, char* pszBombCount)
{
if (gameOpts->render.cockpit.nShipStateLayout)
	return 0;

fontManager.SetColorRGBi (nColor, 1, 0, 0);
x = gameData.renderData.scene.Width () - 3 * GAME_FONT->Width () - gameStates.render.fonts.bHires - 1;
y = gameData.renderData.scene.Height () - 3 * LineSpacing ();
if ((extraGameInfo [0].nWeaponIcons >= 3) && (gameData.renderData.scene.Height () < 670))
	x -= LHX (20);
int32_t nOffsetSave = AdjustCockpitXY (pszBombCount, x, y);
fontManager.PushScale ();
fontManager.SetScale (FontScale ());
int32_t retCode = GrString (x, y, pszBombCount, &nIdBombCount);
fontManager.PopScale ();
ScaleDown ();
gameData.SetStereoOffsetType (nOffsetSave);
return retCode;
}

//	-----------------------------------------------------------------------------

void CHUD::DrawPrimaryAmmoInfo (int32_t ammoCount)
{
DrawAmmoInfo (PRIMARY_AMMO_X, PRIMARY_AMMO_Y, ammoCount, 1);
}

//	-----------------------------------------------------------------------------

void CHUD::DrawSecondaryAmmoInfo (int32_t ammoCount)
{
DrawAmmoInfo (SECONDARY_AMMO_X, SECONDARY_AMMO_Y, ammoCount, 0);
}

//	-----------------------------------------------------------------------------

static const char* CC_AVAILABLE = "\x1\x80\xf0\x80\x0";
static const char* CC_EQUIPPED [3] = {"\x1\xff\x80\x80\x0", "\x1\xff\xe0\x80\x0", "\x1\x80\xff\xff\x0" };
static const char* CC_EMPTY = "\x1\xA0\xA0\xA0\x0";

int32_t CHUD::StrWeaponStateColor (char* pszList, int32_t l, int32_t bAvailable, int32_t bActive)
{
memcpy (pszList + l, bAvailable ? bActive ? CC_EQUIPPED [gameOpts->render.cockpit.nColorScheme] : CC_AVAILABLE : CC_EMPTY, 4);
return l + 4;
}

//	-----------------------------------------------------------------------------

void CHUD::SetWeaponStateColor (int32_t bAvailable, int32_t bActive)
{
	static uint32_t equippedColors [3] = { RED_RGBA, GOLD_RGBA, CYAN_RGBA };

fontManager.SetColorRGBi (bAvailable ? bActive ? equippedColors [gameOpts->render.cockpit.nColorScheme] : /*GREEN_RGBA*/ RGBA (0, 224, 0, 255) : RGBA (160, 160, 160, 255), 1, 0, 0);
}

//	-----------------------------------------------------------------------------

char* CHUD::StrPrimaryWeaponList (char* pszList, char* pszAmmo)
{
// Q6SPVF HPGO - (Quad) Lasers <level>, Spreafire, Plasma, Vulcan, Fusion, Helix, Gauss, Omega
	static const char* szWeaponIds = "LVSPFSGHXO";

	int32_t	n = (gameStates.app.bD1Mission) ? 5 : 10;
	int32_t	l = 0;
	int32_t	nState [2] = {-1, 0};
	int32_t	nMaxAutoSelect = 255;
	bool		bLasers = false;
	char		szList [100];

pszAmmo [0] = '\0';
for (int32_t j = 0; j < n; j++) {
	int32_t bActive, bHave, bAvailable;

#if 0
	if (!gameStates.app.bD1Mission && extraGameInfo [0].bSmartWeaponSwitch && ((j == 1) || (j == 2)) && LOCALPLAYER.primaryWeaponFlags & (1 << (j + 5)))
		continue; // skip Vulcan / Spreadfire if player has Gauss / Helix and smart weapon switch is enabled
#endif

	int32_t	k = hudIcons.GetWeaponIndex (0, j, nMaxAutoSelect);

	if (k == 0)
		k = 5;

	hudIcons.GetWeaponState (bHave, bAvailable, bActive, 0, j, k);
	nState [1] = (bHave && bAvailable) | (bActive << 1);

	if (k == 5) { // standard laser
		if (bLasers)
			continue;
		bLasers = true;
		if (!bHave) {
			hudIcons.GetWeaponState (bHave, bAvailable, bActive, 0, 0, 0); // super laser
			nState [1] = (bHave && bAvailable) | (bActive << 1);
			}
		if (nState [1] != nState [0])
			l = StrWeaponStateColor (szList, l, bHave && bAvailable, bActive);
		szList [l++] = 'L';
#if 0
		szList [l++] = (LOCALPLAYER.flags & PLAYER_FLAGS_QUAD_LASERS) ? 'Q' : 'L';
		szList [l++] = 48 + LOCALPLAYER.LaserLevel () + 1;
#endif
		}
	else if (k == 0) { // 5 == super laser, handled above
		continue;
		}
	else {
		if (nState [1] != nState [0])
			l = StrWeaponStateColor (szList, l, bHave && bAvailable, bActive);
		szList [l++] = szWeaponIds [k];
		}

	nState [0] = nState [1];

	if (bActive) {
		char		szPrefix [2] = {'\0', '\0'}, szSuffix [2] = {'\0', '\0'};
		int32_t	nAmmo;

		StrWeaponStateColor (pszAmmo, 0, bHave && bAvailable, 1);
		if ((k == 0) || (k == 5)) {
			nAmmo = LOCALPLAYER.LaserLevel () + 1;
			if (LOCALPLAYER.flags & PLAYER_FLAGS_QUAD_LASERS)
				szPrefix [0] = 'Q';
			}
		else if ((k == 1) || (k == 6)) {
			nAmmo = X2I ((uint32_t) LOCALPLAYER.primaryAmmo [VULCAN_INDEX] * (uint32_t) VULCAN_AMMO_SCALE);
#if 0
			if (nAmmo >= 1000)
				nAmmo /= 1000;
				szSuffix [0] = 'K';
#endif
			}	
		else if (k == 9)
			nAmmo = gameData.omegaData.xCharge [IsMultiGame] * 100 / MAX_OMEGA_CHARGE;
		else
			continue;

		sprintf (pszAmmo + 4, "%s%d%s", szPrefix, nAmmo, szSuffix);
		}
	}

szList [l] = '\0';
pszList [0] = '\0';
#if 0
if (nLayout == 2) {
	strcpy (pszList + 1, szAmmo);
	strcat (pszList + 1, " ");
	}
else
#endif
	pszList [1] = '\0';
strcat (pszList + 1, szList);
return pszList;
}

//	-----------------------------------------------------------------------------

static int32_t WeaponListTop (void)
{
if (gameOpts->render.cockpit.nCompactHeight == 2)
	return gameData.renderData.scene.Height () / 2 + cockpit->ScaleY (Y_GAUGE_OFFSET (0));
int32_t nLineSpacing = cockpit->LineSpacing ();
return gameData.renderData.scene.Height () / 2 - (gameStates.app.bD1Mission ? 1 : 2) * nLineSpacing + gameOpts->render.cockpit.nCompactHeight * nLineSpacing * 2;
}

//	-----------------------------------------------------------------------------

void CHUD::DrawPrimaryWeaponList (void)
{
// Q6SPVF HPGO - (Quad) Lasers <level>, Spreafire, Plasma, Vulcan, Fusion, Helix, Gauss, Omega
	static const char* szWeaponIds = "LVSPFSGHXO";

	int32_t	n = (gameStates.app.bD1Mission) ? 5 : 10;
	int32_t	nState [2] = {-1, 0};
	int32_t	nLineSpacing = cockpit->LineSpacing ();
	int32_t	x = gameData.renderData.scene.Width () / 2 - cockpit->ScaleX (X_GAUGE_OFFSET (gameOpts->render.cockpit.nCompactWidth) * 3);
	int32_t	y = WeaponListTop ();
	int32_t	t = y;
	int32_t	nMaxAutoSelect = 255;
	bool		bLasers = false;
	char		szLabel [2] = {'\0', '\0'}, szAmmo [20];

szAmmo [0] = '\0';
for (int32_t j = 0; j < n; j++) {
	int32_t bActive, bHave, bAvailable;

	int32_t	k = hudIcons.GetWeaponIndex (0, j, nMaxAutoSelect);
	if (k == 0)
		k = 5;

	hudIcons.GetWeaponState (bHave, bAvailable, bActive, 0, j, k);
	nState [1] = (bHave && bAvailable) | (bActive << 1);

	if (k == 5) { // standard laser
		if (bLasers)
			continue;
		bLasers = true;
		if (!bHave) {
			hudIcons.GetWeaponState (bHave, bAvailable, bActive, 0, 0, 0); // super laser
			nState [1] = (bHave && bAvailable) | (bActive << 1);
			}
		if (nState [1] != nState [0])
			SetWeaponStateColor (bHave && bAvailable, bActive);
#if 1
		szLabel [0] = 'L';
#else
		szLabel [0] = (LOCALPLAYER.flags & PLAYER_FLAGS_QUAD_LASERS) ? 'Q' : 'L';
#endif
		}
	else if (k == 0) { // 5 == super laser, handled above
		continue;
		}
	else {
		if (nState [1] != nState [0])
			SetWeaponStateColor (bHave && bAvailable, bActive);
		szLabel [0] = szWeaponIds [k];
		}

	nState [0] = nState [1];

	//if (bActive) 
	szAmmo [0] = '\0';
	if (bHave && bAvailable) {
		char		szPrefix [2] = {'\0', '\0'}, szSuffix [2] = {'\0', '\0'};
		int32_t	nAmmo = 0;

		if ((k == 0) || (k == 5)) {
			nAmmo = LOCALPLAYER.LaserLevel () + 1;
			if (LOCALPLAYER.flags & PLAYER_FLAGS_QUAD_LASERS)
				szPrefix [0] = 'Q';
			}
		else if ((k == 1) || (k == 6)) {
			nAmmo = X2I ((uint32_t) LOCALPLAYER.primaryAmmo [VULCAN_INDEX] * (uint32_t) VULCAN_AMMO_SCALE);
			if (nAmmo >= 1000) {
				nAmmo /= 1000;
				szSuffix [0] = 'K';
				}
			else if (nAmmo >= 100) {
				nAmmo /= 100;
				szPrefix [0] = '.';
				}
			}	
		else if (k == 9)
			nAmmo = gameData.omegaData.xCharge [IsMultiGame] * 100 / MAX_OMEGA_CHARGE;
		if (nAmmo)
			sprintf (szAmmo, "%s%d%s ", szPrefix, nAmmo, szSuffix);
		}
	strcat (szAmmo, szLabel);
	GrPrintF (NULL, x - StringWidth (szAmmo), y, szAmmo);
	y += nLineSpacing;
	}
DrawSeparator (x + 2, t, x + 2, y - 2);
}

//	-----------------------------------------------------------------------------

int32_t CHUD::StrProxMineStatus (char* pszList, int32_t l, int32_t n, char tag, int32_t* nState)
{
int32_t	bActive, bHave, bAvailable;
hudIcons.GetWeaponState (bHave, bAvailable, bActive, 1, (n == 2) ? 9 : 8, n);
nState [1] = (bHave && bAvailable) | (bActive << 1);
if (nState [1] != nState [0])
	l = StrWeaponStateColor (pszList, l, bHave && bAvailable, bActive);
nState [0] = nState [1];
pszList [l++] = tag;
return l;
}

//	-----------------------------------------------------------------------------

char* CHUD::StrSecondaryWeaponList (char* pszList)
{
// CHSM MGFS - Concussion, Homer, Smart, Mega, Mercury, Guided, Flash, Shaker
	static const char* szWeaponIds = "CHBSMFGSYE";

	int32_t	n = (gameStates.app.bD1Mission) ? 5 : 10;
	int32_t	l = 1;
	int32_t	bActive, bHave, bAvailable;
	int32_t	nLayout = gameStates.menus.nInMenu ? 0 : gameOpts->render.cockpit.nShipStateLayout;
	int32_t	nMaxAutoSelect = 255;
	int32_t	nState [2] = {-1, 0};
	char		szAmmo [20];

pszList [0] = '\0';
szAmmo [0] = '\0';
for (int32_t j = 0; j < n; j++) {

	int32_t k = hudIcons.GetWeaponIndex (1, j, nMaxAutoSelect);

	if ((k == 2) || (k == 7)) // prox bomb, smart mine
			continue;

	hudIcons.GetWeaponState (bHave, bAvailable, bActive, 1, j, k);
	if (bActive) {
		StrWeaponStateColor (szAmmo, 0, bHave && bAvailable, 1);
		sprintf (szAmmo + 4, " %d",  LOCALPLAYER.secondaryAmmo [/*gameStates.app.bD1Mission ? k - (k > 2) :*/ k]);
		}
	nState [1] = (bHave && bAvailable) | (bActive << 1);
	if (nState [1] != nState [0])
		l = StrWeaponStateColor (pszList, l, bHave && bAvailable, bActive);
	pszList [l++] = szWeaponIds [gameStates.app.bD1Mission ? k - (k > 2) : k];
	nState [0] = nState [1];
	}

pszList [l++] = ' ';
l = StrProxMineStatus (pszList, l, 2, 'B', nState);
if (!gameStates.app.bD1Mission) 
	l = StrProxMineStatus (pszList, l, 7, 'S', nState);

pszList [l] = '\0';
if (nLayout == 1)
	strcat (pszList + 1, szAmmo);
return pszList;
}

//	-----------------------------------------------------------------------------

void CHUD::DrawSecondaryWeaponList (void)
{
// CHSM MGFS - Concussion, Homer, Smart, Mega, Mercury, Guided, Flash, Shaker
	static const char* szWeaponIds = "CHBSMFGSYE";

	int32_t	n = (gameStates.app.bD1Mission) ? 5 : 10;
	int32_t	bActive, bHave, bAvailable;
	int32_t	nLayout = gameStates.menus.nInMenu ? 0 : gameOpts->render.cockpit.nShipStateLayout;
	int32_t	nMaxAutoSelect = 255;
	int32_t	nLineSpacing = cockpit->LineSpacing ();
	int32_t	nState [2] = {-1, 0};
	int32_t	x = gameData.renderData.scene.Width () / 2 + cockpit->ScaleX (X_GAUGE_OFFSET (gameOpts->render.cockpit.nCompactWidth) * 3);
	int32_t	y = WeaponListTop ();
	int32_t	t = y;
	char		szLabel [2] = {'\0', '\0'}, szAmmo [20];

for (int32_t nType = 0; nType < 2; nType++) {
	for (int32_t j = 0; j < n; j++) {
		int32_t k = nType ? j : hudIcons.GetWeaponIndex (1, j, nMaxAutoSelect);
		if (nType != ((k == 2) || (k == 7))) // prox bomb, smart mine
			continue;
		hudIcons.GetWeaponState (bHave, bAvailable, bActive, 1, nType ? (j == 2) ? 9 : 8 : j, k);
		nState [1] = (bHave && bAvailable) | (bActive << 1);
		if (nState [1] != nState [0])
			SetWeaponStateColor (bHave && bAvailable, bActive);
		szLabel [0] = szWeaponIds [k];
		GrPrintF (NULL, x, y, szLabel);
		if (LOCALPLAYER.secondaryAmmo [k]) {
			sprintf (szAmmo, "%d", LOCALPLAYER.secondaryAmmo [k]);
			GrPrintF (NULL, x + 64 - StringWidth (szAmmo), y, szAmmo);
			}
		y += nLineSpacing;
		}
	y += nLineSpacing / 2;
	}
DrawSeparator (x - 4, t, x - 4, y - nLineSpacing / 2 - 2);
}

//	-----------------------------------------------------------------------------

int32_t CHUD::DrawAmmoCount (char* szLabel, int32_t x, int32_t y, int32_t j, int32_t k, int32_t* nState)
{
int32_t	bActive, bHave, bAvailable;

hudIcons.GetWeaponState (bHave, bAvailable, bActive, 1, j, k);
nState [1] = (bHave && bAvailable) | (bActive << 1);
if (nState [1] != nState [0])
	SetWeaponStateColor (bHave && bAvailable, bActive);
nState [0] = nState [1];

int32_t wt = fontManager.Current ()->GetCharWidth (szLabel);
uint16_t nAmmo = LOCALPLAYER.secondaryAmmo [k];
char szAmmo [2] = {'\0', '\0'};
if (nAmmo > 9)
	szAmmo [0] = '*'; // '^'
else if (nAmmo)
	szAmmo [0] = char (48 + nAmmo);
else
#if 1
	szAmmo [0] = '-';
#else
	return x + wt; // leave ammo count of unavailable weapons blank
#endif
int32_t dx = Max ((wt - fontManager.Current ()->GetCharWidth (szAmmo)) / 2, 0);
GrPrintF (NULL, x + dx, y, szAmmo);
return x + wt;
}


//	-----------------------------------------------------------------------------

void CHUD::DrawSecondaryAmmoList (char* pszList)
{
// CHSM MGFS - Concussion, Homer, Smart, Mega, Mercury, Guided, Flash, Shaker
	int32_t	n = (gameStates.app.bD1Mission) ? 5 : 10;
	int32_t	l = 0;
	int32_t	nMaxAutoSelect = 255;
	int32_t	nState [2] = {-1, 0};
	int32_t	nLineSpacing = cockpit->LineSpacing ();
	int32_t	x = gameData.renderData.scene.Width () / 2 + cockpit->ScaleX (X_GAUGE_OFFSET (gameOpts->render.cockpit.nCompactWidth));
	int32_t	y = gameData.renderData.scene.Height () / 2 + ScaleY (Y_GAUGE_OFFSET (gameOpts->render.cockpit.nCompactHeight)) + LineSpacing () * 2;

for (int32_t j = 0; j < n; j++) {
	if (pszList [l] == '\01')
		l += 4;
	int32_t k = hudIcons.GetWeaponIndex (1, j, nMaxAutoSelect);
	if ((k == 2) || (k == 7)) // prox bomb, smart mine
			continue;
	x = DrawAmmoCount (pszList + l++, x, y, j, k, nState);
	}

x += fontManager.Current ()->GetCharWidth (pszList + l++);
if (pszList [l] == '\01')
	l += 4;
x = DrawAmmoCount (pszList + l++, x, y, 9, 2, nState);
if (!gameStates.app.bD1Mission) {
	if (pszList [l] == '\01')
		l += 4;
	x = DrawAmmoCount (pszList + l, x, y, 8, 7, nState);
	}
}

//	-----------------------------------------------------------------------------

char* CHUD::StrPrimaryWeapon (char* szLabel)
{
	char			szWeapon [100];
	int32_t		nLayout = gameStates.menus.nInMenu ? 0 : gameOpts->render.cockpit.nShipStateLayout;

strcpy (szWeapon, PRIMARY_WEAPON_NAMES_SHORT (gameData.weaponData.nPrimary));
switch (gameData.weaponData.nPrimary) {
	case LASER_INDEX:
		if (LOCALPLAYER.flags & PLAYER_FLAGS_QUAD_LASERS)
			sprintf (szLabel, "%s %s %i", TXT_QUAD, szWeapon, LOCALPLAYER.LaserLevel () + 1);
		else
			sprintf (szLabel, "%s %i", szWeapon, LOCALPLAYER.LaserLevel () + 1);
		break;

	case SUPER_LASER_INDEX:
		Int3 ();
		break;	//no such thing as super laser

	case VULCAN_INDEX:
	case GAUSS_INDEX:
		sprintf (szLabel, "%s%s%i", szWeapon, nLayout ? " " : ": ", X2I ((uint32_t) LOCALPLAYER.primaryAmmo [VULCAN_INDEX] * (uint32_t) VULCAN_AMMO_SCALE));
		Convert1s (szLabel);
		break;

	case SPREADFIRE_INDEX:
	case PLASMA_INDEX:
	case FUSION_INDEX:
	case HELIX_INDEX:
	case PHOENIX_INDEX:
		strcpy (szLabel, szWeapon);
		break;

	case OMEGA_INDEX:
		sprintf (szLabel, "%s%s%03i", nLayout ? "OMG" : szWeapon, nLayout ? " " : ": ", gameData.omegaData.xCharge [IsMultiGame] * 100 / MAX_OMEGA_CHARGE);
		Convert1s (szLabel);
		break;

	default:
		Int3 ();
		szLabel [0] = 0;
		break;
	}
return szLabel;
}

//	-----------------------------------------------------------------------------

char* CHUD::StrSecondaryWeapon (char* szLabel)
{
sprintf (szLabel, "%s %d", SECONDARY_WEAPON_NAMES_VERY_SHORT (gameData.weaponData.nSecondary), LOCALPLAYER.secondaryAmmo [gameData.weaponData.nSecondary]);
return szLabel;
}

//	-----------------------------------------------------------------------------

//convert '1' characters to special wide ones
void CHUD::DrawWeapons (void)
{
if (cockpit->Hide ())
	return;
if (ogl.VRActive () && EGI_FLAG (nWeaponIcons, 1, 1, 0))
	return;

	char			szLabel [100], szAmmo [20];
	int32_t		y, w, h, aw;
	int32_t		nLayout = gameStates.menus.nInMenu ? 0 : gameOpts->render.cockpit.nShipStateLayout;

	static int32_t nIdWeapons [2] = {0, 0};

SetFontColor (GREEN_RGBA);
y = IsMultiGame ? -4 * LineSpacing () : 0;

m_info.bAdjustCoords = true;

if (!nLayout) {
	StrPrimaryWeapon (szLabel);
	fontManager.Current ()->StringSize (szLabel, w, h, aw);
	nIdWeapons [0] = DrawHUDText (nIdWeapons + 0, -5 - w, y - 2 * LineSpacing (), szLabel);
	}
else {
	ScaleUp ();
	//gameStates.render.grAlpha = 0.5f;
	if (nLayout == 2)
		DrawPrimaryWeaponList ();
	else {
		DrawHUDText (NULL, gameData.renderData.scene.Width () / 2 - ScaleX (X_GAUGE_OFFSET (gameOpts->render.cockpit.nCompactWidth)) - StringWidth (szLabel + 1), 
						 gameData.renderData.scene.Height () / 2 + ScaleY (Y_GAUGE_OFFSET (gameOpts->render.cockpit.nCompactHeight)) + LineSpacing () * ((nLayout == 2) ? 3 : 1), StrPrimaryWeaponList (szLabel, szAmmo));
		DrawHUDText (NULL, gameData.renderData.scene.Width () / 2 - ScaleX (X_GAUGE_OFFSET (gameOpts->render.cockpit.nCompactWidth)) - StringWidth (szAmmo), 
						 gameData.renderData.scene.Height () / 2 + ScaleY (Y_GAUGE_OFFSET (gameOpts->render.cockpit.nCompactHeight)) + LineSpacing () * 2, szAmmo);
		}
	//ScaleDown ();
	}

if (gameData.weaponData.nPrimary == VULCAN_INDEX) {
	if (LOCALPLAYER.primaryAmmo [gameData.weaponData.nPrimary] != m_history [0].ammo [0]) {
		if (gameData.demoData.nState == ND_STATE_RECORDING)
			NDRecordPrimaryAmmo (m_history [0].ammo [0], LOCALPLAYER.primaryAmmo [gameData.weaponData.nPrimary]);
		m_history [0].ammo [0] = LOCALPLAYER.primaryAmmo [gameData.weaponData.nPrimary];
		}
	}

if (gameData.weaponData.nPrimary == OMEGA_INDEX) {
	if (gameData.omegaData.xCharge [IsMultiGame] != m_history [0].xOmegaCharge) {
		if (gameData.demoData.nState == ND_STATE_RECORDING)
			NDRecordPrimaryAmmo (m_history [0].xOmegaCharge, gameData.omegaData.xCharge [IsMultiGame]);
		m_history [0].xOmegaCharge = gameData.omegaData.xCharge [IsMultiGame];
		}
	}

m_info.bAdjustCoords = true;

if (!nLayout) {
	StrSecondaryWeapon (szLabel);
	fontManager.Current ()->StringSize (szLabel, w, h, aw);
	nIdWeapons [1] = DrawHUDText (nIdWeapons + 1, -5 - w, y - LineSpacing (), szLabel);
	}
else {
	SetFontScale (sqrt (floor (float (CCanvas::Current ()->Width ()) / 640.0f)));
	if (nLayout == 2)
		DrawSecondaryWeaponList ();
	else {
		DrawHUDText (NULL, gameData.renderData.scene.Width () / 2 + ScaleX (X_GAUGE_OFFSET (gameOpts->render.cockpit.nCompactWidth)), 
						 gameData.renderData.scene.Height () / 2 + ScaleY (Y_GAUGE_OFFSET (gameOpts->render.cockpit.nCompactHeight)) + LineSpacing (), StrSecondaryWeaponList (szLabel));
		DrawSecondaryAmmoList (szLabel + 1);
		}
	ScaleDown ();
	}


if (LOCALPLAYER.secondaryAmmo [gameData.weaponData.nSecondary] != m_history [0].ammo [1]) {
	if (gameData.demoData.nState == ND_STATE_RECORDING)
		NDRecordSecondaryAmmo (m_history [0].ammo [1], LOCALPLAYER.secondaryAmmo [gameData.weaponData.nSecondary]);
	m_history [0].ammo [1] = LOCALPLAYER.secondaryAmmo [gameData.weaponData.nSecondary];
	}
}

//	-----------------------------------------------------------------------------

void CHUD::DrawInvul (void)
{
if (cockpit->Hide ())
	return;

	static int32_t nIdInvul = 0;

if ((LOCALPLAYER.flags & PLAYER_FLAGS_INVULNERABLE) &&
	 (((LOCALPLAYER.invulnerableTime + INVULNERABLE_TIME_MAX - gameData.timeData.xGame) > I2X (4)) || (gameData.timeData.xGame & 0x8000))) {

	int32_t		nLayout = gameStates.menus.nInMenu ? 0 : gameOpts->render.cockpit.nShipStateLayout;
	
	if (nLayout) {
		ScaleUp ();
		SetFontColor (RGBA (192, 192, 192, 255));
		nIdInvul = DrawHUDText (&nIdInvul, gameData.renderData.scene.Width () / 2 + ScaleX (X_GAUGE_OFFSET (0)), 
										gameData.renderData.scene.Height () / 2 + ScaleY (Y_GAUGE_OFFSET (0)) + ((nLayout == 2) ? 1 : 3) * LineSpacing (), "%s", "INV");
		ScaleDown ();
		}
	else {
		SetFontColor (GREEN_RGBA);

		if (ogl.VRActive ()) {
			int32_t w, h, aw;
			fontManager.Current ()->StringSize ("INVUL CLOAK ", w, h, aw);
			nIdInvul = DrawHUDText (&nIdInvul, CCanvas::Current ()->Width () - w, -2 * LineSpacing (), "%s", "INVUL");
			}
		else {
			int32_t y = IsMultiGame ? -10 * LineSpacing () : -5 * LineSpacing ();
			if ((LOCALPLAYER.flags & PLAYER_FLAGS_AFTERBURNER) && !ShowTextGauges ())
				y -= LineSpacing () + gameStates.render.fonts.bHires + 1;
			nIdInvul = DrawHUDText (&nIdInvul, 2, y, "%s", TXT_INVULNERABLE);
			}	
		}
	}
}

//	-----------------------------------------------------------------------------

void CHUD::DrawCloak (void)
{
if (cockpit->Hide ())
	return;

	static int32_t nIdCloak = 0;

if ((LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED) &&
	 ((LOCALPLAYER.cloakTime + CLOAK_TIME_MAX - gameData.timeData.xGame > I2X (3)) || (gameData.timeData.xGame & 0x8000))) {
	int32_t nLayout = gameStates.menus.nInMenu ? 0 : gameOpts->render.cockpit.nShipStateLayout;
	
	fontManager.SetCurrent (GAME_FONT);
	if (nLayout) {
		ScaleUp ();
		SetFontColor (RGBA (192, 192, 192, 255));
		nIdCloak = DrawHUDText (&nIdCloak, gameData.renderData.scene.Width () / 2 - ScaleX (X_GAUGE_OFFSET (0)) - StringWidth ("CLK", 0), 
										gameData.renderData.scene.Height () / 2 + ScaleY (Y_GAUGE_OFFSET (0)) + ((nLayout == 2) ? 1 : 3) * LineSpacing (), "%s", "CLK");
		ScaleDown ();
		}
	else {
		SetFontColor (GREEN_RGBA);

		if (ogl.VRActive ())
			nIdCloak = DrawHUDText (&nIdCloak, CCanvas::Current ()->Width () - StringWidth ("CLOAK "), -2 * LineSpacing (), "%s", "CLOAK");
		else {
			int32_t y = IsMultiGame ? -7 * LineSpacing () : -4 * LineSpacing ();
			if ((LOCALPLAYER.flags & PLAYER_FLAGS_AFTERBURNER) && !ShowTextGauges ())
				y -= LineSpacing () + gameStates.render.fonts.bHires + 1;
			nIdCloak = DrawHUDText (&nIdCloak, 2, y, "%s", TXT_CLOAKED);
			}
		}
	}
}

//	-----------------------------------------------------------------------------

//draw the icons for number of lives
void CHUD::DrawLives (void)
{
	static int32_t nIdLives = 0;

if (cockpit->Hide ())
	return;
if ((gameData.hudData.msgs [0].nMessages > 0) && (strlen (gameData.hudData.msgs [0].szMsgs [gameData.hudData.msgs [0].nFirst]) > 38))
	return;
if (IsMultiGame) {
	SetFontColor (GREEN_RGBA);
	nIdLives = DrawHUDText (&nIdLives, 10, 3, "%s: %d", TXT_DEATHS, LOCALPLAYER.netKilledTotal);
	}
else if (LOCALPLAYER.lives > 1)  {
	CBitmap* pBm = BitBlt (GAUGE_LIVES, 10, 3, false, false);
	SetFontColor (MEDGREEN_RGBA);
	nIdLives = DrawHUDText (&nIdLives, 10 + pBm->Width () + 4, 4, "x %d", LOCALPLAYER.lives - 1);
	}
}

//	-----------------------------------------------------------------------------

void CHUD::DrawSlowMotion (void)
{
int32_t nLayout = gameStates.menus.nInMenu ? 0 : gameOpts->render.cockpit.nShipStateLayout;

if (!nLayout) {
	CGenericCockpit::DrawSlowMotion ();
	return;
	}

if (Hide ())
	return;
if (gameStates.render.bRearView)
	return;
if ((LOCALPLAYER.Energy () <= I2X (10)) || !(LOCALPLAYER.flags & PLAYER_FLAGS_SLOWMOTION))
	return;

char szSlowMotion [40];

sprintf (szSlowMotion, "M%1.1f  S%1.1f",
			gameStates.gameplay.slowmo [0].fSpeed,
			gameStates.gameplay.slowmo [1].fSpeed);

SetFontColor (GREEN_RGBA);
ScaleUp ();
int32_t s = LineSpacing ();
int32_t x = gameData.renderData.scene.Width () / 2;
int32_t y = gameData.renderData.scene.Height () / 2 + ScaleY (Y_GAUGE_OFFSET (0)) + ((nLayout == 1) ? 4 : 2) * s;
if (nLayout == 2)
	y += s; //(gameOpts->render.cockpit.bSeparators ? s / 2 : s);
if (EGI_FLAG (nDamageModel, 0, 0, 0))
	y += gameOpts->render.cockpit.bSeparators ? s / 2 : s;
int32_t w, h, aw;
fontManager.Current ()->StringSize (szSlowMotion, w, h, aw);
w /= 2;
DrawHUDText (NULL, x - w, y, "%s", szSlowMotion);
if (gameOpts->render.cockpit.bSeparators) {
	int32_t l = x - w - 4;
	int32_t r = x + w + 1;
	int32_t t = y - 4;
	int32_t b = y + h + 3;

	DrawSeparator (x - 3 * w / 2, t - 4, x + 3 * w / 2 - 1, t - 4);

	//CCanvas::Current ()->Color ().Set (0, 255, 0, 255);
	CCanvas::Current ()->Color ().Set (0, 160, 0, 100);
	OglDrawLine (l, t + 4, l, b - 3);
	OglDrawLine (l + 1, t + 2, l + 1, b - 1);
	OglDrawLine (r, t + 4, r, b - 3);
	OglDrawLine (r - 1, t + 2, r - 1, b - 1);

	OglDrawLine (l + 3, t, r - 4, t);
	OglDrawLine (l + 1, t + 1, r - 2, t + 1);
	OglDrawLine (l + 3, b, r - 4, b);
	OglDrawLine (l + 1, b - 1, r - 2, b - 1);

	CCanvas::Current ()->Color ().Set (0, 160, 0, 100);
	OglDrawFilledRect (l + 1, t + 2, r - 2, b - 1);
	}
ScaleDown ();
}

//	-----------------------------------------------------------------------------

void CHUD::DrawStatic (int32_t nWindow)
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
//gameData.renderData.frame.SetLeft ((gameData.renderData.screen.Width () - gameData.renderData.frame.Width ()) / 2);
//gameData.renderData.frame.SetTop ((gameData.renderData.screen.Height () - gameData.renderData.frame.Height ()) / 2);
}

//	-----------------------------------------------------------------------------

bool CHUD::Setup (bool bScene, bool bRebuild)
{
if (bRebuild && !m_info.bRebuild)
	return true;
m_info.bRebuild = false;
if (!CGenericCockpit::Setup (bScene, bRebuild))
	return false;
//Canvas ()->Reactivate ();
//if (!gameStates.render.cameras.bActive)
	gameData.renderData.scene.Activate ("HUD::Setup (scene)");
return true;
}

//	-----------------------------------------------------------------------------

void CHUD::SetupWindow (int32_t nWindow)
{
	static int32_t cockpitWindowScale [4] = {8, 6, 4, 3};

	int32_t x, y, nWindowPos;

int32_t w = (int32_t) (gameData.renderData.frame.Width () / cockpitWindowScale [gameOpts->render.cockpit.nWindowSize] * HUD_ASPECT);	
int32_t h = I2X (w) / gameData.renderData.screen.Aspect ();
if (!gameStates.app.bNostalgia) //(gameOpts->render.cockpit.bWideDisplays)
	w = int32_t (float (w) * float (gameData.renderData.screen.Width ()) / float (gameData.renderData.screen.Height ()) * 0.75f);
	//w = int32_t (w / HUD_ASPECT);
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
	y = gameData.renderData.scene.Height () - h - h / 10;
	if (extraGameInfo [0].nWeaponIcons &&
		 (extraGameInfo [0].nWeaponIcons - gameOpts->render.weaponIcons.bEquipment < 3))
		y -= (int32_t) ((gameOpts->render.weaponIcons.bSmall ? 20.0 : 30.0) * (double) CCanvas::Current ()->Height () / 480.0);
	}
//copy these vars so stereo code can get at them
CCockpitInfo::bWindowDrawn [nWindow] = 1;
gameData.renderData.window.Setup (&gameData.renderData.scene, x, y, w, h);
gameData.renderData.window.Activate ("CHUD::SetupWindow (window)", &gameData.renderData.scene);
glClearDepth (1.0);
glClear (GL_DEPTH_BUFFER_BIT);
}

//	-----------------------------------------------------------------------------

void CHUD::Toggle (void)
{
CGenericCockpit::Activate (CM_LETTERBOX, true);
}

//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------

bool CWideHUD::Setup (bool bScene, bool bRebuild)
{
if (bRebuild && !m_info.bRebuild)
	return true;
m_info.bRebuild = false;
if (!CGenericCockpit::Setup (bScene, bRebuild))
	return false;
// 480.0f / 640.0f * 0.7f is the aspect ratio of the widescreen mode in the reference resolution 640x480 
int32_t h = gameData.renderData.screen.Height () - 21 * gameData.renderData.screen.Width () / 40; //(int32_t) FRound ((float) gameData.renderData.frame.Width () * (480.0f / 640.0f * 0.7f));
*((CViewport*) &gameData.renderData.scene) += CViewport (0, h / 2, 0, -h);
gameData.renderData.scene.Activate ("CWideHUD::Setup (scene)");
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

void CWideHUD::SetupWindow (int32_t nWindow)
{
CHUD::SetupWindow (nWindow);
#if 0
if (SW_y [nWindow] + SW_h [nWindow] > gameData.renderData.frame.Bottom ())
	SW_y [nWindow] -= (gameData.renderData.screen.Height () - gameData.renderData.frame.Height ());
gameData.renderData.frame.SetupPane (pCanvas, SW_x [nWindow], SW_y [nWindow], SW_w [nWindow], SW_h [nWindow]);
#endif
}

//	-----------------------------------------------------------------------------
