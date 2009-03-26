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

#define WEAPON_ICONS 0

static int	optWindowSize, optWindowZoom, optWindowPos, optWindowAlign,	optTgtInd;
#if WEAPON_ICONS
static int	optWeaponIcons, bShowWeaponIcons, optIconAlpha;
#endif

static int nWindowPos, nWindowAlign, nTgtInd;

static const char *szWindowSize [4];
static const char *szWindowPos [2];
static const char *szWindowAlign [3];
static const char *szTgtInd [3];

//------------------------------------------------------------------------------

int CockpitOptionsCallback (CMenu& menu, int& key, int nCurItem, int nState)
{
if (nState)
	return nCurItem;

	CMenuItem*	m;
	int			v;

#if WEAPON_ICONS
m = menu + optWeaponIcons;
v = m->m_value;
if (v != bShowWeaponIcons) {
	bShowWeaponIcons = v;
	key = -2;
	return nCurItem;
	}
#endif

m = menu + optWindowSize;
v = m->m_value;
if (gameOpts->render.cockpit.nWindowSize != v) {
	gameOpts->render.cockpit.nWindowSize = v;
	m->SetText (szWindowSize [v]);
	sprintf (m->m_text, TXT_AUXWIN_SIZE, szWindowSize [v]);
	m->m_bRebuild = 1;
	}

m = menu + optWindowZoom;
v = m->m_value;
if (gameOpts->render.cockpit.nWindowZoom != v) {
	gameOpts->render.cockpit.nWindowZoom = v;
	sprintf (m->m_text, TXT_AUXWIN_ZOOM, gameOpts->render.cockpit.nWindowZoom + 1);
	m->m_bRebuild = 1;
	}

m = menu + optWindowPos;
v = m->m_value;
if (nWindowPos != v) {
	nWindowPos = v;
	sprintf (m->m_text, TXT_AUXWIN_POSITION, szWindowPos [v]);
	m->m_bRebuild = 1;
	}

m = menu + optWindowAlign;
v = m->m_value;
if (nWindowAlign != v) {
	nWindowAlign = v;
	sprintf (m->m_text, TXT_AUXWIN_ALIGNMENT, szWindowAlign [v]);
	m->m_bRebuild = 1;
	}

m = menu + optTgtInd;
v = m->m_value;
if (nTgtInd != v) {
	nTgtInd = v;
	sprintf (m->m_text, TXT_TARGET_INDICATORS, szTgtInd [v]);
	m->m_bRebuild = 1;
	}

return nCurItem;
}

//------------------------------------------------------------------------------

void DefaultCockpitSettings (void);

void CockpitOptionsMenu (void)
{
	static int choice = 0;

	CMenu m;
	int	i;
	int	optTextGauges, optHUD, optMissiles, optPosition, optAlignment;

	char	szSlider [50];

szWindowSize [0] = TXT_AUXWIN_SMALL;
szWindowSize [1] = TXT_AUXWIN_MEDIUM;
szWindowSize [2] = TXT_AUXWIN_LARGE;
szWindowSize [3] = TXT_AUXWIN_HUGE;

szWindowPos [0] = TXT_POS_BOTTOM;
szWindowPos [1] = TXT_POS_TOP;

szWindowAlign [0] = TXT_ALIGN_CORNERS;
szWindowAlign [1] = TXT_ALIGN_MIDDLE;
szWindowAlign [2] = TXT_ALIGN_CENTER;

szTgtInd [0] = TXT_NONE;
szTgtInd [1] = TXT_MISSILES;
szTgtInd [2] = TXT_FULL;

nWindowPos = gameOpts->render.cockpit.nWindowPos / 3;
nWindowAlign = gameOpts->render.cockpit.nWindowPos % 3;

optPosition = optAlignment = optWindowSize = optWindowZoom = optTextGauges = -1;
#if WEAPON_ICONS
int optIconPos = -1;
optWeaponIcons = -1;
bShowWeaponIcons = (extraGameInfo [0].nWeaponIcons != 0);
#endif

nTgtInd = extraGameInfo [0].bMslLockIndicators ? extraGameInfo [0].bTargetIndicators ? 2 : 1 : 0;

do {
	m.Destroy ();
	m.Create (15);

	optHUD = m.AddCheck (TXT_SHOW_HUD, gameOpts->render.cockpit.bHUD, KEY_U, HTX_CPIT_SHOWHUD);
	optMissiles = m.AddCheck (TXT_MISSILE_VIEW, gameOpts->render.cockpit.bMissileView, KEY_M, HTX_CPIT_MSLVIEW);
	optTextGauges = m.AddCheck (TXT_SHOW_GFXGAUGES, !gameOpts->render.cockpit.bTextGauges, KEY_G, HTX_CPIT_GFXGAUGES);
#if 0
	optTgtInd = m.AddCheck (TXT_TARGET_INDICATORS, extraGameInfo [0].bTargetIndicators, KEY_T, HTX_CPIT_TGTIND);
#else
	m.AddText ("", 0);
	sprintf (szSlider, TXT_TARGET_INDICATORS, szTgtInd [nTgtInd]);
	optTgtInd = m.AddSlider (szSlider, nTgtInd, 0, 2, KEY_T, HTX_CPIT_TGTIND);
#endif
#if WEAPON_ICONS
	m.AddText ("", 0);
	optWeaponIcons = m.AddCheck (TXT_SHOW_WEAPONICONS, bShowWeaponIcons, KEY_W, HTX_CPIT_WPNICONS);
	if (bShowWeaponIcons) {
		optIconPos = m.AddRadio (TXT_WPNICONS_TOP, 0, KEY_I, HTX_CPIT_ICONPOS);
		m.AddRadio (TXT_WPNICONS_BTM, 0, KEY_I, HTX_CPIT_ICONPOS);
		m.AddRadio (TXT_WPNICONS_LRB, 0, KEY_I, HTX_CPIT_ICONPOS);
		m.AddRadio (TXT_WPNICONS_LRT, 0, KEY_I, HTX_CPIT_ICONPOS);
		m [optIconPos + NMCLAMP (extraGameInfo [0].nWeaponIcons - 1, 0, 3)].m_value = 1;
		}
	else
		optIconPos = -1;
#endif
	m.AddText ("", 0);
	m.AddText (TXT_COCKPIT_WINDOWS, 0);
	sprintf (szSlider, TXT_AUXWIN_SIZE, szWindowSize [gameOpts->render.cockpit.nWindowSize]);
	optWindowSize = m.AddSlider (szSlider, gameOpts->render.cockpit.nWindowSize, 0, 3, KEY_S, HTX_CPIT_WINSIZE);

	sprintf (szSlider, TXT_AUXWIN_ZOOM, gameOpts->render.cockpit.nWindowZoom + 1);
	optWindowZoom = m.AddSlider (szSlider, gameOpts->render.cockpit.nWindowZoom, 0, 3, KEY_Z, HTX_CPIT_WINZOOM);

	sprintf (szSlider, TXT_AUXWIN_POSITION, szWindowPos [nWindowPos]);
	optWindowPos = m.AddSlider (szSlider, nWindowPos, 0, 1, KEY_P, HTX_AUXWIN_POSITION);

	sprintf (szSlider, TXT_AUXWIN_ALIGNMENT, szWindowAlign [nWindowAlign]);
	optWindowAlign = m.AddSlider (szSlider, nWindowAlign, 0, 2, KEY_A, HTX_AUXWIN_ALIGNMENT);
	m.AddText ("", 0);

	do {
		i = m.Menu (NULL, TXT_COCKPIT_OPTS, &CockpitOptionsCallback, &choice);
		if (i < 0)
			break;
#if WEAPON_ICONS
		if (bShowWeaponIcons && (optIconPos >= 0)) {
			for (int j = 0; j < 4; j++)
				if (m [optIconPos + j].m_value) {
					extraGameInfo [0].nWeaponIcons = j + 1;
					break;
					}
				}
#endif
	} while (i >= 0);

	GET_VAL (gameOpts->render.cockpit.bHUD, optHUD);
	GET_VAL (gameOpts->render.cockpit.bMissileView, optMissiles);
	//GET_VAL (extraGameInfo [0].bTargetIndicators, optTgtInd);
	gameOpts->render.cockpit.bTextGauges = !m [optTextGauges].m_value;
	gameOpts->render.cockpit.nWindowPos = nWindowPos * 3 + nWindowAlign;
#if WEAPON_ICONS
	if (bShowWeaponIcons) {
		if (optIconPos >= 0) {
			for (int j = 0; j < 4; j++)
				if (m [optIconPos + j].m_value) {
					extraGameInfo [0].nWeaponIcons = j + 1;
					break;
					}
				}
		GET_VAL (gameOpts->render.weaponIcons.alpha, optIconAlpha);
		}
#endif
	} while (i == -2);

extraGameInfo [0].bMslLockIndicators = (nTgtInd > 0);
extraGameInfo [0].bTargetIndicators = (nTgtInd > 1);

DefaultCockpitSettings ();
}

//------------------------------------------------------------------------------
//eof
