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

// The code in this file sends a game host's game data to a client connecting to the host

#ifdef HAVE_CONFIG_H
#	include <conf.h>
#endif

#ifndef _WIN32
#	include <arpa/inet.h>
#	include <netinet/in.h> /* for htons & co. */
#endif

#include "descent.h"
#include "timer.h"
#include "byteswap.h"
#include "strutil.h"
#include "ipx.h"
#include "error.h"
#include "hogfile.h"
#include "network.h"
#include "network_lib.h"
#include "netmisc.h"
#include "autodl.h"
#include "tracker.h"
#include "monsterball.h"
#include "text.h"
#include "console.h"

#if DBG
#	define OBJ_PACKETS_PER_FRAME 1
#else
#	define OBJ_PACKETS_PER_FRAME 1
#endif

//------------------------------------------------------------------------------

void NetworkStopResync (tSequencePacket *their)
{
for (short i = 0; i < networkData.nJoining; )
	if (!CmpNetPlayers (networkData.sync [i].player [1].player.callsign, their->player.callsign, 
							  &networkData.sync [i].player [1].player.network, &their->player.network)) {
#if 1      
		console.printf (CON_DBG, "Aborting resync for player %s.\n", their->player.callsign);
#endif
		DeleteSyncData (i);
		}
	else
		i++;
}

//------------------------------------------------------------------------------

static int objFilter [] = {1, 1, 0, 0, 0, 0, 1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1};

static inline int NetworkFilterObject (CObject *objP)
{
	short t = objP->info.nType;
#if DBG
if (t == nDbgObjType)
	BRP;
#endif
if (t >= MAX_OBJECT_TYPES)
	return 1;
if (objFilter [t])
	return 1;
if ((t == OBJ_WEAPON) && (objP->info.nId != SMALLMINE_ID))
	return 1;
return 0;
}

//------------------------------------------------------------------------------

static inline int NetworkObjFrameFilter (tNetworkSyncData *syncP)
{
if (!syncP->objs.nFrame++)
	return 1;
if (syncP->objs.nFrame < syncP->objs.missingFrames.nFrame)
	return 0;
return 1;
}

//------------------------------------------------------------------------------

ubyte objBuf [MAX_PACKET_SIZE];

void NetworkSyncObjects (tNetworkSyncData *syncP)
{
	CObject	*objP;
	sbyte		owner;
	short		nRemoteObj;
	int		bufI, i, h;
	int		nObjFrames = 0;
	int		nPlayer = syncP->player [1].player.connected;

// Send clear OBJECTS array CTrigger and send player num
objFilter [OBJ_MARKER] = !gameStates.app.bHaveExtraGameInfo [1];
for (h = 0; h < OBJ_PACKETS_PER_FRAME; h++) {	// Do more than 1 per frame, try to speed it up without
																// over-stressing the receiver.
	nObjFrames = 0;
	memset (objBuf, 0, MAX_PAYLOAD_SIZE);
	objBuf [0] = PID_OBJECT_DATA;
	bufI = (gameStates.multi.nGameType == UDP_GAME) ? 4 : 3;

	if (syncP->objs.nCurrent == -1) {	// first packet tells the receiver to reset it's object data
		syncP->objs.nSent = 0;
		syncP->objs.nMode = 0;
		syncP->objs.nFrame = 0;
		NW_SET_SHORT (objBuf, bufI, syncP->objs.missingFrames.nFrame ? -3 : -1);		// object number -1          
		NW_SET_BYTE (objBuf, bufI, nPlayer);                            
		bufI += 2;									// Placeholder for nRemoteObj, not used here
		syncP->objs.nCurrent = 0;
		nObjFrames = 1;		// first frame contains "reset object data" info
		}

	for (i = syncP->objs.nCurrent, objP = OBJECTS + i; i <= gameData.objs.nLastObject [0]; i++, objP++) {
		if (NetworkFilterObject (objP))
			continue;
		if (syncP->objs.nMode) { 
			 if ((gameData.multigame.nObjOwner [i] != -1) && (gameData.multigame.nObjOwner [i] != nPlayer))
				continue;
			}
		else {
			if ((gameData.multigame.nObjOwner [i] == -1) || (gameData.multigame.nObjOwner [i] == nPlayer))
				continue;
			}
		if ((MAX_PAYLOAD_SIZE - bufI - 1) < int (sizeof (tBaseObject)) + 5)
			break; // Not enough room for another CObject
		nObjFrames++;
		syncP->objs.nSent++;
		nRemoteObj = ObjnumLocalToRemote (short (i), &owner);
		Assert (owner == gameData.multigame.nObjOwner [i]);
		Assert (nRemoteObj >= 0);
		NW_SET_SHORT (objBuf, bufI, i);      
		NW_SET_BYTE (objBuf, bufI, owner);                                 
		NW_SET_SHORT (objBuf, bufI, nRemoteObj); 
		NW_SET_BYTES (objBuf, bufI, &objP->info, sizeof (tBaseObject));
#if defined(WORDS_BIGENDIAN) || defined(__BIG_ENDIAN__)
		if (gameStates.multi.nGameType >= IPX_GAME)
			SwapObject (reinterpret_cast<CObject*> (objBuf + bufI - sizeof (tBaseObject)));
#endif
		}
	if (nObjFrames) {	// Send any objects we've buffered
		syncP->objs.nCurrent = i;	
		if (NetworkObjFrameFilter (syncP)) {
			objBuf [1] = nObjFrames;  
			if (gameStates.multi.nGameType == UDP_GAME)
				*reinterpret_cast<short*> (objBuf + 2) = INTEL_SHORT (syncP->objs.nFrame);
			else
				objBuf [2] = (ubyte) syncP->objs.nFrame;
			Assert (bufI <= MAX_PAYLOAD_SIZE);
			if (gameStates.multi.nGameType >= IPX_GAME)
				IPXSendInternetPacketData (
					objBuf, bufI, 
					syncP->player [1].player.network.Server (), 
					syncP->player [1].player.network.Node ());
			 }
		}
	if (i > gameData.objs.nLastObject [0]) {
		if (syncP->objs.nMode) {
			syncP->objs.nCurrent = i;
			// Send count so other CSide can make sure he got them all
			objBuf [0] = PID_OBJECT_DATA;
			objBuf [1] = 1;
			syncP->objs.nFrame++;
			if (gameStates.multi.nGameType == UDP_GAME) {
				bufI = 2;
				NW_SET_SHORT (objBuf, bufI, syncP->objs.nFrame); 
				}
			else {
				objBuf [2] = (ubyte) syncP->objs.nFrame;
				bufI = 3;
				}
			nRemoteObj = syncP->objs.missingFrames.nFrame ? -4 : -2;
			NW_SET_SHORT (objBuf, bufI, nRemoteObj);
			NW_SET_SHORT (objBuf, bufI, syncP->objs.nSent);
			syncP->nState = syncP->objs.missingFrames.nFrame ? 1 : 2;
			}
		else {
			syncP->objs.nCurrent = 0;
			syncP->objs.nMode = 1; // go to next mode
			}
		break;
		}
	}
}

//------------------------------------------------------------------------------

void NetworkSyncPlayer (tNetworkSyncData *syncP)
{
	int nPlayer = syncP->player [1].player.connected;

//OLD IPXSendPacketData (objBuf, 8, &syncP->player [1].player.node);
if (gameStates.multi.nGameType >= IPX_GAME)
	IPXSendInternetPacketData (objBuf, 8, 
										syncP->player [1].player.network.Server (), 
										syncP->player [1].player.network.Node ());
// Send sync packet which tells the player who he is and to start!
NetworkSendRejoinSync (nPlayer, syncP);

// Turn off send CObject mode
syncP->objs.nCurrent = -1;
syncP->nState = 3;
syncP->objs.nSent = 0;
syncP->nExtras = 1; // start to send extras
syncP->nExtrasPlayer = nPlayer;
}

//------------------------------------------------------------------------------

void NetworkSyncExtras (tNetworkSyncData *syncP)
{
Assert (syncP->nExtrasPlayer > -1);
if (!IAmGameHost ()) {
#if 1			
  console.printf (CON_DBG, "Hey! I'm not the master and I was gonna send info!\n");
#endif
	}
if (syncP->nExtras == 1)
	NetworkSendFlyThruTriggers (syncP->nExtrasPlayer);
else if (syncP->nExtras == 2)
	NetworkSendDoorUpdates (syncP->nExtrasPlayer);
else if (syncP->nExtras == 3)
	NetworkSendMarkers ();
else if (syncP->nExtras == 4) {
	if (gameData.app.GameMode (GM_MULTI_ROBOTS))
		MultiSendStolenItems ();
	}
else if (syncP->nExtras == 5) {
	if (netGame.GetPlayTimeAllowed () || netGame.GetScoreGoal ())
		MultiSendScoreGoalCounts ();
	}
else if (syncP->nExtras == 6)
	NetworkSendSmashedLights (syncP->nExtrasPlayer);
else if (syncP->nExtras == 7)
	NetworkSendPlayerFlags ();    
else if (syncP->nExtras == 8)
	MultiSendWeapons (1);  
else if (syncP->nExtras == 9)
	MultiSendWeaponStates ();  
else if (syncP->nExtras == 10)
	MultiSendMonsterball (1, 1);  
else {
	syncP->nExtras = 0;
	syncP->nState = 0;
	syncP->nExtrasPlayer = -1;
	memset (&syncP->player [1], 0, sizeof (syncP->player [1]));
	return;
	}
syncP->nExtras++;
}

//------------------------------------------------------------------------------

void NetworkSyncConnection (tNetworkSyncData *syncP)
{
#if 1
	time_t	t = (time_t) SDL_GetTicks ();

if (t < syncP->timeout)
	return;
syncP->timeout = t + 100 / Clamp (PacketsPerSec (), (short) MIN_PPS, (short) DEFAULT_PPS);
#endif
if (syncP->bExtraGameInfo) {
	NetworkSendExtraGameInfo (&syncP->player [0]);
	syncP->bExtraGameInfo = false;
	}
if (syncP->bAllowedPowerups) {
	MultiSendPowerupUpdate ();
	syncP->bAllowedPowerups = false;
	}
else if (syncP->nState == 1) {
	syncP->objs.missingFrames.nFrame = 0;
	NetworkSyncObjects (syncP);
	syncP->bExtraGameInfo = false;
	syncP->bAllowedPowerups = false;
	}
else if (syncP->nState == 2) {
	NetworkSyncPlayer (syncP);
	syncP->bExtraGameInfo = true;
	syncP->bAllowedPowerups = true;
	}
else if (syncP->nState == 3) {
	if (syncP->objs.missingFrames.nFrame) {
		NetworkSyncObjects (syncP);
		if (!syncP->nState)
			syncP->objs.missingFrames.nFrame = 0;
		}
	else
		syncP->nState = 4;
	}
else if (syncP->nState == 4) {
	if (syncP->nExtras) {
		NetworkSyncExtras (syncP);
		if ((syncP->bExtraGameInfo = (syncP->nExtras == 0))) {
			DeleteSyncData (short (syncP - networkData.sync));
			}
		}
	}
}

//------------------------------------------------------------------------------

void NetworkDoSyncFrame (void)
{
for (short i = 0; i < networkData.nJoining; i++)
	NetworkSyncConnection (networkData.sync + i);
}

//------------------------------------------------------------------------------

void NetworkUpdateNetGame (void)
{
	// Update the netgame struct with current game variables

	int i, j;

netGame.m_info.nConnected = 0;
for (i = 0; i < gameData.multiplayer.nPlayers; i++)
	if (gameData.multiplayer.players [i].Connected ())
		netGame.m_info.nConnected++;

// This is great: D2 1.0 and 1.1 ignore upper part of the gameFlags field of
//	the tNetGameInfoLite struct when you're sitting on the join netgame gameData.render.screen.  We can
//	"sneak" Hoard information into this field.  This is better than sending 
//	another packet that could be lost in transit.
if (HoardEquipped ()) {
	if (gameData.app.GameMode (GM_MONSTERBALL))
		netGame.m_info.gameFlags |= NETGAME_FLAG_MONSTERBALL;
	else if (IsEntropyGame)
		netGame.m_info.gameFlags |= NETGAME_FLAG_ENTROPY;
	else if (IsHoardGame) {
		netGame.m_info.gameFlags |= NETGAME_FLAG_HOARD;
		if (IsTeamGame)
			netGame.m_info.gameFlags |= NETGAME_FLAG_TEAM_HOARD;
		}
	}
if (networkData.nStatus == NETSTAT_STARTING)
	return;
netGame.m_info.nNumPlayers = gameData.multiplayer.nPlayers;
netGame.m_info.gameStatus = networkData.nStatus;
netGame.m_info.nMaxPlayers = gameData.multiplayer.nMaxPlayers;
for (i = 0; i < MAX_NUM_NET_PLAYERS; i++) {
	memcpy (netPlayers [0].m_info.players [i].callsign, gameData.multiplayer.players [i].callsign, sizeof (netPlayers [0].m_info.players [i].callsign));
	netPlayers [0].m_info.players [i].connected = gameData.multiplayer.players [i].connected;
	for (j = 0; j < MAX_NUM_NET_PLAYERS; j++)
		*netGame.Kills (i, j) = gameData.multigame.score.matrix [i][j];
	*netGame.Killed (i) = gameData.multiplayer.players [i].netKilledTotal;
	*netGame.PlayerKills (i) = gameData.multiplayer.players [i].netKillsTotal;
	*netGame.PlayerScore (i) = gameData.multiplayer.players [i].score;
	*netGame.PlayerFlags (i) = (gameData.multiplayer.players [i].flags & (PLAYER_FLAGS_BLUE_KEY | PLAYER_FLAGS_RED_KEY | PLAYER_FLAGS_GOLD_KEY));
	}
*netGame.TeamKills (0) = gameData.multigame.score.nTeam [0];
*netGame.TeamKills (1) = gameData.multigame.score.nTeam [1];
netGame.m_info.SetLevel (missionManager.nCurrentLevel);
}

//------------------------------------------------------------------------------

static inline fix SyncPollTimeout (void)
{
#if DBG
return 5000;
#else
return 3000;
#endif
}

//------------------------------------------------------------------------------

void ResetSyncTimeout (bool bInit = false)
{
if (bInit)
	networkData.toSyncPoll.Setup (SyncPollTimeout ());
networkData.toSyncPoll.Start ();
}

//------------------------------------------------------------------------------

void NetworkPackObjects (void)
{
// Switching modes, pack the CObject array
SpecialResetObjects ();
}                               

//------------------------------------------------------------------------------
// wait for sync packets from the game host after having sent join request

int NetworkSyncPoll (CMenu& menu, int& key, int nCurItem, int nState)
{
if (gameData.multiplayer.nPlayers && IAmGameHost ()) {
	key = -3;
	return nCurItem;
	}
if (nState)
	return nCurItem;

	/*int nPackets =*/ NetworkListen ();

if (networkData.nStatus != NETSTAT_WAITING) { // Status changed to playing, exit the menu
	if (NetworkVerifyPlayers ())
		NetworkAbortSync ();
	key = -2;
	return nCurItem;
	}

if (networkData.nStatus == NETSTAT_PLAYING) {
	ResetSyncTimeout ();
	return nCurItem;
	}
if (networkData.toSyncPoll.Expired ()) {	// Poll time expired, re-send request
#if 1			
	console.printf (CON_DBG, "Re-sending join request.\n");
#endif
#if DBG
	audio.PlaySound (SOUND_HUD_MESSAGE, SOUNDCLASS_GENERIC, I2X (1) / 2);
#endif
	if (NetworkSendRequest () < 0)
		key = -2;
	}
return nCurItem;
}

//------------------------------------------------------------------------------

int NetworkWaitForSync (void)
{
	char					text [60];
	CMenu					m (2);
	int					i, choice;
	tSequencePacket	me;

networkData.nStatus = NETSTAT_WAITING;
m.AddText ("", text);
m.AddText ("", const_cast<char*> (TXT_NET_LEAVE));
networkData.nJoinState = 0;
i = NetworkSendRequest ();
if (i < 0) {
#if DBG
	NetworkSendRequest ();
#endif
	return -1;
	}
sprintf (m [0].m_text, "%s\n'%s' %s", TXT_NET_WAITING, netPlayers [0].m_info.players [i].callsign, TXT_NET_TO_ENTER);
ResetSyncTimeout (true);
do {
	gameStates.menus.nInMenu = -gameStates.menus.nInMenu;
	choice = m.Menu (NULL, TXT_HOST_WAIT, NetworkSyncPoll);
	gameStates.menus.nInMenu = -gameStates.menus.nInMenu;
	} while (choice > -1);
if (choice == -3)
	return 0;
if (networkData.nStatus == NETSTAT_PLAYING)  
	return 0;
else if (networkData.nStatus == NETSTAT_AUTODL) {
	if (!downloadManager.DownloadMission (netGame.m_info.szMissionName))
		networkData.nStatus = NETSTAT_MENU;
	else {
		networkData.nStatus = NETSTAT_PLAYING;
		hogFileManager.ReloadMission (gameFolders.missions.szDownloads); // reload hog file's file info 
		return 1;
		}
	}
#if 1			
console.printf (CON_DBG, "Aborting join.\n");
#endif
me.nType = PID_QUIT_JOINING;
memcpy (me.player.callsign, LOCALPLAYER.callsign, CALLSIGN_LEN+1);
if (gameStates.multi.nGameType >= IPX_GAME) {
	memcpy (me.player.network.Node (), IpxGetMyLocalAddress (), 6);
	memcpy (me.player.network.Server (), IpxGetMyServerAddress (), 4);
	SendInternetSequencePacket (me, netPlayers [0].m_info.players [0].network.Server (), 
										 netPlayers [0].m_info.players [0].network.Node ());
}
gameData.multiplayer.nPlayers = 0;
SetFunctionMode (FMODE_MENU);
gameData.app.SetGameMode (GM_GAME_OVER);
return -1;     // they cancelled               
}

//------------------------------------------------------------------------------

static inline fix WaitAllPollTimeout (void)
{
#if DBG
return 5000;
#else
return 3000;
#endif
}

//------------------------------------------------------------------------------

static inline void ResetWaitAllTimeout (void)
{
networkData.toWaitAllPoll = (time_t) SDL_GetTicks () + WaitAllPollTimeout ();
}

//------------------------------------------------------------------------------

int NetworkWaitAllPoll (CMenu& menu, int& key, int nCurItem, int nState)
{
if (nState)
	return nCurItem;

if ((time_t) SDL_GetTicks () > networkData.toWaitAllPoll) {
	NetworkSendAllInfoRequest (PID_SEND_ALL_GAMEINFO, networkData.nSecurityCheck);
	ResetWaitAllTimeout ();
	}
NetworkDoBigWait (networkData.bWaitAllChoice);  
if (networkData.nSecurityCheck == -1)
	key = -2;
return nCurItem;
}
 
//------------------------------------------------------------------------------

int NetworkWaitForAllInfo (int choice)
 {
  int pick;
  
  CMenu m (2);

m.AddText ("", "Press Escape to cancel");
networkData.bWaitAllChoice = choice;
networkData.nStartWaitAllTime=TimerGetApproxSeconds ();
networkData.nSecurityCheck = activeNetGames [choice].m_info.nSecurity;
networkData.nSecurityFlag = 0;

networkData.toWaitAllPoll = 0;
do {
	pick = m.Menu (NULL, TXT_CONNECTING, NetworkWaitAllPoll);
	} while ((pick > -1) && (networkData.nSecurityCheck != -1));
if (networkData.nSecurityCheck == -1) {   
	networkData.nSecurityCheck = 0;     
	return 1;
	}
networkData.nSecurityCheck = 0;      
return 0;
 }

//------------------------------------------------------------------------------

int NetworkWaitForPlayerInfo (void)
{
	int						size = 0, retries = 0;
	ubyte						packet [MAX_PACKET_SIZE];
	CAllNetPlayersInfo	playerData;
	uint						xTimeout;
	ubyte						id = 0;

#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
	CAllNetPlayersInfo info_struct;
#endif

if (gameStates.multi.nGameType >= IPX_GAME)
	if (!networkData.bActive) 
		return 0;
#if 1			
if (!IsNetworkGame && (gameStates.app.nFunctionMode == FMODE_GAME))
	console.printf (CON_DBG, "Calling NetworkWaitForPlayerInfo () when not in net game.\n");
#endif	
if (networkData.nStatus == NETSTAT_PLAYING) {
	Int3 (); //MY GOD! Get Jason...this is the source of many problems
	return 0;
	}
xTimeout = SDL_GetTicks () + 5000;
while (networkData.bWaitingForPlayerInfo && (retries < 50) && (SDL_GetTicks () < xTimeout)) {
	if (gameStates.multi.nGameType >= IPX_GAME) {
		size = IpxGetPacketData (packet);
		id = packet [0];
		}
	if ((size > 0) && (id == PID_PLAYERSINFO)) {
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
		ReceiveNetPlayersPacket (packet, &playerData);
#else
		memcpy (&playerData.m_info, &packet [0], sizeof (playerData.m_info));
#endif
		retries++;
		if (networkData.nSecurityFlag == NETSECURITY_WAIT_FOR_PLAYERS) {
#if SECURITY_CHECK
			if (networkData.nSecurityNum != playerData.m_info.nSecurity)
				continue;
#endif
			networkData.nSecurityFlag = NETSECURITY_OFF;
			networkData.nSecurityNum = 0;
			}
		else {
			networkData.nSecurityFlag = NETSECURITY_WAIT_FOR_GAMEINFO;
			networkData.nSecurityNum = playerData.m_info.nSecurity;
			}
		netPlayers [1] = playerData;
		playerInfoP = &netPlayers [1];
		networkData.bWaitingForPlayerInfo = 0;
		return 1;
		}
	}
return 0;
}

//------------------------------------------------------------------------------

void NetworkDoBigWait (int choice)
{
	int						size;
	ubyte						packet [MAX_PACKET_SIZE], *data;
	CAllNetPlayersInfo	playerData;
  
while (0 < (size = IpxGetPacketData (packet))) {
	data = &packet [0];

	switch (data [0]) {  
		case PID_GAME_INFO:
			if (gameStates.multi.nGameType >= IPX_GAME)
				ReceiveFullNetGamePacket (data, &tempNetInfo); 
			else
				memcpy (&tempNetInfo.m_info, data, tempNetInfo.Size ());
#if SECURITY_CHECK
			if (tempNetInfo.m_info.nSecurity != networkData.nSecurityCheck)
				break;
#endif
			if (networkData.nSecurityFlag == NETSECURITY_WAIT_FOR_GAMEINFO) {
#if SECURITY_CHECK
				if ((playerInfoP->m_info.nSecurity == tempNetInfo.m_info.nSecurity) && (playerInfoP->m_info.nSecurity == networkData.nSecurityCheck)) 
#endif
					{
					activeNetGames [choice] = tempNetInfo;
					activeNetPlayers [choice] = *playerInfoP;
					networkData.nSecurityCheck = -1;
					}
				}
			else {
				networkData.nSecurityFlag = NETSECURITY_WAIT_FOR_PLAYERS;
				networkData.nSecurityNum = tempNetInfo.m_info.nSecurity;
				if (NetworkWaitForPlayerInfo ()) {
#if 1			
					console.printf (CON_DBG, "HUH? Game=%d Player=%d\n", networkData.nSecurityNum, playerInfoP->m_info.nSecurity);
#endif
					activeNetGames [choice] = tempNetInfo;
					activeNetPlayers [choice] = *playerInfoP;
					networkData.nSecurityCheck = -1;
					}
				networkData.nSecurityFlag = 0;
				networkData.nSecurityNum = 0;
				}
			SetupPowerupFilter (&tempNetInfo.m_info);
			break;

		case PID_EXTRA_GAMEINFO: 
			if (gameStates.multi.nGameType >= IPX_GAME) {
				NetworkProcessExtraGameInfo (data);
				}
			break;

		case PID_PLAYERSINFO:
			if (gameStates.multi.nGameType < IPX_GAME)
				playerData = *(reinterpret_cast<tAllNetPlayersInfo*> (data));
			else {
#if !(defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__))
				playerData = *(reinterpret_cast<tAllNetPlayersInfo*> (data));
#else
				ReceiveNetPlayersPacket (data, &playerData);
#endif
				}
#if SECURITY_CHECK
			if (playerData.m_info.nSecurity != networkData.nSecurityCheck) 
				break;     // If this isn't the guy we're looking for, move on
#endif
			netPlayers [1] = playerData;
			playerInfoP = &netPlayers [1];
			networkData.bWaitingForPlayerInfo = 0;
			networkData.nSecurityNum = netPlayers [1].m_info.nSecurity;
			networkData.nSecurityFlag = NETSECURITY_WAIT_FOR_GAMEINFO;
			break;

		default:
			break;
		}
	}
}

//------------------------------------------------------------------------------

int NetworkRequestPoll (CMenu& menu, int& key, int nCurItem, int nState)
{
if (!IAmGameHost ()) {
	key = -2;
	return nCurItem;
	}
if (nState)
	return nCurItem;

int i = 0;

static CTimeout to (500);
// tell other players that I am here
if (to.Expired ()) {
	for (i = 0; i < gameData.multiplayer.nPlayers; i++) {
		if (i != N_LOCALPLAYER) {
			pingStats [i].launchTime = -1; //TimerGetFixedSeconds ();
			NetworkSendPing (i); // tell clients already connected server is still alive
			}
		}
	}
NetworkListen ();

int nReady = 0;

for (i = 0; i < gameData.multiplayer.nPlayers; i++) {
	if ((ubyte) gameData.multiplayer.players [i].connected < 2)
		nReady++;
	}
if (nReady == gameData.multiplayer.nPlayers) // All players have checked in or are disconnected
	key = -2;
return nCurItem;
}

//------------------------------------------------------------------------------

void NetworkWaitForRequests (void)
{
	// Wait for other players to load the level before we send the sync
	int	choice, i;
	CMenu	m (1);

networkData.nStatus = NETSTAT_WAITING;
m.AddText ("", const_cast<char*> (TXT_NET_LEAVE));
NetworkFlush ();
CONNECT (N_LOCALPLAYER, CONNECT_PLAYING);

for (;;) {
	choice = m.Menu (NULL, TXT_CLIENT_WAIT, NetworkRequestPoll);        
	if (choice == -1) {
		// User aborted
		choice = TextBox (NULL, BG_STANDARD, 3, TXT_YES, TXT_NO, TXT_START_NOWAIT, TXT_QUITTING_NOW);
		if (choice == 2)
			return;
		if (choice != 0)
			continue;
		
		// User confirmed abort
		for (i = 0; i < gameData.multiplayer.nPlayers; i++) {
			if (gameData.multiplayer.players [i].Connected () && (i != N_LOCALPLAYER)) {
				if (gameStates.multi.nGameType >= IPX_GAME)
					NetworkDumpPlayer (netPlayers [0].m_info.players [i].network.Server (), netPlayers [0].m_info.players [i].network.Node (), DUMP_ABORTED);
				}
			}
			longjmp (gameExitPoint, 0);  
		}
	else if (choice != -2)
		continue;
	break;
	}
}

//------------------------------------------------------------------------------

/* Do required syncing after each level, before starting new one */
int NetworkLevelSync (void)
{
	int result;
	networkData.bSyncPackInited = 0;

NetworkFlush (); // Flush any old packets
for (;;) {
	if (gameData.multiplayer.nPlayers && IAmGameHost ()) {
		NetworkWaitForRequests ();
		if (IAmGameHost ()) {
			NetworkSendSync ();
			result = (N_LOCALPLAYER < 0) ? -1 : 0;
			break;
			}
		}
	else {
		result = NetworkWaitForSync ();
		if (!(gameData.multiplayer.nPlayers && IAmGameHost ()))
			break;
		}
	}
if (result < 0) {
	CONNECT (N_LOCALPLAYER, CONNECT_DISCONNECTED);
	NetworkSendEndLevelPacket ();
	//longjmp (gameExitPoint, 0);
	}
else
	NetworkCountPowerupsInMine ();
return result;
}

//------------------------------------------------------------------------------

