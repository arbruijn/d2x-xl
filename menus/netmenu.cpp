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

void SetFunctionMode (int);
extern ubyte ipx_MyAddress [10];

//------------------------------------------------------------------------------

void NetworkSetWeaponsAllowed (void)
{
CMenu	m (40);

m.AddCheck (TXT_WA_LASER, netGame.m_info.DoLaserUpgrade, 0, NULL, "laser upgrade");
m.AddCheck (TXT_WA_SLASER, netGame.m_info.DoSuperLaser, 0, NULL, "super laser");
m.AddCheck (TXT_WA_QLASER, netGame.m_info.DoQuadLasers, 0, NULL, "quad lasers");
m.AddCheck (TXT_WA_VULCAN, netGame.m_info.DoVulcan, 0, NULL, "vulcan");
m.AddCheck (TXT_WA_SPREAD, netGame.m_info.DoSpread, 0, NULL, "spreadfire");
m.AddCheck (TXT_WA_PLASMA, netGame.m_info.DoPlasma, 0, NULL, "plasma");
m.AddCheck (TXT_WA_FUSION, netGame.m_info.DoFusions, 0, NULL, "fusion");
m.AddCheck (TXT_WA_GAUSS, netGame.m_info.DoGauss, 0, NULL, "gauss");
m.AddCheck (TXT_WA_HELIX, netGame.m_info.DoHelix, 0, NULL, "helix");
m.AddCheck (TXT_WA_PHOENIX, netGame.m_info.DoPhoenix, 0, NULL, "phoenix");
m.AddCheck (TXT_WA_OMEGA, netGame.m_info.DoOmega, 0, NULL, "omage");

m.AddCheck (TXT_WA_HOMING_MSL, netGame.m_info.DoHoming, 0, NULL, "homingmsl");
m.AddCheck (TXT_WA_PROXBOMB, netGame.m_info.DoProximity, 0, NULL, "proxmine");
m.AddCheck (TXT_WA_SMART_MSL, netGame.m_info.DoSmarts, 0, NULL, "smartmsl");
m.AddCheck (TXT_WA_MEGA_MSL, netGame.m_info.DoMegas, 0, NULL, "megamsl");
m.AddCheck (TXT_WA_FLASH_MSL, netGame.m_info.DoFlash, 0, NULL, "flashmsl");
m.AddCheck (TXT_WA_GUIDED_MSL, netGame.m_info.DoGuided, 0, NULL, "guidedmsl");
m.AddCheck (TXT_WA_SMARTMINE, netGame.m_info.DoSmartMine, 0, NULL, "smartmine");
m.AddCheck (TXT_WA_MERC_MSL, netGame.m_info.DoMercury, 0, NULL, "mercurymsl");
m.AddCheck (TXT_WA_SHAKER_MSL, netGame.m_info.DoEarthShaker, 0, NULL, "eshakermsl");

m.AddCheck (TXT_WA_INVUL, netGame.m_info.DoInvulnerability, 0, NULL, "invul");
m.AddCheck (TXT_WA_CLOAK, netGame.m_info.DoCloak, 0, NULL, "cloak");
m.AddCheck (TXT_WA_BURNER, netGame.m_info.DoAfterburner, 0, NULL, "afterburner");
m.AddCheck (TXT_WA_AMMORACK, netGame.m_info.DoAmmoRack, 0, NULL, "ammorack");
m.AddCheck (TXT_WA_CONVERTER, netGame.m_info.DoConverter, 0, NULL, "converter");
m.AddCheck (TXT_WA_HEADLIGHT, netGame.m_info.DoHeadlight, 0, NULL, "headlight");

m.Menu (NULL, TXT_WA_OBJECTS);

netGame.m_info.DoLaserUpgrade = m.Value ("laser upgrade"); 
netGame.m_info.DoSuperLaser = m.Value ("super laser");
netGame.m_info.DoQuadLasers = m.Value ("quad lasers");  
netGame.m_info.DoVulcan = m.Value ("vulcan");
netGame.m_info.DoSpread = m.Value ("spreadfire");
netGame.m_info.DoPlasma = m.Value ("plasma");
netGame.m_info.DoFusions = m.Value ("fusion");
netGame.m_info.DoGauss = m.Value ("gauss");
netGame.m_info.DoHelix = m.Value ("helix");
netGame.m_info.DoPhoenix = m.Value ("phoenix");
netGame.m_info.DoOmega = m.Value ("omega");

netGame.m_info.DoHoming = m.Value ("homingmsl");
netGame.m_info.DoProximity = m.Value ("proxmine");
netGame.m_info.DoSmarts = m.Value ("smartmsl");
netGame.m_info.DoMegas = m.Value ("megamsl");
netGame.m_info.DoFlash = m.Value ("flashmsl");
netGame.m_info.DoGuided = m.Value ("guidedmsl");
netGame.m_info.DoSmartMine = m.Value ("smartmine");
netGame.m_info.DoMercury = m.Value ("mercuymsl");
netGame.m_info.DoEarthShaker = m.Value ("eshakermsl");

netGame.m_info.DoInvulnerability = m.Value ("invul");
netGame.m_info.DoCloak = m.Value ("cloak");
netGame.m_info.DoAfterburner = m.Value ("afterburner");
netGame.m_info.DoAmmoRack = m.Value ("ammorack");
netGame.m_info.DoConverter = m.Value ("converter");     
netGame.m_info.DoHeadlight = m.Value ("headlight");     
}

//------------------------------------------------------------------------------

fix nLastPlayTime = -1;
int nLastReactorLife = 0;
int nLastScoreGoal = -1;
int nLastMaxPlayers = -1;

//static struct {
//	int nSuicide, nFriendlyFire;
//	int "reactor life", nAnarchy, nTeamAnarchy, nRobotAnarchy, nCoop;
//	int nPlayersOnMap;
//	int nEntOpts, nMBallOpts;
//	int "max. players";
//	int nGameTypes, nCTF, nEnhancedCTF, nHoard, nTeamHoard;
//	int nOpenGame, nClosedGame, nRestrictedGame;
//	int nSetPower, networkMenuOpts."play time", "score goal", nSocket, nMarkerView, nLight;
//	int nDifficulty, nPPS, nShortPkts, nBrightPlayers, nStartInvul;
//	int nDarkness, nTeamDoors, nMultiCheats, nTgtInd;
//	int nHeadlights, nPowerupLights, nSpotSize, nSmokeGrenades, nMaxSmokeGrens;
//	int nShowNames, nAutoTeams, nRotateLevels, nDisableReactor;
//	int nMouseLook, nFastPitch, nSafeUDP, nTowFlags, nCompetition, nPenalty;
//	int nFixedSpawn, nSpawnDelay;
//	int nGameName, nLevel, nLevelText, nMoreOpts, nMission, nMissionName, nD2XOpts, nConfigMenu, nLoadoutMenu, nMissileMenu;
//	//int nDualMiss;
//} networkMenuOpts;

//------------------------------------------------------------------------------


#define	WF(_f)	 ((short) (( (nFilter) & (1 << (_f))) != 0))

void SetAllAllowablesTo (int nFilter)
{
	nLastReactorLife = 0;   //default to zero
   
netGame.m_info.DoMegas = WF (0);
netGame.m_info.DoSmarts = WF (1);
netGame.m_info.DoFusions = WF (2);
netGame.m_info.DoHelix = WF (3);
netGame.m_info.DoPhoenix = WF (4);
netGame.m_info.DoCloak = WF (5);
netGame.m_info.DoInvulnerability = WF (6);
netGame.m_info.DoAfterburner = WF (7);
netGame.m_info.DoGauss = WF (8);
netGame.m_info.DoVulcan = WF (9);
netGame.m_info.DoPlasma = WF (10);
netGame.m_info.DoOmega = WF (11);
netGame.m_info.DoSuperLaser = WF (12);
netGame.m_info.DoProximity = WF (13);
netGame.m_info.DoSpread = WF (14);
netGame.m_info.DoMercury = WF (15);
netGame.m_info.DoSmartMine = WF (16);
netGame.m_info.DoFlash = WF (17);
netGame.m_info.DoGuided = WF (18);
netGame.m_info.DoEarthShaker = WF (19);
netGame.m_info.DoConverter = WF (20);
netGame.m_info.DoAmmoRack = WF (21);
netGame.m_info.DoHeadlight = WF (22);
netGame.m_info.DoHoming = WF (23);
netGame.m_info.DoLaserUpgrade = WF (24);
netGame.m_info.DoQuadLasers = WF (25);
netGame.m_info.BrightPlayers = !WF (26);
netGame.m_info.invul = WF (27);
}

#undef WF

//------------------------------------------------------------------------------

#define	WF(_w, _f)	if (_w) mpParams.nWeaponFilter |= (1 << (_f))

void GetAllAllowables (void)
{
mpParams.nWeaponFilter = 0;
WF (netGame.m_info.DoMegas, 0);
WF (netGame.m_info.DoSmarts, 1);
WF (netGame.m_info.DoFusions, 2);
WF (netGame.m_info.DoHelix, 3);
WF (netGame.m_info.DoPhoenix, 4);
WF (netGame.m_info.DoCloak, 5);
WF (netGame.m_info.DoInvulnerability, 6);
WF (netGame.m_info.DoAfterburner, 7);
WF (netGame.m_info.DoGauss, 8);
WF (netGame.m_info.DoVulcan, 9);
WF (netGame.m_info.DoPlasma, 10);
WF (netGame.m_info.DoOmega, 11);
WF (netGame.m_info.DoSuperLaser, 12);
WF (netGame.m_info.DoProximity, 13);
WF (netGame.m_info.DoSpread, 14);
WF (netGame.m_info.DoMercury, 15);
WF (netGame.m_info.DoSmartMine, 16);
WF (netGame.m_info.DoFlash, 17);
WF (netGame.m_info.DoGuided, 18);
WF (netGame.m_info.DoEarthShaker, 19);
WF (netGame.m_info.DoConverter, 20);
WF (netGame.m_info.DoAmmoRack, 21);
WF (netGame.m_info.DoHeadlight, 22);
WF (netGame.m_info.DoHoming, 23);
WF (netGame.m_info.DoLaserUpgrade, 24);
WF (netGame.m_info.DoQuadLasers, 25);
WF (!netGame.m_info.BrightPlayers, 26);
WF (netGame.m_info.invul, 27);
}

#undef WF

//------------------------------------------------------------------------------

#define ENDLEVEL_SEND_INTERVAL  1000
#define ENDLEVEL_IDLE_TIME      20000

int NetworkEndLevelPoll2 (CMenu& menu, int& key, int nCurItem, int nState)
{
if (nState)
	return nCurItem;

	// Polling loop for End-of-level menu

	static fix t1 = 0;
	int i = 0;
	int nReady = 0;
	int bSecret = 0;
	fix t;

	// Send our endlevel packet at regular intervals
if ((t = SDL_GetTicks ()) > (t1 + ENDLEVEL_SEND_INTERVAL)) {
	NetworkSendEndLevelPacket (); // tell other players that I have left the level and am waiting in the score screen
	t1 = t;
	}
NetworkListen ();
for (i = 0; i < gameData.multiplayer.nPlayers; i++) {
	if ((gameData.multiplayer.players [i].connected != 1) && 
		 (gameData.multiplayer.players [i].connected != 5) && 
		 (gameData.multiplayer.players [i].connected != 6)) {
		nReady++;
		if (gameData.multiplayer.players [i].connected == 4)
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

int NetworkEndLevelPoll3 (CMenu& menu, int& key, int nCurItem, int nState)
{
if (nState)
	return nCurItem;

	// Polling loop for End-of-level menu
   int nReady = 0, i;
 
if (TimerGetApproxSeconds () > (gameData.multiplayer.xStartAbortMenuTime + (I2X (8))))
	key = -2;
NetworkListen ();
for (i = 0; i < gameData.multiplayer.nPlayers; i++)
	if ((gameData.multiplayer.players [i].connected != 1) && 
		 (gameData.multiplayer.players [i].connected != 5) && 
		 (gameData.multiplayer.players [i].connected != 6))
		nReady++;
if (nReady == gameData.multiplayer.nPlayers) // All players have checked in or are disconnected
	key = -2;
return nCurItem;
}

//------------------------------------------------------------------------------

int NetworkStartPoll (CMenu& menu, int& key, int nCurItem, int nState)
{
if (nState)
	return nCurItem;

	int i, n, nm;

	key = key;
	nCurItem = nCurItem;

Assert (networkData.nStatus == NETSTAT_STARTING);
if (!menu [0].Value ()) {
	menu [0].Value () = 1;
	menu [0].Rebuild ();
	}
for (i = 1; i < int (menu.ToS ()); i++) {
	if ((i >= gameData.multiplayer.nPlayers) && menu [i].Value ()) {
		menu [i].Value () = 0;
		menu [i].Rebuild ();
		}
	}
nm = 0;
for (i = 0; i < int (menu.ToS ()); i++) {
	if (menu [i].Value ()) {
		if (++nm > gameData.multiplayer.nPlayers) {
			menu [i].Value () = 0;
			menu [i].Rebuild ();
			}
		}
	}
if (nm > gameData.multiplayer.nMaxPlayers) {
	MsgBox (TXT_ERROR, NULL, 1, TXT_OK, "%s %d %s", TXT_SORRY_ONLY, gameData.multiplayer.nMaxPlayers, TXT_NETPLAYERS_IN);
	// Turn off the last CPlayerData highlighted
	for (i = gameData.multiplayer.nPlayers; i > 0; i--)
		if (menu [i].Value () == 1) {
			menu [i].Value () = 0;
			menu [i].Rebuild ();
			break;
			}
	}

//       if (menu.ToS () > MAX_PLAYERS) return; 

n = netGame.m_info.nNumPlayers;
NetworkListen ();

if (n < netGame.m_info.nNumPlayers) {
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
else if (n > netGame.m_info.nNumPlayers) {
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

static int nGameTypes = -1, nGameItem = -1, nEntropy = -1, nMonsterball = -1;

int NetworkGameParamPoll (CMenu& menu, int& key, int nCurItem, int nState)
{
if (nState)
	return nCurItem;
if (!menu.Available ("max. players"))
	return nCurItem;

CMenuItem& maxPlayers = menu ["max. players"];

	static int oldMaxPlayers = 0;

int i = menu.IndexOf ("anarchy");
if ((nCurItem >= i) && (nCurItem < i + nGameTypes)) {
	if ((nCurItem != nGameItem) && menu [nCurItem].Value ()) {
		nGameItem = nCurItem;
		key = -2;
		return nCurItem;
		}
	}
if ((menu.Available ("entropy") && ((bool) menu.Value ("entropy") == menu.Available ("entropy options"))) ||
	 ((gameOpts->app.bExpertMode == SUPERUSER) && menu.Available ("monsterball") && ((bool) menu.Value ("monsterball") == menu.Available ("monsterball options"))))
	key = -2;
//force restricted game for team games
//obsolete with D2X-W32 as it can assign players to teams automatically
//even in a match and progress, and allows players to switch teams
if (menu.Value ("coop")) {
	oldMaxPlayers = 1;
	if (maxPlayers.Value () > 2)  {
		maxPlayers.Value () = 2;
		maxPlayers.Redraw ();
		}
	if (maxPlayers.MaxValue () > 2) {
		maxPlayers.MaxValue () = 2;
		maxPlayers.Redraw ();
		}
	if (!(netGame.m_info.gameFlags & NETGAME_FLAG_SHOW_MAP))
		netGame.m_info.gameFlags |= NETGAME_FLAG_SHOW_MAP;
	if (netGame.GetPlayTimeAllowed () || netGame.GetScoreGoal ()) {
		netGame.SetPlayTimeAllowed (0);
		netGame.SetScoreGoal (0);
		}
	}
else {// if !Coop game
	if (oldMaxPlayers) {
		oldMaxPlayers = 0;
		maxPlayers.Value () = 
		maxPlayers.MaxValue () = MAX_NUM_NET_PLAYERS - 2;
		}
	}         
if (nLastMaxPlayers != menu.Value ("max. players"))  {
	sprintf (maxPlayers.Text (), TXT_MAX_PLAYERS, maxPlayers.Value () + 2);
	nLastMaxPlayers = maxPlayers.Value ();
	maxPlayers.Rebuild ();
	}               
return nCurItem;
}

//------------------------------------------------------------------------------

int NetworkMoreOptionsPoll (CMenu& menu, int& key, int nCurItem, int nState)
{
if (nState)
	return nCurItem;

if (nLastReactorLife != menu.Value ("reactor life"))   {
	sprintf (menu ["reactor life"].Text (), "%s: %d %s", TXT_REACTOR_LIFE, menu.Value ("reactor life") * 5, TXT_MINUTES_ABBREV);
	nLastReactorLife = menu.Value ("reactor life");
	menu ["reactor life"].Rebuild ();
   }
  
if (menu.Available ("play time") && (menu.Value ("play time") != nLastPlayTime)) {
	nLastPlayTime = mpParams.nMaxTime;
	mpParams.nMaxTime = menu.Value ("play time");
	sprintf (menu ["play time"].Text (), TXT_MAXTIME, nLastPlayTime * 5, TXT_MINUTES_ABBREV);
	menu ["play time"].Rebuild ();
	}
if (menu.Available ("score goal") && (menu.Value ("score goal") != nLastScoreGoal)) {
	nLastScoreGoal = mpParams.nScoreGoal;
	mpParams.nScoreGoal = menu.Value ("score goal");
	sprintf (menu ["score goal"].Text (), TXT_SCOREGOAL, mpParams.nScoreGoal * 5);
	menu ["score goal"].Rebuild ();
	}

return nCurItem;
}

//------------------------------------------------------------------------------

void NetworkMoreGameOptions (void)
{
	static int choice = 0;

	int		i;
	char		szPlayTime [80], szScoreGoal [80], szInvul [50], szSocket [6], szPPS [6];
	CMenu		m;

do {
	m.Destroy ();
	m.Create (40);
	m.AddSlider (TXT_DIFFICULTY, mpParams.nDifficulty, 0, NDL - 1, KEY_D, HTX_GPLAY_DIFFICULTY, "difficulty"); 
	sprintf (szInvul + 1, "%s: %d %s", TXT_REACTOR_LIFE, mpParams.nReactorLife * 5, TXT_MINUTES_ABBREV);
	strupr (szInvul + 1);
	*szInvul = * (TXT_REACTOR_LIFE - 1);
	m.AddSlider (szInvul + 1, mpParams.nReactorLife, 0, 10, KEY_R, HTX_MULTI2_REACTOR, "reactor life"); 
	if (IsCoopGame) {
		nLastPlayTime =
		nLastScoreGoal = 0;
		}
	else {
		sprintf (szPlayTime + 1, TXT_MAXTIME, mpParams.nMaxTime * 5, TXT_MINUTES_ABBREV);
		*szPlayTime = * (TXT_MAXTIME - 1);
		m.AddSlider (szPlayTime + 1, mpParams.nMaxTime, 0, 10, KEY_T, HTX_MULTI2_LVLTIME, "play time"); 
		sprintf (szScoreGoal + 1, TXT_SCOREGOAL, mpParams.nScoreGoal * 5);
		*szScoreGoal = *(TXT_SCOREGOAL - 1);
		m.AddSlider (szScoreGoal + 1, mpParams.nScoreGoal, 0, 10, KEY_K, HTX_MULTI2_SCOREGOAL, "score goal");
		}
	m.AddCheck (TXT_INVUL_RESPAWN, mpParams.bInvul, KEY_I, HTX_MULTI2_INVUL, "spawn invul");
	m.AddCheck (TXT_MARKER_CAMS, mpParams.bMarkerView, KEY_C, HTX_MULTI2_MARKERCAMS, "marker cameras");
	m.AddCheck (TXT_KEEP_LIGHTS, mpParams.bIndestructibleLights, KEY_L, HTX_MULTI2_KEEPLIGHTS, "indestructible lights");
	m.AddCheck (TXT_BRIGHT_SHIPS, mpParams.bBrightPlayers ? 0 : 1, KEY_S, HTX_MULTI2_BRIGHTSHIP, "bright ships");
	m.AddCheck (TXT_SHOW_NAMES, mpParams.bShowAllNames, KEY_E, HTX_MULTI2_SHOWNAMES, "show players names");
	m.AddCheck (TXT_SHOW_PLAYERS, mpParams.bShowPlayersOnAutomap, KEY_A, HTX_MULTI2_SHOWPLRS, "show players on map");
	m.AddCheck (TXT_SHORT_PACKETS, mpParams.bShortPackets, KEY_H, HTX_MULTI2_SHORTPKTS, "short packets");
	m.AddText ("", 0);
	m.AddMenu (TXT_WAOBJECTS_MENU, KEY_O, HTX_MULTI2_OBJECTS, "allowed weapons");
	m.AddText ("", 0);
	sprintf (szSocket, "%d", (gameStates.multi.nGameType == UDP_GAME) ? mpParams.udpPorts [0] + networkData.nPortOffset : networkData.nPortOffset);
	if (gameStates.multi.nGameType >= IPX_GAME) {
		m.AddText (TXT_SOCKET2, KEY_N);
		m.AddInput (szSocket, 5, HTX_MULTI2_SOCKET, "socket");
		}

	sprintf (szPPS, "%d", mpParams.nPPS);
	m.AddText (TXT_PPS, KEY_P);
	m.AddInput (szPPS, 2, HTX_MULTI2_PPS, "PPS");

	nLastScoreGoal = netGame.GetScoreGoal ();
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

mpParams.bInvul = ubyte (m.Value ("spawn invul"));
mpParams.bBrightPlayers = ubyte (m.Value ("bright ships") ? 0 : 1);
mpParams.bShowAllNames = ubyte (m.Value ("show player names"));
mpParams.bMarkerView = ubyte (m.Value ("marker cameras"));
mpParams.bIndestructibleLights = ubyte (m.Value ("indestructible lights"));
mpParams.nDifficulty = m.Value ("difficulty");
mpParams.bShowPlayersOnAutomap = m.Value ("show players on map");
mpParams.bShortPackets = m.Value ("short packets");
mpParams.nPPS = atoi ("PPS");
if (mpParams.nPPS > 20) {
	mpParams.nPPS = 20;
	MsgBox (TXT_ERROR, NULL, 1, TXT_OK, TXT_PPS_HIGH_ERROR);
}
else if (mpParams.nPPS < 2) {
	MsgBox (TXT_ERROR, NULL, 1, TXT_OK, TXT_PPS_HIGH_ERROR);
	mpParams.nPPS = 2;      
}

if (gameStates.multi.nGameType >= IPX_GAME) { 
	int newSocket = atoi (szSocket);
	if ((newSocket < -0xFFFF) || (newSocket > 0xFFFF))
		MsgBox (TXT_ERROR, NULL, 1, TXT_OK, TXT_INV_SOCKET, 
				  (gameStates.multi.nGameType == UDP_GAME) ? mpParams.udpPorts [0] + networkData.nPortOffset : networkData.nPortOffset);
	else if (newSocket != networkData.nPortOffset) {
		networkData.nPortOffset = (gameStates.multi.nGameType == UDP_GAME) ? newSocket - mpParams.udpPorts [0] : newSocket;
		IpxChangeDefaultSocket ((ushort) (IPX_DEFAULT_SOCKET + networkData.nPortOffset));
		}
	}

}

//------------------------------------------------------------------------------

int NetworkD2XOptionsPoll (CMenu& menu, int& key, int nCurItem, int nState)
{
if (nState)
	return nCurItem;

	int	v;

v = menu.Value ("competition");
if (v != extraGameInfo [1].bCompetition) {
	extraGameInfo [1].bCompetition = v;
	key = -2;
	return nCurItem;
	}

if (menu.Available ("coop penalty")) {
	v = menu.Value ("coop penalty");
	if (v != extraGameInfo [1].nCoopPenalty) {
		extraGameInfo [1].nCoopPenalty = v;
		sprintf (menu ["coop penalty"].Text (), TXT_COOP_PENALTY, nCoopPenalties [v], '%');
		menu ["coop penalty"].Rebuild ();
		return nCurItem;
		}
	}

v = menu.Value ("darkness");
if (v != extraGameInfo [1].bDarkness) {
	extraGameInfo [1].bDarkness = v;
	key = -2;
	return nCurItem;
	}

if (menu.Available ("target indicators")) {
	v = menu.Value ("target indicators");
	if (v != (extraGameInfo [1].bTargetIndicators != 0)) {
		extraGameInfo [1].bTargetIndicators = v;
		key = -2;
		return nCurItem;
		}
	}

if (menu.Available ("headlights")) {
	v = menu.Value ("headlights");
	if (v == extraGameInfo [1].headlight.bAvailable) {
		extraGameInfo [1].headlight.bAvailable = !v;
		key = -2;
		return nCurItem;
		}
	}

if (menu.Available ("spot size")) {
	v = menu.Value ("spot size");
	if (v != extraGameInfo [1].nSpotSize) {
		extraGameInfo [1].nSpotSize =
		extraGameInfo [1].nSpotStrength = v;
		sprintf (menu ["spot size"].Text (), TXT_SPOTSIZE, GT (664 + v));
		menu ["spot size"].Rebuild ();
		return nCurItem;
		}
	}

v = menu.Value ("smoke grenades");
if (extraGameInfo [0].bSmokeGrenades != v) {
	extraGameInfo [0].bSmokeGrenades = v;
	key = -2;
	return nCurItem;
	}

if (extraGameInfo [0].bSmokeGrenades && menu.Available ("max. smoke grenades")) {
	v = menu.Value ("max. smoke grenades");
	if (extraGameInfo [0].nMaxSmokeGrenades != v) {
		extraGameInfo [0].nMaxSmokeGrenades = v;
		sprintf (menu ["max. smoke grenades"].Text (), TXT_MAX_SMOKEGRENS, extraGameInfo [0].nMaxSmokeGrenades);
		menu ["max. smoke grenades"].Rebuild ();
		}
	}

if (menu.Available ("spawn delay")) {
	v = menu.Value ("spawn delay") * 5;
	if (extraGameInfo [0].nSpawnDelay != v * 1000) {
		extraGameInfo [0].nSpawnDelay = v * 1000;
		sprintf (menu ["spawn delay"].Text (), TXT_RESPAWN_DELAY, v);
		menu ["spawn delay"].Rebuild ();
		}
	}

return nCurItem;
}

//------------------------------------------------------------------------------

void NetworkD2XOptions (void)
{
	static int choice = 0;

	int		i, optCheckPort = -1;
	char		szSlider [50];
	CMenu		m;

do {
	m.Destroy ();
	m.Create (40);
	m.AddCheck (TXT_COMPETITION_MODE, extraGameInfo [1].bCompetition, KEY_C, HTX_MULTI2_COMPETITION, "competition");
	if (!extraGameInfo [1].bCompetition) {
		m.AddCheck (TXT_FRIENDLY_FIRE, extraGameInfo [0].bFriendlyFire, KEY_F, HTX_MULTI2_FFIRE, "friendly fire");
		m.AddCheck (TXT_NO_SUICIDE, extraGameInfo [0].bInhibitSuicide, KEY_U, HTX_MULTI2_SUICIDE, "inhibit suicide");
		m.AddCheck (TXT_MOUSELOOK, extraGameInfo [1].bMouseLook, KEY_O, HTX_MULTI2_MOUSELOOK, "mouselook");
		m.AddCheck (TXT_FASTPITCH, (extraGameInfo [1].bFastPitch == 1) ? 1 : 0, KEY_P, HTX_MULTI2_FASTPITCH, "fast pitch");
		//m.AddCheck (TXT_DUAL_LAUNCH, extraGameInfo [1].bDualMissileLaunch, KEY_M, HTX_GPLAY_DUALLAUNCH, "dual missile launch");
		}
	if (IsTeamGame) {
		m.AddCheck (TXT_AUTOBALANCE, extraGameInfo [0].bAutoBalanceTeams, KEY_B, HTX_MULTI2_BALANCE, "auto balance");
		m.AddCheck (TXT_TEAMDOORS, mpParams.bTeamDoors, KEY_T, HTX_TEAMDOORS, "team doors");
		}
	if (mpParams.nGameMode == NETGAME_CAPTURE_FLAG)
		m.AddCheck (TXT_TOW_FLAGS, extraGameInfo [1].bTowFlags, KEY_F, HTX_TOW_FLAGS, "tow flags");
	if (!extraGameInfo [1].bCompetition)
		m.AddCheck (TXT_MULTICHEATS, mpParams.bEnableCheats, KEY_T, HTX_MULTICHEATS, "multiplayer cheats");
	m.AddCheck (TXT_MSN_CYCLE, extraGameInfo [1].bRotateLevels, KEY_Y, HTX_MULTI2_MSNCYCLE, "cycle levels"); 

#if 0
	mat.AddCheck (TXT_NO_REACTOR, extraGameInfo [1].bDisableReactor, KEY_R, HTX_MULTI2_NOREACTOR, "disable reactor"); 
#endif
#if UDP_SAFEMODE
	m.AddCheck (TXT_UDP_QUAL, extraGameInfo [0].bSafeUDP, KEY_Q, HTX_MULTI2_UDPQUAL, "safe udp");
#endif

	m.AddCheck (TXT_CHECK_PORT, extraGameInfo [0].bCheckUDPPort, KEY_P, HTX_MULTI2_CHECKPORT, "check ports");
	if (extraGameInfo [1].bDarkness)
		m.AddText ("", 0);
	m.AddCheck (TXT_DARKNESS, extraGameInfo [1].bDarkness, KEY_D, HTX_DARKNESS, "darkness");
	if (extraGameInfo [1].bDarkness) {
		m.AddCheck (TXT_POWERUPLIGHTS, !extraGameInfo [1].bPowerupLights, KEY_P, HTX_POWERUPLIGHTS, "powerups cast light");
		m.AddCheck (TXT_HEADLIGHTS, !extraGameInfo [1].headlight.bAvailable, KEY_H, HTX_HEADLIGHTS, "headlights");
		if (extraGameInfo [1].headlight.bAvailable) {
			sprintf (szSlider + 1, TXT_SPOTSIZE, GT (664 + extraGameInfo [1].nSpotSize));
			strupr (szSlider + 1);
			*szSlider = *(TXT_SPOTSIZE - 1);
			m.AddSlider (szSlider + 1, extraGameInfo [1].nSpotSize, 0, 2, KEY_O, HTX_SPOTSIZE, "spot size"); 
			}
		}
	if (!extraGameInfo [1].bCompetition) {
		if (mpParams.nGameMode == NETGAME_COOPERATIVE) {
			sprintf (szSlider + 1, TXT_COOP_PENALTY, nCoopPenalties [(int) extraGameInfo [1].nCoopPenalty], '%');
			strupr (szSlider + 1);
			*szSlider = *(TXT_COOP_PENALTY - 1);
			m.AddSlider (szSlider + 1, extraGameInfo [1].nCoopPenalty, 0, 9, KEY_O, HTX_COOP_PENALTY, "coop penalty"); 
			}
		m.AddCheck (TXT_MULTI_TGTIND, extraGameInfo [1].bTargetIndicators != 0, KEY_A, HTX_CPIT_TGTIND, "target indicators");
		}

	m.AddCheck (TXT_GPLAY_SMOKEGRENADES, extraGameInfo [0].bSmokeGrenades, KEY_S, HTX_GPLAY_SMOKEGRENADES, "smoke grenades");
	if (extraGameInfo [0].bSmokeGrenades) {
		sprintf (szSlider + 1, TXT_MAX_SMOKEGRENS, extraGameInfo [0].nMaxSmokeGrenades);
		*szSlider = *(TXT_MAX_SMOKEGRENS - 1);
		m.AddSlider (szSlider + 1, extraGameInfo [0].nMaxSmokeGrenades - 1, 0, 3, KEY_X, HTX_GPLAY_MAXGRENADES, "max. smoke grenades");
		}
	m.AddCheck (TXT_FIXED_SPAWN, extraGameInfo [0].bFixedRespawns, KEY_F, HTX_GPLAY_FIXEDSPAWN, "fixed spawn points");
	if (extraGameInfo [0].nSpawnDelay < 0)
		extraGameInfo [0].nSpawnDelay = 0;
	sprintf (szSlider + 1, TXT_RESPAWN_DELAY, extraGameInfo [0].nSpawnDelay / 1000);
	*szSlider = *(TXT_RESPAWN_DELAY - 1);
	m.AddSlider (szSlider + 1, extraGameInfo [0].nSpawnDelay / 5000, 0, 12, KEY_R, HTX_GPLAY_SPAWNDELAY, "spawn delay");
	m.AddText ("", 0);

	i = m.Menu (NULL, TXT_D2XOPTIONS_TITLE, NetworkD2XOptionsPoll, &choice);
  //mpParams."reactor life" = atoi (szInvul)*I2X (60);
	extraGameInfo [1].bDarkness = (ubyte) m.Value ("darkness");
	if (m.Available ("darkness")) {
		if ((mpParams.bDarkness = extraGameInfo [1].bDarkness)) {
			extraGameInfo [1].headlight.bAvailable = !m.Value ("headlights");
			extraGameInfo [1].bPowerupLights = !m.Value ("powerups cast light");
			}
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

static int powerupMap [] = {
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

int NetworkMissileOptionsPoll (CMenu& menu, int& key, int nCurItem, int nState)
{
if (nState)
	return nCurItem;

	int	h, i, v;

for (i = 0; i < int (menu.ToS ()); i++) {
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
	static int choice = 0;

	int		h, i, j;
	char		szSlider [50];
	CMenu		m;

j = sizeofa (powerupMap);
do {
	m.Destroy ();
	m.Create (j);
	for (i = 0; i < j; i++) {
		if ((h = powerupMap [i]) < 0) {
			h = m.AddText (szMslNames [i]);
			m [h].m_bCentered = 1;
			}
		else {
			if (extraGameInfo [0].loadout.nMissiles [h] > nMaxSecondaryAmmo [h])
				extraGameInfo [0].loadout.nMissiles [h] = nMaxSecondaryAmmo [h];
			sprintf (szSlider, szMslNames [i], extraGameInfo [0].loadout.nMissiles [h]);
			strupr (szSlider);
			m.AddSlider (szSlider, extraGameInfo [0].loadout.nMissiles [h], 0, nMaxSecondaryAmmo [h], 0, NULL); 
			}
		}
	i = m.Menu (NULL, TXT_MSLLOADOUT_TITLE, NetworkMissileOptionsPoll, &choice);
	} while (i == -2);
}

//------------------------------------------------------------------------------

int NetworkGetGameType (CMenu& m, int bAnarchyOnly)
{
	int bHoard = HoardEquipped ();
		   
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
	MsgBox (NULL, NULL, 1, TXT_OK, TXT_ANARCHY_ONLY_MISSION);
	m ["anarchy"].Value () = 1;
	m ["robot anarchy"].Value () = 0;
	m ["coop"].Value () = 0;
	return 0;
	}               
else if (m.Value ("robot anarchy")) 
	mpParams.nGameMode = NETGAME_ROBOT_ANARCHY;
else if (m.Value ("coop")) 
	mpParams.nGameMode = NETGAME_COOPERATIVE;
return 1;
}

//------------------------------------------------------------------------------

void BuildGameParamsMenu (CMenu& m, char* szName, char* szLevelText, char* szLevel, char* szIpAddr, char* szMaxPlayers, int nNewMission)
{
	int bHoard = HoardEquipped ();

sprintf (szLevel, "%d", mpParams.nLevel);

m.Destroy ();
m.Create (35);
if (gameStates.multi.nGameType == UDP_GAME) {
	if (UDPGetMyAddress () < 0) 
		strcpy (szIpAddr, TXT_IP_FAIL);
	else {
		sprintf (szIpAddr, "Game Host: %d.%d.%d.%d:%d", 
					ipx_MyAddress [4], 
					ipx_MyAddress [5], 
					ipx_MyAddress [6], 
					ipx_MyAddress [7], 
					mpParams.udpPorts [0]);
		}
	}
m.AddText (TXT_DESCRIPTION, 0); 
m.AddInput (szName, NETGAME_NAME_LEN, HTX_MULTI_NAME, "game name"); 
m.AddMenu (TXT_SEL_MISSION, KEY_I, HTX_MULTI_MISSION, "mission selector");
m.AddText ("", 0, "mission name");
m ["mission name"].Rebuild (); 
if ((nNewMission >= 0) && (missionManager.nLastLevel > 1)) {
	m.AddText (szLevelText, 0, "level number text"); 
	m.AddInput (szLevel, 4, HTX_MULTI_LEVEL, "level number");
	}
m.AddText ("", 0); 
nGameTypes = m.ToS ();
m.AddRadio (TXT_ANARCHY, 0, KEY_A, HTX_MULTI_ANARCHY, "anarchy");
m.AddRadio (TXT_TEAM_ANARCHY, 0, KEY_T, HTX_MULTI_TEAMANA, "team anarchy");
m.AddRadio (TXT_ANARCHY_W_ROBOTS, 0, KEY_R, HTX_MULTI_BOTANA, "robot anarchy");
m.AddRadio (TXT_COOP, 0, KEY_P, HTX_MULTI_COOP, "coop");
m.AddRadio (TXT_CTF, 0, KEY_F, HTX_MULTI_CTF, "CTF");
if (!gameStates.app.bNostalgia)
	m.AddRadio (TXT_CTF_PLUS, 0, KEY_T, HTX_MULTI_CTFPLUS, "CTF+");

if (bHoard) {
	m.AddRadio (TXT_HOARD, 0, KEY_H, HTX_MULTI_HOARD, "hoard");
	m.AddRadio (TXT_TEAM_HOARD, 0, KEY_O, HTX_MULTI_TEAMHOARD, "team hoard");
	if (!gameStates.app.bNostalgia) {
		m.AddRadio (TXT_ENTROPY, 0, KEY_Y, HTX_MULTI_ENTROPY, "entropy");
		m.AddRadio (TXT_MONSTERBALL, 0, KEY_B, HTX_MULTI_MONSTERBALL, "monsterball");
		}
	} 
nGameTypes = m.ToS () - nGameTypes;
m [m.IndexOf ("anarchy") + NMCLAMP (mpParams.nGameType, 0, nGameTypes)].Value () = 1;

m.AddText ("", 0); 

m.AddRadio (TXT_OPEN_GAME, 0, KEY_O, HTX_MULTI_OPENGAME, "open game");
m.AddRadio (TXT_CLOSED_GAME, 0, KEY_C, HTX_MULTI_CLOSEDGAME, "closed game");
m.AddRadio (TXT_RESTR_GAME, 0, KEY_R, HTX_MULTI_RESTRGAME, "restricted game");
m [m.IndexOf ("open game") + NMCLAMP (mpParams.nGameAccess, 0, 2)].Value () = 1;
m.AddText ("", 0);
sprintf (szMaxPlayers + 1, TXT_MAX_PLAYERS, gameData.multiplayer.nMaxPlayers);
*szMaxPlayers = *(TXT_MAX_PLAYERS - 1);
nLastMaxPlayers = gameData.multiplayer.nMaxPlayers - 2;
m.AddSlider (szMaxPlayers + 1, nLastMaxPlayers, 0, nLastMaxPlayers, KEY_X, HTX_MULTI_MAXPLRS, "max. players"); 
m.AddText ("", 0);
m.AddMenu (TXT_MORE_OPTS, KEY_M, HTX_MULTI_MOREOPTS, "more options");
if (!gameStates.app.bNostalgia) {
	m.AddMenu (TXT_MULTI_D2X_OPTS, KEY_X, HTX_MULTI_D2XOPTS, "d2x options");
	if (m.Available ("entropy") && m.Value ("entropy"))
		m.AddMenu (TXT_ENTROPY_OPTS, KEY_E, HTX_MULTI_ENTOPTS, "entropy options");
	if ((gameOpts->app.bExpertMode == SUPERUSER) && m.Available ("monsterball") && m.Value ("monsterball"))
		m.AddMenu (TXT_MONSTERBALL_OPTS, KEY_O, HTX_MULTI_MBALLOPTS, "monsterball options");
	else
		InitMonsterballSettings (&extraGameInfo [0].monsterball);
	m.AddMenu (TXT_GAME_OPTIONS, KEY_O, HTX_MAIN_CONF, "config options");
	m.AddMenu (TXT_LOADOUT_OPTION, KEY_L, HTX_MULTI_LOADOUT, "loadout options");
	m.AddMenu (TXT_MISSILE_LOADOUT, KEY_I, HTX_MISSILE_LOADOUT, "missile options");
	}
}

//------------------------------------------------------------------------------

int GameParamsMenu (CMenu& m, int& key, int& choice, char* szName, char* szLevelText, char* szLevel, char* szIpAddr, int& nNewMission)
{
	int i, bAnarchyOnly = (nNewMission < 0) ? 0 : missionManager [nNewMission].bAnarchyOnly;

if (m ["mission name"].Rebuilding ()) {
	strncpy (netGame.m_info.szMissionName, 
				(nNewMission < 0) ? "" : missionManager [nNewMission].filename, 
				sizeof (netGame.m_info.szMissionName) - 1);
	m ["mission name"].SetText ((nNewMission < 0) ? const_cast<char*> (TXT_NONE_SELECTED) : const_cast<char*> (missionManager [nNewMission].szMissionName));
	if ((nNewMission >= 0) && (missionManager.nLastLevel > 1)) {
		sprintf (szLevelText, "%s (1-%d)", TXT_LEVEL_, missionManager.nLastLevel);
		if (strlen (szLevelText) < 32)
			szLevelText [31] = '\0';
		m ["level number text"].Rebuild ();
		}
	mpParams.nLevel = 1;
	}

gameStates.app.nExtGameStatus = GAMESTAT_NETGAME_OPTIONS; 
key = m.Menu (NULL, (gameStates.multi.nGameType == UDP_GAME) ? szIpAddr : NULL, NetworkGameParamPoll, &choice);
								//TXT_NETGAME_SETUP
if (key == -1)
	return -1;
else if (choice == m.IndexOf ("more options")) {
	if (m [nGameTypes + 3].Value ())
		gameData.app.nGameMode = GM_MULTI_COOP;
	NetworkMoreGameOptions ();
	gameData.app.nGameMode = 0;
	if (gameStates.multi.nGameType == UDP_GAME) {
		sprintf (szIpAddr, "Game Host: %d.%d.%d.%d:%d", 
					ipx_MyAddress [4], ipx_MyAddress [5], ipx_MyAddress [6], ipx_MyAddress [7], mpParams.udpPorts [0]);
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
	int h = SelectAndLoadMission (1, &bAnarchyOnly);
	if (h < 0)
		return 1;
	missionManager.nLastMission = nNewMission = h;
	m ["mission name"].Rebuild ();
	return 2;
	}

if (key != -1) {
	int h, j;
		   
	gameData.multiplayer.nMaxPlayers = m.Value ("max. players") + 2;
	netGame.m_info.nMaxPlayers = gameData.multiplayer.nMaxPlayers;
			
	for (j = 0; j < networkData.nActiveGames; j++)
		if (!stricmp (activeNetGames [j].m_info.szGameName, szName)) {
			MsgBox (TXT_ERROR, NULL, 1, TXT_OK, TXT_DUPLICATE_NAME);
			return 1;
		}
	strncpy (mpParams.szGameName, szName, sizeof (mpParams.szGameName));
	mpParams.nLevel = atoi (szLevel);
	if ((missionManager.nLastLevel > 0) && ((mpParams.nLevel < 1) || (mpParams.nLevel > missionManager.nLastLevel))) {
		MsgBox (TXT_ERROR, NULL, 1, TXT_OK, TXT_LEVEL_OUT_RANGE);
		sprintf (szLevel, "1");
		return 1;
	}

	for (h = i = m.IndexOf ("anarchy"), j = m.IndexOf ("open game"); i < j; i++)
		if (m [i].Value ()) {
			mpParams.nGameType = i - h;
			break;
			}

	for (i = j, j = m.IndexOf ("max. players"); i < j; i++)
		if (m [i].Value ()) {
			mpParams.nGameAccess = i - j;
			break;
			}

	if (!NetworkGetGameType (m, bAnarchyOnly))
		return 1;
	if (m.Value ("closed game"))
		netGame.m_info.gameFlags |= NETGAME_FLAG_CLOSED;
	netGame.m_info.bRefusePlayers = m.Value ("restricted game");
	}
NetworkSetGameMode (mpParams.nGameMode);
if (key == -2)
	return 2;
return 0;
}

//------------------------------------------------------------------------------

int NetworkGetGameParams (int bAutoRun)
{
	int	i, key, choice = 1, nState = 0;
	CMenu	m;
	char	szName [80]; //NETGAME_NAME_LEN+1];
	char	szLevelText [80]; //32];
	char	szMaxPlayers [50];
	char	szIpAddr [80];
	char	szLevel [10]; //5];

	int nNewMission = missionManager.nLastMission;

gameOpts->app.bSinglePlayer = 0;
SetAllAllowablesTo (mpParams.nWeaponFilter);
networkData.nNamesInfoSecurity = -1;

for (i = 0; i < MAX_PLAYERS; i++)
	if (i != gameData.multiplayer.nLocalPlayer)
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
	PrintLog ("   building game parameters menu\n");
	BuildGameParamsMenu (m, szName, szLevelText, szLevel, szIpAddr, szMaxPlayers, nNewMission);
	PrintLog ("   loading game parameters menu\n");
	do {
		nState = GameParamsMenu (m, key, choice, szName, szLevelText, szLevel, szIpAddr, nNewMission);
		if ((nNewMission < 0) && (nState == 0)) {
			MsgBox (TXT_ERROR, NULL, 1, TXT_OK, "Please chose a mission");
			nState = 1;
			}
		} while (nState == 1);
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
netGame.m_info.szMissionName [sizeof (netGame.m_info.szMissionName) - 1] = '\0';
strncpy (netGame.m_info.szMissionTitle, missionManager [nNewMission].szMissionName + (gameOpts->menus.bShowLevelVersion ? 4 : 0), sizeof (netGame.m_info.szMissionTitle));
netGame.m_info.szMissionTitle [sizeof (netGame.m_info.szMissionTitle) - 1] = '\0';
netGame.SetControlInvulTime (mpParams.nReactorLife * 5 * I2X (60));
netGame.SetPlayTimeAllowed (mpParams.nMaxTime);
netGame.SetScoreGoal (mpParams.nScoreGoal * 5);
netGame.SetPacketsPerSec (mpParams.nPPS);
netGame.m_info.invul = mpParams.bInvul;
netGame.m_info.BrightPlayers = mpParams.bBrightPlayers;
netGame.SetShortPackets (mpParams.bShortPackets);
netGame.m_info.bAllowMarkerView = mpParams.bMarkerView;
netGame.m_info.bIndestructibleLights = mpParams.bIndestructibleLights; 
if (mpParams.bShowPlayersOnAutomap)
	netGame.m_info.gameFlags |= NETGAME_FLAG_SHOW_MAP;
else
	netGame.m_info.gameFlags &= ~NETGAME_FLAG_SHOW_MAP;
gameStates.app.nDifficultyLevel = mpParams.nDifficulty;
NetworkAdjustMaxDataSize ();
IpxChangeDefaultSocket ((ushort) (IPX_DEFAULT_SOCKET + networkData.nPortOffset));
return key;
}

//------------------------------------------------------------------------------

static time_t	nQueryTimeout;

static int QueryPoll (CMenu& menu, int& key, int nCurItem, int nState)
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
	int v = (int) (t / 5);
	if (menu [0].Value () != v) {
		menu [0].Value () = v;
		menu [0].Rebuild ();
		}
	key = 0;
	}
return nCurItem;
}

//------------------------------------------------------------------------------

int NetworkFindGame (void)
{
	CMenu	m (3);
	int	i;

if (gameStates.multi.nGameType > IPX_GAME)
	return 0;

networkData.nStatus = NETSTAT_BROWSING;
networkData.nActiveGames = 0;
NetworkSendGameListRequest ();

m.AddGauge ("                    ", -1, 1000); 
m.AddText ("");
m.AddText (TXT_PRESS_ESC);
m [2].m_bCentered = 1;
nQueryTimeout = SDL_GetTicks ();
do {
	i = m.Menu (NULL, TXT_NET_SEARCH, QueryPoll, 0, NULL);
	} while (i >= 0);
return (networkData.nActiveGames >= MAX_ACTIVE_NETGAMES);
}

//------------------------------------------------------------------------------

int ConnectionPacketLevel [] = {0, 1, 1, 1};
int ConnectionSecLevel [] = {12, 3, 5, 7};

int AppletalkConnectionPacketLevel [] = {0, 1, 0};
int AppletalkConnectionSecLevel [] = {10, 3, 8};

#if !DBG

int NetworkChooseConnect ()
{
return 1;
}

#else

int NetworkChooseConnect (void)
{
if (gameStates.multi.nGameType >= IPX_GAME) 
   return 1;
return 0;
}
#endif

//------------------------------------------------------------------------------

int stoport (char *szPort, int *pPort, int *pSign)
{
	int	h, sign;

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

int stoip (char *szServerIpAddr, ubyte *ipAddrP, int* portP)
{
	char	*pi, *pj, *pFields [5], tmp [22];
	int	h, i, j;

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
		ipAddrP [j] = (ubyte) h;
		}
	}
return 1;
}

//------------------------------------------------------------------------------

int IpAddrMenuCallBack (CMenu& menu, int& key, int nCurItem, int nState)
{
return nCurItem;
}

//------------------------------------------------------------------------------

int NetworkGetIpAddr (bool bServer, bool bUDP)
{
	CMenu	m (9);
	int	h, i, j, choice = 0;
	int	nServer = -1, optCheckPort, nPort [2] = {-1, -1};
	bool	bError;

	static char szPort [2][7] = {{'\0','\0','\0','\0','\0','\0','\0'}, {'\0','\0','\0','\0','\0','\0','\0'}};
	static int nSign = 0;

if (bUDP && !tracker.m_bUse) {
	if (!*mpParams.szServerIpAddr) {
		ArchIpxSetDriver (IPX_DRIVER_UDP);
		if (IpxInit (-1) != IPX_INIT_OK) {
			strcpy (mpParams.szServerIpAddr, "0.0.0.0");
			}
		else {
			sprintf (mpParams.szServerIpAddr, "%d.%d.%d.%d", 
						networkData.serverAddress [4], networkData.serverAddress [5], networkData.serverAddress [6], networkData.serverAddress [7]);
			sprintf (szPort [1], "%s%d", 
						 (nSign < 0) ? "-" : (nSign > 0) ? "+" : "", mpParams.udpPorts [1]);
			IpxClose ();
			}
		}
	m.AddText (TXT_HOST_IP, 0);
	nServer = m.AddInput (mpParams.szServerIpAddr, sizeof (mpParams.szServerIpAddr) - 1, HTX_GETIP_SERVER);
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
	m.AddText (GT (1100 + i));
	nPort [i] = m.AddInput (szPort [i], sizeof (szPort [i]) - 1, HTX_GETIP_CLIENT);
	}

m.AddText (TXT_PORT_HELP1, 0);
m.AddText (TXT_PORT_HELP2, 0);
m.AddText ("");
optCheckPort = m.AddCheck (TXT_CHECK_PORT, extraGameInfo [0].bCheckUDPPort, KEY_P, HTX_MULTI2_CHECKPORT);
for (;;) {
	i = m.Menu (NULL, tracker.m_bUse ? TXT_NETWORK_ADDRESSES : TXT_IP_HEADER, &IpAddrMenuCallBack, &choice);
	if (i < 0)
		break;
	if (i >= int (m.ToS ()))
		continue;
	bError = false;
	extraGameInfo [0].bCheckUDPPort = m.Value ("check ports") != 0;
	for (i = j; i < h; i++) { 
		stoport (szPort [i], &mpParams.udpPorts [i], &nSign);
		if (extraGameInfo [0].bCheckUDPPort && !mpParams.udpPorts [i])
			bError = true;
		}
	if (!(tracker.m_bUse || stoip (mpParams.szServerIpAddr, networkData.serverAddress + 4)))
		bError =  true;
	if (!bError)
		return 1;
	MsgBox (NULL, NULL, 1, TXT_OK, TXT_IP_INVALID);
	}
return 0;
} 

//------------------------------------------------------------------------------
/*
 * IpxSetDriver was called do_network_init and located in main/inferno
 * before the change which allows the user to choose the network driver
 * from the game menu instead of having to supply command line args.
 */
void IpxSetDriver (int ipx_driver)
{
	IpxClose ();

	int nIpxError;
	int socket = 0, t;

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
  int		choice;

m.AddText ("Enter an address or hostname:", 0);
m.AddInput (szIP, 50, NULL);
choice = m.Menu (NULL, TXT_JOIN_TCP);
if ((choice == -1) || !*m [1].Text ())
	return;
MsgBox (TXT_SORRY, NULL, 1, TXT_OK, TXT_INV_ADDRESS);
}

//------------------------------------------------------------------------------

