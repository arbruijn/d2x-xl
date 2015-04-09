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

//------------------------------------------------------------------------------

void NetworkReadEndLevelPacket (uint8_t *data)
{
	// Special packet for end of level syncing
	int32_t				nPlayer;
	CEndLevelInfo	eli (reinterpret_cast<tEndLevelInfo*> (data));

#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
	int32_t i, j;

for (i = 0; i < MAX_NUM_NET_PLAYERS; i++)
	for (j = 0; j < MAX_NUM_NET_PLAYERS; j++)
		*eli.ScoreMatrix (i, j) = INTEL_SHORT (*eli.ScoreMatrix (i, j));
*eli.Kills () = INTEL_SHORT (*eli.Kills ());
*eli.Killed () = INTEL_SHORT (*eli.Killed ());
#endif
nPlayer = *eli.Player ();
Assert (nPlayer != N_LOCALPLAYER);
if (nPlayer >= N_PLAYERS) {
	Int3 (); // weird, but it an happen in a coop restore game
	return; // if it happens in a coop restore, don't worry about it
	}
if ((networkData.nStatus == NETSTAT_PLAYING) && (eli.Connected () != 0)) {
	CONNECT (nPlayer, eli.Connected ()); // allow to change connection status
	return; // Only accept disconnect packets if we're not out of the level yet
	}
CONNECT (nPlayer, eli.Connected ());
memcpy (&gameData.multigame.score.matrix [nPlayer][0], eli.ScoreMatrix (), MAX_NUM_NET_PLAYERS * sizeof (int16_t));
PLAYER (nPlayer).netKillsTotal = *eli.Kills ();
PLAYER (nPlayer).netKilledTotal = *eli.Killed ();
if ((PLAYER (nPlayer).connected == 1) && (*eli.SecondsLeft () < gameData.reactor.countdown.nSecsLeft))
	gameData.reactor.countdown.nSecsLeft = *eli.SecondsLeft ();
ResetPlayerTimeout (nPlayer, -1);
}

//------------------------------------------------------------------------------

void NetworkReadEndLevelShortPacket (uint8_t *data)
{
	// Special packet for end of level syncing

	int32_t						nPlayer;
	tEndLevelInfoShort*	eli;

#if 0 //DBG
if (N_LOCALPLAYER)
	audio.PlaySound (SOUND_HUD_MESSAGE);
#endif
eli = reinterpret_cast<tEndLevelInfoShort*> (data);
nPlayer = eli->nPlayer;
Assert (nPlayer != N_LOCALPLAYER);
if (nPlayer >= N_PLAYERS) {
	Int3 (); // weird, but it can happen in a coop restore game
	return; // if it happens in a coop restore, don't worry about it
	}

if ((networkData.nStatus == NETSTAT_PLAYING) && (eli->connected != 0)) {
	CONNECT (nPlayer, eli->connected); // allow to change connection status
	return; // Only accept disconnect packets if we're not out of the level yet
	}
CONNECT (nPlayer, eli->connected);
if ((PLAYER (nPlayer).connected == 1) && (eli->secondsLeft < gameData.reactor.countdown.nSecsLeft))
	gameData.reactor.countdown.nSecsLeft = eli->secondsLeft;
ResetPlayerTimeout (nPlayer, -1);
}

//------------------------------------------------------------------------------

int32_t SetLocalPlayer (CAllNetPlayersInfo* playerInfoP, int32_t nPlayers, int32_t nDefault)
{
	tNetPlayerInfo*	playerP = playerInfoP->m_info.players;
	char					szLocalCallSign [CALLSIGN_LEN+1];
	int32_t					nLocalPlayer = -1;

memcpy (szLocalCallSign, LOCALPLAYER.callsign, CALLSIGN_LEN+1);
for (int32_t i = 0; i < nPlayers; i++, playerP++) {
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

void NetworkProcessSyncPacket (CNetGameInfo * netGameInfoP, int32_t rsinit)
{
	int32_t					i, j;
	tNetPlayerInfo*	playerP;
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
	CNetGameInfo		tmp_info;

if ((gameStates.multi.nGameType >= IPX_GAME) && (netGameInfoP != &netGameInfo)) { // for macintosh -- get the values unpacked to our structure format
	ReceiveFullNetGamePacket (reinterpret_cast<uint8_t*> (netGameInfoP), &tmp_info);
	netGameInfoP = &tmp_info;
	}
#endif

if (rsinit)
	playerInfoP = &netPlayers [0];
	// This function is now called by all people entering the netgame.
if (netGameInfoP != &netGameInfo) {
	char *p = reinterpret_cast<char*> (netGameInfoP);
	int32_t i, j = (int32_t) netGameInfo.Size () - 1, s;
	for (i = 0; i < j; i++, p++) {
		s = *reinterpret_cast<uint16_t*> (p);
		if (s == networkData.nSegmentCheckSum) {
			break;
			}
		else if (((s / 256) + (s % 256) * 256) == networkData.nSegmentCheckSum) {
			break;
			}
		}
	netGameInfo = *netGameInfoP;
	netPlayers [0] = *playerInfoP;
	}
N_PLAYERS = netGameInfoP->m_info.nNumPlayers;
gameStates.app.nDifficultyLevel = netGameInfoP->m_info.difficulty;
networkData.nStatus = netGameInfoP->m_info.gameStatus;
//Assert (gameStates.app.nFunctionMode != FMODE_GAME);
// New code, 11/27
#if 1
console.printf (1, "netGameInfo.m_info.checksum = %d, calculated checksum = %d.\n",
					 netGameInfo.GetSegmentCheckSum (), networkData.nSegmentCheckSum);
#endif
if (netGameInfo.GetSegmentCheckSum () != networkData.nSegmentCheckSum) {
	if (extraGameInfo [0].bAutoDownload)
		networkData.nStatus = NETSTAT_AUTODL;
	else {
		int16_t nInMenu = gameStates.menus.nInMenu;
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
if (SetLocalPlayer (playerInfoP, N_PLAYERS, -1) < -1)
	return;

for (i = 0; i < MAX_NUM_NET_PLAYERS; i++)
	PLAYER (i).netKillsTotal = 0;

for (i = 0, playerP = playerInfoP->m_info.players; i < N_PLAYERS; i++, playerP++) {
	memcpy (PLAYER (i).callsign, playerP->callsign, CALLSIGN_LEN+1);
	if (gameStates.multi.nGameType >= IPX_GAME) {
#ifdef WORDS_NEED_ALIGNMENT
		uint32_t server;
		memcpy (&server, playerP->network.Network (), 4);
		if (server != 0)
			IpxGetLocalTarget (
				reinterpret_cast<uint8_t*> (&server),
				playerInfoP->m_info.players [i].network.Node (),
				PLAYER (i).netAddress);
#else // WORDS_NEED_ALIGNMENT
		if (playerInfoP->m_info.players [i].network.HaveNetwork ())
			IpxGetLocalTarget (
				playerInfoP->m_info.players [i].network.Network (),
				playerInfoP->m_info.players [i].network.Node (),
				PLAYER (i).netAddress);
#endif // WORDS_NEED_ALIGNMENT
		else
			memcpy (PLAYER (i).netAddress, playerInfoP->m_info.players [i].network.Node (), 6);
		}
	PLAYER (i).nPacketsGot = -1;                            // How many packets we got from them
	PLAYER (i).nPacketsSent = 0;                            // How many packets we sent to them
	CONNECT (i, playerP->connected);
	PLAYER (i).netKillsTotal = *netGameInfoP->PlayerKills (i);
	PLAYER (i).netKilledTotal = *netGameInfoP->Killed (i);
	if (networkData.nJoinState || (i != N_LOCALPLAYER))
		PLAYER (i).score = *netGameInfoP->PlayerScore (i);
	for (j = 0; j < MAX_NUM_NET_PLAYERS; j++)
		gameData.multigame.score.matrix [i][j] = *netGameInfoP->Kills (i, j);
	}

if (N_LOCALPLAYER < 0) {
	InfoBox (TXT_ERROR, NULL, BG_STANDARD, 1, TXT_OK, TXT_PLAYER_REJECTED);
	networkData.nStatus = NETSTAT_MENU;
	return;
	}
if (networkData.nJoinState) {
	for (i = 0; i < N_PLAYERS; i++)
		PLAYER (i).netKilledTotal = *netGameInfoP->Killed (i);
	NetworkProcessMonitorVector (netGameInfoP->GetMonitorVector ());
	LOCALPLAYER.timeLevel = netGameInfoP->GetLevelTime ();
	}
gameData.multigame.score.nTeam [0] = *netGameInfoP->TeamKills (0);
gameData.multigame.score.nTeam [1] = *netGameInfoP->TeamKills (1);
CONNECT (N_LOCALPLAYER, CONNECT_PLAYING);
NETPLAYER (N_LOCALPLAYER).connected = CONNECT_PLAYING;
NETPLAYER (N_LOCALPLAYER).rank = GetMyNetRanking ();
if (!networkData.nJoinState) {
	int32_t	j, bGotTeamSpawnPos = (IsTeamGame) && GotTeamSpawnPos ();
	for (i = 0; i < gameData.multiplayer.nPlayerPositions; i++) {
		if (bGotTeamSpawnPos) {
			j = TeamSpawnPos (i);
			if (j < 0)
				j = *netGameInfo.Locations (i);
			}
		else
			j = *netGameInfo.Locations (i);
		MovePlayerToSpawnPos (j, OBJECT (PLAYER (i).nObject));
		}
	}
LOCALOBJECT->SetType (OBJ_PLAYER);
networkData.nStatus = (IAmGameHost () || (networkData.nJoinState >= 4)) ? NETSTAT_PLAYING : NETSTAT_WAITING;
SetFunctionMode (FMODE_GAME);
networkData.bHaveSync = 1;
MultiSortKillList ();
}

//------------------------------------------------------------------------------

void NetworkTrackPackets (int32_t nPlayer, int32_t nPackets)
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

void NetworkReadLongPlayerDataPacket (tFrameInfoLong *pd)
{
	int32_t		nPlayer;
	int32_t		nObject;
	CObject*		objP = NULL;

// tFrameInfoLong should be aligned...for mac, make the necessary adjustments
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
if (gameStates.multi.nGameType >= IPX_GAME) {
	pd->header.nPackets = INTEL_INT (pd->header.nPackets);
	pd->objData.nSegment = INTEL_SHORT (pd->objData.nSegment);
	INTEL_VECTOR (pd->objData.pos);
	INTEL_MATRIX (pd->objData.orient);
	INTEL_VECTOR (pd->objData.vel);
	INTEL_VECTOR (pd->objData.rotVel);
	pd->data.dataSize = INTEL_SHORT (pd->data.dataSize);
	}
#endif

nPlayer = pd->data.nPlayer;
if (nPlayer < 0) {
	Int3 (); // This packet is bogus!!
	return;
	}
nObject = PLAYER (nPlayer).nObject;
if ((networkData.syncInfo [0].nPlayer != -1) && (nPlayer == networkData.syncInfo [0].nPlayer))
	networkData.syncInfo [0].nPlayer = -1;
if (!gameData.multigame.bQuitGame && (nPlayer >= N_PLAYERS)) {
	if (networkData.nStatus != NETSTAT_WAITING) {
		Int3 (); // We missed an important packet!
		//NetworkConsistencyError ();
		}
	return;
	}

if (gameStates.app.bEndLevelSequence || (networkData.nStatus == NETSTAT_ENDLEVEL)) {
	int32_t oldEndlevelSequence = gameStates.app.bEndLevelSequence;
	gameStates.app.bEndLevelSequence = 1;
	if (pd->data.dataSize > 0)
		// pass pd->data to some parser function....
		MultiProcessBigData (pd->data.msgData, pd->data.dataSize);
	gameStates.app.bEndLevelSequence = oldEndlevelSequence;
	return;
	}
PLAYER (nPlayer).m_nLevel = pd->data.nLevel;
if ((int8_t) pd->data.nLevel != missionManager.nCurrentLevel) {
#if 1
	console.printf (CON_DBG, "Got frame packet from CPlayerData %d wrong level %d!\n", nPlayer, pd->data.nLevel);
#endif
	return;
	}

objP = OBJECT (nObject);
NetworkTrackPackets (nPlayer, pd->header.nPackets);
//------------ Read the player's ship's object info ----------------------
objP->info.position.vPos = pd->objData.pos;
objP->info.position.mOrient = pd->objData.orient;
objP->mType.physInfo.velocity = pd->objData.vel;
objP->mType.physInfo.rotVel = pd->objData.rotVel;
PLAYER (nPlayer).m_flightPath.Update (objP);
if ((objP->info.renderType != pd->data.nRenderType) && (pd->data.nRenderType == RT_POLYOBJ))
	MultiMakeGhostPlayer (nPlayer);
OBJECT (nObject)->RelinkToSeg (pd->objData.nSegment);
if (objP->info.movementType == MT_PHYSICS)
	objP->SetThrustFromVelocity ();
//------------ Welcome them back if reconnecting --------------
if (!PLAYER (nPlayer).connected) {
	if (PLAYER (nPlayer).HasLeft ())
		return;
	CONNECT (nPlayer, CONNECT_PLAYING);
	if (gameData.demo.nState == ND_STATE_RECORDING)
		NDRecordMultiReconnect (nPlayer);
	MultiMakeGhostPlayer (nPlayer);
	OBJECT (nObject)->CreateAppearanceEffect ();
	audio.PlaySound (SOUND_HUD_MESSAGE);
	ClipRank (reinterpret_cast<char*> (&NETPLAYER (nPlayer).rank));
	if (gameOpts->multi.bNoRankings)
		HUDInitMessage ("'%s' %s", PLAYER (nPlayer).callsign, TXT_REJOIN);
	else
		HUDInitMessage ("%s'%s' %s",
							 pszRankStrings [NETPLAYER (nPlayer).rank],
							 PLAYER (nPlayer).callsign, TXT_REJOIN);
	MultiSendScore ();
	}
//------------ Parse the extra data at the end ---------------
if (pd->data.dataSize > 0)
	// pass pd->data to some parser function....
	MultiProcessBigData (pd->data.msgData, pd->data.dataSize);
}

//------------------------------------------------------------------------------

#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)

void GetShortFrameInfo (uint8_t *old_info, tFrameInfoShort *new_info)
{
	int32_t m_bufI = 0;

NW_GET_BYTE (old_info, m_bufI, new_info->nType);
/* skip three for pad byte */
m_bufI += 3;
NW_GET_INT (old_info, m_bufI, new_info->nPackets);
NW_GET_BYTES (old_info, m_bufI, new_info->objData.orient, 9);
NW_GET_SHORT (old_info, m_bufI, new_info->objData.coord [0]);
NW_GET_SHORT (old_info, m_bufI, new_info->objData.coord [1]);
NW_GET_SHORT (old_info, m_bufI, new_info->objData.coord [2]);
NW_GET_SHORT (old_info, m_bufI, new_info->objData.nSegment);
NW_GET_SHORT (old_info, m_bufI, new_info->objData.vel [0]);
NW_GET_SHORT (old_info, m_bufI, new_info->objData.vel [1]);
NW_GET_SHORT (old_info, m_bufI, new_info->objData.vel [2]);
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

void NetworkReadShortPlayerDataPacket (tFrameInfoShort *pd)
{
	int32_t nPlayer;
	int32_t nObject;
	CObject * objP = NULL;

// int16_t frame info is not aligned because of tShortPos.  The mac
// will call totally hacked and gross function to fix this up.
#if 0
tFrameInfoShort new_pd;
if (gameStates.multi.nGameType >= IPX_GAME)
	GetShortFrameInfo (reinterpret_cast<uint8_t*> (pd), &new_pd);
else
	memcpy (&new_pd, reinterpret_cast<uint8_t*> (pd), sizeof (tFrameInfoShort));
#endif

nPlayer = pd->data.nPlayer;
if (nPlayer < 0) {
	Int3 (); // This packet is bogus!!
	return;
	}
nObject = PLAYER (nPlayer).nObject;
if (!gameData.multigame.bQuitGame && (nPlayer >= N_PLAYERS)) {
	if (networkData.nStatus != NETSTAT_WAITING) {
		Int3 (); // We missed an important packet!
		NetworkConsistencyError ();
		}
	return;
	}
if ((networkData.syncInfo [0].nPlayer != -1) && (nPlayer == networkData.syncInfo [0].nPlayer)) {
	// Hurray! Someone really really got in the game (I think).
   networkData.syncInfo [0].nPlayer = -1;
	}

if (gameStates.app.bEndLevelSequence || (networkData.nStatus == NETSTAT_ENDLEVEL)) {
	int32_t oldEndlevelSequence = gameStates.app.bEndLevelSequence;
	gameStates.app.bEndLevelSequence = 1;
	if (pd->data.dataSize > 0) {
		// pass pd->data to some parser function....
		MultiProcessBigData (pd->data.msgData, pd->data.dataSize);
		}
	gameStates.app.bEndLevelSequence = oldEndlevelSequence;
	return;
	}
PLAYER (nPlayer).m_nLevel = pd->data.nLevel;
if ((int8_t) pd->data.nLevel != missionManager.nCurrentLevel) {
#if 1
	console.printf (CON_DBG, "Got frame packet from player %d wrong level %d!\n", nPlayer, pd->data.nLevel);
#endif
	return;
	}
objP = OBJECT (nObject);
NetworkTrackPackets (nPlayer, pd->header.nPackets);
//------------ Read the player's ship's CObject info ----------------------
ExtractShortPos (objP, &pd->objData, 0);
if ((objP->info.renderType != pd->data.nRenderType) && (pd->data.nRenderType == RT_POLYOBJ))
	MultiMakeGhostPlayer (nPlayer);
if (objP->info.movementType == MT_PHYSICS)
	objP->SetThrustFromVelocity ();
//------------ Welcome them back if reconnecting --------------
if (!PLAYER (nPlayer).connected) {
	if (PLAYER (nPlayer).HasLeft ())
		return;
	CONNECT (nPlayer, CONNECT_PLAYING);
	if (gameData.demo.nState == ND_STATE_RECORDING)
		NDRecordMultiReconnect (nPlayer);
	MultiMakeGhostPlayer (nPlayer);
	OBJECT (nObject)->CreateAppearanceEffect ();
	audio.PlaySound (SOUND_HUD_MESSAGE);
	ClipRank (reinterpret_cast<char*> (&NETPLAYER (nPlayer).rank));
	if (gameOpts->multi.bNoRankings)
		HUDInitMessage ("'%s' %s", PLAYER (nPlayer).callsign, TXT_REJOIN);
	else
		HUDInitMessage ("%s'%s' %s",
							 pszRankStrings [NETPLAYER (nPlayer).rank],
							 PLAYER (nPlayer).callsign, TXT_REJOIN);
	MultiSendScore ();
	}
//------------ Parse the extra data at the end ---------------
if (pd->data.dataSize > 0) {
	// pass pd->data to some parser function....
	MultiProcessBigData (pd->data.msgData, pd->data.dataSize);
	}
}

//------------------------------------------------------------------------------

int32_t NetworkVerifyPlayers (void)
{
	int32_t			i, j, t, bCoop = IsCoopGame;
	int32_t			nPlayers, nPlayerObjs [MAX_PLAYERS];
	CObject*			objP;
	CPlayerData*	playerP;

for (j = 0, playerP = gameData.multiplayer.players; j < MAX_PLAYERS; j++, playerP++)
	nPlayerObjs [j] = playerP->connected ? playerP->nObject : -1;
#if 0
if (gameData.app.GameMode (GM_MULTI_ROBOTS))
#endif
nPlayers = 0;
FORALL_OBJS (objP) {
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
		int32_t		m_nPlayer;
		int8_t		m_nObjOwner;
		int16_t		m_nLocalObj;
		int16_t		m_nRemoteObj;
		uint16_t		m_nFrame;
		int32_t		m_nState;
		int32_t		m_bufI;
		uint8_t*		m_data;

	public:
		int32_t Run (uint8_t* data);

	private:
		void Abort (void);
		void RequestResync (void);
		int32_t CompareFrames (void);
		int32_t ValidateFrame (void);
		int32_t Start (void);
		int32_t Finish (void);
		int32_t Sync (void);
		int32_t Validate (void);
};

CObjectSynchronizer objectSynchronizer;

bool ObjectIsLinked (CObject *objP, int16_t nSegment);
void ResetSyncTimeout (bool bInit = false);

//------------------------------------------------------------------------------

void CObjectSynchronizer::Abort (void)
{
NetworkAbortSync ();
}

//------------------------------------------------------------------------------

int32_t NetworkRequestSync (void);

void CObjectSynchronizer::RequestResync (void)
{
NetworkRequestSync ();
}

//------------------------------------------------------------------------------

int32_t CObjectSynchronizer::CompareFrames (void)
{
if (networkData.syncInfo [0].objs.nFrame == m_nFrame - 1) {
	networkData.nJoinState = 1;
	return 1;
	}
if (networkData.nJoinState > 1)
	return 0;
if (networkData.syncInfo [0].objs.nFrame >= m_nFrame)
	return 0;
if (networkData.syncInfo [0].objs.nFrame < 0)
	return -1;
networkData.nJoinState = 2;
RequestResync ();
return -1;
}

//------------------------------------------------------------------------------

int32_t CObjectSynchronizer::ValidateFrame (void)
{
if (gameStates.multi.nGameType == UDP_GAME) {
	m_bufI = 2;
	NW_GET_SHORT (m_data, m_bufI, m_nFrame);
	}
else {
	m_nFrame = m_data [2];
	m_bufI = 3;
	}
int32_t syncRes = CompareFrames ();
if (syncRes > 0)
	networkData.syncInfo [0].objs.nFrame = m_nFrame;
return syncRes;
}

//------------------------------------------------------------------------------

int32_t CObjectSynchronizer::Start (void)
{
if ((m_nLocalObj != -1) && (m_nLocalObj != -3))
	return 0;
#if 0 // disallow sync restart
if (networkData.nJoinState)
	return -1;
#endif
// Clear object list
m_nPlayer = m_nObjOwner;
m_nLocalObj =
m_nRemoteObj = -1;
m_nState = 1;

if (networkData.nJoinState == 2) { // re-sync'ing?
	if (networkData.syncInfo [0].objs.nFramesToSkip) {
		networkData.syncInfo [0].objs.nFrame = networkData.syncInfo [0].objs.nFramesToSkip - 1;
		networkData.syncInfo [0].objs.nFramesToSkip = 0;
		}
	}
else {
	InitObjects (false);
	gameData.objData.nObjects = 0;
	ChangePlayerNumTo (m_nPlayer);
	InitMultiPlayerObject (1);
	//ClaimObjectSlot (LOCALPLAYER.nObject);
	}
networkData.nJoinState = 1;
return 1;
}

//------------------------------------------------------------------------------

int32_t CObjectSynchronizer::Sync (void)
{
if (networkData.nJoinState != 1)
	return 0;

// try to allocate each object in the same object list slot as the game host
// this is particularly important for the player objects, as these get used before they actually get allocated
m_nLocalObj = AllocObject (m_nRemoteObj); 
if (((m_nObjOwner == N_LOCALPLAYER) || (m_nObjOwner == -1)) && (m_nLocalObj != m_nRemoteObj)) { // since the object allocator tries its best to do so, ignore the sync state here if that requirement could be met
	m_nState = 0;
	networkData.nJoinState = 0; // start over
	RequestResync ();
	return -1;
	}

CObject* objP = OBJECT (m_nLocalObj);
NW_GET_BYTES (m_data, m_bufI, &objP->info, sizeof (tBaseObject));
if (objP->info.nType != OBJ_NONE) {
#if defined(WORDS_BIGENDIAN) || defined(__BIG_ENDIAN__)
	if (gameStates.multi.nGameType >= IPX_GAME)
		SwapObject (objP);
#endif
	int32_t nSegment = objP->info.nSegment;
	PrintLog (0, "receiving object %d (type: %d, segment: %d)\n", m_nLocalObj, objP->info.nType, nSegment);
	objP->ResetSgmLinks ();
#if OBJ_LIST_TYPE
	objP->ResetLinks ();
	objP->Link ();
#endif
	objP->info.nAttachedObj = -1;
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
return 1;
}

//------------------------------------------------------------------------------
// If the client is missing 10 or more objects, reject the sync data

int32_t CObjectSynchronizer::Validate (void)
{
return abs (m_nRemoteObj - gameData.objData.nObjects) < ((gameStates.multi.nGameType == UDP_GAME) ? 2 : 10);
}

//------------------------------------------------------------------------------

int32_t CObjectSynchronizer::Finish (void)
{
if (m_nLocalObj != -2) 
	return 0;
if (networkData.nJoinState > 1)
	return -1;

if (!m_nState || !Validate ()) {
	Abort ();
	return -1;
	}

NetworkCountPowerupsInMine ();
gameData.objData.RebuildEffects ();
networkData.syncInfo [0].objs.nFrame = 0;
m_nState = 0;
if (networkData.bHaveSync)
	networkData.nStatus = NETSTAT_PLAYING;
networkData.nJoinState = 4;
return 1;
}

//------------------------------------------------------------------------------

int32_t CObjectSynchronizer::Run (uint8_t* data)

{
m_data = data;

ResetSyncTimeout (true);

int32_t syncRes = ValidateFrame ();
if (syncRes < 1)
	return syncRes;

int32_t nObjects = m_data [1];

for (int32_t i = 0; (i < nObjects) && (syncRes > -1); i++) {
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

void NetworkReadObjectPacket (uint8_t* data)
{
#if 0 //DBG
static bool bWait = true;

if (bWait) {
	while (networkThread.RxPacketQueue ().Length () < (gameData.objData.nObjects + 4) / 5)
		G3_SLEEP (0);
	bWait = false;
	}
#endif
objectSynchronizer.Run (data);
}

//------------------------------------------------------------------------------

