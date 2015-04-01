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
#include "systemkeys.h"

static const char *pszAggressivities [6];
static const char *pszWeaponSwitch [3];

void DefaultGameplaySettings (bool bSetup = false);

//------------------------------------------------------------------------------

static const char *pszGuns [] = {"Laser", "Vulcan", "Spreadfire", "Plasma", "Fusion", "Super Laser", "Gauss", "Helix", "Phoenix", "Omega"};
static const char *pszDevices [] = {"Full Map", "Ammo Rack", "Converter", "Quad Lasers", "Afterburner", "Headlight", "Slow Motion", "Bullet Time"};
static int32_t nDeviceFlags [] = {PLAYER_FLAGS_FULLMAP, PLAYER_FLAGS_AMMO_RACK, PLAYER_FLAGS_CONVERTER, PLAYER_FLAGS_QUAD_LASERS,
											 PLAYER_FLAGS_AFTERBURNER, PLAYER_FLAGS_HEADLIGHT, PLAYER_FLAGS_SLOWMOTION, PLAYER_FLAGS_BULLETTIME};

static int32_t optGuns, optDevices;

//------------------------------------------------------------------------------

static inline void SetGunLoadoutFlag (int32_t i, int32_t v)
{
if (v)
	extraGameInfo [0].loadout.nGuns |= (1 << i);
else
	extraGameInfo [0].loadout.nGuns &= ~(1 << i);
}

//------------------------------------------------------------------------------

static inline int32_t GetGunLoadoutFlag (int32_t i)
{
return (extraGameInfo [0].loadout.nGuns & (1 << i)) != 0;
}

//------------------------------------------------------------------------------

static inline void SetDeviceLoadoutFlag (int32_t i, int32_t v)
{
if (v)
	extraGameInfo [0].loadout.nDevice |= nDeviceFlags [i];
else
	extraGameInfo [0].loadout.nDevice &= ~nDeviceFlags [i];
}

//------------------------------------------------------------------------------

static inline int32_t GetDeviceLoadoutFlag (int32_t i)
{
return (extraGameInfo [0].loadout.nDevice & nDeviceFlags [i]) != 0;
}

//------------------------------------------------------------------------------

int32_t LoadoutCallback (CMenu& menu, int32_t& key, int32_t nCurItem, int32_t nState)
{
if (nState)
	return nCurItem;

	CMenuItem	*m = menu + nCurItem;
	int32_t			v = m->Value ();

if (nCurItem == menu.IndexOf (pszGuns [0])) {	//checked/unchecked lasers
	if (v != GetGunLoadoutFlag (0)) {
		SetGunLoadoutFlag (0, v);
		if (!v) {	//if lasers unchecked, also uncheck super lasers
			SetGunLoadoutFlag (5, 0);
			menu [optGuns + 5].Value () = 0;
			}
		}
	}
else if (nCurItem == menu.IndexOf (pszGuns [5])) {	//checked/unchecked super lasers
	if (v != GetGunLoadoutFlag (5)) {
		SetGunLoadoutFlag (5, v);
		if (v) {	// if super lasers checked, also check lasers
			SetGunLoadoutFlag (0, 1);
			menu [optGuns].Value () = 1;
			}
		}
	}
else if (nCurItem == menu.IndexOf (pszDevices [6])) {	//checked/unchecked super lasers
	if (v != GetDeviceLoadoutFlag (6)) {
		SetDeviceLoadoutFlag (6, v);
		if (!v) {	// if super lasers checked, also check lasers
			SetDeviceLoadoutFlag (7, 0);
			menu [optDevices + 7].Value () = 0;
			}
		}
	}
else if (nCurItem == menu.IndexOf (pszDevices [7])) {	//checked/unchecked super lasers
	if (v != GetDeviceLoadoutFlag (7)) {
		SetDeviceLoadoutFlag (7, v);
		if (v) {	// if super lasers checked, also check lasers
			SetDeviceLoadoutFlag (6, 1);
			menu [optDevices + 6].Value () = 1;
			}
		}
	}
return nCurItem;
}

//------------------------------------------------------------------------------

void LoadoutOptionsMenu (void)
{
	static int32_t choice = 0;

	CMenu	m (25);
	int32_t	i, j, nOptions = 0;

m.AddText ("", TXT_GUN_LOADOUT, 0);
for (i = 0; i < (int32_t) sizeofa (pszGuns); i++, nOptions++)
	m.AddCheck (pszGuns [i], pszGuns [i], (extraGameInfo [0].loadout.nGuns & (1 << i)) != 0, 0, HTX_GUN_LOADOUT);
m.AddText ("", "");
m.AddText ("", TXT_DEVICE_LOADOUT, 0);
for (i = 0; i < (int32_t) sizeofa (pszDevices); i++, nOptions++)
	m.AddCheck (pszDevices [i], pszDevices [i], (extraGameInfo [0].loadout.nDevice & (nDeviceFlags [i])) != 0, 0, HTX_DEVICE_LOADOUT);

do {
	i = m.Menu (NULL, TXT_LOADOUT_MENUTITLE, LoadoutCallback, &choice);
	} while (i != -1);

extraGameInfo [0].loadout.nGuns = 0;
for (i = 0, j = m.IndexOf (pszGuns [0]); i < (int32_t) sizeofa (pszGuns); i++) {
	if (m [j + i].Value ())
		extraGameInfo [0].loadout.nGuns |= (1 << i);
	}

extraGameInfo [0].loadout.nDevice = 0;
for (i = 0, j = m.IndexOf (pszDevices [0]); i < (int32_t) sizeofa (pszDevices); i++) {
	if (m [j + i].Value ())
		extraGameInfo [0].loadout.nDevice |= nDeviceFlags [i];
	}

AddPlayerLoadout ();
}

//------------------------------------------------------------------------------

static int32_t nAIAggressivity;

int32_t GameplayOptionsCallback (CMenu& menu, int32_t& key, int32_t nCurItem, int32_t nState)
{
if (nState)
	return nCurItem;

	CMenuItem*	m;
	int32_t			v;

if ((m = menu ["difficulty"])) {
	v = m->Value ();
	if (gameStates.app.nDifficultyLevel != v) {
		gameStates.app.nDifficultyLevel = v;
		if (!IsMultiGame) {
			gameStates.app.nDifficultyLevel = v;
			gameData.bosses.InitGateIntervals ();
			}
		sprintf (m->m_text, TXT_DIFFICULTY2, MENU_DIFFICULTY_TEXT (gameStates.app.nDifficultyLevel));
		m->m_bRebuild = 1;
		}
	}

if ((m = menu ["ai aggressivity"])) {
	v = m->Value ();
	if (nAIAggressivity != v) {
		nAIAggressivity = v;
		sprintf (m->m_text, TXT_AI_AGGRESSIVITY, pszAggressivities [v]);
		m->m_bRebuild = 1;
		}
	}

if ((m = menu ["weapon switch"])) {
	v = m->Value ();
	if (gameOpts->gameplay.nAutoSelectWeapon != v) {
		gameOpts->gameplay.nAutoSelectWeapon = v;
		sprintf (m->m_text, TXT_WEAPON_SWITCH, pszWeaponSwitch [v]);
		m->m_bRebuild = 1;
		}
	}

if ((m = menu ["recharge energy"])) {
	v = m->Value ();
	if (extraGameInfo [0].bRechargeEnergy != v) {
		extraGameInfo [0].bRechargeEnergy = v;
		m->m_bRebuild = 1;
		key = -2;
		}
	}

if ((m = menu ["recharge delay"])) {
	v = m->Value ();
	if (extraGameInfo [0].nRechargeDelay != v) {
		extraGameInfo [0].nRechargeDelay = v;
		sprintf (m->m_text, TXT_RECHARGE_DELAY, extraGameInfo [0].nRechargeDelay);
		m->m_bRebuild = 1;
		}
	}

return nCurItem;
}

//------------------------------------------------------------------------------

void AddShipSelection (CMenu& m, int32_t& optShip)
{
	int32_t h, i;

for (h = i = 0; i < 3; i++) {
	if (missionConfig.m_shipsAllowed [i])
		h++;
	}
if (h < 2) 
	optShip = -1;
else { // more than one ship to chose from
	m.AddText ("", TXT_PLAYERSHIP);
	optShip = (missionConfig.m_shipsAllowed [0]) ? m.AddRadio ("standard ship", TXT_STANDARD_SHIP, 0, KEY_S, HTX_PLAYERSHIP) : -1;
	if (missionConfig.m_shipsAllowed [1]) {
		h = m.AddRadio ("light ship", TXT_LIGHT_SHIP, 0, KEY_I, HTX_PLAYERSHIP);
		if (optShip < 0)
			optShip = h;
		}
	if (missionConfig.m_shipsAllowed [2])
		m.AddRadio ("heavy ship", TXT_HEAVY_SHIP, 0, KEY_F, HTX_PLAYERSHIP);

	for (h = i = 0; i < MAX_SHIP_TYPES; i++) {
		if (missionConfig.m_shipsAllowed [i])
			m [optShip + h++].Value () = (i == gameOpts->gameplay.nShip [0]);
		}	
	}
}

//------------------------------------------------------------------------------

void GetShipSelection (CMenu& m, int32_t& optShip)
{
if (optShip >= 0) {
	for (int32_t i = 0, j = -1; i < MAX_SHIP_TYPES; i++) {
		if (missionConfig.m_shipsAllowed [i]) {
			if (m [optShip + ++j].Value ()) {
				gameOpts->gameplay.nShip [1] = i;
				break;
				}
			}
		}
	}
}

//------------------------------------------------------------------------------

static void InitStrings (void)
{
	static bool bInitialized = false;

if (bInitialized)
	return;
bInitialized = true;

pszAggressivities [0] = TXT_STANDARD;
pszAggressivities [1] = TXT_MODERATE;
pszAggressivities [2] = TXT_MEDIUM;
pszAggressivities [3] = TXT_HIGH;
pszAggressivities [4] = TXT_VERY_HIGH;
pszAggressivities [5] = TXT_EXTREME;

pszWeaponSwitch [0] = TXT_NEVER;
pszWeaponSwitch [1] = TXT_WHEN_EMPTY;
pszWeaponSwitch [2] = TXT_CHOSE_BEST;
}

//------------------------------------------------------------------------------

void GameplayOptionsMenu (void)
{
	static int32_t choice = 0;

	CMenu m;
	int32_t	i;
	int32_t	optShip = -1;
	int32_t	nShip = (gameOpts->gameplay.nShip [0] < 0) ? 0 : gameOpts->gameplay.nShip [0];
	char		szSlider [50];
	bool		bRestricted = gameStates.app.bGameRunning && IsMultiGame && !IsCoopGame;

InitStrings ();

nAIAggressivity = (gameOpts->gameplay.nAIAggressivity && gameOpts->gameplay.nAIAwareness) ? 5 :  gameOpts->gameplay.nAIAggressivity;
do {
	m.Destroy ();
	m.Create (20);

	if (!bRestricted) {
		sprintf (szSlider + 1, TXT_DIFFICULTY2, MENU_DIFFICULTY_TEXT (gameStates.app.nDifficultyLevel));
		*szSlider = *(TXT_DIFFICULTY2 - 1);
		m.AddSlider ("difficulty", szSlider + 1, gameStates.app.nDifficultyLevel, 0, 4, KEY_D, HTX_GPLAY_DIFFICULTY);

		sprintf (szSlider + 1, TXT_AI_AGGRESSIVITY, pszAggressivities [nAIAggressivity]);
		*szSlider = *(TXT_AI_AGGRESSIVITY - 1);
		m.AddSlider ("ai aggressivity", szSlider + 1, nAIAggressivity, 0, 5, KEY_A, HTX_AI_AGGRESSIVITY);
		}

	sprintf (szSlider + 1, TXT_WEAPON_SWITCH, pszWeaponSwitch [gameOpts->gameplay.nAutoSelectWeapon]);
	*szSlider = *(TXT_WEAPON_SWITCH - 1);
	m.AddSlider ("weapon switch", szSlider + 1, gameOpts->gameplay.nAutoSelectWeapon, 0, 2, KEY_W, HTX_WEAPON_SWITCH);

	m.AddText ("", "");
	m.AddCheck ("smart weapon switch", TXT_SMART_WPNSWITCH, extraGameInfo [0].bSmartWeaponSwitch, KEY_M, HTX_GPLAY_SMARTSWITCH);
	if (!bRestricted) {
		m.AddCheck ("headlight drains power", TXT_HEADLIGHT_POWERDRAIN, extraGameInfo [0].headlight.bDrainPower, KEY_O, HTX_HEADLIGHT_POWERDRAIN);
		m.AddCheck ("suppress thief", TXT_SUPPRESS_THIEF, gameOpts->gameplay.bNoThief, KEY_T, HTX_SUPPRESS_THIEF);
		}
	m.AddCheck ("observer mode", TXT_OBSERVER_MODE, gameOpts->gameplay.bObserve, KEY_B, HTX_OBSERVER_MODE);
	if (!bRestricted) {
		m.AddCheck ("use inventory", TXT_USE_INVENTORY, gameOpts->gameplay.bInventory, KEY_U, HTX_GPLAY_INVENTORY);
		m.AddCheck ("spinup gatling", TXT_SPINUP_GATLING, extraGameInfo [0].bGatlingSpeedUp, KEY_G, HTX_SPINUP_GATLING);
		if (!gameStates.app.bGameRunning)
			m.AddCheck ("allow custom weapons", TXT_ALLOW_CUSTOM_WEAPONS, extraGameInfo [0].bAllowCustomWeapons, KEY_C, HTX_ALLOW_CUSTOM_WEAPONS);
		m.AddCheck ("recharge energy", TXT_RECHARGE_ENERGY, extraGameInfo [0].bRechargeEnergy, KEY_R, HTX_RECHARGE_ENERGY);
		if (extraGameInfo [0].bRechargeEnergy) {
			sprintf (szSlider + 1, TXT_RECHARGE_DELAY, CShipEnergy::RechargeDelay (extraGameInfo [0].nRechargeDelay));
			*szSlider = *(TXT_RECHARGE_DELAY - 1);
			m.AddSlider ("recharge delay", szSlider + 1, extraGameInfo [0].nRechargeDelay, 0, CShipEnergy::RechargeDelayCount () - 1, KEY_D, HTX_RECHARGE_DELAY);
			}
		}
	m.AddText ("", "");
	m.AddMenu ("reorder guns", TXT_PRIMARY_PRIO, KEY_P, HTX_OPTIONS_PRIMPRIO);
	m.AddMenu ("reorder missiles", TXT_SECONDARY_PRIO, KEY_E, HTX_OPTIONS_SECPRIO);
	//if (gameStates.app.bGameRunning)
		AddShipSelection (m, optShip);
	if (!bRestricted) {
		m.AddText ("", "");
		m.AddMenu ("loadout options", TXT_LOADOUT_OPTION, KEY_Q, HTX_MULTI_LOADOUT);
		}

	for (;;) {
		if (0 > (i = m.Menu (NULL, TXT_GAMEPLAY_OPTS, GameplayOptionsCallback, &choice)))
			break;
		if (!bRestricted && (choice == m.IndexOf ("loadout options")))
			LoadoutOptionsMenu ();
		else if (choice == m.IndexOf ("reorder guns"))
			ReorderPrimary ();
		else if (choice == m.IndexOf ("reorder missiles"))
			ReorderSecondary ();
		}
	} while (i == -2);

if (!bRestricted) {
	if (nAIAggressivity == 5) {
		gameOpts->gameplay.nAIAwareness = 1;
		gameOpts->gameplay.nAIAggressivity = 4;
		}
	else {
		gameOpts->gameplay.nAIAwareness = 0;
		gameOpts->gameplay.nAIAggressivity = nAIAggressivity;
		}
	}

extraGameInfo [0].bSmartWeaponSwitch = m.Value ("smart weapon switch");
if (!bRestricted)
	GET_VAL (gameOpts->gameplay.bInventory, "use inventory");
GET_VAL (gameOpts->gameplay.bNoThief, "suppress thief");
GET_VAL (gameOpts->gameplay.bObserve, "observer mode");
if (!bRestricted) {
	GET_VAL (extraGameInfo [0].headlight.bDrainPower, "headlight drains power");
	GET_VAL (extraGameInfo [0].bGatlingSpeedUp, "spinup gatling");
	if (!gameStates.app.bGameRunning) {
		GET_VAL (extraGameInfo [0].bAllowCustomWeapons, "allow custom weapons");
		extraGameInfo [1].bAllowCustomWeapons = extraGameInfo [0].bAllowCustomWeapons;
		if (!extraGameInfo [0].bAllowCustomWeapons)
			SetDefaultWeaponProps ();
		}
	}
GetShipSelection (m, optShip);
DefaultGameplaySettings ();
if (IsMultiGame && !COMPETITION && EGI_FLAG (bSmokeGrenades, 0, 0, 0))
	LOCALPLAYER.secondaryAmmo [PROXMINE_INDEX] = 4;
if (IsMultiGame)
	NetworkSendExtraGameInfo (NULL);
if ((optShip >= 0) && (gameOpts->gameplay.nShip [1] != nShip)) {
	missionConfig.m_playerShip = gameOpts->gameplay.nShip [1];
	if (!gameStates.app.bGameRunning) {
		gameOpts->gameplay.nShip [0] = gameOpts->gameplay.nShip [1];
		//gameOpts->gameplay.nShip [1] = -1;
		}
	else {
		gameStates.app.bChangingShip = 1;
		SetChaseCam (0);
		SetFreeCam (0);
		SetRearView (0);
#if 1 //DBG
		LOCALPLAYER.lives++;
#endif
		gameStates.gameplay.xInitialShield [1] = LOCALPLAYER.Shield (false);
		gameStates.gameplay.xInitialEnergy [1] = LOCALPLAYER.Energy (false);
		LOCALPLAYER.SetShield (-1);
		if (LOCALPLAYER.Object ())
			LOCALPLAYER.Object ()->Die ();
		MultiSendWeaponStates ();
		}
	}
}

//------------------------------------------------------------------------------
//eof
