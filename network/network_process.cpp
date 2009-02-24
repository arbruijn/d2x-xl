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
#include "strutil.h"
#include "ipx.h"
#include "error.h"
#include "network.h"
#include "network_lib.h"
#include "netmisc.h"
#include "autodl.h"
#include "tracker.h"
#include "text.h"
#include "monsterball.h"

//------------------------------------------------------------------------------

void NetworkProcessMonitorVector (int vector)
{
	int		i, j;
	int		tm, ec, bm;
	int		count = 0;
	CSegment	*segP = SEGMENTS.Buffer ();
	CSide		*sideP;

for (i = 0; i <= gameData.segs.nLastSegment; i++, segP++) {
	for (j = 0, sideP = segP->m_sides; j < 6; j++, sideP++) {
		if (((tm = sideP->m_nOvlTex) != 0) &&
				((ec = gameData.pig.tex.tMapInfoP [tm].nEffectClip) != -1) &&
				((bm = gameData.eff.effectP [ec].nDestBm) != -1)) {
			if (vector & (1 << count))
				sideP->m_nOvlTex = bm;
			count++;
			Assert (count < 32);
			}
		}
	}
}

//------------------------------------------------------------------------------

void NetworkProcessGameInfo (ubyte *dataP)
{
	int i;
	tNetgameInfo *newGame = reinterpret_cast<tNetgameInfo*> (dataP);

#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
	tNetgameInfo tmp_info;

if (gameStates.multi.nGameType >= IPX_GAME) {
	ReceiveNetGamePacket (dataP, &tmp_info, 0); // get correctly aligned structure
	newGame = &tmp_info;
	}
#endif
networkData.bWaitingForPlayerInfo = 0;
#if SECURITY_CHECK
if (newGame->nSecurity != playerInfoP->nSecurity) {
	Int3 ();     // Get Jason
   return;     // If this first half doesn't go with the second half
   }
#endif
Assert (playerInfoP != NULL);
i = FindActiveNetGame (newGame->szGameName, newGame->nSecurity);
if (i == MAX_ACTIVE_NETGAMES) {
#if 1
	console.printf (CON_DBG, "Too many netgames.\n");
#endif	
	return;
	}
if (i == networkData.nActiveGames) {
	if (newGame->nNumPlayers == 0)
		return;
	networkData.nActiveGames++;
	}
networkData.bGamesChanged = 1;
// MWA  memcpy (&activeNetGames [i], dataP, sizeof (tNetgameInfo);
nLastNetGameUpdate [i] = SDL_GetTicks ();
memcpy (activeNetGames + i, reinterpret_cast<ubyte*> (newGame), sizeof (tNetgameInfo));
memcpy (activeNetPlayers + i, playerInfoP, sizeof (tAllNetPlayersInfo));
if (networkData.nSecurityCheck)
#if SECURITY_CHECK
	if (activeNetGames [i].nSecurity == networkData.nSecurityCheck)
#endif
		networkData.nSecurityCheck = -1;
if (i == networkData.nActiveGames)
if (activeNetGames [i].nNumPlayers == 0) {	// Delete this game
	DeleteActiveNetGame (i);
	networkData.nSecurityCheck = 0;
	}
}

//------------------------------------------------------------------------------

void NetworkProcessLiteInfo (ubyte *dataP)
{
	int				i;
	tNetgameInfo	*actGameP;
	tLiteInfo		*newInfo = reinterpret_cast<tLiteInfo*> (dataP);
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
	tLiteInfo		tmp_info;

if (gameStates.multi.nGameType >= IPX_GAME) {
	ReceiveNetGamePacket (dataP, reinterpret_cast<tNetgameInfo*> (&tmp_info), 1);
	newInfo = &tmp_info;
	}
#endif

networkData.bGamesChanged = 1;
i = FindActiveNetGame (reinterpret_cast<tNetgameInfo*> (newInfo)->szGameName, reinterpret_cast<tNetgameInfo*> (newInfo)->nSecurity);
if (i == MAX_ACTIVE_NETGAMES)
	return;
if (i == networkData.nActiveGames) {
	if (newInfo->nNumPlayers == 0)
		return;
	networkData.nActiveGames++;
	}
actGameP = activeNetGames + i;
memcpy (actGameP, reinterpret_cast<ubyte*> (newInfo), sizeof (tLiteInfo));
nLastNetGameUpdate [i] = SDL_GetTicks ();
// See if this is really a Hoard/Entropy/Monsterball game
// If so, adjust all the dataP accordingly
if (HoardEquipped ()) {
	if (actGameP->gameFlags & (NETGAME_FLAG_HOARD | NETGAME_FLAG_ENTROPY | NETGAME_FLAG_MONSTERBALL)) {
		if ((actGameP->gameFlags & NETGAME_FLAG_MONSTERBALL) == NETGAME_FLAG_MONSTERBALL)
			actGameP->gameMode = NETGAME_MONSTERBALL; 
		else if (actGameP->gameFlags & NETGAME_FLAG_HOARD)
			actGameP->gameMode = NETGAME_HOARD;					  
		else if (actGameP->gameFlags & NETGAME_FLAG_ENTROPY)
			actGameP->gameMode = NETGAME_ENTROPY;					  
		actGameP->gameStatus = NETSTAT_PLAYING;
		if (actGameP->gameFlags & NETGAME_FLAG_TEAM_HOARD)
			actGameP->gameMode = NETGAME_TEAM_HOARD;					  
		if (actGameP->gameFlags & NETGAME_FLAG_REALLY_ENDLEVEL)
			actGameP->gameStatus = NETSTAT_ENDLEVEL;
		if (actGameP->gameFlags & NETGAME_FLAG_REALLY_FORMING)
			actGameP->gameStatus = NETSTAT_STARTING;
		}
	}
if (actGameP->nNumPlayers == 0)
	DeleteActiveNetGame (i);
}

//------------------------------------------------------------------------------

void NetworkProcessExtraGameInfo (ubyte *dataP)
{
	int	i;

ReceiveExtraGameInfoPacket (dataP, extraGameInfo + 1);
SetMonsterballForces ();
LogExtraGameInfo ();
gameStates.app.bHaveExtraGameInfo [1] = 1;
i = FindActiveNetGame (extraGameInfo [1].szGameName, extraGameInfo [1].nSecurity);
if (i < networkData.nActiveGames)
	activeExtraGameInfo [i] = extraGameInfo [1];
else
	memset (activeExtraGameInfo + i, 0, sizeof (activeExtraGameInfo [i]));
}

//------------------------------------------------------------------------------

void NetworkProcessDump (tSequencePacket *their)
{
	// Our request for join was denied.  Tell the user why.

	char temp [40];
	int i;

if (their->player.connected != 7)
	MsgBox (NULL, NULL, 1, TXT_OK, NET_DUMP_STRINGS (their->player.connected));
else {
	for (i = 0; i < gameData.multiplayer.nPlayers; i++) {
		if (!stricmp (their->player.callsign, gameData.multiplayer.players [i].callsign)) {
			if (i!=NetworkWhoIsMaster ()) 
				HUDInitMessage (TXT_KICK_ATTEMPT, their->player.callsign);
			else {
				sprintf (temp, TXT_KICKED_YOU, their->player.callsign);
				MsgBox (NULL, NULL, 1, TXT_OK, &temp);
				if (networkData.nStatus == NETSTAT_PLAYING) {
					gameStates.multi.bIWasKicked=1;
					MultiLeaveGame ();     
					}
				else
					networkData.nStatus = NETSTAT_MENU;
		      }
		   }
 		}
	}
}

//------------------------------------------------------------------------------

void NetworkProcessRequest (tSequencePacket *their)
{
	// Player is ready to receieve a sync packet
	int i;

for (i = 0; i < gameData.multiplayer.nPlayers; i++) {
	if (!CmpNetPlayers (their->player.callsign, netPlayers.players [i].callsign, 
								&their->player.network, &netPlayers.players [i].network)) {
		gameData.multiplayer.players [i].connected = 1;
		break;
		}
	}                       
}

//------------------------------------------------------------------------------

void NetworkProcessPData (char *dataP)
{
Assert (gameData.app.nGameMode & GM_NETWORK);
if (netGame.bShortPackets)
	NetworkReadPDataShortPacket (reinterpret_cast<tFrameInfoShort*> (dataP));
else
	NetworkReadPDataPacket (reinterpret_cast<tFrameInfo*> (dataP));
}

//------------------------------------------------------------------------------

void NetworkProcessNakedPData (char *dataP, int len)
 {
   int nPlayer = dataP [1]; 
   Assert (dataP [0] == PID_NAKED_PDATA);

if (nPlayer < 0) {
#if 1			
   console.printf (CON_DBG, "Naked packet is bad!\n");
#endif
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
if (gameStates.app.bEndLevelSequence || (networkData.nStatus == NETSTAT_ENDLEVEL)) {
	int old_Endlevel_sequence = gameStates.app.bEndLevelSequence;
	gameStates.app.bEndLevelSequence = 1;
	MultiProcessBigData (reinterpret_cast<char*> (dataP + 2), len - 2);
	gameStates.app.bEndLevelSequence = old_Endlevel_sequence;
	return;
	}
MultiProcessBigData (reinterpret_cast<char*> (dataP + 2), len - 2);
 }

//------------------------------------------------------------------------------

void NetworkProcessNamesReturn (char *dataP)
 {
	CMenu	m (15);
   char	mText [15][50], temp [50];
	int	i, l, nInMenu, gnum, num = 0, count = 5, nPlayers;
   
if (networkData.nNamesInfoSecurity != *reinterpret_cast<int*> (dataP + 1)) {
#if 1			
  console.printf (CON_DBG, "Bad security on names return!\n");
  console.printf (CON_DBG, "NIS=%d dataP=%d\n", networkData.nNamesInfoSecurity, *reinterpret_cast<int*> (dataP + 1));
#endif
	return;
	}
nPlayers = dataP [count++]; 
if (nPlayers == 255) {
	gameStates.multi.bSurfingNet = 0;
	networkData.nNamesInfoSecurity = -1;
	MsgBox (NULL, NULL, 1, "OK", "That game is refusing\nname requests.\n");
	gameStates.multi.bSurfingNet=1;
	return;
	}
Assert ((nPlayers > 0) && (nPlayers < MAX_NUM_NET_PLAYERS));
for (i = 0; i < 12; i++) 
	m.AddText (mText [i]);	

#if SECURITY_CHECK
for (gnum = -1, i = 0; i < networkData.nActiveGames; i++) {
	if (networkData.nNamesInfoSecurity == activeNetGames [i].nSecurity) {
		gnum = i;
		break;
		}
	}
if (gnum == -1) {
	gameStates.multi.bSurfingNet = 0;
	networkData.nNamesInfoSecurity = -1;
	MsgBox (NULL, NULL, 1, TXT_OK, TXT_GAME_GONE);
	gameStates.multi.bSurfingNet = 1;
	return;
	}
#else
gnum = 0;
#endif
sprintf (mText [num], TXT_GAME_PLRS, activeNetGames [gnum].szGameName); 
num++;
for (i = 0; i < nPlayers; i++) {
	l = dataP [count++];
	memcpy (temp, dataP + count, CALLSIGN_LEN + 1);
	count += CALLSIGN_LEN + 1;
	if (gameOpts->multi.bNoRankings)
		sprintf (mText [num], "%s", temp);
	else
		sprintf (mText [num], "%s%s", pszRankStrings [l], temp);
	num++;
	}
if (dataP [count] == 99) {
	sprintf (mText [num++], " ");
	sprintf (mText [num++], TXT_SHORT_PACKETS2, dataP [count+1] ? TXT_ON : TXT_OFF);
	sprintf (mText [num++], TXT_PPS2, dataP [count+2]);
	}
bAlreadyShowingInfo = 1;
nInMenu = gameStates.menus.nInMenu;
gameStates.menus.nInMenu = 0;
m.TinyMenu (NULL, NULL);
gameStates.menus.nInMenu = nInMenu;
bAlreadyShowingInfo = 0;
}

//------------------------------------------------------------------------------

void NetworkProcessMissingObjFrames (char *dataP)
{
	tMissingObjFrames	missingObjFrames;

ReceiveMissingObjFramesPacket (reinterpret_cast<ubyte*> (dataP), &missingObjFrames);
tNetworkSyncData *syncP = FindJoiningPlayer (missingObjFrames.nPlayer);
if (syncP) {
	syncP->objs.missingFrames = missingObjFrames;
	syncP->objs.nCurrent = -1;				
	syncP->nState = 3;
	}
}

//------------------------------------------------------------------------------

