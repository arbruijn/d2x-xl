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
#include "inferno.h"
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
#include "newmenu.h"
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
#include "gauges.h"
#include "strutil.h"
#include "reorder.h"
#include "render.h"
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
#ifdef EDITOR
#	include "editor/editor.h"
#endif

//------------------------------------------------------------------------------

static int	nCWSopt, nCWZopt, optTextGauges, optWeaponIcons, bShowWeaponIcons, 
				optIconAlpha, optTgtInd, optDmgInd, optHitInd, optMslLockInd;

static const char *szCWS [4];

//------------------------------------------------------------------------------

int TgtIndOptionsCallback (int nitems, CMenuItem * menus, int * key, int nCurItem)
{
	CMenuItem	*m;
	int			v, j;

m = menus + optTgtInd;
v = m->value;
if (v != (extraGameInfo [0].bTargetIndicators == 0)) {
	for (j = 0; j < 3; j++)
		if (m [optTgtInd + j].value) {
			extraGameInfo [0].bTargetIndicators = j;
			break;
			}
	*key = -2;
	return nCurItem;
	}
m = menus + optDmgInd;
v = m->value;
if (v != extraGameInfo [0].bDamageIndicators) {
	extraGameInfo [0].bDamageIndicators = v;
	*key = -2;
	return nCurItem;
	}
m = menus + optMslLockInd;
v = m->value;
if (v != extraGameInfo [0].bMslLockIndicators) {
	extraGameInfo [0].bMslLockIndicators = v;
	*key = -2;
	return nCurItem;
	}
return nCurItem;
}

//------------------------------------------------------------------------------

void TgtIndOptionsMenu ()
{
	CMenuItem m [15];
	int	i, j, nOptions, choice = 0;
	int	optCloakedInd, optRotateInd;

do {
	memset (m, 0, sizeof (m));
	nOptions = 0;

	m.AddRadio (nOptions, TXT_TGTIND_NONE, 0, KEY_A, 1, HTX_CPIT_TGTIND);
	optTgtInd = nOptions++;
	m.AddRadio (nOptions, TXT_TGTIND_SQUARE, 0, KEY_R, 1, HTX_CPIT_TGTIND);
	nOptions++;
	m.AddRadio (nOptions, TXT_TGTIND_TRIANGLE, 0, KEY_T, 1, HTX_CPIT_TGTIND);
	nOptions++;
	m [optTgtInd + extraGameInfo [0].bTargetIndicators].value = 1;
	if (extraGameInfo [0].bTargetIndicators) {
		m.AddCheck (nOptions, TXT_CLOAKED_INDICATOR, extraGameInfo [0].bCloakedIndicators, KEY_C, HTX_CLOAKED_INDICATOR);
		optCloakedInd = nOptions++;
		}
	else
		optCloakedInd = -1;
	m.AddCheck (nOptions, TXT_DMG_INDICATOR, extraGameInfo [0].bDamageIndicators, KEY_D, HTX_CPIT_DMGIND);
	optDmgInd = nOptions++;
	if (extraGameInfo [0].bTargetIndicators || extraGameInfo [0].bDamageIndicators) {
		m.AddCheck (nOptions, TXT_HIT_INDICATOR, extraGameInfo [0].bTagOnlyHitObjs, KEY_T, HTX_HIT_INDICATOR);
		optHitInd = nOptions++;
		}
	else
		optHitInd = -1;
	m.AddCheck (nOptions, TXT_MSLLOCK_INDICATOR, extraGameInfo [0].bMslLockIndicators, KEY_M, HTX_CPIT_MSLLOCKIND);
	optMslLockInd = nOptions++;
	if (extraGameInfo [0].bMslLockIndicators) {
		m.AddCheck (nOptions, TXT_ROTATE_MSLLOCKIND, gameOpts->render.cockpit.bRotateMslLockInd, KEY_R, HTX_ROTATE_MSLLOCKIND);
		optRotateInd = nOptions++;
		}
	else
		optRotateInd = -1;
	Assert (sizeofa (m) >= (size_t) nOptions);
	do {
		i = ExecMenu1 (NULL, TXT_TGTIND_MENUTITLE, nOptions, m, &TgtIndOptionsCallback, &choice);
	} while (i >= 0);
	if (optTgtInd >= 0) {
		for (j = 0; j < 3; j++)
			if (m [optTgtInd + j].value) {
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

int WeaponIconOptionsCallback (int nitems, CMenuItem * menus, int * key, int nCurItem)
{
	CMenuItem	*m;
	int			v;

m = menus + optWeaponIcons;
v = m->value;
if (v != bShowWeaponIcons) {
	bShowWeaponIcons = v;
	*key = -2;
	return nCurItem;
	}
return nCurItem;
}

//------------------------------------------------------------------------------

void WeaponIconOptionsMenu (void)
{
	CMenuItem m [35];
	int	i, j, nOptions, choice = 0;
	int	optSmallIcons, optIconSort, optIconAmmo, optIconPos, optEquipIcons;

bShowWeaponIcons = (extraGameInfo [0].nWeaponIcons != 0);
do {
	memset (m, 0, sizeof (m));
	nOptions = 0;

	m.AddCheck (nOptions, TXT_SHOW_WEAPONICONS, bShowWeaponIcons, KEY_W, HTX_CPIT_WPNICONS);
	optWeaponIcons = nOptions++;
	if (bShowWeaponIcons && gameOpts->app.bExpertMode) {
		m.AddCheck (nOptions, TXT_SHOW_EQUIPICONS, gameOpts->render.weaponIcons.bEquipment, KEY_Q, HTX_CPIT_EQUIPICONS);
		optEquipIcons = nOptions++;
		m.AddCheck (nOptions, TXT_SMALL_WPNICONS, gameOpts->render.weaponIcons.bSmall, KEY_I, HTX_CPIT_SMALLICONS);
		optSmallIcons = nOptions++;
		m.AddCheck (nOptions, TXT_SORT_WPNICONS, gameOpts->render.weaponIcons.nSort, KEY_T, HTX_CPIT_SORTICONS);
		optIconSort = nOptions++;
		m.AddCheck (nOptions, TXT_AMMO_WPNICONS, gameOpts->render.weaponIcons.bShowAmmo, KEY_A, HTX_CPIT_ICONAMMO);
		optIconAmmo = nOptions++;
		optIconPos = nOptions;
		m.AddRadio (nOptions, TXT_WPNICONS_TOP, 0, KEY_I, 3, HTX_CPIT_ICONPOS);
		nOptions++;
		m.AddRadio (nOptions, TXT_WPNICONS_BTM, 0, KEY_I, 3, HTX_CPIT_ICONPOS);
		nOptions++;
		m.AddRadio (nOptions, TXT_WPNICONS_LRB, 0, KEY_I, 3, HTX_CPIT_ICONPOS);
		nOptions++;
		m.AddRadio (nOptions, TXT_WPNICONS_LRT, 0, KEY_I, 3, HTX_CPIT_ICONPOS);
		nOptions++;
		m [optIconPos + NMCLAMP (extraGameInfo [0].nWeaponIcons - 1, 0, 3)].value = 1;
		m.AddSlider (nOptions, TXT_ICON_DIM, gameOpts->render.weaponIcons.alpha, 0, 8, KEY_D, HTX_CPIT_ICONDIM);
		optIconAlpha = nOptions++;
		m.AddText (nOptions, "", 0);
		nOptions++;
		}
	else
		optEquipIcons =
		optSmallIcons =
		optIconSort =
		optIconPos =
		optIconAmmo = 
		optIconAlpha = -1;
	Assert (sizeofa (m) >= (size_t) nOptions);
	do {
		i = ExecMenu1 (NULL, TXT_WPNICON_MENUTITLE, nOptions, m, &WeaponIconOptionsCallback, &choice);
	} while (i >= 0);
	if (bShowWeaponIcons) {
		if (gameOpts->app.bExpertMode) {
			GET_VAL (gameOpts->render.weaponIcons.bEquipment, optEquipIcons);
			GET_VAL (gameOpts->render.weaponIcons.bSmall, optSmallIcons);
			GET_VAL (gameOpts->render.weaponIcons.nSort, optIconSort);
			GET_VAL (gameOpts->render.weaponIcons.bShowAmmo, optIconAmmo);
			if (optIconPos >= 0)
				for (j = 0; j < 4; j++)
					if (m [optIconPos + j].value) {
						extraGameInfo [0].nWeaponIcons = j + 1;
						break;
						}
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
}

//------------------------------------------------------------------------------

int GaugeOptionsCallback (int nitems, CMenuItem * menus, int * key, int nCurItem)
{
	CMenuItem	*m;
	int			v;

m = menus + optTextGauges;
v = !m->value;
if (v != gameOpts->render.cockpit.bTextGauges) {
	gameOpts->render.cockpit.bTextGauges = v;
	*key = -2;
	return nCurItem;
	}
return nCurItem;
}

//------------------------------------------------------------------------------

void GaugeOptionsMenu (void)
{
	CMenuItem m [10];
	int	i, nOptions, choice = 0;
	int	optScaleGauges, optFlashGauges, optShieldWarn, optObjectTally, optPlayerStats;

do {
	memset (m, 0, sizeof (m));
	nOptions = 0;
	m.AddCheck (nOptions, TXT_SHOW_GFXGAUGES, !gameOpts->render.cockpit.bTextGauges, KEY_P, HTX_CPIT_GFXGAUGES);
	optTextGauges = nOptions++;
	if (!gameOpts->render.cockpit.bTextGauges && gameOpts->app.bExpertMode) {
		m.AddCheck (nOptions, TXT_SCALE_GAUGES, gameOpts->render.cockpit.bScaleGauges, KEY_C, HTX_CPIT_SCALEGAUGES);
		optScaleGauges = nOptions++;
		m.AddCheck (nOptions, TXT_FLASH_GAUGES, gameOpts->render.cockpit.bFlashGauges, KEY_F, HTX_CPIT_FLASHGAUGES);
		optFlashGauges = nOptions++;
		}
	else
		optScaleGauges =
		optFlashGauges = -1;
	m.AddCheck (nOptions, TXT_SHIELD_WARNING, gameOpts->gameplay.bShieldWarning, KEY_W, HTX_CPIT_SHIELDWARN);
	optShieldWarn = nOptions++;
	m.AddCheck (nOptions, TXT_OBJECT_TALLY, gameOpts->render.cockpit.bObjectTally, KEY_T, HTX_CPIT_OBJTALLY);
	optObjectTally = nOptions++;
	m.AddCheck (nOptions, TXT_PLAYER_STATS, gameOpts->render.cockpit.bPlayerStats, KEY_S, HTX_CPIT_PLAYERSTATS);
	optPlayerStats = nOptions++;
	do {
		i = ExecMenu1 (NULL, TXT_GAUGES_MENUTITLE, nOptions, m, &GaugeOptionsCallback, &choice);
	} while (i >= 0);
	if (!(gameOpts->render.cockpit.bTextGauges = !m [optTextGauges].value)) {
		if (gameOpts->app.bExpertMode) {
			GET_VAL (gameOpts->render.cockpit.bScaleGauges, optScaleGauges);
			GET_VAL (gameOpts->render.cockpit.bFlashGauges, optFlashGauges);
			GET_VAL (gameOpts->gameplay.bShieldWarning, optShieldWarn);
			GET_VAL (gameOpts->render.cockpit.bObjectTally, optObjectTally);
			GET_VAL (gameOpts->render.cockpit.bPlayerStats, optPlayerStats);
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

//------------------------------------------------------------------------------

int CockpitOptionsCallback (int nitems, CMenuItem * menus, int * key, int nCurItem)
{
	CMenuItem	*m;
	int			v;

if (gameOpts->app.bExpertMode) {
	m = menus + nCWSopt;
	v = m->value;
	if (gameOpts->render.cockpit.nWindowSize != v) {
		gameOpts->render.cockpit.nWindowSize = v;
		m->text = const_cast<char*> (szCWS [v]);
		m->rebuild = 1;
		}

	m = menus + nCWZopt;
	v = m->value;
	if (gameOpts->render.cockpit.nWindowZoom != v) {
		gameOpts->render.cockpit.nWindowZoom = v;
		sprintf (m->text, TXT_CW_ZOOM, gameOpts->render.cockpit.nWindowZoom + 1);
		m->rebuild = 1;
		}
	}
return nCurItem;
}

//------------------------------------------------------------------------------

void CockpitOptionsMenu (void)
{
	CMenuItem m [30];
	int	i, nOptions, 
			nPosition = gameOpts->render.cockpit.nWindowPos / 3, 
			nAlignment = gameOpts->render.cockpit.nWindowPos % 3, 
			choice = 0;
	int	optGauges, optHUD, optReticle, optGuided, optPosition, optAlignment,
			optMissileView, optMouseInd, optSplitMsgs, optHUDMsgs, optTgtInd, optWeaponIcons;

	char szCockpitWindowZoom [40];

szCWS [0] = TXT_CWS_SMALL;
szCWS [1] = TXT_CWS_MEDIUM;
szCWS [2] = TXT_CWS_LARGE;
szCWS [3] = TXT_CWS_HUGE;
optPosition = optAlignment = nCWSopt = nCWZopt = optTextGauges = optWeaponIcons = optIconAlpha = -1;
bShowWeaponIcons = (extraGameInfo [0].nWeaponIcons != 0);
do {
	memset (m, 0, sizeof (m));
	nOptions = 0;

	if (gameOpts->app.bExpertMode) {
		m.AddSlider (nOptions, szCWS [gameOpts->render.cockpit.nWindowSize], gameOpts->render.cockpit.nWindowSize, 0, 3, KEY_S, HTX_CPIT_WINSIZE);
		nCWSopt = nOptions++;
		sprintf (szCockpitWindowZoom, TXT_CW_ZOOM, gameOpts->render.cockpit.nWindowZoom + 1);
		m.AddSlider (nOptions, szCockpitWindowZoom, gameOpts->render.cockpit.nWindowZoom, 0, 3, KEY_Z, HTX_CPIT_WINZOOM);
		nCWZopt = nOptions++;
		m.AddText (nOptions, "", 0);
		nOptions++;
		m.AddText (nOptions, TXT_AUXWIN_POSITION, 0);
		nOptions++;
		m.AddRadio (nOptions, TXT_POS_BOTTOM, nPosition == 0, KEY_B, 10, HTX_AUXWIN_POSITION);
		optPosition = nOptions++;
		m.AddRadio (nOptions, TXT_POS_TOP, nPosition == 1, KEY_T, 10, HTX_AUXWIN_POSITION);
		nOptions++;
		m.AddText (nOptions, "", 0);
		nOptions++;
		m.AddText (nOptions, TXT_AUXWIN_ALIGNMENT, 0);
		nOptions++;
		m.AddRadio (nOptions, TXT_ALIGN_CORNERS, nAlignment == 0, KEY_O, 11, HTX_AUXWIN_ALIGNMENT);
		optAlignment = nOptions++;
		m.AddRadio (nOptions, TXT_ALIGN_MIDDLE, nAlignment == 1, KEY_I, 11, HTX_AUXWIN_ALIGNMENT);
		nOptions++;
		m.AddRadio (nOptions, TXT_ALIGN_CENTER, nAlignment == 2, KEY_E, 11, HTX_AUXWIN_ALIGNMENT);
		nOptions++;
		m.AddText (nOptions, "", 0);
		nOptions++;
		m.AddCheck (nOptions, TXT_SHOW_HUD, gameOpts->render.cockpit.bHUD, KEY_U, HTX_CPIT_SHOWHUD);
		optHUD = nOptions++;
		m.AddCheck (nOptions, TXT_SHOW_HUDMSGS, gameOpts->render.cockpit.bHUDMsgs, KEY_M, HTX_CPIT_SHOWHUDMSGS);
		optHUDMsgs = nOptions++;
		m.AddCheck (nOptions, TXT_SHOW_RETICLE, gameOpts->render.cockpit.bReticle, KEY_R, HTX_CPIT_SHOWRETICLE);
		optReticle = nOptions++;
		if (gameOpts->input.mouse.bJoystick) {
			m.AddCheck (nOptions, TXT_SHOW_MOUSEIND, gameOpts->render.cockpit.bMouseIndicator, KEY_O, HTX_CPIT_MOUSEIND);
			optMouseInd = nOptions++;
			}
		else
			optMouseInd = -1;
		}
	else
		optHUD =
		optHUDMsgs =
		optMouseInd = 
		optReticle = -1;
	m.AddCheck (nOptions, TXT_EXTRA_PLRMSGS, gameOpts->render.cockpit.bSplitHUDMsgs, KEY_P, HTX_CPIT_SPLITMSGS);
	optSplitMsgs = nOptions++;
	m.AddCheck (nOptions, TXT_MISSILE_VIEW, gameOpts->render.cockpit.bMissileView, KEY_I, HTX_CPITMSLVIEW);
	optMissileView = nOptions++;
	m.AddCheck (nOptions, TXT_GUIDED_MAINVIEW, gameOpts->render.cockpit.bGuidedInMainView, KEY_F, HTX_CPIT_GUIDEDVIEW);
	optGuided = nOptions++;
	m.AddText (nOptions, "", 0);
	nOptions++;
	m.AddMenu (nOptions, TXT_TGTIND_MENUCALL, KEY_T, "");
	optTgtInd = nOptions++;
	m.AddMenu (nOptions, TXT_WPNICON_MENUCALL, KEY_W, "");
	optWeaponIcons = nOptions++;
	m.AddMenu (nOptions, TXT_GAUGES_MENUCALL, KEY_G, "");
	optGauges = nOptions++;
	Assert (sizeofa (m) >= (size_t) nOptions);
	do {
		i = ExecMenu1 (NULL, TXT_COCKPIT_OPTS, nOptions, m, &CockpitOptionsCallback, &choice);
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
	GET_VAL (gameOpts->render.cockpit.bHUDMsgs, optHUDMsgs);
	GET_VAL (gameOpts->render.cockpit.bSplitHUDMsgs, optSplitMsgs);
	if ((optAlignment >= 0) && (optPosition >= 0)) {
		for (nPosition = 0; nPosition < 2; nPosition++)
			if (m [optPosition + nPosition].value)
				break;
		for (nAlignment = 0; nAlignment < 3; nAlignment++)
			if (m [optAlignment + nAlignment].value)
				break;
		gameOpts->render.cockpit.nWindowPos = nPosition * 3 + nAlignment;
		}
	} while (i == -2);
}

//------------------------------------------------------------------------------
//eof
