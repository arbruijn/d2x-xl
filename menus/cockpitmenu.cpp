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
//	int32_t	nPos;
//	int32_t	nSize;
//	int32_t	nAlign;
//	int32_t	nZoom;
//	int32_t	nType [2];
//} tWindowOpts;
//
//typedef struct tRadarOpts {
//	int32_t	nPos;
//	int32_t	nSize;
//	int32_t	nRange;
//	int32_t	nColor;
//	int32_t	nStyle;
//} tRadarOpts;
//
//static struct {
//	tWindowOpts	windows;
//	tRadarOpts	radar;
//	int32_t			nTgtInd;
//} cockpitOpts;

static int32_t nWindowPos, nWindowAlign, nTgtInd;

#if WEAPON_ICONS
static int32_t	optWeaponIcons, bShowWeaponIcons, optIconAlpha;
#endif

static const char* szHUDType [3];
static const char* szHUDWidth [3];
static const char* szHUDHeight [3];
static const char* szLayout [3];
static const char* szColorScheme [3];
static const char* szWindowType [7];
static const char* szWindowSize [4];
static const char* szWindowPos [2];
static const char* szWindowAlign [3];
static const char* szTgtInd [3];
static const char* szRadarSize [3];
static const char* szRadarRange [5];
static const char* szRadarColor [3];
static const char* szRadarStyle [2];

static int32_t nWinFuncs, winFunc [2], winFuncList [CV_FUNC_COUNT], winFuncMap [CV_FUNC_COUNT];

//------------------------------------------------------------------------------

static const char* windowTypeIds [2] = {"left window type", "right window type"};

int32_t CockpitOptionsCallback (CMenu& menu, int32_t& key, int32_t nCurItem, int32_t nState)
{
if (nState)
	return nCurItem;

	CMenuItem*	m;
	int32_t		v;

#if WEAPON_ICONS
mat = menu + optWeaponIcons;
dir = mat->Value ();
if (dir != bShowWeaponIcons) {
	bShowWeaponIcons = dir;
	key = -2;
	return nCurItem;
	}
#endif

if ((m = menu ["show hud"])) {
	v = m->Value ();
	if (gameOpts->render.cockpit.bHUD != v) {
		gameOpts->render.cockpit.bHUD = v;
		sprintf (m->m_text, TXT_SHOW_HUD, szHUDType [v]);
		m->Rebuild ();
		}
	}

for (int32_t i = 0; i < 2; i++) {
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
		key = -2;
		}
	}

if ((m = menu ["compact hud"])) {
	v = m->Value ();
	if (gameOpts->render.cockpit.nShipStateLayout != v) {
		if (!gameOpts->render.cockpit.nShipStateLayout)
			nCurItem = -nCurItem;
		gameOpts->render.cockpit.nShipStateLayout = v;
		m->Rebuild ();
		key = -2;
		}
	}

if ((m = menu ["color scheme"])) {
	v = m->Value ();
	if (gameOpts->render.cockpit.nColorScheme != v) {
		gameOpts->render.cockpit.nColorScheme = v;
		sprintf (m->m_text, TXT_HUD_COLOR_SCHEME, szColorScheme [v]);
		m->Rebuild ();
		}
	}

if ((m = menu ["hud width"])) {
	v = m->Value ();
	if (gameOpts->render.cockpit.nCompactWidth != v) {
		gameOpts->render.cockpit.nCompactWidth = v;
		sprintf (m->m_text, TXT_COMPACT_HUD_WIDTH, szHUDWidth [v]);
		m->Rebuild ();
		}
	}

if ((m = menu ["hud height"])) {
	v = m->Value ();
	if (gameOpts->render.cockpit.nCompactHeight != v) {
		gameOpts->render.cockpit.nCompactHeight = v;
		sprintf (m->m_text, TXT_COMPACT_HUD_HEIGHT, szHUDHeight [v]);
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

szHUDType [0] = TXT_OFF;
szHUDType [1] = TXT_MINIMAL;
szHUDType [2] = TXT_FULL;

szLayout [0] = TXT_OFF;
szLayout [1] = TXT_HORIZONTAL;
szLayout [2] = TXT_VERTICAL;

szHUDWidth [0] = TXT_NARROW;
szHUDWidth [1] = TXT_MEDIUM;
szHUDWidth [2] = TXT_WIDE;

szHUDHeight [0] = TXT_COMPACT_HUD_HIGH;
szHUDHeight [1] = TXT_COMPACT_HUD_MIDDLE;
szHUDHeight [2] = TXT_COMPACT_HUD_LOW;

szColorScheme [0] = TXT_RED;
szColorScheme [1] = TXT_YELLOW;
szColorScheme [2] = TXT_BLUE;

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

int32_t GatherWindowFunctions (int32_t winFuncList [CV_FUNC_COUNT], int32_t winFuncMap [CV_FUNC_COUNT])
{
	int32_t	i = 0;

winFuncList [i++] = CV_NONE;
if (FindEscort())
	winFuncList [i++] = CV_ESCORT;
winFuncList [i++] = CV_REAR;
if (IsCoopGame || IsTeamGame) 
	winFuncList [i++] = CV_COOP;
if (!IsMultiGame || IsCoopGame || netGameInfo.m_info.bAllowMarkerView)
	winFuncList [i++] = CV_MARKER;
if (!(gameStates.app.bNostalgia || COMPETITION) && EGI_FLAG (bRadarEnabled, 0, 1, 0) &&
	 (!IsMultiGame || (netGameInfo.m_info.gameFlags & NETGAME_FLAG_SHOW_MAP))) {
	winFuncList [i++] = CV_RADAR_TOPDOWN;
	winFuncList [i++] = CV_RADAR_HEADSUP;
	}
memset (winFuncMap, 0xFF, sizeofa (winFuncMap));
for (int32_t j = 0; j < i; j++)
	winFuncMap [winFuncList [j]] = j;
return i;
}

//------------------------------------------------------------------------------

void DefaultCockpitSettings (bool bSetup = false);

void CockpitOptionsMenu (void)
{
	static int32_t choice = 0;

	CMenu m;
	int32_t	i;

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

	sprintf (szSlider, TXT_SHOW_HUD, szHUDType [gameOpts->render.cockpit.bHUD]);
	m.AddSlider ("show hud", szSlider, gameOpts->render.cockpit.bHUD, 0, 2, KEY_U, HTX_CPIT_SHOWHUD);
	m.AddText ("", "");

	m.AddCheck ("show reticle", TXT_SHOW_RETICLE, gameOpts->render.cockpit.bReticle, KEY_S, HTX_CPIT_SHOWRETICLE);
	m.AddCheck ("missile view", TXT_MISSILE_VIEW, gameOpts->render.cockpit.bMissileView, KEY_V, HTX_CPIT_MSLVIEW);
#if 0
	m.AddCheck ("compact hud", TXT_COMPACT_HUD, gameOpts->render.cockpit.nShipStateLayout, KEY_G, HTX_CPIT_GFXGAUGES);
#endif
	if (!gameOpts->render.cockpit.nShipStateLayout)
		m.AddCheck ("text gauges", TXT_SHOW_GFXGAUGES, !gameOpts->render.cockpit.bTextGauges, KEY_G, HTX_COMPACT_HUD);
	m.AddCheck ("object tally", TXT_OBJECT_TALLY, gameOpts->render.cockpit.bObjectTally, KEY_Y, HTX_CPIT_OBJTALLY);
	m.AddCheck ("zoom style", TXT_ZOOM_SMOOTH, extraGameInfo [IsMultiGame].nZoomMode - 1, KEY_O, HTX_GPLAY_ZOOMSMOOTH);
	if (gameOpts->app.bExpertMode)
		m.AddCheck ("sort weapon icons", TXT_SORT_WPNICONS, gameOpts->render.weaponIcons.nSort	, KEY_S, HTX_CPIT_SORTICONS);
#if 0
	m.AddCheck ("target indicators", TXT_TARGET_INDICATORS, extraGameInfo [0].bTargetIndicators, KEY_T, HTX_CPIT_TGTIND);
#else
	m.AddText ("", "");
#if 1
	sprintf (szSlider, TXT_SHIP_STATE_LAYOUT, szLayout [gameOpts->render.cockpit.nShipStateLayout]);
	m.AddSlider ("compact hud", szSlider, gameOpts->render.cockpit.nShipStateLayout, 0, 2, KEY_M, HTX_SHIP_STATE_LAYOUT);
	if (gameOpts->render.cockpit.nShipStateLayout) {
		sprintf (szSlider, TXT_HUD_COLOR_SCHEME, szColorScheme [gameOpts->render.cockpit.nColorScheme]);
		m.AddSlider ("color scheme", szSlider, gameOpts->render.cockpit.nColorScheme, 0, 2, KEY_C, HTX_HUD_COLOR_SCHEME);
		if (gameOpts->render.cockpit.nShipStateLayout == 2) {
			if (gameOpts->app.bExpertMode) {
				sprintf (szSlider, TXT_COMPACT_HUD_WIDTH, szHUDWidth [gameOpts->render.cockpit.nCompactWidth]);
				m.AddSlider ("hud width", szSlider, gameOpts->render.cockpit.nCompactWidth, 0, 2, KEY_W, HTX_COMPACT_HUD_WIDTH);
				sprintf (szSlider, TXT_COMPACT_HUD_HEIGHT, szHUDHeight [gameOpts->render.cockpit.nCompactHeight]);
				m.AddSlider ("hud height", szSlider, gameOpts->render.cockpit.nCompactHeight, 0, 2, KEY_H, HTX_COMPACT_HUD_HEIGHT);
				m.AddCheck ("hud separators", TXT_HUD_SEPARATORS, gameOpts->render.cockpit.bSeparators, KEY_P, HTX_HUD_SEPARATORS);
				}
			}
		m.AddText ("", "");
		}
#endif
	sprintf (szSlider, TXT_TARGET_INDICATORS, szTgtInd [nTgtInd]);
	m.AddSlider ("target indicators", szSlider, nTgtInd, 0, 2, KEY_I, HTX_CPIT_TGTIND);
	if (nTgtInd)
		m.AddCheck ("hide target indicators", TXT_HIDE_TGTIND, extraGameInfo [0].bHideIndicators, KEY_B, HTX_HIDE_TGTIND);
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
			for (int32_t j = 0; j < 4; j++)
				if (mat [optIconPos + j].Value ()) {
					extraGameInfo [0].nWeaponIcons = j + 1;
					break;
					}
				}
#endif
	} while (i >= 0);

	for (int32_t j = 0; j < 2; j++)
		gameStates.render.cockpit.n3DView [i] = winFuncList [winFunc [j]];
	GET_VAL (gameOpts->render.cockpit.bReticle, "show reticle");
	GET_VAL (gameOpts->render.cockpit.bHUD, "show hud");
	GET_VAL (gameOpts->render.cockpit.bMissileView, "missile view");
	GET_VAL (gameOpts->render.cockpit.bObjectTally, "object tally");
	if (gameOpts->app.bExpertMode)
		GET_VAL (gameOpts->render.weaponIcons.nSort, "sort weapon icons");
	GET_VAL (extraGameInfo [0].bHideIndicators, "hide target indicators");
	GET_VAL (gameOpts->render.cockpit.nShipStateLayout, "compact hud");
	GET_VAL (gameOpts->render.cockpit.bSeparators, "hud separators");
	if (gameOpts->render.cockpit.nShipStateLayout)
		GET_VAL (gameOpts->render.cockpit.nColorScheme, "color scheme");
	//GET_VAL (extraGameInfo [0].bTargetIndicators, "target indicators");
	if (m.Available ( "text gauges"))
		gameOpts->render.cockpit.bTextGauges = !m.Value ("text gauges");
	gameOpts->render.cockpit.nWindowPos = nWindowPos * 3 + nWindowAlign;
//if (gameOpts->app.bExpertMode)
	extraGameInfo [IsMultiGame].nZoomMode = m.Value ("zoom style") + 1;
#if WEAPON_ICONS
	if (bShowWeaponIcons) {
		int32_t h = m.IndexOf ("weapon icons top");
		if (h >= 0) {
			for (int32_t j = 0; j < 4; j++)
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
