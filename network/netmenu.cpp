#ifdef HAVE_CONFIG_H
#	include <conf.h>
#endif

#ifdef RCS
static char rcsid [] = "$Id: network.c, v 1.24 2003/10/12 09:38:48 btb Exp $";
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
#include "mono.h"
#include "ipx.h"
#include "newmenu.h"
#include "key.h"
#include "gauges.h"
#include "object.h"
#include "error.h"
#include "laser.h"
#include "gamesave.h"
#include "gamemine.h"
#include "player.h"
#include "loadgame.h"
#include "fireball.h"
#include "network.h"
#include "game.h"
#include "multi.h"
#include "endlevel.h"
#include "palette.h"
#include "cntrlcen.h"
#include "powerup.h"
#include "menu.h"
#include "sounds.h"
#include "text.h"
#include "highscores.h"
#include "newdemo.h"
#include "multibot.h"
#include "wall.h"
#include "bm.h"
#include "effects.h"
#include "physics.h"
#include "switch.h"
#include "automap.h"
#include "byteswap.h"
#include "netmisc.h"
#include "kconfig.h"
#include "playsave.h"
#include "cfile.h"
#include "ipx.h"
#ifdef _WIN32
#	include "win32/include/ipx_udp.h"
#	include "win32/include/ipx_drv.h"
#else
#	include "linux/include/ipx_udp.h"
#	include "linux/include/ipx_drv.h"
#endif
#include "autodl.h"
#include "tracker.h"
#include "newmenu.h"
#include "gamefont.h"
#include "gameseg.h"
#include "multi.h"
#include "collide.h"
#include "hudmsg.h"
#include "vers_id.h"

#define LHX(x)      (gameStates.menus.bHires?2* (x):x)
#define LHY(y)      (gameStates.menus.bHires? (24* (y))/10:y)

#define AGI	activeNetGames [choice]
#define AXI activeExtraGameInfo [choice]

/* the following are the possible packet identificators.
 * they are stored in the "nType" field of the packet structs.
 * they are offset 4 bytes from the beginning of the raw IPX data
 * because of the "driver's" ipx_packetnum (see linuxnet.c).
 */


char szNMTextBuffer [MAX_ACTIVE_NETGAMES + 5][100];

extern struct ipx_recv_data ipx_udpSrc;

extern void SetFunctionMode (int);
extern unsigned char ipx_MyAddress [10];

//------------------------------------------------------------------------------

void NetworkSetWeaponsAllowed (void)
 {
  int opt = 0, choice, optPrimary, optSecond, opt_power;
  tMenuItem m [40];
  
  optPrimary = opt;
  memset (m, 0, sizeof (m));
  ADD_CHECK (opt, TXT_WA_LASER, netGame.DoLaserUpgrade, 0, NULL); opt++;
  ADD_CHECK (opt, TXT_WA_SLASER, netGame.DoSuperLaser, 0, NULL); opt++;
  ADD_CHECK (opt, TXT_WA_QLASER, netGame.DoQuadLasers, 0, NULL); opt++;
  ADD_CHECK (opt, TXT_WA_VULCAN, netGame.DoVulcan, 0, NULL); opt++;
  ADD_CHECK (opt, TXT_WA_SPREAD, netGame.DoSpread, 0, NULL); opt++;
  ADD_CHECK (opt, TXT_WA_PLASMA, netGame.DoPlasma, 0, NULL); opt++;
  ADD_CHECK (opt, TXT_WA_FUSION, netGame.DoFusions, 0, NULL); opt++;
  ADD_CHECK (opt, TXT_WA_GAUSS, netGame.DoGauss, 0, NULL); opt++;
  ADD_CHECK (opt, TXT_WA_HELIX, netGame.DoHelix, 0, NULL); opt++;
  ADD_CHECK (opt, TXT_WA_PHOENIX, netGame.DoPhoenix, 0, NULL); opt++;
  ADD_CHECK (opt, TXT_WA_OMEGA, netGame.DoOmega, 0, NULL); opt++;
  
  optSecond = opt;   
  ADD_CHECK (opt, TXT_WA_HOMING_MSL, netGame.DoHoming, 0, NULL); opt++;
  ADD_CHECK (opt, TXT_WA_PROXBOMB, netGame.DoProximity, 0, NULL); opt++;
  ADD_CHECK (opt, TXT_WA_SMART_MSL, netGame.DoSmarts, 0, NULL); opt++;
  ADD_CHECK (opt, TXT_WA_MEGA_MSL, netGame.DoMegas, 0, NULL); opt++;
  ADD_CHECK (opt, TXT_WA_FLASH_MSL, netGame.DoFlash, 0, NULL); opt++;
  ADD_CHECK (opt, TXT_WA_GUIDED_MSL, netGame.DoGuided, 0, NULL); opt++;
  ADD_CHECK (opt, TXT_WA_SMARTMINE, netGame.DoSmartMine, 0, NULL); opt++;
  ADD_CHECK (opt, TXT_WA_MERC_MSL, netGame.DoMercury, 0, NULL); opt++;
  ADD_CHECK (opt, TXT_WA_SHAKER_MSL, netGame.DoEarthShaker, 0, NULL); opt++;

  opt_power = opt;
  ADD_CHECK (opt, TXT_WA_INVUL, netGame.DoInvulnerability, 0, NULL); opt++;
  ADD_CHECK (opt, TXT_WA_CLOAK, netGame.DoCloak, 0, NULL); opt++;
  ADD_CHECK (opt, TXT_WA_BURNER, netGame.DoAfterburner, 0, NULL); opt++;
  ADD_CHECK (opt, TXT_WA_AMMORACK, netGame.DoAmmoRack, 0, NULL); opt++;
  ADD_CHECK (opt, TXT_WA_CONVERTER, netGame.DoConverter, 0, NULL); opt++;
  ADD_CHECK (opt, TXT_WA_HEADLIGHT, netGame.DoHeadlight, 0, NULL); opt++;
  
  choice = ExecMenu (NULL, TXT_WA_OBJECTS, opt, m, NULL, NULL);

  netGame.DoLaserUpgrade = m [optPrimary].value; 
  netGame.DoSuperLaser = m [optPrimary+1].value;
  netGame.DoQuadLasers = m [optPrimary+2].value;  
  netGame.DoVulcan = m [optPrimary+3].value;
  netGame.DoSpread = m [optPrimary+4].value;
  netGame.DoPlasma = m [optPrimary+5].value;
  netGame.DoFusions = m [optPrimary+6].value;
  netGame.DoGauss = m [optPrimary+7].value;
  netGame.DoHelix = m [optPrimary+8].value;
  netGame.DoPhoenix = m [optPrimary+9].value;
  netGame.DoOmega = m [optPrimary+10].value;
  
  netGame.DoHoming = m [optSecond].value;
  netGame.DoProximity = m [optSecond+1].value;
  netGame.DoSmarts = m [optSecond+2].value;
  netGame.DoMegas = m [optSecond+3].value;
  netGame.DoFlash = m [optSecond+4].value;
  netGame.DoGuided = m [optSecond+5].value;
  netGame.DoSmartMine = m [optSecond+6].value;
  netGame.DoMercury = m [optSecond+7].value;
  netGame.DoEarthShaker = m [optSecond+8].value;

  netGame.DoInvulnerability = m [opt_power].value;
  netGame.DoCloak = m [opt_power+1].value;
  netGame.DoAfterburner = m [opt_power+2].value;
  netGame.DoAmmoRack = m [opt_power+3].value;
  netGame.DoConverter = m [opt_power+4].value;     
  netGame.DoHeadlight = m [opt_power+5].value;     
  
 }

//------------------------------------------------------------------------------

static int 
	optCapVirLim, optCapTimLim, optMaxVirCap, optBumpVirCap, optBashVirCap, optVirGenTim, 
	optVirLife, optVirStab, optRevRooms, optEnergyFill, optShieldFill, optShieldDmg, 
	optOvrTex, optBrRooms, optFF, optSuicide, optPlrHand, optTogglesMenu, optTextureMenu;

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

void NetworkEndLevelPoll2 (int nitems, tMenuItem * menus, int * key, int cItem)
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
		*key = -3;
	else
		*key = -2;
	}
}


//------------------------------------------------------------------------------

void NetworkEndLevelPoll3 (int nitems, tMenuItem * menus, int * key, int cItem)
{
	// Polling loop for End-of-level menu
   int num_ready = 0, i;
 
if (TimerGetApproxSeconds () > (gameData.multiplayer.xStartAbortMenuTime+ (F1_0 * 8)))
	*key = -2;
NetworkListen ();
for (i = 0; i < gameData.multiplayer.nPlayers; i++)
	if ((gameData.multiplayer.players [i].connected != 1) && (gameData.multiplayer.players [i].connected != 5) && (gameData.multiplayer.players [i].connected != 6))
		num_ready++;
if (num_ready == gameData.multiplayer.nPlayers) // All players have checked in or are disconnected
	*key = -2;
}

//------------------------------------------------------------------------------

void NetworkStartPoll (int nitems, tMenuItem * menus, int * key, int cItem)
{
	int i, n, nm;

	key = key;
	cItem = cItem;

Assert (networkData.nStatus == NETSTAT_STARTING);
if (!menus [0].value) {
	menus [0].value = 1;
	menus [0].rebuild = 1;
	}
for (i = 1; i < nitems; i++) {
	if ((i >= gameData.multiplayer.nPlayers) && menus [i].value) {
		menus [i].value = 0;
		menus [i].rebuild = 1;
		}
	}
nm = 0;
for (i = 0; i < nitems; i++) {
	if (menus [i].value) {
		if (++nm > gameData.multiplayer.nPlayers)   {
			menus [i].value = 0;
			menus [i].rebuild = 1;
			}
		}
	}
if (nm > gameData.multiplayer.nMaxPlayers) {
	ExecMessageBox (TXT_ERROR, NULL, 1, TXT_OK, "%s %d %s", TXT_SORRY_ONLY, gameData.multiplayer.nMaxPlayers, TXT_NETPLAYERS_IN);
	// Turn off the last tPlayer highlighted
	for (i = gameData.multiplayer.nPlayers; i > 0; i--)
		if (menus [i].value == 1) {
			menus [i].value = 0;
			menus [i].rebuild = 1;
			break;
			}
	}

//       if (nitems > MAX_PLAYERS) return; 

n = netGame.nNumPlayers;
NetworkListen ();

if (n < netGame.nNumPlayers) {
	DigiPlaySample (SOUND_HUD_MESSAGE, F1_0);
	if (gameOpts->multi.bNoRankings)
	   sprintf (menus [gameData.multiplayer.nPlayers - 1].text, "%d. %-20s", 
					gameData.multiplayer.nPlayers, 
					netPlayers.players [gameData.multiplayer.nPlayers-1].callsign);
	else
	   sprintf (menus [gameData.multiplayer.nPlayers - 1].text, "%d. %s%-20s", 
					gameData.multiplayer.nPlayers, 
					pszRankStrings [netPlayers.players [gameData.multiplayer.nPlayers-1].rank], 
					netPlayers.players [gameData.multiplayer.nPlayers-1].callsign);
	menus [gameData.multiplayer.nPlayers - 1].rebuild = 1;
	if (gameData.multiplayer.nPlayers <= gameData.multiplayer.nMaxPlayers)
		menus [gameData.multiplayer.nPlayers - 1].value = 1;
	} 
else if (n > netGame.nNumPlayers) {
	// One got removed...
   DigiPlaySample (SOUND_HUD_KILL, F1_0);
	for (i = 0; i < gameData.multiplayer.nPlayers; i++) {
		if (gameOpts->multi.bNoRankings)
			sprintf (menus [i].text, "%d. %-20s", i+1, netPlayers.players [i].callsign);
		else
			sprintf (menus [i].text, "%d. %s%-20s", i+1, pszRankStrings [netPlayers.players [i].rank], netPlayers.players [i].callsign);
		menus [i].value = (i < gameData.multiplayer.nMaxPlayers);
		menus [i].rebuild = 1;
		}
	for (i = gameData.multiplayer.nPlayers; i<n; i++)  {
		sprintf (menus [i].text, "%d. ", i+1);          // Clear out the deleted entries...
		menus [i].value = 0;
		menus [i].rebuild = 1;
		}
   }
}

//------------------------------------------------------------------------------

static int optGameTypes, nGameTypes, nGameItem;

void NetworkGameParamPoll (int nitems, tMenuItem * menus, int * key, int cItem)
{
	static int oldmaxnet = 0;

if ((cItem >= optGameTypes) && (cItem < optGameTypes + nGameTypes)) {
	if ((cItem != nGameItem) && (menus [cItem].value)) {
		nGameItem = cItem;
		*key = -2;
		return;
		}
	}
if ((menus [optEntropy].value == (optEntOpts < 0)) ||
	 (menus [optMonsterball].value == (optMBallOpts < 0)))
	*key = -2;
//force restricted game for team games
//obsolete with D2X-W32 as it can assign players to teams automatically
//even in a match and progress, and allows players to switch teams
if (menus [optCoop].value) {
	oldmaxnet = 1;
	if (menus [optMaxNet].value>2)  {
		menus [optMaxNet].value = 2;
		menus [optMaxNet].redraw = 1;
		}
	if (menus [optMaxNet].maxValue>2) {
		menus [optMaxNet].maxValue = 2;
		menus [optMaxNet].redraw = 1;
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
		menus [optMaxNet].value = 6;
		menus [optMaxNet].maxValue = 6;
		}
	}         
if (lastMaxNet != menus [optMaxNet].value)  {
	sprintf (menus [optMaxNet].text, TXT_MAX_PLAYERS, menus [optMaxNet].value+2);
	lastMaxNet = menus [optMaxNet].value;
	menus [optMaxNet].rebuild = 1;
	}               
 }

//------------------------------------------------------------------------------

fix LastPTA;
int LastKillGoal;

// Jeez -- mac compiler can't handle all of these on the same decl line.
int optSetPower, optPlayTime, optKillGoal, optSocket, optMarkerView, optLight;
int optDifficulty, optPPS, optShortPkts, optBrightPlayers, optStartInvul;
int optDarkness, optTeamDoors, optMultiCheats, optTgtInd, optDmgInd, optMslLockInd, optFriendlyInd, optHitInd;
int optHeadlights, optPowerupLights, optSpotSize;
int optShowNames, optAutoTeams, optDualMiss, optRotateLevels, optDisableReactor;
int optMouseLook, optFastPitch, optSafeUDP, optTowFlags, optCompetition, optPenalty;

//------------------------------------------------------------------------------

void NetworkMoreOptionsPoll (int nitems, tMenuItem * menus, int * key, int cItem)
{
if (nLastReactorLife != menus [optReactorLife].value)   {
	sprintf (menus [optReactorLife].text, "%s: %d %s", TXT_REACTOR_LIFE, menus [optReactorLife].value*5, TXT_MINUTES_ABBREV);
	nLastReactorLife = menus [optReactorLife].value;
	menus [optReactorLife].rebuild = 1;
   }
  
if (menus [optPlayTime].value != LastPTA) {
	if (gameData.app.nGameMode & GM_MULTI_COOP) {
		LastPTA = 0;
		ExecMessageBox ("Sorry", NULL, 1, TXT_OK, TXT_COOP_ERROR);
		menus [optPlayTime].value = 0;
		menus [optPlayTime].rebuild = 1;
		return;
		}

	netGame.xPlayTimeAllowed = mpParams.nMaxTime = menus [optPlayTime].value;
	sprintf (menus [optPlayTime].text, TXT_MAXTIME, netGame.xPlayTimeAllowed*5, TXT_MINUTES_ABBREV);
	LastPTA = netGame.xPlayTimeAllowed;
	menus [optPlayTime].rebuild = 1;
	}
if (menus [optKillGoal].value!= LastKillGoal) {
	if (gameData.app.nGameMode & GM_MULTI_COOP) {
		ExecMessageBox ("Sorry", NULL, 1, TXT_OK, TXT_COOP_ERROR);
		menus [optKillGoal].value = 0;
		menus [optKillGoal].rebuild = 1;
		LastKillGoal = 0;
		return;
		}

mpParams.nKillGoal = netGame.KillGoal = menus [optKillGoal].value;
	sprintf (menus [optKillGoal].text, TXT_KILLGOAL, netGame.KillGoal*5);
	LastKillGoal = netGame.KillGoal;
	menus [optKillGoal].rebuild = 1;
	}
}

//------------------------------------------------------------------------------

void NetworkMoreGameOptions ()
 {
  int		opt = 0, i, choice = 0;
  char	szPlayTime [80], szKillGoal [80], szInvul [50],
			socket_string [6], packstring [6];
  tMenuItem m [40];

do {
	memset (m, 0, sizeof (m));
	opt = 0;
	ADD_SLIDER (opt, TXT_DIFFICULTY, mpParams.nDifficulty, 0, NDL - 1, KEY_D, HTX_GPLAY_DIFFICULTY); 
	optDifficulty = opt++;
	sprintf (szInvul + 1, "%s: %d %s", TXT_REACTOR_LIFE, mpParams.nReactorLife * 5, TXT_MINUTES_ABBREV);
	strupr (szInvul + 1);
	*szInvul = * (TXT_REACTOR_LIFE - 1);
	ADD_SLIDER (opt, szInvul + 1, mpParams.nReactorLife, 0, 10, KEY_R, HTX_MULTI2_REACTOR); 
	optReactorLife = opt++;
	sprintf (szPlayTime + 1, TXT_MAXTIME, netGame.xPlayTimeAllowed*5, TXT_MINUTES_ABBREV);
	*szPlayTime = * (TXT_MAXTIME - 1);
	ADD_SLIDER (opt, szPlayTime + 1, mpParams.nMaxTime, 0, 10, KEY_T, HTX_MULTI2_LVLTIME); 
	optPlayTime = opt++;
	sprintf (szKillGoal + 1, TXT_KILLGOAL, netGame.KillGoal*5);
	*szKillGoal = * (TXT_KILLGOAL - 1);
	ADD_SLIDER (opt, szKillGoal + 1, mpParams.nKillGoal, 0, 10, KEY_K, HTX_MULTI2_KILLGOAL);
	optKillGoal = opt++;
	ADD_CHECK (opt, TXT_INVUL_RESPAWN, mpParams.bInvul, KEY_I, HTX_MULTI2_INVUL);
	optStartInvul = opt++;
	ADD_CHECK (opt, TXT_MARKER_CAMS, mpParams.bMarkerView, KEY_C, HTX_MULTI2_MARKERCAMS);
	optMarkerView = opt++;
	ADD_CHECK (opt, TXT_KEEP_LIGHTS, mpParams.bAlwaysBright, KEY_L, HTX_MULTI2_KEEPLIGHTS);
	optLight = opt++;
	ADD_CHECK (opt, TXT_BRIGHT_SHIPS, mpParams.bBrightPlayers ? 0 : 1, KEY_S, HTX_MULTI2_BRIGHTSHIP);
	optBrightPlayers = opt++;
	ADD_CHECK (opt, TXT_SHOW_NAMES, mpParams.bShowAllNames, KEY_E, HTX_MULTI2_SHOWNAMES);
	optShowNames = opt++;
	ADD_CHECK (opt, TXT_SHOW_PLAYERS, mpParams.bShowPlayersOnAutomap, KEY_A, HTX_MULTI2_SHOWPLRS);
	optPlayersOnMap = opt++;
	ADD_CHECK (opt, TXT_SHORT_PACKETS, mpParams.bShortPackets, KEY_H, HTX_MULTI2_SHORTPKTS);
	optShortPkts = opt++;
	ADD_TEXT (opt, "", 0);
	opt++;
	ADD_MENU (opt, TXT_WAOBJECTS_MENU, KEY_O, HTX_MULTI2_OBJECTS);
	optSetPower = opt++;
	ADD_TEXT (opt, "", 0);
	opt++;
	sprintf (socket_string, "%d", (gameStates.multi.nGameType == UDP_GAME) ? udpBasePort [1] + networkData.nSocket : networkData.nSocket);
	if (gameStates.multi.nGameType >= IPX_GAME) {
		ADD_TEXT (opt, TXT_SOCKET2, KEY_N);
		opt++;
		ADD_INPUT (opt, socket_string, 5, HTX_MULTI2_SOCKET);
		optSocket = opt++;
		}

	sprintf (packstring, "%d", mpParams.nPPS);
	ADD_TEXT (opt, TXT_PPS, KEY_P);
	opt++;
	ADD_INPUT (opt, packstring, 2, HTX_MULTI2_PPS);
	optPPS = opt++;

	LastKillGoal = netGame.KillGoal;
	LastPTA = mpParams.nMaxTime;

doMenu:

	gameStates.app.nExtGameStatus = GAMESTAT_MORE_NETGAME_OPTIONS; 
	Assert (sizeofa (m) >= opt);
	i = ExecMenu1 (NULL, TXT_MORE_MPOPTIONS, opt, m, NetworkMoreOptionsPoll, &choice);
	} while (i == -2);

   //mpParams.nReactorLife = atoi (szInvul)*60*F1_0;
mpParams.nReactorLife = m [optReactorLife].value;
netGame.control_invulTime = mpParams.nReactorLife * 5 * F1_0 * 60;

if (i == optSetPower) {
	NetworkSetWeaponsAllowed ();
	GetAllAllowables ();
	goto doMenu;
	}
mpParams.nPPS = atoi (packstring);
if (mpParams.nPPS > 20) {
	mpParams.nPPS = 20;
	ExecMessageBox (TXT_ERROR, NULL, 1, TXT_OK, TXT_PPS_HIGH_ERROR);
}
else if (mpParams.nPPS<2) {
	ExecMessageBox (TXT_ERROR, NULL, 1, TXT_OK, TXT_PPS_HIGH_ERROR);
	mpParams.nPPS = 2;      
}
netGame.nPacketsPerSec = mpParams.nPPS;
if (gameStates.multi.nGameType >= IPX_GAME) { 
	int newSocket = atoi (socket_string);
	if ((newSocket < -0xFFFF) || (newSocket > 0xFFFF))
		ExecMessageBox (TXT_ERROR, NULL, 1, TXT_OK, 
							TXT_INV_SOCKET, 
							(gameStates.multi.nGameType == UDP_GAME) ? udpBasePort [1] : networkData.nSocket);
	else if (newSocket != networkData.nSocket) {
		networkData.nSocket = (gameStates.multi.nGameType == UDP_GAME) ? newSocket - UDP_BASEPORT : newSocket;
		IpxChangeDefaultSocket ((ushort) (IPX_DEFAULT_SOCKET + networkData.nSocket));
		}
	}

netGame.invul = m [optStartInvul].value;
mpParams.bInvul = (ubyte) netGame.invul;
netGame.BrightPlayers = m [optBrightPlayers].value ? 0 : 1;
mpParams.bBrightPlayers = (ubyte) netGame.BrightPlayers;
mpParams.bShortPackets = netGame.bShortPackets = m [optShortPkts].value;
netGame.ShowAllNames = m [optShowNames].value;
mpParams.bShowAllNames = (ubyte) netGame.ShowAllNames;
NetworkAdjustMaxDataSize ();
//  extraGameInfo [0].bEnhancedCTF = (m [optEnhancedCTF].value != 0);

netGame.bAllowMarkerView = m [optMarkerView].value;
mpParams.bMarkerView = (ubyte) netGame.bAllowMarkerView;
netGame.AlwaysLighting = m [optLight].value; 
mpParams.bAlwaysBright = (ubyte) netGame.AlwaysLighting;
mpParams.nDifficulty = gameStates.app.nDifficultyLevel = m [optDifficulty].value;
if ((mpParams.bShowPlayersOnAutomap = m [optPlayersOnMap].value))
	netGame.gameFlags |= NETGAME_FLAG_SHOW_MAP;
else
	netGame.gameFlags &= ~NETGAME_FLAG_SHOW_MAP;
}

//------------------------------------------------------------------------------

void NetworkD2XOptionsPoll (int nitems, tMenuItem * menus, int * key, int cItem)
{
	int	v, j;

v = menus [optCompetition].value;
if (v != extraGameInfo [1].bCompetition) {
	extraGameInfo [1].bCompetition = v;
	*key = -2;
	return;
	}
if (optPenalty > 0) {
	v = menus [optPenalty].value;
	if (v != extraGameInfo [1].nCoopPenalty) {
		extraGameInfo [1].nCoopPenalty = v;
		sprintf (menus [optPenalty].text, TXT_COOP_PENALTY, nCoopPenalties [v], '%');
		menus [optPenalty].rebuild = 1;
		return;
		}
	}
v = menus [optDarkness].value;
if (v != extraGameInfo [1].bDarkness) {
	extraGameInfo [1].bDarkness = v;
	*key = -2;
	return;
	}
if (optTgtInd >= 0) {
	v = menus [optTgtInd].value;
	if (v != (extraGameInfo [1].bTargetIndicators == 0)) {
		for (j = 0; j < 3; j++)
			if (menus [optTgtInd + j].value) {
				extraGameInfo [1].bTargetIndicators = j;
				break;
				}
		*key = -2;
		return;
		}
	}
if (optDmgInd >= 0) {
	v = menus [optDmgInd].value;
	if (v != extraGameInfo [1].bDamageIndicators) {
		extraGameInfo [1].bDamageIndicators = v;
		*key = -2;
		return;
		}
	}
if (optHeadlights >= 0) {
	v = menus [optHeadlights].value;
	if (v == extraGameInfo [1].headlight.bAvailable) {
		extraGameInfo [1].headlight.bAvailable = !v;
		*key = -2;
		return;
		}
	}
if (optSpotSize >= 0) {
	v = menus [optSpotSize].value;
	if (v != extraGameInfo [1].nSpotSize) {
		extraGameInfo [1].nSpotSize =
		extraGameInfo [1].nSpotStrength = v;
		sprintf (menus [optSpotSize].text, TXT_SPOTSIZE, GT (664 + v));
		menus [optSpotSize].rebuild = 1;
		return;
		}
	}
}

//------------------------------------------------------------------------------

void NetworkD2XOptions ()
 {
  int		opt = 0, i, j, choice = 0, optCheckPort = -1;
  char	szSpotSize [50];
  char	szPenalty [50];
  tMenuItem m [40];

do {
	memset (m, 0, sizeof (m));
	opt = 0;
	ADD_CHECK (opt, TXT_COMPETITION_MODE, extraGameInfo [1].bCompetition, KEY_C, HTX_MULTI2_COMPETITION);
	optCompetition = opt++;
	if (!extraGameInfo [1].bCompetition) {
		ADD_CHECK (opt, TXT_FRIENDLY_FIRE, extraGameInfo [0].bFriendlyFire, KEY_F, HTX_MULTI2_FFIRE);
		optFF = opt++;
		ADD_CHECK (opt, TXT_NO_SUICIDE, extraGameInfo [0].bInhibitSuicide, KEY_U, HTX_MULTI2_SUICIDE);
		optSuicide = opt++;
		ADD_CHECK (opt, TXT_MOUSELOOK, extraGameInfo [1].bMouseLook, KEY_O, HTX_MULTI2_MOUSELOOK);
		optMouseLook = opt++;
		ADD_CHECK (opt, TXT_FASTPITCH, (extraGameInfo [1].bFastPitch == 1) ? 1 : 0, KEY_P, HTX_MULTI2_FASTPITCH);
		optFastPitch = opt++;
		ADD_CHECK (opt, TXT_DUAL_LAUNCH, extraGameInfo [1].bDualMissileLaunch, KEY_M, HTX_GPLAY_DUALLAUNCH);
		optDualMiss = opt++;
		}
	else
		optFF =
		optSuicide =
		optMouseLook =
		optFastPitch =
		optDualMiss = -1;
	if (IsTeamGame) {
		ADD_CHECK (opt, TXT_AUTOBALANCE, extraGameInfo [0].bAutoBalanceTeams, KEY_B, HTX_MULTI2_BALANCE);
		optAutoTeams = opt++;
		ADD_CHECK (opt, TXT_TEAMDOORS, mpParams.bTeamDoors, KEY_T, HTX_TEAMDOORS);
		optTeamDoors = opt++;
		}
	else
		optTeamDoors =
		optAutoTeams = -1;
	if (mpParams.nGameMode == NETGAME_CAPTURE_FLAG) {
		ADD_CHECK (opt, TXT_TOW_FLAGS, extraGameInfo [1].bTowFlags, KEY_F, HTX_TOW_FLAGS);
		optTowFlags = opt++;
		}
	else
		optTowFlags = -1;
	if (!extraGameInfo [1].bCompetition) {
		ADD_CHECK (opt, TXT_MULTICHEATS, mpParams.bEnableCheats, KEY_T, HTX_MULTICHEATS);
		optMultiCheats = opt++;
		}
	else
		optMultiCheats = -1;
	ADD_CHECK (opt, TXT_MSN_CYCLE, extraGameInfo [1].bRotateLevels, KEY_Y, HTX_MULTI2_MSNCYCLE); 
	optRotateLevels = opt++;
#if 0
		ADD_CHECK (opt, TXT_NO_REACTOR, extraGameInfo [1].bDisableReactor, KEY_R, HTX_MULTI2_NOREACTOR); 
		optDisableReactor = opt++;
#else
		optDisableReactor = -1;
#endif
#if UDP_SAFEMODE
	ADD_CHECK (opt, TXT_UDP_QUAL, extraGameInfo [0].bSafeUDP, KEY_Q, HTX_MULTI2_UDPQUAL);
	optSafeUDP = opt++;
#endif
	ADD_CHECK (opt, TXT_CHECK_PORT, extraGameInfo [0].bCheckUDPPort, KEY_P, HTX_MULTI2_CHECKPORT);
	optCheckPort = opt++;
	if (extraGameInfo [1].bDarkness) {
		ADD_TEXT (opt, "", 0);
		opt++;
		}
	ADD_CHECK (opt, TXT_DARKNESS, extraGameInfo [1].bDarkness, KEY_D, HTX_DARKNESS);
	optDarkness = opt++;
	if (extraGameInfo [1].bDarkness) {
		ADD_CHECK (opt, TXT_POWERUPLIGHTS, !extraGameInfo [1].bPowerupLights, KEY_P, HTX_POWERUPLIGHTS);
		optPowerupLights = opt++;
		ADD_CHECK (opt, TXT_HEADLIGHTS, !extraGameInfo [1].headlight.bAvailable, KEY_H, HTX_HEADLIGHTS);
		optHeadlights = opt++;
		if (extraGameInfo [1].headlight.bAvailable) {
			sprintf (szSpotSize + 1, TXT_SPOTSIZE, GT (664 + extraGameInfo [1].nSpotSize));
			strupr (szSpotSize + 1);
			*szSpotSize = *(TXT_SPOTSIZE - 1);
			ADD_SLIDER (opt, szSpotSize + 1, extraGameInfo [1].nSpotSize, 0, 2, KEY_O, HTX_SPOTSIZE); 
			optSpotSize = opt++;
			}
		else
			optSpotSize = -1;
		}
	else
		optHeadlights =
		optPowerupLights =
		optSpotSize = -1;
	ADD_TEXT (opt, "", 0);
	opt++;
	if (!extraGameInfo [1].bCompetition) {
		if (mpParams.nGameMode == NETGAME_COOPERATIVE) {
			sprintf (szPenalty + 1, TXT_COOP_PENALTY, nCoopPenalties [(int) extraGameInfo [1].nCoopPenalty], '%');
			strupr (szPenalty + 1);
			*szPenalty = *(TXT_COOP_PENALTY - 1);
			ADD_SLIDER (opt, szPenalty + 1, extraGameInfo [1].nCoopPenalty, 0, 9, KEY_O, HTX_COOP_PENALTY); 
			optPenalty = opt++;
			}
		else
			optPenalty = -1;
		ADD_RADIO (opt, TXT_TGTIND_NONE, 0, KEY_A, 1, HTX_CPIT_TGTIND);
		optTgtInd = opt++;
		ADD_RADIO (opt, TXT_TGTIND_SQUARE, 0, KEY_R, 1, HTX_CPIT_TGTIND);
		opt++;
		ADD_RADIO (opt, TXT_TGTIND_TRIANGLE, 0, KEY_T, 1, HTX_CPIT_TGTIND);
		opt++;
		m [optTgtInd + extraGameInfo [1].bTargetIndicators].value = 1;
		if (extraGameInfo [1].bTargetIndicators) {
			ADD_CHECK (opt, TXT_FRIENDLY_INDICATOR, extraGameInfo [1].bFriendlyIndicators, KEY_F, HTX_FRIENDLY_INDICATOR);
			optFriendlyInd = opt++;
			}
		else
			optFriendlyInd = -1;
		ADD_CHECK (opt, TXT_DMG_INDICATOR, extraGameInfo [1].bDamageIndicators, KEY_D, HTX_CPIT_DMGIND);
		optDmgInd = opt++;
		ADD_CHECK (opt, TXT_MSLLOCK_INDICATOR, extraGameInfo [1].bMslLockIndicators, KEY_G, HTX_CPIT_MSLLOCKIND);
		optMslLockInd = opt++;
		if (extraGameInfo [1].bTargetIndicators || extraGameInfo [1].bDamageIndicators) {
			ADD_CHECK (opt, TXT_HIT_INDICATOR, extraGameInfo [1].bTagOnlyHitObjs, KEY_T, HTX_HIT_INDICATOR);
			optHitInd = opt++;
			}
		else {
			optPenalty =
			optHitInd = -1;
			extraGameInfo [1].nCoopPenalty = 0;
			}
		ADD_TEXT (opt, "", 0);
		opt++;
		}
	else
		optTgtInd =
		optDmgInd =
		optMslLockInd = -1;
	i = ExecMenu1 (NULL, TXT_D2XOPTIONS_TITLE, opt, m, NetworkD2XOptionsPoll, &choice);
  //mpParams.nReactorLife = atoi (szInvul)*60*F1_0;
	extraGameInfo [1].bDarkness = (ubyte) m [optDarkness].value;
	if (optDarkness >= 0) {
		if ((mpParams.bDarkness = extraGameInfo [1].bDarkness)) {
			extraGameInfo [1].headlight.bAvailable = !m [optHeadlights].value;
			extraGameInfo [1].bPowerupLights = !m [optPowerupLights].value;
			}
		}
	GET_VAL (extraGameInfo [1].bTowFlags, optTowFlags);
	GET_VAL (extraGameInfo [1].bTeamDoors, optTeamDoors);
	mpParams.bTeamDoors = extraGameInfo [1].bTeamDoors;
	if (optAutoTeams >= 0)
		extraGameInfo [0].bAutoBalanceTeams = (m [optAutoTeams].value != 0);
	#if UDP_SAFEMODE
	extraGameInfo [0].bSafeUDP = (m [optSafeUDP].value != 0);
	#endif
	extraGameInfo [0].bCheckUDPPort = (m [optCheckPort].value != 0);
	extraGameInfo [1].bRotateLevels = m [optRotateLevels].value;
	if (!COMPETITION) {
		GET_VAL (extraGameInfo [1].bEnableCheats, optMultiCheats);
		mpParams.bEnableCheats = extraGameInfo [1].bEnableCheats;
		GET_VAL (extraGameInfo [0].bFriendlyFire, optFF);
		GET_VAL (extraGameInfo [0].bInhibitSuicide, optSuicide);
		GET_VAL (extraGameInfo [1].bMouseLook, optMouseLook);
		if (optFastPitch >= 0)
			extraGameInfo [1].bFastPitch = m [optFastPitch].value ? 1 : 2;
		GET_VAL (extraGameInfo [1].bDualMissileLaunch, optDualMiss);
		GET_VAL (extraGameInfo [1].bDisableReactor, optDisableReactor);
		if (optTgtInd >= 0) {
			for (j = 0; j < 3; j++)
				if (m [optTgtInd + j].value) {
					extraGameInfo [1].bTargetIndicators = j;
					break;
					}
			GET_VAL (extraGameInfo [1].bFriendlyIndicators, optFriendlyInd);
			}
		GET_VAL (extraGameInfo [1].bDamageIndicators, optDmgInd);
		GET_VAL (extraGameInfo [1].bMslLockIndicators, optMslLockInd);
		GET_VAL (extraGameInfo [1].bTagOnlyHitObjs, optHitInd);
		}
	} while (i == -2);
}

//------------------------------------------------------------------------------

#define SetTextOpt(_text) \
	m [opt].nType = NM_TYPE_TEXT; \
	m [opt++].text = _text

#define SetInputOpt(_label, _text, Value, _len) \
	SetTextOpt (_label); \
	m [opt].nType = NM_TYPE_INPUT; \
	sprintf (_text, "%d", Value); \
	m [opt].text = _text; \
	m [opt].value = Value; \
	m [opt].text_len = _len; \
	m [opt].szHelp = HTX_ONLINE_MANUAL

#define SetRadioOpt(_text, _group, _key) \
	m [opt].nType = NM_TYPE_RADIO; \
	m [opt].text = _text; \
	m [opt].value = 0; \
	m [opt].group = _group; \
	m [opt++].key = _key; \
	m [opt].szHelp = HTX_ONLINE_MANUAL

#define SetCheckOpt(_text, Value, _key) \
	m [opt].nType = NM_TYPE_CHECK; \
	m [opt].text = _text; \
	m [opt].value = Value; \
	m [opt].key = _key; \
	m [opt].szHelp = HTX_ONLINE_MANUAL

//------------------------------------------------------------------------------

void NetworkDummyCallback (int nitems, tMenuItem * menus, int * key, int cItem) {}
  
void NetworkEntropyToggleOptions ()
{
	tMenuItem	m [12];
	int				opt = 0;

memset (m, 0, sizeof (m));

ADD_CHECK (opt, TXT_ENT_HANDICAP, extraGameInfo [0].entropy.bPlayerHandicap, KEY_H, HTX_ONLINE_MANUAL);
optPlrHand = opt++;
ADD_CHECK (opt, TXT_ENT_CONQWARN, extraGameInfo [0].entropy.bDoConquerWarning, KEY_W, HTX_ONLINE_MANUAL);
optRevRooms = opt++;
ADD_CHECK (opt, TXT_ENT_REVERT, extraGameInfo [0].entropy.bRevertRooms, KEY_R, HTX_ONLINE_MANUAL);
optRevRooms = opt++;
SetTextOpt ("");
SetTextOpt (TXT_ENT_VIRSTAB);
optVirStab = opt;
SetRadioOpt (TXT_ENT_VIRSTAB_DROP, 2, KEY_D);
SetRadioOpt (TXT_ENT_VIRSTAB_ENEMY, 2, KEY_T);
SetRadioOpt (TXT_ENT_VIRSTAB_TOUCH, 2, KEY_L);
SetRadioOpt (TXT_ENT_VIRSTAB_NEVER, 2, KEY_N);
m [optVirStab + extraGameInfo [0].entropy.nVirusStability].value = 1;

Assert (sizeofa (m) >= opt);
ExecMenu1 (NULL, TXT_ENT_TOGGLES, opt, m, NetworkDummyCallback, 0);

extraGameInfo [0].entropy.bRevertRooms = m [optRevRooms].value;
extraGameInfo [0].entropy.bPlayerHandicap = m [optPlrHand].value;
for (extraGameInfo [0].entropy.nVirusStability = 0; 
	  extraGameInfo [0].entropy.nVirusStability < 3; 
	  extraGameInfo [0].entropy.nVirusStability++)
	if (m [optVirStab + extraGameInfo [0].entropy.nVirusStability].value)
		break;
}

//------------------------------------------------------------------------------

void NetworkEntropyTextureOptions ()
{
	tMenuItem	m [7];
	int				opt = 0;

memset (m, 0, sizeof (m));

optOvrTex = opt;
SetRadioOpt (TXT_ENT_TEX_KEEP, 1, KEY_K);
SetRadioOpt (TXT_ENT_TEX_OVERRIDE, 1, KEY_O);
SetRadioOpt (TXT_ENT_TEX_COLOR, 1, KEY_C);
m [optOvrTex + extraGameInfo [0].entropy.nOverrideTextures].value = 1;
SetTextOpt ("");
ADD_CHECK (opt, TXT_ENT_TEX_BRIGHTEN, extraGameInfo [0].entropy.bBrightenRooms, KEY_B, HTX_ONLINE_MANUAL);
optBrRooms = opt++;

Assert (sizeofa (m) >= opt);
ExecMenu1 (NULL, TXT_ENT_TEXTURES, opt, m, NetworkDummyCallback, 0);

extraGameInfo [0].entropy.bBrightenRooms = m [optBrRooms].value;
for (extraGameInfo [0].entropy.nOverrideTextures = 0; 
	  extraGameInfo [0].entropy.nOverrideTextures < 3; 
	  extraGameInfo [0].entropy.nOverrideTextures++)
	if (m [optOvrTex + extraGameInfo [0].entropy.nOverrideTextures].value)
		break;
}

//------------------------------------------------------------------------------

void NetworkEntropyOptions (void)
{
	tMenuItem	m [25];
	int			i, opt = 0;
	char			szCapVirLim [10], szCapTimLim [10], szMaxVirCap [10], szBumpVirCap [10], 
					szBashVirCap [10], szVirGenTim [10], szVirLife [10], 
					szEnergyFill [10], szShieldFill [10], szShieldDmg [10];

memset (m, 0, sizeof (m));

SetInputOpt (TXT_ENT_VIRLIM, szCapVirLim, extraGameInfo [0].entropy.nCaptureVirusLimit, 3);
optCapVirLim = opt++;
SetInputOpt (TXT_ENT_CAPTIME, szCapTimLim, extraGameInfo [0].entropy.nCaptureTimeLimit, 3);
optCapTimLim = opt++;
SetInputOpt (TXT_ENT_MAXCAP, szMaxVirCap, extraGameInfo [0].entropy.nMaxVirusCapacity, 6);
optMaxVirCap = opt++;
SetInputOpt (TXT_ENT_BUMPCAP, szBumpVirCap, extraGameInfo [0].entropy.nBumpVirusCapacity, 3);
optBumpVirCap = opt++;
SetInputOpt (TXT_ENT_BASHCAP, szBashVirCap, extraGameInfo [0].entropy.nBashVirusCapacity, 3);
optBashVirCap = opt++;
SetInputOpt (TXT_ENT_GENTIME, szVirGenTim, extraGameInfo [0].entropy.nVirusGenTime, 3);
optVirGenTim = opt++;
SetInputOpt (TXT_ENT_VIRLIFE, szVirLife, extraGameInfo [0].entropy.nVirusLifespan, 3);
optVirLife = opt++;
SetInputOpt (TXT_ENT_EFILL, szEnergyFill, extraGameInfo [0].entropy.nEnergyFillRate, 3);
optEnergyFill = opt++;
SetInputOpt (TXT_ENT_SFILL, szShieldFill, extraGameInfo [0].entropy.nShieldFillRate, 3);
optShieldFill = opt++;
SetInputOpt (TXT_ENT_DMGRATE, szShieldDmg, extraGameInfo [0].entropy.nShieldDamageRate, 3);
optShieldDmg = opt++;

SetTextOpt ("");
optTogglesMenu = opt;
m [opt].nType = NM_TYPE_MENU;  
m [opt].text = TXT_ENT_TGLMENU; 
m [opt++].key = KEY_E;
optTextureMenu = opt;
m [opt].nType = NM_TYPE_MENU;  
m [opt].text = TXT_ENT_TEXMENU; 
m [opt++].key = KEY_T;
Assert (sizeofa (m) >= opt);

for (;;) {
	i = ExecMenu1 (NULL, "Entropy Options", opt, m, NetworkDummyCallback, 0);
	if (i == optTogglesMenu)
		NetworkEntropyToggleOptions ();
	else if (i == optTextureMenu)
		NetworkEntropyTextureOptions ();
	else
		break;
	}
extraGameInfo [0].entropy.nCaptureVirusLimit = (char) atol (m [optCapVirLim].text);
extraGameInfo [0].entropy.nCaptureTimeLimit = (char) atol (m [optCapTimLim].text);
extraGameInfo [0].entropy.nMaxVirusCapacity = (ushort) atol (m [optMaxVirCap].text);
extraGameInfo [0].entropy.nBumpVirusCapacity = (char) atol (m [optBumpVirCap].text);
extraGameInfo [0].entropy.nBashVirusCapacity = (char) atol (m [optBashVirCap].text);
extraGameInfo [0].entropy.nVirusGenTime = (char) atol (m [optVirGenTim].text);
extraGameInfo [0].entropy.nVirusLifespan = (char) atol (m [optVirLife].text);
extraGameInfo [0].entropy.nEnergyFillRate = (ushort) atol (m [optEnergyFill].text);
extraGameInfo [0].entropy.nShieldFillRate = (ushort) atol (m [optShieldFill].text);
extraGameInfo [0].entropy.nShieldDamageRate = (ushort) atol (m [optShieldDmg].text);
}

//------------------------------------------------------------------------------

static int nBonusOpt, nSizeModOpt, nPyroForceOpt;

void MonsterballMenuCallback (int nitems, tMenuItem * menus, int * key, int cItem)
{
	tMenuItem	*m;
	int			v;

m = menus + nPyroForceOpt;
v = m->value + 1;
if (v != extraGameInfo [0].monsterball.forces [24].nForce) {
	extraGameInfo [0].monsterball.forces [24].nForce = v;
	sprintf (m->text, TXT_MBALL_PYROFORCE, v);
	m->rebuild = 1;
	//*key = -2;
	return;
	}
m = menus + nBonusOpt;
v = m->value + 1;
if (v != extraGameInfo [0].monsterball.nBonus) {
	extraGameInfo [0].monsterball.nBonus = v;
	sprintf (m->text, TXT_GOAL_BONUS, v);
	m->rebuild = 1;
	//*key = -2;
	return;
	}
m = menus + nSizeModOpt;
v = m->value + 2;
if (v != extraGameInfo [0].monsterball.nSizeMod) {
	extraGameInfo [0].monsterball.nSizeMod = v;
	sprintf (m->text, TXT_MBALL_SIZE, v / 2, (v & 1) ? 5 : 0);
	m->rebuild = 1;
	//*key = -2;
	return;
	}
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

static char *szWeaponTexts [] = {
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
	int	i, h = sizeofa (nOptionToForce);

for (i = 0; i < h - 1; i++)
	if ((dForce >= nOptionToForce [i]) && (dForce < nOptionToForce [i + 1]))
		break;
return i;
}

extern short nMonsterballForces [];
extern short nMonsterballPyroForce;

void NetworkMonsterballOptions (void)
{
	tMenuItem		m [35];
	int					h, i, j, opt = 0, optDefaultForces;
	char					szBonus [60], szSize [60], szPyroForce [60];
	tMonsterballForce	*pf = extraGameInfo [0].monsterball.forces;

h = sizeofa (optionToWeaponId);
j = sizeofa (nOptionToForce);
memset (m, 0, sizeof (m));
for (i = opt = 0; i < h; i++, opt++, pf++) {
	ADD_SLIDER (opt, szWeaponTexts [i], ForceToOption (pf->nForce), 
				   0, j - 1, 0, NULL);
	if (pf->nWeaponId == FLARE_ID) {
		opt++;
		ADD_TEXT (opt, "", 0);
		}
	}
ADD_TEXT (opt, "", 0);
opt++;
sprintf (szPyroForce + 1, TXT_MBALL_PYROFORCE, pf->nForce);
*szPyroForce = *(TXT_MBALL_PYROFORCE - 1);
ADD_SLIDER (opt, szPyroForce + 1, pf->nForce - 1, 0, 9, 0, NULL);
nPyroForceOpt = opt++;
ADD_TEXT (opt, "", 0);
opt++;
sprintf (szBonus + 1, TXT_GOAL_BONUS, extraGameInfo [0].monsterball.nBonus);
*szBonus = *(TXT_GOAL_BONUS - 1);
ADD_SLIDER (opt, szBonus + 1, extraGameInfo [0].monsterball.nBonus - 1, 0, 9, 0, HTX_GOAL_BONUS);
nBonusOpt = opt++;
i = extraGameInfo [0].monsterball.nSizeMod;
sprintf (szSize + 1, TXT_MBALL_SIZE, i / 2, (i & 1) ? 5 : 0);
*szSize = *(TXT_MBALL_SIZE - 1);
ADD_SLIDER (opt, szSize + 1, extraGameInfo [0].monsterball.nSizeMod - 2, 0, 8, 0, HTX_MBALL_SIZE);
nSizeModOpt = opt++;
ADD_TEXT (opt, "", 0);
opt++;
ADD_MENU (opt, "Set default values", 0, NULL);
optDefaultForces = opt++;
Assert (sizeofa (m) >= opt);

for (;;) {
	i = ExecMenu1 (NULL, "Monsterball Impact Forces", opt, m, MonsterballMenuCallback, 0);
	if (i == -1)
		break;
	if (i != optDefaultForces)
		break;
	InitMonsterballSettings (&extraGameInfo [0].monsterball);
	pf = extraGameInfo [0].monsterball.forces;
	for (i = 0; i < h + 1; i++, pf++) {
		m [i].value = ForceToOption (pf->nForce);
		if (pf->nWeaponId == FLARE_ID)
			i++;
		}
	m [nPyroForceOpt].value = NMCLAMP (pf->nForce - 1, 0, 9);
	m [nSizeModOpt].value = extraGameInfo [0].monsterball.nSizeMod - 2;
	}
pf = extraGameInfo [0].monsterball.forces;
for (i = 0; i < h; i++, pf++)
	pf->nForce = nOptionToForce [m [i].value];
pf->nForce = m [nPyroForceOpt].value + 1;
extraGameInfo [0].monsterball.nBonus = m [nBonusOpt].value + 1;
extraGameInfo [0].monsterball.nSizeMod = m [nSizeModOpt].value + 2;
}

//------------------------------------------------------------------------------

int NetworkGetGameType (tMenuItem *m, int bAnarchyOnly)
{
	int bHoard = HoardEquipped ();
		   
if (m [optAnarchy].value)
	mpParams.nGameMode = NETGAME_ANARCHY;
else if (m [optTeamAnarchy].value)
	mpParams.nGameMode = NETGAME_TEAM_ANARCHY;
else if (m [optCTF].value) {
	mpParams.nGameMode = NETGAME_CAPTURE_FLAG;
	extraGameInfo [0].bEnhancedCTF = 0;
	}
else if (m [optEnhancedCTF].value) {
	mpParams.nGameMode = NETGAME_CAPTURE_FLAG;
	extraGameInfo [0].bEnhancedCTF = 1;
	}
else if (bHoard && m [optHoard].value)
	mpParams.nGameMode = NETGAME_HOARD;
else if (bHoard && m [optTeamHoard].value)
	mpParams.nGameMode = NETGAME_TEAM_HOARD;
else if (bHoard && m [optEntropy].value)
	mpParams.nGameMode = NETGAME_ENTROPY;
else if (bHoard && m [optMonsterball].value)
	mpParams.nGameMode = NETGAME_MONSTERBALL;
else if (bAnarchyOnly) {
	ExecMessageBox (NULL, NULL, 1, TXT_OK, TXT_ANARCHY_ONLY_MISSION);
	m [optAnarchy].value = 1;
	m [optRobotAnarchy].value =
	m [optCoop].value = 0;
	return 0;
	}               
else if (m [optRobotAnarchy].value) 
	mpParams.nGameMode = NETGAME_ROBOT_ANARCHY;
else if (m [optCoop].value) 
	mpParams.nGameMode = NETGAME_COOPERATIVE;
return 1;
}

//------------------------------------------------------------------------------

int NetworkGetGameParams (int bAutoRun)
{
	int i, key, choice = 1;
	int opt, optGameName, optLevel, optLevelText, optMoreOpts, 
		 optMission, optMissionName, optD2XOpts, optConfigMenu;
	tMenuItem m [35];
	char name [NETGAME_NAME_LEN+1];
	char szLevelText [32];
	char szMaxNet [50];
	char szIpAddr [80];
	char szLevel [5];

	int nNewMission = gameData.missions.nLastMission;
	int bAnarchyOnly = 0;
	int bHoard = HoardEquipped ();

SetAllAllowablesTo (mpParams.nWeaponFilter);
networkData.nNamesInfoSecurity = -1;

if (nNewMission >= 0)
	bAnarchyOnly = gameData.missions.list [nNewMission].bAnarchyOnly;
for (i = 0; i < MAX_PLAYERS; i++)
	if (i!= gameData.multiplayer.nLocalPlayer)
		gameData.multiplayer.players [i].callsign [0] = 0;

gameData.multiplayer.nMaxPlayers = MAX_NUM_NET_PLAYERS;
//netGame.KillGoal = 0;
//netGame.xPlayTimeAllowed = 0;
//netGame.bAllowMarkerView = 1;
#if 0 // can be called via menu option now so you don't need to chose a level if you have one already
nNewMission = MultiChooseMission (&bAnarchyOnly);
if (nNewMission < 0)
	return -1;
#endif
if (!(FindArg ("-pps") && FindArg ("-shortpackets")))
	if (!NetworkChooseConnect ())
		return -1;

sprintf (name, "%s%s", LOCALPLAYER.callsign, TXT_S_GAME);
if (bAutoRun)
	return 1;

nGameItem = -1;

build_menu:

sprintf (szLevel, "%d", mpParams.nLevel);

memset (m, 0, sizeof (m));
opt = 0;
if (gameStates.multi.nGameType == UDP_GAME) {
	if (UDPGetMyAddress () < 0) 
		strcpy (szIpAddr, TXT_IP_FAIL);
	else {
		sprintf (szIpAddr, "Game Host: %d.%d.%d.%d:%d", 
					ipx_MyAddress [4], 
					ipx_MyAddress [5], 
					ipx_MyAddress [6], 
					ipx_MyAddress [7], 
					udpBasePort [1]);
		}
	}
ADD_TEXT (opt, TXT_DESCRIPTION, 0); 
opt++;
ADD_INPUT (opt, name, NETGAME_NAME_LEN, HTX_MULTI_NAME); 
optGameName = opt++;
ADD_MENU (opt, TXT_SEL_MISSION, KEY_I, HTX_MULTI_MISSION);
optMission = opt++;
ADD_TEXT (opt, "", 0);
m [opt].rebuild = 1; 
optMissionName = opt++;

//      if (gameData.missions.nLastSecretLevel < -1)
//              sprintf (szLevelText+strlen (szLevelText)-1, ", S1-S%d)", -gameData.missions.nLastSecretLevel);
//      else if (gameData.missions.nLastSecretLevel == -1)
//              sprintf (szLevelText+strlen (szLevelText)-1, ", S1)");

if ((nNewMission >= 0) && (gameData.missions.nLastLevel > 1)) {
	ADD_TEXT (opt, szLevelText, 0); 
	optLevelText = opt++;
	ADD_INPUT (opt, szLevel, 4, HTX_MULTI_LEVEL);
	optLevel = opt++;
	}
else
	optLevelText = -1;
//	m [opt].nType = NM_TYPE_TEXT; m [opt].text = TXT_OPTIONS; opt++;

ADD_TEXT (opt, "", 0); 
opt++; 
optGameTypes = opt;
ADD_RADIO (opt, TXT_ANARCHY, 0, KEY_A, 0, HTX_MULTI_ANARCHY);
optAnarchy = opt++;
ADD_RADIO (opt, TXT_TEAM_ANARCHY, 0, KEY_T, 0, HTX_MULTI_TEAMANA);
optTeamAnarchy = opt++;
ADD_RADIO (opt, TXT_ANARCHY_W_ROBOTS, 0, KEY_R, 0, HTX_MULTI_BOTANA);
optRobotAnarchy = opt++;
ADD_RADIO (opt, TXT_COOP, 0, KEY_P, 0, HTX_MULTI_COOP);
optCoop = opt++;
ADD_RADIO (opt, TXT_CTF, 0, KEY_F, 0, HTX_MULTI_CTF);
optCTF = opt++;
if (!gameStates.app.bNostalgia) {
	ADD_RADIO (opt, TXT_CTF_PLUS, 0, KEY_T, 0, HTX_MULTI_CTFPLUS);
	optEnhancedCTF = opt++;
	}

optEntropy =
optMonsterball = -1;
if (bHoard) {
	ADD_RADIO (opt, TXT_HOARD, 0, KEY_H, 0, HTX_MULTI_HOARD);
	optHoard = opt++;
	ADD_RADIO (opt, TXT_TEAM_HOARD, 0, KEY_H, 0, HTX_MULTI_TEAMHOARD);
	optTeamHoard = opt++;
	if (!gameStates.app.bNostalgia) {
		ADD_RADIO (opt, TXT_ENTROPY, 0, KEY_Y, 0, HTX_MULTI_ENTROPY);
		optEntropy = opt++;
		ADD_RADIO (opt, TXT_MONSTERBALL, 0, KEY_B, 0, HTX_MULTI_MONSTERBALL);
		optMonsterball = opt++;
		}
	} 
nGameTypes = opt - optGameTypes;
ADD_TEXT (opt, "", 0); 
opt++; 

m [optGameTypes + NMCLAMP (mpParams.nGameType, 0, opt - optGameTypes - 1)].value = 1;

ADD_RADIO (opt, TXT_OPEN_GAME, 0, KEY_O, 1, HTX_MULTI_OPENGAME);
optOpenGame = opt++;
ADD_RADIO (opt, TXT_CLOSED_GAME, 0, KEY_C, 1, HTX_MULTI_CLOSEDGAME);
optClosedGame = opt++;
ADD_RADIO (opt, TXT_RESTR_GAME, 0, KEY_R, 1, HTX_MULTI_RESTRGAME);
optRestrictedGame = opt++;

m [optOpenGame + NMCLAMP (mpParams.nGameAccess, 0, 2)].value = 1;

//      m [opt].nType = NM_TYPE_CHECK; m [opt].text = TXT_SHOW_IDS; m [opt].value = 0; opt++;

ADD_TEXT (opt, "", 0);
opt++; 
sprintf (szMaxNet + 1, TXT_MAX_PLAYERS, gameData.multiplayer.nMaxPlayers);
*szMaxNet = * (TXT_MAX_PLAYERS - 1);
lastMaxNet = gameData.multiplayer.nMaxPlayers - 2;
ADD_SLIDER (opt, szMaxNet + 1, lastMaxNet, 0, lastMaxNet, KEY_X, HTX_MULTI_MAXPLRS); 
optMaxNet = opt++;
ADD_TEXT (opt, "", 0);
opt++; 
ADD_MENU (opt, TXT_MORE_OPTS, KEY_M, HTX_MULTI_MOREOPTS);
optMoreOpts = opt++;
optConfigMenu =
optD2XOpts =
optEntOpts =
optMBallOpts = -1;
if (!gameStates.app.bNostalgia) {
	ADD_MENU (opt, TXT_MULTI_D2X_OPTS, KEY_X, HTX_MULTI_D2XOPTS);
	optD2XOpts = opt++;
	if (m [optEntropy].value) {
		ADD_MENU (opt, TXT_ENTROPY_OPTS, KEY_E, HTX_MULTI_ENTOPTS);
		optEntOpts = opt++;
		}
	if (m [optMonsterball].value) {
		ADD_MENU (opt, TXT_MONSTERBALL_OPTS, KEY_O, HTX_MULTI_MBALLOPTS);
		optMBallOpts = opt++;
		}
	ADD_MENU (opt, TXT_GAME_OPTIONS, KEY_O, HTX_MAIN_CONF);
	optConfigMenu = opt++;
	}

doMenu:

if (m [optMissionName].rebuild) {
	strncpy (netGame.szMissionName, 
				(nNewMission < 0) ? "" : gameData.missions.list [nNewMission].filename, 
				sizeof (netGame.szMissionName) - 1);
	m [optMissionName].text = 
			(nNewMission < 0) ? 
		TXT_NONE_SELECTED : 
		gameData.missions.list [nNewMission].szMissionName;
	if ((nNewMission >= 0) && (gameData.missions.nLastLevel > 1)) {
		sprintf (szLevelText, "%s (1-%d)", TXT_LEVEL_, gameData.missions.nLastLevel);
		Assert (strlen (szLevelText) < 32);
		m [optLevelText].rebuild = 1;
		}
	mpParams.nLevel = 1;
	}

gameStates.app.nExtGameStatus = GAMESTAT_NETGAME_OPTIONS; 
Assert (sizeofa (m) >= opt);
key = ExecMenu1 (NULL, (gameStates.multi.nGameType == UDP_GAME) ? szIpAddr : NULL, 
						opt, m, NetworkGameParamPoll, &choice);
								//TXT_NETGAME_SETUP
if (key == -1)
	return -1;
else if (choice == optMoreOpts) {
	if (m [optGameTypes + 3].value)
		gameData.app.nGameMode = GM_MULTI_COOP;
		NetworkMoreGameOptions ();
		gameData.app.nGameMode = 0;
		if (gameStates.multi.nGameType == UDP_GAME) {
			sprintf (szIpAddr, "Game Host: %d.%d.%d.%d:%d", 
			ipx_MyAddress [4], 
			ipx_MyAddress [5], 
			ipx_MyAddress [6], 
			ipx_MyAddress [7], 
			udpBasePort [1]);
			}
		goto doMenu;
	}
else if (!gameStates.app.bNostalgia && (optD2XOpts >= 0) && (choice == optD2XOpts)) {
	NetworkGetGameType (m, bAnarchyOnly);
	NetworkD2XOptions ();
	goto doMenu;
	}
else if (!gameStates.app.bNostalgia && (optEntOpts >= 0) && (choice == optEntOpts)) {
	NetworkEntropyOptions ();
	goto doMenu;
	}
else if (!gameStates.app.bNostalgia && (optMBallOpts >= 0) && (choice == optMBallOpts)) {
	NetworkMonsterballOptions ();
	goto doMenu;
	}
else if (!gameStates.app.bNostalgia && (optConfigMenu >= 0) && (choice == optConfigMenu)) {
	ConfigMenu ();
	goto doMenu;
	}
else if (choice == optMission) {
	int h = SelectAndLoadMission (1, &bAnarchyOnly);
	if (h < 0)
		goto doMenu;
	gameData.missions.nLastMission = nNewMission = h;
	m [optMissionName].rebuild = 1;
	goto build_menu;
	}

if (key != -1) {
	int j;
		   
	gameData.multiplayer.nMaxPlayers = m [optMaxNet].value + 2;
	netGame.nMaxPlayers = gameData.multiplayer.nMaxPlayers;
			
	for (j = 0; j < networkData.nActiveGames; j++)
		if (!stricmp (activeNetGames [j].szGameName, name)) {
			ExecMessageBox (TXT_ERROR, NULL, 1, TXT_OK, TXT_DUPLICATE_NAME);
			goto doMenu;
		}
	strncpy (mpParams.szGameName, name, sizeof (mpParams.szGameName));
	mpParams.nLevel = atoi (szLevel);
	if ((gameData.missions.nLastLevel > 0) && ((mpParams.nLevel < 1) || 
		 (mpParams.nLevel > gameData.missions.nLastLevel))) {
		ExecMessageBox (TXT_ERROR, NULL, 1, TXT_OK, TXT_LEVEL_OUT_RANGE);
		sprintf (szLevel, "1");
		goto doMenu;
	}

	for (i = optGameTypes; i < optOpenGame; i++)
		if (m [i].value) {
			mpParams.nGameType = i - optGameTypes;
			break;
			}

	for (i = optOpenGame; i < optMaxNet; i++)
		if (m [i].value) {
			mpParams.nGameAccess = i - optOpenGame;
			break;
			}

	if (!NetworkGetGameType (m, bAnarchyOnly))
		goto doMenu;
	if (m [optClosedGame].value)
		netGame.gameFlags |= NETGAME_FLAG_CLOSED;
	netGame.bRefusePlayers = m [optRestrictedGame].value;
	}
NetworkSetGameMode (mpParams.nGameMode);
if (key == -2)
	goto build_menu;

if (nNewMission < 0) {
	ExecMessageBox (TXT_ERROR, NULL, 1, TXT_OK, "Please chose a mission");
	goto doMenu;
	}
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
	extraGameInfo [0].bSafeUDP = (m [optSafeUDP].value != 0);
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
netGame.control_invulTime = mpParams.nReactorLife * 5 * F1_0 * 60;
IpxChangeDefaultSocket ((ushort) (IPX_DEFAULT_SOCKET + networkData.nSocket));
return key;
}

//------------------------------------------------------------------------------

static time_t	nQueryTimeout;

static void QueryPoll (int nItems, tMenuItem *m, int *key, int cItem)
{
	time_t t;

if (NetworkListen () && (networkData.nActiveGames >= MAX_ACTIVE_NETGAMES))
	*key = -2;
else if (*key == KEY_ESC)
	*key = -3;
else if ((t = SDL_GetTicks () - nQueryTimeout) > 5000)
	*key = -4;
else {
	int v = (int) (t / 5);
	if (m [0].value != v) {
		m [0].value = v;
		m [0].rebuild = 1;
		}
	*key = 0;
	}
return;
}

//------------------------------------------------------------------------------

int NetworkFindGame (void)
{
	tMenuItem	m [3];
	int i;

if (gameStates.multi.nGameType > IPX_GAME)
	return 0;

networkData.nStatus = NETSTAT_BROWSING;
networkData.nActiveGames = 0;
NetworkSendGameListRequest ();

memset (m, 0, sizeof (m));
ADD_GAUGE (0, "                    ", 0, 1000); 
ADD_TEXT (1, "", 0);
ADD_TEXT (2, TXT_PRESS_ESC, 0);
m [2].centered = 1;
nQueryTimeout = SDL_GetTicks ();
do {
	i = ExecMenu2 (NULL, TXT_NET_SEARCH, 3, m, QueryPoll, 0, NULL);
	} while (i >= 0);
return (networkData.nActiveGames >= MAX_ACTIVE_NETGAMES);
}

//------------------------------------------------------------------------------

int NetworkSelectTeams (void)
{
	tMenuItem m [MAX_PLAYERS+4];
	int choice, opt, opt_team_b;
	ubyte teamVector = 0;
	char team_names [2] [CALLSIGN_LEN+1];
	int i;
	int pnums [MAX_PLAYERS+2];

	// One-time initialization

	for (i = gameData.multiplayer.nPlayers/2; i < gameData.multiplayer.nPlayers; i++) // Put first half of players on team A
	{
		teamVector |= (1 << i);
	}

	sprintf (team_names [0], "%s", TXT_BLUE);
	sprintf (team_names [1], "%s", TXT_RED);

	// Here comes da menu
doMenu:

	memset (m, 0, sizeof (m));

	m [0].nType = NM_TYPE_INPUT; 
	m [0].text = team_names [0]; 
	m [0].text_len = CALLSIGN_LEN; 

	opt = 1;
	for (i = 0; i < gameData.multiplayer.nPlayers; i++)
	{
		if (!(teamVector & (1 << i)))
		{
			m [opt].nType = NM_TYPE_MENU; 
			m [opt].text = netPlayers.players [i].callsign; 
			pnums [opt] = i; 
			opt++;
		}
	}
	opt_team_b = opt;
	m [opt].nType = NM_TYPE_INPUT; 
	m [opt].text = team_names [1]; 
	m [opt].text_len = CALLSIGN_LEN; 
	opt++;
	for (i = 0; i < gameData.multiplayer.nPlayers; i++)
	{
		if (teamVector & (1 << i))
		{
			m [opt].nType = NM_TYPE_MENU; 
			m [opt].text = netPlayers.players [i].callsign; 
			pnums [opt] = i; 
			opt++;
		}
	}
	m [opt].nType = NM_TYPE_TEXT; 
	m [opt].text = ""; 
	opt++;
	m [opt].nType = NM_TYPE_MENU; 
	m [opt].text = TXT_ACCEPT; 
	m [opt].key = KEY_A;
	opt++;

	Assert (opt <= MAX_PLAYERS+4);

	choice = ExecMenu (NULL, TXT_TEAM_SELECTION, opt, m, NULL, NULL);

	if (choice == opt-1)
	{
		if ((opt-2-opt_team_b < 2) || (opt_team_b == 1)) 
		{
			ExecMessageBox (NULL, NULL, 1, TXT_OK, TXT_TEAM_MUST_ONE);
		}
	
		netGame.teamVector = teamVector;
		strcpy (netGame.team_name [0], team_names [0]);
		strcpy (netGame.team_name [1], team_names [1]);
		return 1;
	}

	else if ((choice > 0) && (choice < opt_team_b)) {
		teamVector |= (1 << pnums [choice]);
	}
	else if ((choice > opt_team_b) && (choice < opt-2)) {
		teamVector &= ~ (1 << pnums [choice]);
	}
	else if (choice == -1)
		return 0;
	goto doMenu;
}

//------------------------------------------------------------------------------

int NetworkSelectPlayers (int bAutoRun)
{
	int i, j, choice = 1;
   tMenuItem m [MAX_PLAYERS+4];
   char text [MAX_PLAYERS+4] [45];
	char title [50];
	int nSavePlayers;              //how may people would like to join

NetworkAddPlayer (&networkData.mySeq);
if (bAutoRun)
	return 1;

memset (m, 0, sizeof (m));
for (i = 0; i< MAX_PLAYERS+4; i++) {
	sprintf (text [i], "%d.  %-20s", i+1, "");
	m [i].nType = NM_TYPE_CHECK; 
	m [i].text = text [i]; 
	m [i].value = 0;
	}
m [0].value = 1;                         // Assume server will play...
if (gameOpts->multi.bNoRankings)
	sprintf (text [0], "%d. %-20s", 1, LOCALPLAYER.callsign);
else
	sprintf (text [0], "%d. %s%-20s", 1, pszRankStrings [netPlayers.players [gameData.multiplayer.nLocalPlayer].rank], LOCALPLAYER.callsign);
sprintf (title, "%s %d %s", TXT_TEAM_SELECT, gameData.multiplayer.nMaxPlayers, TXT_TEAM_PRESS_ENTER);

GetPlayersAgain:

gameStates.app.nExtGameStatus = GAMESTAT_NETGAME_PLAYER_SELECT;
Assert (sizeofa (m) >= MAX_PLAYERS + 4);
j = ExecMenu1 (NULL, title, MAX_PLAYERS + 4, m, NetworkStartPoll, &choice);
nSavePlayers = gameData.multiplayer.nPlayers;
if (j < 0) {
	// Aborted!                                     
	// Dump all players and go back to menu mode

abort:
	for (i = 1; i < nSavePlayers; i++) {
		if (gameStates.multi.nGameType >= IPX_GAME)
			NetworkDumpPlayer (
				netPlayers.players [i].network.ipx.server, 
				netPlayers.players [i].network.ipx.node, 
				DUMP_ABORTED);
		}

	netGame.nNumPlayers = 0;
	NetworkSendGameInfo (0); // Tell everyone we're bailing
	ipx_handle_leave_game (); // Tell the network driver we're bailing too
	networkData.nStatus = NETSTAT_MENU;
	return 0;
	}
// Count number of players chosen
gameData.multiplayer.nPlayers = 0;
for (i = 0; i < nSavePlayers; i++) {
	if (m [i].value) 
		gameData.multiplayer.nPlayers++;
	}
if (gameData.multiplayer.nPlayers > netGame.nMaxPlayers) {
	ExecMessageBox (TXT_ERROR, NULL, 1, TXT_OK, "%s %d %s", TXT_SORRY_ONLY, gameData.multiplayer.nMaxPlayers, TXT_NETPLAYERS_IN);
	gameData.multiplayer.nPlayers = nSavePlayers;
	goto GetPlayersAgain;
	}
#ifndef _DEBUG
if (gameData.multiplayer.nPlayers < 2) {
	ExecMessageBox (TXT_WARNING, NULL, 1, TXT_OK, TXT_TEAM_ATLEAST_TWO);
#	if 0
	gameData.multiplayer.nPlayers = nSavePlayers;
	goto GetPlayersAgain;
#	endif
	}
#endif

#ifndef _DEBUG
if ((netGame.gameMode == NETGAME_TEAM_ANARCHY ||
	  netGame.gameMode == NETGAME_CAPTURE_FLAG || 
	  netGame.gameMode == NETGAME_TEAM_HOARD) && 
	 (gameData.multiplayer.nPlayers < 2)) {
	ExecMessageBox (TXT_ERROR, NULL, 1, TXT_OK, TXT_NEED_2PLAYERS);
	gameData.multiplayer.nPlayers = nSavePlayers;
#if 0	
	goto GetPlayersAgain;
#endif	
	}
#endif

// Remove players that aren't marked.
gameData.multiplayer.nPlayers = 0;
for (i = 0; i < nSavePlayers; i++) {
	if (m [i].value) {
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
			netPlayers.players [gameData.multiplayer.nPlayers].version_major = netPlayers.players [i].version_major;
			netPlayers.players [gameData.multiplayer.nPlayers].version_minor = netPlayers.players [i].version_minor;
			netPlayers.players [gameData.multiplayer.nPlayers].rank = netPlayers.players [i].rank;
			ClipRank ((char *) &netPlayers.players [gameData.multiplayer.nPlayers].rank);
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
	netPlayers.players [i].version_major = 0;
	netPlayers.players [i].version_minor = 0;
	netPlayers.players [i].rank = 0;
	}
#if 1			
con_printf (CONDBG, "Select teams: Game mode is %d\n", netGame.gameMode);
#endif
if (netGame.gameMode == NETGAME_TEAM_ANARCHY ||
	 netGame.gameMode == NETGAME_CAPTURE_FLAG ||
	 netGame.gameMode == NETGAME_TEAM_HOARD ||
	 netGame.gameMode == NETGAME_ENTROPY ||
	 netGame.gameMode == NETGAME_MONSTERBALL)
	if (!NetworkSelectTeams ())
		goto abort;
return 1; 
}

//------------------------------------------------------------------------------

void InitNetgameMenuOption (tMenuItem *m, int j)
{
m += j;
if (!m->text) {
	m->text = szNMTextBuffer [j];
	m->nType = NM_TYPE_MENU;
	}
sprintf (m->text, "%2d.                                                     ", 
			j - 1 - gameStates.multi.bUseTracker);
m->value = 0;
m->rebuild = 1;
}

//------------------------------------------------------------------------------

void InitNetgameMenu (tMenuItem *m, int i)
{
	int j;

for (j = i + 2 + gameStates.multi.bUseTracker; i < MAX_ACTIVE_NETGAMES; i++, j++)
	InitNetgameMenuOption (m, j);
}

//------------------------------------------------------------------------------

extern int nTabs [];

char *PruneText (char *pszDest, char *pszSrc, int nSize, int nPos, int nVersion)
{
	int		lDots, lMax, l, tx, ty, ta;
	char		*psz;
	grsFont	*curFont = grdCurCanv->cvFont;

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
grdCurCanv->cvFont = SMALL_FONT;
GrGetStringSize ("... ", &lDots, &ty, &ta);
GrGetStringSize (pszDest, &tx, &ty, &ta);
l = (int) strlen (pszDest);
lMax = LHX (nTabs [nPos]) - LHX (nTabs [nPos - 1]);
if (tx > lMax) {
	lMax -= lDots;
	do {
		pszDest [--l] = '\0';
		GrGetStringSize (pszDest, &tx, &ty, &ta);
	} while (tx > lMax);
	strcat (pszDest, "...");
	}
grdCurCanv->cvFont = curFont; 
return pszDest;
}

//------------------------------------------------------------------------------

char *szModeLetters []  = 
	{"ANRCHY", 
	 "TEAM", 
	 "ROBO", 
	 "COOP", 
	 "FLAG", 
	 "HOARD", 
	 "TMHOARD", 
	 "ENTROPY",
	 "MONSTER"};

void NetworkJoinPoll (int nitems, tMenuItem * menus, int * key, int cItem)
{
	// Polling loop for Join Game menu
	static fix t1 = 0;
	int	t = SDL_GetTicks ();
	int	i, h = 2 + gameStates.multi.bUseTracker, osocket, nJoinStatus, bPlaySound = 0;
	char	*psz;
	char	szOption [200];
	char	szTrackers [100];

if (gameStates.multi.bUseTracker) {
	i = ActiveTrackerCount (0);
	menus [1].color = ORANGE_RGBA;
	sprintf (szTrackers, TXT_TRACKERS_FOUND, i, (i == 1) ? "" : "s");
	if (strcmp (menus [1].text, szTrackers)) {
		strcpy (menus [1].text, szTrackers);
//			menus [1].x = (short) 0x8000;
		menus [1].rebuild = 1;
		}
	}

if ((gameStates.multi.nGameType >= IPX_GAME) && networkData.bAllowSocketChanges) {
	osocket = networkData.nSocket;
	if (*key == KEY_PAGEDOWN) { 
		networkData.nSocket--; 
		*key = 0; 
		}
	else if (*key == KEY_PAGEUP) { 
		networkData.nSocket++; 
		*key = 0; 
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
		sprintf (menus [0].text, TXT_CURR_SOCK, 
					 (gameStates.multi.nGameType == IPX_GAME) ? "IPX" : "UDP", 
					 networkData.nSocket);
		menus [0].rebuild = 1;
#if 1			
		con_printf (0, TXT_CHANGE_SOCK, networkData.nSocket);
#endif
		NetworkListen ();
		IpxChangeDefaultSocket ((ushort) (IPX_DEFAULT_SOCKET + networkData.nSocket));
		RestartNetSearching (menus);
		NetworkSendGameListRequest ();
		return;
		}
	}
	// send a request for game info every 3 seconds
#ifdef _DEBUG
if (!networkData.nActiveGames)
#endif
	if (gameStates.multi.nGameType >= IPX_GAME) {
		if (t > t1 + 3000) {
			if (!NetworkSendGameListRequest ())
				return;
			t1 = t;
			}
		}
NetworkListen ();
DeleteTimedOutNetGames ();
if (networkData.bGamesChanged || (networkData.nActiveGames != networkData.nLastActiveGames)) {
	networkData.bGamesChanged = 0;
	networkData.nLastActiveGames = networkData.nActiveGames;
#if 1			
	con_printf (CONDBG, "Found %d netgames.\n", networkData.nActiveGames);
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
		if (strcmp (szOption, menus [h].text)) {
			memcpy (menus [h].text, szOption, 100);
			menus [h].rebuild = 1;
			menus [h].unavailable = (nLevelVersion == 0);
			bPlaySound = 1;
			}
		}
	for (i = 3 + networkData.nActiveGames; i < MAX_ACTIVE_NETGAMES; i++, h++)
		InitNetgameMenuOption (menus, h);
	}
#if 0
else
	for (i = 3; i < networkData.nActiveGames; i++, h++)
		menus [h].value = SDL_GetTicks ();
for (i = 3 + networkData.nActiveGames; i < MAX_ACTIVE_NETGAMES; i++, h++)
	if (menus [h].value && (t - menus [h].value > 10000)) 
		{
		InitNetgameMenuOption (menus, h);
		bPlaySound = 1;
		}
#endif
if (bPlaySound)
	DigiPlaySample (SOUND_HUD_MESSAGE, F1_0);
}


//------------------------------------------------------------------------------

int NetworkBrowseGames (void)
{
	tMenuItem	m [MAX_ACTIVE_NETGAMES + 5];
	int			choice, i, bAutoRun = gameData.multiplayer.autoNG.bValid;
	bkg			bg;
	char			callsign [CALLSIGN_LEN+1];

//LogErr ("launching netgame browser\n");
memcpy (callsign, LOCALPLAYER.callsign, sizeof (callsign));
memset (&bg, 0, sizeof (bg));
bg.bIgnoreBg = 1;
if (gameStates.multi.nGameType >= IPX_GAME) {
	if (!networkData.bActive) {
		ExecMessageBox (NULL, NULL, 1, TXT_OK, TXT_IPX_NOT_FOUND);
		return 0;
		}
	}
//LogErr ("   NetworkInit\n");
NetworkInit ();
gameData.multiplayer.nPlayers = 0;
setjmp (gameExitPoint);
networkData.nSyncState = 0; 
networkData.nJoinState = 0;
networkData.nStatus = NETSTAT_BROWSING; // We are looking at a game menu
IpxChangeDefaultSocket ((ushort) (IPX_DEFAULT_SOCKET + networkData.nSocket));
//LogErr ("   NetworkFlush\n");
NetworkFlush ();
//LogErr ("   NetworkListen\n");
NetworkListen ();  // Throw out old info
//LogErr ("   NetworkSendGameListRequest\n");
NetworkSendGameListRequest (); // broadcast a request for lists
networkData.nActiveGames = 0;
networkData.nLastActiveGames = 0;
memset (activeNetGames, 0, sizeof (activeNetGames));
memset (activeNetPlayers, 0, sizeof (activeNetPlayers));
if (!bAutoRun) {
	GrSetFontColorRGBi (RGBA_PAL (15, 15, 23), 1, 0, 0);
	memset (m, 0, sizeof (m));
	m [0].text = szNMTextBuffer [0];
	m [0].nType = NM_TYPE_TEXT;
	m [0].noscroll = 1;
	m [0].x = (short) 0x8000;
	//m [0].x = (short) 0x8000;
	if (gameStates.multi.nGameType >= IPX_GAME) {
		if (networkData.bAllowSocketChanges)
			sprintf (m [0].text, TXT_CURR_SOCK, 
										 (gameStates.multi.nGameType == IPX_GAME) ? "IPX" : "UDP", networkData.nSocket);
		else
			*m [0].text = '\0';
		}
	i = 1;
	if (gameStates.multi.bUseTracker) {
		m [i].nType = NM_TYPE_TEXT;
		m [i].text = szNMTextBuffer [i];
		strcpy (m [i].text, TXT_0TRACKERS);
		m [i].x = (short) 0x8000;
		m [i].noscroll = 1;
		i++;
		}
	m [i].text = szNMTextBuffer [i];
	m [i].nType = NM_TYPE_TEXT;
	strcpy (m [i].text, TXT_GAME_BROWSER);
	m [i].noscroll = 1;
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
	NMLoadBackground (MENU_PCX_NAME, &bg, 0);             //load this here so if we abort after loading level, we restore the palette
	GrPaletteStepLoad (NULL);
	choice = ExecMenuTiny (TXT_NETGAMES, NULL, MAX_ACTIVE_NETGAMES + 2 + gameStates.multi.bUseTracker, m, NetworkJoinPoll);
	NMRemoveBackground (&bg);
	gameStates.multi.bSurfingNet = 0;
	}
if (choice == -1) {
	ChangePlayerNumTo (0);
	memcpy (LOCALPLAYER.callsign, callsign, sizeof (callsign));
	networkData.nStatus = NETSTAT_MENU;
	return 0; // they cancelled               
	}               
choice -= (2 + gameStates.multi.bUseTracker);
if (choice >= networkData.nActiveGames) {
	ExecMessageBox (TXT_SORRY, NULL, 1, TXT_OK, TXT_INVALID_CHOICE);
	goto doMenu;
	}

// Choice has been made and looks legit
if (AGI.gameStatus == NETSTAT_ENDLEVEL) {
	ExecMessageBox (TXT_SORRY, NULL, 1, TXT_OK, TXT_NET_GAME_BETWEEN2);
	goto doMenu;
	}
if (AGI.protocol_version != MULTI_PROTO_VERSION) {
	if (AGI.protocol_version == 3) {
		ExecMessageBox (TXT_SORRY, NULL, 1, TXT_OK, TXT_INCOMPAT1);
		}
	else if (AGI.protocol_version == 4) {
		}
	else {
		char	szFmt [200], szError [200];

		sprintf (szFmt, "%s%s", TXT_VERSION_MISMATCH, TXT_NETGAME_VERSIONS);
		sprintf (szError, szFmt, MULTI_PROTO_VERSION, AGI.protocol_version);
		ExecMessageBox (TXT_SORRY, NULL, 1, TXT_OK, szError);
		}
	goto doMenu;
	}

if (gameStates.multi.bUseTracker) {
	//LogErr ("   getting server lists from trackers\n");
	GetServerFromList (choice);
	}
// Check for valid mission name
con_printf (CONDBG, TXT_LOADING_MSN, AGI.szMissionName);
if (!(LoadMissionByName (AGI.szMissionName, -1) ||
		(DownloadMission (AGI.szMissionName) &&
		 LoadMissionByName (AGI.szMissionName, -1)))) {
	LogErr ("Mission '%s' not found%s\n", AGI.szMissionName);
	ExecMessageBox (NULL, NULL, 1, TXT_OK, TXT_MISSION_NOT_FOUND);
	goto doMenu;
	}
if (IS_D2_OEM && (AGI.nLevel > 8)) {
	ExecMessageBox (NULL, NULL, 1, TXT_OK, TXT_OEM_ONLY8);
	goto doMenu;
	}
if (IS_MAC_SHARE && (AGI.nLevel > 4)) {
	ExecMessageBox (NULL, NULL, 1, TXT_OK, TXT_SHARE_ONLY4);
	goto doMenu;
	}
if (!NetworkWaitForAllInfo (choice)) {
	ExecMessageBox (TXT_SORRY, NULL, 1, TXT_OK, TXT_JOIN_ERROR);
	networkData.nStatus = NETSTAT_BROWSING; // We are looking at a game menu
	goto doMenu;
	}       

networkData.nStatus = NETSTAT_BROWSING; // We are looking at a game menu
  if (!CanJoinNetgame (activeNetGames + choice, activeNetPlayers + choice)) {
	if (AGI.nNumPlayers == AGI.nMaxPlayers)
		ExecMessageBox (TXT_SORRY, NULL, 1, TXT_OK, TXT_GAME_FULL);
	else
		ExecMessageBox (TXT_SORRY, NULL, 1, TXT_OK, TXT_IN_PROGRESS);
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
//LogErr ("loading level\n");
StartNewLevel (netGame.nLevel, 0);
//LogErr ("exiting netgame browser\n");
NMRemoveBackground (&bg);
return 1;         // look ma, we're in a game!!!
}

//------------------------------------------------------------------------------

int ConnectionPacketLevel [] = {0, 1, 1, 1};
int ConnectionSecLevel [] = {12, 3, 5, 7};

int AppletalkConnectionPacketLevel [] = {0, 1, 0};
int AppletalkConnectionSecLevel [] = {10, 3, 8};

#ifndef _DEBUG
int NetworkChooseConnect ()
{
return 1;
}
#else
int NetworkChooseConnect ()
{
#if 0
tMenuItem m [16];
int choice, opt = 0;
#endif
if (gameStates.multi.nGameType >= IPX_GAME) {  
#if 0
	memset (m, 0, sizeof (m));
	m [opt].nType = NM_TYPE_MENU;  
	m [opt].text = "Local Subnet"; 
	opt++;
	m [opt].nType = NM_TYPE_MENU;  
	m [opt].text = "14.4 modem over Internet"; 
	opt++;
	m [opt].nType = NM_TYPE_MENU;  
	m [opt].text = "28.8 modem over Internet"; 
	opt++;
	m [opt].nType = NM_TYPE_MENU;  
	m [opt].text = "ISDN or T1 over Internet"; 
	opt++;

	Assert (sizeofa (m) >= opt);
	choice = ExecMenu1 (NULL, "Choose connection nType", opt, m, NULL, 0);

	if (choice<0)
		return (NULL);

	Assert (choice> = 0 && choice< = 3);

	netGame.bShortPackets = ConnectionPacketLevel [choice];
	netGame.nPacketsPerSec = ConnectionSecLevel [choice];
#endif
   return 1;
	}
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

int stoip (char *szServerIpAddr, unsigned char *pIpAddr)
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
		return stoport (pFields [j], udpBasePort, NULL); 
	else {
		h = atol (pFields [j]);
		if ((h < 0) || (h > 255))
			return 0;
		pIpAddr [j] = (unsigned char) h;
		}
	}
return 1;
}

//------------------------------------------------------------------------------

void IpAddrMenuCallBack (int nitems, tMenuItem * menus, int * key, int cItem)
{
}

//------------------------------------------------------------------------------

int NetworkGetIpAddr (void)
{
	tMenuItem m [9];
	int i, choice = 0;
	int opt = 0, optServer = -1, optPort = -1;
	int commands;

#ifdef _DEBUG
	static char szLocalIpAddr [16] = {'0', '.', '0', '.', '0', '.', '0', '\0'};
#else
#endif
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
	//	strcpy (szLocalIpAddr, szServerIpAddr);
		}
	}
memset (m, 0, sizeof (m));
if (!gameStates.multi.bUseTracker) {
	ADD_TEXT (opt, TXT_HOST_IP, 0);
	//IP [\":\"<port> | {{\"+\" | \"-\"} <offset>]\n\n (e.g. 127.0.0.1 for an arbitrary port, \nor 127.0.0.1:28342 for a fixed port, \nor 127.0.0.1:+1 for host port + offset)\n";       
	//m [opt].text = "Enter the game host' IP address.\nYou can specify a port to use\nby entering a colon followed by the port number.\nYou can use an 
	opt++;
	ADD_INPUT (opt, mpParams.szServerIpAddr, sizeof (mpParams.szServerIpAddr) - 1, HTX_GETIP_SERVER);
	optServer = opt++;
	/*
	m [opt].nType = NM_TYPE_TEXT;  
	m [opt].text = "\nServer IP address:";
	opt++;
	m [opt].nType = NM_TYPE_INPUT; 
	m [opt].text = szLocalIpAddr; 
	m [opt].text_len = sizeof (szLocalIpAddr)-1;         
	opt++;
	*/
	ADD_TEXT (opt, TXT_CLIENT_PORT, 0);
	opt++;
	}
if ((mpParams.udpClientPort < 0) || (mpParams.udpClientPort > 65535))
	mpParams.udpClientPort = 0;
sprintf (szClientPort, "%u", mpParams.udpClientPort);
ADD_INPUT (opt, szClientPort, sizeof (szClientPort) - 1, HTX_GETIP_CLIENT);
optPort = opt++;
ADD_TEXT (opt, TXT_PORT_HELP1, 0);
ADD_TEXT (opt, TXT_PORT_HELP2, 0);
opt++;
commands = opt;
Assert (sizeofa (m) >= opt);
for (;;) {
	i = ExecMenu1 (NULL, gameStates.multi.bUseTracker ? TXT_CLIENT_PORT + 1 : TXT_IP_HEADER, 
						opt, m, &IpAddrMenuCallBack, &choice);
	if (i < 0)
		break;
	if (i < commands) {
		if ((i == optServer) || (i == optPort)) {
			if (gameStates.multi.bUseTracker || stoip (mpParams.szServerIpAddr, ipx_ServerAddress + 4)) {
				stoport (szClientPort, &mpParams.udpClientPort, &nClientPortSign);
				if (gameStates.multi.bCheckPorts && !mpParams.udpClientPort)
					ExecMessageBox (NULL, NULL, 1, TXT_OK, TXT_IP_INVALID);
				else
					return 1;
				}
			ExecMessageBox (NULL, NULL, 1, TXT_OK, TXT_IP_INVALID);
			}
		}
	}
return 0;
} 

//------------------------------------------------------------------------------

extern tNetgameInfo activeNetGames [];
extern tExtraGameInfo activeExtraGameInfo [];

#define FLAGTEXT(_b)	((_b) ? TXT_ON : TXT_OFF)

#define	INITFLAGS(_t) \
			{sprintf (mTexts [opt], _t); j = 0;}

#define	ADDFLAG(_f,_t) \
		if (_f) {if (j) strcat (mTexts [opt], ", "); strcat (mTexts [opt], _t); j++; }

void ShowNetGameInfo (int choice)
 {
	tMenuItem	m [20];
   char			mTexts [20][200];
	int			i, j, nInMenu, opt = 0;

#ifndef _DEBUG
if (choice >= networkData.nActiveGames)
	return;
#endif
memset (m, 0, sizeof (m));
memset (mTexts, 0, sizeof (mTexts));
for (i = 0; i < 20; i++) {
	m [i].text = (char *) (mTexts + i);
	m [i].nType = NM_TYPE_TEXT;	
	}
sprintf (mTexts [opt], TXT_NGI_GAME, AGI.szGameName); 
opt++;
sprintf (mTexts [opt], TXT_NGI_MISSION, AGI.szMissionTitle); 
opt++;
sprintf (mTexts [opt], TXT_NGI_LEVEL, AGI.nLevel); 
opt++;
sprintf (mTexts [opt], TXT_NGI_SKILL, MENU_DIFFICULTY_TEXT (AGI.difficulty)); 
opt++;
opt++;
#ifndef _DEBUG
if (!*AXI.szGameName) {
	sprintf (mTexts [opt], "Gamehost is not using D2X-XL or running in pure mode");
	opt++;
	}
else 
#endif
	{
	if (AXI.bShadows || AXI.bUseSmoke || AXI.bBrightObjects || (!AXI.bCompetition && AXI.bUseLightnings)) {
		INITFLAGS ("Graphics Fx: "); 
		ADDFLAG (AXI.bShadows, "Shadows");
		ADDFLAG (AXI.bUseSmoke, "Smoke");
		if (!AXI.bCompetition)
			ADDFLAG (AXI.bUseLightnings, "Lightnings");
		ADDFLAG (AXI.bBrightObjects, "Bright Objects");
		}
	else
		strcpy (mTexts [opt], "Graphics Fx: None");
	opt++;
	if (!AXI.bCompetition && (AXI.bLightTrails || AXI.bShockwaves || AXI.bTracers)) {
		INITFLAGS ("Weapon Fx: ");
		ADDFLAG (AXI.bLightTrails, "Light trails");
		ADDFLAG (AXI.bShockwaves, "Shockwaves");
		ADDFLAG (AXI.bTracers, "Tracers");
		ADDFLAG (AXI.bShowWeapons, "Weapons");
		}
	else
		sprintf (mTexts [opt], "Weapon Fx: None");
	opt++;
	if (!AXI.bCompetition && (AXI.bDamageExplosions || AXI.bPlayerShield)) {
		INITFLAGS ("Ship Fx: ");
		ADDFLAG (AXI.bPlayerShield, "Shield");
		ADDFLAG (AXI.bDamageExplosions, "Damage");
		ADDFLAG (AXI.bGatlingSpeedUp, "Gatling speedup");
		}
	else
		sprintf (mTexts [opt], "Ship Fx: None");
	opt++;
	if (AXI.nWeaponIcons || (!AXI.bCompetition && (AXI.bTargetIndicators || AXI.bDamageIndicators))) {
		INITFLAGS ("HUD extensions: ");
		ADDFLAG (AXI.nWeaponIcons != 0, "Icons");
		ADDFLAG (!AXI.bCompetition && AXI.bTargetIndicators, "Tgt indicators");
		ADDFLAG (!AXI.bCompetition && AXI.bDamageIndicators, "Dmg indicators");
		ADDFLAG (!AXI.bCompetition && AXI.bMslLockIndicators, "Trk indicators");
		}
	else
		strcat (mTexts [opt], "HUD extensions: None");
	opt++;
	if (!AXI.bCompetition && AXI.bRadarEnabled) {
		INITFLAGS ("Radar: ");
		ADDFLAG ((AGI.gameFlags & NETGAME_FLAG_SHOW_MAP) != 0, "Players");
		ADDFLAG (AXI.nRadar, "Radar");
		ADDFLAG (AXI.bPowerupsOnRadar, "Powerups");
		ADDFLAG (AXI.bRobotsOnRadar, "Robots");
		}
	else
		strcat (mTexts [opt], "Radar: off");
	opt++;
	if (!AXI.bCompetition && (AXI.bMouseLook || AXI.bFastPitch)) {
		INITFLAGS ("Controls ext.: ");
		ADDFLAG (AXI.bMouseLook, "mouselook");
		ADDFLAG (AXI.bFastPitch, "fast pitch");
		}
	else
		strcat (mTexts [opt], "Controls ext.: None");
	opt++;
	if (!AXI.bCompetition && 
		 (AXI.bDualMissileLaunch || !AXI.bFriendlyFire || AXI.bInhibitSuicide || 
		  AXI.bEnableCheats || AXI.bDarkness || (AXI.nFusionRamp != 2))) {
		INITFLAGS ("Gameplay ext.: ");
		ADDFLAG (AXI.bEnableCheats, "Cheats");
		ADDFLAG (AXI.bDarkness, "Darkness");
		ADDFLAG (AXI.bSmokeGrenades, "Smoke Grens");
		ADDFLAG (AXI.bDualMissileLaunch, "Dual Msls");
		ADDFLAG (AXI.nFusionRamp != 2, "Fusion ramp");
		ADDFLAG (!AXI.bFriendlyFire, "no FF");
		ADDFLAG (AXI.bInhibitSuicide, "no suicide");
		ADDFLAG (AXI.bKillMissiles, "shoot msls");
		ADDFLAG (AXI.bTripleFusion, "tri fusion");
		ADDFLAG (AXI.bEnhancedShakers, "enh shakers");
		ADDFLAG (AXI.nHitboxes, "hit boxes");
		}
	else
		strcat (mTexts [opt], "Gameplay ext.: None");
	opt++;
	}
bAlreadyShowingInfo = 1;
nInMenu = gameStates.menus.nInMenu;
gameStates.menus.nInMenu = 0;
ExecMenutiny2 (NULL, TXT_NETGAME_INFO, opt, m, NULL);
gameStates.menus.nInMenu = nInMenu;
bAlreadyShowingInfo = 0;
 }

//------------------------------------------------------------------------------

