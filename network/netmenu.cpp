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

#include "inferno.h"
#include "strutil.h"
#include "args.h"
#include "timer.h"
#include "ipx.h"
#include "ipx_udp.h"
#include "newmenu.h"
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
#include "menubackground.h"

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
  m.AddCheck (TXT_WA_LASER, netGame.DoLaserUpgrade, 0, NULL);
  m.AddCheck (TXT_WA_SLASER, netGame.DoSuperLaser, 0, NULL);
  m.AddCheck (TXT_WA_QLASER, netGame.DoQuadLasers, 0, NULL);
  m.AddCheck (TXT_WA_VULCAN, netGame.DoVulcan, 0, NULL);
  m.AddCheck (TXT_WA_SPREAD, netGame.DoSpread, 0, NULL);
  m.AddCheck (TXT_WA_PLASMA, netGame.DoPlasma, 0, NULL);
  m.AddCheck (TXT_WA_FUSION, netGame.DoFusions, 0, NULL);
  m.AddCheck (TXT_WA_GAUSS, netGame.DoGauss, 0, NULL);
  m.AddCheck (TXT_WA_HELIX, netGame.DoHelix, 0, NULL);
  m.AddCheck (TXT_WA_PHOENIX, netGame.DoPhoenix, 0, NULL);
  m.AddCheck (TXT_WA_OMEGA, netGame.DoOmega, 0, NULL);
  
  optSecond = m.ToS ();  
  m.AddCheck (TXT_WA_HOMING_MSL, netGame.DoHoming, 0, NULL);
  m.AddCheck (TXT_WA_PROXBOMB, netGame.DoProximity, 0, NULL);
  m.AddCheck (TXT_WA_SMART_MSL, netGame.DoSmarts, 0, NULL);
  m.AddCheck (TXT_WA_MEGA_MSL, netGame.DoMegas, 0, NULL);
  m.AddCheck (TXT_WA_FLASH_MSL, netGame.DoFlash, 0, NULL);
  m.AddCheck (TXT_WA_GUIDED_MSL, netGame.DoGuided, 0, NULL);
  m.AddCheck (TXT_WA_SMARTMINE, netGame.DoSmartMine, 0, NULL);
  m.AddCheck (TXT_WA_MERC_MSL, netGame.DoMercury, 0, NULL);
  m.AddCheck (TXT_WA_SHAKER_MSL, netGame.DoEarthShaker, 0, NULL);

  opt_power = opt;
  m.AddCheck (TXT_WA_INVUL, netGame.DoInvulnerability, 0, NULL);
  m.AddCheck (TXT_WA_CLOAK, netGame.DoCloak, 0, NULL);
  m.AddCheck (TXT_WA_BURNER, netGame.DoAfterburner, 0, NULL);
  m.AddCheck (TXT_WA_AMMORACK, netGame.DoAmmoRack, 0, NULL);
  m.AddCheck (TXT_WA_CONVERTER, netGame.DoConverter, 0, NULL);
  m.AddCheck (TXT_WA_HEADLIGHT, netGame.DoHeadlight, 0, NULL);
  
  choice = m.Menu (NULL, TXT_WA_OBJECTS);

  netGame.DoLaserUpgrade = m [optPrimary].m_value; 
  netGame.DoSuperLaser = m [optPrimary+1].m_value;
  netGame.DoQuadLasers = m [optPrimary+2].m_value;  
  netGame.DoVulcan = m [optPrimary+3].m_value;
  netGame.DoSpread = m [optPrimary+4].m_value;
  netGame.DoPlasma = m [optPrimary+5].m_value;
  netGame.DoFusions = m [optPrimary+6].m_value;
  netGame.DoGauss = m [optPrimary+7].m_value;
  netGame.DoHelix = m [optPrimary+8].m_value;
  netGame.DoPhoenix = m [optPrimary+9].m_value;
  netGame.DoOmega = m [optPrimary+10].m_value;
  
  netGame.DoHoming = m [optSecond].m_value;
  netGame.DoProximity = m [optSecond+1].m_value;
  netGame.DoSmarts = m [optSecond+2].m_value;
  netGame.DoMegas = m [optSecond+3].m_value;
  netGame.DoFlash = m [optSecond+4].m_value;
  netGame.DoGuided = m [optSecond+5].m_value;
  netGame.DoSmartMine = m [optSecond+6].m_value;
  netGame.DoMercury = m [optSecond+7].m_value;
  netGame.DoEarthShaker = m [optSecond+8].m_value;

  netGame.DoInvulnerability = m [opt_power].m_value;
  netGame.DoCloak = m [opt_power+1].m_value;
  netGame.DoAfterburner = m [opt_power+2].m_value;
  netGame.DoAmmoRack = m [opt_power+3].m_value;
  netGame.DoConverter = m [opt_power+4].m_value;     
  netGame.DoHeadlight = m [opt_power+5].m_value;     
  
 }

//------------------------------------------------------------------------------

static int optFF, optSuicide, optPlrHand, optTogglesMenu, optTextureMenu;

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
   
netGame.DoMegas = WF (0);
netGame.DoSmarts = WF (1);
netGame.DoFusions = WF (2);
netGame.DoHelix = WF (3);
netGame.DoPhoenix = WF (4);
netGame.DoCloak = WF (5);
netGame.DoInvulnerability = WF (6);
netGame.DoAfterburner = WF (7);
netGame.DoGauss = WF (8);
netGame.DoVulcan = WF (9);
netGame.DoPlasma = WF (10);
netGame.DoOmega = WF (11);
netGame.DoSuperLaser = WF (12);
netGame.DoProximity = WF (13);
netGame.DoSpread = WF (14);
netGame.DoMercury = WF (15);
netGame.DoSmartMine = WF (16);
netGame.DoFlash = WF (17);
netGame.DoGuided = WF (18);
netGame.DoEarthShaker = WF (19);
netGame.DoConverter = WF (20);
netGame.DoAmmoRack = WF (21);
netGame.DoHeadlight = WF (22);
netGame.DoHoming = WF (23);
netGame.DoLaserUpgrade = WF (24);
netGame.DoQuadLasers = WF (25);
netGame.BrightPlayers = !WF (26);
netGame.invul = WF (27);
}

#undef WF

//------------------------------------------------------------------------------

#define	WF(_w, _f)	if (_w) mpParams.nWeaponFilter |= (1 << (_f))

void GetAllAllowables (void)
{
mpParams.nWeaponFilter = 0;
WF (netGame.DoMegas, 0);
WF (netGame.DoSmarts, 1);
WF (netGame.DoFusions, 2);
WF (netGame.DoHelix, 3);
WF (netGame.DoPhoenix, 4);
WF (netGame.DoCloak, 5);
WF (netGame.DoInvulnerability, 6);
WF (netGame.DoAfterburner, 7);
WF (netGame.DoGauss, 8);
WF (netGame.DoVulcan, 9);
WF (netGame.DoPlasma, 10);
WF (netGame.DoOmega, 11);
WF (netGame.DoSuperLaser, 12);
WF (netGame.DoProximity, 13);
WF (netGame.DoSpread, 14);
WF (netGame.DoMercury, 15);
WF (netGame.DoSmartMine, 16);
WF (netGame.DoFlash, 17);
WF (netGame.DoGuided, 18);
WF (netGame.DoEarthShaker, 19);
WF (netGame.DoConverter, 20);
WF (netGame.DoAmmoRack, 21);
WF (netGame.DoHeadlight, 22);
WF (netGame.DoHoming, 23);
WF (netGame.DoLaserUpgrade, 24);
WF (netGame.DoQuadLasers, 25);
WF (!netGame.BrightPlayers, 26);
WF (netGame.invul, 27);
}

#undef WF

//------------------------------------------------------------------------------

#define ENDLEVEL_SEND_INTERVAL  2000
#define ENDLEVEL_IDLE_TIME      20000

int NetworkEndLevelPoll2 (CMenu& menu, int& key, int nCurItem)
{
	// Polling loop for End-of-level menu

	static fix t1 = 0;
	int i = 0;
	int num_ready = 0;
	int goto_secret = 0;
	fix t;

	// Send our endlevel packet at regular intervals
if ((t = SDL_GetTicks ()) > (t1 + ENDLEVEL_SEND_INTERVAL)) {
	NetworkSendEndLevelPacket ();
	t1 = t;
	}
NetworkListen ();
for (i = 0; i < gameData.multiplayer.nPlayers; i++) {
	if ((gameData.multiplayer.players [i].connected != 1) && 
		 (gameData.multiplayer.players [i].connected != 5) && 
		 (gameData.multiplayer.players [i].connected != 6))
		num_ready++;
	if (gameData.multiplayer.players [i].connected == 4)
		goto_secret = 1;                                        
	}
if (num_ready == gameData.multiplayer.nPlayers) {// All players have checked in or are disconnected
	if (goto_secret)
		key = -3;
	else
		key = -2;
	}
return nCurItem;
}

//------------------------------------------------------------------------------

int NetworkEndLevelPoll3 (CMenu& menu, int& key, int nCurItem)
{
	// Polling loop for End-of-level menu
   int num_ready = 0, i;
 
if (TimerGetApproxSeconds () > (gameData.multiplayer.xStartAbortMenuTime+ (F1_0 * 8)))
	key = -2;
NetworkListen ();
for (i = 0; i < gameData.multiplayer.nPlayers; i++)
	if ((gameData.multiplayer.players [i].connected != 1) && 
		 (gameData.multiplayer.players [i].connected != 5) && 
		 (gameData.multiplayer.players [i].connected != 6))
		num_ready++;
if (num_ready == gameData.multiplayer.nPlayers) // All players have checked in or are disconnected
	key = -2;
return nCurItem;
}

//------------------------------------------------------------------------------

int NetworkStartPoll (CMenu& menu, int& key, int nCurItem)
{
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

n = netGame.nNumPlayers;
NetworkListen ();

if (n < netGame.nNumPlayers) {
	DigiPlaySample (SOUND_HUD_MESSAGE, F1_0);
	if (gameOpts->multi.bNoRankings)
	   sprintf (menu [gameData.multiplayer.nPlayers - 1].m_text, "%d. %-20s", 
					gameData.multiplayer.nPlayers, 
					netPlayers.players [gameData.multiplayer.nPlayers-1].callsign);
	else
	   sprintf (menu [gameData.multiplayer.nPlayers - 1].m_text, "%d. %s%-20s", 
					gameData.multiplayer.nPlayers, 
					pszRankStrings [netPlayers.players [gameData.multiplayer.nPlayers-1].rank], 
					netPlayers.players [gameData.multiplayer.nPlayers-1].callsign);
	menu [gameData.multiplayer.nPlayers - 1].m_bRebuild = 1;
	if (gameData.multiplayer.nPlayers <= gameData.multiplayer.nMaxPlayers)
		menu [gameData.multiplayer.nPlayers - 1].m_value = 1;
	} 
else if (n > netGame.nNumPlayers) {
	// One got removed...
   DigiPlaySample (SOUND_HUD_KILL, F1_0);
	for (i = 0; i < gameData.multiplayer.nPlayers; i++) {
		if (gameOpts->multi.bNoRankings)
			sprintf (menu [i].m_text, "%d. %-20s", i+1, netPlayers.players [i].callsign);
		else
			sprintf (menu [i].m_text, "%d. %s%-20s", i+1, pszRankStrings [netPlayers.players [i].rank], netPlayers.players [i].callsign);
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

int NetworkGameParamPoll (CMenu& menu, int& key, int nCurItem)
{
	static int oldmaxnet = 0;

if ((nCurItem >= optGameTypes) && (nCurItem < optGameTypes + nGameTypes)) {
	if ((nCurItem != nGameItem) && (menu [nCurItem].m_value)) {
		nGameItem = nCurItem;
		key = -2;
		return nCurItem;
		}
	}
if ((menu [optEntropy].m_value == (optEntOpts < 0)) ||
	 (menu [optMonsterball].m_value == (optMBallOpts < 0)))
	key = -2;
//force restricted game for team games
//obsolete with D2X-W32 as it can assign players to teams automatically
//even in a match and progress, and allows players to switch teams
if (menu [optCoop].m_value) {
	oldmaxnet = 1;
	if (menu [optMaxNet].m_value>2)  {
		menu [optMaxNet].m_value = 2;
		menu [optMaxNet].m_bRedraw = 1;
		}
	if (menu [optMaxNet].m_maxValue > 2) {
		menu [optMaxNet].m_maxValue = 2;
		menu [optMaxNet].m_bRedraw = 1;
		}
	if (!(netGame.gameFlags & NETGAME_FLAG_SHOW_MAP))
		netGame.gameFlags |= NETGAME_FLAG_SHOW_MAP;
	if (netGame.xPlayTimeAllowed || netGame.KillGoal) {
		netGame.xPlayTimeAllowed = 0;
		netGame.KillGoal = 0;
		}
	}
else {// if !Coop game
	if (oldmaxnet) {
		oldmaxnet = 0;
		menu [optMaxNet].m_value = 6;
		menu [optMaxNet].m_maxValue = 6;
		}
	}         
if (lastMaxNet != menu [optMaxNet].m_value)  {
	sprintf (menu [optMaxNet].m_text, TXT_MAX_PLAYERS, menu [optMaxNet].m_value+2);
	lastMaxNet = menu [optMaxNet].m_value;
	menu [optMaxNet].m_bRebuild = 1;
	}               
return nCurItem;
}

//------------------------------------------------------------------------------

fix LastPTA;
int LastKillGoal;

int optSetPower, optPlayTime, optKillGoal, optSocket, optMarkerView, optLight;
int optDifficulty, optPPS, optShortPkts, optBrightPlayers, optStartInvul;
int optDarkness, optTeamDoors, optMultiCheats, optTgtInd;
int optDmgIndicator, optMslLockIndicator, optFriendlyIndicator, optHitIndicator;
int optHeadlights, optPowerupLights, optSpotSize;
int optShowNames, optAutoTeams, optDualMiss, optRotateLevels, optDisableReactor;
int optMouseLook, optFastPitch, optSafeUDP, optTowFlags, optCompetition, optPenalty;

//------------------------------------------------------------------------------

int NetworkMoreOptionsPoll (CMenu& menu, int& key, int nCurItem)
{
if (nLastReactorLife != menu [optReactorLife].m_value)   {
	sprintf (menu [optReactorLife].m_text, "%s: %d %s", TXT_REACTOR_LIFE, menu [optReactorLife].m_value*5, TXT_MINUTES_ABBREV);
	nLastReactorLife = menu [optReactorLife].m_value;
	menu [optReactorLife].m_bRebuild = 1;
   }
  
if ((optPlayTime >= 0) && (menu [optPlayTime].m_value != LastPTA)) {
	netGame.xPlayTimeAllowed = mpParams.nMaxTime = menu [optPlayTime].m_value;
	sprintf (menu [optPlayTime].m_text, TXT_MAXTIME, netGame.xPlayTimeAllowed * 5, TXT_MINUTES_ABBREV);
	LastPTA = netGame.xPlayTimeAllowed;
	menu [optPlayTime].m_bRebuild = 1;
	}
if ((optKillGoal >= 0) && (menu [optKillGoal].m_value != LastKillGoal)) {
	mpParams.nKillGoal = netGame.KillGoal = menu [optKillGoal].m_value;
	sprintf (menu [optKillGoal].m_text, TXT_KILLGOAL, netGame.KillGoal*5);
	LastKillGoal = netGame.KillGoal;
	menu [optKillGoal].m_bRebuild = 1;
	}
return nCurItem;
}

//------------------------------------------------------------------------------

void NetworkMoreGameOptions (void)
 {
  int		i, choice = 0;
  char	szPlayTime [80], szKillGoal [80], szInvul [50],
			socket_string [6], packstring [6];
  CMenu	m;

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
		optKillGoal = -1;
		LastPTA =
		LastKillGoal = 0;
		}
	else {
		sprintf (szPlayTime + 1, TXT_MAXTIME, netGame.xPlayTimeAllowed*5, TXT_MINUTES_ABBREV);
		*szPlayTime = * (TXT_MAXTIME - 1);
		optPlayTime = m.AddSlider (szPlayTime + 1, mpParams.nMaxTime, 0, 10, KEY_T, HTX_MULTI2_LVLTIME); 
		sprintf (szKillGoal + 1, TXT_KILLGOAL, netGame.KillGoal * 5);
		*szKillGoal = * (TXT_KILLGOAL - 1);
		optKillGoal = m.AddSlider (szKillGoal + 1, mpParams.nKillGoal, 0, 10, KEY_K, HTX_MULTI2_KILLGOAL);
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
	sprintf (socket_string, "%d", (gameStates.multi.nGameType == UDP_GAME) ? 
				udpBasePorts [1] + networkData.nSocket : networkData.nSocket);
	if (gameStates.multi.nGameType >= IPX_GAME) {
		m.AddText (TXT_SOCKET2, KEY_N);
		optSocket = m.AddInput (socket_string, 5, HTX_MULTI2_SOCKET);
		}

	sprintf (packstring, "%d", mpParams.nPPS);
	m.AddText (TXT_PPS, KEY_P);
	optPPS = m.AddInput (packstring, 2, HTX_MULTI2_PPS);

	LastKillGoal = netGame.KillGoal;
	LastPTA = mpParams.nMaxTime;

doMenu:

	gameStates.app.nExtGameStatus = GAMESTAT_MORE_NETGAME_OPTIONS; 
	i = m.Menu (NULL, TXT_MORE_MPOPTIONS, NetworkMoreOptionsPoll, &choice);
	} while (i == -2);

   //mpParams.nReactorLife = atoi (szInvul)*60*F1_0;
mpParams.nReactorLife = m [optReactorLife].m_value;
netGame.controlInvulTime = mpParams.nReactorLife * 5 * F1_0 * 60;

if (i == optSetPower) {
	NetworkSetWeaponsAllowed ();
	GetAllAllowables ();
	goto doMenu;
	}
mpParams.nPPS = atoi (packstring);
if (mpParams.nPPS > 20) {
	mpParams.nPPS = 20;
	MsgBox (TXT_ERROR, NULL, 1, TXT_OK, TXT_PPS_HIGH_ERROR);
}
else if (mpParams.nPPS<2) {
	MsgBox (TXT_ERROR, NULL, 1, TXT_OK, TXT_PPS_HIGH_ERROR);
	mpParams.nPPS = 2;      
}
netGame.nPacketsPerSec = mpParams.nPPS;
if (gameStates.multi.nGameType >= IPX_GAME) { 
	int newSocket = atoi (socket_string);
	if ((newSocket < -0xFFFF) || (newSocket > 0xFFFF))
		MsgBox (TXT_ERROR, NULL, 1, TXT_OK, 
							 TXT_INV_SOCKET, 
							 (gameStates.multi.nGameType == UDP_GAME) ? udpBasePorts [1] : networkData.nSocket);
	else if (newSocket != networkData.nSocket) {
		networkData.nSocket = (gameStates.multi.nGameType == UDP_GAME) ? newSocket - UDP_BASEPORT : newSocket;
		IpxChangeDefaultSocket ((ushort) (IPX_DEFAULT_SOCKET + networkData.nSocket));
		}
	}

netGame.invul = m [optStartInvul].m_value;
mpParams.bInvul = (ubyte) netGame.invul;
netGame.BrightPlayers = m [optBrightPlayers].m_value ? 0 : 1;
mpParams.bBrightPlayers = (ubyte) netGame.BrightPlayers;
mpParams.bShortPackets = netGame.bShortPackets = m [optShortPkts].m_value;
netGame.bShowAllNames = m [optShowNames].m_value;
mpParams.bShowAllNames = (ubyte) netGame.bShowAllNames;
NetworkAdjustMaxDataSize ();
//  extraGameInfo [0].bEnhancedCTF = (m [optEnhancedCTF].m_value != 0);

netGame.bAllowMarkerView = m [optMarkerView].m_value;
mpParams.bMarkerView = (ubyte) netGame.bAllowMarkerView;
netGame.bIndestructibleLights = m [optLight].m_value; 
mpParams.bIndestructibleLights = (ubyte) netGame.bIndestructibleLights;
mpParams.nDifficulty = gameStates.app.nDifficultyLevel = m [optDifficulty].m_value;
if ((mpParams.bShowPlayersOnAutomap = m [optPlayersOnMap].m_value))
	netGame.gameFlags |= NETGAME_FLAG_SHOW_MAP;
else
	netGame.gameFlags &= ~NETGAME_FLAG_SHOW_MAP;
}

//------------------------------------------------------------------------------

int NetworkD2XOptionsPoll (CMenu& menu, int& key, int nCurItem)
{
	int	v, j;

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
	if (v != (extraGameInfo [1].bTargetIndicators == 0)) {
		for (j = 0; j < 3; j++)
			if (menu [optTgtInd + j].m_value) {
				extraGameInfo [1].bTargetIndicators = j;
				break;
				}
		key = -2;
		return nCurItem;
		}
	}
if (optDmgIndicator >= 0) {
	v = menu [optDmgIndicator].m_value;
	if (v != extraGameInfo [1].bDamageIndicators) {
		extraGameInfo [1].bDamageIndicators = v;
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
return nCurItem;
}

//------------------------------------------------------------------------------

void NetworkD2XOptions (void)
 {
  int		i, j, choice = 0, optCheckPort = -1;
  char	szSpotSize [50];
  char	szPenalty [50];
  CMenu	m;

do {
	m.Destroy ();
	m.Create (40);
	optCompetition = m.AddCheck (TXT_COMPETITION_MODE, extraGameInfo [1].bCompetition, KEY_C, HTX_MULTI2_COMPETITION);
	if (!extraGameInfo [1].bCompetition) {
		optFF = m.AddCheck (TXT_FRIENDLY_FIRE, extraGameInfo [0].bFriendlyFire, KEY_F, HTX_MULTI2_FFIRE);
		optSuicide = m.AddCheck (TXT_NO_SUICIDE, extraGameInfo [0].bInhibitSuicide, KEY_U, HTX_MULTI2_SUICIDE);
		optMouseLook = m.AddCheck (TXT_MOUSELOOK, extraGameInfo [1].bMouseLook, KEY_O, HTX_MULTI2_MOUSELOOK);
		optFastPitch = m.AddCheck (TXT_FASTPITCH, (extraGameInfo [1].bFastPitch == 1) ? 1 : 0, KEY_P, HTX_MULTI2_FASTPITCH);
		optDualMiss = m.AddCheck (TXT_DUAL_LAUNCH, extraGameInfo [1].bDualMissileLaunch, KEY_M, HTX_GPLAY_DUALLAUNCH);
		}
	else
		optFF =
		optSuicide =
		optMouseLook =
		optFastPitch =
		optDualMiss = -1;
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
		optDisableReactor = m.AddCheck (TXT_NO_REACTOR, extraGameInfo [1].bDisableReactor, KEY_R, HTX_MULTI2_NOREACTOR); 
#else
		optDisableReactor = -1;
#endif
#if UDP_SAFEMODE
	optSafeUDP = m.AddCheck (TXT_UDP_QUAL, extraGameInfo [0].bSafeUDP, KEY_Q, HTX_MULTI2_UDPQUAL);
#endif
	optCheckPort = m.AddCheck (TXT_CHECK_PORT, extraGameInfo [0].bCheckUDPPort, KEY_P, HTX_MULTI2_CHECKPORT);
	if (extraGameInfo [1].bDarkness)
		m.AddText ("", 0);
	optDarkness = m.AddCheck (TXT_DARKNESS, extraGameInfo [1].bDarkness, KEY_D, HTX_DARKNESS);
	if (extraGameInfo [1].bDarkness) {
		optPowerupLights = m.AddCheck (TXT_POWERUPLIGHTS, !extraGameInfo [1].bPowerupLights, KEY_P, HTX_POWERUPLIGHTS);
		optHeadlights = m.AddCheck (TXT_HEADLIGHTS, !extraGameInfo [1].headlight.bAvailable, KEY_H, HTX_HEADLIGHTS);
		if (extraGameInfo [1].headlight.bAvailable) {
			sprintf (szSpotSize + 1, TXT_SPOTSIZE, GT (664 + extraGameInfo [1].nSpotSize));
			strupr (szSpotSize + 1);
			*szSpotSize = *(TXT_SPOTSIZE - 1);
			optSpotSize = m.AddSlider (szSpotSize + 1, extraGameInfo [1].nSpotSize, 0, 2, KEY_O, HTX_SPOTSIZE); 
			}
		else
			optSpotSize = -1;
		}
	else
		optHeadlights =
		optPowerupLights =
		optSpotSize = -1;
	m.AddText ("", 0);
	if (!extraGameInfo [1].bCompetition) {
		if (mpParams.nGameMode == NETGAME_COOPERATIVE) {
			sprintf (szPenalty + 1, TXT_COOP_PENALTY, nCoopPenalties [(int) extraGameInfo [1].nCoopPenalty], '%');
			strupr (szPenalty + 1);
			*szPenalty = *(TXT_COOP_PENALTY - 1);
			optPenalty = m.AddSlider (szPenalty + 1, extraGameInfo [1].nCoopPenalty, 0, 9, KEY_O, HTX_COOP_PENALTY); 
			}
		else
			optPenalty = -1;
		optTgtInd = m.AddRadio (TXT_TGTIND_NONE, 0, KEY_A, 1, HTX_CPIT_TGTIND);
		m.AddRadio (TXT_TGTIND_SQUARE, 0, KEY_R, 1, HTX_CPIT_TGTIND);
		m.AddRadio (TXT_TGTIND_TRIANGLE, 0, KEY_T, 1, HTX_CPIT_TGTIND);
		m [optTgtInd + extraGameInfo [1].bTargetIndicators].m_value = 1;
		if (extraGameInfo [1].bTargetIndicators)
			optFriendlyIndicator = m.AddCheck (TXT_FRIENDLY_INDICATOR, extraGameInfo [1].bFriendlyIndicators, KEY_F, HTX_FRIENDLY_INDICATOR);
		else
			optFriendlyIndicator = -1;
		optDmgIndicator = m.AddCheck (TXT_DMG_INDICATOR, extraGameInfo [1].bDamageIndicators, KEY_D, HTX_CPIT_DMGIND);
		optMslLockIndicator = m.AddCheck (TXT_MSLLOCK_INDICATOR, extraGameInfo [1].bMslLockIndicators, KEY_G, HTX_CPIT_MSLLOCKIND);
		if (extraGameInfo [1].bTargetIndicators || extraGameInfo [1].bDamageIndicators)
			optHitIndicator = m.AddCheck (TXT_HIT_INDICATOR, extraGameInfo [1].bTagOnlyHitObjs, KEY_T, HTX_HIT_INDICATOR);
		else {
			optPenalty =
			optHitIndicator = -1;
			extraGameInfo [1].nCoopPenalty = 0;
			}
		m.AddText ("", 0);
		}
	else
		optTgtInd =
		optDmgIndicator =
		optMslLockIndicator = -1;
	i = m.Menu (NULL, TXT_D2XOPTIONS_TITLE, NetworkD2XOptionsPoll, &choice);
  //mpParams.nReactorLife = atoi (szInvul)*60*F1_0;
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
	extraGameInfo [0].bSafeUDP = (m [optSafeUDP].m_value != 0);
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
		GET_VAL (extraGameInfo [1].bDualMissileLaunch, optDualMiss);
		GET_VAL (extraGameInfo [1].bDisableReactor, optDisableReactor);
		if (optTgtInd >= 0) {
			for (j = 0; j < 3; j++)
				if (m [optTgtInd + j].m_value) {
					extraGameInfo [1].bTargetIndicators = j;
					break;
					}
			GET_VAL (extraGameInfo [1].bFriendlyIndicators, optFriendlyIndicator);
			}
		GET_VAL (extraGameInfo [1].bDamageIndicators, optDmgIndicator);
		GET_VAL (extraGameInfo [1].bMslLockIndicators, optMslLockIndicator);
		GET_VAL (extraGameInfo [1].bTagOnlyHitObjs, optHitIndicator);
		}
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

static int optGameName, optLevel, optLevelText, optMoreOpts, optMission, optMissionName, optD2XOpts, optConfigMenu;

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
					udpBasePorts [1]);
		}
	}
m.AddText (TXT_DESCRIPTION, 0); 
optGameName = m.AddInput (szName, NETGAME_NAME_LEN, HTX_MULTI_NAME); 
optMission = m.AddMenu (TXT_SEL_MISSION, KEY_I, HTX_MULTI_MISSION);
optMissionName = m.AddText ("", 0);
m [optMissionName].m_bRebuild = 1; 
if ((nNewMission >= 0) && (gameData.missions.nLastLevel > 1)) {
	optLevelText = m.AddText (szLevelText, 0); 
	optLevel = m.AddInput (szLevel, 4, HTX_MULTI_LEVEL);
	}
else
	optLevelText = -1;
m.AddText ("", 0); 
optGameTypes = 
optAnarchy = m.AddRadio (TXT_ANARCHY, 0, KEY_A, 0, HTX_MULTI_ANARCHY);
optTeamAnarchy = m.AddRadio (TXT_TEAM_ANARCHY, 0, KEY_T, 0, HTX_MULTI_TEAMANA);
optRobotAnarchy = m.AddRadio (TXT_ANARCHY_W_ROBOTS, 0, KEY_R, 0, HTX_MULTI_BOTANA);
optCoop = m.AddRadio (TXT_COOP, 0, KEY_P, 0, HTX_MULTI_COOP);
optCTF = m.AddRadio (TXT_CTF, 0, KEY_F, 0, HTX_MULTI_CTF);
if (!gameStates.app.bNostalgia)
	optEnhancedCTF = m.AddRadio (TXT_CTF_PLUS, 0, KEY_T, 0, HTX_MULTI_CTFPLUS);

optEntropy =
optMonsterball = -1;
if (bHoard) {
	optHoard = m.AddRadio (TXT_HOARD, 0, KEY_H, 0, HTX_MULTI_HOARD);
	optTeamHoard = m.AddRadio (TXT_TEAM_HOARD, 0, KEY_H, 0, HTX_MULTI_TEAMHOARD);
	if (!gameStates.app.bNostalgia) {
		optEntropy = m.AddRadio (TXT_ENTROPY, 0, KEY_Y, 0, HTX_MULTI_ENTROPY);
		optMonsterball = m.AddRadio (TXT_MONSTERBALL, 0, KEY_B, 0, HTX_MULTI_MONSTERBALL);
		}
	} 
nGameTypes = m.ToS () - 1 - optGameTypes;
m [optGameTypes + NMCLAMP (mpParams.nGameType, 0, nGameTypes)].m_value = 1;

m.AddText ("", 0); 

optOpenGame = m.AddRadio (TXT_OPEN_GAME, 0, KEY_O, 1, HTX_MULTI_OPENGAME);
optClosedGame = m.AddRadio (TXT_CLOSED_GAME, 0, KEY_C, 1, HTX_MULTI_CLOSEDGAME);
optRestrictedGame = m.AddRadio (TXT_RESTR_GAME, 0, KEY_R, 1, HTX_MULTI_RESTRGAME);
m [optOpenGame + NMCLAMP (mpParams.nGameAccess, 0, 2)].m_value = 1;
m.AddText ("", 0);
sprintf (szMaxNet + 1, TXT_MAX_PLAYERS, gameData.multiplayer.nMaxPlayers);
*szMaxNet = * (TXT_MAX_PLAYERS - 1);
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
	if (m [optEntropy].m_value)
		optEntOpts = m.AddMenu (TXT_ENTROPY_OPTS, KEY_E, HTX_MULTI_ENTOPTS);
	if (m [optMonsterball].m_value)
		optMBallOpts = m.AddMenu (TXT_MONSTERBALL_OPTS, KEY_O, HTX_MULTI_MBALLOPTS);
	optConfigMenu = m.AddMenu (TXT_GAME_OPTIONS, KEY_O, HTX_MAIN_CONF);
	}
}

//------------------------------------------------------------------------------

int GameParamsMenu (CMenu& m, int& key, int& choice, char* szName, char* szLevelText, char* szLevel, char* szIpAddr, int& nNewMission)
{
	int i, bAnarchyOnly = (nNewMission < 0) ? 0 : gameData.missions.list [nNewMission].bAnarchyOnly;

if (m [optMissionName].m_bRebuild) {
	strncpy (netGame.szMissionName, 
				(nNewMission < 0) ? "" : gameData.missions.list [nNewMission].filename, 
				sizeof (netGame.szMissionName) - 1);
	m [optMissionName].m_text = (nNewMission < 0) ? const_cast<char*> (TXT_NONE_SELECTED) : const_cast<char*> (gameData.missions.list [nNewMission].szMissionName);
	if ((nNewMission >= 0) && (gameData.missions.nLastLevel > 1)) {
		sprintf (szLevelText, "%s (1-%d)", TXT_LEVEL_, gameData.missions.nLastLevel);
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
					ipx_MyAddress [4], ipx_MyAddress [5], ipx_MyAddress [6], ipx_MyAddress [7], udpBasePorts [1]);
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
else if (choice == optMission) {
	int h = SelectAndLoadMission (1, &bAnarchyOnly);
	if (h < 0)
		return 1;
	gameData.missions.nLastMission = nNewMission = h;
	m [optMissionName].m_bRebuild = 1;
	return 2;
	}

if (key != -1) {
	int j;
		   
	gameData.multiplayer.nMaxPlayers = m [optMaxNet].m_value + 2;
	netGame.nMaxPlayers = gameData.multiplayer.nMaxPlayers;
			
	for (j = 0; j < networkData.nActiveGames; j++)
		if (!stricmp (activeNetGames [j].szGameName, szName)) {
			MsgBox (TXT_ERROR, NULL, 1, TXT_OK, TXT_DUPLICATE_NAME);
			return 1;
		}
	strncpy (mpParams.szGameName, szName, sizeof (mpParams.szGameName));
	mpParams.nLevel = atoi (szLevel);
	if ((gameData.missions.nLastLevel > 0) && ((mpParams.nLevel < 1) || (mpParams.nLevel > gameData.missions.nLastLevel))) {
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
		netGame.gameFlags |= NETGAME_FLAG_CLOSED;
	netGame.bRefusePlayers = m [optRestrictedGame].m_value;
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

	int nNewMission = gameData.missions.nLastMission;
	int bAnarchyOnly = 0;
	int bHoard = HoardEquipped ();

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
	extraGameInfo [0].bSafeUDP = (m [optSafeUDP].m_value != 0);
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
netGame.szMissionName [sizeof (netGame.szMissionName) - 1] = '\0';
strcpy (netGame.szMissionTitle, gameData.missions.list [nNewMission].szMissionName + (gameOpts->menus.bShowLevelVersion ? 4 : 0));
netGame.controlInvulTime = mpParams.nReactorLife * 5 * F1_0 * 60;
IpxChangeDefaultSocket ((ushort) (IPX_DEFAULT_SOCKET + networkData.nSocket));
return key;
}

//------------------------------------------------------------------------------

static time_t	nQueryTimeout;

static int QueryPoll (CMenu& menu, int& key, int nCurItem)
{
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

m.AddGauge ("                    ", 0, 1000); 
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

int stoip (char *szServerIpAddr, ubyte *pIpAddr)
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
		return stoport (pFields [j], udpBasePorts, NULL); 
	else {
		h = atol (pFields [j]);
		if ((h < 0) || (h > 255))
			return 0;
		pIpAddr [j] = (ubyte) h;
		}
	}
return 1;
}

//------------------------------------------------------------------------------

int IpAddrMenuCallBack (CMenu& menu, int& key, int nCurItem)
{
return nCurItem;
}

//------------------------------------------------------------------------------

int NetworkGetIpAddr (void)
{
	CMenu	m (9);
	int	i, choice = 0;
	int	opt = 0, optServer = -1, optPort = -1;

	static char szClientPort [7] = {'\0'};
	static int nClientPortSign = 0;

if (!gameStates.multi.bUseTracker) {
	if (!*mpParams.szServerIpAddr) {
		ArchIpxSetDriver (IPX_DRIVER_UDP);
		if (IpxInit (-1) != IPX_INIT_OK) {
			strcpy (mpParams.szServerIpAddr, "0.0.0.0");
			}
		else {
			sprintf (mpParams.szServerIpAddr, "%d.%d.%d.%d", 
						ipx_ServerAddress [4], ipx_ServerAddress [5], ipx_ServerAddress [6], ipx_ServerAddress [7]);
			sprintf (szClientPort, "%s%d", 
						 (nClientPortSign < 0) ? "-" : (nClientPortSign > 0) ? "+" : "", mpParams.udpClientPort);
			IpxClose ();
			}
		}
	}
if (!gameStates.multi.bUseTracker) {
	m.AddText (TXT_HOST_IP, 0);
	//IP [\":\"<port> | {{\"+\" | \"-\"} <offset>]\n\n (e.g. 127.0.0.1 for an arbitrary port, \nor 127.0.0.1:28342 for a fixed port, \nor 127.0.0.1:+1 for host port + offset)\n";       
	//m [opt].m_text = "Enter the game host' IP address.\nYou can specify a port to use\nby entering a colon followed by the port number.\nYou can use an 
	optServer = m.AddInput (mpParams.szServerIpAddr, sizeof (mpParams.szServerIpAddr) - 1, HTX_GETIP_SERVER);
	m.AddText (TXT_CLIENT_PORT, 0);
	}
if ((mpParams.udpClientPort < 0) || (mpParams.udpClientPort > 65535))
	mpParams.udpClientPort = 0;
sprintf (szClientPort, "%u", mpParams.udpClientPort);
optPort = m.AddInput (szClientPort, sizeof (szClientPort) - 1, HTX_GETIP_CLIENT);
m.AddText (TXT_PORT_HELP1, 0);
m.AddText (TXT_PORT_HELP2, 0);
for (;;) {
	i = m.Menu (NULL, gameStates.multi.bUseTracker ? TXT_CLIENT_PORT + 1 : TXT_IP_HEADER, &IpAddrMenuCallBack, &choice);
	if (i < 0)
		break;
	if (i < int (m.ToS ())) {
		if ((i == optServer) || (i == optPort)) {
			if (gameStates.multi.bUseTracker || stoip (mpParams.szServerIpAddr, ipx_ServerAddress + 4)) {
				stoport (szClientPort, &mpParams.udpClientPort, &nClientPortSign);
				if (gameStates.multi.bCheckPorts && !mpParams.udpClientPort)
					MsgBox (NULL, NULL, 1, TXT_OK, TXT_IP_INVALID);
				else
					return 1;
				}
			MsgBox (NULL, NULL, 1, TXT_OK, TXT_IP_INVALID);
			}
		}
	}
return 0;
} 

//------------------------------------------------------------------------------

