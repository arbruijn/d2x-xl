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

#include "inferno.h"
#include "strutil.h"
#include "ipx.h"
#include "error.h"
#include "network.h"
#include "network_lib.h"
#include "netmisc.h"
#include "gameseg.h"

//------------------------------------------------------------------------------

char *iptos (char *pszIP, char *addr)
{
sprintf (pszIP, "%d.%d.%d.%d:%d",
			addr [0], addr [1], addr [2], addr [3],
			ntohs (*reinterpret_cast<short*> (addr + 4)));
return pszIP;
}

//------------------------------------------------------------------------------

void ClipRank (char *rank)
{
 // This function insures no crashes when dealing with D2 1.0
if ((*rank < 0) || (*rank > 9))
	*rank = 0;
 }

//------------------------------------------------------------------------------

int NetworkWhoIsMaster (void)
{
	// Who is the master of this game?

	int i;

if (!IsMultiGame)
	return gameData.multiplayer.nLocalPlayer;
for (i = 0; i < gameData.multiplayer.nPlayers; i++)
	if (gameData.multiplayer.players [i].connected)
		return i;
return gameData.multiplayer.nLocalPlayer;
}

//------------------------------------------------------------------------------

int NetworkIAmMaster (void)
{
return NetworkWhoIsMaster () == gameData.multiplayer.nLocalPlayer;
}

//------------------------------------------------------------------------------

int NetworkHowManyConnected (void)
 {
  int num = 0, i;

for (i = 0; i < gameData.multiplayer.nPlayers; i++)
	if (gameData.multiplayer.players [i].connected)
		num++;
return num;
}

//------------------------------------------------------------------------------

int CmpNetPlayers (char *callsign1, char *callsign2, tNetworkInfo *network1, tNetworkInfo *network2)
{
if ((gameStates.multi.nGameType == IPX_GAME) ||
	 ((gameStates.multi.nGameType == UDP_GAME) && gameStates.multi.bCheckPorts)) {
	if (memcmp (network1, network2, sizeof (tNetworkInfo)))
		return 1;
	}
else if (gameStates.multi.nGameType == UDP_GAME) {
	if (memcmp (network1, network2, extraGameInfo [1].bCheckUDPPort ? sizeof (tNetworkInfo) : sizeof (tNetworkInfo) - 2))	//bCheck the port too
		return 1;
	}
#ifdef MACINTOSH
else {
	if (network1->appletalk.node != network2->appletalk.node)
		return 1;
	if (network1->appletalk.net == network2->appletalk.net)
		return 1;
	}
#endif
if (callsign1 && callsign2 && stricmp (callsign1, callsign2))
	return 1;
#if DBG
HUDMessage (0, "'%s' not recognized", callsign1);
#endif
return 0;
}

//------------------------------------------------------------------------------

#define LOCAL_NODE \
	((gameStates.multi.bHaveLocalAddress && (gameStates.multi.nGameType == UDP_GAME)) ? \
	 ipx_LocalAddress + 4 : networkData.thisPlayer.player.network.ipx.node)


int CmpLocalPlayer (tNetworkInfo *pNetwork, char *pszNetCallSign, char *pszLocalCallSign)
{
if (stricmp (pszNetCallSign, pszLocalCallSign))
	return 1;
#if 0
// if restoring a multiplayer game that had been played via UDP/IP,
// CPlayerData network addresses may have changed, so we have to rely on the callsigns
// This will cause problems if several players with identical callsigns participate
if (gameStates.multi.nGameType == UDP_GAME)
	return 0;
#endif
if (gameStates.multi.nGameType >= IPX_GAME) {
	return memcmp (pNetwork->ipx.node, LOCAL_NODE, extraGameInfo [1].bCheckUDPPort ? 6 : 4) ? 1 : 0;
	}
#ifdef MACINTOSH
if (pNetwork->appletalk.node != networkData.thisPlayer.player.network.appletalk.node)
	return 1;
#endif
return 0;
}

//------------------------------------------------------------------------------

char *NetworkGetPlayerName (int nObject)
{
if (nObject < 0)
	return NULL;
if (OBJECTS [nObject].info.nType != OBJ_PLAYER)
	return NULL;
if (OBJECTS [nObject].info.nId >= MAX_PLAYERS)
	return NULL;
if (OBJECTS [nObject].info.nId >= gameData.multiplayer.nPlayers)
	return NULL;
return gameData.multiplayer.players [OBJECTS [nObject].info.nId].callsign;
}

//------------------------------------------------------------------------------

void DeleteActiveNetGame (int i)
{
if (i < --networkData.nActiveGames) {
	int h = networkData.nActiveGames - i;
	memcpy (activeNetGames + i, activeNetGames + i + 1, sizeof (tNetgameInfo) * h);
	memcpy (activeNetPlayers + i, activeNetPlayers + i + 1, sizeof (tAllNetPlayersInfo) * h);
	memcpy (nLastNetGameUpdate + i, nLastNetGameUpdate + i + 1, sizeof (int) * h);
	}
networkData.bGamesChanged = 1;
}

//------------------------------------------------------------------------------

void DeleteTimedOutNetGames (void)
{
	int	i, t = SDL_GetTicks ();
	int	bPlaySound = 0;

for (i = networkData.nActiveGames; i > 0;)
	if (t - nLastNetGameUpdate [--i] > 10000) {
		DeleteActiveNetGame (i);
		bPlaySound = 1;
		}
if (bPlaySound)
	DigiPlaySample (SOUND_HUD_MESSAGE,F1_0);
}

//------------------------------------------------------------------------------

int FindActiveNetGame (char *pszGameName, int nSecurity)
{
	int	i;

for (i = 0; i < networkData.nActiveGames; i++) {
	if (!stricmp (activeNetGames [i].szGameName, pszGameName)
#if SECURITY_CHECK
		 && (activeNetGames [i].nSecurity == nSecurity)
#endif
		 )
		break;
	}
return i;
}

//------------------------------------------------------------------------------

int NetworkObjnumIsPast (int nObject, tNetworkSyncData *syncP)
{
	// determine whether or not a given CObject number has already been sent
	// to a re-joining player.
	int nPlayer = syncP->player [1].player.connected;
	int nObjMode = !((gameData.multigame.nObjOwner [nObject] == -1) || (gameData.multigame.nObjOwner [nObject] == nPlayer));

if (!syncP->nState)
	return 0; // We're not sending OBJECTS to a new CPlayerData
if (nObjMode > syncP->objs.nMode)
	return 0;
else if (nObjMode < syncP->objs.nMode)
	return 1;
else if (nObject < syncP->objs.nCurrent)
	return 1;
else
	return 0;
}

//------------------------------------------------------------------------------

void NetworkSetGameMode (int gameMode)
{
	gameData.multigame.kills.bShowList = 1;

if (gameMode == NETGAME_ANARCHY)
	;
else if (gameMode == NETGAME_TEAM_ANARCHY)
	gameData.app.nGameMode = GM_TEAM;
else if (gameMode == NETGAME_ROBOT_ANARCHY)
	gameData.app.nGameMode = GM_MULTI_ROBOTS;
else if (gameMode == NETGAME_COOPERATIVE)
	gameData.app.nGameMode = GM_MULTI_COOP | GM_MULTI_ROBOTS;
else if (gameMode == NETGAME_CAPTURE_FLAG)
		gameData.app.nGameMode = GM_TEAM | GM_CAPTURE;
else if (HoardEquipped ()) {
	if (gameMode == NETGAME_HOARD)
		gameData.app.nGameMode = GM_HOARD;
	else if (gameMode == NETGAME_TEAM_HOARD)
		gameData.app.nGameMode = GM_HOARD | GM_TEAM;
	else if (gameMode == NETGAME_ENTROPY)
		gameData.app.nGameMode = GM_ENTROPY | GM_TEAM;
	else if (gameMode == NETGAME_MONSTERBALL)
		gameData.app.nGameMode = GM_MONSTERBALL | GM_TEAM;
	}
else
	Int3 ();
gameData.app.nGameMode |= GM_NETWORK;
if (gameData.app.nGameMode & GM_TEAM)
	gameData.multigame.kills.bShowList = 3;
}

//------------------------------------------------------------------------------

int GotTeamSpawnPos (void)
{
	int	i, j;

for (i = 0; i < gameData.multiplayer.nPlayerPositions; i++) {
	j = FindSegByPos (*PlayerSpawnPos (i), -1, 1, 0);
	gameData.multiplayer.playerInit [i].nSegType = (j < 0) ? SEGMENT_IS_NOTHING : gameData.segs.segment2s [j].special;
	switch (gameData.multiplayer.playerInit [i].nSegType) {
		case SEGMENT_IS_GOAL_BLUE:
		case SEGMENT_IS_TEAM_BLUE:
		case SEGMENT_IS_GOAL_RED:
		case SEGMENT_IS_TEAM_RED:
			break;
		default:
			return 0;
		}
	}
return 1;
}

//------------------------------------------------------------------------------
// Find the proper initial spawn location for CPlayerData i from team t

int TeamSpawnPos (int i)
{
	int	h, j, t = GetTeam (i);

// first find out how many players before CPlayerData i are in the same team
// result stored in h
for (h = j = 0; j < i; j++)
	if (GetTeam (j) == t)
		h++;
// assign the spawn location # (h+1) to CPlayerData i
for (j = 0; j < gameData.multiplayer.nPlayerPositions; j++) {
	switch (gameData.multiplayer.playerInit [j].nSegType) {
		case SEGMENT_IS_GOAL_BLUE:
		case SEGMENT_IS_TEAM_BLUE:
			if (!t && (--h < 0))
				return j;
			break;
		case SEGMENT_IS_GOAL_RED:
		case SEGMENT_IS_TEAM_RED:
			if (t && (--h < 0))
				return j;
			break;
		}
	}
return -1;
}

//------------------------------------------------------------------------------

void NetworkCountPowerupsInMine (void)
{
  int 		i;
  CObject	*objP;

memset (gameData.multiplayer.powerupsInMine, 0, sizeof (gameData.multiplayer.powerupsInMine));
FORALL_POWERUP_OBJS (objP, i) {
	gameData.multiplayer.powerupsInMine [objP->info.nId]++;
	if (MultiPowerupIs4Pack (objP->info.nId))
		gameData.multiplayer.powerupsInMine [objP->info.nId - 1] += 4;
	}
}

//------------------------------------------------------------------------------

void NetworkAdjustMaxDataSize ()
{
networkData.nMaxXDataSize = netGame.bShortPackets ? NET_XDATA_SIZE : NET_XDATA_SIZE;
}

//------------------------------------------------------------------------------

#ifdef NETPROFILING
void OpenSendLog ()
 {
  int i;

SendLogFile = reinterpret_cast<FILE*> (fopen ("sendlog.net", "w"));
for (i = 0; i < 100; i++)
	TTSent [i] = 0;
 }

 //------------------------------------------------------------------------------

void OpenReceiveLog ()
 {
int i;

ReceiveLogFile= reinterpret_cast<FILE*> (fopen ("recvlog.net", "w"));
for (i = 0; i < 100; i++)
	TTRecv [i] = 0;
 }
#endif

//------------------------------------------------------------------------------

void NetworkRequestPlayerNames (int n)
{
NetworkSendAllInfoRequest (PID_GAME_PLAYERS, activeNetGames [n].nSecurity);
networkData.nNamesInfoSecurity = activeNetGames [n].nSecurity;
}

//------------------------------------------------------------------------------

int HoardEquipped ()
{
	static int checked=-1;

if (checked == -1) {
	if (CFile::Exist ("hoard.ham", gameFolders.szDataDir, 0))
		checked=1;
	else
		checked=0;
	}
return (checked);
}

//------------------------------------------------------------------------------

