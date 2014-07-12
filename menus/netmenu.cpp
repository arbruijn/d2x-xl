#ifdef HAVE_CONFIG_H
#	include <conf.h>
#endif

#define PATCH12

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef _WIN32
#	include <winsock.h>
#else
#	include <sys/socket.h>
#endif

#include "descent.h"
#include "strutil.h"
#include "args.h"
#include "timer.h"
#include "ipx.h"
#include "ipx_udp.h"
#include "menu.h"
#include "key.h"
#include "error.h"
#include "network.h"
#include "network_lib.h"
#include "menu.h"
#include "text.h"
#include "byteswap.h"
#include "netmisc.h"
#include "kconfig.h"
#include "autodl.h"
#include "tracker.h"
#include "gamefont.h"
#include "netmenu.h"
#include "monsterball.h"
#include "menubackground.h"
#include "console.h"
#if DBG
#	include "cstring.h"
#endif

#define LHX(x)      (gameStates.menus.bHires?2* (x):x)
#define LHY(y)      (gameStates.menus.bHires? (24* (y))/10:y)

/* the following are the possible packet identificators.
 * they are stored in the "nType" field of the packet structs.
 * they are offset 4 bytes from the beginning of the raw IPX data
 * because of the "driver's" ipx_packetnum (see linuxnet.c).
 */

extern CNetworkAddress ipx_MyAddress;
static const char* pszMissionName = "";

//------------------------------------------------------------------------------

void NetworkSetWeaponsAllowed (void)
{
CMenu	m (40);

m.AddCheck ("laser upgrade", TXT_WA_LASER, netGameInfo.m_info.DoLaserUpgrade, 0, NULL);
m.AddCheck ("super laser", TXT_WA_SLASER, netGameInfo.m_info.DoSuperLaser, 0, NULL);
m.AddCheck ("quad lasers", TXT_WA_QLASER, netGameInfo.m_info.DoQuadLasers, 0, NULL);
m.AddCheck ("vulcan", TXT_WA_VULCAN, netGameInfo.m_info.DoVulcan, 0, NULL);
m.AddCheck ("spreadfire", TXT_WA_SPREAD, netGameInfo.m_info.DoSpread, 0, NULL);
m.AddCheck ("plasma", TXT_WA_PLASMA, netGameInfo.m_info.DoPlasma, 0, NULL);
m.AddCheck ("fusion", TXT_WA_FUSION, netGameInfo.m_info.DoFusions, 0, NULL);
m.AddCheck ("gauss", TXT_WA_GAUSS, netGameInfo.m_info.DoGauss, 0, NULL);
m.AddCheck ("helix", TXT_WA_HELIX, netGameInfo.m_info.DoHelix, 0, NULL);
m.AddCheck ("phoenix", TXT_WA_PHOENIX, netGameInfo.m_info.DoPhoenix, 0, NULL);
m.AddCheck ("omega", TXT_WA_OMEGA, netGameInfo.m_info.DoOmega, 0, NULL);

m.AddCheck ("homingmsl", TXT_WA_HOMING_MSL, netGameInfo.m_info.DoHoming, 0, NULL);
m.AddCheck ("proxmine", TXT_WA_PROXBOMB, netGameInfo.m_info.DoProximity, 0, NULL);
m.AddCheck ("smartmsl", TXT_WA_SMART_MSL, netGameInfo.m_info.DoSmarts, 0, NULL);
m.AddCheck ("megamsl", TXT_WA_MEGA_MSL, netGameInfo.m_info.DoMegas, 0, NULL);
m.AddCheck ("flashmsl", TXT_WA_FLASH_MSL, netGameInfo.m_info.DoFlash, 0, NULL);
m.AddCheck ("guidedmsl", TXT_WA_GUIDED_MSL, netGameInfo.m_info.DoGuided, 0, NULL);
m.AddCheck ("smartmine", TXT_WA_SMARTMINE, netGameInfo.m_info.DoSmartMine, 0, NULL);
m.AddCheck ("mercurymsl", TXT_WA_MERC_MSL, netGameInfo.m_info.DoMercury, 0, NULL);
m.AddCheck ("eshakermsl", TXT_WA_SHAKER_MSL, netGameInfo.m_info.DoEarthShaker, 0, NULL);

m.AddCheck ("invul", TXT_WA_INVUL, netGameInfo.m_info.DoInvulnerability, 0, NULL);
m.AddCheck ("cloak", TXT_WA_CLOAK, netGameInfo.m_info.DoCloak, 0, NULL);
m.AddCheck ("afterburner", TXT_WA_BURNER, netGameInfo.m_info.DoAfterburner, 0, NULL);
m.AddCheck ("ammorack", TXT_WA_AMMORACK, netGameInfo.m_info.DoAmmoRack, 0, NULL);
m.AddCheck ("converter", TXT_WA_CONVERTER, netGameInfo.m_info.DoConverter, 0, NULL);
m.AddCheck ("headlight", TXT_WA_HEADLIGHT, netGameInfo.m_info.DoHeadlight, 0, NULL);

m.Menu (NULL, TXT_WA_OBJECTS);

netGameInfo.m_info.DoLaserUpgrade = m.Value ("laser upgrade"); 
netGameInfo.m_info.DoSuperLaser = m.Value ("super laser");
netGameInfo.m_info.DoQuadLasers = m.Value ("quad lasers");  
netGameInfo.m_info.DoVulcan = m.Value ("vulcan");
netGameInfo.m_info.DoSpread = m.Value ("spreadfire");
netGameInfo.m_info.DoPlasma = m.Value ("plasma");
netGameInfo.m_info.DoFusions = m.Value ("fusion");
netGameInfo.m_info.DoGauss = m.Value ("gauss");
netGameInfo.m_info.DoHelix = m.Value ("helix");
netGameInfo.m_info.DoPhoenix = m.Value ("phoenix");
netGameInfo.m_info.DoOmega = m.Value ("omega");

netGameInfo.m_info.DoHoming = m.Value ("homingmsl");
netGameInfo.m_info.DoProximity = m.Value ("proxmine");
netGameInfo.m_info.DoSmarts = m.Value ("smartmsl");
netGameInfo.m_info.DoMegas = m.Value ("megamsl");
netGameInfo.m_info.DoFlash = m.Value ("flashmsl");
netGameInfo.m_info.DoGuided = m.Value ("guidedmsl");
netGameInfo.m_info.DoSmartMine = m.Value ("smartmine");
netGameInfo.m_info.DoMercury = m.Value ("mercurymsl");
netGameInfo.m_info.DoEarthShaker = m.Value ("eshakermsl");

netGameInfo.m_info.DoInvulnerability = m.Value ("invul");
netGameInfo.m_info.DoCloak = m.Value ("cloak");
netGameInfo.m_info.DoAfterburner = m.Value ("afterburner");
netGameInfo.m_info.DoAmmoRack = m.Value ("ammorack");
netGameInfo.m_info.DoConverter = m.Value ("converter");     
netGameInfo.m_info.DoHeadlight = m.Value ("headlight");     
}

//------------------------------------------------------------------------------

fix nLastPlayTime = -1;
int32_t nLastReactorLife = 0;
int32_t nLastScoreGoal = -1;
int32_t nLastMaxPlayers = -1;

//------------------------------------------------------------------------------


#define	WF(_f)	 ((int16_t) (( (nFilter) & (1 << (_f))) != 0))

void SetAllAllowablesTo (int32_t nFilter)
{
	nLastReactorLife = 0;   //default to zero
   
netGameInfo.m_info.DoMegas = WF (0);
netGameInfo.m_info.DoSmarts = WF (1);
netGameInfo.m_info.DoFusions = WF (2);
netGameInfo.m_info.DoHelix = WF (3);
netGameInfo.m_info.DoPhoenix = WF (4);
netGameInfo.m_info.DoCloak = WF (5);
netGameInfo.m_info.DoInvulnerability = WF (6);
netGameInfo.m_info.DoAfterburner = WF (7);
netGameInfo.m_info.DoGauss = WF (8);
netGameInfo.m_info.DoVulcan = WF (9);
netGameInfo.m_info.DoPlasma = WF (10);
netGameInfo.m_info.DoOmega = WF (11);
netGameInfo.m_info.DoSuperLaser = WF (12);
netGameInfo.m_info.DoProximity = WF (13);
netGameInfo.m_info.DoSpread = WF (14);
netGameInfo.m_info.DoMercury = WF (15);
netGameInfo.m_info.DoSmartMine = WF (16);
netGameInfo.m_info.DoFlash = WF (17);
netGameInfo.m_info.DoGuided = WF (18);
netGameInfo.m_info.DoEarthShaker = WF (19);
netGameInfo.m_info.DoConverter = WF (20);
netGameInfo.m_info.DoAmmoRack = WF (21);
netGameInfo.m_info.DoHeadlight = WF (22);
netGameInfo.m_info.DoHoming = WF (23);
netGameInfo.m_info.DoLaserUpgrade = WF (24);
netGameInfo.m_info.DoQuadLasers = WF (25);
netGameInfo.m_info.BrightPlayers = !WF (26);
netGameInfo.m_info.invul = WF (27);
}

#undef WF

//------------------------------------------------------------------------------

#define	WF(_w, _f)	if (_w) mpParams.nWeaponFilter |= (1 << (_f))

void GetAllAllowables (void)
{
mpParams.nWeaponFilter = 0;
WF (netGameInfo.m_info.DoMegas, 0);
WF (netGameInfo.m_info.DoSmarts, 1);
WF (netGameInfo.m_info.DoFusions, 2);
WF (netGameInfo.m_info.DoHelix, 3);
WF (netGameInfo.m_info.DoPhoenix, 4);
WF (netGameInfo.m_info.DoCloak, 5);
WF (netGameInfo.m_info.DoInvulnerability, 6);
WF (netGameInfo.m_info.DoAfterburner, 7);
WF (netGameInfo.m_info.DoGauss, 8);
WF (netGameInfo.m_info.DoVulcan, 9);
WF (netGameInfo.m_info.DoPlasma, 10);
WF (netGameInfo.m_info.DoOmega, 11);
WF (netGameInfo.m_info.DoSuperLaser, 12);
WF (netGameInfo.m_info.DoProximity, 13);
WF (netGameInfo.m_info.DoSpread, 14);
WF (netGameInfo.m_info.DoMercury, 15);
WF (netGameInfo.m_info.DoSmartMine, 16);
WF (netGameInfo.m_info.DoFlash, 17);
WF (netGameInfo.m_info.DoGuided, 18);
WF (netGameInfo.m_info.DoEarthShaker, 19);
WF (netGameInfo.m_info.DoConverter, 20);
WF (netGameInfo.m_info.DoAmmoRack, 21);
WF (netGameInfo.m_info.DoHeadlight, 22);
WF (netGameInfo.m_info.DoHoming, 23);
WF (netGameInfo.m_info.DoLaserUpgrade, 24);
WF (netGameInfo.m_info.DoQuadLasers, 25);
WF (!netGameInfo.m_info.BrightPlayers, 26);
WF (netGameInfo.m_info.invul, 27);
}

#undef WF

//------------------------------------------------------------------------------

#define ENDLEVEL_SEND_INTERVAL  500
#define ENDLEVEL_IDLE_TIME      20000

int32_t NetworkEndLevelPoll2 (CMenu& menu, int32_t& key, int32_t nCurItem, int32_t nState)
{
if (nState)
	return nCurItem;

	// Polling loop for End-of-level menu

// Send our endlevel packet at regular intervals
NetworkSendLifeSign ();
NetworkListen ();

int32_t nReady = 0;
int32_t bSecret = 0;

for (int32_t i = 0; i < gameData.multiplayer.nPlayers; i++) {
	if ((gameData.multiplayer.players [i].connected != CONNECT_PLAYING) && 
		 (gameData.multiplayer.players [i].connected != CONNECT_ESCAPE_TUNNEL) && 
		 (gameData.multiplayer.players [i].connected != CONNECT_END_MENU)) {
		nReady++;
		if (gameData.multiplayer.players [i].connected == CONNECT_FOUND_SECRET)
			bSecret = 1;                                        
		}
	}
if (nReady == gameData.multiplayer.nPlayers) {// All players have checked in or are disconnected
	if (bSecret)
		key = -3;
	else
		key = -2;
	}
return nCurItem;
}

//------------------------------------------------------------------------------

int32_t NetworkEndLevelPoll3 (CMenu& menu, int32_t& key, int32_t nCurItem, int32_t nState)
{
if (nState)
	return nCurItem;

	// Polling loop for End-of-level menu
   int32_t nReady = 0, i;
 
if (TimerGetApproxSeconds () > (gameData.multiplayer.xStartAbortMenuTime + (I2X (8))))
	key = -2;
//if (!networkThread.Available ())
	NetworkListen ();
for (i = 0; i < gameData.multiplayer.nPlayers; i++)
	if ((gameData.multiplayer.players [i].connected != CONNECT_PLAYING) && 
		 (gameData.multiplayer.players [i].connected != CONNECT_ESCAPE_TUNNEL) && 
		 (gameData.multiplayer.players [i].connected != CONNECT_END_MENU))
		nReady++;
if (nReady == gameData.multiplayer.nPlayers) // All players have checked in or are disconnected
	key = -2;
return nCurItem;
}

//------------------------------------------------------------------------------

int32_t NetworkStartPoll (CMenu& menu, int32_t& key, int32_t nCurItem, int32_t nState)
{
if (nState)
	return nCurItem;

	int32_t i, n, nm;

Assert (networkData.nStatus == NETSTAT_STARTING);
if (!menu [0].Value ()) {
	menu [0].Value () = 1;
	menu [0].Rebuild ();
	}
for (i = 1; i < int32_t (menu.ToS ()); i++) {
	if ((i >= gameData.multiplayer.nPlayers) && menu [i].Value ()) {
		menu [i].Value () = 0;
		menu [i].Rebuild ();
		}
	}
nm = 0;
for (i = 0; i < int32_t (menu.ToS ()); i++) {
	if (menu [i].Value ()) {
		if (++nm > gameData.multiplayer.nPlayers) {
			menu [i].Value () = 0;
			menu [i].Rebuild ();
			}
		}
	}
if (nm > gameData.multiplayer.nMaxPlayers) {
	InfoBox (TXT_ERROR, NULL, BG_STANDARD, 1, TXT_OK, "%s %d %s", TXT_SORRY_ONLY, gameData.multiplayer.nMaxPlayers, TXT_NETPLAYERS_IN);
	// Turn off the last CPlayerData highlighted
	for (i = gameData.multiplayer.nPlayers; i > 0; i--)
		if (menu [i].Value () == 1) {
			menu [i].Value () = 0;
			menu [i].Rebuild ();
			break;
			}
	}

//       if (menu.ToS () > MAX_PLAYERS) return; 

n = netGameInfo.m_info.nNumPlayers;
NetworkListen ();

if (n < netGameInfo.m_info.nNumPlayers) {
	audio.PlaySound (SOUND_HUD_MESSAGE);
	if (gameOpts->multi.bNoRankings)
	   sprintf (menu [gameData.multiplayer.nPlayers - 1].Text (), "%d. %-20s", 
					gameData.multiplayer.nPlayers, 
					netPlayers [0].m_info.players [gameData.multiplayer.nPlayers-1].callsign);
	else
	   sprintf (menu [gameData.multiplayer.nPlayers - 1].Text (), "%d. %s%-20s", 
					gameData.multiplayer.nPlayers, 
					pszRankStrings [netPlayers [0].m_info.players [gameData.multiplayer.nPlayers-1].rank], 
					netPlayers [0].m_info.players [gameData.multiplayer.nPlayers-1].callsign);
	menu [gameData.multiplayer.nPlayers - 1].Rebuild ();
	if (gameData.multiplayer.nPlayers <= gameData.multiplayer.nMaxPlayers)
		menu [gameData.multiplayer.nPlayers - 1].Value () = 1;
	} 
else if (n > netGameInfo.m_info.nNumPlayers) {
	// One got removed...
   audio.PlaySound (SOUND_HUD_KILL);
	for (i = 0; i < gameData.multiplayer.nPlayers; i++) {
		if (gameOpts->multi.bNoRankings)
			sprintf (menu [i].Text (), "%d. %-20s", i+1, netPlayers [0].m_info.players [i].callsign);
		else
			sprintf (menu [i].Text (), "%d. %s%-20s", i+1, pszRankStrings [netPlayers [0].m_info.players [i].rank], netPlayers [0].m_info.players [i].callsign);
		menu [i].Value () = (i < gameData.multiplayer.nMaxPlayers);
		menu [i].Rebuild ();
		}
	for (i = gameData.multiplayer.nPlayers; i<n; i++)  {
		sprintf (menu [i].Text (), "%d. ", i+1);          // Clear out the deleted entries...
		menu [i].Value () = 0;
		menu [i].Rebuild ();
		}
   }
return nCurItem;
}

//------------------------------------------------------------------------------

const char* szGameTypeList [10];
const char* szGameAccessList [3];

static int32_t nGameTypes = -1, nGameItem = -1;
static int32_t oldMaxPlayers = 0;

//------------------------------------------------------------------------------

inline void AdjustMaxPlayers (CMenuItem* maxPlayers, bool bCoop)
{
if (bCoop) { // coop
	oldMaxPlayers = 1;
	if (maxPlayers->Value () > 2)  {
		maxPlayers->Value () = 2;
		maxPlayers->Redraw ();
		}
	if (!(netGameInfo.m_info.gameFlags & NETGAME_FLAG_SHOW_MAP))
		netGameInfo.m_info.gameFlags |= NETGAME_FLAG_SHOW_MAP;
	if (netGameInfo.GetPlayTimeAllowed () || netGameInfo.GetScoreGoal ()) {
		netGameInfo.SetPlayTimeAllowed (0);
		netGameInfo.SetScoreGoal (0);
		}
	}
else {
	if (oldMaxPlayers) {
		oldMaxPlayers = 0;
		maxPlayers->Value () = 
		maxPlayers->MaxValue () = MAX_NUM_NET_PLAYERS - 2;
		}
	}
}

//------------------------------------------------------------------------------

int32_t NetworkGameParamPoll (CMenu& menu, int32_t& key, int32_t nCurItem, int32_t nState)
{
if (nState)
	return nCurItem;
if (!menu.Available ("max. players"))
	return nCurItem;

CMenuItem* maxPlayers = menu ["max. players"];

if (gameStates.app.bNostalgia) {
	int32_t i = menu.IndexOf ("anarchy");
	if ((nCurItem >= i) && (nCurItem < i + nGameTypes)) {
		if ((nCurItem != nGameItem) && menu [nCurItem].Value ()) {
			nGameItem = nCurItem;
			key = -2;
			return nCurItem;
			}
		}
	// enable/disable entropy / monsterball options menu links
	if ((menu.Available ("entropy") && ((menu.Value ("entropy") != 0) == !menu.Available ("entropy options"))) ||
		 (menu.Available ("monsterball") && ((menu.Value ("monsterball") != 0) == !menu.Available ("monsterball options")) && (gameOpts->app.bExpertMode == SUPERUSER)))
		key = -2;
	//force restricted game for team games
	//obsolete with D2X-W32 as it can assign players to teams automatically
	//even in a match and progress, and allows players to switch teams
	AdjustMaxPlayers (maxPlayers, menu.Value ("coop") != 0);
	}
else {
	CMenuItem*	m;
	int32_t		v;
	char			szSlider [50];

	if ((m = menu ["game type"])) {
		v = m->Value ();
		if (mpParams.nGameType != v) {
			mpParams.nGameType = v;
			sprintf (szSlider + 1, TXT_GAME_TYPE, szGameTypeList [mpParams.nGameType]);
			*szSlider = *(TXT_GAME_TYPE - 1);
			strcpy (m->Text (), szSlider + 1);
			m->Rebuild ();
			AdjustMaxPlayers (maxPlayers, v == 3);
			// enable/disable entropy / monsterball options menu links
			if (((m->MaxValue () == sizeofa (szGameTypeList) - 1) && ((v == 8) != menu.Available ("entropy options"))) ||
				 ((m->MaxValue () == sizeofa (szGameTypeList) - 1) && ((v == 9) != menu.Available ("monsterball options")) && (gameOpts->app.bExpertMode == SUPERUSER)))
				key = -2;
			}
		}  

	if ((m = menu ["game access"])) {
		v = m->Value ();
		if (mpParams.nGameAccess != v) {
			mpParams.nGameAccess = v;
			sprintf (szSlider + 1, TXT_GAME_ACCESS, szGameAccessList [mpParams.nGameAccess]);
			*szSlider = *(TXT_GAME_ACCESS - 1);
			strcpy (m->Text (), szSlider + 1);
			m->Rebuild ();
			}
		}  

	if ((m = menu ["level number"])) {
		v = m->Value () + 1;
		if (mpParams.nLevel != v) {
			mpParams.nLevel = v;
			sprintf (szSlider, "%s %d", TXT_LEVEL_, mpParams.nLevel);
			strcpy (m->Text (), szSlider);
			m->Rebuild ();
			}
		}  
	}

if (nLastMaxPlayers != menu.Value ("max. players"))  {
	sprintf (maxPlayers->Text (), TXT_MAX_PLAYERS, maxPlayers->Value () + 2);
	nLastMaxPlayers = maxPlayers->Value ();
	maxPlayers->Rebuild ();
	}               
return nCurItem;
}

//------------------------------------------------------------------------------

const uint8_t ppsTable [] = {5, 10, 20, 30, 50, 100};

static inline uint8_t FindNearestPPS (void)
{
int32_t i = sizeof (ppsTable);
while (i && ppsTable [--i] > mpParams.nMinPPS)
	;
return i;
}

//------------------------------------------------------------------------------

int32_t NetworkMoreOptionsPoll (CMenu& menu, int32_t& key, int32_t nCurItem, int32_t nState)
{
if (nState)
	return nCurItem;

CMenuItem*	m;
int32_t		v;

if ((m = menu ["reactor life"])) {
	v = m->Value ();
	if (nLastReactorLife != v)  {
		sprintf (m->Text (), "%s: %d %s", TXT_REACTOR_LIFE, menu.Value ("reactor life") * 5, TXT_MINUTES_ABBREV);
		nLastReactorLife = menu.Value ("reactor life");
		m->Rebuild ();
		}
	}  

if ((m = menu ["play time"])) {
	v = m->Value ();
	if (nLastPlayTime != v) {
		nLastPlayTime = mpParams.nMaxTime;
		mpParams.nMaxTime = v;
		sprintf (m->Text (), TXT_MAXTIME, nLastPlayTime * 5, TXT_MINUTES_ABBREV);
		m->Rebuild ();
		}
	}

if ((m = menu ["score goal"])) {
	v = m->Value ();
	if (nLastScoreGoal != v) {
		nLastScoreGoal = mpParams.nScoreGoal;
		mpParams.nScoreGoal = v;
		sprintf (m->Text (), TXT_SCOREGOAL, mpParams.nScoreGoal * 5);
		m->Rebuild ();
		}
	}

if ((m = menu ["pps"])) {
	v = m->Value ();
	if (mpParams.nMinPPS != v) {
		mpParams.nMinPPS = ppsTable [v];
		sprintf (m->Text (), TXT_PPS2, mpParams.nMinPPS);
		m->Rebuild ();
		}
	}  


return nCurItem;
}

//------------------------------------------------------------------------------

void NetworkMoreGameOptions (void)
{
	static int32_t choice = 0;

	int32_t	i;
	char		szPlayTime [80], szScoreGoal [80], szInvul [50], szSocket [6], szSlider [50];
	CMenu		m;

do {
	m.Destroy ();
	m.Create (40);
	m.AddSlider ("difficulty", TXT_DIFFICULTY, mpParams.nDifficulty, 0, NDL - 1, KEY_D, HTX_GPLAY_DIFFICULTY); 
	sprintf (szInvul + 1, "%s: %d %s", TXT_REACTOR_LIFE, mpParams.nReactorLife * 5, TXT_MINUTES_ABBREV);
	strupr (szInvul + 1);
	*szInvul = * (TXT_REACTOR_LIFE - 1);
	m.AddSlider ("reactor life", szInvul + 1, mpParams.nReactorLife, 0, 10, KEY_R, HTX_MULTI2_REACTOR); 
	if (IsCoopGame) {
		nLastPlayTime =
		nLastScoreGoal = 0;
		}
	else {
		sprintf (szPlayTime + 1, TXT_MAXTIME, mpParams.nMaxTime * 5, TXT_MINUTES_ABBREV);
		*szPlayTime = * (TXT_MAXTIME - 1);
		m.AddSlider ("play time", szPlayTime + 1, mpParams.nMaxTime, 0, 10, KEY_T, HTX_MULTI2_LVLTIME); 
		sprintf (szScoreGoal + 1, TXT_SCOREGOAL, mpParams.nScoreGoal * 5);
		*szScoreGoal = *(TXT_SCOREGOAL - 1);
		m.AddSlider ("score goal", szScoreGoal + 1, mpParams.nScoreGoal, 0, 10, KEY_K, HTX_MULTI2_SCOREGOAL);
		}
	m.AddText ("", "");
	m.AddCheck ("spawn invul", TXT_INVUL_RESPAWN, mpParams.bInvul, KEY_I, HTX_MULTI2_INVUL);
	m.AddCheck ("marker cameras", TXT_MARKER_CAMS, mpParams.bMarkerView, KEY_C, HTX_MULTI2_MARKERCAMS);
	m.AddCheck ("indestructible lights", TXT_KEEP_LIGHTS, mpParams.bIndestructibleLights, KEY_L, HTX_MULTI2_KEEPLIGHTS);
	m.AddCheck ("bright ships", TXT_BRIGHT_SHIPS, mpParams.bBrightPlayers ? 0 : 1, KEY_S, HTX_MULTI2_BRIGHTSHIP);
	m.AddCheck ("show players names", TXT_SHOW_NAMES, mpParams.bShowAllNames, KEY_E, HTX_MULTI2_SHOWNAMES);
	m.AddCheck ("show players on map", TXT_SHOW_PLAYERS, mpParams.bShowPlayersOnAutomap, KEY_A, HTX_MULTI2_SHOWPLRS);
	if (gameStates.app.bNostalgia)
		m.AddCheck ("short packets", TXT_SHORT_PACKETS, mpParams.bShortPackets, KEY_H, HTX_MULTI2_SHORTPKTS);
	if (!gameStates.app.bGameRunning)
		m.AddCheck ("allow custom weapons", TXT_ALLOW_CUSTOM_WEAPONS, extraGameInfo [0].bAllowCustomWeapons, KEY_C, HTX_ALLOW_CUSTOM_WEAPONS);
	m.AddText ("", "");
	m.AddMenu ("allowed weapons", TXT_WAOBJECTS_MENU, KEY_O, HTX_MULTI2_OBJECTS);
	m.AddText ("", "");
	sprintf (szSocket, "%d", (gameStates.multi.nGameType == UDP_GAME) ? mpParams.udpPorts [0] + networkData.nPortOffset : networkData.nPortOffset);
	if (gameStates.multi.nGameType >= IPX_GAME) {
		m.AddText ("", TXT_SOCKET2, KEY_N);
		m.AddInput ("socket", szSocket, 5, HTX_MULTI2_SOCKET);
		}
#if 1
	m.AddText ("", "");
	sprintf (szSlider + 1, TXT_PPS2, mpParams.nMinPPS);
	strupr (szSlider + 1);
	*szSlider = *(TXT_PPS2 - 1);
	m.AddSlider ("pps", szSlider + 1, FindNearestPPS (), 0, sizeof (ppsTable) - 1, KEY_O, HTX_SPOTSIZE); 
#else
	char szPPS [20], szPPSLabel [40];
	sprintf (szPPS, "%d", mpParams.nMinPPS);
	sprintf (szPPSLabel, TXT_PPS, MIN_PPS, MAX_PPS);
	m.AddText ("", szPPSLabel, KEY_P);
	m.AddInput ("PPS", szPPS, 3, HTX_MULTI2_PPS);
#endif
	nLastScoreGoal = netGameInfo.GetScoreGoal ();
	nLastPlayTime = mpParams.nMaxTime;

doMenu:

	gameStates.app.nExtGameStatus = GAMESTAT_MORE_NETGAME_OPTIONS; 
	i = m.Menu (NULL, TXT_MORE_MPOPTIONS, NetworkMoreOptionsPoll, &choice);
	} while (i == -2);

   //mpParams."reactor life" = atoi (szInvul)*I2X (60);
mpParams.nReactorLife = m.Value ("reactor life");

if (i == m.IndexOf ("allowed weapons")) {
	NetworkSetWeaponsAllowed ();
	GetAllAllowables ();
	goto doMenu;
	}

if (!gameStates.app.bGameRunning) {
	extraGameInfo [0].bAllowCustomWeapons = uint8_t (m.Value ("allow custom weapons"));
	extraGameInfo [1].bAllowCustomWeapons = extraGameInfo [0].bAllowCustomWeapons;
	if (!extraGameInfo [0].bAllowCustomWeapons)
		SetDefaultWeaponProps ();
	}	
mpParams.bInvul = uint8_t (m.Value ("spawn invul"));
mpParams.bBrightPlayers = uint8_t (m.Value ("bright ships") ? 0 : 1);
mpParams.bShowAllNames = uint8_t (m.Value ("show player names"));
mpParams.bMarkerView = uint8_t (m.Value ("marker cameras"));
mpParams.bIndestructibleLights = uint8_t (m.Value ("indestructible lights"));
mpParams.nDifficulty = m.Value ("difficulty");
mpParams.bShowPlayersOnAutomap = m.Value ("show players on map");
if (gameStates.app.bNostalgia)
	mpParams.bShortPackets = m.Value ("short packets");
#if 1
#else
mpParams.nMinPPS = Clamp (m.ToInt ("PPS"), MIN_PPS, MAX_PPS);
#endif

if (gameStates.multi.nGameType >= IPX_GAME) { 
	int32_t newSocket = atoi (szSocket);
	if ((newSocket < -0xFFFF) || (newSocket > 0xFFFF))
		InfoBox (TXT_ERROR, NULL, BG_STANDARD, 1, TXT_OK, TXT_INV_SOCKET,
				  (gameStates.multi.nGameType == UDP_GAME) ? mpParams.udpPorts [0] + networkData.nPortOffset : networkData.nPortOffset);
	else if (newSocket != networkData.nPortOffset) {
		networkData.nPortOffset = (gameStates.multi.nGameType == UDP_GAME) ? newSocket - mpParams.udpPorts [0] : newSocket;
		IpxChangeDefaultSocket ((uint16_t) (IPX_DEFAULT_SOCKET + networkData.nPortOffset));
		}
	}

}

//------------------------------------------------------------------------------

int32_t NetworkD2XOptionsPoll (CMenu& menu, int32_t& key, int32_t nCurItem, int32_t nState)
{
if (nState)
	return nCurItem;

	CMenuItem*	m;
	int32_t			v;

if ((m = menu ["competition"])) {
	v = m->Value ();
	if (v != extraGameInfo [1].bCompetition) {
		extraGameInfo [1].bCompetition = v;
		key = -2;
		return nCurItem;
		}
	}

if (!extraGameInfo [1].bCompetition) {
	if ((m = menu ["coop penalty"])) {
		v = m->Value ();
		if (v != extraGameInfo [1].nCoopPenalty) {
			extraGameInfo [1].nCoopPenalty = v;
			sprintf (m->Text (), TXT_COOP_PENALTY, nCoopPenalties [v], '%');
			m->Rebuild ();
			return nCurItem;
			}
		}

	if ((m = menu ["target indicators"])) {
		v = m->Value ();
		if (v != (extraGameInfo [1].bTargetIndicators != 0)) {
			extraGameInfo [1].bTargetIndicators = v;
			key = -2;
			return nCurItem;
			}
		}
	}

if ((m = menu ["darkness"])) {
	v = m->Value ();
	if (v != extraGameInfo [1].bDarkness) {
		extraGameInfo [1].bDarkness = v;
		key = -2;
		return nCurItem;
		}
	}

if (extraGameInfo [1].bDarkness) {
	if ((m = menu ["allow headlights"])) {
		v = m->Value ();
		if (v == extraGameInfo [1].headlight.bAvailable) {
			extraGameInfo [1].headlight.bAvailable = !v;
			key = -2;
			return nCurItem;
			}
		}

	if (extraGameInfo [1].headlight.bAvailable && (m = menu ["spot size"])) {
		v = m->Value ();
		if (v != extraGameInfo [1].nSpotSize) {
			extraGameInfo [1].nSpotSize =
			extraGameInfo [1].nSpotStrength = v;
			sprintf (m->Text (), TXT_SPOTSIZE, GT (664 + v));
			m->Rebuild ();
			return nCurItem;
			}
		}
	}	

if ((m = menu ["smoke grenades"])) {
	v = m->Value ();
	if (extraGameInfo [0].bSmokeGrenades != v) {
		extraGameInfo [0].bSmokeGrenades = v;
		key = -2;
		return nCurItem;
		}
	}

if (extraGameInfo [0].bSmokeGrenades && (m = menu ["max. smoke grenades"])) {
	v = m->Value ();
	if (extraGameInfo [0].nMaxSmokeGrenades != v) {
		extraGameInfo [0].nMaxSmokeGrenades = v;
		sprintf (m->Text (), TXT_MAX_SMOKEGRENS, extraGameInfo [0].nMaxSmokeGrenades);
		m->Rebuild ();
		}
	}

if ((m = menu ["spawn delay"])) {
	v = m->Value () * 5;
	if (extraGameInfo [0].nSpawnDelay != v * 1000) {
		extraGameInfo [0].nSpawnDelay = v * 1000;
		sprintf (m->Text (), TXT_RESPAWN_DELAY, v);
		m->Rebuild ();
		}
	}

if (!(IsMultiGame && gameStates.app.bGameRunning) && (m = menu ["speed"])) {
	v = m->Value ();
	if (extraGameInfo [0].nSpeedScale != v) {
		extraGameInfo [0].nSpeedScale = v;
		v = 100 + v * 25;
		sprintf (m->m_text, TXT_GAME_SPEED, v / 100, v % 100);
		m->m_bRebuild = 1;
		}
	}
return nCurItem;
}

//------------------------------------------------------------------------------

void NetworkD2XOptions (void)
{
	static int32_t choice = 0;

	int32_t	i;
	char		szSlider [50];
	CMenu		m;

do {
	m.Destroy ();
	m.Create (40);
	m.AddCheck ("competition", TXT_COMPETITION_MODE, extraGameInfo [1].bCompetition, KEY_C, HTX_MULTI2_COMPETITION);
	if (!extraGameInfo [1].bCompetition) {
		m.AddCheck ("friendly fire", TXT_FRIENDLY_FIRE, extraGameInfo [0].bFriendlyFire, KEY_F, HTX_MULTI2_FFIRE);
		m.AddCheck ("inhibit suicide", TXT_NO_SUICIDE, extraGameInfo [0].bInhibitSuicide, KEY_U, HTX_MULTI2_SUICIDE);
		m.AddCheck ("mouselook", TXT_MOUSELOOK, extraGameInfo [1].bMouseLook, KEY_O, HTX_MULTI2_MOUSELOOK);
		m.AddCheck ("fast pitch", TXT_FASTPITCH, (extraGameInfo [1].bFastPitch == 1) ? 1 : 0, KEY_P, HTX_MULTI2_FASTPITCH);
		//m.AddCheck (, TXT_DUAL_LAUNCH, extraGameInfo [1].bDualMissileLaunch, KEY_M, HTX_GPLAY_DUALLAUNCH, "dual missile launch");
		}
	if (IsTeamGame) {
		m.AddCheck ("auto balance", TXT_AUTOBALANCE, extraGameInfo [0].bAutoBalanceTeams, KEY_B, HTX_MULTI2_BALANCE);
		m.AddCheck ("team doors", TXT_TEAMDOORS, mpParams.bTeamDoors, KEY_T, HTX_TEAMDOORS);
		}
	if (mpParams.nGameMode == NETGAME_CAPTURE_FLAG)
		m.AddCheck ("tow flags", TXT_TOW_FLAGS, extraGameInfo [1].bTowFlags, KEY_F, HTX_TOW_FLAGS);
	if (!extraGameInfo [1].bCompetition)
		m.AddCheck ("multiplayer cheats", TXT_MULTICHEATS, mpParams.bEnableCheats, KEY_T, HTX_MULTICHEATS);
	m.AddCheck ("cycle levels", TXT_MSN_CYCLE, extraGameInfo [1].bRotateLevels, KEY_Y, HTX_MULTI2_MSNCYCLE); 

#if 0
	mat.AddCheck (TXT_NO_REACTOR, extraGameInfo [1].bDisableReactor, KEY_R, HTX_MULTI2_NOREACTOR, "disable reactor"); 
#endif
#if UDP_SAFEMODE
	m.AddCheck (, TXT_UDP_QUAL, extraGameInfo [0].bSafeUDP, KEY_Q, HTX_MULTI2_UDPQUAL, "safe udp");
#endif

	m.AddCheck ("check ports", TXT_CHECK_PORT, extraGameInfo [0].bCheckUDPPort, KEY_P, HTX_MULTI2_CHECKPORT);
	if (extraGameInfo [1].bDarkness)
		m.AddText ("", "");
	m.AddCheck ("darkness", TXT_DARKNESS, extraGameInfo [1].bDarkness, KEY_D, HTX_DARKNESS);
	if (extraGameInfo [1].bDarkness) {
		m.AddCheck ("bright powerups", TXT_POWERUPLIGHTS, !extraGameInfo [1].bPowerupLights, KEY_P, HTX_POWERUPLIGHTS);
		m.AddCheck ("allow headlights", TXT_HEADLIGHTS, !extraGameInfo [1].headlight.bAvailable, KEY_H, HTX_HEADLIGHTS);
		if (extraGameInfo [1].headlight.bAvailable) {
			sprintf (szSlider + 1, TXT_SPOTSIZE, GT (664 + extraGameInfo [1].nSpotSize));
			strupr (szSlider + 1);
			*szSlider = *(TXT_SPOTSIZE - 1);
			m.AddSlider ("spot size", szSlider + 1, extraGameInfo [1].nSpotSize, 0, 2, KEY_O, HTX_SPOTSIZE); 
			}
		}
	if (!extraGameInfo [1].bCompetition) {
		if (mpParams.nGameMode == NETGAME_COOPERATIVE) {
			sprintf (szSlider + 1, TXT_COOP_PENALTY, nCoopPenalties [(int32_t) extraGameInfo [1].nCoopPenalty], '%');
			strupr (szSlider + 1);
			*szSlider = *(TXT_COOP_PENALTY - 1);
			m.AddSlider ("coop penalty", szSlider + 1, extraGameInfo [1].nCoopPenalty, 0, 9, KEY_O, HTX_COOP_PENALTY); 
			}
		m.AddCheck ("target indicators", TXT_MULTI_TGTIND, extraGameInfo [1].bTargetIndicators != 0, KEY_A, HTX_CPIT_TGTIND);
		}

	m.AddCheck ("smoke grenades", TXT_GPLAY_SMOKEGRENADES, extraGameInfo [0].bSmokeGrenades, KEY_S, HTX_GPLAY_SMOKEGRENADES);
	if (extraGameInfo [0].bSmokeGrenades) {
		sprintf (szSlider + 1, TXT_MAX_SMOKEGRENS, extraGameInfo [0].nMaxSmokeGrenades);
		*szSlider = *(TXT_MAX_SMOKEGRENS - 1);
		m.AddSlider ("max. smoke grenades", szSlider + 1, extraGameInfo [0].nMaxSmokeGrenades - 1, 0, 3, KEY_X, HTX_GPLAY_MAXGRENADES);
		}
	m.AddCheck ("fixed spawn points", TXT_FIXED_SPAWN, extraGameInfo [0].bFixedRespawns, KEY_F, HTX_GPLAY_FIXEDSPAWN);

	if (!COMPETITION) {
		m.AddText ("", TXT_SHIPS_ALLOWED);
		m.AddCheck ("standard ship", TXT_STANDARD_SHIP, extraGameInfo [0].shipsAllowed [0], KEY_S, HTX_PLAYERSHIP);
		m.AddCheck ("light ship", TXT_LIGHT_SHIP, extraGameInfo [0].shipsAllowed [1], KEY_I, HTX_PLAYERSHIP);
		m.AddCheck ("heavy ship", TXT_HEAVY_SHIP, extraGameInfo [0].shipsAllowed [2], KEY_F, HTX_PLAYERSHIP);
		m.AddText ("", "");
		}

	if (extraGameInfo [0].nSpawnDelay < 0)
		extraGameInfo [0].nSpawnDelay = 0;
	sprintf (szSlider + 1, TXT_RESPAWN_DELAY, extraGameInfo [0].nSpawnDelay / 1000);
	*szSlider = *(TXT_RESPAWN_DELAY - 1);
	m.AddSlider ("spawn delay", szSlider + 1, extraGameInfo [0].nSpawnDelay / 5000, 0, 12, KEY_R, HTX_GPLAY_SPAWNDELAY);
	if (!(IsMultiGame && gameStates.app.bGameRunning)) {
		int32_t v = 100 + extraGameInfo [0].nSpeedScale * 25;
		sprintf (szSlider + 1, TXT_GAME_SPEED, v / 100, v % 100);
		*szSlider = *(TXT_GAME_SPEED - 1);
		m.AddSlider ("speed", szSlider + 1, extraGameInfo [0].nSpeedScale, 0, 4, KEY_S, HTX_GAME_SPEED);
		}

	m.AddText ("", "");

	i = m.Menu (NULL, TXT_D2XOPTIONS_TITLE, NetworkD2XOptionsPoll, &choice);
  //mpParams."reactor life" = atoi (szInvul)*I2X (60);
	if (m.Available ("darkness")) {
		extraGameInfo [1].bDarkness = (uint8_t) m.Value ("darkness");
		if ((mpParams.bDarkness = extraGameInfo [1].bDarkness)) {
			extraGameInfo [1].headlight.bAvailable = !m.Value ("headlights");
			extraGameInfo [1].bPowerupLights = !m.Value ("bright powerups");
			}
		}
	if (m.Available ("standard ship")) {
		GET_VAL (extraGameInfo [0].shipsAllowed [0], "standard ship");
		GET_VAL (extraGameInfo [0].shipsAllowed [1], "light ship");
		GET_VAL (extraGameInfo [0].shipsAllowed [2], "heavy ship");
		int32_t j;
		for (j = 0; j < MAX_SHIP_TYPES; j++)
			missionConfig.m_shipsAllowed [j] = extraGameInfo [0].shipsAllowed [j];
		for (j = 0; j < MAX_SHIP_TYPES; j++)
			if (extraGameInfo [0].shipsAllowed [j])
				break;
		if (j == MAX_SHIP_TYPES)
			extraGameInfo [0].shipsAllowed [0] = 1;
		}

	GET_VAL (extraGameInfo [1].bTowFlags, "tow flags");
	GET_VAL (extraGameInfo [1].bTeamDoors, "team doors");
	mpParams.bTeamDoors = extraGameInfo [1].bTeamDoors;
	if (m.Available ("auto balance"))
		extraGameInfo [0].bAutoBalanceTeams = (m.Value ("auto balance") != 0);
#if UDP_SAFEMODE
	extraGameInfo [0].bSafeUDP = (m.Value ("safe UDP") != 0);
#endif
	extraGameInfo [0].bCheckUDPPort = (m.Value ("check ports") != 0);
	extraGameInfo [1].bRotateLevels = m.Value ("cycle levels");
	if (!COMPETITION) {
		GET_VAL (extraGameInfo [1].bEnableCheats, "multiplayer cheats");
		mpParams.bEnableCheats = extraGameInfo [1].bEnableCheats;
		GET_VAL (extraGameInfo [0].bFriendlyFire, "friendly fire");
		GET_VAL (extraGameInfo [0].bInhibitSuicide, "inhibit suicide");
		GET_VAL (extraGameInfo [1].bMouseLook, "mouselook");
		if (m.Available ("fast pitch"))
			extraGameInfo [1].bFastPitch = m.Value ("fast pitch") ? 1 : 2;
		//GET_VAL (extraGameInfo [1].bDualMissileLaunch, nDualMiss);
		GET_VAL (extraGameInfo [1].bDisableReactor, "disable reactor");
		GET_VAL (extraGameInfo [1].bTargetIndicators, "target indicators");
		GET_VAL (extraGameInfo [1].bFixedRespawns, "fixed spawn points");
		extraGameInfo [1].bDamageIndicators = extraGameInfo [1].bTargetIndicators;
		extraGameInfo [1].bMslLockIndicators = extraGameInfo [1].bTargetIndicators;
		extraGameInfo [1].bFriendlyIndicators = 1;
		extraGameInfo [1].bTagOnlyHitObjs = 1;
		}
	} while (i == -2);
}

//------------------------------------------------------------------------------

static const char* szMslNames [] = {
	"===Missiles===",
	"",
	"Concussion: %d",
	"Homing: %d",
	"Smart: %d",
	"Mega: %d",
	"Flash: %d",
	"Guided: %d",
	"Mercury: %d",
	"Earth Shaker: %d",
	"",
	"===Mines===",
	"",
	"Proximity: %d",
	"Smart: %d"
};

static int32_t powerupMap [] = {
	-1,
	-1,
	CONCUSSION_INDEX,
	HOMING_INDEX,
	SMART_INDEX,
	MEGA_INDEX,
	FLASHMSL_INDEX,
	GUIDED_INDEX,
	MERCURY_INDEX,
	EARTHSHAKER_INDEX,
	-1,
	-1,
	-1,
	PROXMINE_INDEX,
	SMARTMINE_INDEX
};

int32_t NetworkMissileOptionsPoll (CMenu& menu, int32_t& key, int32_t nCurItem, int32_t nState)
{
if (nState)
	return nCurItem;

	int32_t	h, i, v;

for (i = 0; i < int32_t (menu.ToS ()); i++) {
	if (menu [i].m_nType != NM_TYPE_TEXT) {
		h = powerupMap [i];
		v = menu [i].Value ();
		if (v != extraGameInfo [0].loadout.nMissiles [h]) {
			extraGameInfo [0].loadout.nMissiles [h] = v;
			sprintf (menu [i].Text (), szMslNames [i], v);
			menu [i].Rebuild ();
			}
		}
	}
return nCurItem;
}

//------------------------------------------------------------------------------

void MissileLoadoutMenu (void)
{
	static int32_t choice = 0;

	int32_t		h, i, j;
	char		szSlider [50];
	CMenu		m;

j = sizeofa (powerupMap);
do {
	m.Destroy ();
	m.Create (j);
	for (i = 0; i < j; i++) {
		if ((h = powerupMap [i]) < 0) {
			h = m.AddText ("", szMslNames [i]);
			m [h].m_bCentered = 1;
			}
		else {
			if (extraGameInfo [0].loadout.nMissiles [h] > nMaxSecondaryAmmo [h])
				extraGameInfo [0].loadout.nMissiles [h] = nMaxSecondaryAmmo [h];
			sprintf (szSlider, szMslNames [i], extraGameInfo [0].loadout.nMissiles [h]);
			strupr (szSlider);
			m.AddSlider ("", szSlider, extraGameInfo [0].loadout.nMissiles [h], 0, nMaxSecondaryAmmo [h], 0); 
			}
		}
	i = m.Menu (NULL, TXT_MSLLOADOUT_TITLE, NetworkMissileOptionsPoll, &choice);
	} while (i == -2);
}

//------------------------------------------------------------------------------

int32_t NetworkGetGameType (CMenu& m, int32_t bAnarchyOnly)
{
	int32_t bHoard = HoardEquipped ();
		
if (gameStates.app.bNostalgia) {
	if (m.Value ("anarchy"))
		mpParams.nGameMode = NETGAME_ANARCHY;
	else if (m.Value ("team anarchy"))
		mpParams.nGameMode = NETGAME_TEAM_ANARCHY;
	else if (m.Value ("CTF")) {
		mpParams.nGameMode = NETGAME_CAPTURE_FLAG;
		extraGameInfo [0].bEnhancedCTF = 0;
		}
	else if (m.Value ("CTF+")) {
		mpParams.nGameMode = NETGAME_CAPTURE_FLAG;
		extraGameInfo [0].bEnhancedCTF = 1;
		}
	else if (bHoard && m.Value ("hoard"))
		mpParams.nGameMode = NETGAME_HOARD;
	else if (bHoard && m.Value ("team hoard"))
		mpParams.nGameMode = NETGAME_TEAM_HOARD;
	else if (bHoard && m.Value ("entropy"))
		mpParams.nGameMode = NETGAME_ENTROPY;
	else if (bHoard && m.Value ("monsterball"))
		mpParams.nGameMode = NETGAME_MONSTERBALL;
	else if (bAnarchyOnly) {
		TextBox (NULL, BG_STANDARD, 1, TXT_OK, TXT_ANARCHY_ONLY_MISSION);
		m.SetValue ("anarchy", 1);
		m.SetValue ("robot anarchy", 0);
		m.SetValue ("coop", 0);
		return 0;
		}               
	else if (m.Value ("robot anarchy")) 
		mpParams.nGameMode = NETGAME_ROBOT_ANARCHY;
	else if (m.Value ("coop")) 
		mpParams.nGameMode = NETGAME_COOPERATIVE;
	}
else {
	if (mpParams.nGameType <= NETGAME_CAPTURE_FLAG) {
		mpParams.nGameMode = mpParams.nGameType;
		if (mpParams.nGameMode == NETGAME_CAPTURE_FLAG)
			extraGameInfo [0].bEnhancedCTF = 0;
		}
	else {
		mpParams.nGameMode = mpParams.nGameType - 1;
		if (mpParams.nGameMode == NETGAME_CAPTURE_FLAG)
			extraGameInfo [0].bEnhancedCTF = 1;
		}
	}
return 1;
}

//------------------------------------------------------------------------------

void BuildGameParamsMenu (CMenu& m, char* szName, char* szLevelText, char* szLevel, char* szIpAddr, char* szMaxPlayers, int32_t nNewMission)
{
	int32_t bHoard = HoardEquipped ();

sprintf (szLevel, "%d", mpParams.nLevel);

m.Destroy ();
m.Create (35);
if (gameStates.multi.nGameType == UDP_GAME) {
	if (UDPGetMyAddress () < 0) 
		strcpy (szIpAddr, TXT_IP_FAIL);
	else {
		sprintf (szIpAddr, "Game Host: %d.%d.%d.%d:%d", 
					ipx_MyAddress.m_address.node.portAddress.ip.octets [0], 
					ipx_MyAddress.m_address.node.portAddress.ip.octets [1], 
					ipx_MyAddress.m_address.node.portAddress.ip.octets [2], 
					ipx_MyAddress.m_address.node.portAddress.ip.octets [3], 
					mpParams.udpPorts [0]);
		}
	}
m.AddText ("", TXT_DESCRIPTION, 0); 
m.AddInput ("game name", szName, NETGAME_NAME_LEN, HTX_MULTI_NAME); 
if (!gameStates.app.bNostalgia)
	m.AddText ("", "");
m.AddMenu ("mission selector", TXT_SEL_MISSION, KEY_I, HTX_MULTI_MISSION);
if (gameStates.app.bNostalgia) {
	m.AddText ("mission name", "", 0);
	m ["mission name"]->Rebuild (); 
	pszMissionName = "mission name";
	}
else {
	m ["mission selector"]->Rebuild (); 
	pszMissionName = "mission selector";
	}
if ((nNewMission >= 0) && (missionManager.nLastLevel > 1)) {
	if (gameStates.app.bNostalgia) {
		m.AddText ("level number text", szLevelText, 0); 
		m.AddInput ("level number", szLevel, 4, HTX_MULTI_LEVEL);
		}
	else {
		sprintf (szLevelText, "%s %d", TXT_LEVEL_, NMCLAMP (mpParams.nLevel, 1, missionManager.nLastLevel));
		m.AddSlider ("level number", szLevelText, NMCLAMP (mpParams.nLevel, 1, missionManager.nLastLevel) - 1, 0, missionManager.nLastLevel - 1);
		}
	}
m.AddText ("", ""); 
nGameTypes = m.ToS ();
if (gameStates.app.bNostalgia) {
	m.AddRadio ("anarchy", TXT_ANARCHY, 0, KEY_A, HTX_MULTI_ANARCHY);
	m.AddRadio ("team anarchy", TXT_TEAM_ANARCHY, 0, KEY_T, HTX_MULTI_TEAMANA);
	m.AddRadio ("robot anarchy", TXT_ROBOT_ANARCHY, 0, KEY_R, HTX_MULTI_BOTANA);
	m.AddRadio ("coop", TXT_COOP, 0, KEY_P, HTX_MULTI_COOP);
	m.AddRadio ("CTF", TXT_CTF, 0, KEY_F, HTX_MULTI_CTF);
#if 0
	if (!gameStates.app.bNostalgia)
		m.AddRadio ("CTF+", TXT_CTF_PLUS, 0, KEY_T, HTX_MULTI_CTFPLUS);
#endif
	if (bHoard) {
		m.AddRadio ("hoard", TXT_HOARD, 0, KEY_H, HTX_MULTI_HOARD);
		m.AddRadio ("team hoard", TXT_TEAM_HOARD, 0, KEY_O, HTX_MULTI_TEAMHOARD);
#if 0
		if (!gameStates.app.bNostalgia) {
			m.AddRadio ("entropy", TXT_ENTROPY, 0, KEY_Y, HTX_MULTI_ENTROPY);
			m.AddRadio ("monsterball", TXT_MONSTERBALL, 0, KEY_B, HTX_MULTI_MONSTERBALL);
			}
#endif
		} 
	nGameTypes = m.ToS () - nGameTypes;
	m [m.IndexOf ("anarchy") + NMCLAMP (mpParams.nGameType, 0, nGameTypes)].Value () = 1;
	m.AddText ("end of game types", ""); 
	m.AddRadio ("open game", TXT_OPEN_GAME, 0, KEY_O, HTX_MULTI_OPENGAME);
	m.AddRadio ("closed game", TXT_CLOSED_GAME, 0, KEY_C, HTX_MULTI_CLOSEDGAME);
	m.AddRadio ("restricted game", TXT_RESTRICTED_GAME, 0, KEY_R, HTX_MULTI_RESTRGAME);
	m [m.IndexOf ("open game") + NMCLAMP (mpParams.nGameAccess, 0, 2)].Value () = 1;
	m.AddText ("end of game access", "");
	}
else {
	szGameTypeList [0] = TXT_ANARCHY;
	szGameTypeList [1] = TXT_TEAM_ANARCHY; 
	szGameTypeList [2] = TXT_ROBOT_ANARCHY; 
	szGameTypeList [3] = TXT_COOP; 
	szGameTypeList [4] = TXT_CTF; 
	szGameTypeList [5] = TXT_CTF_PLUS; 
	szGameTypeList [6] = TXT_HOARD; 
	szGameTypeList [7] = TXT_TEAM_HOARD; 
	szGameTypeList [8] = TXT_ENTROPY; 
	szGameTypeList [9] = TXT_MONSTERBALL;
	szGameAccessList [0] = TXT_ACCESS_ALL;
	szGameAccessList [1] = TXT_ACCESS_INVITE;
	szGameAccessList [2] = TXT_ACCESS_CONFIRM;

	char szSlider [50];

	nGameTypes = sizeofa (szGameTypeList) - (bHoard ? 1 : 5);
	sprintf (szSlider + 1, TXT_GAME_TYPE, szGameTypeList [mpParams.nGameType]);
	*szSlider = *(TXT_GAME_TYPE - 1);
	m.AddSlider ("game type", szSlider + 1, NMCLAMP (mpParams.nGameType, 0, nGameTypes), 0, nGameTypes, KEY_T, HTX_GAME_TYPE);
	sprintf (szSlider + 1, TXT_GAME_ACCESS, szGameAccessList [mpParams.nGameAccess]);
	*szSlider = *(TXT_GAME_ACCESS - 1);
	m.AddSlider ("game access", szSlider + 1, NMCLAMP (mpParams.nGameAccess, 0, 2), 0, 2, KEY_A, HTX_GAME_ACCESS);
	}

sprintf (szMaxPlayers + 1, TXT_MAX_PLAYERS, gameData.multiplayer.nMaxPlayers);
*szMaxPlayers = *(TXT_MAX_PLAYERS - 1);
nLastMaxPlayers = gameData.multiplayer.nMaxPlayers - 2;
m.AddSlider ("max. players", szMaxPlayers + 1, nLastMaxPlayers, 0, nLastMaxPlayers, KEY_P, HTX_MULTI_MAXPLRS); 
m.AddText ("", "");
m.AddMenu ("more options", TXT_MORE_OPTS, KEY_M, HTX_MULTI_MOREOPTS);
if (!gameStates.app.bNostalgia) {
	m.AddMenu ("d2x options", TXT_MULTI_D2X_OPTS, KEY_X, HTX_MULTI_D2XOPTS);
	if (mpParams.nGameType == 8)
		m.AddMenu ("entropy options", TXT_ENTROPY_OPTS, KEY_E, HTX_MULTI_ENTOPTS);
	if ((gameOpts->app.bExpertMode == SUPERUSER) && (mpParams.nGameType == 9))
		m.AddMenu ("monsterball options", TXT_MONSTERBALL_OPTS, KEY_B, HTX_MULTI_MBALLOPTS);
	else
		InitMonsterballSettings (&extraGameInfo [0].monsterball);
	m.AddMenu ("config options", TXT_GAME_OPTIONS, KEY_O, HTX_MAIN_CONF);
	m.AddMenu ("loadout options", TXT_LOADOUT_OPTION, KEY_I, HTX_MULTI_LOADOUT);
	m.AddMenu ("missile options", TXT_MISSILE_LOADOUT, KEY_C, HTX_MISSILE_LOADOUT);
	}
}

//------------------------------------------------------------------------------

int32_t GameParamsMenu (CMenu& m, int32_t& key, int32_t& choice, char* szName, char* szLevelText, char* szLevel, char* szIpAddr, int32_t& nNewMission)
{
	int32_t i, bAnarchyOnly = (nNewMission < 0) ? 0 : missionManager [nNewMission].bAnarchyOnly;

if (m [pszMissionName]->Rebuilding ()) {
	int32_t nOffset = 0;
	if ((nNewMission >= 0) &&
		 (strstr (missionManager [nNewMission].szMissionName, "(D1) ") || strstr (missionManager [nNewMission].szMissionName, "(D2) ") || strstr (missionManager [nNewMission].szMissionName, "(XL) ")))
		nOffset = 5;
	strncpy (netGameInfo.m_info.szMissionName, 
				(nNewMission < 0) ? "" : missionManager [nNewMission].filename, 
				sizeof (netGameInfo.m_info.szMissionName) - 1);
	if (gameStates.app.bNostalgia) {
		m ["mission name"]->SetText ((nNewMission < 0) ? const_cast<char*> (TXT_NONE_SELECTED) : const_cast<char*> (missionManager [nNewMission].szMissionName + nOffset));
		if (gameStates.app.bNostalgia && (nNewMission >= 0) && (missionManager.nLastLevel > 1)) {
			sprintf (szLevelText, "%s (1-%d)", TXT_LEVEL_, missionManager.nLastLevel);
			if (strlen (szLevelText) < 32)
				szLevelText [31] = '\0';
			strcpy (m ["level number text"]->Text (), szLevelText);
			m ["level number text"]->Rebuild ();
			}
		mpParams.nLevel = 1;
		}
	else {
		char szLabel [200];
		sprintf (szLabel, "%s %s", TXT_SEL_MISSION, (nNewMission < 0) ? const_cast<char*> (TXT_NONE_SELECTED) : const_cast<char*> (missionManager [nNewMission].szMissionName + nOffset));
		CMenuItem* mi = m ["mission selector"];
		m ["mission selector"]->SetText (szLabel);
		}
	mpParams.nLevel = 1;
	}

if (!gameStates.app.bNostalgia && (nNewMission >= 0) && !m.Available ("launch game")) {
	m.AddText ("", "");
	m.AddMenu ("launch game", TXT_LAUNCH_GAME, KEY_L, "");
	m ["launch game"]->m_bCentered = 1;
	}

gameStates.app.nExtGameStatus = GAMESTAT_NETGAME_OPTIONS; 
key = m.Menu (NULL, (gameStates.multi.nGameType == UDP_GAME) ? szIpAddr : NULL, NetworkGameParamPoll, &choice);
								//TXT_NETGAME_SETUP
if (key == -1)
	return -1;
else if (m.Available ("launch game") && (choice == m.IndexOf ("launch game")))
	key = 0;
else if (choice == m.IndexOf ("more options")) {
	if (m [nGameTypes + 3].Value ())
		gameData.app.SetGameMode (GM_MULTI_COOP);
	NetworkMoreGameOptions ();
	gameData.app.SetGameMode (0);
	if (gameStates.multi.nGameType == UDP_GAME) {
		sprintf (szIpAddr, "Game Host: %d.%d.%d.%d:%d", 
					ipx_MyAddress.m_address.node.portAddress.ip.octets [0], 
					ipx_MyAddress.m_address.node.portAddress.ip.octets [1], 
					ipx_MyAddress.m_address.node.portAddress.ip.octets [2], 
					ipx_MyAddress.m_address.node.portAddress.ip.octets [3], 
					mpParams.udpPorts [0]);
		}
	return 1;
	}
else if (!gameStates.app.bNostalgia && (choice == m.IndexOf ("d2x options"))) {
	NetworkGetGameType (m, bAnarchyOnly);
	NetworkD2XOptions ();
	return 1;
	}
else if (!gameStates.app.bNostalgia && (choice == m.IndexOf ("entropy options"))) {
	NetworkEntropyOptions ();
	return 1;
	}
else if (!gameStates.app.bNostalgia && (choice == m.IndexOf ("monsterball options"))) {
	NetworkMonsterballOptions ();
	return 1;
	}
else if (!gameStates.app.bNostalgia && (choice == m.IndexOf ("config options"))) {
	ConfigMenu ();
	return 1;
	}
else if (!gameStates.app.bNostalgia && (choice == m.IndexOf ("loadout options"))) {
	LoadoutOptionsMenu ();
	return 1;
	}
else if (!gameStates.app.bNostalgia && (choice == m.IndexOf ("missile options"))) {
	MissileLoadoutMenu ();
	return 1;
	}
else if (choice == m.IndexOf ("mission selector")) {
	int32_t h = SelectAndLoadMission (1, &bAnarchyOnly);
	if (h < 0)
		return 1;
	missionManager.nLastMission = nNewMission = h;
	m [pszMissionName]->Rebuild ();
	mpParams.nLevel = 1;
	return 2;
	}

if (key != -1) {
	int32_t h, j;
		   
	gameData.multiplayer.nMaxPlayers = m.Value ("max. players") + 2;
	netGameInfo.m_info.nMaxPlayers = gameData.multiplayer.nMaxPlayers;
			
	for (j = 0; j < networkData.nActiveGames; j++)
		if (!stricmp (activeNetGames [j].m_info.szGameName, szName)) {
			InfoBox (TXT_ERROR, NULL, BG_STANDARD, 1, TXT_OK, TXT_DUPLICATE_NAME);
			return 1;
		}
	strncpy (mpParams.szGameName, szName, sizeof (mpParams.szGameName));
	mpParams.nLevel = atoi (szLevel);
	if ((missionManager.nLastLevel > 0) && ((mpParams.nLevel < 1) || (mpParams.nLevel > missionManager.nLastLevel))) {
		InfoBox (TXT_ERROR, NULL, BG_STANDARD, 1, TXT_OK, TXT_LEVEL_OUT_RANGE);
		sprintf (szLevel, "1");
		return 1;
	}

	for (h = i = m.IndexOf ("anarchy"), j = m.IndexOf ("end of game types"); i < j; i++)
		if (m [i].Value ()) {
			mpParams.nGameType = i - h;
			break;
			}

	for (h = i = m.IndexOf ("open game"), j = m.IndexOf ("end of game access"); i < j; i++)
		if (m [i].Value ()) {
			mpParams.nGameAccess = i - h;
			break;
			}

	if (!NetworkGetGameType (m, bAnarchyOnly))
		return 1;
	if (m.Value ("closed game"))
		netGameInfo.m_info.gameFlags |= NETGAME_FLAG_CLOSED;
	netGameInfo.m_info.bRefusePlayers = m.Value ("restricted game");
	}
NetworkSetGameMode (mpParams.nGameMode);
if (key == -2)
	return 2;
return 0;
}

//------------------------------------------------------------------------------

int32_t NetworkGetGameParams (int32_t bAutoRun)
{
	int32_t	i, key, choice = 2, nState = 0;
	CMenu	m;
	char	szName [80]; //NETGAME_NAME_LEN+1];
	char	szLevelText [80]; //32];
	char	szMaxPlayers [50];
	char	szIpAddr [80];
	char	szLevel [10]; //5];

	int32_t nNewMission = missionManager.nLastMission;

gameOpts->app.bSinglePlayer = 0;
SetAllAllowablesTo (mpParams.nWeaponFilter);
networkData.nNamesInfoSecurity = -1;

for (i = 0; i < MAX_PLAYERS; i++)
	if (i != N_LOCALPLAYER)
		gameData.multiplayer.players [i].callsign [0] = 0;

gameData.multiplayer.nMaxPlayers = MAX_NUM_NET_PLAYERS;
if (!(FindArg ("-pps") && FindArg ("-shortpackets")))
	if (!NetworkChooseConnect ())
		return -1;

sprintf (szName, "%s%s", LOCALPLAYER.callsign, TXT_S_GAME);
if (bAutoRun)
	return 1;

nGameItem = -1;

*szLevelText = 
*szMaxPlayers =
*szIpAddr =
*szLevel = '\0';

do {
	PrintLog (1, "building game parameters menu\n");
	BuildGameParamsMenu (m, szName, szLevelText, szLevel, szIpAddr, szMaxPlayers, nNewMission);
	PrintLog (-1);
	PrintLog (1, "loading game parameters menu\n");
	do {
		nState = GameParamsMenu (m, key, choice, szName, szLevelText, szLevel, szIpAddr, nNewMission);
		if ((nNewMission < 0) && (nState == 0)) {
			InfoBox (TXT_ERROR, NULL, BG_STANDARD, 1, TXT_OK, "Please chose a mission");
			nState = 1;
			}
		} while (nState == 1);
	PrintLog (-1);
	} while (nState == 2);

if (nState < 0)
	return -1;

if (gameStates.app.bNostalgia) {
	extraGameInfo [1].bDarkness = 0;
	extraGameInfo [1].bTowFlags = 0;
	extraGameInfo [1].bTeamDoors = 0;
	mpParams.bTeamDoors = extraGameInfo [1].bTeamDoors;
	extraGameInfo [1].bEnableCheats = 0;
	mpParams.bEnableCheats = extraGameInfo [1].bEnableCheats;
	extraGameInfo [0].bFriendlyFire = 1;
	extraGameInfo [0].bInhibitSuicide = 0;
	extraGameInfo [0].bAutoBalanceTeams = 0;
#if UDP_SAFEMODE
	extraGameInfo [0].bSafeUDP = (mat [nSafeUDP) != 0);
#endif
	extraGameInfo [1].bMouseLook = 0;
	extraGameInfo [1].bFastPitch = 2;
	extraGameInfo [1].bDualMissileLaunch = 0;
	extraGameInfo [1].bRotateLevels = 0;
	extraGameInfo [1].bDisableReactor = 0;
	extraGameInfo [1].bTargetIndicators = 0;
	extraGameInfo [1].bFriendlyIndicators = 0;
	extraGameInfo [1].bDamageIndicators = 0;
	extraGameInfo [1].bMslLockIndicators = 0;
	extraGameInfo [1].bTagOnlyHitObjs = 0;
	}
netGameInfo.m_info.szMissionName [sizeof (netGameInfo.m_info.szMissionName) - 1] = '\0';
strncpy (netGameInfo.m_info.szMissionTitle, missionManager [nNewMission].szMissionName + (gameOpts->menus.bShowLevelVersion ? 4 : 0), sizeof (netGameInfo.m_info.szMissionTitle));
netGameInfo.m_info.szMissionTitle [sizeof (netGameInfo.m_info.szMissionTitle) - 1] = '\0';
netGameInfo.SetControlInvulTime (mpParams.nReactorLife * 5 * I2X (60));
netGameInfo.SetPlayTimeAllowed (mpParams.nMaxTime);
netGameInfo.SetScoreGoal (mpParams.nScoreGoal * 5);
netGameInfo.SetMinPPS (mpParams.nMinPPS);
netGameInfo.m_info.invul = mpParams.bInvul;
netGameInfo.m_info.BrightPlayers = mpParams.bBrightPlayers;
netGameInfo.SetShortPackets (mpParams.bShortPackets);
netGameInfo.m_info.bAllowMarkerView = mpParams.bMarkerView;
netGameInfo.m_info.bIndestructibleLights = mpParams.bIndestructibleLights; 
if (mpParams.bShowPlayersOnAutomap)
	netGameInfo.m_info.gameFlags |= NETGAME_FLAG_SHOW_MAP;
else
	netGameInfo.m_info.gameFlags &= ~NETGAME_FLAG_SHOW_MAP;
gameStates.app.nDifficultyLevel = mpParams.nDifficulty;
IpxChangeDefaultSocket ((uint16_t) (IPX_DEFAULT_SOCKET + networkData.nPortOffset));
return key;
}

//------------------------------------------------------------------------------

static time_t	nQueryTimeout;

static int32_t QueryPoll (CMenu& menu, int32_t& key, int32_t nCurItem, int32_t nState)
{
if (nState)
	return nCurItem;

	time_t t;

if (NetworkListen () && (networkData.nActiveGames >= MAX_ACTIVE_NETGAMES))
	key = -2;
else if (key == KEY_ESC)
	key = -3;
else if ((t = SDL_GetTicks () - nQueryTimeout) > 5000)
	key = -4;
else {
	int32_t v = (int32_t) (t / 5);
	if (menu [0].Value () != v) {
		menu [0].Value () = v;
		menu [0].Rebuild ();
		}
	key = 0;
	}
return nCurItem;
}

//------------------------------------------------------------------------------

int32_t NetworkFindGame (void)
{
	CMenu	m (3);
	int32_t	i;

if (gameStates.multi.nGameType > IPX_GAME)
	return 0;

networkData.nStatus = NETSTAT_BROWSING;
networkData.nActiveGames = 0;
NetworkSendGameListRequest ();

m.AddGauge ("", "                    ", -1, 1000); 
m.AddText ("", "");
m.AddText ("", TXT_PRESS_ESC);
m [2].m_bCentered = 1;
nQueryTimeout = SDL_GetTicks ();
do {
	i = m.Menu (NULL, TXT_NET_SEARCH, QueryPoll, NULL);
	} while (i >= 0);
return (networkData.nActiveGames >= MAX_ACTIVE_NETGAMES);
}

//------------------------------------------------------------------------------

int32_t ConnectionPacketLevel [] = {0, 1, 1, 1};
int32_t ConnectionSecLevel [] = {12, 3, 5, 7};

int32_t AppletalkConnectionPacketLevel [] = {0, 1, 0};
int32_t AppletalkConnectionSecLevel [] = {10, 3, 8};

#if !DBG

int32_t NetworkChooseConnect ()
{
return 1;
}

#else

int32_t NetworkChooseConnect (void)
{
if (gameStates.multi.nGameType >= IPX_GAME) 
   return 1;
return 0;
}
#endif

//------------------------------------------------------------------------------

int32_t stoport (char *szPort, int32_t *pPort, int32_t *pSign)
{
	int32_t	h, sign;

if (*szPort == '+')
	sign = 1;
else if (*szPort == '-')
	sign = -1;
else
	sign = 0;
if (sign && !pSign)
	return 0;
h = atol (szPort + (pSign && *pSign != 0));
*pPort = sign ? UDP_BASEPORT + sign * h : h;
if (pSign)
	*pSign = sign;
return 1;
}

//------------------------------------------------------------------------------

int32_t stoip (char *szServerIpAddr, uint8_t *ipAddrP, int32_t* portP)
{
	char	*pi, *pj, *pFields [5], tmp [22];
	int32_t	h, i, j;

memset (pFields, 0, sizeof (pFields));
memcpy (tmp, szServerIpAddr, sizeof (tmp));

for (pi = pj = tmp, i = 0;;) {
	if (!*pi) {
		if (i < 3)
			return 0;
		pFields [i++] = pj;
		break;
		}
	else if (*pi == '.') {
		if (i > 3)
			return 0;
		pFields [i++] = pj;
		}
	else if ((*pi == ':') || (*pi == '-')) {
		if (i != 3)
			return 0;
		pFields [i++] = pj;
		}
	else {
		pi++;
		continue;
		}
	* (pi++) = '\0';
	pj = pi;
	}
if (i < 3)
	return 0;
for (j = 0; j < i; j++) {
	if (!pFields [j])
		return 0;
	if (j == 4)
		return stoport (pFields [j], portP ? portP : mpParams.udpPorts, NULL); 
	else {
		h = atol (pFields [j]);
		if ((h < 0) || (h > 255))
			return 0;
		ipAddrP [j] = (uint8_t) h;
		}
	}
return 1;
}

//------------------------------------------------------------------------------

int32_t IpAddrMenuCallBack (CMenu& menu, int32_t& key, int32_t nCurItem, int32_t nState)
{
return nCurItem;
}

//------------------------------------------------------------------------------

int32_t NetworkGetIpAddr (bool bServer, bool bUDP)
{
	CMenu	m (9);
	int32_t	h, i, j, choice = 0;
	bool	bError;
	char	szId [100];

	static char szPort [2][7] = {{'\0','\0','\0','\0','\0','\0','\0'}, {'\0','\0','\0','\0','\0','\0','\0'}};
	static int32_t nSign = 0;

if (bUDP && !tracker.m_bUse) {
	if (!*mpParams.szServerIpAddr) {
		ArchIpxSetDriver (IPX_DRIVER_UDP);
		if (IpxInit (-1) != IPX_INIT_OK) {
			strcpy (mpParams.szServerIpAddr, "0.0.0.0");
			}
		else {
			sprintf (mpParams.szServerIpAddr, "%d.%d.%d.%d", 
						networkData.serverAddress.m_address.node.portAddress.ip.octets [0],
						networkData.serverAddress.m_address.node.portAddress.ip.octets [1],
						networkData.serverAddress.m_address.node.portAddress.ip.octets [2],
						networkData.serverAddress.m_address.node.portAddress.ip.octets [3]);
			sprintf (szPort [1], "%s%d", 
						 (nSign < 0) ? "-" : (nSign > 0) ? "+" : "", mpParams.udpPorts [1]);
			IpxClose ();
			}
		}
	m.AddText (TXT_HOST_IP, 0);
	m.AddInput ("ip address", mpParams.szServerIpAddr, sizeof (mpParams.szServerIpAddr) - 1, HTX_GETIP_SERVER);
	j = 0;
	h = bServer ? 1 : 2;
	}
else if (bServer) {
	j = 0;
	h = 1;
	}
else {
	j = 0;
	h = 2;
	}
for (i = j; i < h; i++) {
	if ((mpParams.udpPorts [i] < 0) || (mpParams.udpPorts [i] > 65535))
		mpParams.udpPorts [i] = UDP_BASEPORT;
	sprintf (szPort [i], "%u", mpParams.udpPorts [i]);
	m.AddText ("", GT (1100 + i));
	sprintf (szId, "client addr %d", i);
	m.AddInput (szId, szPort [i], sizeof (szPort [i]) - 1, HTX_GETIP_CLIENT);
	}

m.AddText ("", TXT_PORT_HELP1, 0);
m.AddText ("", TXT_PORT_HELP2, 0);
m.AddText ("", "");
m.AddCheck ("check ports", TXT_CHECK_PORT, extraGameInfo [0].bCheckUDPPort, KEY_P, HTX_MULTI2_CHECKPORT);
for (;;) {
	i = m.Menu (NULL, tracker.m_bUse ? TXT_NETWORK_ADDRESSES : TXT_IP_HEADER, &IpAddrMenuCallBack, &choice);
	if (i < 0)
		break;
	if (i >= int32_t (m.ToS ()))
		continue;
	bError = false;
	extraGameInfo [0].bCheckUDPPort = m.Value ("check ports") != 0;
	for (i = j; i < h; i++) { 
		stoport (szPort [i], &mpParams.udpPorts [i], &nSign);
		if (extraGameInfo [0].bCheckUDPPort && !mpParams.udpPorts [i])
			bError = true;
		}
	if (!tracker.m_bUse && !stoip (mpParams.szServerIpAddr, networkData.serverAddress.Server ()))
		bError =  true;
	if (!bError)
		return 1;
	TextBox (NULL, BG_STANDARD, 1, TXT_OK, TXT_IP_INVALID);
	}
return 0;
} 

//------------------------------------------------------------------------------
/*
 * IpxSetDriver was called do_network_init and located in main/inferno
 * before the change which allows the user to choose the network driver
 * from the game menu instead of having to supply command line args.
 */
void IpxSetDriver (int32_t ipx_driver)
{
	IpxClose ();

	int32_t nIpxError;
	int32_t socket = 0, t;

if ((t = FindArg ("-socket")))
	socket = atoi (appConfig [t + 1]);
ArchIpxSetDriver (ipx_driver);
if ((nIpxError = IpxInit (IPX_DEFAULT_SOCKET + socket)) == IPX_INIT_OK)
	networkData.bActive = 1;
else {
#if 1 //TRACE
switch (nIpxError) {
	case IPX_NOT_INSTALLED: 
		console.printf (CON_VERBOSE, "%s\n", TXT_NO_NETWORK); 
		break;
	case IPX_SOCKET_TABLE_FULL: 
		console.printf (CON_VERBOSE, "%s 0x%x.\n", TXT_SOCKET_ERROR, IPX_DEFAULT_SOCKET + socket); 
		break;
	case IPX_NO_LOW_DOS_MEM: 
		console.printf (CON_VERBOSE, "%s\n", TXT_MEMORY_IPX); 
		break;
	default: 
		console.printf (CON_VERBOSE, "%s %d", TXT_ERROR_IPX, nIpxError);
	}
	console.printf (CON_VERBOSE, "%s\n", TXT_NETWORK_DISABLED);
#endif
	networkData.bActive = 0;		// Assume no network
	}
if (gameStates.multi.nGameType != UDP_GAME) {
	IpxReadUserFile ("descent.usr");
	IpxReadNetworkFile ("descent.net");
	}
}

//------------------------------------------------------------------------------

void DoNewIPAddress (void)
{
  CMenu	m (2);
  char	szIP [30];
  int32_t		choice;

m.AddText ("Enter an address or hostname:", 0);
m.AddInput ("ip address", szIP, 50, NULL);
choice = m.Menu (NULL, TXT_JOIN_TCP);
if ((choice == -1) || !*m [1].Text ())
	return;
InfoBox (TXT_SORRY, NULL, BG_STANDARD, 1, TXT_OK, TXT_INV_ADDRESS);
}

//------------------------------------------------------------------------------

