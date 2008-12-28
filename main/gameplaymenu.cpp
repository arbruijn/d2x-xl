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

#if DBG

const char *menuBgNames [4][2] = {
 {"menu.pcx", "menub.pcx"},
 {"menuo.pcx", "menuob.pcx"},
 {"menud.pcx", "menud.pcx"},
 {"menub.pcx", "menub.pcx"}
	};

#else

const char *menuBgNames [4][2] = {
 {"\x01menu.pcx", "\x01menub.pcx"},
 {"\x01menuo.pcx", "\x01menuob.pcx"},
 {"\x01menud.pcx", "\x01menud.pcx"},
 {"\x01menub.pcx", "\x01menub.pcx"}
	};
#endif

//------------------------------------------------------------------------------

static struct {
	int	nDifficulty;
	int	nSpawnDelay;
	int	nSmokeGrens;
	int	nMaxSmokeGrens;
	int	nAIAggressivity;
	int	nHeadlightAvailable;
} gplayOpts;

//------------------------------------------------------------------------------

static const char *pszGuns [] = {"Laser", "Vulcan", "Spreadfire", "Plasma", "Fusion", "Super Laser", "Gauss", "Helix", "Phoenix", "Omega"};
static const char *pszDevices [] = {"Full Map", "Ammo Rack", "Converter", "Quad Lasers", "Afterburner", "Headlight", "Slow Motion", "Bullet Time"};
static int nDeviceFlags [] = {PLAYER_FLAGS_FULLMAP, PLAYER_FLAGS_AMMO_RACK, PLAYER_FLAGS_CONVERTER, PLAYER_FLAGS_QUAD_LASERS, 
										PLAYER_FLAGS_AFTERBURNER, PLAYER_FLAGS_HEADLIGHT, PLAYER_FLAGS_SLOWMOTION, PLAYER_FLAGS_BULLETTIME};

static int optGuns, optDevices;

//------------------------------------------------------------------------------

static inline void SetGunLoadoutFlag (int i, int v)
{
if (v)
	extraGameInfo [0].loadout.nGuns |= (1 << i);
else
	extraGameInfo [0].loadout.nGuns &= ~(1 << i);
}

//------------------------------------------------------------------------------

static inline int GetGunLoadoutFlag (int i)
{
return (extraGameInfo [0].loadout.nGuns & (1 << i)) != 0;
}

//------------------------------------------------------------------------------

static inline void SetDeviceLoadoutFlag (int i, int v)
{
if (v)
	extraGameInfo [0].loadout.nDevices |= nDeviceFlags [i];
else
	extraGameInfo [0].loadout.nDevices &= ~nDeviceFlags [i];
}

//------------------------------------------------------------------------------

static inline int GetDeviceLoadoutFlag (int i)
{
return (extraGameInfo [0].loadout.nDevices & nDeviceFlags [i]) != 0;
}

//------------------------------------------------------------------------------

int LoadoutCallback (int nitems, CMenuItem * menus, int * key, int nCurItem)
{
	CMenuItem	*m = menus + nCurItem;
	int			v = m->value;

if (nCurItem == optGuns) {	//checked/unchecked lasers
	if (v != GetGunLoadoutFlag (0)) {
		SetGunLoadoutFlag (0, v);
		if (!v) {	//if lasers unchecked, also uncheck super lasers
			SetGunLoadoutFlag (5, 0);
			menus [optGuns + 5].value = 0;
			}
		}
	}
else if (nCurItem == optGuns + 5) {	//checked/unchecked super lasers
	if (v != GetGunLoadoutFlag (5)) {
		SetGunLoadoutFlag (5, v);
		if (v) {	// if super lasers checked, also check lasers
			SetGunLoadoutFlag (0, 1);
			menus [optGuns].value = 1;
			}
		}
	}
else if (nCurItem == optDevices + 6) {	//checked/unchecked super lasers
	if (v != GetDeviceLoadoutFlag (6)) {
		SetDeviceLoadoutFlag (6, v);
		if (!v) {	// if super lasers checked, also check lasers
			SetDeviceLoadoutFlag (7, 0);
			menus [optDevices + 7].value = 0;
			}
		}
	}
else if (nCurItem == optDevices + 7) {	//checked/unchecked super lasers
	if (v != GetDeviceLoadoutFlag (7)) {
		SetDeviceLoadoutFlag (7, v);
		if (v) {	// if super lasers checked, also check lasers
			SetDeviceLoadoutFlag (6, 1);
			menus [optDevices + 6].value = 1;
			}
		}
	}
return nCurItem;
}

//------------------------------------------------------------------------------

void LoadoutOptions (void)
{
	CMenuItem	m [25];
	int			i, nOptions = 0;

memset (m, 0, sizeof (m));
m.AddText (nOptions, TXT_GUN_LOADOUT, 0);
nOptions++;
for (i = 0, optGuns = nOptions; i < (int) sizeofa (pszGuns); i++, nOptions++)
	m.AddCheck (nOptions, pszGuns [i], (extraGameInfo [0].loadout.nGuns & (1 << i)) != 0, 0, HTX_GUN_LOADOUT);
m.AddText (nOptions, "", 0);
nOptions++;
m.AddText (nOptions, TXT_DEVICE_LOADOUT, 0);
nOptions++;
for (i = 0, optDevices = nOptions; i < (int) sizeofa (pszDevices); i++, nOptions++)
	m.AddCheck (nOptions, pszDevices [i], (extraGameInfo [0].loadout.nDevices & (nDeviceFlags [i])) != 0, 0, HTX_DEVICE_LOADOUT);
Assert (sizeofa (m) >= (size_t) nOptions);
do {
	i = ExecMenu1 (NULL, TXT_LOADOUT_MENUTITLE, nOptions, m, LoadoutCallback, 0);
	} while (i != -1);
extraGameInfo [0].loadout.nGuns = 0;
for (i = 0; i < (int) sizeofa (pszGuns); i++) {
	if (m [optGuns + i].value)
		extraGameInfo [0].loadout.nGuns |= (1 << i);
	}
extraGameInfo [0].loadout.nDevices = 0;
for (i = 0; i < (int) sizeofa (pszDevices); i++) {
	if (m [optDevices + i].value)
		extraGameInfo [0].loadout.nDevices |= nDeviceFlags [i];
	}
AddPlayerLoadout ();
}

//------------------------------------------------------------------------------

static const char *pszMslTurnSpeeds [3];
static const char *pszMslStartSpeeds [4];
static const char *pszAggressivities [5];

int GameplayOptionsCallback (int nitems, CMenuItem * menus, int * key, int nCurItem)
{
	CMenuItem	*m;
	int			v;

m = menus + gplayOpts.nDifficulty;
v = m->value;
if (gameStates.app.nDifficultyLevel != v) {
	gameStates.app.nDifficultyLevel = v;
	if (!IsMultiGame) {
		gameStates.app.nDifficultyLevel = v;
		InitGateIntervals ();
		}
	sprintf (m->text, TXT_DIFFICULTY2, MENU_DIFFICULTY_TEXT (gameStates.app.nDifficultyLevel));
	m->rebuild = 1;
	}

m = menus + gplayOpts.nHeadlightAvailable;
v = m->value;
if (extraGameInfo [0].headlight.bAvailable != v) {
	extraGameInfo [0].headlight.bAvailable = v;
	*key = -2;
	return nCurItem;
	}

if (gameOpts->app.bExpertMode) {
	m = menus + gplayOpts.nSpawnDelay;
	v = (m->value - 1) * 5;
	if (extraGameInfo [0].nSpawnDelay != v * 1000) {
		extraGameInfo [0].nSpawnDelay = v * 1000;
		sprintf (m->text, TXT_RESPAWN_DELAY, (v < 0) ? -1 : v);
		m->rebuild = 1;
		}

	m = menus + gplayOpts.nSmokeGrens;
	v = m->value;
	if (extraGameInfo [0].bSmokeGrenades != v) {
		extraGameInfo [0].bSmokeGrenades = v;
		*key = -2;
		return nCurItem;
		}

	m = menus + gplayOpts.nAIAggressivity;
	v = m->value;
	if (gameOpts->gameplay.nAIAggressivity != v) {
		gameOpts->gameplay.nAIAggressivity = v;
		sprintf (m->text, TXT_AI_AGGRESSIVITY, pszAggressivities [v]);
		m->rebuild = 1;
		}

	if (gplayOpts.nMaxSmokeGrens >= 0) {
		m = menus + gplayOpts.nMaxSmokeGrens;
		v = m->value + 1;
		if (extraGameInfo [0].nMaxSmokeGrenades != v) {
			extraGameInfo [0].nMaxSmokeGrenades = v;
			sprintf (m->text, TXT_MAX_SMOKEGRENS, extraGameInfo [0].nMaxSmokeGrenades);
			m->rebuild = 1;
			}
		}
	}
return nCurItem;
}

//------------------------------------------------------------------------------

void GameplayOptionsMenu (void)
{
	CMenuItem m [40];
	int	i, j, nOptions = 0, choice = 0;
	int	optFixedSpawn = -1, optSnipeMode = -1, optAutoSel = -1, optInventory = -1, 
			optDualMiss = -1, optDropAll = -1, optImmortal = -1, optMultiBosses = -1, optTripleFusion = -1,
			optEnhancedShakers = -1, optSmartWeaponSwitch = -1, optWeaponDrop = -1, optIdleAnims = -1, 
			optAwareness = -1, optHeadlightBuiltIn = -1, optHeadlightPowerDrain = -1, optHeadlightOnWhenPickedUp = -1,
			optRotateMarkers = -1, optLoadout, optUseD1AI = -1, optNoThief = -1;
	char	szRespawnDelay [60];
	char	szDifficulty [50], szMaxSmokeGrens [50], szAggressivity [50];

pszAggressivities [0] = TXT_STANDARD;
pszAggressivities [1] = TXT_MODERATE;
pszAggressivities [2] = TXT_MEDIUM;
pszAggressivities [3] = TXT_HIGH;
pszAggressivities [4] = TXT_VERY_HIGH;

do {
	memset (&m, 0, sizeof (m));
	nOptions = 0;
	sprintf (szDifficulty + 1, TXT_DIFFICULTY2, MENU_DIFFICULTY_TEXT (gameStates.app.nDifficultyLevel));
	*szDifficulty = *(TXT_DIFFICULTY2 - 1);
	m.AddSlider (nOptions, szDifficulty + 1, gameStates.app.nDifficultyLevel, 0, 4, KEY_D, HTX_GPLAY_DIFFICULTY);
	gplayOpts.nDifficulty = nOptions++;
	if (gameOpts->app.bExpertMode) {
		sprintf (szRespawnDelay + 1, TXT_RESPAWN_DELAY, (extraGameInfo [0].nSpawnDelay < 0) ? -1 : extraGameInfo [0].nSpawnDelay / 1000);
		*szRespawnDelay = *(TXT_RESPAWN_DELAY - 1);
		m.AddSlider (nOptions, szRespawnDelay + 1, extraGameInfo [0].nSpawnDelay / 5000 + 1, 0, 13, KEY_R, HTX_GPLAY_SPAWNDELAY);
		gplayOpts.nSpawnDelay = nOptions++;
		m.AddText (nOptions, "", 0);
		nOptions++;
		m.AddCheck (nOptions, TXT_HEADLIGHT_AVAILABLE, extraGameInfo [0].headlight.bAvailable, KEY_A, HTX_HEADLIGHT_AVAILABLE);
		gplayOpts.nHeadlightAvailable = nOptions++;
		if (extraGameInfo [0].headlight.bAvailable) {
			m.AddCheck (nOptions, TXT_HEADLIGHT_ON, gameOpts->gameplay.bHeadlightOnWhenPickedUp, KEY_O, HTX_MISC_HEADLIGHT);
			optHeadlightOnWhenPickedUp = nOptions++;
			m.AddCheck (nOptions, TXT_HEADLIGHT_BUILTIN, extraGameInfo [0].headlight.bBuiltIn, KEY_L, HTX_HEADLIGHT_BUILTIN);
			optHeadlightBuiltIn = nOptions++;
			m.AddCheck (nOptions, TXT_HEADLIGHT_POWERDRAIN, extraGameInfo [0].headlight.bDrainPower, KEY_W, HTX_HEADLIGHT_POWERDRAIN);
			optHeadlightPowerDrain = nOptions++;
			}
		m.AddCheck (nOptions, TXT_USE_INVENTORY, gameOpts->gameplay.bInventory, KEY_V, HTX_GPLAY_INVENTORY);
		optInventory = nOptions++;
		m.AddCheck (nOptions, TXT_ROTATE_MARKERS, extraGameInfo [0].bRotateMarkers, KEY_K, HTX_ROTATE_MARKERS);
		optRotateMarkers = nOptions++;
		m.AddText (nOptions, "", 0);
		nOptions++;
		m.AddCheck (nOptions, TXT_MULTI_BOSSES, extraGameInfo [0].bMultiBosses, KEY_M, HTX_GPLAY_MULTIBOSS);
		optMultiBosses = nOptions++;
		m.AddCheck (nOptions, TXT_IDLE_ANIMS, gameOpts->gameplay.bIdleAnims, KEY_D, HTX_GPLAY_IDLEANIMS);
		optIdleAnims = nOptions++;
		m.AddCheck (nOptions, TXT_SUPPRESS_THIEF, gameOpts->gameplay.bNoThief, KEY_T, HTX_SUPPRESS_THIEF);
		optNoThief = nOptions++;
		m.AddCheck (nOptions, TXT_AI_AWARENESS, gameOpts->gameplay.nAIAwareness, KEY_I, HTX_AI_AWARENESS);
		optAwareness = nOptions++;
		sprintf (szAggressivity + 1, TXT_AI_AGGRESSIVITY, pszAggressivities [gameOpts->gameplay.nAIAggressivity]);
		*szAggressivity = *(TXT_AI_AGGRESSIVITY - 1);
		m.AddSlider (nOptions, szAggressivity + 1, gameOpts->gameplay.nAIAggressivity, 0, 4, KEY_G, HTX_AI_AGGRESSIVITY);
		gplayOpts.nAIAggressivity = nOptions++;
		m.AddText (nOptions, "", 0);
		nOptions++;
		m.AddCheck (nOptions, TXT_ALWAYS_RESPAWN, extraGameInfo [0].bImmortalPowerups, KEY_P, HTX_GPLAY_ALWAYSRESP);
		optImmortal = nOptions++;
		m.AddCheck (nOptions, TXT_FIXED_SPAWN, extraGameInfo [0].bFixedRespawns, KEY_F, HTX_GPLAY_FIXEDSPAWN);
		optFixedSpawn = nOptions++;
		m.AddCheck (nOptions, TXT_DROP_ALL, extraGameInfo [0].bDropAllMissiles, KEY_A, HTX_GPLAY_DROPALL);
		optDropAll = nOptions++;
		m.AddCheck (nOptions, TXT_DROP_QUADSUPER, extraGameInfo [0].nWeaponDropMode, KEY_Q, HTX_GPLAY_DROPQUAD);
		optWeaponDrop = nOptions++;
		m.AddCheck (nOptions, TXT_DUAL_LAUNCH, extraGameInfo [0].bDualMissileLaunch, KEY_U, HTX_GPLAY_DUALLAUNCH);
		optDualMiss = nOptions++;
		m.AddCheck (nOptions, TXT_TRIPLE_FUSION, extraGameInfo [0].bTripleFusion, KEY_U, HTX_GPLAY_TRIFUSION);
		optTripleFusion = nOptions++;
		m.AddCheck (nOptions, TXT_ENHANCED_SHAKERS, extraGameInfo [0].bEnhancedShakers, KEY_B, HTX_ENHANCED_SHAKERS);
		optEnhancedShakers = nOptions++;
		m.AddCheck (nOptions, TXT_USE_D1_AI, gameOpts->gameplay.bUseD1AI, KEY_I, HTX_USE_D1_AI);
		optUseD1AI = nOptions++;
		if (extraGameInfo [0].bSmokeGrenades) {
			m.AddText (nOptions, "", 0);
			nOptions++;
			}
		m.AddCheck (nOptions, TXT_GPLAY_SMOKEGRENADES, extraGameInfo [0].bSmokeGrenades, KEY_S, HTX_GPLAY_SMOKEGRENADES);
		gplayOpts.nSmokeGrens = nOptions++;
		if (extraGameInfo [0].bSmokeGrenades) {
			sprintf (szMaxSmokeGrens + 1, TXT_MAX_SMOKEGRENS, extraGameInfo [0].nMaxSmokeGrenades);
			*szMaxSmokeGrens = *(TXT_MAX_SMOKEGRENS - 1);
			m.AddSlider (nOptions, szMaxSmokeGrens + 1, extraGameInfo [0].nMaxSmokeGrenades - 1, 0, 3, KEY_X, HTX_GPLAY_MAXGRENADES);
			gplayOpts.nMaxSmokeGrens = nOptions++;
			}
		else
			gplayOpts.nMaxSmokeGrens = -1;
		}
	m.AddText (nOptions, "", 0);
	nOptions++;
	m.AddCheck (nOptions, TXT_SMART_WPNSWITCH, extraGameInfo [0].bSmartWeaponSwitch, KEY_W, HTX_GPLAY_SMARTSWITCH);
	optSmartWeaponSwitch = nOptions++;
	optAutoSel = nOptions;
	m.AddRadio (nOptions, TXT_WPNSEL_NEVER, 0, KEY_N, 2, HTX_GPLAY_WSELNEVER);
	nOptions++;
	m.AddRadio (nOptions, TXT_WPNSEL_EMPTY, 0, KEY_Y, 2, HTX_GPLAY_WSELEMPTY);
	nOptions++;
	m.AddRadio (nOptions, TXT_WPNSEL_ALWAYS, 0, KEY_T, 2, HTX_GPLAY_WSELALWAYS);
	nOptions++;
	m.AddText (nOptions, "", 0);
	nOptions++;
	optSnipeMode = nOptions;
	m.AddRadio (nOptions, TXT_ZOOM_OFF, 0, KEY_D, 3, HTX_GPLAY_ZOOMOFF);
	nOptions++;
	m.AddRadio (nOptions, TXT_ZOOM_FIXED, 0, KEY_X, 3, HTX_GPLAY_ZOOMFIXED);
	nOptions++;
	m.AddRadio (nOptions, TXT_ZOOM_SMOOTH, 0, KEY_Z, 3, HTX_GPLAY_ZOOMSMOOTH);
	nOptions++;
	m [optAutoSel + NMCLAMP (gameOpts->gameplay.nAutoSelectWeapon, 0, 2)].value = 1;
	m [optSnipeMode + NMCLAMP (extraGameInfo [0].nZoomMode, 0, 2)].value = 1;
	m.AddText (nOptions, "", 0);
	nOptions++;
	m.AddMenu (nOptions, TXT_LOADOUT_OPTION, KEY_L, HTX_MULTI_LOADOUT);
	optLoadout = nOptions++;
	Assert (sizeofa (m) >= (size_t) nOptions);
	for (;;) {
		if (0 > (i = ExecMenu1 (NULL, TXT_GAMEPLAY_OPTS, nOptions, m, &GameplayOptionsCallback, &choice)))
			break;
		if (choice == optLoadout)
			LoadoutOptions ();
		};
	} while (i == -2);
if (gameOpts->app.bExpertMode) {
	extraGameInfo [0].headlight.bAvailable = m [gplayOpts.nHeadlightAvailable].value;
	extraGameInfo [0].bFixedRespawns = m [optFixedSpawn].value;
	extraGameInfo [0].bSmokeGrenades = m [gplayOpts.nSmokeGrens].value;
	extraGameInfo [0].bDualMissileLaunch = m [optDualMiss].value;
	extraGameInfo [0].bDropAllMissiles = m [optDropAll].value;
	extraGameInfo [0].bImmortalPowerups = m [optImmortal].value;
	extraGameInfo [0].bMultiBosses = m [optMultiBosses].value;
	extraGameInfo [0].bSmartWeaponSwitch = m [optSmartWeaponSwitch].value;
	extraGameInfo [0].bTripleFusion = m [optTripleFusion].value;
	extraGameInfo [0].bEnhancedShakers = m [optEnhancedShakers].value;
	extraGameInfo [0].bRotateMarkers = m [optRotateMarkers].value;
	extraGameInfo [0].nWeaponDropMode = m [optWeaponDrop].value;
	GET_VAL (gameOpts->gameplay.bInventory, optInventory);
	GET_VAL (gameOpts->gameplay.bIdleAnims, optIdleAnims);
	GET_VAL (gameOpts->gameplay.nAIAwareness, optAwareness);
	GET_VAL (gameOpts->gameplay.bUseD1AI, optUseD1AI);
	GET_VAL (gameOpts->gameplay.bNoThief, optNoThief);
	GET_VAL (gameOpts->gameplay.bHeadlightOnWhenPickedUp, optHeadlightOnWhenPickedUp);
	GET_VAL (extraGameInfo [0].headlight.bDrainPower, optHeadlightPowerDrain);
	GET_VAL (extraGameInfo [0].headlight.bBuiltIn, optHeadlightBuiltIn);
	}
else {
#if EXPMODE_DEFAULTS
	extraGameInfo [0].bFixedRespawns = 0;
	extraGameInfo [0].bDualMissileLaunch = 0;
	extraGameInfo [0].bDropAllMissiles = 0;
	extraGameInfo [0].bImmortalPowerups = 0;
	extraGameInfo [0].bMultiBosses = 1;
	extraGameInfo [0].bSmartWeaponSwitch = 0;
	extraGameInfo [0].nWeaponDropMode = 1;
	gameOpts->gameplay.bInventory = 0;
	gameOpts->gameplay.bIdleAnims = 0;
	gameOpts->gameplay.nAIAwareness = 0;
	gameOpts->gameplay.bHeadlightOnWhenPickedUp = 0;
#endif
	}
for (j = 0; j < 3; j++)
	if (m [optAutoSel + j].value) {
		gameOpts->gameplay.nAutoSelectWeapon = j;
		break;
		}
for (j = 0; j < 3; j++)
	if (m [optSnipeMode + j].value) {
		extraGameInfo [0].nZoomMode = j;
		break;
		}
if (!COMPETITION && EGI_FLAG (bSmokeGrenades, 0, 0, 0))
	LOCALPLAYER.secondaryAmmo [PROXMINE_INDEX] = 4;
if (IsMultiGame)
	NetworkSendExtraGameInfo (NULL);
}

//------------------------------------------------------------------------------

static int nOptDebrisLife;

//------------------------------------------------------------------------------
//eof
