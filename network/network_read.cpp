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
#include "objeffects.h"
#include "network.h"
#include "network_lib.h"
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

//------------------------------------------------------------------------------

void NetworkReadEndLevelPacket (ubyte *dataP)
{
	// Special packet for end of level syncing
	int nPlayer;
	tEndLevelInfo *end = (tEndLevelInfo *)dataP;
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
	int i, j;

for (i = 0; i < MAX_PLAYERS; i++)
	for (j = 0; j < MAX_PLAYERS; j++)
		end->killMatrix [i][j] = INTEL_SHORT (end->killMatrix [i][j]);
end->kills = INTEL_SHORT (end->kills);
end->killed = INTEL_SHORT (end->killed);
#endif
nPlayer = end->nPlayer;
Assert (nPlayer != gameData.multiplayer.nLocalPlayer);
if (nPlayer>=gameData.multiplayer.nPlayers) {              
	Int3 (); // weird, but it an happen in a coop restore game
	return; // if it happens in a coop restore, don't worry about it
	}
if ((networkData.nStatus == NETSTAT_PLAYING) && (end->connected != 0))
	return; // Only accept disconnect packets if we're not out of the level yet
gameData.multiplayer.players [nPlayer].connected = end->connected;
memcpy (&gameData.multigame.kills.matrix [nPlayer][0], end->killMatrix, MAX_PLAYERS*sizeof (short));
gameData.multiplayer.players [nPlayer].netKillsTotal = end->kills;
gameData.multiplayer.players [nPlayer].netKilledTotal = end->killed;
if ((gameData.multiplayer.players [nPlayer].connected == 1) && (end->seconds_left < gameData.reactor.countdown.nSecsLeft))
	gameData.reactor.countdown.nSecsLeft = end->seconds_left;
ResetPlayerTimeout (nPlayer, -1);
}

//------------------------------------------------------------------------------

void NetworkReadEndLevelShortPacket (ubyte *dataP)
{
	// Special packet for end of level syncing

	int nPlayer;
	tEndLevelInfoShort *end;       

end = (tEndLevelInfoShort *)dataP;
nPlayer = end->nPlayer;
Assert (nPlayer != gameData.multiplayer.nLocalPlayer);
if (nPlayer >= gameData.multiplayer.nPlayers) {              
	Int3 (); // weird, but it can happen in a coop restore game
	return; // if it happens in a coop restore, don't worry about it
	}

if ((networkData.nStatus == NETSTAT_PLAYING) && (end->connected != 0))
	return; // Only accept disconnect packets if we're not out of the level yet
gameData.multiplayer.players [nPlayer].connected = end->connected;
if ((gameData.multiplayer.players [nPlayer].connected == 1) && (end->seconds_left < gameData.reactor.countdown.nSecsLeft))
	gameData.reactor.countdown.nSecsLeft = end->seconds_left;
ResetPlayerTimeout (nPlayer, -1);
}

//------------------------------------------------------------------------------

void NetworkReadSyncPacket (tNetgameInfo * sp, int rsinit)
{
	int				i, j;
	char				szLocalCallSign [CALLSIGN_LEN+1];
	tNetPlayerInfo	*playerP;
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
	tNetgameInfo	tmp_info;

if ((gameStates.multi.nGameType >= IPX_GAME) && (sp != &netGame)) { // for macintosh -- get the values unpacked to our structure format
	ReceiveFullNetGamePacket ((ubyte *)sp, &tmp_info);
	sp = &tmp_info;
	}
#endif

if (rsinit)
	tmpPlayersInfo = &netPlayers;
	// This function is now called by all people entering the netgame.
if (sp != &netGame) {
	char *p = (char *) sp;
	ushort h;
	int i, s;
	for (i = 0, h = -1; i < (int) sizeof (tNetgameInfo) - 1; i++, p++) {
		s = *((ushort *) p);
		if (s == networkData.nSegmentCheckSum) {
			h = i;
			break;
			}
		else if (((s / 256) + (s % 256) * 256) == networkData.nSegmentCheckSum) {
			h = i;
			break;
			}
		}
	memcpy (&netGame, sp, sizeof (tNetgameInfo));
	memcpy (&netPlayers, tmpPlayersInfo, sizeof (tAllNetPlayersInfo));
	}
gameData.multiplayer.nPlayers = sp->nNumPlayers;
gameStates.app.nDifficultyLevel = sp->difficulty;
networkData.nStatus = sp->gameStatus;
//Assert (gameStates.app.nFunctionMode != FMODE_GAME);
// New code, 11/27
#if 1			
con_printf (1, "netGame.checksum = %d, calculated checksum = %d.\n", 
			   netGame.nSegmentCheckSum, networkData.nSegmentCheckSum);
#endif
if (netGame.nSegmentCheckSum != networkData.nSegmentCheckSum) {
	if (extraGameInfo [0].bAutoDownload)
		networkData.nStatus = NETSTAT_AUTODL;
	else {
		short nInMenu = gameStates.menus.nInMenu;
		gameStates.menus.nInMenu = 0;
		networkData.nStatus = NETSTAT_MENU;
		ExecMessageBox (TXT_ERROR, NULL, 1, TXT_OK, TXT_NETLEVEL_MISMATCH);
		gameStates.menus.nInMenu = nInMenu;
		}
#if 1//def RELEASE
		return;
#endif
	}
// Discover my tPlayer number
memcpy (szLocalCallSign, LOCALPLAYER.callsign, CALLSIGN_LEN+1);
gameData.multiplayer.nLocalPlayer = -1;
for (i = 0; i < MAX_NUM_NET_PLAYERS; i++)
	gameData.multiplayer.players [i].netKillsTotal = 0;

for (i = 0, playerP = tmpPlayersInfo->players; i < gameData.multiplayer.nPlayers; i++, playerP++) {
	if (!CmpLocalPlayer (&playerP->network, playerP->callsign, szLocalCallSign)) {
		if (gameData.multiplayer.nLocalPlayer != -1) {
			Int3 (); // Hey, we've found ourselves twice
			ExecMessageBox (TXT_ERROR, NULL, 1, TXT_OK, TXT_DUPLICATE_PLAYERS);
			con_printf (CONDBG, TXT_FOUND_TWICE);
			networkData.nStatus = NETSTAT_MENU;
			return; 
			}
		ChangePlayerNumTo (i);
		}
	memcpy (gameData.multiplayer.players [i].callsign, playerP->callsign, CALLSIGN_LEN+1);
	if (gameStates.multi.nGameType >= IPX_GAME) {
#ifdef WORDS_NEED_ALIGNMENT
		uint server;
		memcpy (&server, playerP->network.ipx.server, 4);
		if (server != 0)
			IpxGetLocalTarget (
				(ubyte *)&server, 
				tmpPlayersInfo->players [i].network.ipx.node, 
				gameData.multiplayer.players [i].netAddress);
#else // WORDS_NEED_ALIGNMENT
		if ((* (uint *)tmpPlayersInfo->players [i].network.ipx.server) != 0)
			IpxGetLocalTarget (
				tmpPlayersInfo->players [i].network.ipx.server, 
				tmpPlayersInfo->players [i].network.ipx.node, 
				gameData.multiplayer.players [i].netAddress);
#endif // WORDS_NEED_ALIGNMENT
		else
			memcpy (gameData.multiplayer.players [i].netAddress, tmpPlayersInfo->players [i].network.ipx.node, 6);
		}
	gameData.multiplayer.players [i].nPacketsGot = 0;                             // How many packets we got from them
	gameData.multiplayer.players [i].nPacketsSent = 0;                            // How many packets we sent to them
	gameData.multiplayer.players [i].connected = playerP->connected;
	gameData.multiplayer.players [i].netKillsTotal = sp->playerKills [i];
	gameData.multiplayer.players [i].netKilledTotal = sp->killed [i];
	if (networkData.nJoinState || (i != gameData.multiplayer.nLocalPlayer))
		gameData.multiplayer.players [i].score = sp->player_score [i];
	for (j = 0; j < MAX_NUM_NET_PLAYERS; j++)
		gameData.multigame.kills.matrix [i][j] = sp->kills [i][j];
	}

if (gameData.multiplayer.nLocalPlayer < 0) {
	ExecMessageBox (TXT_ERROR, NULL, 1, TXT_OK, TXT_PLAYER_REJECTED);
	networkData.nStatus = NETSTAT_MENU;
	return;
	}
if (networkData.nJoinState) {
	for (i = 0; i < gameData.multiplayer.nPlayers; i++)
		gameData.multiplayer.players [i].netKilledTotal = sp->killed [i];
	NetworkProcessMonitorVector (sp->monitor_vector);
	LOCALPLAYER.timeLevel = sp->xLevelTime;
	}
gameData.multigame.kills.nTeam [0] = sp->teamKills [0];
gameData.multigame.kills.nTeam [1] = sp->teamKills [1];
LOCALPLAYER.connected = 1;
netPlayers.players [gameData.multiplayer.nLocalPlayer].connected = 1;
netPlayers.players [gameData.multiplayer.nLocalPlayer].rank = GetMyNetRanking ();
if (!networkData.nJoinState) {
	int	j, bGotTeamSpawnPos = (IsTeamGame) && GotTeamSpawnPos ();
	for (i = 0; i < gameData.multiplayer.nPlayerPositions; i++) {
		if (bGotTeamSpawnPos) {
			j = TeamSpawnPos (i);
			if (j < 0)
				j = netGame.locations [i];
			}
		else
			j = netGame.locations [i];
		GetPlayerSpawn (j, OBJECTS + gameData.multiplayer.players [i].nObject);
		}
	}
OBJECTS [LOCALPLAYER.nObject].nType = OBJ_PLAYER;
networkData.nStatus = (NetworkIAmMaster () || (networkData.nJoinState >= 4)) ? NETSTAT_PLAYING : NETSTAT_WAITING;
SetFunctionMode (FMODE_GAME);
networkData.bHaveSync = 1;
MultiSortKillList ();
}

//------------------------------------------------------------------------------

void NetworkReadPDataPacket (tFrameInfo *pd)
{
	int nTheirPlayer;
	int theirObjNum;
	tObject * theirObjP = NULL;

// tFrameInfo should be aligned...for mac, make the necessary adjustments
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
if (gameStates.multi.nGameType >= IPX_GAME) {
	pd->numpackets = INTEL_INT (pd->numpackets);
	pd->obj_pos.p.x = INTEL_INT (pd->obj_pos.p.x);
	pd->obj_pos.p.y = INTEL_INT (pd->obj_pos.p.y);
	pd->obj_pos.p.z = INTEL_INT (pd->obj_pos.p.z);
	pd->obj_orient.rVec.p.x = (fix)INTEL_INT ((int)pd->obj_orient.rVec.p.x);
	pd->obj_orient.rVec.p.y = (fix)INTEL_INT ((int)pd->obj_orient.rVec.p.y);
	pd->obj_orient.rVec.p.z = (fix)INTEL_INT ((int)pd->obj_orient.rVec.p.z);
	pd->obj_orient.uVec.p.x = (fix)INTEL_INT ((int)pd->obj_orient.uVec.p.x);
	pd->obj_orient.uVec.p.y = (fix)INTEL_INT ((int)pd->obj_orient.uVec.p.y);
	pd->obj_orient.uVec.p.z = (fix)INTEL_INT ((int)pd->obj_orient.uVec.p.z);
	pd->obj_orient.fVec.p.x = (fix)INTEL_INT ((int)pd->obj_orient.fVec.p.x);
	pd->obj_orient.fVec.p.y = (fix)INTEL_INT ((int)pd->obj_orient.fVec.p.y);
	pd->obj_orient.fVec.p.z = (fix)INTEL_INT ((int)pd->obj_orient.fVec.p.z);
	pd->phys_velocity.p.x = (fix)INTEL_INT ((int)pd->phys_velocity.p.x);
	pd->phys_velocity.p.y = (fix)INTEL_INT ((int)pd->phys_velocity.p.y);
	pd->phys_velocity.p.z = (fix)INTEL_INT ((int)pd->phys_velocity.p.z);
	pd->phys_rotvel.p.x = (fix)INTEL_INT ((int)pd->phys_rotvel.p.x);
	pd->phys_rotvel.p.y = (fix)INTEL_INT ((int)pd->phys_rotvel.p.y);
	pd->phys_rotvel.p.z = (fix)INTEL_INT ((int)pd->phys_rotvel.p.z);
	pd->obj_segnum = INTEL_SHORT (pd->obj_segnum);
	pd->data_size = INTEL_SHORT (pd->data_size);
	}
#endif
nTheirPlayer = pd->nPlayer;
theirObjNum = gameData.multiplayer.players [pd->nPlayer].nObject;
if (nTheirPlayer < 0) {
	Int3 (); // This packet is bogus!!
	return;
	}
if ((networkData.sync.nPlayer != -1) && (nTheirPlayer == networkData.sync.nPlayer))
	networkData.sync.nPlayer = -1;
if (!gameData.multigame.bQuitGame && (nTheirPlayer >= gameData.multiplayer.nPlayers)) {
	if (networkData.nStatus!=NETSTAT_WAITING) {
		Int3 (); // We missed an important packet!
		NetworkConsistencyError ();
		}
	return;
	}
if (gameStates.app.bEndLevelSequence || (networkData.nStatus == NETSTAT_ENDLEVEL)) {
	int old_Endlevel_sequence = gameStates.app.bEndLevelSequence;
	gameStates.app.bEndLevelSequence = 1;
	if (pd->data_size > 0)
		// pass pd->data to some parser function....
		MultiProcessBigData ((char *) pd->data, pd->data_size);
	gameStates.app.bEndLevelSequence = old_Endlevel_sequence;
	return;
	}
if ((sbyte)pd->level_num != gameData.missions.nCurrentLevel) {
#if 1			
	con_printf (CONDBG, "Got frame packet from tPlayer %d wrong level %d!\n", pd->nPlayer, pd->level_num);
#endif
	return;
	}

theirObjP = OBJECTS + theirObjNum;
//------------- Keep track of missed packets -----------------
gameData.multiplayer.players [nTheirPlayer].nPacketsGot++;
networkData.nTotalPacketsGot++;
ResetPlayerTimeout (nTheirPlayer, -1);
if  (pd->numpackets != gameData.multiplayer.players [nTheirPlayer].nPacketsGot) {
	networkData.nMissedPackets = pd->numpackets - gameData.multiplayer.players [nTheirPlayer].nPacketsGot;
	if ((pd->numpackets-gameData.multiplayer.players [nTheirPlayer].nPacketsGot) > 0)
		networkData.nTotalMissedPackets += pd->numpackets-gameData.multiplayer.players [nTheirPlayer].nPacketsGot;
#if 1			
	if (networkData.nMissedPackets > 0)       
		con_printf (0, 
			"Missed %d packets from tPlayer #%d (%d total)\n", 
			pd->numpackets-gameData.multiplayer.players [nTheirPlayer].nPacketsGot, 
			nTheirPlayer, 
			networkData.nMissedPackets);
	else
		con_printf (CONDBG, 
			"Got %d late packets from tPlayer #%d (%d total)\n", 
			gameData.multiplayer.players [nTheirPlayer].nPacketsGot-pd->numpackets, 
			nTheirPlayer, 
			networkData.nMissedPackets);
#endif
	gameData.multiplayer.players [nTheirPlayer].nPacketsGot = pd->numpackets;
	}
//------------ Read the tPlayer's ship's tObject info ----------------------
theirObjP->position.vPos = pd->obj_pos;
theirObjP->position.mOrient = pd->obj_orient;
theirObjP->mType.physInfo.velocity = pd->phys_velocity;
theirObjP->mType.physInfo.rotVel = pd->phys_rotvel;
if ((theirObjP->renderType != pd->obj_renderType) && (pd->obj_renderType == RT_POLYOBJ))
	MultiMakeGhostPlayer (nTheirPlayer);
RelinkObject (theirObjNum, pd->obj_segnum);
if (theirObjP->movementType == MT_PHYSICS)
	SetThrustFromVelocity (theirObjP);
//------------ Welcome them back if reconnecting --------------
if (!gameData.multiplayer.players [nTheirPlayer].connected) {
	gameData.multiplayer.players [nTheirPlayer].connected = 1;
	if (gameData.demo.nState == ND_STATE_RECORDING)
		NDRecordMultiReconnect (nTheirPlayer);
	MultiMakeGhostPlayer (nTheirPlayer);
	CreatePlayerAppearanceEffect (OBJECTS + theirObjNum);
	DigiPlaySample (SOUND_HUD_MESSAGE, F1_0);
	ClipRank ((char *) &netPlayers.players [nTheirPlayer].rank);
	if (gameOpts->multi.bNoRankings)      
		HUDInitMessage ("'%s' %s", gameData.multiplayer.players [nTheirPlayer].callsign, TXT_REJOIN);
	else
		HUDInitMessage ("%s'%s' %s", 
							 pszRankStrings [netPlayers.players [nTheirPlayer].rank], 
							 gameData.multiplayer.players [nTheirPlayer].callsign, TXT_REJOIN);
	MultiSendScore ();
	}
//------------ Parse the extra dataP at the end ---------------
if (pd->data_size > 0)
	// pass pd->data to some parser function....
	MultiProcessBigData ((char *) pd->data, pd->data_size);
}

//------------------------------------------------------------------------------

#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)

void GetShortFrameInfo (ubyte *old_info, tFrameInfoShort *new_info)
{
	int bufI = 0;

NW_GET_BYTE (old_info, bufI, new_info->nType);
/* skip three for pad byte */                                       
bufI += 3;
NW_GET_INT (old_info, bufI, new_info->numpackets);
NW_GET_BYTES (old_info, bufI, new_info->thepos.bytemat, 9);
NW_GET_SHORT (old_info, bufI, new_info->thepos.xo);
NW_GET_SHORT (old_info, bufI, new_info->thepos.yo);
NW_GET_SHORT (old_info, bufI, new_info->thepos.zo);
NW_GET_SHORT (old_info, bufI, new_info->thepos.nSegment);
NW_GET_SHORT (old_info, bufI, new_info->thepos.velx);
NW_GET_SHORT (old_info, bufI, new_info->thepos.vely);
NW_GET_SHORT (old_info, bufI, new_info->thepos.velz);
NW_GET_SHORT (old_info, bufI, new_info->data_size);
NW_GET_BYTE (old_info, bufI, new_info->nPlayer);
NW_GET_BYTE (old_info, bufI, new_info->obj_renderType);
NW_GET_BYTE (old_info, bufI, new_info->level_num);
NW_GET_BYTES (old_info, bufI, new_info->data, new_info->data_size);
}
#else

#define GetShortFrameInfo(old_info, new_info) \
	memcpy (new_info, old_info, sizeof (tFrameInfoShort))

#endif

//------------------------------------------------------------------------------

void NetworkReadPDataShortPacket (tFrameInfoShort *pd)
{
	int nTheirPlayer;
	int theirObjNum;
	tObject * theirObjP = NULL;
	tFrameInfoShort new_pd;

// short frame info is not aligned because of tShortPos.  The mac
// will call totally hacked and gross function to fix this up.

if (gameStates.multi.nGameType >= IPX_GAME)
	GetShortFrameInfo ((ubyte *)pd, &new_pd);
else
	memcpy (&new_pd, (ubyte *)pd, sizeof (tFrameInfoShort));
nTheirPlayer = new_pd.nPlayer;
theirObjNum = gameData.multiplayer.players [new_pd.nPlayer].nObject;
if (nTheirPlayer < 0) {
	Int3 (); // This packet is bogus!!
	return;
	}
if (!gameData.multigame.bQuitGame && (nTheirPlayer >= gameData.multiplayer.nPlayers)) {
	if (networkData.nStatus!=NETSTAT_WAITING) {
		Int3 (); // We missed an important packet!
		NetworkConsistencyError ();
		}
	return;
	}
if ((networkData.sync.nPlayer != -1) && (nTheirPlayer == networkData.sync.nPlayer)) {
	// Hurray! Someone really really got in the game (I think).
   networkData.sync.nPlayer = -1;
	}

if (gameStates.app.bEndLevelSequence || (networkData.nStatus == NETSTAT_ENDLEVEL)) {
	int old_Endlevel_sequence = gameStates.app.bEndLevelSequence;
	gameStates.app.bEndLevelSequence = 1;
	if (new_pd.data_size > 0) {
		// pass pd->data to some parser function....
		MultiProcessBigData ((char *) new_pd.data, new_pd.data_size);
		}
	gameStates.app.bEndLevelSequence = old_Endlevel_sequence;
	return;
	}
if ((sbyte)new_pd.level_num != gameData.missions.nCurrentLevel) {
#if 1			
	con_printf (CONDBG, "Got frame packet from tPlayer %d wrong level %d!\n", new_pd.nPlayer, new_pd.level_num);
#endif
	return;
	}
theirObjP = OBJECTS + theirObjNum;
//------------- Keep track of missed packets -----------------
gameData.multiplayer.players [nTheirPlayer].nPacketsGot++;
networkData.nTotalPacketsGot++;
networkData.nLastPacketTime [nTheirPlayer] = SDL_GetTicks ();
if  (new_pd.numpackets != gameData.multiplayer.players [nTheirPlayer].nPacketsGot)      {
	networkData.nMissedPackets = new_pd.numpackets-gameData.multiplayer.players [nTheirPlayer].nPacketsGot;
	if ((new_pd.numpackets-gameData.multiplayer.players [nTheirPlayer].nPacketsGot)>0)
		networkData.nTotalMissedPackets += new_pd.numpackets-gameData.multiplayer.players [nTheirPlayer].nPacketsGot;
#if 1			
	if (networkData.nMissedPackets > 0)       
		con_printf (CONDBG, 
			"Missed %d packets from tPlayer #%d (%d total)\n", 
			new_pd.numpackets-gameData.multiplayer.players [nTheirPlayer].nPacketsGot, 
			nTheirPlayer, 
			networkData.nMissedPackets);
	else
		con_printf (CONDBG, 
			"Got %d late packets from tPlayer #%d (%d total)\n", 
			gameData.multiplayer.players [nTheirPlayer].nPacketsGot-new_pd.numpackets, 
			nTheirPlayer, 
			networkData.nMissedPackets);
#endif
		gameData.multiplayer.players [nTheirPlayer].nPacketsGot = new_pd.numpackets;
	}
//------------ Read the tPlayer's ship's tObject info ----------------------
ExtractShortPos (theirObjP, &new_pd.thepos, 0);
if ((theirObjP->renderType != new_pd.obj_renderType) && (new_pd.obj_renderType == RT_POLYOBJ))
	MultiMakeGhostPlayer (nTheirPlayer);
if (theirObjP->movementType == MT_PHYSICS)
	SetThrustFromVelocity (theirObjP);
//------------ Welcome them back if reconnecting --------------
if (!gameData.multiplayer.players [nTheirPlayer].connected) {
	gameData.multiplayer.players [nTheirPlayer].connected = 1;
	if (gameData.demo.nState == ND_STATE_RECORDING)
		NDRecordMultiReconnect (nTheirPlayer);
	MultiMakeGhostPlayer (nTheirPlayer);
	CreatePlayerAppearanceEffect (OBJECTS + theirObjNum);
	DigiPlaySample (SOUND_HUD_MESSAGE, F1_0);
	ClipRank ((char *) &netPlayers.players [nTheirPlayer].rank);
	if (gameOpts->multi.bNoRankings)
		HUDInitMessage ("'%s' %s", gameData.multiplayer.players [nTheirPlayer].callsign, TXT_REJOIN);
	else
		HUDInitMessage ("%s'%s' %s", 
							 pszRankStrings [netPlayers.players [nTheirPlayer].rank], 
							 gameData.multiplayer.players [nTheirPlayer].callsign, TXT_REJOIN);
	MultiSendScore ();
	}
//------------ Parse the extra dataP at the end ---------------
if (new_pd.data_size>0) {
	// pass pd->data to some parser function....
	MultiProcessBigData ((char *) new_pd.data, new_pd.data_size);
	}
}

//------------------------------------------------------------------------------

int NetworkVerifyObjects (int nRemoteObj, int nLocalObjs)
{
return !gameStates.app.bHaveExtraGameInfo [1] && (nRemoteObj - nLocalObjs > 10) ? -1 : 0;
}

//------------------------------------------------------------------------------

int NetworkVerifyPlayers (void)
{
	int		i, j, t, bCoop = IsCoopGame;
	int		nPlayers, nPlayerObjs [MAX_PLAYERS], bHaveReactor = !bCoop;
	tObject	*objP = OBJECTS;
	tPlayer	*playerP;

for (j = 0, playerP = gameData.multiplayer.players; j < MAX_PLAYERS; j++, playerP++)
	nPlayerObjs [j] = playerP->connected ? playerP->nObject : -1;
#if 0
if (gameData.app.nGameMode & GM_MULTI_ROBOTS)
#endif
//	bHaveReactor = 1;	// multiplayer maps do not need a control center ...
for (i = 0, nPlayers = 0; i <= gameData.objs.nLastObject [0]; i++, objP++) {
	t = objP->nType;
	if (t == OBJ_GHOST) {
		for (j = 0; j < MAX_PLAYERS; j++) {
			if (nPlayerObjs [j] == i) {
				nPlayers++;
				break;
				}
			}
		}
	else if (t == OBJ_PLAYER) {
		if (!(i && bCoop))
			nPlayers++;
		}
	else if (t == OBJ_COOP) {
		if (bCoop)
			nPlayers++;
		}
	else if (bCoop) {
		if ((t == OBJ_REACTOR) || ((t == OBJ_ROBOT) && ROBOTINFO (objP->id).bossFlag))
			bHaveReactor = 1;
		}
	if (nPlayers >= gameData.multiplayer.nMaxPlayers)
		return 1;
	}
return !bHaveReactor;
}

//------------------------------------------------------------------------------

void NetworkAbortSync (void)
{
ExecMessageBox (NULL, NULL, 1, TXT_OK, TXT_NET_SYNC_FAILED);
networkData.nStatus = NETSTAT_MENU;                          
}

//------------------------------------------------------------------------------

static int NetworkCheckMissingFrames (void)
{
if (networkData.nPrevFrame == networkData.sync.objs.nFrame - 1)
	return 1;
if (!networkData.nPrevFrame || (networkData.nPrevFrame >= networkData.sync.objs.nFrame))
	return 0;
networkData.sync.objs.missingFrames.nFrame = networkData.nPrevFrame + 1;
networkData.nJoinState = 2;
NetworkSendMissingObjFrames ();
return -1;
}

//------------------------------------------------------------------------------

inline bool ObjectIsLinked (tObject *objP, short nSegment)
{
if (nSegment != -1) {
	short nObject = OBJ_IDX (objP);
	for (short i = gameData.segs.objects [objP->nSegment], j = -1; i >= 0; j = i, i = OBJECTS [i].next) {
		if (i == nObject) {
			objP->prev = j;
			return true;
			}
		}
	}
return false;
}

//------------------------------------------------------------------------------

void NetworkReadObjectPacket (ubyte *dataP)
{
	static int		nPlayer = 0;
	static int		nMode = 0;
	static short	objectCount = 0;

	// Object from another net tPlayer we need to sync with
	tObject	*objP;
	short		nObject, nRemoteObj;
	sbyte		nObjOwner;
	short		nSegment, i;

	int		nObjects = dataP [1];
	int		bufI;

networkData.nPrevFrame = networkData.sync.objs.nFrame;
if (gameStates.multi.nGameType == UDP_GAME) {
	bufI = 2;
	NW_GET_SHORT (dataP, bufI, networkData.sync.objs.nFrame);
	}
else {
	networkData.sync.objs.nFrame = dataP [2];
	bufI = 3;
	}
i = NetworkCheckMissingFrames ();
if (!i) {
	networkData.toSyncPoll = 0;
	return;
	}
else if (i < 0)
	return;
#ifdef _DEBUG
//PrintLog ("Receiving object packet %d (prev: %d)\n", networkData.nPrevFrame, networkData.sync.objs.nFrame);
#endif
 for (i = 0; i < nObjects; i++) {
	NW_GET_SHORT (dataP, bufI, nObject);                   
	NW_GET_BYTE (dataP, bufI, nObjOwner);                                          
	NW_GET_SHORT (dataP, bufI, nRemoteObj);
	if ((nObject == -1) || (nObject == -3)) {
		// Clear tObject array
		nPlayer = nObjOwner;
		ChangePlayerNumTo (nPlayer);
		nMode = 1;
		objectCount = 0;
		networkData.nPrevFrame = networkData.sync.objs.nFrame - 1;
		if (nObject == -3) {
			if (networkData.nJoinState != 2)
				return;
#ifdef _DEBUG
			PrintLog ("Receiving missing object packets\n");
#endif
			networkData.nJoinState = 3;
			}
		else {
			if (networkData.nJoinState)
				return;
			InitObjects ();
			networkData.nJoinState = 1;
			}
		networkData.sync.objs.missingFrames.nFrame = 0;
		}
	else if ((nObject == -2) || (nObject == -4)) {	// Special debug checksum marker for entire send
 		if (!nMode && NetworkVerifyObjects (nRemoteObj, objectCount)) {
			NetworkAbortSync ();
			return;
			}
		networkData.sync.objs.nFrame = 0;
		nMode = 0;
		if (networkData.bHaveSync)
			networkData.nStatus = NETSTAT_PLAYING;
		networkData.nJoinState = 4;
		}
	else if (networkData.nJoinState & 1) {
#if 1			
		con_printf (CONDBG, "Got a type 3 object packet!\n");
#endif
		objectCount++;
		nObject = nRemoteObj;
		if (!InsertObject (nObject)) {
			if (networkData.nJoinState == 3) {
				FreeObject (nObject);
				if (!InsertObject (nObject))
					nObject = -1;
				}
			else
				nObject = AllocObject ();
			}
		if ((nObjOwner != nPlayer) && (nObjOwner != -1)) {
			if (nMode == 1)
				nMode = 0;
			else
				Int3 (); // SEE ROB
			}
		if (nObject != -1) {
			Assert (nObject < MAX_OBJECTS);
			objP = OBJECTS + nObject;
#ifdef _DEBUG
			if (objP->nSegment >= 0)
				objP = objP;
#endif
			if (ObjectIsLinked (objP, objP->nSegment))
				UnlinkObject (nObject);
			NW_GET_BYTES (dataP, bufI, objP, sizeof (tObject));
			if (objP->nType != OBJ_NONE) {
				if (gameStates.multi.nGameType >= IPX_GAME)
					SwapObject (objP);
				nSegment = objP->nSegment;
				objP->next = objP->prev = objP->nSegment = -1;
				objP->attachedObj = -1;
				if (nSegment < 0)
					nSegment = FindSegByPos (&objP->position.vPos, -1, 1, 0);
				if (!ObjectIsLinked (objP, nSegment))
					LinkObject (OBJ_IDX (objP), nSegment);
				if ((objP->nType == OBJ_PLAYER) || (objP->nType == OBJ_GHOST))
					RemapLocalPlayerObject (nObject, nRemoteObj);
				if (nObjOwner == nPlayer) 
					MapObjnumLocalToLocal (nObject);
				else if (nObjOwner != -1)
					MapObjnumLocalToRemote (nObject, nRemoteObj, nObjOwner);
				else
					gameData.multigame.nObjOwner [nObject] = -1;
				}
			}
		} // For a standard onbject
	} // For each tObject in packet
gameData.objs.nObjects = objectCount;
//gameData.objs.nLastObject [0] = gameData.objs.nObjects - 1;
}

//------------------------------------------------------------------------------

