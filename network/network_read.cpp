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
#include "segmath.h"
#include "objeffects.h"
#include "physics.h"
#include "console.h"

#if DBG
int VerifyObjLists (int nObject = -1);
#endif

//------------------------------------------------------------------------------

void NetworkReadEndLevelPacket (ubyte *dataP)
{
	// Special packet for end of level syncing
	int				nPlayer;
	CEndLevelInfo	eli (reinterpret_cast<tEndLevelInfo*> (dataP));

#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
	int i, j;

for (i = 0; i < MAX_NUM_NET_PLAYERS; i++)
	for (j = 0; j < MAX_NUM_NET_PLAYERS; j++)
		*eli.ScoreMatrix (i, j) = INTEL_SHORT (*eli.ScoreMatrix (i, j));
*eli.Kills () = INTEL_SHORT (*eli.Kills ());
*eli.Killed () = INTEL_SHORT (*eli.Killed ());
#endif
nPlayer = *eli.Player ();
Assert (nPlayer != N_LOCALPLAYER);
if (nPlayer >= gameData.multiplayer.nPlayers) {
	Int3 (); // weird, but it an happen in a coop restore game
	return; // if it happens in a coop restore, don't worry about it
	}
if ((networkData.nStatus == NETSTAT_PLAYING) && (eli.Connected () != 0)) {
	CONNECT (nPlayer, eli.Connected ()); // allow to change connection status
	return; // Only accept disconnect packets if we're not out of the level yet
	}
CONNECT (nPlayer, eli.Connected ());
memcpy (&gameData.multigame.score.matrix [nPlayer][0], eli.ScoreMatrix (), MAX_NUM_NET_PLAYERS * sizeof (short));
gameData.multiplayer.players [nPlayer].netKillsTotal = *eli.Kills ();
gameData.multiplayer.players [nPlayer].netKilledTotal = *eli.Killed ();
if ((gameData.multiplayer.players [nPlayer].connected == 1) && (*eli.SecondsLeft () < gameData.reactor.countdown.nSecsLeft))
	gameData.reactor.countdown.nSecsLeft = *eli.SecondsLeft ();
ResetPlayerTimeout (nPlayer, -1);
}

//------------------------------------------------------------------------------

void NetworkReadEndLevelShortPacket (ubyte *dataP)
{
	// Special packet for end of level syncing

	int						nPlayer;
	tEndLevelInfoShort*	eli;

#if 0 //DBG
if (N_LOCALPLAYER)
	audio.PlaySound (SOUND_HUD_MESSAGE);
#endif
eli = reinterpret_cast<tEndLevelInfoShort*> (dataP);
nPlayer = eli->nPlayer;
Assert (nPlayer != N_LOCALPLAYER);
if (nPlayer >= gameData.multiplayer.nPlayers) {
	Int3 (); // weird, but it can happen in a coop restore game
	return; // if it happens in a coop restore, don't worry about it
	}

if ((networkData.nStatus == NETSTAT_PLAYING) && (eli->connected != 0)) {
	CONNECT (nPlayer, eli->connected); // allow to change connection status
	return; // Only accept disconnect packets if we're not out of the level yet
	}
CONNECT (nPlayer, eli->connected);
if ((gameData.multiplayer.players [nPlayer].connected == 1) && (eli->secondsLeft < gameData.reactor.countdown.nSecsLeft))
	gameData.reactor.countdown.nSecsLeft = eli->secondsLeft;
ResetPlayerTimeout (nPlayer, -1);
}

//------------------------------------------------------------------------------

int SetLocalPlayer (CAllNetPlayersInfo* playerInfoP, int nPlayers, int nDefault)
{
	tNetPlayerInfo*	playerP = playerInfoP->m_info.players;
	char					szLocalCallSign [CALLSIGN_LEN+1];
	int					nLocalPlayer = -1;

memcpy (szLocalCallSign, LOCALPLAYER.callsign, CALLSIGN_LEN+1);
for (int i = 0; i < nPlayers; i++, playerP++) {
	if (!CmpLocalPlayer (&playerP->network, playerP->callsign, szLocalCallSign)) {
		if (nLocalPlayer != -1) {
			InfoBox (TXT_ERROR, NULL, BG_STANDARD, 1, TXT_OK, TXT_DUPLICATE_PLAYERS);
			console.printf (CON_DBG, TXT_FOUND_TWICE);
			networkData.nStatus = NETSTAT_MENU;
			return -2;
			}
		ChangePlayerNumTo (nLocalPlayer = i);
		}
	}
return N_LOCALPLAYER = ((nLocalPlayer < 0) ? nDefault : nLocalPlayer);
}

//------------------------------------------------------------------------------

void NetworkProcessSyncPacket (CNetGameInfo * netGameInfoP, int rsinit)
{
	int					i, j;
	tNetPlayerInfo*	playerP;
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
	CNetGameInfo		tmp_info;

if ((gameStates.multi.nGameType >= IPX_GAME) && (netGameInfoP != &netGame)) { // for macintosh -- get the values unpacked to our structure format
	ReceiveFullNetGamePacket (reinterpret_cast<ubyte*> (netGameInfoP), &tmp_info);
	netGameInfoP = &tmp_info;
	}
#endif

if (rsinit)
	playerInfoP = &netPlayers [0];
	// This function is now called by all people entering the netgame.
if (netGameInfoP != &netGame) {
	char *p = reinterpret_cast<char*> (netGameInfoP);
	int i, j = (int) netGame.Size () - 1, s;
	for (i = 0; i < j; i++, p++) {
		s = *reinterpret_cast<ushort*> (p);
		if (s == networkData.nSegmentCheckSum) {
			break;
			}
		else if (((s / 256) + (s % 256) * 256) == networkData.nSegmentCheckSum) {
			break;
			}
		}
	netGame = *netGameInfoP;
	netPlayers [0] = *playerInfoP;
	}
gameData.multiplayer.nPlayers = netGameInfoP->m_info.nNumPlayers;
gameStates.app.nDifficultyLevel = netGameInfoP->m_info.difficulty;
networkData.nStatus = netGameInfoP->m_info.gameStatus;
//Assert (gameStates.app.nFunctionMode != FMODE_GAME);
// New code, 11/27
#if 1
console.printf (1, "netGame.m_info.checksum = %d, calculated checksum = %d.\n",
					 netGame.GetSegmentCheckSum (), networkData.nSegmentCheckSum);
#endif
if (netGame.GetSegmentCheckSum () != networkData.nSegmentCheckSum) {
	if (extraGameInfo [0].bAutoDownload)
		networkData.nStatus = NETSTAT_AUTODL;
	else {
		short nInMenu = gameStates.menus.nInMenu;
		gameStates.menus.nInMenu = 0;
		networkData.nStatus = NETSTAT_MENU;
		InfoBox (TXT_ERROR, NULL, BG_STANDARD, 1, TXT_OK, TXT_NETLEVEL_MISMATCH);
		gameStates.menus.nInMenu = nInMenu;
		}
#if 1//!DBG
		return;
#endif
	}
// Discover my player number
#if DBG
VerifyObjLists (LOCALPLAYER.nObject);
#endif
if (SetLocalPlayer (playerInfoP, gameData.multiplayer.nPlayers, -1) < -1)
	return;
#if DBG
VerifyObjLists (LOCALPLAYER.nObject);
#endif

for (i = 0; i < MAX_NUM_NET_PLAYERS; i++)
	gameData.multiplayer.players [i].netKillsTotal = 0;

for (i = 0, playerP = playerInfoP->m_info.players; i < gameData.multiplayer.nPlayers; i++, playerP++) {
	memcpy (gameData.multiplayer.players [i].callsign, playerP->callsign, CALLSIGN_LEN+1);
	if (gameStates.multi.nGameType >= IPX_GAME) {
#ifdef WORDS_NEED_ALIGNMENT
		uint server;
		memcpy (&server, playerP->network.Network (), 4);
		if (server != 0)
			IpxGetLocalTarget (
				reinterpret_cast<ubyte*> (&server),
				playerInfoP->m_info.players [i].network.Node (),
				gameData.multiplayer.players [i].netAddress);
#else // WORDS_NEED_ALIGNMENT
		if (playerInfoP->m_info.players [i].network.HaveNetwork ())
			IpxGetLocalTarget (
				playerInfoP->m_info.players [i].network.Network (),
				playerInfoP->m_info.players [i].network.Node (),
				gameData.multiplayer.players [i].netAddress);
#endif // WORDS_NEED_ALIGNMENT
		else
			memcpy (gameData.multiplayer.players [i].netAddress, playerInfoP->m_info.players [i].network.Node (), 6);
		}
	gameData.multiplayer.players [i].nPacketsGot = -1;                            // How many packets we got from them
	gameData.multiplayer.players [i].nPacketsSent = 0;                            // How many packets we sent to them
	CONNECT (i, playerP->connected);
	gameData.multiplayer.players [i].netKillsTotal = *netGameInfoP->PlayerKills (i);
	gameData.multiplayer.players [i].netKilledTotal = *netGameInfoP->Killed (i);
	if (networkData.nJoinState || (i != N_LOCALPLAYER))
		gameData.multiplayer.players [i].score = *netGameInfoP->PlayerScore (i);
	for (j = 0; j < MAX_NUM_NET_PLAYERS; j++)
		gameData.multigame.score.matrix [i][j] = *netGameInfoP->Kills (i, j);
	}

if (N_LOCALPLAYER < 0) {
	InfoBox (TXT_ERROR, NULL, BG_STANDARD, 1, TXT_OK, TXT_PLAYER_REJECTED);
	networkData.nStatus = NETSTAT_MENU;
	return;
	}
if (networkData.nJoinState) {
	for (i = 0; i < gameData.multiplayer.nPlayers; i++)
		gameData.multiplayer.players [i].netKilledTotal = *netGameInfoP->Killed (i);
	NetworkProcessMonitorVector (netGameInfoP->GetMonitorVector ());
	LOCALPLAYER.timeLevel = netGameInfoP->GetLevelTime ();
	}
gameData.multigame.score.nTeam [0] = *netGameInfoP->TeamKills (0);
gameData.multigame.score.nTeam [1] = *netGameInfoP->TeamKills (1);
CONNECT (N_LOCALPLAYER, CONNECT_PLAYING);
netPlayers [0].m_info.players [N_LOCALPLAYER].connected = CONNECT_PLAYING;
netPlayers [0].m_info.players [N_LOCALPLAYER].rank = GetMyNetRanking ();
if (!networkData.nJoinState) {
	int	j, bGotTeamSpawnPos = (IsTeamGame) && GotTeamSpawnPos ();
	for (i = 0; i < gameData.multiplayer.nPlayerPositions; i++) {
		if (bGotTeamSpawnPos) {
			j = TeamSpawnPos (i);
			if (j < 0)
				j = *netGame.Locations (i);
			}
		else
			j = *netGame.Locations (i);
#if DBG
		VerifyObjLists (gameData.multiplayer.players [i].nObject);
#endif
		GetPlayerSpawn (j, OBJECTS + gameData.multiplayer.players [i].nObject);
#if DBG
		VerifyObjLists (gameData.multiplayer.players [i].nObject);
#endif
		}
	}
#if DBG
VerifyObjLists (LOCALPLAYER.nObject);
#endif
OBJECTS [LOCALPLAYER.nObject].SetType (OBJ_PLAYER);
#if DBG
VerifyObjLists (LOCALPLAYER.nObject);
#endif
networkData.nStatus = (IAmGameHost () || (networkData.nJoinState >= 4)) ? NETSTAT_PLAYING : NETSTAT_WAITING;
SetFunctionMode (FMODE_GAME);
networkData.bHaveSync = 1;
MultiSortKillList ();
}

//------------------------------------------------------------------------------

void NetworkTrackPackets (int nPlayer, int nPackets)
{
	CPlayerData*	playerP = gameData.multiplayer.players + nPlayer;

if (playerP->nPacketsGot < 0) {
	playerP->nPacketsGot = nPackets;
	networkData.nTotalPacketsGot += nPackets;
	}
else {
	playerP->nPacketsGot++;
	networkData.nTotalPacketsGot++;
	}
ResetPlayerTimeout (nPlayer, -1);
if  (nPackets != playerP->nPacketsGot) {
	networkData.nMissedPackets = nPackets - playerP->nPacketsGot;
	if (nPackets - playerP->nPacketsGot > 0)
		networkData.nTotalMissedPackets += nPackets - playerP->nPacketsGot;
#if 1
	if (networkData.nMissedPackets > 0)
		console.printf (0,
			"Missed %d packets from player #%d (%d total)\n",
			nPackets-playerP->nPacketsGot,
			nPlayer,
			networkData.nMissedPackets);
	else
		console.printf (CON_DBG,
			"Got %d late packets from player #%d (%d total)\n",
			playerP->nPacketsGot-nPackets,
			nPlayer,
			networkData.nMissedPackets);
#endif
	playerP->nPacketsGot = nPackets;
	}
}

//------------------------------------------------------------------------------

void NetworkReadPDataLongPacket (tFrameInfoLong *pd)
{
	int		nPlayer;
	int		theirObjNum;
	CObject* objP = NULL;

// tFrameInfoLong should be aligned...for mac, make the necessary adjustments
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
if (gameStates.multi.nGameType >= IPX_GAME) {
	pd->nPackets = INTEL_INT (pd->nPackets);
	pd->objPos.dir.coord.x = INTEL_INT (pd->objPos.dir.coord.x);
	pd->objPos.dir.coord.y = INTEL_INT (pd->objPos.dir.coord.y);
	pd->objPos.dir.coord.z = INTEL_INT (pd->objPos.dir.coord.z);
	pd->objOrient.mat.dir.r.dir.coord.x = (fix)INTEL_INT ((int)pd->objOrient.mat.dir.r.dir.coord.x);
	pd->objOrient.mat.dir.r.dir.coord.y = (fix)INTEL_INT ((int)pd->objOrient.mat.dir.r.dir.coord.y);
	pd->objOrient.mat.dir.r.dir.coord.z = (fix)INTEL_INT ((int)pd->objOrient.mat.dir.r.dir.coord.z);
	pd->objOrient.mat.dir.u.dir.coord.x = (fix)INTEL_INT ((int)pd->objOrient.mat.dir.u.dir.coord.x);
	pd->objOrient.mat.dir.u.dir.coord.y = (fix)INTEL_INT ((int)pd->objOrient.mat.dir.u.dir.coord.y);
	pd->objOrient.mat.dir.u.dir.coord.z = (fix)INTEL_INT ((int)pd->objOrient.mat.dir.u.dir.coord.z);
	pd->objOrient.mat.dir.f.dir.coord.x = (fix)INTEL_INT ((int)pd->objOrient.mat.dir.f.dir.coord.x);
	pd->objOrient.mat.dir.f.dir.coord.y = (fix)INTEL_INT ((int)pd->objOrient.mat.dir.f.dir.coord.y);
	pd->objOrient.mat.dir.f.dir.coord.z = (fix)INTEL_INT ((int)pd->objOrient.mat.dir.f.dir.coord.z);
	pd->physVelocity.dir.coord.x = (fix)INTEL_INT ((int)pd->physVelocity.dir.coord.x);
	pd->physVelocity.dir.coord.y = (fix)INTEL_INT ((int)pd->physVelocity.dir.coord.y);
	pd->physVelocity.dir.coord.z = (fix)INTEL_INT ((int)pd->physVelocity.dir.coord.z);
	pd->physRotVel.dir.coord.x = (fix)INTEL_INT ((int)pd->physRotVel.dir.coord.x);
	pd->physRotVel.dir.coord.y = (fix)INTEL_INT ((int)pd->physRotVel.dir.coord.y);
	pd->physRotVel.dir.coord.z = (fix)INTEL_INT ((int)pd->physRotVel.dir.coord.z);
	pd->nObjSeg = INTEL_SHORT (pd->nObjSeg);
	pd->dataSize = INTEL_SHORT (pd->dataSize);
	}
#endif

nPlayer = pd->nPlayer;
theirObjNum = gameData.multiplayer.players [nPlayer].nObject;
if (nPlayer < 0) {
	Int3 (); // This packet is bogus!!
	return;
	}
if ((networkData.sync [0].nPlayer != -1) && (nPlayer == networkData.sync [0].nPlayer))
	networkData.sync [0].nPlayer = -1;
if (!gameData.multigame.bQuitGame && (nPlayer >= gameData.multiplayer.nPlayers)) {
	if (networkData.nStatus != NETSTAT_WAITING) {
		Int3 (); // We missed an important packet!
		//NetworkConsistencyError ();
		}
	return;
	}

if (gameStates.app.bEndLevelSequence || (networkData.nStatus == NETSTAT_ENDLEVEL)) {
	int oldEndlevelSequence = gameStates.app.bEndLevelSequence;
	gameStates.app.bEndLevelSequence = 1;
	if (pd->dataSize > 0)
		// pass pd->data to some parser function....
		MultiProcessBigData (reinterpret_cast<char*> (pd->data), pd->dataSize);
	gameStates.app.bEndLevelSequence = oldEndlevelSequence;
	return;
	}
gameData.multiplayer.players [pd->nPlayer].m_nLevel = pd->nLevel;
if ((sbyte)pd->nLevel != missionManager.nCurrentLevel) {
#if 1
	console.printf (CON_DBG, "Got frame packet from CPlayerData %d wrong level %d!\n", pd->nPlayer, pd->nLevel);
#endif
	return;
	}

objP = OBJECTS + theirObjNum;
NetworkTrackPackets (nPlayer, pd->nPackets);
//------------ Read the player's ship's object info ----------------------
objP->info.position.vPos = pd->objPos;
objP->info.position.mOrient = pd->objOrient;
objP->mType.physInfo.velocity = pd->physVelocity;
objP->mType.physInfo.rotVel = pd->physRotVel;
if ((objP->info.renderType != pd->objRenderType) && (pd->objRenderType == RT_POLYOBJ))
	MultiMakeGhostPlayer (nPlayer);
OBJECTS [theirObjNum].RelinkToSeg (pd->nObjSeg);
if (objP->info.movementType == MT_PHYSICS)
	objP->SetThrustFromVelocity ();
//------------ Welcome them back if reconnecting --------------
if (!gameData.multiplayer.players [nPlayer].connected) {
	if (gameData.multiplayer.players [nPlayer].HasLeft ())
		return;
	CONNECT (nPlayer, CONNECT_PLAYING);
	if (gameData.demo.nState == ND_STATE_RECORDING)
		NDRecordMultiReconnect (nPlayer);
	MultiMakeGhostPlayer (nPlayer);
	OBJECTS [theirObjNum].CreateAppearanceEffect ();
	audio.PlaySound (SOUND_HUD_MESSAGE);
	ClipRank (reinterpret_cast<char*> (&netPlayers [0].m_info.players [nPlayer].rank));
	if (gameOpts->multi.bNoRankings)
		HUDInitMessage ("'%s' %s", gameData.multiplayer.players [nPlayer].callsign, TXT_REJOIN);
	else
		HUDInitMessage ("%s'%s' %s",
							 pszRankStrings [netPlayers [0].m_info.players [nPlayer].rank],
							 gameData.multiplayer.players [nPlayer].callsign, TXT_REJOIN);
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
	int m_bufI = 0;

NW_GET_BYTE (old_info, m_bufI, new_info->nType);
/* skip three for pad byte */
m_bufI += 3;
NW_GET_INT (old_info, m_bufI, new_info->nPackets);
NW_GET_BYTES (old_info, m_bufI, new_info->objPos.orient, 9);
NW_GET_SHORT (old_info, m_bufI, new_info->objPos.coord [0]);
NW_GET_SHORT (old_info, m_bufI, new_info->objPos.coord [1]);
NW_GET_SHORT (old_info, m_bufI, new_info->objPos.coord [2]);
NW_GET_SHORT (old_info, m_bufI, new_info->objPos.nSegment);
NW_GET_SHORT (old_info, m_bufI, new_info->objPos.vel [0]);
NW_GET_SHORT (old_info, m_bufI, new_info->objPos.vel [1]);
NW_GET_SHORT (old_info, m_bufI, new_info->objPos.vel [2]);
NW_GET_SHORT (old_info, m_bufI, new_info->dataSize);
NW_GET_BYTE (old_info, m_bufI, new_info->nPlayer);
NW_GET_BYTE (old_info, m_bufI, new_info->objRenderType);
NW_GET_BYTE (old_info, m_bufI, new_info->nLevel);
NW_GET_BYTES (old_info, m_bufI, new_info->data, new_info->dataSize);
}
#else

#define GetShortFrameInfo(old_info, new_info) \
	memcpy (new_info, old_info, sizeof (tFrameInfoShort))

#endif

//------------------------------------------------------------------------------

void NetworkReadPDataShortPacket (tFrameInfoShort *pd)
{
	int nPlayer;
	int nObject;
	CObject * objP = NULL;

// short frame info is not aligned because of tShortPos.  The mac
// will call totally hacked and gross function to fix this up.
#if 0
tFrameInfoShort new_pd;
if (gameStates.multi.nGameType >= IPX_GAME)
	GetShortFrameInfo (reinterpret_cast<ubyte*> (pd), &new_pd);
else
	memcpy (&new_pd, reinterpret_cast<ubyte*> (pd), sizeof (tFrameInfoShort));
#endif

nPlayer = pd->nPlayer;
nObject = gameData.multiplayer.players [pd->nPlayer].nObject;
if (nPlayer < 0) {
	Int3 (); // This packet is bogus!!
	return;
	}
if (!gameData.multigame.bQuitGame && (nPlayer >= gameData.multiplayer.nPlayers)) {
	if (networkData.nStatus != NETSTAT_WAITING) {
		Int3 (); // We missed an important packet!
		NetworkConsistencyError ();
		}
	return;
	}
if ((networkData.sync [0].nPlayer != -1) && (nPlayer == networkData.sync [0].nPlayer)) {
	// Hurray! Someone really really got in the game (I think).
   networkData.sync [0].nPlayer = -1;
	}

if (gameStates.app.bEndLevelSequence || (networkData.nStatus == NETSTAT_ENDLEVEL)) {
	int oldEndlevelSequence = gameStates.app.bEndLevelSequence;
	gameStates.app.bEndLevelSequence = 1;
	if (pd->dataSize > 0) {
		// pass pd->data to some parser function....
		MultiProcessBigData (reinterpret_cast<char*> (pd->data), pd->dataSize);
		}
	gameStates.app.bEndLevelSequence = oldEndlevelSequence;
	return;
	}
gameData.multiplayer.players [pd->nPlayer].m_nLevel = pd->nLevel;
if ((sbyte) pd->nLevel != missionManager.nCurrentLevel) {
#if 1
	console.printf (CON_DBG, "Got frame packet from player %d wrong level %d!\n", pd->nPlayer, pd->nLevel);
#endif
	return;
	}
objP = OBJECTS + nObject;
NetworkTrackPackets (nPlayer, pd->nPackets);
//------------ Read the player's ship's CObject info ----------------------
ExtractShortPos (objP, &pd->objPos, 0);
if ((objP->info.renderType != pd->objRenderType) && (pd->objRenderType == RT_POLYOBJ))
	MultiMakeGhostPlayer (nPlayer);
if (objP->info.movementType == MT_PHYSICS)
	objP->SetThrustFromVelocity ();
//------------ Welcome them back if reconnecting --------------
if (!gameData.multiplayer.players [nPlayer].connected) {
	if (gameData.multiplayer.players [nPlayer].HasLeft ())
		return;
	CONNECT (nPlayer, CONNECT_PLAYING);
	if (gameData.demo.nState == ND_STATE_RECORDING)
		NDRecordMultiReconnect (nPlayer);
	MultiMakeGhostPlayer (nPlayer);
	OBJECTS [nObject].CreateAppearanceEffect ();
	audio.PlaySound (SOUND_HUD_MESSAGE);
	ClipRank (reinterpret_cast<char*> (&netPlayers [0].m_info.players [nPlayer].rank));
	if (gameOpts->multi.bNoRankings)
		HUDInitMessage ("'%s' %s", gameData.multiplayer.players [nPlayer].callsign, TXT_REJOIN);
	else
		HUDInitMessage ("%s'%s' %s",
							 pszRankStrings [netPlayers [0].m_info.players [nPlayer].rank],
							 gameData.multiplayer.players [nPlayer].callsign, TXT_REJOIN);
	MultiSendScore ();
	}
//------------ Parse the extra dataP at the end ---------------
if (pd->dataSize > 0) {
	// pass pd->data to some parser function....
	MultiProcessBigData (reinterpret_cast<char*> (pd->data), pd->dataSize);
	}
}

//------------------------------------------------------------------------------

int NetworkVerifyPlayers (void)
{
	int				i, j, t, bCoop = IsCoopGame;
	int				nPlayers, nPlayerObjs [MAX_PLAYERS];
	CObject*			objP;
	CPlayerData*	playerP;

for (j = 0, playerP = gameData.multiplayer.players; j < MAX_PLAYERS; j++, playerP++)
	nPlayerObjs [j] = playerP->connected ? playerP->nObject : -1;
#if 0
if (gameData.app.GameMode (GM_MULTI_ROBOTS))
#endif
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
#if 0
	else if (bCoop) {
		}
#endif
	if (nPlayers > gameData.multiplayer.nMaxPlayers)
		return 1;
	}
return 0;
}

//------------------------------------------------------------------------------

void NetworkAbortSync (void)
{
TextBox (NULL, BG_STANDARD, 1, TXT_OK, TXT_NET_SYNC_FAILED);
networkData.nStatus = NETSTAT_MENU;
}

//------------------------------------------------------------------------------

class CObjectSynchronizer {
	private:
		int		m_nPlayer;
		sbyte		m_nObjOwner;
		short		m_nLocalObj;
		short		m_nRemoteObj;
		ushort	m_nFrame;
		int		m_nState;
		int		m_bufI;
		ubyte*	m_data;

	public:
		int Run (ubyte* dataP);

	private:
		void Abort (void);
		void RequestResync (void);
		int CompareFrames (void);
		int ValidateFrame (void);
		int Start (void);
		int Finish (void);
		int Sync (void);
		int Validate (void);
};

CObjectSynchronizer objectSynchronizer;

bool ObjectIsLinked (CObject *objP, short nSegment);
void ResetSyncTimeout (bool bInit = false);

//------------------------------------------------------------------------------

void CObjectSynchronizer::Abort (void)
{
NetworkAbortSync ();
}

//------------------------------------------------------------------------------

void CObjectSynchronizer::RequestResync (void)
{
networkData.nJoinState = 2;
NetworkSendMissingObjFrames ();
}

//------------------------------------------------------------------------------

int CObjectSynchronizer::CompareFrames (void)
{
if (networkData.nPrevFrame == m_nFrame - 1)
	return 1;
if (networkData.nPrevFrame >= m_nFrame)
	return -1;
if (networkData.nPrevFrame < 0)
	return -1;
#if 0
networkData.sync [0].objs.missingFrames.nFrame = m_nRemoteObj + 1; 
#else
networkData.sync [0].objs.missingFrames.nFrame = networkData.nPrevFrame + 1;
#endif
RequestResync ();
return 0;
}

//------------------------------------------------------------------------------

int CObjectSynchronizer::ValidateFrame (void)
{
if (networkData.nJoinState > 1)
	return 0;
networkData.nPrevFrame = networkData.sync [0].objs.nFrame;
if (gameStates.multi.nGameType == UDP_GAME) {
	m_bufI = 2;
	NW_GET_SHORT (m_data, m_bufI, m_nFrame);
	}
else {
	m_nFrame = m_data [2];
	m_bufI = 3;
	}
int syncRes = CompareFrames ();
if (syncRes < 0) {
	if (!syncRes) 
		networkData.toSyncPoll.Start (0);
	return syncRes;
	}
ResetSyncTimeout (true);
networkData.sync [0].objs.nFrame = m_nFrame;
return 1;
}

//------------------------------------------------------------------------------

int CObjectSynchronizer::Start (void)
{
if ((m_nLocalObj != -1) && (m_nLocalObj != -3))
	return 0;
#if 0 // disallow sync restart
if (networkData.nJoinState)
	return -1;
#endif
// Clear object list
m_nPlayer = m_nObjOwner;
networkData.nPrevFrame = networkData.sync [0].objs.nFrame - 1;
InitObjects (false);
ChangePlayerNumTo (m_nPlayer);
InitMultiPlayerObject (1);
m_nLocalObj =
m_nRemoteObj = -1;
gameData.objs.nObjects = 0;
networkData.nJoinState = 1;
networkData.sync [0].objs.missingFrames.nFrame = 0;
m_nState = 1;
#if DBG
VerifyObjLists (N_LOCALPLAYER);
#endif
return 1;
}

//------------------------------------------------------------------------------
// The object sync'er places all incoming objects at the same object table
// entries as on the remote sync'ing system. If the table entries of the current
// local and remote objects are too far apart, too much has changed in the 
// remote (reference's) system's game state and the local game state

int CObjectSynchronizer::Validate (void)
{
return (/*!gameStates.app.bHaveExtraGameInfo [1] &&*/ (m_nRemoteObj - m_nLocalObj > 10)) ? -1 : 0;
}

//------------------------------------------------------------------------------

int CObjectSynchronizer::Finish (void)
{
if (m_nLocalObj != -2) 
	return 0;

if (!m_nState || !Validate ()) {
	Abort ();
	return -1;
	}

NetworkCountPowerupsInMine ();
gameData.objs.RebuildEffects ();
networkData.sync [0].objs.nFrame = 0;
m_nState = 0;
if (networkData.bHaveSync)
	networkData.nStatus = NETSTAT_PLAYING;
networkData.nJoinState = 4;
#if DBG
VerifyObjLists (N_LOCALPLAYER);
#endif
return 1;
}

//------------------------------------------------------------------------------

int CObjectSynchronizer::Sync (void)
{
if (networkData.nJoinState != 1)
	return 0;

// try to allocate each object in the same object list slot as the game host
// this is particularly important for the player objects, as these get used before they actually get allocated
m_nLocalObj = AllocObject (m_nRemoteObj); 
if ((m_nObjOwner != N_LOCALPLAYER) && (m_nObjOwner != -1)) 
	m_nState = 0;
else { // for the local player and for unknown object owners, the new object must be allocated at the same object list position as on the remote server!
	if (/*(m_nState != 1) ||*/ (m_nLocalObj != m_nRemoteObj)) { // since the object allocator tries its best to do so, ignore the sync state here if that requirement could me met
		RequestResync ();
		return -1;
		}
	}
if (m_nLocalObj < 0) 
	return 0;

CObject* objP = OBJECTS + m_nLocalObj;
objP->Unlink (true);
while (ObjectIsLinked (objP, objP->info.nSegment))
	objP->UnlinkFromSeg ();
NW_GET_BYTES (m_data, m_bufI, &objP->info, sizeof (tBaseObject));
if (objP->info.nType != OBJ_NONE) {
#if defined(WORDS_BIGENDIAN) || defined(__BIG_ENDIAN__)
	if (gameStates.multi.nGameType >= IPX_GAME)
		SwapObject (objP);
#endif
	int nSegment = objP->info.nSegment;
	PrintLog (0, "receiving object %d (type: %d, segment: %d)\n", m_nLocalObj, objP->info.nType, nSegment);
	objP->ResetSgmLinks ();
	objP->ResetLinks ();
	objP->info.nAttachedObj = -1;
	objP->Link ();
	if (nSegment < 0) {
		nSegment = FindSegByPos (objP->info.position.vPos, -1, 1, 0);
		if (nSegment < 0) {
#if DBG
			nSegment = FindSegByPos (objP->info.position.vPos, -1, 1, 0); 
#endif
			nSegment = FindSegByPos (objP->info.position.vPos, -1, 0, 0);
			if (nSegment < 0) {
				NetworkAbortSync ();
				return -1;
				}
			}
		}
	if (!ObjectIsLinked (objP, nSegment))
		objP->LinkToSeg (nSegment);
	if ((objP->info.nType == OBJ_PLAYER) || (objP->info.nType == OBJ_GHOST))
		RemapLocalPlayerObject (m_nLocalObj, m_nRemoteObj);
#if 1
	SetObjNumMapping (m_nLocalObj, m_nRemoteObj, m_nObjOwner);
#else
	if (m_nObjOwner == m_nPlayer)
		SetLocalObjNumMapping (m_nLocalObj);
	else if (m_nObjOwner != -1)
		SetObjNumMapping (m_nLocalObj, m_nRemoteObj, m_nObjOwner);
	else
		gameData.multigame.m_nObjOwner [m_nLocalObj] = -1;
#endif
	if (objP->Type () == OBJ_MONSTERBALL) {
		gameData.hoard.monsterballP = objP;
		gameData.hoard.nMonsterballSeg = nSegment;
		}
	}
#if DBG
VerifyObjLists (objP->Index ());
#endif
return 1;
}

//------------------------------------------------------------------------------

int CObjectSynchronizer::Run (ubyte* dataP)
{
m_data = dataP;

int syncRes = ValidateFrame ();
if (syncRes < 1)
	return syncRes;

int nObjects = m_data [1];

for (int i = 0; (i < nObjects) && (syncRes > -1); i++) {
#if DBG
	VerifyObjLists (LOCALPLAYER.nObject);
#endif
	NW_GET_SHORT (m_data, m_bufI, m_nLocalObj);
	NW_GET_BYTE (m_data, m_bufI, m_nObjOwner);
	NW_GET_SHORT (m_data, m_bufI, m_nRemoteObj);
	if ((syncRes = Start ()))
		continue;
	if ((syncRes = Finish ()))
		continue;
	Sync ();
	} 
return syncRes;
}

//------------------------------------------------------------------------------

void NetworkReadObjectPacket (ubyte* dataP)
{
objectSynchronizer.Run (dataP);
}

//------------------------------------------------------------------------------

