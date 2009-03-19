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

#include "menu.h"
#include "descent.h"
#include "ipx.h"
#include "key.h"
#include "iff.h"
#include "u_mem.h"
#include "error.h"
#include "screens.h"
#include "joy.h"
#include "slew.h"
#include "args.h"
#include "hogfile.h"
#include "newdemo.h"
#include "timer.h"
#include "text.h"
#include "gamefont.h"
#include "menu.h"
#include "network.h"
#include "network_lib.h"
#include "netmenu.h"
#include "scores.h"
#include "joydefs.h"
#include "playsave.h"
#include "kconfig.h"
#include "credits.h"
#include "texmap.h"
#include "state.h"
#include "movie.h"
#include "gamepal.h"
#include "cockpit.h"
#include "strutil.h"
#include "reorder.h"
#include "rendermine.h"
#include "light.h"
#include "lightmap.h"
#include "autodl.h"
#include "tracker.h"
#include "omega.h"
#include "lightning.h"
#include "vers_id.h"
#include "input.h"
#include "collide.h"
#include "objrender.h"
#include "sparkeffect.h"
#include "renderthreads.h"
#include "soundthreads.h"
#include "menubackground.h"

//------------------------------------------------------------------------------

static int	nCWSopt, nCWZopt, optTextGauges, optWeaponIcons, bShowWeaponIcons, 
				optIconAlpha, optTgtInd, optDmgInd, optHitInd, optMslLockInd;

static const char *szCWS [4];

//------------------------------------------------------------------------------

int TgtIndOptionsCallback (CMenu& menu, int& key, int nCurItem, int nState)
{
if (nState)
	return nCurItem;

	CMenuItem	*m;
	int			v, j;

m = menu + optTgtInd;
v = m->m_value;
if (v != (extraGameInfo [0].bTargetIndicators == 0)) {
	for (j = 0; j < 3; j++)
		if (m [optTgtInd + j].m_value) {
			extraGameInfo [0].bTargetIndicators = j;
			break;
			}
	key = -2;
	return nCurItem;
	}
m = menu + optDmgInd;
v = m->m_value;
if (v != extraGameInfo [0].bDamageIndicators) {
	extraGameInfo [0].bDamageIndicators = v;
	key = -2;
	return nCurItem;
	}
m = menu + optMslLockInd;
v = m->m_value;
if (v != extraGameInfo [0].bMslLockIndicators) {
	extraGameInfo [0].bMslLockIndicators = v;
	key = -2;
	return nCurItem;
	}
return nCurItem;
}

//------------------------------------------------------------------------------

void TgtIndOptionsMenu (void)
{
	CMenu	m;
	int	i, j, choice = 0;
	int	optCloakedInd, optRotateInd;

do {
	m.Destroy ();
	m.Create (15);

	optTgtInd = m.AddRadio (TXT_TGTIND_NONE, 0, KEY_A, HTX_CPIT_TGTIND);
	m.AddRadio (TXT_TGTIND_SQUARE, 0, KEY_R, HTX_CPIT_TGTIND);
	m.AddRadio (TXT_TGTIND_TRIANGLE, 0, KEY_T, HTX_CPIT_TGTIND);
	m [optTgtInd + extraGameInfo [0].bTargetIndicators].m_value = 1;
	if (extraGameInfo [0].bTargetIndicators)
		optCloakedInd = m.AddCheck (TXT_CLOAKED_INDICATOR, extraGameInfo [0].bCloakedIndicators, KEY_C, HTX_CLOAKED_INDICATOR);
	else
		optCloakedInd = -1;
	optDmgInd = m.AddCheck (TXT_DMG_INDICATOR, extraGameInfo [0].bDamageIndicators, KEY_D, HTX_CPIT_DMGIND);
	if (extraGameInfo [0].bTargetIndicators || extraGameInfo [0].bDamageIndicators)
		optHitInd = m.AddCheck (TXT_HIT_INDICATOR, extraGameInfo [0].bTagOnlyHitObjs, KEY_T, HTX_HIT_INDICATOR);
	else
		optHitInd = -1;
	optMslLockInd = m.AddCheck (TXT_MSLLOCK_INDICATOR, extraGameInfo [0].bMslLockIndicators, KEY_M, HTX_CPIT_MSLLOCKIND);
	if (extraGameInfo [0].bMslLockIndicators)
		optRotateInd = m.AddCheck (TXT_ROTATE_MSLLOCKIND, gameOpts->render.cockpit.bRotateMslLockInd, KEY_R, HTX_ROTATE_MSLLOCKIND);
	else
		optRotateInd = -1;
	do {
		i = m.Menu (NULL, TXT_TGTIND_MENUTITLE, &TgtIndOptionsCallback, &choice);
	} while (i >= 0);
	if (optTgtInd >= 0) {
		for (j = 0; j < 3; j++)
			if (m [optTgtInd + j].m_value) {
				extraGameInfo [0].bTargetIndicators = j;
				break;
				}
		GET_VAL (extraGameInfo [0].bCloakedIndicators, optCloakedInd);
		}
	GET_VAL (extraGameInfo [0].bDamageIndicators, optDmgInd);
	GET_VAL (extraGameInfo [0].bMslLockIndicators, optMslLockInd);
	GET_VAL (gameOpts->render.cockpit.bRotateMslLockInd, optRotateInd);
	GET_VAL (extraGameInfo [0].bTagOnlyHitObjs, optHitInd);
	} while (i == -2);
}

//------------------------------------------------------------------------------

int WeaponIconOptionsCallback (CMenu& menu, int& key, int nCurItem, int nState)
{
if (nState)
	return nCurItem;

	CMenuItem	*m;
	int			v;

m = menu + optWeaponIcons;
v = m->m_value;
if (v != bShowWeaponIcons) {
	bShowWeaponIcons = v;
	key = -2;
	return nCurItem;
	}
return nCurItem;
}

//------------------------------------------------------------------------------

void WeaponIconOptionsMenu (void)
{
	CMenu m (35);
	int	i, j, choice = 0;

#if SIMPLE_MENUS

	int	optIconPos, optHiliteColor;

bShowWeaponIcons = (extraGameInfo [0].nWeaponIcons != 0);
do {
	m.Destroy ();
	m.Create (35);
	optWeaponIcons = m.AddCheck (TXT_SHOW_WEAPONICONS, bShowWeaponIcons, KEY_W, HTX_CPIT_WPNICONS);
	if (bShowWeaponIcons) {
		m.AddText ("", 0);
		optIconPos = m.AddRadio (TXT_WPNICONS_TOP, 0, KEY_I, HTX_CPIT_ICONPOS);
		m.AddRadio (TXT_WPNICONS_BTM, 0, KEY_I, HTX_CPIT_ICONPOS);
		m.AddRadio (TXT_WPNICONS_LRB, 0, KEY_I, HTX_CPIT_ICONPOS);
		m.AddRadio (TXT_WPNICONS_LRT, 0, KEY_I, HTX_CPIT_ICONPOS);
		m [optIconPos + NMCLAMP (extraGameInfo [0].nWeaponIcons - 1, 0, 3)].m_value = 1;
		m.AddText ("", 0);
		optHiliteColor = m.AddRadio (TXT_HUD_HILITE_YELLOW, 0, KEY_Y, HTX_HUD_HILITE_COLOR);
		m.AddRadio (TXT_HUD_HILITE_BLUE, 0, KEY_B, HTX_HUD_HILITE_COLOR);
		m [optHiliteColor + NMCLAMP (gameOpts->render.weaponIcons.nHiliteColor, 0, 1)].m_value = 1;
		m.AddText ("", 0);
		}
	else
		optIconPos = -1;
	do {
		i = m.Menu (NULL, TXT_WPNICON_MENUTITLE, &WeaponIconOptionsCallback, &choice);
	} while (i >= 0);
	if (bShowWeaponIcons) {
		if (gameOpts->app.bExpertMode) {
			if (optIconPos >= 0) {
				for (j = 0; j < 4; j++)
					if (m [optIconPos + j].m_value) {
						extraGameInfo [0].nWeaponIcons = j + 1;
						break;
						}
				}
			if (optHiliteColor >= 0)
				gameOpts->render.weaponIcons.nHiliteColor = m [optHiliteColor + 1].m_value != 0;
			GET_VAL (gameOpts->render.weaponIcons.alpha, optIconAlpha);
			}
		else {
#if EXPMODE_DEFAULTS
			gameOpts->render.weaponIcons.bEquipment = 1;
			gameOpts->render.weaponIcons.bSmall = 1;
			gameOpts->render.weaponIcons.nSort = 1;
			gameOpts->render.weaponIcons.bShowAmmo = 1;
			gameOpts->render.weaponIcons.alpha = 3;
#endif
			}
		}
	else
		extraGameInfo [0].nWeaponIcons = 0;
	} while (i == -2);

gameOpts->render.weaponIcons.bEquipment = bShowWeaponIcons;
gameOpts->render.weaponIcons.bSmall = 1;
gameOpts->render.weaponIcons.nSort = 1;
gameOpts->render.weaponIcons.bShowAmmo = 1;
gameOpts->render.weaponIcons.bBoldHighlight = 1;
gameOpts->render.weaponIcons.alpha = 3;

#else

	int	optSmallIcons, optIconSort, optIconAmmo, optIconPos, optEquipIcons, optBoldHilite, optHiliteColor;

bShowWeaponIcons = (extraGameInfo [0].nWeaponIcons != 0);
do {
	m.Destroy ();
	m.Create (35);
	optWeaponIcons = m.AddCheck (TXT_SHOW_WEAPONICONS, bShowWeaponIcons, KEY_W, HTX_CPIT_WPNICONS);
	if (bShowWeaponIcons && gameOpts->app.bExpertMode) {
		optEquipIcons = m.AddCheck (TXT_SHOW_EQUIPICONS, gameOpts->render.weaponIcons.bEquipment, KEY_Q, HTX_CPIT_EQUIPICONS);
		optSmallIcons = m.AddCheck (TXT_SMALL_WPNICONS, gameOpts->render.weaponIcons.bSmall, KEY_I, HTX_CPIT_SMALLICONS);
		optIconSort = m.AddCheck (TXT_SORT_WPNICONS, gameOpts->render.weaponIcons.nSort, KEY_T, HTX_CPIT_SORTICONS);
		optIconAmmo = m.AddCheck (TXT_AMMO_WPNICONS, gameOpts->render.weaponIcons.bShowAmmo, KEY_A, HTX_CPIT_ICONAMMO);
		optBoldHilite = m.AddCheck (TXT_BOLD_ICON_HIGHLIGHT, gameOpts->render.weaponIcons.bBoldHighlight, KEY_B, HTX_BOLD_ICON_HIGHLIGHT);
		m.AddText ("", 0);
		optIconPos = m.AddRadio (TXT_WPNICONS_TOP, 0, KEY_I, HTX_CPIT_ICONPOS);
		m.AddRadio (TXT_WPNICONS_BTM, 0, KEY_I, HTX_CPIT_ICONPOS);
		m.AddRadio (TXT_WPNICONS_LRB, 0, KEY_I, HTX_CPIT_ICONPOS);
		m.AddRadio (TXT_WPNICONS_LRT, 0, KEY_I, HTX_CPIT_ICONPOS);
		m [optIconPos + NMCLAMP (extraGameInfo [0].nWeaponIcons - 1, 0, 3)].m_value = 1;
		m.AddText ("", 0);
		optHiliteColor = m.AddRadio (TXT_HUD_HILITE_YELLOW, 0, KEY_Y, HTX_HUD_HILITE_COLOR);
		m.AddRadio (TXT_HUD_HILITE_BLUE, 0, KEY_B, HTX_HUD_HILITE_COLOR);
		m [optHiliteColor + NMCLAMP (gameOpts->render.weaponIcons.nHiliteColor, 0, 1)].m_value = 1;
		m.AddText ("", 0);
		optIconAlpha = m.AddSlider (TXT_ICON_DIM, gameOpts->render.weaponIcons.alpha, 0, 8, KEY_D, HTX_CPIT_ICONDIM);
		m.AddText ("", 0);
		}
	else
		optEquipIcons =
		optSmallIcons =
		optIconSort =
		optIconPos =
		optIconAmmo = 
		optBoldHilite =
		optIconAlpha = -1;
	do {
		i = m.Menu (NULL, TXT_WPNICON_MENUTITLE, &WeaponIconOptionsCallback, &choice);
	} while (i >= 0);
	if (bShowWeaponIcons) {
		if (gameOpts->app.bExpertMode) {
			GET_VAL (gameOpts->render.weaponIcons.bEquipment, optEquipIcons);
			GET_VAL (gameOpts->render.weaponIcons.bSmall, optSmallIcons);
			GET_VAL (gameOpts->render.weaponIcons.nSort, optIconSort);
			GET_VAL (gameOpts->render.weaponIcons.bShowAmmo, optIconAmmo);
			GET_VAL (gameOpts->render.weaponIcons.bBoldHighlight, optBoldHilite);
			if (optIconPos >= 0)
				for (j = 0; j < 4; j++)
					if (m [optIconPos + j].m_value) {
						extraGameInfo [0].nWeaponIcons = j + 1;
						break;
						}
			if (optHiliteColor >= 0)
				gameOpts->render.weaponIcons.nHiliteColor = m [optHiliteColor + 1].m_value != 0;
			GET_VAL (gameOpts->render.weaponIcons.alpha, optIconAlpha);
			}
		else {
#if EXPMODE_DEFAULTS
			gameOpts->render.weaponIcons.bEquipment = 1;
			gameOpts->render.weaponIcons.bSmall = 1;
			gameOpts->render.weaponIcons.nSort = 1;
			gameOpts->render.weaponIcons.bShowAmmo = 1;
			gameOpts->render.weaponIcons.alpha = 3;
#endif
			}
		}
	else
		extraGameInfo [0].nWeaponIcons = 0;
	} while (i == -2);

#endif

}

//------------------------------------------------------------------------------

int GaugeOptionsCallback (CMenu& menu, int& key, int nCurItem, int nState)
{
if (nState)
	return nCurItem;

	CMenuItem	*m;
	int			v;

m = menu + optTextGauges;
v = !m->m_value;
if (v != gameOpts->render.cockpit.bTextGauges) {
	gameOpts->render.cockpit.bTextGauges = v;
	key = -2;
	return nCurItem;
	}
return nCurItem;
}

//------------------------------------------------------------------------------

#if !SIMPLE_MENUS

void GaugeOptionsMenu (void)
{
	CMenu m;
	int	i, choice = 0;
	int	optScaleGauges, optFlashGauges, optShieldWarn, optObjectTally, optPlayerStats;

do {
	m.Destroy ();
	m.Create (10);
	optTextGauges = m.AddCheck (TXT_SHOW_GFXGAUGES, !gameOpts->render.cockpit.bTextGauges, KEY_P, HTX_CPIT_GFXGAUGES);
	if (!gameOpts->render.cockpit.bTextGauges && gameOpts->app.bExpertMode) {
		optScaleGauges = m.AddCheck (TXT_SCALE_GAUGES, gameOpts->render.cockpit.bScaleGauges, KEY_C, HTX_CPIT_SCALEGAUGES);
		optFlashGauges = m.AddCheck (TXT_FLASH_GAUGES, gameOpts->render.cockpit.bFlashGauges, KEY_F, HTX_CPIT_FLASHGAUGES);
		}
	else
		optScaleGauges =
		optFlashGauges = -1;
	optShieldWarn = m.AddCheck (TXT_SHIELD_WARNING, gameOpts->gameplay.bShieldWarning, KEY_W, HTX_CPIT_SHIELDWARN);
	optObjectTally = m.AddCheck (TXT_OBJECT_TALLY, gameOpts->render.cockpit.bObjectTally, KEY_T, HTX_CPIT_OBJTALLY);
	optPlayerStats = m.AddCheck (TXT_PLAYER_STATS, gameOpts->render.cockpit.bPlayerStats, KEY_S, HTX_CPIT_PLAYERSTATS);
	do {
		i = m.Menu (NULL, TXT_GAUGES_MENUTITLE, &GaugeOptionsCallback, &choice);
	} while (i >= 0);
	GET_VAL (gameOpts->gameplay.bShieldWarning, optShieldWarn);
	GET_VAL (gameOpts->render.cockpit.bObjectTally, optObjectTally);
	GET_VAL (gameOpts->render.cockpit.bPlayerStats, optPlayerStats);
	if (!(gameOpts->render.cockpit.bTextGauges = !m [optTextGauges].m_value)) {
		if (gameOpts->app.bExpertMode) {
			GET_VAL (gameOpts->render.cockpit.bScaleGauges, optScaleGauges);
			GET_VAL (gameOpts->render.cockpit.bFlashGauges, optFlashGauges);
			}
		else {
#if EXPMODE_DEFAULTS
			gameOpts->render.cockpit.bScaleGauges = 1;
			gameOpts->render.cockpit.bFlashGauges = 1;
			gameOpts->gameplay.bShieldWarning = 0;
			gameOpts->render.cockpit.bObjectTally = 0;
			gameOpts->render.cockpit.bPlayerStats = 0;
#endif
			}
		}
	} while (i == -2);
}

#endif


//------------------------------------------------------------------------------

int CockpitOptionsCallback (CMenu& menu, int& key, int nCurItem, int nState)
{
if (nState)
	return nCurItem;

	CMenuItem	*m;
	int			v;

if (gameOpts->app.bExpertMode) {
	m = menu + nCWSopt;
	v = m->m_value;
	if (gameOpts->render.cockpit.nWindowSize != v) {
		gameOpts->render.cockpit.nWindowSize = v;
		m->SetText (szCWS [v]);
		m->m_bRebuild = 1;
		}

	m = menu + nCWZopt;
	v = m->m_value;
	if (gameOpts->render.cockpit.nWindowZoom != v) {
		gameOpts->render.cockpit.nWindowZoom = v;
		sprintf (m->m_text, TXT_CW_ZOOM, gameOpts->render.cockpit.nWindowZoom + 1);
		m->m_bRebuild = 1;
		}
	}
return nCurItem;
}

//------------------------------------------------------------------------------

void CockpitOptionsMenu (void)
{
	CMenu m;
	int	i, 
			nPosition = gameOpts->render.cockpit.nWindowPos / 3, 
			nAlignment = gameOpts->render.cockpit.nWindowPos % 3, 
			choice = 0;
	int	optTextGauges, optHUD, optPosition, optAlignment, optTgtInd, optWeaponIcons;

	char szCockpitWindowZoom [40];

szCWS [0] = TXT_CWS_SMALL;
szCWS [1] = TXT_CWS_MEDIUM;
szCWS [2] = TXT_CWS_LARGE;
szCWS [3] = TXT_CWS_HUGE;
optPosition = optAlignment = nCWSopt = nCWZopt = optTextGauges = optWeaponIcons = -1;
bShowWeaponIcons = (extraGameInfo [0].nWeaponIcons != 0);

#if SIMPLE_MENUS

do {
	m.Destroy ();
	m.Create (20);

	if (gameOpts->app.bExpertMode) {
		nCWSopt = m.AddSlider (szCWS [gameOpts->render.cockpit.nWindowSize], gameOpts->render.cockpit.nWindowSize, 0, 3, KEY_S, HTX_CPIT_WINSIZE);
		sprintf (szCockpitWindowZoom, TXT_CW_ZOOM, gameOpts->render.cockpit.nWindowZoom + 1);
		nCWZopt = m.AddSlider (szCockpitWindowZoom, gameOpts->render.cockpit.nWindowZoom, 0, 3, KEY_Z, HTX_CPIT_WINZOOM);
		m.AddText ("", 0);
		m.AddText (TXT_AUXWIN_POSITION, 0);
		optPosition = m.AddRadio (TXT_POS_BOTTOM, nPosition == 0, KEY_B, HTX_AUXWIN_POSITION);
		m.AddRadio (TXT_POS_TOP, nPosition == 1, KEY_T, HTX_AUXWIN_POSITION);
		m.AddText ("", 0);
		m.AddText (TXT_AUXWIN_ALIGNMENT, 0);
		optAlignment = m.AddRadio (TXT_ALIGN_CORNERS, nAlignment == 0, KEY_O, HTX_AUXWIN_ALIGNMENT);
		m.AddRadio (TXT_ALIGN_MIDDLE, nAlignment == 1, KEY_I, HTX_AUXWIN_ALIGNMENT);
		m.AddRadio (TXT_ALIGN_CENTER, nAlignment == 2, KEY_E, HTX_AUXWIN_ALIGNMENT);
		m.AddText ("", 0);
		}
	else {
		gameOpts->render.cockpit.nWindowSize = 0;
		gameOpts->render.cockpit.nWindowZoom = 0;
		gameOpts->render.cockpit.nWindowPos = 1;
		}
	optHUD = m.AddCheck (TXT_SHOW_HUD, gameOpts->render.cockpit.bHUD, KEY_U, HTX_CPIT_SHOWHUD);
	optTextGauges = m.AddCheck (TXT_SHOW_GFXGAUGES, !gameOpts->render.cockpit.bTextGauges, KEY_P, HTX_CPIT_GFXGAUGES);
	m.AddText ("", 0);

	optTgtInd = m.AddMenu (TXT_TGTIND_MENUCALL, KEY_T, "");
	optWeaponIcons = m.AddMenu (TXT_WPNICON_MENUCALL, KEY_W, "");
	do {
		i = m.Menu (NULL, TXT_COCKPIT_OPTS, &CockpitOptionsCallback, &choice);
		if (i < 0)
			break;
		if ((optTgtInd >= 0) && (i == optTgtInd))
			TgtIndOptionsMenu ();
		else if ((optWeaponIcons >= 0) && (i == optWeaponIcons))
			WeaponIconOptionsMenu ();
	} while (i >= 0);

	GET_VAL (gameOpts->render.cockpit.bHUD, optHUD);
	gameOpts->render.cockpit.bTextGauges = !m [optTextGauges].m_value;

	if (gameOpts->app.bExpertMode) {
		if ((optAlignment >= 0) && (optPosition >= 0)) {
			for (nPosition = 0; nPosition < 2; nPosition++)
				if (m [optPosition + nPosition].m_value)
					break;
			for (nAlignment = 0; nAlignment < 3; nAlignment++)
				if (m [optAlignment + nAlignment].m_value)
					break;
			gameOpts->render.cockpit.nWindowPos = nPosition * 3 + nAlignment;
			}
		}
	} while (i == -2);


gameOpts->render.cockpit.bScaleGauges = 1;
gameOpts->render.cockpit.bFlashGauges = 1;
gameOpts->gameplay.bShieldWarning = 0;
gameOpts->render.cockpit.bObjectTally = 1;
gameOpts->render.cockpit.bPlayerStats = 0;

gameOpts->render.cockpit.bReticle = 1;
gameOpts->render.cockpit.bMissileView = 1;
gameOpts->render.cockpit.bGuidedInMainView = 1;
gameOpts->render.cockpit.bMouseIndicator = 1;
gameOpts->render.cockpit.bHUDMsgs = 1;
gameOpts->render.cockpit.bSplitHUDMsgs = 1;
gameOpts->render.cockpit.bWideDisplays = 1;

#else

	int	optGauges, optReticle, optGuided, optWideDisplays,
			optMissileView, optMouseInd, optSplitMsgs, optHUDMsgs;
do {
	m.Destroy ();
	m.Create (30);

	if (gameOpts->app.bExpertMode) {
		nCWSopt = m.AddSlider (szCWS [gameOpts->render.cockpit.nWindowSize], gameOpts->render.cockpit.nWindowSize, 0, 3, KEY_S, HTX_CPIT_WINSIZE);
		sprintf (szCockpitWindowZoom, TXT_CW_ZOOM, gameOpts->render.cockpit.nWindowZoom + 1);
		nCWZopt = m.AddSlider (szCockpitWindowZoom, gameOpts->render.cockpit.nWindowZoom, 0, 3, KEY_Z, HTX_CPIT_WINZOOM);
		m.AddText ("", 0);
		m.AddText (TXT_AUXWIN_POSITION, 0);
		optPosition = m.AddRadio (TXT_POS_BOTTOM, nPosition == 0, KEY_B, HTX_AUXWIN_POSITION);
		m.AddRadio (TXT_POS_TOP, nPosition == 1, KEY_T, HTX_AUXWIN_POSITION);
		m.AddText ("", 0);
		m.AddText (TXT_AUXWIN_ALIGNMENT, 0);
		optAlignment = m.AddRadio (TXT_ALIGN_CORNERS, nAlignment == 0, KEY_O, HTX_AUXWIN_ALIGNMENT);
		m.AddRadio (TXT_ALIGN_MIDDLE, nAlignment == 1, KEY_I, HTX_AUXWIN_ALIGNMENT);
		m.AddRadio (TXT_ALIGN_CENTER, nAlignment == 2, KEY_E, HTX_AUXWIN_ALIGNMENT);
		m.AddText ("", 0);
		optHUD = m.AddCheck (TXT_SHOW_HUD, gameOpts->render.cockpit.bHUD, KEY_U, HTX_CPIT_SHOWHUD);
		optWideDisplays = m.AddCheck (TXT_CPIT_WIDE_DISPLAYS, gameOpts->render.cockpit.bWideDisplays, KEY_D, HTX_CPIT_WIDE_DISPLAYS);
		optHUDMsgs = m.AddCheck (TXT_SHOW_HUDMSGS, gameOpts->render.cockpit.bHUDMsgs, KEY_M, HTX_CPIT_SHOWHUDMSGS);
		optReticle = m.AddCheck (TXT_SHOW_RETICLE, gameOpts->render.cockpit.bReticle, KEY_R, HTX_CPIT_SHOWRETICLE);
		if (gameOpts->input.mouse.bJoystick)
			optMouseInd = m.AddCheck (TXT_SHOW_MOUSEIND, gameOpts->render.cockpit.bMouseIndicator, KEY_O, HTX_CPIT_MOUSEIND);
		else
			optMouseInd = -1;
		}
	else
		optHUD =
		optWideDisplays =
		optHUDMsgs =
		optMouseInd = 
		optReticle = -1;
	optSplitMsgs = m.AddCheck (TXT_EXTRA_PLRMSGS, gameOpts->render.cockpit.bSplitHUDMsgs, KEY_P, HTX_CPIT_SPLITMSGS);
	optMissileView = m.AddCheck (TXT_MISSILE_VIEW, gameOpts->render.cockpit.bMissileView, KEY_I, HTX_CPITMSLVIEW);
	optGuided = m.AddCheck (TXT_GUIDED_MAINVIEW, gameOpts->render.cockpit.bGuidedInMainView, KEY_F, HTX_CPIT_GUIDEDVIEW);
	m.AddText ("", 0);
	optTgtInd = m.AddMenu (TXT_TGTIND_MENUCALL, KEY_T, "");
	optWeaponIcons = m.AddMenu (TXT_WPNICON_MENUCALL, KEY_W, "");
	optGauges = m.AddMenu (TXT_GAUGES_MENUCALL, KEY_G, "");
	do {
		i = m.Menu (NULL, TXT_COCKPIT_OPTS, &CockpitOptionsCallback, &choice);
		if (i < 0)
			break;
		if ((optTgtInd >= 0) && (i == optTgtInd))
			TgtIndOptionsMenu ();
		else if ((optWeaponIcons >= 0) && (i == optWeaponIcons))
			WeaponIconOptionsMenu ();
		else if ((optGauges >= 0) && (i == optGauges))
			GaugeOptionsMenu ();
	} while (i >= 0);
	GET_VAL (gameOpts->render.cockpit.bReticle, optReticle);
	GET_VAL (gameOpts->render.cockpit.bMissileView, optMissileView);
	GET_VAL (gameOpts->render.cockpit.bGuidedInMainView, optGuided);
	GET_VAL (gameOpts->render.cockpit.bMouseIndicator, optMouseInd);
	GET_VAL (gameOpts->render.cockpit.bHUD, optHUD);
	GET_VAL (gameOpts->render.cockpit.bWideDisplays, optWideDisplays);
	GET_VAL (gameOpts->render.cockpit.bHUDMsgs, optHUDMsgs);
	GET_VAL (gameOpts->render.cockpit.bSplitHUDMsgs, optSplitMsgs);
	if ((optAlignment >= 0) && (optPosition >= 0)) {
		for (nPosition = 0; nPosition < 2; nPosition++)
			if (m [optPosition + nPosition].m_value)
				break;
		for (nAlignment = 0; nAlignment < 3; nAlignment++)
			if (m [optAlignment + nAlignment].m_value)
				break;
		gameOpts->render.cockpit.nWindowPos = nPosition * 3 + nAlignment;
		}
	} while (i == -2);

#endif

}

//------------------------------------------------------------------------------
//eof
