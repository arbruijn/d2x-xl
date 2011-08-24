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
  int		opt = 0, choice, optPrimary, optSecond, opt_power;
  
  optPrimary = 0;
  m.AddCheck (TXT_WA_LASER, netGame.m_info.DoLaserUpgrade, 0, NULL);
  m.AddCheck (TXT_WA_SLASER, netGame.m_info.DoSuperLaser, 0, NULL);
  m.AddCheck (TXT_WA_QLASER, netGame.m_info.DoQuadLasers, 0, NULL);
  m.AddCheck (TXT_WA_VULCAN, netGame.m_info.DoVulcan, 0, NULL);
  m.AddCheck (TXT_WA_SPREAD, netGame.m_info.DoSpread, 0, NULL);
  m.AddCheck (TXT_WA_PLASMA, netGame.m_info.DoPlasma, 0, NULL);
  m.AddCheck (TXT_WA_FUSION, netGame.m_info.DoFusions, 0, NULL);
  m.AddCheck (TXT_WA_GAUSS, netGame.m_info.DoGauss, 0, NULL);
  m.AddCheck (TXT_WA_HELIX, netGame.m_info.DoHelix, 0, NULL);
  m.AddCheck (TXT_WA_PHOENIX, netGame.m_info.DoPhoenix, 0, NULL);
  m.AddCheck (TXT_WA_OMEGA, netGame.m_info.DoOmega, 0, NULL);
  
  optSecond = m.ToS ();  
  m.AddCheck (TXT_WA_HOMING_MSL, netGame.m_info.DoHoming, 0, NULL);
  m.AddCheck (TXT_WA_PROXBOMB, netGame.m_info.DoProximity, 0, NULL);
  m.AddCheck (TXT_WA_SMART_MSL, netGame.m_info.DoSmarts, 0, NULL);
  m.AddCheck (TXT_WA_MEGA_MSL, netGame.m_info.DoMegas, 0, NULL);
  m.AddCheck (TXT_WA_FLASH_MSL, netGame.m_info.DoFlash, 0, NULL);
  m.AddCheck (TXT_WA_GUIDED_MSL, netGame.m_info.DoGuided, 0, NULL);
  m.AddCheck (TXT_WA_SMARTMINE, netGame.m_info.DoSmartMine, 0, NULL);
  m.AddCheck (TXT_WA_MERC_MSL, netGame.m_info.DoMercury, 0, NULL);
  m.AddCheck (TXT_WA_SHAKER_MSL, netGame.m_info.DoEarthShaker, 0, NULL);

  opt_power = opt;
  m.AddCheck (TXT_WA_INVUL, netGame.m_info.DoInvulnerability, 0, NULL);
  m.AddCheck (TXT_WA_CLOAK, netGame.m_info.DoCloak, 0, NULL);
  m.AddCheck (TXT_WA_BURNER, netGame.m_info.DoAfterburner, 0, NULL);
  m.AddCheck (TXT_WA_AMMORACK, netGame.m_info.DoAmmoRack, 0, NULL);
  m.AddCheck (TXT_WA_CONVERTER, netGame.m_info.DoConverter, 0, NULL);
  m.AddCheck (TXT_WA_HEADLIGHT, netGame.m_info.DoHeadlight, 0, NULL);
  
  choice = m.Menu (NULL, TXT_WA_OBJECTS);

  netGame.m_info.DoLaserUpgrade = m [optPrimary].m_value; 
  netGame.m_info.DoSuperLaser = m [optPrimary+1].m_value;
  netGame.m_info.DoQuadLasers = m [optPrimary+2].m_value;  
  netGame.m_info.DoVulcan = m [optPrimary+3].m_value;
  netGame.m_info.DoSpread = m [optPrimary+4].m_value;
  netGame.m_info.DoPlasma = m [optPrimary+5].m_value;
  netGame.m_info.DoFusions = m [optPrimary+6].m_value;
  netGame.m_info.DoGauss = m [optPrimary+7].m_value;
  netGame.m_info.DoHelix = m [optPrimary+8].m_value;
  netGame.m_info.DoPhoenix = m [optPrimary+9].m_value;
  netGame.m_info.DoOmega = m [optPrimary+10].m_value;
  
  netGame.m_info.DoHoming = m [optSecond].m_value;
  netGame.m_info.DoProximity = m [optSecond+1].m_value;
  netGame.m_info.DoSmarts = m [optSecond+2].m_value;
  netGame.m_info.DoMegas = m [optSecond+3].m_value;
  netGame.m_info.DoFlash = m [optSecond+4].m_value;
  netGame.m_info.DoGuided = m [optSecond+5].m_value;
  netGame.m_info.DoSmartMine = m [optSecond+6].m_value;
  netGame.m_info.DoMercury = m [optSecond+7].m_value;
  netGame.m_info.DoEarthShaker = m [optSecond+8].m_value;

  netGame.m_info.DoInvulnerability = m [opt_power].m_value;
  netGame.m_info.DoCloak = m [opt_power+1].m_value;
  netGame.m_info.DoAfterburner = m [opt_power+2].m_value;
  netGame.m_info.DoAmmoRack = m [opt_power+3].m_value;
  netGame.m_info.DoConverter = m [opt_power+4].m_value;     
  netGame.m_info.DoHeadlight = m [opt_power+5].m_value;     
  
 }

//------------------------------------------------------------------------------

static int optFF, optSuicide;

int nLastReactorLife = 0;
int optReactorLife, optAnarchy, optTeamAnarchy, optRobotAnarchy, optCoop;
int optPlayersOnMap, optMaxNet;
int optEntOpts, optMBallOpts;
int lastMaxNet, optCTF, optEnhancedCTF, optHoard, optTeamHoard, optEntropy, optMonsterball;
int optOpenGame, optClosedGame, optRestrictedGame;

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
if (!menu [0].m_value) {
	menu [0].m_value = 1;
	menu [0].m_bRebuild = 1;
	}
for (i = 1; i < int (menu.ToS ()); i++) {
	if ((i >= gameData.multiplayer.nPlayers) && menu [i].m_value) {
		menu [i].m_value = 0;
		menu [i].m_bRebuild = 1;
		}
	}
nm = 0;
for (i = 0; i < int (menu.ToS ()); i++) {
	if (menu [i].m_value) {
		if (++nm > gameData.multiplayer.nPlayers) {
			menu [i].m_value = 0;
			menu [i].m_bRebuild = 1;
			}
		}
	}
if (nm > gameData.multiplayer.nMaxPlayers) {
	MsgBox (TXT_ERROR, NULL, 1, TXT_OK, "%s %d %s", TXT_SORRY_ONLY, gameData.multiplayer.nMaxPlayers, TXT_NETPLAYERS_IN);
	// Turn off the last CPlayerData highlighted
	for (i = gameData.multiplayer.nPlayers; i > 0; i--)
		if (menu [i].m_value == 1) {
			menu [i].m_value = 0;
			menu [i].m_bRebuild = 1;
			break;
			}
	}

//       if (menu.ToS () > MAX_PLAYERS) return; 

n = netGame.m_info.nNumPlayers;
NetworkListen ();

if (n < netGame.m_info.nNumPlayers) {
	audio.PlaySound (SOUND_HUD_MESSAGE);
	if (gameOpts->multi.bNoRankings)
	   sprintf (menu [gameData.multiplayer.nPlayers - 1].m_text, "%d. %-20s", 
					gameData.multiplayer.nPlayers, 
					netPlayers [0].m_info.players [gameData.multiplayer.nPlayers-1].callsign);
	else
	   sprintf (menu [gameData.multiplayer.nPlayers - 1].m_text, "%d. %s%-20s", 
					gameData.multiplayer.nPlayers, 
					pszRankStrings [netPlayers [0].m_info.players [gameData.multiplayer.nPlayers-1].rank], 
					netPlayers [0].m_info.players [gameData.multiplayer.nPlayers-1].callsign);
	menu [gameData.multiplayer.nPlayers - 1].m_bRebuild = 1;
	if (gameData.multiplayer.nPlayers <= gameData.multiplayer.nMaxPlayers)
		menu [gameData.multiplayer.nPlayers - 1].m_value = 1;
	} 
else if (n > netGame.m_info.nNumPlayers) {
	// One got removed...
   audio.PlaySound (SOUND_HUD_KILL);
	for (i = 0; i < gameData.multiplayer.nPlayers; i++) {
		if (gameOpts->multi.bNoRankings)
			sprintf (menu [i].m_text, "%d. %-20s", i+1, netPlayers [0].m_info.players [i].callsign);
		else
			sprintf (menu [i].m_text, "%d. %s%-20s", i+1, pszRankStrings [netPlayers [0].m_info.players [i].rank], netPlayers [0].m_info.players [i].callsign);
		menu [i].m_value = (i < gameData.multiplayer.nMaxPlayers);
		menu [i].m_bRebuild = 1;
		}
	for (i = gameData.multiplayer.nPlayers; i<n; i++)  {
		sprintf (menu [i].m_text, "%d. ", i+1);          // Clear out the deleted entries...
		menu [i].m_value = 0;
		menu [i].m_bRebuild = 1;
		}
   }
return nCurItem;
}

//------------------------------------------------------------------------------

static int optGameTypes, nGameTypes, nGameItem;

int NetworkGameParamPoll (CMenu& menu, int& key, int nCurItem, int nState)
{
if (nState)
	return nCurItem;

	static int oldmaxnet = 0;

if ((nCurItem >= optGameTypes) && (nCurItem < optGameTypes + nGameTypes)) {
	if ((nCurItem != nGameItem) && (menu [nCurItem].m_value)) {
		nGameItem = nCurItem;
		key = -2;
		return nCurItem;
		}
	}
if (((optEntropy >= 0) && (menu [optEntropy].m_value == (optEntOpts < 0))) ||
	 ((optMonsterball >= 0) && (gameOpts->app.bExpertMode == SUPERUSER) && (menu [optMonsterball].m_value == (optMBallOpts < 0))))
	key = -2;
//force restricted game for team games
//obsolete with D2X-W32 as it can assign players to teams automatically
//even in a match and progress, and allows players to switch teams
if (menu [optCoop].m_value) {
	oldmaxnet = 1;
	if (menu [optMaxNet].m_value > 2)  {
		menu [optMaxNet].m_value = 2;
		menu [optMaxNet].m_bRedraw = 1;
		}
	if (menu [optMaxNet].m_maxValue > 2) {
		menu [optMaxNet].m_maxValue = 2;
		menu [optMaxNet].m_bRedraw = 1;
		}
	if (!(netGame.m_info.gameFlags & NETGAME_FLAG_SHOW_MAP))
		netGame.m_info.gameFlags |= NETGAME_FLAG_SHOW_MAP;
	if (netGame.GetPlayTimeAllowed () || netGame.GetScoreGoal ()) {
		netGame.SetPlayTimeAllowed (0);
		netGame.SetScoreGoal (0);
		}
	}
else {// if !Coop game
	if (oldmaxnet) {
		oldmaxnet = 0;
		menu [optMaxNet].m_value = 
		menu [optMaxNet].m_maxValue = MAX_NUM_NET_PLAYERS - 2;
		}
	}         
if (lastMaxNet != menu [optMaxNet].m_value)  {
	sprintf (menu [optMaxNet].m_text, TXT_MAX_PLAYERS, menu [optMaxNet].m_value + 2);
	lastMaxNet = menu [optMaxNet].m_value;
	menu [optMaxNet].m_bRebuild = 1;
	}               
return nCurItem;
}

//------------------------------------------------------------------------------

fix nLastPTA;
int nLastScoreGoal;

int optSetPower, optPlayTime, optScoreGoal, optSocket, optMarkerView, optLight;
int optDifficulty, optPPS, optShortPkts, optBrightPlayers, optStartInvul;
int optDarkness, optTeamDoors, optMultiCheats, optTgtInd;
int optHeadlights, optPowerupLights, optSpotSize, optSmokeGrenades, optMaxSmokeGrens;
int optShowNames, optAutoTeams, optRotateLevels, optDisableReactor;
int optMouseLook, optFastPitch, optSafeUDP, optTowFlags, optCompetition, optPenalty;
int optFixedSpawn, optSpawnDelay;
//int optDualMiss;

//------------------------------------------------------------------------------

int NetworkMoreOptionsPoll (CMenu& menu, int& key, int nCurItem, int nState)
{
if (nState)
	return nCurItem;

if (nLastReactorLife != menu [optReactorLife].m_value)   {
	sprintf (menu [optReactorLife].m_text, "%s: %d %s", TXT_REACTOR_LIFE, menu [optReactorLife].m_value*5, TXT_MINUTES_ABBREV);
	nLastReactorLife = menu [optReactorLife].m_value;
	menu [optReactorLife].m_bRebuild = 1;
   }
  
if ((optPlayTime >= 0) && (menu [optPlayTime].m_value != nLastPTA)) {
	nLastPTA = mpParams.nMaxTime;
	mpParams.nMaxTime = menu [optPlayTime].m_value;
	sprintf (menu [optPlayTime].m_text, TXT_MAXTIME, nLastPTA * 5, TXT_MINUTES_ABBREV);
	menu [optPlayTime].m_bRebuild = 1;
	}
if ((optScoreGoal >= 0) && (menu [optScoreGoal].m_value != nLastScoreGoal)) {
	nLastScoreGoal = mpParams.nScoreGoal;
	mpParams.nScoreGoal = menu [optScoreGoal].m_value;
	sprintf (menu [optScoreGoal].m_text, TXT_SCOREGOAL, mpParams.nScoreGoal * 5);
	menu [optScoreGoal].m_bRebuild = 1;
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
	optDifficulty = m.AddSlider (TXT_DIFFICULTY, mpParams.nDifficulty, 0, NDL - 1, KEY_D, HTX_GPLAY_DIFFICULTY); 
	sprintf (szInvul + 1, "%s: %d %s", TXT_REACTOR_LIFE, mpParams.nReactorLife * 5, TXT_MINUTES_ABBREV);
	strupr (szInvul + 1);
	*szInvul = * (TXT_REACTOR_LIFE - 1);
	optReactorLife = m.AddSlider (szInvul + 1, mpParams.nReactorLife, 0, 10, KEY_R, HTX_MULTI2_REACTOR); 
	if (IsCoopGame) {
		optPlayTime =
		optScoreGoal = -1;
		nLastPTA =
		nLastScoreGoal = 0;
		}
	else {
		sprintf (szPlayTime + 1, TXT_MAXTIME, mpParams.nMaxTime * 5, TXT_MINUTES_ABBREV);
		*szPlayTime = * (TXT_MAXTIME - 1);
		optPlayTime = m.AddSlider (szPlayTime + 1, mpParams.nMaxTime, 0, 10, KEY_T, HTX_MULTI2_LVLTIME); 
		sprintf (szScoreGoal + 1, TXT_SCOREGOAL, mpParams.nScoreGoal * 5);
		*szScoreGoal = * (TXT_SCOREGOAL - 1);
		optScoreGoal = m.AddSlider (szScoreGoal + 1, mpParams.nScoreGoal, 0, 10, KEY_K, HTX_MULTI2_SCOREGOAL);
		}
	optStartInvul = m.AddCheck (TXT_INVUL_RESPAWN, mpParams.bInvul, KEY_I, HTX_MULTI2_INVUL);
	optMarkerView = m.AddCheck (TXT_MARKER_CAMS, mpParams.bMarkerView, KEY_C, HTX_MULTI2_MARKERCAMS);
	optLight = m.AddCheck (TXT_KEEP_LIGHTS, mpParams.bIndestructibleLights, KEY_L, HTX_MULTI2_KEEPLIGHTS);
	optBrightPlayers = m.AddCheck (TXT_BRIGHT_SHIPS, mpParams.bBrightPlayers ? 0 : 1, KEY_S, HTX_MULTI2_BRIGHTSHIP);
	optShowNames = m.AddCheck (TXT_SHOW_NAMES, mpParams.bShowAllNames, KEY_E, HTX_MULTI2_SHOWNAMES);
	optPlayersOnMap = m.AddCheck (TXT_SHOW_PLAYERS, mpParams.bShowPlayersOnAutomap, KEY_A, HTX_MULTI2_SHOWPLRS);
	optShortPkts = m.AddCheck (TXT_SHORT_PACKETS, mpParams.bShortPackets, KEY_H, HTX_MULTI2_SHORTPKTS);
	m.AddText ("", 0);
	optSetPower = m.AddMenu (TXT_WAOBJECTS_MENU, KEY_O, HTX_MULTI2_OBJECTS);
	m.AddText ("", 0);
	sprintf (szSocket, "%d", (gameStates.multi.nGameType == UDP_GAME) ? mpParams.udpPorts [0] + networkData.nPortOffset : networkData.nPortOffset);
	if (gameStates.multi.nGameType >= IPX_GAME) {
		m.AddText (TXT_SOCKET2, KEY_N);
		optSocket = m.AddInput (szSocket, 5, HTX_MULTI2_SOCKET);
		}

	sprintf (szPPS, "%d", mpParams.nPPS);
	m.AddText (TXT_PPS, KEY_P);
	optPPS = m.AddInput (szPPS, 2, HTX_MULTI2_PPS);

	nLastScoreGoal = netGame.GetScoreGoal ();
	nLastPTA = mpParams.nMaxTime;

doMenu:

	gameStates.app.nExtGameStatus = GAMESTAT_MORE_NETGAME_OPTIONS; 
	i = m.Menu (NULL, TXT_MORE_MPOPTIONS, NetworkMoreOptionsPoll, &choice);
	} while (i == -2);

   //mpParams.nReactorLife = atoi (szInvul)*I2X (60);
mpParams.nReactorLife = m [optReactorLife].m_value;

if (i == optSetPower) {
	NetworkSetWeaponsAllowed ();
	GetAllAllowables ();
	goto doMenu;
	}

mpParams.bInvul = ubyte (m [optStartInvul].m_value);
mpParams.bBrightPlayers = ubyte (m [optBrightPlayers].m_value ? 0 : 1);
mpParams.bShowAllNames = ubyte (m [optShowNames].m_value);
mpParams.bMarkerView = ubyte (m [optMarkerView].m_value);
mpParams.bIndestructibleLights = ubyte (m [optLight].m_value);
mpParams.nDifficulty = m [optDifficulty].m_value;
mpParams.bShowPlayersOnAutomap = m [optPlayersOnMap].m_value;
mpParams.bShortPackets = m [optShortPkts].m_value;
mpParams.nPPS = atoi (szPPS);
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

v = menu [optCompetition].m_value;
if (v != extraGameInfo [1].bCompetition) {
	extraGameInfo [1].bCompetition = v;
	key = -2;
	return nCurItem;
	}

if (optPenalty > 0) {
	v = menu [optPenalty].m_value;
	if (v != extraGameInfo [1].nCoopPenalty) {
		extraGameInfo [1].nCoopPenalty = v;
		sprintf (menu [optPenalty].m_text, TXT_COOP_PENALTY, nCoopPenalties [v], '%');
		menu [optPenalty].m_bRebuild = 1;
		return nCurItem;
		}
	}

v = menu [optDarkness].m_value;
if (v != extraGameInfo [1].bDarkness) {
	extraGameInfo [1].bDarkness = v;
	key = -2;
	return nCurItem;
	}

if (optTgtInd >= 0) {
	v = menu [optTgtInd].m_value;
	if (v != (extraGameInfo [1].bTargetIndicators != 0)) {
		extraGameInfo [1].bTargetIndicators = v;
		key = -2;
		return nCurItem;
		}
	}

if (optHeadlights >= 0) {
	v = menu [optHeadlights].m_value;
	if (v == extraGameInfo [1].headlight.bAvailable) {
		extraGameInfo [1].headlight.bAvailable = !v;
		key = -2;
		return nCurItem;
		}
	}

if (optSpotSize >= 0) {
	v = menu [optSpotSize].m_value;
	if (v != extraGameInfo [1].nSpotSize) {
		extraGameInfo [1].nSpotSize =
		extraGameInfo [1].nSpotStrength = v;
		sprintf (menu [optSpotSize].m_text, TXT_SPOTSIZE, GT (664 + v));
		menu [optSpotSize].m_bRebuild = 1;
		return nCurItem;
		}
	}

v = menu [optSmokeGrenades].m_value;
if (extraGameInfo [0].bSmokeGrenades != v) {
	extraGameInfo [0].bSmokeGrenades = v;
	key = -2;
	return nCurItem;
	}

if (extraGameInfo [0].bSmokeGrenades && (optMaxSmokeGrens >= 0)) {
	v = menu [optMaxSmokeGrens].m_value;
	if (extraGameInfo [0].nMaxSmokeGrenades != v) {
		extraGameInfo [0].nMaxSmokeGrenades = v;
		sprintf (menu [optMaxSmokeGrens].m_text, TXT_MAX_SMOKEGRENS, extraGameInfo [0].nMaxSmokeGrenades);
		menu [optMaxSmokeGrens].m_bRebuild = 1;
		}
	}

if (optSpawnDelay >= 0) {
	v = menu [optSpawnDelay].m_value * 5;
	if (extraGameInfo [0].nSpawnDelay != v * 1000) {
		extraGameInfo [0].nSpawnDelay = v * 1000;
		sprintf (menu [optSpawnDelay].m_text, TXT_RESPAWN_DELAY, v);
		menu [optSpawnDelay].m_bRebuild = 1;
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
	optCompetition = m.AddCheck (TXT_COMPETITION_MODE, extraGameInfo [1].bCompetition, KEY_C, HTX_MULTI2_COMPETITION);
	if (!extraGameInfo [1].bCompetition) {
		optFF = m.AddCheck (TXT_FRIENDLY_FIRE, extraGameInfo [0].bFriendlyFire, KEY_F, HTX_MULTI2_FFIRE);
		optSuicide = m.AddCheck (TXT_NO_SUICIDE, extraGameInfo [0].bInhibitSuicide, KEY_U, HTX_MULTI2_SUICIDE);
		optMouseLook = m.AddCheck (TXT_MOUSELOOK, extraGameInfo [1].bMouseLook, KEY_O, HTX_MULTI2_MOUSELOOK);
		optFastPitch = m.AddCheck (TXT_FASTPITCH, (extraGameInfo [1].bFastPitch == 1) ? 1 : 0, KEY_P, HTX_MULTI2_FASTPITCH);
		//optDualMiss = m.AddCheck (TXT_DUAL_LAUNCH, extraGameInfo [1].bDualMissileLaunch, KEY_M, HTX_GPLAY_DUALLAUNCH);
		}
	else
		optFF =
		optSuicide =
		optMouseLook =
		optFastPitch = -1;
		//optDualMiss = -1;
	if (IsTeamGame) {
		optAutoTeams = m.AddCheck (TXT_AUTOBALANCE, extraGameInfo [0].bAutoBalanceTeams, KEY_B, HTX_MULTI2_BALANCE);
		optTeamDoors = m.AddCheck (TXT_TEAMDOORS, mpParams.bTeamDoors, KEY_T, HTX_TEAMDOORS);
		}
	else
		optTeamDoors =
		optAutoTeams = -1;
	if (mpParams.nGameMode == NETGAME_CAPTURE_FLAG)
		optTowFlags = m.AddCheck (TXT_TOW_FLAGS, extraGameInfo [1].bTowFlags, KEY_F, HTX_TOW_FLAGS);
	else
		optTowFlags = -1;
	if (!extraGameInfo [1].bCompetition)
		optMultiCheats = m.AddCheck (TXT_MULTICHEATS, mpParams.bEnableCheats, KEY_T, HTX_MULTICHEATS);
	else
		optMultiCheats = -1;
	optRotateLevels = m.AddCheck (TXT_MSN_CYCLE, extraGameInfo [1].bRotateLevels, KEY_Y, HTX_MULTI2_MSNCYCLE); 
#if 0
		optDisableReactor = mat.AddCheck (TXT_NO_REACTOR, extraGameInfo [1].bDisableReactor, KEY_R, HTX_MULTI2_NOREACTOR); 
#else
		optDisableReactor = -1;
#endif
#if UDP_SAFEMODE
	optSafeUDP = mat.AddCheck (TXT_UDP_QUAL, extraGameInfo [0].bSafeUDP, KEY_Q, HTX_MULTI2_UDPQUAL);
#endif
	optCheckPort = m.AddCheck (TXT_CHECK_PORT, extraGameInfo [0].bCheckUDPPort, KEY_P, HTX_MULTI2_CHECKPORT);
	if (extraGameInfo [1].bDarkness)
		m.AddText ("", 0);
	optDarkness = m.AddCheck (TXT_DARKNESS, extraGameInfo [1].bDarkness, KEY_D, HTX_DARKNESS);
	if (extraGameInfo [1].bDarkness) {
		optPowerupLights = m.AddCheck (TXT_POWERUPLIGHTS, !extraGameInfo [1].bPowerupLights, KEY_P, HTX_POWERUPLIGHTS);
		optHeadlights = m.AddCheck (TXT_HEADLIGHTS, !extraGameInfo [1].headlight.bAvailable, KEY_H, HTX_HEADLIGHTS);
		if (extraGameInfo [1].headlight.bAvailable) {
			sprintf (szSlider + 1, TXT_SPOTSIZE, GT (664 + extraGameInfo [1].nSpotSize));
			strupr (szSlider + 1);
			*szSlider = *(TXT_SPOTSIZE - 1);
			optSpotSize = m.AddSlider (szSlider + 1, extraGameInfo [1].nSpotSize, 0, 2, KEY_O, HTX_SPOTSIZE); 
			}
		else
			optSpotSize = -1;
		}
	else
		optHeadlights =
		optPowerupLights =
		optSpotSize = -1;
	if (!extraGameInfo [1].bCompetition) {
		if (mpParams.nGameMode == NETGAME_COOPERATIVE) {
			sprintf (szSlider + 1, TXT_COOP_PENALTY, nCoopPenalties [(int) extraGameInfo [1].nCoopPenalty], '%');
			strupr (szSlider + 1);
			*szSlider = *(TXT_COOP_PENALTY - 1);
			optPenalty = m.AddSlider (szSlider + 1, extraGameInfo [1].nCoopPenalty, 0, 9, KEY_O, HTX_COOP_PENALTY); 
			}
		else
			optPenalty = -1;
		optTgtInd = m.AddCheck (TXT_MULTI_TGTIND, extraGameInfo [1].bTargetIndicators != 0, KEY_A, HTX_CPIT_TGTIND);
		}
	else
		optTgtInd = -1;

	optSmokeGrenades = m.AddCheck (TXT_GPLAY_SMOKEGRENADES, extraGameInfo [0].bSmokeGrenades, KEY_S, HTX_GPLAY_SMOKEGRENADES);
	if (extraGameInfo [0].bSmokeGrenades) {
		sprintf (szSlider + 1, TXT_MAX_SMOKEGRENS, extraGameInfo [0].nMaxSmokeGrenades);
		*szSlider = *(TXT_MAX_SMOKEGRENS - 1);
		optMaxSmokeGrens = m.AddSlider (szSlider + 1, extraGameInfo [0].nMaxSmokeGrenades - 1, 0, 3, KEY_X, HTX_GPLAY_MAXGRENADES);
		}
	else
		optMaxSmokeGrens = -1;
	optFixedSpawn = m.AddCheck (TXT_FIXED_SPAWN, extraGameInfo [0].bFixedRespawns, KEY_F, HTX_GPLAY_FIXEDSPAWN);
	if (extraGameInfo [0].nSpawnDelay < 0)
		extraGameInfo [0].nSpawnDelay = 0;
	sprintf (szSlider + 1, TXT_RESPAWN_DELAY, extraGameInfo [0].nSpawnDelay / 1000);
	*szSlider = *(TXT_RESPAWN_DELAY - 1);
	optSpawnDelay = m.AddSlider (szSlider + 1, extraGameInfo [0].nSpawnDelay / 5000, 0, 12, KEY_R, HTX_GPLAY_SPAWNDELAY);
	m.AddText ("", 0);

	i = m.Menu (NULL, TXT_D2XOPTIONS_TITLE, NetworkD2XOptionsPoll, &choice);
  //mpParams.nReactorLife = atoi (szInvul)*I2X (60);
	extraGameInfo [1].bDarkness = (ubyte) m [optDarkness].m_value;
	if (optDarkness >= 0) {
		if ((mpParams.bDarkness = extraGameInfo [1].bDarkness)) {
			extraGameInfo [1].headlight.bAvailable = !m [optHeadlights].m_value;
			extraGameInfo [1].bPowerupLights = !m [optPowerupLights].m_value;
			}
		}
	GET_VAL (extraGameInfo [1].bTowFlags, optTowFlags);
	GET_VAL (extraGameInfo [1].bTeamDoors, optTeamDoors);
	mpParams.bTeamDoors = extraGameInfo [1].bTeamDoors;
	if (optAutoTeams >= 0)
		extraGameInfo [0].bAutoBalanceTeams = (m [optAutoTeams].m_value != 0);
	#if UDP_SAFEMODE
	extraGameInfo [0].bSafeUDP = (mat [optSafeUDP].m_value != 0);
	#endif
	extraGameInfo [0].bCheckUDPPort = (m [optCheckPort].m_value != 0);
	extraGameInfo [1].bRotateLevels = m [optRotateLevels].m_value;
	if (!COMPETITION) {
		GET_VAL (extraGameInfo [1].bEnableCheats, optMultiCheats);
		mpParams.bEnableCheats = extraGameInfo [1].bEnableCheats;
		GET_VAL (extraGameInfo [0].bFriendlyFire, optFF);
		GET_VAL (extraGameInfo [0].bInhibitSuicide, optSuicide);
		GET_VAL (extraGameInfo [1].bMouseLook, optMouseLook);
		if (optFastPitch >= 0)
			extraGameInfo [1].bFastPitch = m [optFastPitch].m_value ? 1 : 2;
		//GET_VAL (extraGameInfo [1].bDualMissileLaunch, optDualMiss);
		GET_VAL (extraGameInfo [1].bDisableReactor, optDisableReactor);
		GET_VAL (extraGameInfo [1].bTargetIndicators, optTgtInd);
		GET_VAL (extraGameInfo [1].bFixedRespawns, optFixedSpawn);
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
		v = menu [i].m_value;
		if (v != extraGameInfo [0].loadout.nMissiles [h]) {
			extraGameInfo [0].loadout.nMissiles [h] = v;
			sprintf (menu [i].m_text, szMslNames [i], v);
			menu [i].m_bRebuild = true;
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
		   
if (m [optAnarchy].m_value)
	mpParams.nGameMode = NETGAME_ANARCHY;
else if (m [optTeamAnarchy].m_value)
	mpParams.nGameMode = NETGAME_TEAM_ANARCHY;
else if (m [optCTF].m_value) {
	mpParams.nGameMode = NETGAME_CAPTURE_FLAG;
	extraGameInfo [0].bEnhancedCTF = 0;
	}
else if (m [optEnhancedCTF].m_value) {
	mpParams.nGameMode = NETGAME_CAPTURE_FLAG;
	extraGameInfo [0].bEnhancedCTF = 1;
	}
else if (bHoard && m [optHoard].m_value)
	mpParams.nGameMode = NETGAME_HOARD;
else if (bHoard && m [optTeamHoard].m_value)
	mpParams.nGameMode = NETGAME_TEAM_HOARD;
else if (bHoard && m [optEntropy].m_value)
	mpParams.nGameMode = NETGAME_ENTROPY;
else if (bHoard && m [optMonsterball].m_value)
	mpParams.nGameMode = NETGAME_MONSTERBALL;
else if (bAnarchyOnly) {
	MsgBox (NULL, NULL, 1, TXT_OK, TXT_ANARCHY_ONLY_MISSION);
	m [optAnarchy].m_value = 1;
	m [optRobotAnarchy].m_value =
	m [optCoop].m_value = 0;
	return 0;
	}               
else if (m [optRobotAnarchy].m_value) 
	mpParams.nGameMode = NETGAME_ROBOT_ANARCHY;
else if (m [optCoop].m_value) 
	mpParams.nGameMode = NETGAME_COOPERATIVE;
return 1;
}

//------------------------------------------------------------------------------

static int optGameName, optLevel, optLevelText, optMoreOpts, optMission, optMissionName, optD2XOpts, optConfigMenu, optLoadoutMenu, optMissileMenu;

void BuildGameParamsMenu (CMenu& m, char* szName, char* szLevelText, char* szLevel, char* szIpAddr, char* szMaxNet, int nNewMission)
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
optGameName = m.AddInput (szName, NETGAME_NAME_LEN, HTX_MULTI_NAME); 
optMission = m.AddMenu (TXT_SEL_MISSION, KEY_I, HTX_MULTI_MISSION);
optMissionName = m.AddText ("", 0);
m [optMissionName].m_bRebuild = 1; 
if ((nNewMission >= 0) && (missionManager.nLastLevel > 1)) {
	optLevelText = m.AddText (szLevelText, 0); 
	optLevel = m.AddInput (szLevel, 4, HTX_MULTI_LEVEL);
	}
else
	optLevelText = -1;
m.AddText ("", 0); 
optGameTypes = 
optAnarchy = m.AddRadio (TXT_ANARCHY, 0, KEY_A, HTX_MULTI_ANARCHY);
optTeamAnarchy = m.AddRadio (TXT_TEAM_ANARCHY, 0, KEY_T, HTX_MULTI_TEAMANA);
optRobotAnarchy = m.AddRadio (TXT_ANARCHY_W_ROBOTS, 0, KEY_R, HTX_MULTI_BOTANA);
optCoop = m.AddRadio (TXT_COOP, 0, KEY_P, HTX_MULTI_COOP);
optCTF = m.AddRadio (TXT_CTF, 0, KEY_F, HTX_MULTI_CTF);
if (!gameStates.app.bNostalgia)
	optEnhancedCTF = m.AddRadio (TXT_CTF_PLUS, 0, KEY_T, HTX_MULTI_CTFPLUS);

optEntropy =
optMonsterball = -1;
if (bHoard) {
	optHoard = m.AddRadio (TXT_HOARD, 0, KEY_H, HTX_MULTI_HOARD);
	optTeamHoard = m.AddRadio (TXT_TEAM_HOARD, 0, KEY_O, HTX_MULTI_TEAMHOARD);
	if (!gameStates.app.bNostalgia) {
		optEntropy = m.AddRadio (TXT_ENTROPY, 0, KEY_Y, HTX_MULTI_ENTROPY);
		optMonsterball = m.AddRadio (TXT_MONSTERBALL, 0, KEY_B, HTX_MULTI_MONSTERBALL);
		}
	} 
nGameTypes = m.ToS () - 1 - optGameTypes;
m [optGameTypes + NMCLAMP (mpParams.nGameType, 0, nGameTypes)].m_value = 1;

m.AddText ("", 0); 

optOpenGame = m.AddRadio (TXT_OPEN_GAME, 0, KEY_O, HTX_MULTI_OPENGAME);
optClosedGame = m.AddRadio (TXT_CLOSED_GAME, 0, KEY_C, HTX_MULTI_CLOSEDGAME);
optRestrictedGame = m.AddRadio (TXT_RESTR_GAME, 0, KEY_R, HTX_MULTI_RESTRGAME);
m [optOpenGame + NMCLAMP (mpParams.nGameAccess, 0, 2)].m_value = 1;
m.AddText ("", 0);
sprintf (szMaxNet + 1, TXT_MAX_PLAYERS, gameData.multiplayer.nMaxPlayers);
*szMaxNet = *(TXT_MAX_PLAYERS - 1);
lastMaxNet = gameData.multiplayer.nMaxPlayers - 2;
optMaxNet = m.AddSlider (szMaxNet + 1, lastMaxNet, 0, lastMaxNet, KEY_X, HTX_MULTI_MAXPLRS); 
m.AddText ("", 0);
optMoreOpts = m.AddMenu (TXT_MORE_OPTS, KEY_M, HTX_MULTI_MOREOPTS);
optConfigMenu =
optD2XOpts =
optEntOpts =
optMBallOpts = -1;
if (!gameStates.app.bNostalgia) {
	optD2XOpts = m.AddMenu (TXT_MULTI_D2X_OPTS, KEY_X, HTX_MULTI_D2XOPTS);
	if ((optEntropy >= 0) && m [optEntropy].m_value)
		optEntOpts = m.AddMenu (TXT_ENTROPY_OPTS, KEY_E, HTX_MULTI_ENTOPTS);
	if (m [optMonsterball].m_value && (gameOpts->app.bExpertMode == SUPERUSER))
		optMBallOpts = m.AddMenu (TXT_MONSTERBALL_OPTS, KEY_O, HTX_MULTI_MBALLOPTS);
	else
		InitMonsterballSettings (&extraGameInfo [0].monsterball);
	optConfigMenu = m.AddMenu (TXT_GAME_OPTIONS, KEY_O, HTX_MAIN_CONF);
	optLoadoutMenu = m.AddMenu (TXT_LOADOUT_OPTION, KEY_L, HTX_MULTI_LOADOUT);
	optMissileMenu = m.AddMenu (TXT_MISSILE_LOADOUT, KEY_I, HTX_MISSILE_LOADOUT);
	}
}

//------------------------------------------------------------------------------

int GameParamsMenu (CMenu& m, int& key, int& choice, char* szName, char* szLevelText, char* szLevel, char* szIpAddr, int& nNewMission)
{
	int i, bAnarchyOnly = (nNewMission < 0) ? 0 : missionManager [nNewMission].bAnarchyOnly;

if (m [optMissionName].m_bRebuild) {
	strncpy (netGame.m_info.szMissionName, 
				(nNewMission < 0) ? "" : missionManager [nNewMission].filename, 
				sizeof (netGame.m_info.szMissionName) - 1);
	m [optMissionName].SetText ((nNewMission < 0) ? const_cast<char*> (TXT_NONE_SELECTED) : const_cast<char*> (missionManager [nNewMission].szMissionName));
	if ((nNewMission >= 0) && (missionManager.nLastLevel > 1)) {
		sprintf (szLevelText, "%s (1-%d)", TXT_LEVEL_, missionManager.nLastLevel);
		Assert (strlen (szLevelText) < 32);
		m [optLevelText].m_bRebuild = 1;
		}
	mpParams.nLevel = 1;
	}

gameStates.app.nExtGameStatus = GAMESTAT_NETGAME_OPTIONS; 
key = m.Menu (NULL, (gameStates.multi.nGameType == UDP_GAME) ? szIpAddr : NULL, NetworkGameParamPoll, &choice);
								//TXT_NETGAME_SETUP
if (key == -1)
	return -1;
else if (choice == optMoreOpts) {
	if (m [optGameTypes + 3].m_value)
		gameData.app.nGameMode = GM_MULTI_COOP;
	NetworkMoreGameOptions ();
	gameData.app.nGameMode = 0;
	if (gameStates.multi.nGameType == UDP_GAME) {
		sprintf (szIpAddr, "Game Host: %d.%d.%d.%d:%d", 
					ipx_MyAddress [4], ipx_MyAddress [5], ipx_MyAddress [6], ipx_MyAddress [7], mpParams.udpPorts [0]);
		}
	return 1;
	}
else if (!gameStates.app.bNostalgia && (optD2XOpts >= 0) && (choice == optD2XOpts)) {
	NetworkGetGameType (m, bAnarchyOnly);
	NetworkD2XOptions ();
	return 1;
	}
else if (!gameStates.app.bNostalgia && (optEntOpts >= 0) && (choice == optEntOpts)) {
	NetworkEntropyOptions ();
	return 1;
	}
else if (!gameStates.app.bNostalgia && (optMBallOpts >= 0) && (choice == optMBallOpts)) {
	NetworkMonsterballOptions ();
	return 1;
	}
else if (!gameStates.app.bNostalgia && (optConfigMenu >= 0) && (choice == optConfigMenu)) {
	ConfigMenu ();
	return 1;
	}
else if (!gameStates.app.bNostalgia && (optLoadoutMenu >= 0) && (choice == optLoadoutMenu)) {
	LoadoutOptionsMenu ();
	return 1;
	}
else if (!gameStates.app.bNostalgia && (optMissileMenu >= 0) && (choice == optMissileMenu)) {
	MissileLoadoutMenu ();
	return 1;
	}
else if (choice == optMission) {
	int h = SelectAndLoadMission (1, &bAnarchyOnly);
	if (h < 0)
		return 1;
	missionManager.nLastMission = nNewMission = h;
	m [optMissionName].m_bRebuild = 1;
	return 2;
	}

if (key != -1) {
	int j;
		   
	gameData.multiplayer.nMaxPlayers = m [optMaxNet].m_value + 2;
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

	for (i = optGameTypes; i < optOpenGame; i++)
		if (m [i].m_value) {
			mpParams.nGameType = i - optGameTypes;
			break;
			}

	for (i = optOpenGame; i < optMaxNet; i++)
		if (m [i].m_value) {
			mpParams.nGameAccess = i - optOpenGame;
			break;
			}

	if (!NetworkGetGameType (m, bAnarchyOnly))
		return 1;
	if (m [optClosedGame].m_value)
		netGame.m_info.gameFlags |= NETGAME_FLAG_CLOSED;
	netGame.m_info.bRefusePlayers = m [optRestrictedGame].m_value;
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
	char	szName [NETGAME_NAME_LEN+1];
	char	szLevelText [32];
	char	szMaxNet [50];
	char	szIpAddr [80];
	char	szLevel [5];

	int nNewMission = missionManager.nLastMission;

gameOpts->app.bSinglePlayer = 0;
SetAllAllowablesTo (mpParams.nWeaponFilter);
networkData.nNamesInfoSecurity = -1;

for (i = 0; i < MAX_PLAYERS; i++)
	if (i!= gameData.multiplayer.nLocalPlayer)
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
*szMaxNet =
*szIpAddr =
*szLevel = '\0';

do {
	BuildGameParamsMenu (m, szName, szLevelText, szLevel, szIpAddr, szMaxNet, nNewMission);
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
	extraGameInfo [0].bSafeUDP = (mat [optSafeUDP].m_value != 0);
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
	if (menu [0].m_value != v) {
		menu [0].m_value = v;
		menu [0].m_bRebuild = 1;
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
	int	optServer = -1, optCheckPort, optPort [2] = {-1, -1};
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
	optServer = m.AddInput (mpParams.szServerIpAddr, sizeof (mpParams.szServerIpAddr) - 1, HTX_GETIP_SERVER);
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
	optPort [i] = m.AddInput (szPort [i], sizeof (szPort [i]) - 1, HTX_GETIP_CLIENT);
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
	extraGameInfo [0].bCheckUDPPort = m [optCheckPort].m_value != 0;
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
if ((choice == -1) || !*m [1].m_text)
	return;
MsgBox (TXT_SORRY, NULL, 1, TXT_OK, TXT_INV_ADDRESS);
}

//------------------------------------------------------------------------------

