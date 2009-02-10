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
#include "network.h"
#include "network_lib.h"
#include "netmisc.h"
#include "autodl.h"
#include "tracker.h"
#include "monsterball.h"
#include "text.h"

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
		console.printf (CON_DBG, "Aborting resync for CPlayerData %s.\n", their->player.callsign);
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
	nDbgObjType = nDbgObjType;
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

ubyte objBuf [MAX_PACKETSIZE];

void NetworkSyncObjects (tNetworkSyncData *syncP)
{
	CObject	*objP;
	sbyte		owner;
	short		nRemoteObj;
	int		bufI, i, h;
	int		nObjFrames = 0;
	int		nPlayer = syncP->player [1].player.connected;

// Send clear OBJECTS array CTrigger and send CPlayerData num
objFilter [OBJ_MARKER] = !gameStates.app.bHaveExtraGameInfo [1];
for (h = 0; h < OBJ_PACKETS_PER_FRAME; h++) {	// Do more than 1 per frame, try to speed it up without
																// over-stressing the receiver.
	nObjFrames = 0;
	memset (objBuf, 0, DATALIMIT);
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
		syncP->objs.nFrame = 0;
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
		if ((DATALIMIT - bufI - 1) < int (sizeof (tBaseObject)) + 5)
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
			Assert (bufI <= DATALIMIT);
			if (gameStates.multi.nGameType >= IPX_GAME)
				IPXSendInternetPacketData (
					objBuf, bufI, 
					syncP->player [1].player.network.ipx.server, 
					syncP->player [1].player.network.ipx.node);
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
										syncP->player [1].player.network.ipx.server, 
										syncP->player [1].player.network.ipx.node);
// Send sync packet which tells the CPlayerData who he is and to start!
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
if (!NetworkIAmMaster ()) {
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
	if (gameData.app.nGameMode & GM_MULTI_ROBOTS)
		MultiSendStolenItems ();
	}
else if (syncP->nExtras == 5) {
	if (netGame.xPlayTimeAllowed || netGame.KillGoal)
		MultiSendKillGoalCounts ();
	}
else if (syncP->nExtras == 6)
	NetworkSendSmashedLights (syncP->nExtrasPlayer);
else if (syncP->nExtras == 7)
	NetworkSendPlayerFlags ();    
else if (syncP->nExtras == 8)
	MultiSendPowerupUpdate ();  
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
	time_t	t = (time_t) SDL_GetTicks ();

if (t < syncP->timeout)
	return;
syncP->timeout = t + (/*(gameStates.multi.nGameType == UDP_GAME) ? 200 :*/ 2000 / PacketsPerSec ());
if (syncP->bExtraGameInfo) {
	NetworkSendExtraGameInfo (&syncP->player [0]);
	syncP->bExtraGameInfo = 0;
	}
else if (syncP->nState == 1) {
	syncP->objs.missingFrames.nFrame = 0;
	NetworkSyncObjects (syncP);
	syncP->bExtraGameInfo = 1;
	}
else if (syncP->nState == 2) {
	NetworkSyncPlayer (syncP);
	syncP->bExtraGameInfo = 1;
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
			DeleteSyncData (syncP - networkData.sync);
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

netGame.nConnected = 0;
for (i = 0; i < gameData.multiplayer.nPlayers; i++)
	if (gameData.multiplayer.players [i].connected)
		netGame.nConnected++;

// This is great: D2 1.0 and 1.1 ignore upper part of the gameFlags field of
//	the tLiteInfo struct when you're sitting on the join netgame screen.  We can
//	"sneak" Hoard information into this field.  This is better than sending 
//	another packet that could be lost in transit.
if (HoardEquipped ()) {
	if (gameData.app.nGameMode & GM_MONSTERBALL)
		netGame.gameFlags |= NETGAME_FLAG_MONSTERBALL;
	else if (gameData.app.nGameMode & GM_ENTROPY)
		netGame.gameFlags |= NETGAME_FLAG_ENTROPY;
	else if (gameData.app.nGameMode & GM_HOARD) {
		netGame.gameFlags |= NETGAME_FLAG_HOARD;
		if (gameData.app.nGameMode & GM_TEAM)
			netGame.gameFlags |= NETGAME_FLAG_TEAM_HOARD;
		}
	}
if (networkData.nStatus == NETSTAT_STARTING)
	return;
netGame.nNumPlayers = gameData.multiplayer.nPlayers;
netGame.gameStatus = networkData.nStatus;
netGame.nMaxPlayers = gameData.multiplayer.nMaxPlayers;
for (i = 0; i < MAX_NUM_NET_PLAYERS; i++) {
	netPlayers.players [i].connected = gameData.multiplayer.players [i].connected;
	for (j = 0; j < MAX_NUM_NET_PLAYERS; j++)
		netGame.kills [i][j] = gameData.multigame.kills.matrix [i][j];
	netGame.killed [i] = gameData.multiplayer.players [i].netKilledTotal;
	netGame.playerKills [i] = gameData.multiplayer.players [i].netKillsTotal;
	netGame.playerScore [i] = gameData.multiplayer.players [i].score;
	netGame.playerFlags [i] = (gameData.multiplayer.players [i].flags & (PLAYER_FLAGS_BLUE_KEY | PLAYER_FLAGS_RED_KEY | PLAYER_FLAGS_GOLD_KEY));
	}
netGame.teamKills [0] = gameData.multigame.kills.nTeam [0];
netGame.teamKills [1] = gameData.multigame.kills.nTeam [1];
netGame.nLevel = gameData.missions.nCurrentLevel;
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

static inline void ResetSyncTimeout (void)
{
networkData.toSyncPoll = SDL_GetTicks () + SyncPollTimeout ();
}

//------------------------------------------------------------------------------

void NetworkPackObjects (void)
{
// Switching modes, pack the CObject array
SpecialResetObjects ();
}                               

//------------------------------------------------------------------------------
/* Polling loop waiting for sync packet to start game
 * after having sent request
 */
int NetworkSyncPoll (CMenu& menu, int& key, int nCurItem, int nState)
{
if (nState)
	return nCurItem;

	int	nPackets = NetworkListen ();

if (networkData.nStatus != NETSTAT_WAITING) { // Status changed to playing, exit the menu
	if (NetworkVerifyPlayers ())
		NetworkAbortSync ();
	key = -2;
	return nCurItem;
	}
#if 1 //ndef _DEBUG
if (nPackets || (networkData.nStatus == NETSTAT_PLAYING)) {
	ResetSyncTimeout ();
	return nCurItem;
	}
#endif
if ((time_t) SDL_GetTicks () > networkData.toSyncPoll) {	// Poll time expired, re-send request
#if 1			
	console.printf (CON_DBG, "Re-sending join request.\n");
#endif
#if DBG
	audio.PlaySound (SOUND_HUD_MESSAGE, SOUNDCLASS_GENERIC, I2X (1) / 2);
#endif
	ResetSyncTimeout ();
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
m.AddText (text);
m.AddText (const_cast<char*> (TXT_NET_LEAVE));
networkData.nJoinState = 0;
i = NetworkSendRequest ();
if (i < 0) {
#if DBG
	NetworkSendRequest ();
#endif
	return -1;
	}
sprintf (m [0].m_text, "%s\n'%s' %s", TXT_NET_WAITING, netPlayers.players [i].callsign, TXT_NET_TO_ENTER);
networkData.toSyncPoll = 0;
do {
	choice = m.Menu (NULL, TXT_WAIT, NetworkSyncPoll);
	} while (choice > -1);
if (networkData.nStatus == NETSTAT_PLAYING)  
	return 0;
else if (networkData.nStatus == NETSTAT_AUTODL)
	if (downloadManager.DownloadMission (netGame.szMissionName))
		return 1;
#if 1			
console.printf (CON_DBG, "Aborting join.\n");
#endif
me.nType = PID_QUIT_JOINING;
memcpy (me.player.callsign, LOCALPLAYER.callsign, CALLSIGN_LEN+1);
if (gameStates.multi.nGameType >= IPX_GAME) {
	memcpy (me.player.network.ipx.node, IpxGetMyLocalAddress (), 6);
	memcpy (me.player.network.ipx.server, IpxGetMyServerAddress (), 4);
	SendInternetSequencePacket (me, netPlayers.players [0].network.ipx.server, 
										 netPlayers.players [0].network.ipx.node);
}
gameData.multiplayer.nPlayers = 0;
SetFunctionMode (FMODE_MENU);
gameData.app.nGameMode = GM_GAME_OVER;
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

m.AddText ("Press Escape to cancel");
networkData.bWaitAllChoice = choice;
networkData.nStartWaitAllTime=TimerGetApproxSeconds ();
networkData.nSecurityCheck = activeNetGames [choice].nSecurity;
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
	ubyte						packet [MAX_PACKETSIZE];
	tAllNetPlayersInfo	*tempPlayerP;
	uint			xTimeout;
	ubyte id = 0;

#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
	tAllNetPlayersInfo info_struct;
#endif

if (gameStates.multi.nGameType >= IPX_GAME)
	if (!networkData.bActive) 
		return 0;
#if 1			
if (!(gameData.app.nGameMode & GM_NETWORK) && (gameStates.app.nFunctionMode == FMODE_GAME))
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
		ReceiveNetPlayersPacket (packet, &info_struct);
		tempPlayerP = &info_struct;
#else
		tempPlayerP = reinterpret_cast<tAllNetPlayersInfo*> (packet);
#endif
		retries++;
		if (networkData.nSecurityFlag == NETSECURITY_WAIT_FOR_PLAYERS) {
#if SECURITY_CHECK
			if (networkData.nSecurityNum != tempPlayerP->nSecurity)
				continue;
#endif
			memcpy (&tmpPlayersBase, reinterpret_cast<ubyte*> (tempPlayerP), sizeof (tAllNetPlayersInfo));
			tmpPlayersInfo = &tmpPlayersBase;
			networkData.nSecurityFlag = NETSECURITY_OFF;
			networkData.nSecurityNum = 0;
			networkData.bWaitingForPlayerInfo = 0;
			return 1;
			}
		else {
			networkData.nSecurityNum = tempPlayerP->nSecurity;
			networkData.nSecurityFlag = NETSECURITY_WAIT_FOR_GAMEINFO;
			memcpy (&tmpPlayersBase, reinterpret_cast<ubyte*> (tempPlayerP), sizeof (tAllNetPlayersInfo));
			tmpPlayersInfo = &tmpPlayersBase;
			networkData.bWaitingForPlayerInfo = 0;
			return 1;
			}
		}
	}
return 0;
}

//------------------------------------------------------------------------------

void NetworkDoBigWait (int choice)
{
	int						size;
	ubyte						packet [MAX_PACKETSIZE], *data;
	tAllNetPlayersInfo	*tempPlayerP;
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
	tAllNetPlayersInfo	tempPlayer;
#endif
  
while (0 < (size = IpxGetPacketData (packet))) {
	data = &packet [0];

	switch (data [0]) {  
		case PID_GAME_INFO:
			if (gameStates.multi.nGameType >= IPX_GAME)
				ReceiveFullNetGamePacket (data, &tempNetInfo); 
			else
				memcpy (reinterpret_cast<ubyte*> (&tempNetInfo), data, sizeof (tNetgameInfo));
#if SECURITY_CHECK
			if (tempNetInfo.nSecurity != networkData.nSecurityCheck)
				break;
#endif
			if (networkData.nSecurityFlag == NETSECURITY_WAIT_FOR_GAMEINFO) {
#if SECURITY_CHECK
				if (tmpPlayersInfo->nSecurity == tempNetInfo.nSecurity) {
#else
				 {
#endif
#if SECURITY_CHECK
					if (tmpPlayersInfo->nSecurity == networkData.nSecurityCheck) {
#else
					 {
#endif
						memcpy (&activeNetGames + choice, reinterpret_cast<ubyte*> (&tempNetInfo), sizeof (tNetgameInfo));
						memcpy (activeNetPlayers + choice, tmpPlayersInfo, sizeof (tAllNetPlayersInfo));
						networkData.nSecurityCheck = -1;
						}
					}
				}
			else {
				networkData.nSecurityFlag = NETSECURITY_WAIT_FOR_PLAYERS;
				networkData.nSecurityNum = tempNetInfo.nSecurity;
				if (NetworkWaitForPlayerInfo ()) {
#if 1			
					console.printf (CON_DBG, "HUH? Game=%d Player=%d\n", 
									networkData.nSecurityNum, tmpPlayersInfo->nSecurity);
#endif
					memcpy (activeNetGames + choice, reinterpret_cast<ubyte*> (&tempNetInfo), sizeof (tNetgameInfo));
					memcpy (activeNetPlayers + choice, tmpPlayersInfo, sizeof (tAllNetPlayersInfo));
					networkData.nSecurityCheck = -1;
					}
				networkData.nSecurityFlag = 0;
				networkData.nSecurityNum = 0;
				}
			break;

		case PID_EXTRA_GAMEINFO: 
			if (gameStates.multi.nGameType >= IPX_GAME) {
				NetworkProcessExtraGameInfo (data);
				}
			break;

		case PID_PLAYERSINFO:
			if (gameStates.multi.nGameType < IPX_GAME)
				tempPlayerP = reinterpret_cast<tAllNetPlayersInfo*> (data);
			else {
#if !(defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__))
				tempPlayerP = reinterpret_cast<tAllNetPlayersInfo*> (data);
#else
				ReceiveNetPlayersPacket (data, &tempPlayer);
				tempPlayerP = &tempPlayer;
#endif
				}
#if SECURITY_CHECK
			if (tempPlayerP->nSecurity != networkData.nSecurityCheck) 
				break;     // If this isn't the guy we're looking for, move on
#endif
			memcpy (&tmpPlayersBase, tempPlayerP, sizeof (tAllNetPlayersInfo));
			tmpPlayersInfo = &tmpPlayersBase;
			networkData.bWaitingForPlayerInfo = 0;
			networkData.nSecurityNum = tmpPlayersBase.nSecurity;
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
if (nState)
	return nCurItem;

	int i = 0;
	int nReady = 0;

NetworkListen ();
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
m.AddText (const_cast<char*> (TXT_NET_LEAVE));
NetworkFlush ();
LOCALPLAYER.connected = 1;

for (;;) {
	choice = m.Menu (NULL, TXT_WAIT, NetworkRequestPoll);        
	if (choice == -1) {
		// User aborted
		choice = MsgBox (NULL, NULL, 3, TXT_YES, TXT_NO, TXT_START_NOWAIT, TXT_QUITTING_NOW);
		if (choice == 2)
			return;
		if (choice != 0)
			continue;
		
		// User confirmed abort
		for (i = 0; i < gameData.multiplayer.nPlayers; i++) {
			if ((gameData.multiplayer.players [i].connected != 0) && (i != gameData.multiplayer.nLocalPlayer)) {
				if (gameStates.multi.nGameType >= IPX_GAME)
					NetworkDumpPlayer (netPlayers.players [i].network.ipx.server, netPlayers.players [i].network.ipx.node, DUMP_ABORTED);
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
if (!gameData.multiplayer.nPlayers)
	result = NetworkWaitForSync ();
else if (NetworkIAmMaster ()) {
	NetworkWaitForRequests ();
	NetworkSendSync ();
	result = (gameData.multiplayer.nLocalPlayer < 0) ? -1 : 0;
	}
else
	result = NetworkWaitForSync ();
if (result < 0) {
	LOCALPLAYER.connected = 0;
	NetworkSendEndLevelPacket ();
	longjmp (gameExitPoint, 0);
	}
NetworkCountPowerupsInMine ();
return result;
}

//------------------------------------------------------------------------------

