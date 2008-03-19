/* $Id: network.c, v 1.24 2003/10/12 09:38:48 btb Exp $ */
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
#include <conf.h>
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
#ifndef _WIN32
#	include <arpa/inet.h>
#	include <netinet/in.h> /* for htons & co. */
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
#include "objsmoke.h"
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
#include "reactor.h"
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
#include "autodl.h"
#include "tracker.h"
#include "newmenu.h"
#include "gamefont.h"
#include "gameseg.h"
#include "hudmsg.h"
#include "vers_id.h"
#include "netmenu.h"
#include "banlist.h"
#include "collide.h"
#include "ipx.h"
#ifdef _WIN32
#	include "win32/include/ipx_udp.h"
#	include "win32/include/ipx_drv.h"
#else
#	include "linux/include/ipx_udp.h"
#	include "linux/include/ipx_drv.h"
#endif

#ifdef _DEBUG
#	define OBJ_PACKETS_PER_FRAME 1
#else
#	define OBJ_PACKETS_PER_FRAME 1
#endif

//------------------------------------------------------------------------------

void NetworkStopResync (tSequencePacket *their)
{
if (!CmpNetPlayers (networkData.playerRejoining.player.callsign, their->player.callsign, 
						  &networkData.playerRejoining.player.network, &their->player.network)) {
#if 1      
	con_printf (CONDBG, "Aborting resync for tPlayer %s.\n", their->player.callsign);
#endif
	networkData.nSyncState = 0;
	networkData.nSyncExtras = 0;
	networkData.nJoinState = 0;
	networkData.nPlayerJoiningExtras = -1;
	networkData.nSentObjs = -1;
	}
}

//------------------------------------------------------------------------------

static int objFilter [] = {1, 1, 0, 0, 0, 0, 1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1};

static inline int NetworkFilterObject (tObject *objP)
{
	short t = objP->nType;
#ifdef _DEBUG
if (t == nDbgObjType)
	nDbgObjType = nDbgObjType;
#endif
if (t >= MAX_OBJECT_TYPES)
	return 1;
if (objFilter [t])
	return 1;
if ((t == OBJ_WEAPON) && (objP->id != SMALLMINE_ID))
	return 1;
return 0;
}

//------------------------------------------------------------------------------

static inline int NetworkObjFrameFilter (void)
{
if (!networkData.nSyncFrame++)
	return 1;
if (networkData.nSyncFrame < networkData.missingObjFrames.nFrame)
	return 0;
return 1;
}

//------------------------------------------------------------------------------

ubyte objBuf [MAX_PACKETSIZE];

void NetworkSyncObjects (void)
{
	tObject	*objP;
	sbyte		owner;
	short		nRemoteObj;
	int		bufI, i, h;
	int		nObjFrames = 0;
	int		nPlayer = networkData.playerRejoining.player.connected;

// Send clear gameData.objs.objects array tTrigger and send tPlayer num
Assert (networkData.nSyncState != 0);
Assert (nPlayer >= 0);
Assert (nPlayer < gameData.multiplayer.nMaxPlayers);

objFilter [OBJ_MARKER] = !gameStates.app.bHaveExtraGameInfo [1];
for (h = 0; h < OBJ_PACKETS_PER_FRAME; h++) {	// Do more than 1 per frame, try to speed it up without
																// over-stressing the receiver.
	nObjFrames = 0;
	memset (objBuf, 0, DATALIMIT);
	objBuf [0] = PID_OBJECT_DATA;
	bufI = (gameStates.multi.nGameType == UDP_GAME) ? 4 : 3;

	if (networkData.nSentObjs == -1) {	// first packet tells the receiver to reset it's object data
		networkData.nSyncObjs = 0;
		networkData.bSendObjectMode = 0;
		networkData.nSyncFrame = 0;
		NW_SET_SHORT (objBuf, bufI, networkData.missingObjFrames.nFrame ? -3 : -1);		// object number -1          
		NW_SET_BYTE (objBuf, bufI, nPlayer);                            
		bufI += 2;									// Placeholder for nRemoteObj, not used here
		networkData.nSentObjs = 0;
		networkData.nSyncFrame = 0;
		nObjFrames = 1;		// first frame contains "reset object data" info
		}

	for (i = networkData.nSentObjs, objP = OBJECTS + i; i <= gameData.objs.nLastObject; i++, objP++) {
		if (NetworkFilterObject (objP))
			continue;
		if (networkData.bSendObjectMode) { 
			 if ((gameData.multigame.nObjOwner [i] != -1) && (gameData.multigame.nObjOwner [i] != nPlayer))
				continue;
			}
		else {
			if ((gameData.multigame.nObjOwner [i] == -1) || (gameData.multigame.nObjOwner [i] == nPlayer))
				continue;
			}
		if ((DATALIMIT - bufI - 1) < (sizeof (tObject) + 5))
			break; // Not enough room for another tObject
		nObjFrames++;
		networkData.nSyncObjs++;
		nRemoteObj = ObjnumLocalToRemote ((short) i, &owner);
		Assert (owner == gameData.multigame.nObjOwner [i]);
		NW_SET_SHORT (objBuf, bufI, i);      
		NW_SET_BYTE (objBuf, bufI, owner);                                 
		NW_SET_SHORT (objBuf, bufI, nRemoteObj); 
		NW_SET_BYTES (objBuf, bufI, objP, sizeof (tObject));
		if (gameStates.multi.nGameType >= IPX_GAME)
			SwapObject ((tObject *) (objBuf + bufI - sizeof (tObject)));
		}
	if (nObjFrames) {	// Send any objects we've buffered
		networkData.nSentObjs = i;	
		if (NetworkObjFrameFilter ()) {
			objBuf [1] = nObjFrames;  
			if (gameStates.multi.nGameType == UDP_GAME)
				*((short *) (objBuf + 2)) = INTEL_SHORT (networkData.nSyncFrame);
			else
				objBuf [2] = (ubyte) networkData.nSyncFrame;
			Assert (bufI <= DATALIMIT);
			if (gameStates.multi.nGameType >= IPX_GAME)
				IPXSendInternetPacketData (
					objBuf, bufI, 
					networkData.playerRejoining.player.network.ipx.server, 
					networkData.playerRejoining.player.network.ipx.node);
			 }
		}
	if (i > gameData.objs.nLastObject) {
		if (networkData.bSendObjectMode) {
			networkData.nSentObjs = i;
			// Send count so other tSide can make sure he got them all
			objBuf [0] = PID_OBJECT_DATA;
			objBuf [1] = 1;
			networkData.nSyncFrame++;
			if (gameStates.multi.nGameType == UDP_GAME) {
				bufI = 2;
				NW_SET_SHORT (objBuf, bufI, networkData.nSyncFrame); 
				}
			else {
				objBuf [2] = (ubyte) networkData.nSyncFrame;
				bufI = 3;
				}
			nRemoteObj = networkData.missingObjFrames.nFrame ? -4 : -2;
			NW_SET_SHORT (objBuf, bufI, nRemoteObj);
			NW_SET_SHORT (objBuf, bufI, networkData.nSyncObjs);
			networkData.nSyncState = networkData.missingObjFrames.nFrame ? 0 : 2;
			}
		else {
			networkData.nSentObjs = 0;
			networkData.bSendObjectMode = 1; // go to next mode
			}
		break;
		}
	}
}

//------------------------------------------------------------------------------

void NetworkSyncPlayer (void)
{
	int nPlayer = networkData.playerRejoining.player.connected;

//OLD IPXSendPacketData (objBuf, 8, &networkData.playerRejoining.player.node);
if (gameStates.multi.nGameType >= IPX_GAME)
	IPXSendInternetPacketData (objBuf, 8, 
										networkData.playerRejoining.player.network.ipx.server, 
										networkData.playerRejoining.player.network.ipx.node);
// Send sync packet which tells the tPlayer who he is and to start!
NetworkSendRejoinSync (nPlayer);
networkData.bVerifyPlayerJoined = nPlayer;

// Turn off send tObject mode
networkData.nSentObjs = -1;
networkData.nSyncState = 3;
networkData.nSyncObjs = 0;
networkData.nSyncExtras = 1; // start to send extras
networkData.nPlayerJoiningExtras = nPlayer;
return;
}

//------------------------------------------------------------------------------

void NetworkSyncExtras (void)
{
Assert (networkData.nPlayerJoiningExtras >- 1);
if (!NetworkIAmMaster ()) {
#if 1			
  con_printf (CONDBG, "Hey! I'm not the master and I was gonna send info!\n");
#endif
	}
if (networkData.nSyncExtras == 1)
	NetworkSendFlyThruTriggers (networkData.nPlayerJoiningExtras);
else if (networkData.nSyncExtras == 2)
	NetworkSendDoorUpdates (networkData.nPlayerJoiningExtras);
else if (networkData.nSyncExtras == 3)
	NetworkSendMarkers ();
else if (networkData.nSyncExtras == 4) {
	if (gameData.app.nGameMode & GM_MULTI_ROBOTS)
		MultiSendStolenItems ();
	}
else if (networkData.nSyncExtras == 5) {
	if (netGame.xPlayTimeAllowed || netGame.KillGoal)
		MultiSendKillGoalCounts ();
	}
else if (networkData.nSyncExtras == 6)
	NetworkSendSmashedLights (networkData.nPlayerJoiningExtras);
else if (networkData.nSyncExtras == 7)
	NetworkSendPlayerFlags ();    
else if (networkData.nSyncExtras == 8)
	MultiSendPowerupUpdate ();  
else {
	networkData.nSyncExtras = 0;
	networkData.nSyncState = 0;
	networkData.nPlayerJoiningExtras = -1;
	memset (&networkData.playerRejoining, 0, sizeof (networkData.playerRejoining));
	return;
	}
networkData.nSyncExtras++;
}

//------------------------------------------------------------------------------

void NetworkDoSyncFrame (void)
{
	time_t	t = (time_t) SDL_GetTicks ();

if (t < networkData.toSyncFrame)
	return;
networkData.toSyncFrame = t + ((gameStates.multi.nGameType == UDP_GAME) ? 200 : 1000 / PacketsPerSec ());
if (networkData.bSyncExtraGameInfo) {
	NetworkSendExtraGameInfo (&networkData.joinSeq);
	networkData.bSyncExtraGameInfo = 0;
	}
else if (networkData.nSyncState == 1) {
	networkData.missingObjFrames.nFrame = 0;
	NetworkSyncObjects ();
	networkData.bSyncExtraGameInfo = 1;
	}
else if (networkData.nSyncState == 2) {
	NetworkSyncPlayer ();
	networkData.bSyncExtraGameInfo = 1;
	}
else if (networkData.nSyncState == 3) {
	if (networkData.missingObjFrames.nFrame) {
		NetworkSyncObjects ();
		if (!networkData.nSyncState)
			networkData.missingObjFrames.nFrame = 0;
		}
	else
		networkData.nSyncState = 0;
	}
else {
	if (networkData.nSyncExtras && (networkData.bVerifyPlayerJoined == -1))
		NetworkSyncExtras ();
		networkData.bSyncExtraGameInfo = (networkData.nSyncExtras == 0);
	}
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
	netGame.player_score [i] = gameData.multiplayer.players [i].score;
	netGame.playerFlags [i] = (gameData.multiplayer.players [i].flags & (PLAYER_FLAGS_BLUE_KEY | PLAYER_FLAGS_RED_KEY | PLAYER_FLAGS_GOLD_KEY));
	}
netGame.teamKills [0] = gameData.multigame.kills.nTeam [0];
netGame.teamKills [1] = gameData.multigame.kills.nTeam [1];
netGame.nLevel = gameData.missions.nCurrentLevel;
}

//------------------------------------------------------------------------------

static inline fix SyncPollTimeout (void)
{
#ifdef _DEBUG
return 5000;
#else
return 2000;
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
// Switching modes, pack the tObject array
SpecialResetObjects ();
}                               

//------------------------------------------------------------------------------
/* Polling loop waiting for sync packet to start game
 * after having sent request
 */
void NetworkSyncPoll (int nitems, tMenuItem * menus, int * key, int citem)
{
	int	nPackets = NetworkListen ();

if (networkData.nStatus != NETSTAT_WAITING) { // Status changed to playing, exit the menu
	if (NetworkVerifyPlayers ())
		NetworkAbortSync ();
	*key = -2;
	return;
	}
#if 1 //ndef _DEBUG
if (nPackets || (networkData.nJoinState >= 4)) {
	if (networkData.nJoinState)
		ResetSyncTimeout ();
	return;
	}
#endif
if ((time_t) SDL_GetTicks () > networkData.toSyncPoll) {	// Poll time expired, re-send request
#if 1			
	con_printf (CONDBG, "Re-sending join request.\n");
#endif
#ifdef _DEBUG
	DigiPlaySample (SOUND_HUD_MESSAGE, F1_0 / 2);
#endif
	ResetSyncTimeout ();
	if (NetworkSendRequest () < 0)
		*key = -2;
	}
}

//------------------------------------------------------------------------------

int NetworkWaitForSync (void)
{
	char					text [60];
	tMenuItem			m [2];
	int					i, choice;
	tSequencePacket	me;

networkData.nStatus = NETSTAT_WAITING;
memset (m, 0, sizeof (m));
m [0].nType = NM_TYPE_TEXT; 
m [0].text = text;
m [1].nType = NM_TYPE_TEXT; 
m [1].text = TXT_NET_LEAVE;
networkData.nJoinState = 0;
i = NetworkSendRequest ();
if (i < 0) {
#ifdef _DEBUG
	NetworkSendRequest ();
#endif
	return -1;
	}
sprintf (m [0].text, "%s\n'%s' %s", TXT_NET_WAITING, netPlayers.players [i].callsign, TXT_NET_TO_ENTER);
networkData.toSyncPoll = 0;
do {
	choice = ExecMenu (NULL, TXT_WAIT, 2, m, NetworkSyncPoll, NULL);
	} while (choice > -1);
if (networkData.nStatus == NETSTAT_PLAYING)  
	return 0;
else if (networkData.nStatus == NETSTAT_AUTODL)
	if (DownloadMission (netGame.szMissionName))
		return 1;
#if 1			
con_printf (CONDBG, "Aborting join.\n");
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
#ifdef _DEBUG
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

void NetworkWaitAllPoll (int nitems, tMenuItem * menus, int * key, int citem)
{
if ((time_t) SDL_GetTicks () > networkData.toWaitAllPoll) {
	NetworkSendAllInfoRequest (PID_SEND_ALL_GAMEINFO, networkData.nSecurityCheck);
	ResetWaitAllTimeout ();
	}
NetworkDoBigWait (networkData.bWaitAllChoice);  
if (networkData.nSecurityCheck == -1)
	*key = -2;
}
 
//------------------------------------------------------------------------------

int NetworkWaitForAllInfo (int choice)
 {
  int pick;
  
  tMenuItem m [2];

memset (m, 0, sizeof (m));
m [0].nType = NM_TYPE_TEXT; 
m [0].text = "Press Escape to cancel";
networkData.bWaitAllChoice = choice;
networkData.nStartWaitAllTime=TimerGetApproxSeconds ();
networkData.nSecurityCheck = activeNetGames [choice].nSecurity;
networkData.nSecurityFlag = 0;

networkData.toWaitAllPoll = 0;
do {
	pick = ExecMenu (NULL, TXT_CONNECTING, 1, m, NetworkWaitAllPoll, NULL);
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
	fix						xTimeout;
	ubyte id = 0;

#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
	tAllNetPlayersInfo info_struct;
#endif

if (gameStates.multi.nGameType >= IPX_GAME)
	if (!networkData.bActive) 
		return 0;
#if 1			
if (!(gameData.app.nGameMode & GM_NETWORK) && (gameStates.app.nFunctionMode == FMODE_GAME))
	con_printf (CONDBG, "Calling NetworkWaitForPlayerInfo () when not in net game.\n");
#endif	
if (networkData.nStatus == NETSTAT_PLAYING) {
	Int3 (); //MY GOD! Get Jason...this is the source of many problems
	return 0;
	}
xTimeout = TimerGetApproxSeconds () * F1_0 * 5;
while (networkData.bWaitingForPlayerInfo && (retries < 50) && 
	    (TimerGetApproxSeconds () < xTimeout)) {
	if (gameStates.multi.nGameType >= IPX_GAME) {
		size = IpxGetPacketData (packet);
		id = packet [0];
		}
	if ((size > 0) && (id == PID_PLAYERSINFO)) {
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
		ReceiveNetPlayersPacket (packet, &info_struct);
		tempPlayerP = &info_struct;
#else
		tempPlayerP = (tAllNetPlayersInfo *)packet;
#endif
		retries++;
		if (networkData.nSecurityFlag == NETSECURITY_WAIT_FOR_PLAYERS) {
#if SECURITY_CHECK
			if (networkData.nSecurityNum != tempPlayerP->nSecurity)
				continue;
#endif
			memcpy (&tmpPlayersBase, (ubyte *) tempPlayerP, sizeof (tAllNetPlayersInfo));
			tmpPlayersInfo = &tmpPlayersBase;
			networkData.nSecurityFlag = NETSECURITY_OFF;
			networkData.nSecurityNum = 0;
			networkData.bWaitingForPlayerInfo = 0;
			return 1;
			}
		else {
			networkData.nSecurityNum = tempPlayerP->nSecurity;
			networkData.nSecurityFlag=NETSECURITY_WAIT_FOR_GAMEINFO;
			memcpy (&tmpPlayersBase, (ubyte *)tempPlayerP, sizeof (tAllNetPlayersInfo));
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
				memcpy ((ubyte *) &tempNetInfo, data, sizeof (tNetgameInfo));
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
						memcpy (&activeNetGames + choice, (ubyte *) &tempNetInfo, sizeof (tNetgameInfo));
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
					con_printf (CONDBG, "HUH? Game=%d Player=%d\n", 
									networkData.nSecurityNum, tmpPlayersInfo->nSecurity);
#endif
					memcpy (activeNetGames + choice, (ubyte *)&tempNetInfo, sizeof (tNetgameInfo));
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
				tempPlayerP = (tAllNetPlayersInfo *) data;
			else {
#if !(defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__))
				tempPlayerP = (tAllNetPlayersInfo *) data;
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

void NetworkRequestPoll (int nitems, tMenuItem * menus, int * key, int citem)
{
	// Polling loop for waiting-for-requests menu

	int i = 0;
	int nReady = 0;

NetworkListen ();
for (i = 0; i < gameData.multiplayer.nPlayers; i++) {
	if ((ubyte) gameData.multiplayer.players [i].connected < 2)
		nReady++;
	}
if (nReady == gameData.multiplayer.nPlayers) // All players have checked in or are disconnected
	*key = -2;
}

//------------------------------------------------------------------------------

void NetworkWaitForRequests (void)
{
	// Wait for other players to load the level before we send the sync
	int choice, i;
	tMenuItem m [1];

networkData.nStatus = NETSTAT_WAITING;
memset (m, 0, sizeof (m));
m [0].nType= NM_TYPE_TEXT; 
m [0].text = TXT_NET_LEAVE;
NetworkFlush ();
LOCALPLAYER.connected = 1;

do_menu:

choice = ExecMenu (NULL, TXT_WAIT, 1, m, NetworkRequestPoll, NULL);        
if (choice == -1) {
	// User aborted
	choice = ExecMessageBox (NULL, NULL, 3, TXT_YES, TXT_NO, TXT_START_NOWAIT, TXT_QUITTING_NOW);
	if (choice == 2)
		return;
	if (choice != 0)
		goto do_menu;
	
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
	goto do_menu;
}

//------------------------------------------------------------------------------

/* Do required syncing after each level, before starting new one */
int NetworkLevelSync (void)
{
	int result;
	networkData.mySyncPackInited = 0;

//networkData.nMySegsCheckSum = NetMiscCalcCheckSum (gameData.segs.segments, sizeof (tSegment)* (gameData.segs.nLastSegment+1);
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

