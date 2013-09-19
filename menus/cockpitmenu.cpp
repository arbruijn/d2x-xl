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
#include "gamecntl.h"
#include "menu.h"
#include "network.h"
#include "network_lib.h"
#include "netmenu.h"
#include "scores.h"
#include "joydefs.h"
#include "playerprofile.h"
#include "kconfig.h"
#include "credits.h"
#include "texmap.h"
#include "savegame.h"
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

//typedef struct tWindowOpts {
//	int	nPos;
//	int	nSize;
//	int	nAlign;
//	int	nZoom;
//	int	nType [2];
//} tWindowOpts;
//
//typedef struct tRadarOpts {
//	int	nPos;
//	int	nSize;
//	int	nRange;
//	int	nColor;
//	int	nStyle;
//} tRadarOpts;
//
//static struct {
//	tWindowOpts	windows;
//	tRadarOpts	radar;
//	int			nTgtInd;
//} cockpitOpts;

static int nWindowPos, nWindowAlign, nTgtInd;

#if WEAPON_ICONS
static int	optWeaponIcons, bShowWeaponIcons, optIconAlpha;
#endif

static const char* szWindowType [7];
static const char* szWindowSize [4];
static const char* szWindowPos [2];
static const char* szWindowAlign [3];
static const char* szTgtInd [3];
static const char* szRadarSize [3];
static const char* szRadarRange [5];
static const char* szRadarColor [3];
static const char* szRadarStyle [2];

static int nWinFuncs, winFunc [2], winFuncList [CV_FUNC_COUNT], winFuncMap [CV_FUNC_COUNT];

//------------------------------------------------------------------------------

static char* windowTypeIds [2] = {"left window type", "right window type"};

int CockpitOptionsCallback (CMenu& menu, int& key, int nCurItem, int nState)
{
if (nState)
	return nCurItem;

	CMenuItem*	m;
	int			v;

#if WEAPON_ICONS
mat = menu + optWeaponIcons;
dir = mat->Value ();
if (dir != bShowWeaponIcons) {
	bShowWeaponIcons = dir;
	key = -2;
	return nCurItem;
	}
#endif

for (int i = 0; i < 2; i++) {
	if ((m = menu [windowTypeIds [i]])) {
		v = m->Value ();
		if (v != winFunc [i]) {
			winFunc [i] = v;
			gameStates.render.cockpit.n3DView [i] = winFuncList [v];
			sprintf (m->m_text, GT (1163 + i), szWindowType [v]);
			m->Rebuild ();
			}
		}
	}

if ((m = menu ["cockpit window size"])) {
	v = m->Value ();
	if (gameOpts->render.cockpit.nWindowSize != v) {
		gameOpts->render.cockpit.nWindowSize = v;
		m->SetText (szWindowSize [v]);
		sprintf (m->m_text, TXT_AUXWIN_SIZE, szWindowSize [v]);
		m->Rebuild ();
		}
	}

if ((m = menu ["cockpit window zoom"])) {
	v = m->Value ();
	if (gameOpts->render.cockpit.nWindowZoom != v) {
		gameOpts->render.cockpit.nWindowZoom = v;
		sprintf (m->m_text, TXT_AUXWIN_ZOOM, gameOpts->render.cockpit.nWindowZoom + 1);
		m->Rebuild ();
		}
	}

if ((m = menu ["cockpit window pos"])) {
	v = m->Value ();
	if (nWindowPos != v) {
		nWindowPos = v;
		sprintf (m->m_text, TXT_AUXWIN_POSITION, szWindowPos [v]);
		m->Rebuild ();
		}
	}

if ((m = menu ["cockpit window align"])) {
	v = m->Value ();
	if (nWindowAlign != v) {
		nWindowAlign = v;
		sprintf (m->m_text, TXT_AUXWIN_ALIGNMENT, szWindowAlign [v]);
		m->Rebuild ();
		}
	}

if ((m = menu ["cockpit radar pos"])) {
	v = m->Value ();
	if (gameOpts->render.cockpit.nRadarPos != v) {
		gameOpts->render.cockpit.nRadarPos = v;
		sprintf (m->m_text, TXT_RADAR_POSITION, szWindowPos [v]);
		m->Rebuild ();
		}
	}

if ((m = menu ["cockpit radar size"])) {
	v = m->Value ();
	if (gameOpts->render.cockpit.nRadarSize != v) {
		gameOpts->render.cockpit.nRadarSize = v;
		sprintf (m->m_text, TXT_RADAR_SIZE, szRadarSize [v]);
		m->Rebuild ();
		}
	}

if ((m = menu ["cockpit radar range"])) {
	v = m->Value ();
	if (gameOpts->render.cockpit.nRadarRange != v) {
		gameOpts->render.cockpit.nRadarRange = v;
		sprintf (m->m_text, TXT_RADAR_RANGE, szRadarRange [v]);
		m->Rebuild ();
		}
	}

if ((m = menu ["cockpit radar color"])) {
	v = m->Value ();
	if (gameOpts->render.cockpit.nRadarColor != v) {
		gameOpts->render.cockpit.nRadarColor = v;
		sprintf (m->m_text, TXT_RADAR_COLOR, szRadarColor [v]);
		m->Rebuild ();
		}
	}

if ((m = menu ["cockpit radar style"])) {
	v = m->Value ();
	if (gameOpts->render.cockpit.nRadarStyle != v) {
		gameOpts->render.cockpit.nRadarStyle = v;
		sprintf (m->m_text, TXT_RADAR_STYLE, szRadarStyle [v]);
		m->Rebuild ();
		}
	}

if ((m = menu ["target indicators"])) {
	v = m->Value ();
	if (nTgtInd != v) {
		nTgtInd = v;
		sprintf (m->m_text, TXT_TARGET_INDICATORS, szTgtInd [v]);
		m->Rebuild ();
		}
	}

return nCurItem;
}

//------------------------------------------------------------------------------

static void InitStrings (void)
{
	static bool bInitialized = false;

if (bInitialized)
	return;
bInitialized = true;

szWindowSize [0] = TXT_AUXWIN_SMALL;
szWindowSize [1] = TXT_AUXWIN_MEDIUM;
szWindowSize [2] = TXT_AUXWIN_LARGE;
szWindowSize [3] = TXT_AUXWIN_HUGE;

szWindowPos [0] = TXT_POS_BOTTOM;
szWindowPos [1] = TXT_POS_TOP;

szWindowAlign [0] = TXT_ALIGN_CORNERS;
szWindowAlign [1] = TXT_ALIGN_MIDDLE;
szWindowAlign [2] = TXT_ALIGN_CENTER;

szRadarSize [0] = TXT_SMALL;
szRadarSize [1] = TXT_MEDIUM;
szRadarSize [2] = TXT_LARGE;

szRadarRange [0] = TXT_OFF;
szRadarRange [1] = TXT_SHORT;
szRadarRange [2] = TXT_MEDIUM;
szRadarRange [3] = TXT_LONG;
szRadarRange [4] = TXT_EXTREME;

szRadarColor [0] = TXT_GREEN;
szRadarColor [1] = TXT_AMBER;
szRadarColor [2] = TXT_BLUE;

szRadarStyle [0] = TXT_MONOCHROME;
szRadarStyle [1] = TXT_MULTICOLOR;

szTgtInd [0] = TXT_NONE;
szTgtInd [1] = TXT_MISSILES;
szTgtInd [2] = TXT_FULL;
}

//------------------------------------------------------------------------------
// build a list of all available cockpit window functions

int GatherWindowFunctions (int winFuncList [CV_FUNC_COUNT], int winFuncMap [CV_FUNC_COUNT])
{
	int	i = 0;

winFuncList [i++] = CV_NONE;
if (FindEscort())
	winFuncList [i++] = CV_ESCORT;
winFuncList [i++] = CV_REAR;
if (IsCoopGame || IsTeamGame) 
	winFuncList [i++] = CV_COOP;
if (!IsMultiGame || IsCoopGame || netGame.m_info.bAllowMarkerView)
	winFuncList [i++] = CV_MARKER;
if (!(gameStates.app.bNostalgia || COMPETITION) && EGI_FLAG (bRadarEnabled, 0, 1, 0) &&
	 (!IsMultiGame || (netGame.m_info.gameFlags & NETGAME_FLAG_SHOW_MAP))) {
	winFuncList [i++] = CV_RADAR_TOPDOWN;
	winFuncList [i++] = CV_RADAR_HEADSUP;
	}
memset (winFuncMap, 0xFF, sizeofa (winFuncMap));
for (int j = 0; j < i; j++)
	winFuncMap [winFuncList [j]] = j;
return i;
}

//------------------------------------------------------------------------------

void DefaultCockpitSettings (bool bSetup = false);

void CockpitOptionsMenu (void)
{
	static int choice = 0;

	CMenu m;
	int	i;

	char	szSlider [50];

nWinFuncs = GatherWindowFunctions (winFuncList, winFuncMap);
for (i = 0; i < 2; i++)
	winFunc [i] = winFuncMap [gameStates.render.cockpit.n3DView [i]];
for (i = 0; i < nWinFuncs; i++)
	szWindowType [i] = GT (1156 + winFuncList [i]);

InitStrings ();

nWindowPos = gameOpts->render.cockpit.nWindowPos / 3;
nWindowAlign = gameOpts->render.cockpit.nWindowPos % 3;

#if WEAPON_ICONS
bShowWeaponIcons = (extraGameInfo [0].nWeaponIcons != 0);
#endif

nTgtInd = extraGameInfo [0].bMslLockIndicators ? extraGameInfo [0].bTargetIndicators ? 2 : 1 : 0;

do {
	m.Destroy ();
	m.Create (20);

	m.AddCheck ("show hud", TXT_SHOW_HUD, gameOpts->render.cockpit.bHUD, KEY_U, HTX_CPIT_SHOWHUD);
	sprintf (szSlider, TXT_SHOW_HUD, szWindowSize [gameOpts->render.cockpit.bHUD]);
	m.AddSlider ("show hud", szSlider, gameOpts->render.cockpit.nWindowSize, 0, 3, KEY_I, HTX_CPIT_WINSIZE);


	m.AddCheck ("show reticle", TXT_SHOW_RETICLE, gameOpts->render.cockpit.bReticle, KEY_S, HTX_CPIT_SHOWRETICLE);
	m.AddCheck ("missile view", TXT_MISSILE_VIEW, gameOpts->render.cockpit.bMissileView, KEY_M, HTX_CPIT_MSLVIEW);
	m.AddCheck ("text gauges", TXT_SHOW_GFXGAUGES, !gameOpts->render.cockpit.bTextGauges, KEY_G, HTX_CPIT_GFXGAUGES);
	m.AddCheck ("object tally", TXT_OBJECT_TALLY, gameOpts->render.cockpit.bObjectTally, KEY_T, HTX_CPIT_OBJTALLY);
	m.AddCheck ("zoom style", TXT_ZOOM_SMOOTH, extraGameInfo [IsMultiGame].nZoomMode - 1, KEY_O, HTX_GPLAY_ZOOMSMOOTH);
#if 0
	m.AddCheck ("target indicators", TXT_TARGET_INDICATORS, extraGameInfo [0].bTargetIndicators, KEY_T, HTX_CPIT_TGTIND);
#else
	m.AddText ("", "");
	sprintf (szSlider, TXT_TARGET_INDICATORS, szTgtInd [nTgtInd]);
	m.AddSlider ("target indicators", szSlider, nTgtInd, 0, 2, KEY_T, HTX_CPIT_TGTIND);
	if (nTgtInd)
		m.AddCheck ("hide target indicators", TXT_HIDE_TGTIND, extraGameInfo [0].bHideIndicators, KEY_H, HTX_HIDE_TGTIND);
#endif
#if WEAPON_ICONS
	m.AddText ("", "");
	mat.AddCheck (TXT_SHOW_WEAPONICONS, bShowWeaponIcons, KEY_W, HTX_CPIT_WPNICONS, "weapon icons");
	if (bShowWeaponIcons) {
		m.AddRadio ("icon pos top", TXT_WPNICONS_TOP, 0, KEY_I, HTX_CPIT_ICONPOS);
		m.AddRadio ("icon pos btm", TXT_WPNICONS_BTM, 0, KEY_I, HTX_CPIT_ICONPOS);
		m.AddRadio ("icon pos lrb", TXT_WPNICONS_LRB, 0, KEY_I, HTX_CPIT_ICONPOS);
		m.AddRadio ("icon pos lrt", TXT_WPNICONS_LRT, 0, KEY_I, HTX_CPIT_ICONPOS);
		m [m.IndexOf ("icon pos top") + NMCLAMP (extraGameInfo [0].nWeaponIcons - 1, 0, 3)].Value () = 1;
		}
#endif
	m.AddText ("", "");
	m [m.AddText ("", TXT_COCKPIT_WINDOWS)].m_bCentered = 1;
	m.AddText ("", "");

	for (i = 0; i < 2; i++) {
		sprintf (szSlider, GT (1163 + i), szWindowType [winFunc [i]]);
		m.AddSlider (windowTypeIds [i], szSlider, winFunc [i], 0, nWinFuncs - 1, i ? KEY_R : KEY_L, HTX_CPIT_WINTYPE);
		}

	sprintf (szSlider, TXT_AUXWIN_SIZE, szWindowSize [gameOpts->render.cockpit.nWindowSize]);
	m.AddSlider ("cockpit window size", szSlider, gameOpts->render.cockpit.nWindowSize, 0, 3, KEY_I, HTX_CPIT_WINSIZE);

	sprintf (szSlider, TXT_AUXWIN_ZOOM, gameOpts->render.cockpit.nWindowZoom + 1);
	m.AddSlider ("cockpit window zoom", szSlider, gameOpts->render.cockpit.nWindowZoom, 0, 3, KEY_Z, HTX_CPIT_WINZOOM);

	sprintf (szSlider, TXT_AUXWIN_POSITION, szWindowPos [nWindowPos]);
	m.AddSlider ("cockpit window pos", szSlider, nWindowPos, 0, 1, KEY_P, HTX_AUXWIN_POSITION);

	sprintf (szSlider, TXT_AUXWIN_ALIGNMENT, szWindowAlign [nWindowAlign]);
	m.AddSlider ("cockpit window align", szSlider, nWindowAlign, 0, 2, KEY_A, HTX_AUXWIN_ALIGNMENT);

	m.AddText ("", "");

	sprintf (szSlider, TXT_RADAR_POSITION, szWindowPos [gameOpts->render.cockpit.nRadarPos]);
	m.AddSlider ("cockpit radar pos", szSlider, gameOpts->render.cockpit.nRadarPos, 0, 1, KEY_O, HTX_CPIT_RADARPOS);

	sprintf (szSlider, TXT_RADAR_SIZE, szRadarSize [gameOpts->render.cockpit.nRadarSize]);
	m.AddSlider ("cockpit radar size", szSlider, gameOpts->render.cockpit.nRadarSize, 0, 2, KEY_O, HTX_CPIT_RADARSIZE);

	sprintf (szSlider, TXT_RADAR_RANGE, szRadarRange [gameOpts->render.cockpit.nRadarRange]);
	m.AddSlider ("cockpit radar range", szSlider, gameOpts->render.cockpit.nRadarRange, 0, 4, KEY_R, HTX_CPIT_RADARRANGE);

	sprintf (szSlider, TXT_RADAR_COLOR, szRadarColor [gameOpts->render.cockpit.nRadarColor]);
	m.AddSlider ("cockpit radar color", szSlider, gameOpts->render.cockpit.nRadarColor, 0, 2, KEY_C, HTX_CPIT_RADARCOLOR);

	sprintf (szSlider, TXT_RADAR_STYLE, szRadarStyle [gameOpts->render.cockpit.nRadarStyle]);
	m.AddSlider ("cockpit radar style", szSlider, gameOpts->render.cockpit.nRadarStyle, 0, 1, KEY_S, HTX_CPIT_RADARSTYLE);

	do {
		i = m.Menu (NULL, TXT_COCKPIT_OPTS, &CockpitOptionsCallback, &choice);
		if (i < 0)
			break;
#if WEAPON_ICONS
		if (bShowWeaponIcons && (optIconPos >= 0)) {
			for (int j = 0; j < 4; j++)
				if (mat [optIconPos + j].Value ()) {
					extraGameInfo [0].nWeaponIcons = j + 1;
					break;
					}
				}
#endif
	} while (i >= 0);

	for (i = 0; i < 2; i++)
		gameStates.render.cockpit.n3DView [i] = winFuncList [winFunc [i]];
	GET_VAL (gameOpts->render.cockpit.bReticle, "show reticle");
	GET_VAL (gameOpts->render.cockpit.bHUD, "show hud");
	GET_VAL (gameOpts->render.cockpit.bMissileView, "missile view");
	GET_VAL (gameOpts->render.cockpit.bObjectTally, "object tally");
	GET_VAL (extraGameInfo [0].bHideIndicators, "hide target indicators");
	//GET_VAL (extraGameInfo [0].bTargetIndicators, "target indicators");
	gameOpts->render.cockpit.bTextGauges = !m.Value ("text gauges");
	gameOpts->render.cockpit.nWindowPos = nWindowPos * 3 + nWindowAlign;
//if (gameOpts->app.bExpertMode)
	extraGameInfo [IsMultiGame].nZoomMode = m.Value ("zoom style") + 1;
#if WEAPON_ICONS
	if (bShowWeaponIcons) {
		int h = m.IndexOf ("weapon icons top");
		if (h >= 0) {
			for (int j = 0; j < 4; j++)
				if (mat [h + j].Value ()) {
					extraGameInfo [0].nWeaponIcons = j + 1;
					break;
					}
				}
		GET_VAL (gameOpts->render.weaponIcons.alpha, "icon alpha");
		}
#endif
	} while (i == -2);

extraGameInfo [0].bMslLockIndicators = (nTgtInd > 0);
extraGameInfo [0].bTargetIndicators = (nTgtInd > 1);

DefaultCockpitSettings ();
}

//------------------------------------------------------------------------------
//eof
