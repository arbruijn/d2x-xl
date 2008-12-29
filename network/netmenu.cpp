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
#include "menubackground.h"

#define LHX(x)      (gameStates.menus.bHires?2* (x):x)
#define LHY(y)      (gameStates.menus.bHires? (24* (y))/10:y)

/* the following are the possible packet identificators.
 * they are stored in the "nType" field of the packet structs.
 * they are offset 4 bytes from the beginning of the raw IPX data
 * because of the "driver's" ipx_packetnum (see linuxnet.c).
 */


extern void SetFunctionMode (int);
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

void NetworkEntropyToggleOptions (void)
{
	CMenu	m (12);
	int	opt = 0;

optPlrHand = m.AddCheck (TXT_ENT_HANDICAP, extraGameInfo [0].entropy.bPlayerHandicap, KEY_H, HTX_ONLINE_MANUAL);
optConqWarn = m.AddCheck (TXT_ENT_CONQWARN, extraGameInfo [0].entropy.bDoConquerWarning, KEY_W, HTX_ONLINE_MANUAL);
optRevRooms = m.AddCheck (TXT_ENT_REVERT, extraGameInfo [0].entropy.bRevertRooms, KEY_R, HTX_ONLINE_MANUAL);
m.AddText ("");
m.AddText (TXT_ENT_VIRSTAB);
optVirStab = opt;
m.AddRadio (TXT_ENT_VIRSTAB_DROP, 0, 2, KEY_D);
m.AddRadio (TXT_ENT_VIRSTAB_ENEMY, 0, 2, KEY_T);
m.AddRadio (TXT_ENT_VIRSTAB_TOUCH, 0, 2, KEY_L);
m.AddRadio (TXT_ENT_VIRSTAB_NEVER, 0, 2, KEY_N);
m [optVirStab + extraGameInfo [0].entropy.nVirusStability].m_value = 1;

Assert (sizeofa (m) >= (size_t) opt);
m.Menu (NULL, TXT_ENT_TOGGLES, NULL, 0);

extraGameInfo [0].entropy.bRevertRooms = m [optRevRooms].m_value;
extraGameInfo [0].entropy.bDoConquerWarning = m [optConqWarn].m_value;
extraGameInfo [0].entropy.bPlayerHandicap = m [optPlrHand].m_value;
for (extraGameInfo [0].entropy.nVirusStability = 0; 
	  extraGameInfo [0].entropy.nVirusStability < 3; 
	  extraGameInfo [0].entropy.nVirusStability++)
	if (m [optVirStab + extraGameInfo [0].entropy.nVirusStability].m_value)
		break;
}

//------------------------------------------------------------------------------

void NetworkEntropyTextureOptions (void)
{
	CMenu	m (7);

optOvrTex = m.AddRadio (TXT_ENT_TEX_KEEP, 0, 1, KEY_K, HTX_ONLINE_MANUAL);
m.AddRadio (TXT_ENT_TEX_OVERRIDE, 0, 1, KEY_O, HTX_ONLINE_MANUAL);
m.AddRadio (TXT_ENT_TEX_COLOR, 0, 1, KEY_C, HTX_ONLINE_MANUAL);
m [optOvrTex + extraGameInfo [0].entropy.nOverrideTextures].m_value = 1;
m.AddText ("");
optBrRooms = m.AddCheck (TXT_ENT_TEX_BRIGHTEN, extraGameInfo [0].entropy.bBrightenRooms, KEY_B, HTX_ONLINE_MANUAL);

m.Menu (NULL, TXT_ENT_TEXTURES, NULL, 0);

extraGameInfo [0].entropy.bBrightenRooms = m [optBrRooms].m_value;
for (extraGameInfo [0].entropy.nOverrideTextures = 0; 
	  extraGameInfo [0].entropy.nOverrideTextures < 3; 
	  extraGameInfo [0].entropy.nOverrideTextures++)
	if (m [optOvrTex + extraGameInfo [0].entropy.nOverrideTextures].m_value)
		break;
}

//------------------------------------------------------------------------------

void NetworkEntropyOptions (void)
{
	CMenu		m (25);
	int		i, opt = 0;
	char		szCapVirLim [10], szCapTimLim [10], szMaxVirCap [10], szBumpVirCap [10], 
				szBashVirCap [10], szVirGenTim [10], szVirLife [10], 
				szEnergyFill [10], szShieldFill [10], szShieldDmg [10];

optCapVirLim = m.AddInput (TXT_ENT_VIRLIM, szCapVirLim, extraGameInfo [0].entropy.nCaptureVirusLimit, 3, HTX_ONLINE_MANUAL);
optCapTimLim = m.AddInput (TXT_ENT_CAPTIME, szCapTimLim, extraGameInfo [0].entropy.nCaptureTimeLimit, 3, HTX_ONLINE_MANUAL);
optMaxVirCap = m.AddInput (TXT_ENT_MAXCAP, szMaxVirCap, extraGameInfo [0].entropy.nMaxVirusCapacity, 6, HTX_ONLINE_MANUAL);
optBumpVirCap = m.AddInput (TXT_ENT_BUMPCAP, szBumpVirCap, extraGameInfo [0].entropy.nBumpVirusCapacity, 3, HTX_ONLINE_MANUAL);
optBashVirCap = m.AddInput (TXT_ENT_BASHCAP, szBashVirCap, extraGameInfo [0].entropy.nBashVirusCapacity, 3, HTX_ONLINE_MANUAL);
optVirGenTim = m.AddInput (TXT_ENT_GENTIME, szVirGenTim, extraGameInfo [0].entropy.nVirusGenTime, 3, HTX_ONLINE_MANUAL);
optVirLife = m.AddInput (TXT_ENT_VIRLIFE, szVirLife, extraGameInfo [0].entropy.nVirusLifespan, 3, HTX_ONLINE_MANUAL);
optEnergyFill = m.AddInput (TXT_ENT_EFILL, szEnergyFill, extraGameInfo [0].entropy.nEnergyFillRate, 3, HTX_ONLINE_MANUAL);
optShieldFill = m.AddInput (TXT_ENT_SFILL, szShieldFill, extraGameInfo [0].entropy.nShieldFillRate, 3, HTX_ONLINE_MANUAL);
optShieldDmg = m.AddInput (TXT_ENT_DMGRATE, szShieldDmg, extraGameInfo [0].entropy.nShieldDamageRate, 3, HTX_ONLINE_MANUAL);
m.AddText ("");
optTogglesMenu = m.AddMenu (TXT_ENT_TGLMENU, KEY_E);
optTextureMenu = m.AddMenu (TXT_ENT_TEXMENU, KEY_T);

for (;;) {
	i = m.Menu (NULL, "Entropy Options", NULL, 0);
	if (i == optTogglesMenu)
		NetworkEntropyToggleOptions ();
	else if (i == optTextureMenu)
		NetworkEntropyTextureOptions ();
	else
		break;
	}
extraGameInfo [0].entropy.nCaptureVirusLimit = (char) atol (m [optCapVirLim].m_text);
extraGameInfo [0].entropy.nCaptureTimeLimit = (char) atol (m [optCapTimLim].m_text);
extraGameInfo [0].entropy.nMaxVirusCapacity = (ushort) atol (m [optMaxVirCap].m_text);
extraGameInfo [0].entropy.nBumpVirusCapacity = (char) atol (m [optBumpVirCap].m_text);
extraGameInfo [0].entropy.nBashVirusCapacity = (char) atol (m [optBashVirCap].m_text);
extraGameInfo [0].entropy.nVirusGenTime = (char) atol (m [optVirGenTim].m_text);
extraGameInfo [0].entropy.nVirusLifespan = (char) atol (m [optVirLife].m_text);
extraGameInfo [0].entropy.nEnergyFillRate = (ushort) atol (m [optEnergyFill].m_text);
extraGameInfo [0].entropy.nShieldFillRate = (ushort) atol (m [optShieldFill].m_text);
extraGameInfo [0].entropy.nShieldDamageRate = (ushort) atol (m [optShieldDmg].m_text);
}

//------------------------------------------------------------------------------

static int nBonusOpt, nSizeModOpt, nPyroForceOpt;

int MonsterballMenuCallback (CMenu& menu, int& key, int nCurItem)
{
	CMenuItem	*m;
	int			v;

m = menu + nPyroForceOpt;
v = m->m_value + 1;
if (v != extraGameInfo [0].monsterball.forces [24].nForce) {
	extraGameInfo [0].monsterball.forces [24].nForce = v;
	sprintf (m->m_text, TXT_MBALL_PYROFORCE, v);
	m->m_bRebuild = 1;
	//key = -2;
	return nCurItem;
	}
m = menu + nBonusOpt;
v = m->m_value + 1;
if (v != extraGameInfo [0].monsterball.nBonus) {
	extraGameInfo [0].monsterball.nBonus = v;
	sprintf (m->m_text, TXT_GOAL_BONUS, v);
	m->m_bRebuild = 1;
	//key = -2;
	return nCurItem;
	}
m = menu + nSizeModOpt;
v = m->m_value + 2;
if (v != extraGameInfo [0].monsterball.nSizeMod) {
	extraGameInfo [0].monsterball.nSizeMod = v;
	sprintf (m->m_text, TXT_MBALL_SIZE, v / 2, (v & 1) ? 5 : 0);
	m->m_bRebuild = 1;
	//key = -2;
	}
return nCurItem;
}

//------------------------------------------------------------------------------

static int optionToWeaponId [] = {
	LASER_ID, 
	LASER_ID + 1, 
	LASER_ID + 2, 
	LASER_ID + 3, 
	SPREADFIRE_ID, 
	VULCAN_ID, 
	PLASMA_ID, 
	FUSION_ID, 
	SUPERLASER_ID, 
	SUPERLASER_ID + 1, 
	HELIX_ID, 
	GAUSS_ID, 
	PHOENIX_ID, 
	OMEGA_ID, 
	FLARE_ID, 
	CONCUSSION_ID, 
	HOMINGMSL_ID, 
	SMARTMSL_ID, 
	MEGAMSL_ID, 
	FLASHMSL_ID, 
	GUIDEDMSL_ID, 
	MERCURYMSL_ID, 
	EARTHSHAKER_ID, 
	EARTHSHAKER_MEGA_ID
	};

ubyte nOptionToForce [] = {
	5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 
	60, 70, 80, 90, 100, 110, 120, 130, 140, 150, 175, 200, 250
	};

static const char *szWeaponTexts [] = {
	"Laser 1", 
	"Laser 2", 
	"Laser 3", 
	"Laser 4", 
	"Spreadfire", 
	"Vulcan", 
	"Plasma", 
	"Fusion", 
	"Superlaser 1", 
	"Superlaser 2", 
	"Helix", 
	"Gauss", 
	"Phoenix", 
	"Omega", 
	"Flare", 
	"Concussion", 
	"Homing", 
	"Smart", 
	"Mega", 
	"Flash", 
	"Guided", 
	"Mercury", 
	"Earthshaker", 
	"Shaker Bomblet"
	};

static inline int ForceToOption (double dForce)
{
	int	i, h = (int) sizeofa (nOptionToForce);

for (i = 0; i < h - 1; i++)
	if ((dForce >= nOptionToForce [i]) && (dForce < nOptionToForce [i + 1]))
		break;
return i;
}

void NetworkMonsterballOptions (void)
{
	CMenu					m (35);
	int					h, i, j, opt = 0, optDefaultForces;
	char					szBonus [60], szSize [60], szPyroForce [60];
	tMonsterballForce	*pf = extraGameInfo [0].monsterball.forces;

h = (int) sizeofa (optionToWeaponId);
j = (int) sizeofa (nOptionToForce);
for (i = opt = 0; i < h; i++, opt++, pf++) {
	m.AddSlider (szWeaponTexts [i], ForceToOption (pf->nForce), 0, j - 1, 0, NULL);
	if (pf->nWeaponId == FLARE_ID)
		m.AddText ("", 0);
	}
m.AddText ("", 0);
sprintf (szPyroForce + 1, TXT_MBALL_PYROFORCE, pf->nForce);
*szPyroForce = *(TXT_MBALL_PYROFORCE - 1);
nPyroForceOpt = m.AddSlider (szPyroForce + 1, pf->nForce - 1, 0, 9, 0, NULL);
m.AddText ("", 0);
sprintf (szBonus + 1, TXT_GOAL_BONUS, extraGameInfo [0].monsterball.nBonus);
*szBonus = *(TXT_GOAL_BONUS - 1);
nBonusOpt = m.AddSlider (szBonus + 1, extraGameInfo [0].monsterball.nBonus - 1, 0, 9, 0, HTX_GOAL_BONUS);
i = extraGameInfo [0].monsterball.nSizeMod;
sprintf (szSize + 1, TXT_MBALL_SIZE, i / 2, (i & 1) ? 5 : 0);
*szSize = *(TXT_MBALL_SIZE - 1);
nSizeModOpt = m.AddSlider (szSize + 1, extraGameInfo [0].monsterball.nSizeMod - 2, 0, 8, 0, HTX_MBALL_SIZE);
m.AddText ("", 0);
optDefaultForces = m.AddMenu ("Set default values", 0, NULL);

for (;;) {
	i = m.Menu (NULL, "Monsterball Impact Forces", MonsterballMenuCallback, 0);
	if (i == -1)
		break;
	if (i != optDefaultForces)
		break;
	InitMonsterballSettings (&extraGameInfo [0].monsterball);
	pf = extraGameInfo [0].monsterball.forces;
	for (i = 0; i < h + 1; i++, pf++) {
		m [i].m_value = ForceToOption (pf->nForce);
		if (pf->nWeaponId == FLARE_ID)
			i++;
		}
	m [nPyroForceOpt].m_value = NMCLAMP (pf->nForce - 1, 0, 9);
	m [nSizeModOpt].m_value = extraGameInfo [0].monsterball.nSizeMod - 2;
	}
pf = extraGameInfo [0].monsterball.forces;
for (i = 0; i < h; i++, pf++)
	pf->nForce = nOptionToForce [m [i].m_value];
pf->nForce = m [nPyroForceOpt].m_value + 1;
extraGameInfo [0].monsterball.nBonus = m [nBonusOpt].m_value + 1;
extraGameInfo [0].monsterball.nSizeMod = m [nSizeModOpt].m_value + 2;
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

int NetworkSelectTeams (void)
{
	CMenu	m;
	int	choice, opt_team_b;
	ubyte teamVector = 0;
	char	teamNames [2][CALLSIGN_LEN+1];
	int	i;
	int	playerIds [MAX_PLAYERS+2];

	// One-time initialization

for (i = gameData.multiplayer.nPlayers/2; i < gameData.multiplayer.nPlayers; i++) // Put first half of players on team A
	teamVector |= (1 << i);
sprintf (teamNames [0], "%s", TXT_BLUE);
sprintf (teamNames [1], "%s", TXT_RED);

for (;;) {
	m.Destroy ();
	m.Create (MAX_PLAYERS + 4);

	m.AddInput (teamNames [0], CALLSIGN_LEN);
	for (i = 0; i < gameData.multiplayer.nPlayers; i++) {
		if (!(teamVector & (1 << i))) {
			m.AddMenu (netPlayers.players [i].callsign);
			playerIds [m.ToS () - 1] = i;
			}
		}
	opt_team_b = m.AddInput (teamNames [1], CALLSIGN_LEN); 
	for (i = 0; i < gameData.multiplayer.nPlayers; i++) {
		if (teamVector & (1 << i)) {
			m.AddMenu (netPlayers.players [i].callsign);
			playerIds [m.ToS () - 1] = i;
			}
		}
	m.AddText ("");
	m.AddMenu (TXT_ACCEPT, KEY_A);

	choice = m.Menu (NULL, TXT_TEAM_SELECTION, NULL, NULL);

	if (choice == m.ToS () - 1) {
		if ((m.ToS () - 2 - opt_team_b < 2) || (opt_team_b == 1)) 
			MsgBox (NULL, NULL, 1, TXT_OK, TXT_TEAM_MUST_ONE);
		netGame.teamVector = teamVector;
		strcpy (netGame.szTeamName [0], teamNames [0]);
		strcpy (netGame.szTeamName [1], teamNames [1]);
		return 1;
		}
	else if ((choice > 0) && (choice < opt_team_b)) 
		teamVector |= (1 << playerIds [choice]);
	else if ((choice > opt_team_b) && (choice < int (m.ToS ()) - 2)) 
		teamVector &= ~ (1 << playerIds [choice]);
	else if (choice == -1)
		return 0;
	}
}

//------------------------------------------------------------------------------

static bool GetSelectedPlayers (CMenu& m, int nSavePlayers)
{
// Count number of players chosen
gameData.multiplayer.nPlayers = 0;
for (int i = 0; i < nSavePlayers; i++) {
	if (m [i].m_value) 
		gameData.multiplayer.nPlayers++;
	}
if (gameData.multiplayer.nPlayers > netGame.nMaxPlayers) {
	MsgBox (TXT_ERROR, NULL, 1, TXT_OK, "%s %d %s", TXT_SORRY_ONLY, gameData.multiplayer.nMaxPlayers, TXT_NETPLAYERS_IN);
	gameData.multiplayer.nPlayers = nSavePlayers;
	return true;
	}
#if !DBG
if (gameData.multiplayer.nPlayers < 2) {
	MsgBox (TXT_WARNING, NULL, 1, TXT_OK, TXT_TEAM_ATLEAST_TWO);
#	if 0
	gameData.multiplayer.nPlayers = nSavePlayers;
	return true;
#	endif
	}
#endif

#if !DBG
if (((netGame.gameMode == NETGAME_TEAM_ANARCHY) ||
	  (netGame.gameMode == NETGAME_CAPTURE_FLAG) || 
	  (netGame.gameMode == NETGAME_TEAM_HOARD)) && 
	 (gameData.multiplayer.nPlayers < 2)) {
	MsgBox (TXT_ERROR, NULL, 1, TXT_OK, TXT_NEED_2PLAYERS);
	gameData.multiplayer.nPlayers = nSavePlayers;
#if 0	
	return true;
#endif	
	}
#endif
return false;
}

//------------------------------------------------------------------------------

int AbortPlayerSelection (int nSavePlayers)
{
for (int i = 1; i < nSavePlayers; i++) {
	if (gameStates.multi.nGameType >= IPX_GAME)
		NetworkDumpPlayer (
			netPlayers.players [i].network.ipx.server, 
			netPlayers.players [i].network.ipx.node, 
			DUMP_ABORTED);
	}

netGame.nNumPlayers = 0;
NetworkSendGameInfo (0); // Tell everyone we're bailing
IpxHandleLeaveGame (); // Tell the network driver we're bailing too
networkData.nStatus = NETSTAT_MENU;
return 0;
}

//------------------------------------------------------------------------------

int NetworkSelectPlayers (int bAutoRun)
{
	int		i, j, choice = 1;
   CMenu		m (MAX_PLAYERS + 4);
   char		text [MAX_PLAYERS+4][45];
	char		title [50];
	int		nSavePlayers;              //how may people would like to join

NetworkAddPlayer (&networkData.thisPlayer);
if (bAutoRun)
	return 1;

for (i = 0; i < MAX_PLAYERS + 4; i++) {
	sprintf (text [i], "%d.  %-20s", i+1, "");
	m.AddCheck (text [i], 0);
	}
m [0].m_value = 1;                         // Assume server will play...
if (gameOpts->multi.bNoRankings)
	sprintf (text [0], "%d. %-20s", 1, LOCALPLAYER.callsign);
else
	sprintf (text [0], "%d. %s%-20s", 1, pszRankStrings [netPlayers.players [gameData.multiplayer.nLocalPlayer].rank], LOCALPLAYER.callsign);
sprintf (title, "%s %d %s", TXT_TEAM_SELECT, gameData.multiplayer.nMaxPlayers, TXT_TEAM_PRESS_ENTER);

do {
	gameStates.app.nExtGameStatus = GAMESTAT_NETGAME_PLAYER_SELECT;
	j = m.Menu (NULL, title, NetworkStartPoll, &choice);
	nSavePlayers = gameData.multiplayer.nPlayers;
	if (j < 0)
		return AbortPlayerSelection (nSavePlayers);
	} while (GetSelectedPlayers (m, nSavePlayers));
// Remove players that aren't marked.
gameData.multiplayer.nPlayers = 0;
for (i = 0; i < nSavePlayers; i++) {
	if (m [i].m_value) {
		if (i > gameData.multiplayer.nPlayers) {
			if (gameStates.multi.nGameType >= IPX_GAME) {
				memcpy (netPlayers.players [gameData.multiplayer.nPlayers].network.ipx.node, 
				netPlayers.players [i].network.ipx.node, 6);
				memcpy (netPlayers.players [gameData.multiplayer.nPlayers].network.ipx.server, 
				netPlayers.players [i].network.ipx.server, 4);
				}
			else {
				netPlayers.players [gameData.multiplayer.nPlayers].network.appletalk.node = netPlayers.players [i].network.appletalk.node;
				netPlayers.players [gameData.multiplayer.nPlayers].network.appletalk.net = netPlayers.players [i].network.appletalk.net;
				netPlayers.players [gameData.multiplayer.nPlayers].network.appletalk.socket = netPlayers.players [i].network.appletalk.socket;
				}
			memcpy (
				netPlayers.players [gameData.multiplayer.nPlayers].callsign, 
				netPlayers.players [i].callsign, CALLSIGN_LEN+1);
			netPlayers.players [gameData.multiplayer.nPlayers].versionMajor = netPlayers.players [i].versionMajor;
			netPlayers.players [gameData.multiplayer.nPlayers].versionMinor = netPlayers.players [i].versionMinor;
			netPlayers.players [gameData.multiplayer.nPlayers].rank = netPlayers.players [i].rank;
			ClipRank (reinterpret_cast<char*> (&netPlayers.players [gameData.multiplayer.nPlayers].rank));
			NetworkCheckForOldVersion ((char)i);
			}
		gameData.multiplayer.players [gameData.multiplayer.nPlayers].connected = 1;
		gameData.multiplayer.nPlayers++;
		}
	else {
		if (gameStates.multi.nGameType >= IPX_GAME)
			NetworkDumpPlayer (netPlayers.players [i].network.ipx.server, netPlayers.players [i].network.ipx.node, DUMP_DORK);
		}
	}

for (i = gameData.multiplayer.nPlayers; i < MAX_NUM_NET_PLAYERS; i++) {
	if (gameStates.multi.nGameType >= IPX_GAME) {
		memset (netPlayers.players [i].network.ipx.node, 0, 6);
		memset (netPlayers.players [i].network.ipx.server, 0, 4);
	   }
	else {
		netPlayers.players [i].network.appletalk.node = 0;
		netPlayers.players [i].network.appletalk.net = 0;
		netPlayers.players [i].network.appletalk.socket = 0;
	   }
	memset (netPlayers.players [i].callsign, 0, CALLSIGN_LEN+1);
	netPlayers.players [i].versionMajor = 0;
	netPlayers.players [i].versionMinor = 0;
	netPlayers.players [i].rank = 0;
	}
#if 1			
console.printf (CON_DBG, "Select teams: Game mode is %d\n", netGame.gameMode);
#endif
if (netGame.gameMode == NETGAME_TEAM_ANARCHY ||
	 netGame.gameMode == NETGAME_CAPTURE_FLAG ||
	 netGame.gameMode == NETGAME_TEAM_HOARD ||
	 netGame.gameMode == NETGAME_ENTROPY ||
	 netGame.gameMode == NETGAME_MONSTERBALL)
	if (!NetworkSelectTeams ())
		return AbortPlayerSelection (nSavePlayers);
return 1; 
}

//------------------------------------------------------------------------------

void InitNetgameMenuOption (CMenu& menu, int j)
{
CMenuItem* m = menu + j;
if (!m->m_text) {
	m->m_text = szNMTextBuffer [j];
	m->m_nType = NM_TYPE_MENU;
	}
sprintf (m->m_text, "%2d.                                                     ", j - 1 - gameStates.multi.bUseTracker);
m->m_value = 0;
m->m_bRebuild = 1;
}

//------------------------------------------------------------------------------

void InitNetgameMenu (CMenu& menu, int i)
{
	int j;

for (j = i + 2 + gameStates.multi.bUseTracker; i < MAX_ACTIVE_NETGAMES; i++, j++)
	InitNetgameMenuOption (menu, j);
}

//------------------------------------------------------------------------------

extern int nTabs [];

char *PruneText (char *pszDest, char *pszSrc, int nSize, int nPos, int nVersion)
{
	int		lDots, lMax, l, tx, ty, ta;
	char*		psz;
	CFont*	curFont = CCanvas::Current ()->Font ();

if (gameOpts->menus.bShowLevelVersion && (nVersion >= 0)) {
	if (nVersion)
		sprintf (pszDest, "[%d] ", nVersion);
	else
		strcpy (pszDest, "[?] ");
	strncat (pszDest + 4, pszSrc, nSize - 4);
	}
else
	strncpy (pszDest, pszSrc, nSize);
pszDest [nSize - 1] = '\0';
if ((psz = strchr (pszDest, '\t')))
	*psz = '\0';
fontManager.SetCurrent (SMALL_FONT);
fontManager.Current ()->StringSize ("... ", lDots, ty, ta);
fontManager.Current ()->StringSize (pszDest, tx, ty, ta);
l = (int) strlen (pszDest);
lMax = LHX (nTabs [nPos]) - LHX (nTabs [nPos - 1]);
if (tx > lMax) {
	lMax -= lDots;
	do {
		pszDest [--l] = '\0';
		fontManager.Current ()->StringSize (pszDest, tx, ty, ta);
	} while (tx > lMax);
	strcat (pszDest, "...");
	}
fontManager.SetCurrent (curFont); 
return pszDest;
}

//------------------------------------------------------------------------------

const char *szModeLetters []  = 
 {"ANRCHY", 
	 "TEAM", 
	 "ROBO", 
	 "COOP", 
	 "FLAG", 
	 "HOARD", 
	 "TMHOARD", 
	 "ENTROPY",
	 "MONSTER"};

int NetworkJoinPoll (CMenu& menu, int& key, int nCurItem)
{
	// Polling loop for Join Game menu
	static fix t1 = 0;
	int	t = SDL_GetTicks ();
	int	i, h = 2 + gameStates.multi.bUseTracker, osocket, nJoinStatus, bPlaySound = 0;
	const char	*psz;
	char	szOption [200];
	char	szTrackers [100];

if (gameStates.multi.bUseTracker) {
	i = ActiveTrackerCount (0);
	menu [1].m_color = ORANGE_RGBA;
	sprintf (szTrackers, TXT_TRACKERS_FOUND, i, (i == 1) ? "" : "s");
	if (strcmp (menu [1].m_text, szTrackers)) {
		strcpy (menu [1].m_text, szTrackers);
//			menu [1].x = (short) 0x8000;
		menu [1].m_bRebuild = 1;
		}
	}

if ((gameStates.multi.nGameType >= IPX_GAME) && networkData.bAllowSocketChanges) {
	osocket = networkData.nSocket;
	if (key == KEY_PAGEDOWN) { 
		networkData.nSocket--; 
		key = 0; 
		}
	else if (key == KEY_PAGEUP) { 
		networkData.nSocket++; 
		key = 0; 
		}
	if (networkData.nSocket > 99)
		networkData.nSocket = -99;
	else if (networkData.nSocket < -99)
		networkData.nSocket = 99;
	if (networkData.nSocket + IPX_DEFAULT_SOCKET > 0x8000)
		networkData.nSocket = 0x8000 - IPX_DEFAULT_SOCKET;
	if (networkData.nSocket + IPX_DEFAULT_SOCKET < 0)
		networkData.nSocket = IPX_DEFAULT_SOCKET;
	if (networkData.nSocket != osocket) {
		sprintf (menu [0].m_text, TXT_CURR_SOCK, 
					 (gameStates.multi.nGameType == IPX_GAME) ? "IPX" : "UDP", 
					 networkData.nSocket);
		menu [0].m_bRebuild = 1;
#if 1			
		console.printf (0, TXT_CHANGE_SOCK, networkData.nSocket);
#endif
		NetworkListen ();
		IpxChangeDefaultSocket ((ushort) (IPX_DEFAULT_SOCKET + networkData.nSocket));
		RestartNetSearching (menu);
		NetworkSendGameListRequest ();
		return nCurItem;
		}
	}
	// send a request for game info every 3 seconds
#if DBG
if (!networkData.nActiveGames)
#endif
	if (gameStates.multi.nGameType >= IPX_GAME) {
		if (t > t1 + 3000) {
			if (!NetworkSendGameListRequest ())
				return nCurItem;
			t1 = t;
			}
		}
NetworkListen ();
DeleteTimedOutNetGames ();
if (networkData.bGamesChanged || (networkData.nActiveGames != networkData.nLastActiveGames)) {
	networkData.bGamesChanged = 0;
	networkData.nLastActiveGames = networkData.nActiveGames;
#if 1			
	console.printf (CON_DBG, "Found %d netgames.\n", networkData.nActiveGames);
#endif
	// Copy the active games data into the menu options
	for (i = 0; i < networkData.nActiveGames; i++, h++) {
			int gameStatus = activeNetGames [i].gameStatus;
			int nplayers = 0;
			char szLevelName [20], szMissionName [50], szGameName [50];
			int nLevelVersion = gameOpts->menus.bShowLevelVersion ? FindMissionByName (activeNetGames [i].szMissionName, -1) : -1;

		// These next two loops protect against menu skewing
		// if missiontitle or gamename contain a tab

		PruneText (szMissionName, activeNetGames [i].szMissionTitle, sizeof (szMissionName), 4, nLevelVersion);
		PruneText (szGameName, activeNetGames [i].szGameName, sizeof (szGameName), 1, -1);
		nplayers = activeNetGames [i].nConnected;
		if (activeNetGames [i].nLevel < 0)
			sprintf (szLevelName, "S%d", -activeNetGames [i].nLevel);
		else
			sprintf (szLevelName, "%d", activeNetGames [i].nLevel);
		if (gameStatus == NETSTAT_STARTING)
			psz = "Forming";
		else if (gameStatus == NETSTAT_PLAYING) {
			nJoinStatus = CanJoinNetgame (activeNetGames + i, NULL);

			if (nJoinStatus == 1)
				psz = "Open";
			else if (nJoinStatus == 2)
				psz = "Full";
			else if (nJoinStatus == 3)
				psz = "Restrict";
			else
				psz = "Closed";
			}
		else 
			psz = "Between";
		sprintf (szOption, "%2d.\t%s\t%s\t%d/%d\t%s\t%s\t%s", 
					i + 1, szGameName, szModeLetters [activeNetGames [i].gameMode], nplayers, 
					activeNetGames [i].nMaxPlayers, szMissionName, szLevelName, psz);
		Assert (strlen (szOption) < 100);
		if (strcmp (szOption, menu [h].m_text)) {
			memcpy (menu [h].m_text, szOption, 100);
			menu [h].m_bRebuild = 1;
			menu [h].m_bUnavailable = (nLevelVersion == 0);
			bPlaySound = 1;
			}
		}
	for (i = 3 + networkData.nActiveGames; i < MAX_ACTIVE_NETGAMES; i++, h++)
		InitNetgameMenuOption (menu, h);
	}
#if 0
else
	for (i = 3; i < networkData.nActiveGames; i++, h++)
		menu [h].m_value = SDL_GetTicks ();
for (i = 3 + networkData.nActiveGames; i < MAX_ACTIVE_NETGAMES; i++, h++)
	if (menu [h].m_value && (t - menu [h].m_value > 10000)) 
	 {
		InitNetgameMenuOption (menu, h);
		bPlaySound = 1;
		}
#endif
if (bPlaySound)
	DigiPlaySample (SOUND_HUD_MESSAGE, F1_0);
return nCurItem;
}


//------------------------------------------------------------------------------

int NetworkBrowseGames (void)
{
	CMenu	m (MAX_ACTIVE_NETGAMES + 5);
	int	choice, i, bAutoRun = gameData.multiplayer.autoNG.bValid;
	char	callsign [CALLSIGN_LEN+1];

//PrintLog ("launching netgame browser\n");
memcpy (callsign, LOCALPLAYER.callsign, sizeof (callsign));
if (gameStates.multi.nGameType >= IPX_GAME) {
	if (!networkData.bActive) {
		MsgBox (NULL, NULL, 1, TXT_OK, TXT_IPX_NOT_FOUND);
		return 0;
		}
	}
//PrintLog ("   NetworkInit\n");
NetworkInit ();
gameData.multiplayer.nPlayers = 0;
setjmp (gameExitPoint);
networkData.nJoining = 0; 
networkData.nJoinState = 0;
networkData.nStatus = NETSTAT_BROWSING; // We are looking at a game menu
IpxChangeDefaultSocket ((ushort) (IPX_DEFAULT_SOCKET + networkData.nSocket));
//PrintLog ("   NetworkFlush\n");
NetworkFlush ();
//PrintLog ("   NetworkListen\n");
NetworkListen ();  // Throw out old info
//PrintLog ("   NetworkSendGameListRequest\n");
NetworkSendGameListRequest (); // broadcast a request for lists
networkData.nActiveGames = 0;
networkData.nLastActiveGames = 0;
memset (activeNetGames, 0, sizeof (activeNetGames));
memset (activeNetPlayers, 0, sizeof (activeNetPlayers));
if (!bAutoRun) {
	fontManager.SetColorRGBi (RGBA_PAL (15, 15, 23), 1, 0, 0);
	m.AddText (szNMTextBuffer [0]);
	m.Top ()->m_bNoScroll = 1;
	m.Top ()->m_x = (short) 0x8000;	//centered
	if (gameStates.multi.nGameType >= IPX_GAME) {
		if (networkData.bAllowSocketChanges)
			sprintf (m.Top ()->m_text, TXT_CURR_SOCK, (gameStates.multi.nGameType == IPX_GAME) ? "IPX" : "UDP", networkData.nSocket);
		else
			*m.Top ()->m_text = '\0';
		}
	i = 1;
	if (gameStates.multi.bUseTracker) {
		m.AddText (szNMTextBuffer [i]);
		strcpy (m.Top ()->m_text, TXT_0TRACKERS);
		m.Top ()->m_x = (short) 0x8000;
		m.Top ()->m_bNoScroll = 1;
		}
	m.AddText (szNMTextBuffer [i]);
	strcpy (m.Top ()->m_text, TXT_GAME_BROWSER);
	m.Top ()->m_bNoScroll = 1;
	InitNetgameMenu (m, 0);
	}
networkData.bGamesChanged = 1;    

doMenu:

gameStates.app.nExtGameStatus = GAMESTAT_JOIN_NETGAME;
if (bAutoRun) {
	fix t, t0 = 0;
	do {
		if ((t = SDL_GetTicks ()) > t0 + F1_0) {
			t0 = t;
			NetworkSendGameListRequest ();
			}
		NetworkListen ();
		if (KeyInKey () == KEY_ESC)
			return 0;
		} while (!networkData.nActiveGames);
	choice = 0;
	}
else {
	gameStates.multi.bSurfingNet = 1;
//	NMLoadBackground (MenuPCXName (), &bg, 0);             //load this here so if we abort after loading level, we restore the palette
//	paletteManager.LoadEffect  ();
	choice = m.Menu (TXT_NETGAMES, NULL, NetworkJoinPoll, NULL, NULL, LHX (340), -1, 1);
//	backgroundManager.Remove ();
	gameStates.multi.bSurfingNet = 0;
	}
if (choice == -1) {
	ChangePlayerNumTo (0);
	memcpy (LOCALPLAYER.callsign, callsign, sizeof (callsign));
	networkData.nStatus = NETSTAT_MENU;
	return 0; // they cancelled               
	}               
choice -= (2 + gameStates.multi.bUseTracker);
if ((choice < 0) || (choice >= networkData.nActiveGames)) {
	//MsgBox (TXT_SORRY, NULL, 1, TXT_OK, TXT_INVALID_CHOICE);
	goto doMenu;
	}

// Choice has been made and looks legit
if (AGI.gameStatus == NETSTAT_ENDLEVEL) {
	MsgBox (TXT_SORRY, NULL, 1, TXT_OK, TXT_NET_GAME_BETWEEN2);
	goto doMenu;
	}
if (AGI.protocolVersion != MULTI_PROTO_VERSION) {
	if (AGI.protocolVersion == 3) {
		MsgBox (TXT_SORRY, NULL, 1, TXT_OK, TXT_INCOMPAT1);
		}
	else if (AGI.protocolVersion == 4) {
		}
	else {
		char	szFmt [200], szError [200];

		sprintf (szFmt, "%s%s", TXT_VERSION_MISMATCH, TXT_NETGAME_VERSIONS);
		sprintf (szError, szFmt, MULTI_PROTO_VERSION, AGI.protocolVersion);
		MsgBox (TXT_SORRY, NULL, 1, TXT_OK, szError);
		}
	goto doMenu;
	}

if (gameStates.multi.bUseTracker) {
	//PrintLog ("   getting server lists from trackers\n");
	GetServerFromList (choice);
	}
// Check for valid mission name
console.printf (CON_DBG, TXT_LOADING_MSN, AGI.szMissionName);
if (!(LoadMissionByName (AGI.szMissionName, -1) ||
		(DownloadMission (AGI.szMissionName) &&
		 LoadMissionByName (AGI.szMissionName, -1)))) {
	PrintLog ("Mission '%s' not found%s\n", AGI.szMissionName);
	MsgBox (NULL, NULL, 1, TXT_OK, TXT_MISSION_NOT_FOUND);
	goto doMenu;
	}
if (IS_D2_OEM && (AGI.nLevel > 8)) {
	MsgBox (NULL, NULL, 1, TXT_OK, TXT_OEM_ONLY8);
	goto doMenu;
	}
if (IS_MAC_SHARE && (AGI.nLevel > 4)) {
	MsgBox (NULL, NULL, 1, TXT_OK, TXT_SHARE_ONLY4);
	goto doMenu;
	}
if (!NetworkWaitForAllInfo (choice)) {
	MsgBox (TXT_SORRY, NULL, 1, TXT_OK, TXT_JOIN_ERROR);
	networkData.nStatus = NETSTAT_BROWSING; // We are looking at a game menu
	goto doMenu;
	}       

networkData.nStatus = NETSTAT_BROWSING; // We are looking at a game menu
  if (!CanJoinNetgame (activeNetGames + choice, activeNetPlayers + choice)) {
	if (AGI.nNumPlayers == AGI.nMaxPlayers)
		MsgBox (TXT_SORRY, NULL, 1, TXT_OK, TXT_GAME_FULL);
	else
		MsgBox (TXT_SORRY, NULL, 1, TXT_OK, TXT_IN_PROGRESS);
	goto doMenu;
	}
// Choice is valid, prepare to join in
memcpy (&netGame, activeNetGames + choice, sizeof (tNetgameInfo));
memcpy (&netPlayers, activeNetPlayers + choice, sizeof (tAllNetPlayersInfo));
gameStates.app.nDifficultyLevel = netGame.difficulty;
gameData.multiplayer.nMaxPlayers = netGame.nMaxPlayers;
ChangePlayerNumTo (1);
memcpy (LOCALPLAYER.callsign, callsign, sizeof (callsign));
// Handle the extra data for the network driver
// For the mcast4 driver, this is the game's multicast address, to
// which the driver subscribes.
if (IpxHandleNetGameAuxData (netGame.AuxData) < 0) {
	networkData.nStatus = NETSTAT_BROWSING;
	goto doMenu;
	}
NetworkSetGameMode (netGame.gameMode);
NetworkAdjustMaxDataSize ();
//PrintLog ("loading level\n");
StartNewLevel (netGame.nLevel, 0);
//PrintLog ("exiting netgame browser\n");
backgroundManager.Remove ();
return 1;         // look ma, we're in a game!!!
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

