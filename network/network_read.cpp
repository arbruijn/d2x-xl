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
#include "byteswap.h"
#include "error.h"
#include "timer.h"
#include "strutil.h"
#include "ipx.h"
#include "network.h"
#include "network_lib.h"
#include "netmisc.h"
#include "text.h"
#include "newdemo.h"
#include "gameseg.h"
#include "objeffects.h"
#include "physics.h"

//------------------------------------------------------------------------------

void NetworkReadEndLevelPacket (ubyte *dataP)
{
	// Special packet for end of level syncing
	int nPlayer;
	tEndLevelInfo *end = reinterpret_cast<tEndLevelInfo*> (dataP);
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

end = reinterpret_cast<tEndLevelInfoShort*> (dataP);
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
	ReceiveFullNetGamePacket (reinterpret_cast<ubyte*> (sp), &tmp_info);
	sp = &tmp_info;
	}
#endif

if (rsinit)
	playerInfoP = &netPlayers;
	// This function is now called by all people entering the netgame.
if (sp != &netGame) {
	char *p = reinterpret_cast<char*> (sp);
	ushort h;
	int i, s;
	for (i = 0, h = -1; i < (int) sizeof (tNetgameInfo) - 1; i++, p++) {
		s = *reinterpret_cast<ushort*> (p);
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
	memcpy (&netPlayers, playerInfoP, sizeof (tAllNetPlayersInfo));
	}
gameData.multiplayer.nPlayers = sp->nNumPlayers;
gameStates.app.nDifficultyLevel = sp->difficulty;
networkData.nStatus = sp->gameStatus;
//Assert (gameStates.app.nFunctionMode != FMODE_GAME);
// New code, 11/27
#if 1
console.printf (1, "netGame.checksum = %d, calculated checksum = %d.\n",
					 netGame.nSegmentCheckSum, networkData.nSegmentCheckSum);
#endif
if (netGame.nSegmentCheckSum != networkData.nSegmentCheckSum) {
	if (extraGameInfo [0].bAutoDownload)
		networkData.nStatus = NETSTAT_AUTODL;
	else {
		short nInMenu = gameStates.menus.nInMenu;
		gameStates.menus.nInMenu = 0;
		networkData.nStatus = NETSTAT_MENU;
		MsgBox (TXT_ERROR, NULL, 1, TXT_OK, TXT_NETLEVEL_MISMATCH);
		gameStates.menus.nInMenu = nInMenu;
		}
#if 1//!DBG
		return;
#endif
	}
// Discover my CPlayerData number
memcpy (szLocalCallSign, LOCALPLAYER.callsign, CALLSIGN_LEN+1);
gameData.multiplayer.nLocalPlayer = -1;
for (i = 0; i < MAX_NUM_NET_PLAYERS; i++)
	gameData.multiplayer.players [i].netKillsTotal = 0;

for (i = 0, playerP = playerInfoP->players; i < gameData.multiplayer.nPlayers; i++, playerP++) {
	if (!CmpLocalPlayer (&playerP->network, playerP->callsign, szLocalCallSign)) {
		if (gameData.multiplayer.nLocalPlayer != -1) {
			Int3 (); // Hey, we've found ourselves twice
			MsgBox (TXT_ERROR, NULL, 1, TXT_OK, TXT_DUPLICATE_PLAYERS);
			console.printf (CON_DBG, TXT_FOUND_TWICE);
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
				reinterpret_cast<ubyte*> (&server),
				playerInfoP->players [i].network.ipx.node,
				gameData.multiplayer.players [i].netAddress);
#else // WORDS_NEED_ALIGNMENT
		if (*reinterpret_cast<uint*> (playerInfoP->players [i].network.ipx.server) != 0)
			IpxGetLocalTarget (
				playerInfoP->players [i].network.ipx.server,
				playerInfoP->players [i].network.ipx.node,
				gameData.multiplayer.players [i].netAddress);
#endif // WORDS_NEED_ALIGNMENT
		else
			memcpy (gameData.multiplayer.players [i].netAddress, playerInfoP->players [i].network.ipx.node, 6);
		}
	gameData.multiplayer.players [i].nPacketsGot = 0;                             // How many packets we got from them
	gameData.multiplayer.players [i].nPacketsSent = 0;                            // How many packets we sent to them
	gameData.multiplayer.players [i].connected = playerP->connected;
	gameData.multiplayer.players [i].netKillsTotal = sp->playerKills [i];
	gameData.multiplayer.players [i].netKilledTotal = sp->killed [i];
	if (networkData.nJoinState || (i != gameData.multiplayer.nLocalPlayer))
		gameData.multiplayer.players [i].score = sp->playerScore [i];
	for (j = 0; j < MAX_NUM_NET_PLAYERS; j++)
		gameData.multigame.kills.matrix [i][j] = sp->kills [i][j];
	}

if (gameData.multiplayer.nLocalPlayer < 0) {
	MsgBox (TXT_ERROR, NULL, 1, TXT_OK, TXT_PLAYER_REJECTED);
	networkData.nStatus = NETSTAT_MENU;
	return;
	}
if (networkData.nJoinState) {
	for (i = 0; i < gameData.multiplayer.nPlayers; i++)
		gameData.multiplayer.players [i].netKilledTotal = sp->killed [i];
	NetworkProcessMonitorVector (sp->monitorVector);
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
OBJECTS [LOCALPLAYER.nObject].SetType (OBJ_PLAYER);
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
	CObject * theirObjP = NULL;

// tFrameInfo should be aligned...for mac, make the necessary adjustments
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
if (gameStates.multi.nGameType >= IPX_GAME) {
	pd->nPackets = INTEL_INT (pd->nPackets);
	pd->objPos[X] = INTEL_INT (pd->objPos[X]);
	pd->objPos[Y] = INTEL_INT (pd->objPos[Y]);
	pd->objPos[Z] = INTEL_INT (pd->objPos[Z]);
	pd->objOrient.RVec ()[X] = (fix)INTEL_INT ((int)pd->objOrient.RVec ()[X]);
	pd->objOrient.RVec ()[Y] = (fix)INTEL_INT ((int)pd->objOrient.RVec ()[Y]);
	pd->objOrient.RVec ()[Z] = (fix)INTEL_INT ((int)pd->objOrient.RVec ()[Z]);
	pd->objOrient.UVec ()[X] = (fix)INTEL_INT ((int)pd->objOrient.UVec ()[X]);
	pd->objOrient.UVec ()[Y] = (fix)INTEL_INT ((int)pd->objOrient.UVec ()[Y]);
	pd->objOrient.UVec ()[Z] = (fix)INTEL_INT ((int)pd->objOrient.UVec ()[Z]);
	pd->objOrient.FVec ()[X] = (fix)INTEL_INT ((int)pd->objOrient.FVec ()[X]);
	pd->objOrient.FVec ()[Y] = (fix)INTEL_INT ((int)pd->objOrient.FVec ()[Y]);
	pd->objOrient.FVec ()[Z] = (fix)INTEL_INT ((int)pd->objOrient.FVec ()[Z]);
	pd->physVelocity[X] = (fix)INTEL_INT ((int)pd->physVelocity[X]);
	pd->physVelocity[Y] = (fix)INTEL_INT ((int)pd->physVelocity[Y]);
	pd->physVelocity[Z] = (fix)INTEL_INT ((int)pd->physVelocity[Z]);
	pd->physRotVel[X] = (fix)INTEL_INT ((int)pd->physRotVel[X]);
	pd->physRotVel[Y] = (fix)INTEL_INT ((int)pd->physRotVel[Y]);
	pd->physRotVel[Z] = (fix)INTEL_INT ((int)pd->physRotVel[Z]);
	pd->nObjSeg = INTEL_SHORT (pd->nObjSeg);
	pd->dataSize = INTEL_SHORT (pd->dataSize);
	}
#endif
nTheirPlayer = pd->nPlayer;
theirObjNum = gameData.multiplayer.players [pd->nPlayer].nObject;
if (nTheirPlayer < 0) {
	Int3 (); // This packet is bogus!!
	return;
	}
if ((networkData.sync [0].nPlayer != -1) && (nTheirPlayer == networkData.sync [0].nPlayer))
	networkData.sync [0].nPlayer = -1;
if (!gameData.multigame.bQuitGame && (nTheirPlayer >= gameData.multiplayer.nPlayers)) {
	if (networkData.nStatus != NETSTAT_WAITING) {
		Int3 (); // We missed an important packet!
		NetworkConsistencyError ();
		}
	return;
	}
if (gameStates.app.bEndLevelSequence || (networkData.nStatus == NETSTAT_ENDLEVEL)) {
	int old_Endlevel_sequence = gameStates.app.bEndLevelSequence;
	gameStates.app.bEndLevelSequence = 1;
	if (pd->dataSize > 0)
		// pass pd->data to some parser function....
		MultiProcessBigData (reinterpret_cast<char*> (pd->data), pd->dataSize);
	gameStates.app.bEndLevelSequence = old_Endlevel_sequence;
	return;
	}
if ((sbyte)pd->nLevel != gameData.missions.nCurrentLevel) {
#if 1
	console.printf (CON_DBG, "Got frame packet from CPlayerData %d wrong level %d!\n", pd->nPlayer, pd->nLevel);
#endif
	return;
	}

theirObjP = OBJECTS + theirObjNum;
//------------- Keep track of missed packets -----------------
gameData.multiplayer.players [nTheirPlayer].nPacketsGot++;
networkData.nTotalPacketsGot++;
ResetPlayerTimeout (nTheirPlayer, -1);
if  (pd->nPackets != gameData.multiplayer.players [nTheirPlayer].nPacketsGot) {
	networkData.nMissedPackets = pd->nPackets - gameData.multiplayer.players [nTheirPlayer].nPacketsGot;
	if ((pd->nPackets-gameData.multiplayer.players [nTheirPlayer].nPacketsGot) > 0)
		networkData.nTotalMissedPackets += pd->nPackets-gameData.multiplayer.players [nTheirPlayer].nPacketsGot;
#if 1
	if (networkData.nMissedPackets > 0)
		console.printf (0,
			"Missed %d packets from CPlayerData #%d (%d total)\n",
			pd->nPackets-gameData.multiplayer.players [nTheirPlayer].nPacketsGot,
			nTheirPlayer,
			networkData.nMissedPackets);
	else
		console.printf (CON_DBG,
			"Got %d late packets from CPlayerData #%d (%d total)\n",
			gameData.multiplayer.players [nTheirPlayer].nPacketsGot-pd->nPackets,
			nTheirPlayer,
			networkData.nMissedPackets);
#endif
	gameData.multiplayer.players [nTheirPlayer].nPacketsGot = pd->nPackets;
	}
//------------ Read the CPlayerData's ship's CObject info ----------------------
theirObjP->info.position.vPos = pd->objPos;
theirObjP->info.position.mOrient = pd->objOrient;
theirObjP->mType.physInfo.velocity = pd->physVelocity;
theirObjP->mType.physInfo.rotVel = pd->physRotVel;
if ((theirObjP->info.renderType != pd->objRenderType) && (pd->objRenderType == RT_POLYOBJ))
	MultiMakeGhostPlayer (nTheirPlayer);
OBJECTS [theirObjNum].RelinkToSeg (pd->nObjSeg);
if (theirObjP->info.movementType == MT_PHYSICS)
	theirObjP->SetThrustFromVelocity ();
//------------ Welcome them back if reconnecting --------------
if (!gameData.multiplayer.players [nTheirPlayer].connected) {
	gameData.multiplayer.players [nTheirPlayer].connected = 1;
	if (gameData.demo.nState == ND_STATE_RECORDING)
		NDRecordMultiReconnect (nTheirPlayer);
	MultiMakeGhostPlayer (nTheirPlayer);
	OBJECTS [theirObjNum].CreateAppearanceEffect ();
	audio.PlaySound (SOUND_HUD_MESSAGE);
	ClipRank (reinterpret_cast<char*> (&netPlayers.players [nTheirPlayer].rank));
	if (gameOpts->multi.bNoRankings)
		HUDInitMessage ("'%s' %s", gameData.multiplayer.players [nTheirPlayer].callsign, TXT_REJOIN);
	else
		HUDInitMessage ("%s'%s' %s",
							 pszRankStrings [netPlayers.players [nTheirPlayer].rank],
							 gameData.multiplayer.players [nTheirPlayer].callsign, TXT_REJOIN);
	MultiSendScore ();
	}
//------------ Parse the extra dataP at the end ---------------
if (pd->dataSize > 0)
	// pass pd->data to some parser function....
	MultiProcessBigData (reinterpret_cast<char*> (pd->data), pd->dataSize);
}

//------------------------------------------------------------------------------

#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)

void GetShortFrameInfo (ubyte *old_info, tFrameInfoShort *new_info)
{
	int bufI = 0;

NW_GET_BYTE (old_info, bufI, new_info->nType);
/* skip three for pad byte */
bufI += 3;
NW_GET_INT (old_info, bufI, new_info->nPackets);
NW_GET_BYTES (old_info, bufI, new_info->objPos.orient, 9);
NW_GET_SHORT (old_info, bufI, new_info->objPos.pos [0]);
NW_GET_SHORT (old_info, bufI, new_info->objPos.pos [1]);
NW_GET_SHORT (old_info, bufI, new_info->objPos.pos [2]);
NW_GET_SHORT (old_info, bufI, new_info->objPos.nSegment);
NW_GET_SHORT (old_info, bufI, new_info->objPos.vel [0]);
NW_GET_SHORT (old_info, bufI, new_info->objPos.vel [1]);
NW_GET_SHORT (old_info, bufI, new_info->objPos.vel [2]);
NW_GET_SHORT (old_info, bufI, new_info->dataSize);
NW_GET_BYTE (old_info, bufI, new_info->nPlayer);
NW_GET_BYTE (old_info, bufI, new_info->objRenderType);
NW_GET_BYTE (old_info, bufI, new_info->nLevel);
NW_GET_BYTES (old_info, bufI, new_info->data, new_info->dataSize);
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
	CObject * theirObjP = NULL;
	tFrameInfoShort new_pd;

// short frame info is not aligned because of tShortPos.  The mac
// will call totally hacked and gross function to fix this up.

if (gameStates.multi.nGameType >= IPX_GAME)
	GetShortFrameInfo (reinterpret_cast<ubyte*> (pd), &new_pd);
else
	memcpy (&new_pd, reinterpret_cast<ubyte*> (pd), sizeof (tFrameInfoShort));
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
if ((networkData.sync [0].nPlayer != -1) && (nTheirPlayer == networkData.sync [0].nPlayer)) {
	// Hurray! Someone really really got in the game (I think).
   networkData.sync [0].nPlayer = -1;
	}

if (gameStates.app.bEndLevelSequence || (networkData.nStatus == NETSTAT_ENDLEVEL)) {
	int old_Endlevel_sequence = gameStates.app.bEndLevelSequence;
	gameStates.app.bEndLevelSequence = 1;
	if (new_pd.dataSize > 0) {
		// pass pd->data to some parser function....
		MultiProcessBigData (reinterpret_cast<char*> (new_pd.data), new_pd.dataSize);
		}
	gameStates.app.bEndLevelSequence = old_Endlevel_sequence;
	return;
	}
if ((sbyte)new_pd.nLevel != gameData.missions.nCurrentLevel) {
#if 1
	console.printf (CON_DBG, "Got frame packet from CPlayerData %d wrong level %d!\n", new_pd.nPlayer, new_pd.nLevel);
#endif
	return;
	}
theirObjP = OBJECTS + theirObjNum;
//------------- Keep track of missed packets -----------------
gameData.multiplayer.players [nTheirPlayer].nPacketsGot++;
networkData.nTotalPacketsGot++;
networkData.nLastPacketTime [nTheirPlayer] = SDL_GetTicks ();
if  (new_pd.nPackets != gameData.multiplayer.players [nTheirPlayer].nPacketsGot) {
	networkData.nMissedPackets = new_pd.nPackets-gameData.multiplayer.players [nTheirPlayer].nPacketsGot;
	if ((new_pd.nPackets-gameData.multiplayer.players [nTheirPlayer].nPacketsGot)>0)
		networkData.nTotalMissedPackets += new_pd.nPackets-gameData.multiplayer.players [nTheirPlayer].nPacketsGot;
#if 1
	if (networkData.nMissedPackets > 0)
		console.printf (CON_DBG,
			"Missed %d packets from CPlayerData #%d (%d total)\n",
			new_pd.nPackets-gameData.multiplayer.players [nTheirPlayer].nPacketsGot,
			nTheirPlayer,
			networkData.nMissedPackets);
	else
		console.printf (CON_DBG,
			"Got %d late packets from CPlayerData #%d (%d total)\n",
			gameData.multiplayer.players [nTheirPlayer].nPacketsGot-new_pd.nPackets,
			nTheirPlayer,
			networkData.nMissedPackets);
#endif
		gameData.multiplayer.players [nTheirPlayer].nPacketsGot = new_pd.nPackets;
	}
//------------ Read the CPlayerData's ship's CObject info ----------------------
ExtractShortPos (theirObjP, &new_pd.objPos, 0);
if ((theirObjP->info.renderType != new_pd.objRenderType) && (new_pd.objRenderType == RT_POLYOBJ))
	MultiMakeGhostPlayer (nTheirPlayer);
if (theirObjP->info.movementType == MT_PHYSICS)
	theirObjP->SetThrustFromVelocity ();
//------------ Welcome them back if reconnecting --------------
if (!gameData.multiplayer.players [nTheirPlayer].connected) {
	gameData.multiplayer.players [nTheirPlayer].connected = 1;
	if (gameData.demo.nState == ND_STATE_RECORDING)
		NDRecordMultiReconnect (nTheirPlayer);
	MultiMakeGhostPlayer (nTheirPlayer);
	OBJECTS [theirObjNum].CreateAppearanceEffect ();
	audio.PlaySound (SOUND_HUD_MESSAGE);
	ClipRank (reinterpret_cast<char*> (&netPlayers.players [nTheirPlayer].rank));
	if (gameOpts->multi.bNoRankings)
		HUDInitMessage ("'%s' %s", gameData.multiplayer.players [nTheirPlayer].callsign, TXT_REJOIN);
	else
		HUDInitMessage ("%s'%s' %s",
							 pszRankStrings [netPlayers.players [nTheirPlayer].rank],
							 gameData.multiplayer.players [nTheirPlayer].callsign, TXT_REJOIN);
	MultiSendScore ();
	}
//------------ Parse the extra dataP at the end ---------------
if (new_pd.dataSize>0) {
	// pass pd->data to some parser function....
	MultiProcessBigData (reinterpret_cast<char*> (new_pd.data), new_pd.dataSize);
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
	CObject	*objP;
	CPlayerData	*playerP;

for (j = 0, playerP = gameData.multiplayer.players; j < MAX_PLAYERS; j++, playerP++)
	nPlayerObjs [j] = playerP->connected ? playerP->nObject : -1;
#if 0
if (gameData.app.nGameMode & GM_MULTI_ROBOTS)
#endif
//	bHaveReactor = 1;	// multiplayer maps do not need a control center ...
nPlayers = 0;
FORALL_OBJS (objP, i) {
	i = objP->Index ();
	t = objP->info.nType;
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
		if ((t == OBJ_REACTOR) || ((t == OBJ_ROBOT) && ROBOTINFO (objP->info.nId).bossFlag))
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
MsgBox (NULL, NULL, 1, TXT_OK, TXT_NET_SYNC_FAILED);
networkData.nStatus = NETSTAT_MENU;
}

//------------------------------------------------------------------------------

static int NetworkCheckMissingFrames (void)
{
if (networkData.nPrevFrame == networkData.sync [0].objs.nFrame - 1)
	return 1;
if (!networkData.nPrevFrame || (networkData.nPrevFrame >= networkData.sync [0].objs.nFrame))
	return 0;
networkData.sync [0].objs.missingFrames.nFrame = networkData.nPrevFrame + 1;
networkData.nJoinState = 2;
NetworkSendMissingObjFrames ();
return -1;
}

//------------------------------------------------------------------------------

inline bool ObjectIsLinked (CObject *objP, short nSegment)
{
if (nSegment != -1) {
	short nObject = objP->Index ();
	for (short i = SEGMENTS [nSegment].m_objects, j = -1; i >= 0; j = i, i = OBJECTS [i].info.nNextInSeg) {
		if (i == nObject) {
			objP->info.nPrevInSeg = j;
			return true;
			}
		}
	}
return false;
}

//------------------------------------------------------------------------------

void NetworkReadObjectPacket (ubyte *dataP)
{
	static int	nPlayer = 0;
	static int	nMode = 0;

	// Object from another net CPlayerData we need to sync with
	CObject		*objP;
	short			nObject, nRemoteObj;
	sbyte			nObjOwner;
	short			nSegment, i;

	int			nObjects = dataP [1];
	int			bufI;

networkData.nPrevFrame = networkData.sync [0].objs.nFrame;
if (gameStates.multi.nGameType == UDP_GAME) {
	bufI = 2;
	NW_GET_SHORT (dataP, bufI, networkData.sync [0].objs.nFrame);
	}
else {
	networkData.sync [0].objs.nFrame = dataP [2];
	bufI = 3;
	}
i = NetworkCheckMissingFrames ();
if (!i) {
	networkData.toSyncPoll = 0;
	return;
	}
else if (i < 0)
	return;
#if DBG
//PrintLog ("Receiving object packet %d (prev: %d)\n", networkData.nPrevFrame, networkData.sync [0].objs.nFrame);
#endif
 for (i = 0; i < nObjects; i++) {
	objP = NULL;
	NW_GET_SHORT (dataP, bufI, nObject);
	NW_GET_BYTE (dataP, bufI, nObjOwner);
	NW_GET_SHORT (dataP, bufI, nRemoteObj);
	if ((nObject == -1) || (nObject == -3)) {
		// Clear CObject array
		nPlayer = nObjOwner;
		ChangePlayerNumTo (nPlayer);
		nMode = 1;
		networkData.nPrevFrame = networkData.sync [0].objs.nFrame - 1;
		if (nObject == -3) {
			if (networkData.nJoinState != 2)
				return;
#if DBG
			PrintLog ("Receiving missing object packets\n");
#endif
			networkData.nJoinState = 3;
			}
		else {
			if (networkData.nJoinState)
				return;
			InitObjects ();
			gameData.objs.nObjects = 0;
			networkData.nJoinState = 1;
			}
		networkData.sync [0].objs.missingFrames.nFrame = 0;
		}
	else if ((nObject == -2) || (nObject == -4)) {	// Special debug checksum marker for entire send
 		if (!nMode && NetworkVerifyObjects (nRemoteObj, gameData.objs.nObjects)) {
			NetworkAbortSync ();
			return;
			}
		gameData.objs.RebuildEffects ();
		networkData.sync [0].objs.nFrame = 0;
		nMode = 0;
		if (networkData.bHaveSync)
			networkData.nStatus = NETSTAT_PLAYING;
		networkData.nJoinState = 4;
		}
	else if (networkData.nJoinState & 1) {
#if 1
		console.printf (CON_DBG, "Got a type 3 object packet!\n");
#endif
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
			Assert (nObject < LEVEL_OBJECTS);
			objP = OBJECTS + nObject;
#if 1//def _DEBUG
			if (objP->info.nSegment >= 0)
				nDbgObj = objP->Index ();
#endif
			objP->Unlink (true);
			while (ObjectIsLinked (objP, objP->info.nSegment))
				objP->UnlinkFromSeg ();
			NW_GET_BYTES (dataP, bufI, &objP->info, sizeof (tBaseObject));
			if (objP->info.nType != OBJ_NONE) {
#if defined(WORDS_BIGENDIAN) || defined(__BIG_ENDIAN__)
				if (gameStates.multi.nGameType >= IPX_GAME)
					SwapObject (objP);
#endif
				nSegment = objP->info.nSegment;
				PrintLog ("receiving object %d (type: %d, segment: %d)\n", nObject, objP->info.nType, nSegment);
				objP->ResetSgmLinks ();
				objP->ResetLinks ();
				objP->info.nAttachedObj = -1;
				objP->Link ();
				if (nSegment < 0)
					nSegment = FindSegByPos (objP->info.position.vPos, -1, 1, 0);
				if (!ObjectIsLinked (objP, nSegment))
					objP->LinkToSeg (nSegment);
				if ((objP->info.nType == OBJ_PLAYER) || (objP->info.nType == OBJ_GHOST))
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
	} // For each CObject in packet
//gameData.objs.nLastObject [0] = gameData.objs.nObjects - 1;
}

//------------------------------------------------------------------------------

