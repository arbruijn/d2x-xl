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

static const char *pszAggressivities [6];
static const char *pszWeaponSwitch [3];

void DefaultGameplaySettings (void);

//------------------------------------------------------------------------------

static struct {
	int	nDifficulty;
	int	nSpawnDelay;
	int	nSmokeGrens;
	int	nMaxSmokeGrens;
	int	nAIAggressivity;
	int	nHeadlightAvailable;
	int	nWeaponSwitch;
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
	extraGameInfo [0].loadout.nDevice |= nDeviceFlags [i];
else
	extraGameInfo [0].loadout.nDevice &= ~nDeviceFlags [i];
}

//------------------------------------------------------------------------------

static inline int GetDeviceLoadoutFlag (int i)
{
return (extraGameInfo [0].loadout.nDevice & nDeviceFlags [i]) != 0;
}

//------------------------------------------------------------------------------

int LoadoutCallback (CMenu& menu, int& key, int nCurItem, int nState)
{
if (nState)
	return nCurItem;

	CMenuItem	*m = menu + nCurItem;
	int			v = m->m_value;

if (nCurItem == optGuns) {	//checked/unchecked lasers
	if (v != GetGunLoadoutFlag (0)) {
		SetGunLoadoutFlag (0, v);
		if (!v) {	//if lasers unchecked, also uncheck super lasers
			SetGunLoadoutFlag (5, 0);
			menu [optGuns + 5].m_value = 0;
			}
		}
	}
else if (nCurItem == optGuns + 5) {	//checked/unchecked super lasers
	if (v != GetGunLoadoutFlag (5)) {
		SetGunLoadoutFlag (5, v);
		if (v) {	// if super lasers checked, also check lasers
			SetGunLoadoutFlag (0, 1);
			menu [optGuns].m_value = 1;
			}
		}
	}
else if (nCurItem == optDevices + 6) {	//checked/unchecked super lasers
	if (v != GetDeviceLoadoutFlag (6)) {
		SetDeviceLoadoutFlag (6, v);
		if (!v) {	// if super lasers checked, also check lasers
			SetDeviceLoadoutFlag (7, 0);
			menu [optDevices + 7].m_value = 0;
			}
		}
	}
else if (nCurItem == optDevices + 7) {	//checked/unchecked super lasers
	if (v != GetDeviceLoadoutFlag (7)) {
		SetDeviceLoadoutFlag (7, v);
		if (v) {	// if super lasers checked, also check lasers
			SetDeviceLoadoutFlag (6, 1);
			menu [optDevices + 6].m_value = 1;
			}
		}
	}
return nCurItem;
}

//------------------------------------------------------------------------------

void LoadoutOptionsMenu (void)
{
	static int choice = 0;

	CMenu	m (25);
	int	i, nOptions = 0;

m.AddText (TXT_GUN_LOADOUT, 0);
optGuns = m.ToS ();
for (i = 0; i < (int) sizeofa (pszGuns); i++, nOptions++)
	m.AddCheck (pszGuns [i], (extraGameInfo [0].loadout.nGuns & (1 << i)) != 0, 0, HTX_GUN_LOADOUT);
m.AddText ("", 0);
m.AddText (TXT_DEVICE_LOADOUT, 0);
optDevices = m.ToS ();
for (i = 0; i < (int) sizeofa (pszDevices); i++, nOptions++)
	m.AddCheck (pszDevices [i], (extraGameInfo [0].loadout.nDevice & (nDeviceFlags [i])) != 0, 0, HTX_DEVICE_LOADOUT);
do {
	i = m.Menu (NULL, TXT_LOADOUT_MENUTITLE, LoadoutCallback, &choice);
	} while (i != -1);
extraGameInfo [0].loadout.nGuns = 0;
for (i = 0; i < (int) sizeofa (pszGuns); i++) {
	if (m [optGuns + i].m_value)
		extraGameInfo [0].loadout.nGuns |= (1 << i);
	}
extraGameInfo [0].loadout.nDevice = 0;
for (i = 0; i < (int) sizeofa (pszDevices); i++) {
	if (m [optDevices + i].m_value)
		extraGameInfo [0].loadout.nDevice |= nDeviceFlags [i];
	}
AddPlayerLoadout ();
}

//------------------------------------------------------------------------------

static int nAIAggressivity;

int GameplayOptionsCallback (CMenu& menu, int& key, int nCurItem, int nState)
{
if (nState)
	return nCurItem;

	CMenuItem*	m;
	int			v;

m = menu + gplayOpts.nDifficulty;
v = m->m_value;
if (gameStates.app.nDifficultyLevel != v) {
	gameStates.app.nDifficultyLevel = v;
	if (!IsMultiGame) {
		gameStates.app.nDifficultyLevel = v;
		gameData.bosses.InitGateIntervals ();
		}
	sprintf (m->m_text, TXT_DIFFICULTY2, MENU_DIFFICULTY_TEXT (gameStates.app.nDifficultyLevel));
	m->m_bRebuild = 1;
	}

m = menu + gplayOpts.nAIAggressivity;
v = m->m_value;
if (nAIAggressivity != v) {
	nAIAggressivity = v;
	sprintf (m->m_text, TXT_AI_AGGRESSIVITY, pszAggressivities [v]);
	m->m_bRebuild = 1;
	}

m = menu + gplayOpts.nWeaponSwitch;
v = m->m_value;
if (gameOpts->gameplay.nAutoSelectWeapon != v) {
	gameOpts->gameplay.nAutoSelectWeapon = v;
	sprintf (m->m_text, TXT_WEAPON_SWITCH, pszWeaponSwitch [v]);
	m->m_bRebuild = 1;
	}

return nCurItem;
}

//------------------------------------------------------------------------------

void GameplayOptionsMenu (void)
{
	static int choice = 0;

	CMenu m;
	int	i;
	int	optSmartWeaponSwitch = -1, optHeadlightBuiltIn = -1, optHeadlightPowerDrain = -1, optNoThief = -1;
	int	optReorderPrim, optReorderSec;
	char	szSlider [50];

pszAggressivities [0] = TXT_STANDARD;
pszAggressivities [1] = TXT_MODERATE;
pszAggressivities [2] = TXT_MEDIUM;
pszAggressivities [3] = TXT_HIGH;
pszAggressivities [4] = TXT_VERY_HIGH;
pszAggressivities [5] = TXT_EXTREME;

pszWeaponSwitch [0] = TXT_NEVER;
pszWeaponSwitch [1] = TXT_WHEN_EMPTY;
pszWeaponSwitch [2] = TXT_CHOSE_BEST;

nAIAggressivity = (gameOpts->gameplay.nAIAggressivity && gameOpts->gameplay.nAIAwareness) ? 5 :  gameOpts->gameplay.nAIAggressivity;
do {
	m.Destroy ();
	m.Create (20);

	sprintf (szSlider + 1, TXT_DIFFICULTY2, MENU_DIFFICULTY_TEXT (gameStates.app.nDifficultyLevel));
	*szSlider = *(TXT_DIFFICULTY2 - 1);
	gplayOpts.nDifficulty = m.AddSlider (szSlider + 1, gameStates.app.nDifficultyLevel, 0, 4, KEY_D, HTX_GPLAY_DIFFICULTY);

	sprintf (szSlider + 1, TXT_AI_AGGRESSIVITY, pszAggressivities [nAIAggressivity]);
	*szSlider = *(TXT_AI_AGGRESSIVITY - 1);
	gplayOpts.nAIAggressivity = m.AddSlider (szSlider + 1, nAIAggressivity, 0, 5, KEY_A, HTX_AI_AGGRESSIVITY);

	sprintf (szSlider + 1, TXT_WEAPON_SWITCH, pszWeaponSwitch [gameOpts->gameplay.nAutoSelectWeapon]);
	*szSlider = *(TXT_WEAPON_SWITCH - 1);
	gplayOpts.nWeaponSwitch = m.AddSlider (szSlider + 1, gameOpts->gameplay.nAutoSelectWeapon, 0, 2, KEY_W, HTX_WEAPON_SWITCH);

	m.AddText ("", 0);
	optSmartWeaponSwitch = m.AddCheck (TXT_SMART_WPNSWITCH, extraGameInfo [0].bSmartWeaponSwitch, KEY_S, HTX_GPLAY_SMARTSWITCH);
	optHeadlightBuiltIn = m.AddCheck (TXT_HEADLIGHT_BUILTIN, extraGameInfo [0].headlight.bBuiltIn, KEY_B, HTX_HEADLIGHT_BUILTIN);
	optHeadlightPowerDrain = m.AddCheck (TXT_HEADLIGHT_POWERDRAIN, extraGameInfo [0].headlight.bDrainPower, KEY_H, HTX_HEADLIGHT_POWERDRAIN);
	optNoThief = m.AddCheck (TXT_SUPPRESS_THIEF, gameOpts->gameplay.bNoThief, KEY_T, HTX_SUPPRESS_THIEF);
	m.AddText ("");
	optReorderPrim = m.AddMenu (TXT_PRIMARY_PRIO, KEY_P, HTX_OPTIONS_PRIMPRIO);
	optReorderSec = m.AddMenu (TXT_SECONDARY_PRIO, KEY_E, HTX_OPTIONS_SECPRIO);
	for (;;) {
		if (0 > (i = m.Menu (NULL, TXT_GAMEPLAY_OPTS, GameplayOptionsCallback, &choice)))
			break;
		if (choice == optReorderPrim)
			ReorderPrimary ();
		else if (choice == optReorderSec)
			ReorderSecondary ();
		};
	} while (i == -2);
if (nAIAggressivity == 5) {
	gameOpts->gameplay.nAIAwareness = 1;
	gameOpts->gameplay.nAIAggressivity = nAIAggressivity - 1;
	}
else {
	gameOpts->gameplay.nAIAwareness = 0;
	gameOpts->gameplay.nAIAggressivity = nAIAggressivity;
	}

extraGameInfo [0].headlight.bAvailable = m [gplayOpts.nHeadlightAvailable].m_value;
extraGameInfo [0].bSmartWeaponSwitch = m [optSmartWeaponSwitch].m_value;
GET_VAL (gameOpts->gameplay.bNoThief, optNoThief);
GET_VAL (extraGameInfo [0].headlight.bDrainPower, optHeadlightPowerDrain);
GET_VAL (extraGameInfo [0].headlight.bBuiltIn, optHeadlightBuiltIn);
DefaultGameplaySettings ();
if (IsMultiGame && !COMPETITION && EGI_FLAG (bSmokeGrenades, 0, 0, 0))
	LOCALPLAYER.secondaryAmmo [PROXMINE_INDEX] = 4;
if (IsMultiGame)
	NetworkSendExtraGameInfo (NULL);
}

//------------------------------------------------------------------------------
//eof
