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

void NetworkStopResync (tPlayerSyncData *their)
{
for (int16_t i = 0; i < networkData.nJoining; )
	if (!CmpNetPlayers (networkData.syncInfo [i].player [1].player.callsign, their->player.callsign, 
							  &networkData.syncInfo [i].player [1].player.network, &their->player.network)) {
#if 1      
		console.printf (CON_DBG, "Aborting resync for player %s.\n", their->player.callsign);
#endif
		DeleteSyncData (i);
		}
	else
		i++;
}

//------------------------------------------------------------------------------

static int32_t objFilter [] = {1, 1, 0, 0, 0, 0, 1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1};

static inline int32_t NetworkFilterObject (CObject *objP)
{
	int16_t t = objP->info.nType;
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

static inline int32_t NetworkObjFrameFilter (tNetworkSyncInfo *syncInfoP)
{
if (!syncInfoP->objs.nFrame++)
	return 1;
if (!syncInfoP->objs.nFramesToSkip)
	return 1;
if (syncInfoP->objs.nFrame <= syncInfoP->objs.nFramesToSkip)
	return 0;
syncInfoP->objs.nFramesToSkip = 0;
return 1;
}

//------------------------------------------------------------------------------

static inline bool SendObject (int32_t nMode, int32_t nLocalObj, int32_t nPlayer)
{
if (NetworkFilterObject (OBJECTS + nLocalObj)) 
	return false;
return nMode
		 ? (gameData.multigame.nObjOwner [nLocalObj] == -1) || (gameData.multigame.nObjOwner [nLocalObj] == nPlayer) // send objects owned by the local player or by nobody
		 : (gameData.multigame.nObjOwner [nLocalObj] != -1) && (gameData.multigame.nObjOwner [nLocalObj] != nPlayer); // send objects owned by other players
}

//------------------------------------------------------------------------------

uint8_t objBuf [MAX_PACKET_SIZE];

void NetworkSyncObjects (tNetworkSyncInfo *syncInfoP)
{
	int32_t		bufI, nLocalObj, nPacketsLeft;
	int32_t		nObjFrames = 0;
	int32_t		nPlayer = syncInfoP->player [1].player.connected;
	
syncInfoP->bDeferredSync = networkThread.SendInBackground ();

// Send clear OBJECTS array CTrigger and send player num
objFilter [OBJ_MARKER] = !gameStates.app.bHaveExtraGameInfo [1];
for (nPacketsLeft = OBJ_PACKETS_PER_FRAME; nPacketsLeft; nPacketsLeft--) {
	nObjFrames = 0;
	memset (objBuf, 0, MAX_PAYLOAD_SIZE);
	objBuf [0] = PID_OBJECT_DATA;
	bufI = (gameStates.multi.nGameType == UDP_GAME) ? 4 : 3;

	if (syncInfoP->objs.nCurrent == -1) {	// first packet tells the receiver to reset it's object data
		syncInfoP->objs.nSent = 0;
		syncInfoP->objs.nMode = 0;
		syncInfoP->objs.nFrame = 0;
		if (!syncInfoP->objs.nFramesToSkip) {
			//NetworkSendRejoinSync (nPlayer, syncInfoP);
			NW_SET_SHORT (objBuf, bufI, -1);		// object number -1          
			NW_SET_BYTE (objBuf, bufI, nPlayer);                            
			NW_SET_SHORT (objBuf, bufI, -1);		// Placeholder for nRemoteObj, not used here
			}
		syncInfoP->objs.nCurrent = 0;
		nObjFrames = 1;		// first frame contains "reset object data" info
		}

	for (nLocalObj = syncInfoP->objs.nCurrent; nLocalObj <= gameData.objs.nLastObject [0]; nLocalObj++) {
		if (SendObject (syncInfoP->objs.nMode, nLocalObj, nPlayer)) {
			if ((MAX_PAYLOAD_SIZE - bufI - 1) < int32_t (sizeof (tBaseObject)) + 5)
				break; // Not enough room for another CObject
			int8_t	nObjOwner;
			int16_t nRemoteObj = GetRemoteObjNum (int16_t (nLocalObj), nObjOwner);
			NW_SET_SHORT (objBuf, bufI, nLocalObj);      
			NW_SET_BYTE (objBuf, bufI, nObjOwner);                                 
			NW_SET_SHORT (objBuf, bufI, nRemoteObj); 
			NW_SET_BYTES (objBuf, bufI, &OBJECTS [nLocalObj].info, sizeof (tBaseObject));
#if defined(WORDS_BIGENDIAN) || defined(__BIG_ENDIAN__)
			if (gameStates.multi.nGameType >= IPX_GAME)
				SwapObject (reinterpret_cast<CObject*> (objBuf + bufI - sizeof (tBaseObject)));
#endif
			nObjFrames++;
			syncInfoP->objs.nSent++;
			}
		}

	if (nObjFrames) {	// Send any objects we've buffered
		syncInfoP->objs.nCurrent = nLocalObj;	
		//if (NetworkObjFrameFilter (syncInfoP)) { // this statement skips any objects successfully sync'd in case the client has reported missing frames
		if (++syncInfoP->objs.nFrame > syncInfoP->objs.nFramesToSkip) {
			objBuf [1] = nObjFrames;  
			if (gameStates.multi.nGameType == UDP_GAME)
				*reinterpret_cast<int16_t*> (objBuf + 2) = INTEL_SHORT (syncInfoP->objs.nFrame);
			else
				objBuf [2] = (uint8_t) syncInfoP->objs.nFrame;
			Assert (bufI <= MAX_PAYLOAD_SIZE);
			if (gameStates.multi.nGameType >= IPX_GAME) {
				if (!syncInfoP->bDeferredSync)
					IPXSendInternetPacketData (objBuf, bufI, syncInfoP->player [1].player.network.Network (), syncInfoP->player [1].player.network.Node ());
				else if (!networkThread.Send (objBuf, bufI, syncInfoP->player [1].player.network.Network (), syncInfoP->player [1].player.network.Node ())) {
					syncInfoP->bDeferredSync = false;
					nPacketsLeft = OBJ_PACKETS_PER_FRAME;
					syncInfoP->objs.nCurrent = -1;
					break;
					}
				}	
			}
		}

	if (syncInfoP->objs.nCurrent < 0)
		continue;

	if (nLocalObj > gameData.objs.nLastObject [0]) {
		if (syncInfoP->objs.nMode) { // need to send the finishing object data
			syncInfoP->objs.nCurrent = nLocalObj;
			// Send count so other CSide can make sure he got them all
			objBuf [0] = PID_OBJECT_DATA;
			objBuf [1] = 1;
			syncInfoP->objs.nFrame++;
			if (gameStates.multi.nGameType == UDP_GAME) {
				bufI = 2;
				NW_SET_SHORT (objBuf, bufI, syncInfoP->objs.nFrame); 
				}
			else {
				objBuf [2] = (uint8_t) syncInfoP->objs.nFrame;
				bufI = 3;
				}
			NW_SET_SHORT (objBuf, bufI, -2);
			NW_SET_BYTE (objBuf, bufI, -1);                                 
			NW_SET_SHORT (objBuf, bufI, syncInfoP->objs.nSent);
			syncInfoP->nState = 2;
			}
		else {
			syncInfoP->objs.nCurrent = 0;
			syncInfoP->objs.nMode = 1; // go to next mode
			}
		break;
		}
	}
}

//------------------------------------------------------------------------------

void NetworkSyncPlayer (tNetworkSyncInfo *syncInfoP)
{
	int32_t nPlayer = syncInfoP->player [1].player.connected;

//OLD IPXSendPacketData (objBuf, 8, &syncInfoP->player [1].player.node);
if (gameStates.multi.nGameType >= IPX_GAME)
	networkThread.Send (objBuf, (gameStates.multi.nGameType == UDP_GAME) ? 9 : 8, syncInfoP->player [1].player.network.Network (), syncInfoP->player [1].player.network.Node ());
// Send sync packet which tells the player who he is and to start!
NetworkSendRejoinSync (nPlayer, syncInfoP);

// Turn off send CObject mode
syncInfoP->objs.nCurrent = -1;
syncInfoP->nState = 3;
syncInfoP->objs.nSent = 0;
syncInfoP->nExtras = 1; // start to send extras
syncInfoP->nExtrasPlayer = nPlayer;
}

//------------------------------------------------------------------------------

void NetworkSyncExtras (tNetworkSyncInfo *syncInfoP)
{
Assert (syncInfoP->nExtrasPlayer > -1);
if (!IAmGameHost ()) {
#if 1			
  console.printf (CON_DBG, "Hey! I'm not the master and I was gonna send info!\n");
#endif
	}
if (syncInfoP->nExtras == 1)
	NetworkSendFlyThruTriggers (syncInfoP->nExtrasPlayer);
else if (syncInfoP->nExtras == 2)
	NetworkSendDoorUpdates (syncInfoP->nExtrasPlayer);
else if (syncInfoP->nExtras == 3)
	NetworkSendMarkers ();
else if (syncInfoP->nExtras == 4) {
	if (gameData.app.GameMode (GM_MULTI_ROBOTS))
		MultiSendStolenItems ();
	}
else if (syncInfoP->nExtras == 5) {
	if (netGameInfo.GetPlayTimeAllowed () || netGameInfo.GetScoreGoal ())
		MultiSendScoreGoalCounts ();
	}
else if (syncInfoP->nExtras == 6)
	NetworkSendSmashedLights (syncInfoP->nExtrasPlayer);
else if (syncInfoP->nExtras == 7)
	NetworkSendPlayerFlags ();    
else if (syncInfoP->nExtras == 8)
	MultiSendWeapons (1);  
else if (syncInfoP->nExtras == 9)
	MultiSendWeaponStates ();  
else if (syncInfoP->nExtras == 10)
	MultiSendMonsterball (1, 1);  
else {
	syncInfoP->nExtras = 0;
	syncInfoP->nState = 0;
	syncInfoP->nExtrasPlayer = -1;
	memset (&syncInfoP->player [1], 0, sizeof (syncInfoP->player [1]));
	return;
	}
syncInfoP->nExtras++;
}

//------------------------------------------------------------------------------

void NetworkSyncConnection (tNetworkSyncInfo *syncInfoP)
{
#if 1
	time_t	t = (time_t) SDL_GetTicks ();

if (t < syncInfoP->timeout)
	return;
syncInfoP->timeout = t + 100 / Clamp (MinPPS (), (int16_t) MIN_PPS, (int16_t) DEFAULT_PPS);
#endif
if (syncInfoP->bExtraGameInfo) {
	NetworkSendExtraGameInfo (&syncInfoP->player [0]);
	syncInfoP->bExtraGameInfo = false;
	}
if (syncInfoP->bAllowedPowerups) {
	MultiSendPowerupUpdate ();
	syncInfoP->bAllowedPowerups = false;
	}
else if (syncInfoP->nState == 1) {
	NetworkSyncObjects (syncInfoP);
	syncInfoP->bExtraGameInfo = false;
	syncInfoP->bAllowedPowerups = false;
	}
else if (syncInfoP->nState == 2) {
	if (!networkThread.SyncInProgress ()) {
		NetworkSyncPlayer (syncInfoP);
		syncInfoP->bExtraGameInfo = true;
		syncInfoP->bAllowedPowerups = true;
		}
	}
else if (syncInfoP->nState == 3) {
	if (syncInfoP->nExtras) {
		NetworkSyncExtras (syncInfoP);
		if ((syncInfoP->bExtraGameInfo = (syncInfoP->nExtras == 0))) {
			DeleteSyncData (int16_t (syncInfoP - networkData.syncInfo));
			}
		}
	}
}

//------------------------------------------------------------------------------

void NetworkDoSyncFrame (void)
{
for (int16_t i = 0; i < networkData.nJoining; i++)
	NetworkSyncConnection (networkData.syncInfo + i);
}

//------------------------------------------------------------------------------

void NetworkUpdateNetGame (void)
{
	// Update the netgame struct with current game variables

	int32_t i, j;

netGameInfo.m_info.nConnected = 0;
for (i = 0; i < N_PLAYERS; i++)
	if (PLAYER (i).IsConnected ())
		netGameInfo.m_info.nConnected++;

// This is great: D2 1.0 and 1.1 ignore upper part of the gameFlags field of
//	the tNetGameInfoLite struct when you're sitting on the join netgame gameData.render.screen.  We can
//	"sneak" Hoard information into this field.  This is better than sending 
//	another packet that could be lost in transit.
if (HoardEquipped ()) {
	if (gameData.app.GameMode (GM_MONSTERBALL))
		netGameInfo.m_info.gameFlags |= NETGAME_FLAG_MONSTERBALL;
	else if (IsEntropyGame)
		netGameInfo.m_info.gameFlags |= NETGAME_FLAG_ENTROPY;
	else if (IsHoardGame) {
		netGameInfo.m_info.gameFlags |= NETGAME_FLAG_HOARD;
		if (IsTeamGame)
			netGameInfo.m_info.gameFlags |= NETGAME_FLAG_TEAM_HOARD;
		}
	}
if (networkData.nStatus == NETSTAT_STARTING)
	return;
netGameInfo.m_info.nNumPlayers = N_PLAYERS;
netGameInfo.m_info.gameStatus = networkData.nStatus;
netGameInfo.m_info.nMaxPlayers = gameData.multiplayer.nMaxPlayers;
for (i = 0; i < MAX_NUM_NET_PLAYERS; i++) {
	memcpy (NETPLAYER (i).callsign, PLAYER (i).callsign, sizeof (NETPLAYER (i).callsign));
	NETPLAYER (i).connected = PLAYER (i).connected;
	for (j = 0; j < MAX_NUM_NET_PLAYERS; j++)
		*netGameInfo.Kills (i, j) = gameData.multigame.score.matrix [i][j];
	*netGameInfo.Killed (i) = PLAYER (i).netKilledTotal;
	*netGameInfo.PlayerKills (i) = PLAYER (i).netKillsTotal;
	*netGameInfo.PlayerScore (i) = PLAYER (i).score;
	*netGameInfo.PlayerFlags (i) = (PLAYER (i).flags & (PLAYER_FLAGS_BLUE_KEY | PLAYER_FLAGS_RED_KEY | PLAYER_FLAGS_GOLD_KEY));
	}
*netGameInfo.TeamKills (0) = gameData.multigame.score.nTeam [0];
*netGameInfo.TeamKills (1) = gameData.multigame.score.nTeam [1];
netGameInfo.m_info.SetLevel (missionManager.nCurrentLevel);
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

int32_t NetworkRequestSync (void) 
{
if ((networkData.nJoinState == 1) || (networkData.nJoinState == 2)) {
	if (networkData.syncInfo [0].objs.nFrame < 2) {
		networkData.syncInfo [0].objs.nFrame = 0;
		networkData.syncInfo [0].objs.nFramesToSkip = 0;
		//networkData.nJoinState = 0;
		}
	else {
		networkData.syncInfo [0].objs.nFramesToSkip = networkData.syncInfo [0].objs.nFrame;
		networkData.nJoinState = 2;
		}
	}
ResetSyncTimeout (); // make the join poll time out and send this request immediately 
NetworkFlush (); // Flush any old packets
return NetworkSendRequest ();
}

//------------------------------------------------------------------------------
// wait for sync packets from the game host after having sent join request

int32_t NetworkSyncPoll (CMenu& menu, int32_t& key, int32_t nCurItem, int32_t nState)
{
if (N_PLAYERS && IAmGameHost ()) {
	key = -3;
	return nCurItem;
	}
if (nState)
	return nCurItem;

NetworkListen ();

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
	if (NetworkRequestSync () < 0)
		key = -2;
	}
return nCurItem;
}

//------------------------------------------------------------------------------

int32_t NetworkWaitForSync (void)
{
	char					text [60];
	CMenu					m (2);
	int32_t				i, choice;
	tPlayerSyncData	me;

networkData.nStatus = NETSTAT_WAITING;
m.AddText ("", text);
m.AddText ("", const_cast<char*> (TXT_NET_LEAVE));
networkData.nJoinState = 0;
#if 1
i = NetworkSendRequest ();
if (i < 0) {
#if DBG
	NetworkSendRequest ();
#endif
	return -1;
	}
#endif
sprintf (m [0].m_text, "%s\n'%s' %s", TXT_NET_WAITING, NETPLAYER (i).callsign, TXT_NET_TO_ENTER);
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
	if (!downloadManager.DownloadMission (netGameInfo.m_info.szMissionName))
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
	me.player.network.SetNode (IpxGetMyLocalAddress ());
	me.player.network.SetNetwork (IpxGetMyServerAddress ());
	SendInternetPlayerSyncData (me, NETPLAYER (0).network.Network (), NETPLAYER (0).network.Node ());
}
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
networkData.toWaitAllPoll.Setup (WaitAllPollTimeout ());
networkData.toWaitAllPoll.Start ();
}

//------------------------------------------------------------------------------

int32_t NetworkWaitAllPoll (CMenu& menu, int32_t& key, int32_t nCurItem, int32_t nState)
{
if (nState)
	return nCurItem;

if (networkData.toWaitAllPoll.Expired ()) {
	NetworkSendAllInfoRequest (PID_SEND_ALL_GAMEINFO, networkData.nSecurityCheck);
	ResetWaitAllTimeout ();
	}
NetworkDoBigWait (networkData.bWaitAllChoice);  
if (networkData.nSecurityCheck == -1)
	key = -2;
return nCurItem;
}
 
//------------------------------------------------------------------------------

int32_t NetworkWaitForAllInfo (int32_t choice)
 {
  int32_t pick;
  
  CMenu m (2);

m.AddText ("", "Press Escape to cancel");
networkData.bWaitAllChoice = choice;
networkData.nStartWaitAllTime=TimerGetApproxSeconds ();
networkData.nSecurityCheck = activeNetGames [choice].m_info.nSecurity;
networkData.nSecurityFlag = 0;

networkData.toWaitAllPoll.Setup (WaitAllPollTimeout ());
networkData.toWaitAllPoll.Start (-1, true);
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

int32_t NetworkWaitForPlayerInfo (void)
{
	uint8_t					packet [MAX_PACKET_SIZE];
	CAllNetPlayersInfo	playerData;

#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
	CAllNetPlayersInfo info_struct;
#endif
if (!networkData.bWaitingForPlayerInfo)
	return 0;
if ((gameStates.multi.nGameType < IPX_GAME) || !networkData.bActive) 
	return 0;
#if 1			
if (!IsNetworkGame && (gameStates.app.nFunctionMode == FMODE_GAME))
	console.printf (CON_DBG, "Calling NetworkWaitForPlayerInfo () when not in net game.\n");
#endif	
if (networkData.nStatus == NETSTAT_PLAYING) {
	Int3 (); //MY GOD! Get Jason...this is the source of many problems
	return 0;
	}

uint32_t xTimeout = SDL_GetTicks () + 5000;
while (SDL_GetTicks () < xTimeout) {
	int32_t size = networkThread.GetPacketData (packet);
	uint8_t id = packet [0];
	if ((size > 0) && (id == PID_PLAYERSINFO)) {
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
		ReceiveNetPlayersPacket (packet, &playerData);
#else
		memcpy (&playerData.m_info, &packet [0], sizeof (playerData.m_info));
#endif
		if (networkData.nSecurityFlag == NETSECURITY_WAIT_FOR_PLAYERS) {
#if SECURITY_CHECK
			if (networkData.nSecurityNum != playerData.m_info.nSecurity) {
				G3_SLEEP (1);
				continue;
				}
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
	G3_SLEEP (1);
	}
return 0;
}

//------------------------------------------------------------------------------

void NetworkDoBigWait (int32_t choice)
{
	int32_t					size;
	uint8_t					packet [MAX_PACKET_SIZE], *data;
	CAllNetPlayersInfo	playerData;
  
while (0 < (size = networkThread.GetPacketData (packet))) {
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
					networkData.bHaveSync = 1;
					//NetworkProcessSyncPacket (&tempNetInfo, 0);
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

void NetworkSendLifeSign (void)
{
networkThread.SendLifeSign (true);
}

//------------------------------------------------------------------------------

int32_t NetworkRequestPoll (CMenu& menu, int32_t& key, int32_t nCurItem, int32_t nState)
{
if (!IAmGameHost ()) {
	key = -2;
	return nCurItem;
	}
if (nState)
	return nCurItem;

NetworkSendLifeSign ();
NetworkListen ();

int32_t nReady = 0;

for (int32_t i = 0; i < N_PLAYERS; i++) {
	if ((uint8_t) PLAYER (i).connected < 2)
		nReady++;
	}
if (nReady == N_PLAYERS) // All players have checked in or are disconnected
	key = -2;
return nCurItem;
}

//------------------------------------------------------------------------------

void NetworkWaitForRequests (void)
{
	// Wait for other players to load the level before we send the sync
	int32_t	choice, i;
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
		for (i = 0; i < N_PLAYERS; i++) {
			if (PLAYER (i).IsConnected () && (i != N_LOCALPLAYER)) {
				if (gameStates.multi.nGameType >= IPX_GAME)
					NetworkDumpPlayer (NETPLAYER (i).network.Network (), NETPLAYER (i).network.Node (), DUMP_ABORTED);
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
int32_t NetworkLevelSync (void)
{
	int32_t result;
	networkData.bSyncPackInited = 0;

//networkThread.SetListen (false);
bool bSuspend = networkThread.Resume ();
NetworkFlush (); // Flush any old packets
networkData.bHaveSync = 0;
for (;;) {
	if (N_PLAYERS && IAmGameHost ()) {
		NetworkWaitForRequests ();
		if (IAmGameHost ()) {
			NetworkSendSync ();
			result = (N_LOCALPLAYER < 0) ? -1 : 0;
			break;
			}
		}
	else {
		result = NetworkWaitForSync ();
		if (!(N_PLAYERS && IAmGameHost ()))
			break;
		}
	}
if (bSuspend)
	networkThread.Suspend ();
if (result < 0) {
	NetworkLeaveGame (false);
	NetworkSendEndLevelPacket ();
	//longjmp (gameExitPoint, 0);
	}
else
	NetworkCountPowerupsInMine ();
//networkThread.SetListen (true);
return result;
}

//------------------------------------------------------------------------------
