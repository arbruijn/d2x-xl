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
#include "console.h"

//------------------------------------------------------------------------------

void NetworkProcessMonitorVector (int32_t vector)
{
	int32_t		i, j;
	int32_t		tm, ec, bm;
	int32_t		count = 0;
	CSegment	*segP = SEGMENTS.Buffer ();
	CSide		*sideP;

for (i = 0; i <= gameData.segs.nLastSegment; i++, segP++) {
	for (j = 0, sideP = segP->m_sides; j < SEGMENT_SIDE_COUNT; j++, sideP++) {
		if (!sideP->FaceCount ())
			continue;
		if (((tm = sideP->m_nOvlTex) != 0) &&
				((ec = gameData.pig.tex.tMapInfoP [tm].nEffectClip) != -1) &&
				((bm = gameData.effects.effectP [ec].nDestBm) != -1)) {
			if (vector & (1 << count))
				sideP->m_nOvlTex = bm;
			count++;
			//Assert (count < 32);
			}
		}
	}
}

//------------------------------------------------------------------------------

void NetworkProcessGameInfo (uint8_t *data)
{
	CNetGameInfo	newGame (reinterpret_cast<tNetGameInfo*> (data));

#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
	CNetGameInfo tmp_info;

if (gameStates.multi.nGameType >= IPX_GAME) {
	ReceiveNetGamePacket (data, &tmp_info, 0); // get correctly aligned structure
	newGame = tmp_info;
	}
#endif
networkData.bWaitingForPlayerInfo = 0;
#if SECURITY_CHECK
if (newGame.m_info.nSecurity != playerInfoP->m_info.nSecurity) {
	Int3 ();     // Get Jason
   return;     // If this first half doesn't go with the second half
   }
#endif
Assert (playerInfoP != NULL);
int32_t i = FindActiveNetGame (newGame.m_info.szGameName, newGame.m_info.nSecurity);
if (i == MAX_ACTIVE_NETGAMES) {
#if 1
	console.printf (CON_DBG, "Too many netgames.\n");
#endif	
	return;
	}
if (i == networkData.nActiveGames) {
	if (newGame.m_info.nNumPlayers == 0)
		return;
	networkData.nActiveGames++;
	}
networkData.bGamesChanged = 1;
// MWA  memcpy (&activeNetGames [i], data, sizeof (tNetGameInfo);
nLastNetGameUpdate [i] = SDL_GetTicks ();
activeNetGames [i] = newGame;
activeNetPlayers [i] = *playerInfoP;
if (networkData.nSecurityCheck)
#if SECURITY_CHECK
	if (activeNetGames [i].m_info.nSecurity == networkData.nSecurityCheck)
#endif
		networkData.nSecurityCheck = -1;
if (i == networkData.nActiveGames)
if (activeNetGames [i].m_info.nNumPlayers == 0) {	// Delete this game
	DeleteActiveNetGame (i);
	networkData.nSecurityCheck = 0;
	}
}

//------------------------------------------------------------------------------

void NetworkProcessLiteInfo (uint8_t *data)
{
	int32_t					i;
	CNetGameInfo*		actGameP;
	tNetGameInfoLite*	newInfo = reinterpret_cast<tNetGameInfoLite*> (data);
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
	CNetGameInfo		tmp_info;

if (gameStates.multi.nGameType >= IPX_GAME) {
	ReceiveNetGamePacket (data, &tmp_info, 1);
	newInfo = reinterpret_cast<tNetGameInfoLite*> (&tmp_info.m_info);
	}
#endif

networkData.bGamesChanged = 1;
i = FindActiveNetGame (reinterpret_cast<tNetGameInfo*> (newInfo)->szGameName, reinterpret_cast<tNetGameInfo*> (newInfo)->nSecurity);
if (i == MAX_ACTIVE_NETGAMES)
	return;
if (i == networkData.nActiveGames) {
	if (newInfo->nNumPlayers == 0)
		return;
	networkData.nActiveGames++;
	}
actGameP = activeNetGames + i;
memcpy (actGameP, reinterpret_cast<uint8_t*> (newInfo), sizeof (tNetGameInfoLite));
memcpy (actGameP->m_server, networkData.packetSource.Network (), sizeof (actGameP->m_server));
nLastNetGameUpdate [i] = SDL_GetTicks ();
// See if this is really a Hoard/Entropy/Monsterball game
// If so, adjust all the data accordingly
if (HoardEquipped ()) {
	if (actGameP->m_info.gameFlags & (NETGAME_FLAG_HOARD | NETGAME_FLAG_ENTROPY | NETGAME_FLAG_MONSTERBALL)) {
		if ((actGameP->m_info.gameFlags & NETGAME_FLAG_MONSTERBALL) == NETGAME_FLAG_MONSTERBALL)
			actGameP->m_info.gameMode = NETGAME_MONSTERBALL; 
		else if (actGameP->m_info.gameFlags & NETGAME_FLAG_HOARD)
			actGameP->m_info.gameMode = NETGAME_HOARD;					  
		else if (actGameP->m_info.gameFlags & NETGAME_FLAG_ENTROPY)
			actGameP->m_info.gameMode = NETGAME_ENTROPY;					  
		actGameP->m_info.gameStatus = NETSTAT_PLAYING;
		if (actGameP->m_info.gameFlags & NETGAME_FLAG_TEAM_HOARD)
			actGameP->m_info.gameMode = NETGAME_TEAM_HOARD;					  
		if (actGameP->m_info.gameFlags & NETGAME_FLAG_REALLY_ENDLEVEL)
			actGameP->m_info.gameStatus = NETSTAT_ENDLEVEL;
		if (actGameP->m_info.gameFlags & NETGAME_FLAG_REALLY_FORMING)
			actGameP->m_info.gameStatus = NETSTAT_STARTING;
		}
	}
if (actGameP->m_info.nNumPlayers == 0)
	DeleteActiveNetGame (i);
}

//------------------------------------------------------------------------------

int32_t NetworkProcessExtraGameInfo (uint8_t *data)
{
ReceiveExtraGameInfoPacket (data, extraGameInfo + 1);
memcpy (extraGameInfo, extraGameInfo + 1, sizeof (extraGameInfo [0]));
if (extraGameInfo [1].nVersion != EGI_DATA_VERSION) {
	InfoBox (TXT_SORRY, NULL, BG_STANDARD, 1, TXT_OK, TXT_D2X_VERSION_MISMATCH);
	networkData.nStatus = NETSTAT_MENU;
	return 0;
	}
if (!extraGameInfo [1].bAllowCustomWeapons)
	SetDefaultWeaponProps ();
SetMonsterballForces ();
LogExtraGameInfo ();
gameStates.app.bHaveExtraGameInfo [1] = 1;
int32_t i = FindActiveNetGame (extraGameInfo [1].szGameName, extraGameInfo [1].nSecurity);
if (i < networkData.nActiveGames)
	activeExtraGameInfo [i] = extraGameInfo [1];
else
	memset (activeExtraGameInfo + i, 0, sizeof (activeExtraGameInfo [i]));
for (i = 0; i < 3; i++)
	missionConfig.m_shipsAllowed [i] = extraGameInfo [i].shipsAllowed [i];
return 1;
}

//------------------------------------------------------------------------------

void NetworkProcessDump (tSequencePacket *their)
{
	// Our request for join was denied.  Tell the user why.

if (their->player.connected != CONNECT_ADVANCE_LEVEL) {
	TextBox (NULL, BG_STANDARD, 1, TXT_OK, NET_DUMP_STRINGS (their->player.connected));
	networkData.nStatus = NETSTAT_MENU;
	}
else {
	for (int32_t i = 0; i < gameData.multiplayer.nPlayers; i++) {
		if (!stricmp (their->player.callsign, gameData.multiplayer.players [i].callsign)) {
			if (i != WhoIsGameHost ()) 
				HUDInitMessage (TXT_KICK_ATTEMPT, their->player.callsign);
			else {
				char temp [40];
				sprintf (temp, TXT_KICKED_YOU, their->player.callsign);
				TextBox (NULL, BG_STANDARD, 1, TXT_OK, &temp);
				if (networkData.nStatus == NETSTAT_PLAYING) {
					gameStates.multi.bIWasKicked = 1;
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
for (int32_t nPlayer = 0; nPlayer < gameData.multiplayer.nPlayers; nPlayer++) {
	if (!CmpNetPlayers (their->player.callsign, netPlayers [0].m_info.players [nPlayer].callsign, 
							  &their->player.network, &netPlayers [0].m_info.players [nPlayer].network)) {
		CONNECT (nPlayer, CONNECT_PLAYING);
		gameData.multiplayer.players [nPlayer].m_nLevel = missionManager.nCurrentLevel;
		break;
		}
	}                       
}

//------------------------------------------------------------------------------

void NetworkProcessPData (uint8_t* data)
{
if (netGame.GetShortPackets ())
	NetworkReadPDataShortPacket (reinterpret_cast<tFrameInfoShort*> (data));
else
	NetworkReadPDataLongPacket (reinterpret_cast<tFrameInfoLong*> (data));
}

//------------------------------------------------------------------------------

void NetworkProcessNakedPData (uint8_t *data, int32_t len)
 {
   int32_t nPlayer = data [1]; 
   Assert (data [0] == PID_NAKED_PDATA);

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
	int32_t oldEndlevelSequence = gameStates.app.bEndLevelSequence;
	gameStates.app.bEndLevelSequence = 1;
	MultiProcessBigData (data + 2, len - 2);
	gameStates.app.bEndLevelSequence = oldEndlevelSequence;
	return;
	}
MultiProcessBigData (data + 2, len - 2);
 }

//------------------------------------------------------------------------------

void NetworkProcessNamesReturn (uint8_t* data)
 {
	CMenu	m (15);
   char	mText [15][50], temp [50];
	int32_t	i, l, nInMenu, gnum, num = 0, count = 5, nPlayers;
   
memset (mText, 0, sizeof (mText));
if (networkData.nNamesInfoSecurity != *reinterpret_cast<int32_t*> (data + 1)) {
#if 1			
  console.printf (CON_DBG, "Bad security on names return!\n");
  console.printf (CON_DBG, "NIS=%d data=%d\n", networkData.nNamesInfoSecurity, *reinterpret_cast<int32_t*> (data + 1));
#endif
	return;
	}
nPlayers = data [count++]; 
if (nPlayers == 255) {
	gameStates.multi.bSurfingNet = 0;
	networkData.nNamesInfoSecurity = -1;
	TextBox (NULL, BG_STANDARD, 1, "OK", "That game is refusing\nname requests.\n");
	gameStates.multi.bSurfingNet=1;
	return;
	}
Assert ((nPlayers > 0) && (nPlayers < MAX_NUM_NET_PLAYERS));

#if SECURITY_CHECK
for (gnum = -1, i = 0; i < networkData.nActiveGames; i++) {
	if (networkData.nNamesInfoSecurity == activeNetGames [i].m_info.nSecurity) {
		gnum = i;
		break;
		}
	}
if (gnum == -1) {
	gameStates.multi.bSurfingNet = 0;
	networkData.nNamesInfoSecurity = -1;
	TextBox (NULL, BG_STANDARD, 1, TXT_OK, TXT_GAME_GONE);
	gameStates.multi.bSurfingNet = 1;
	return;
	}
#else
gnum = 0;
#endif
sprintf (mText [num], TXT_GAME_PLRS, activeNetGames [gnum].m_info.szGameName); 
num++;
for (i = 0; i < nPlayers; i++) {
	l = data [count++];
	memcpy (temp, data + count, CALLSIGN_LEN + 1);
	count += CALLSIGN_LEN + 1;
	if (gameOpts->multi.bNoRankings)
		sprintf (mText [num], "%s", temp);
	else
		sprintf (mText [num], "%s%s", pszRankStrings [l], temp);
	num++;
	}
if (data [count] == 99) {
	sprintf (mText [num++], " ");
	sprintf (mText [num++], TXT_SHORT_PACKETS2, data [count+1] ? TXT_ON : TXT_OFF);
	sprintf (mText [num++], TXT_PPS2, data [count+2]);
	}
for (i = 0; i < num; i++) 
	m.AddText ("", mText [i]);	
bAlreadyShowingInfo = 1;
nInMenu = gameStates.menus.nInMenu;
gameStates.menus.nInMenu = 0;
m.TinyMenu (NULL, NULL);
gameStates.menus.nInMenu = nInMenu;
bAlreadyShowingInfo = 0;
}

//------------------------------------------------------------------------------

void NetworkProcessMissingObjFrames (uint8_t* data)
{
	tMissingObjFrames	missingObjFrames;

ReceiveMissingObjFramesPacket (data, &missingObjFrames);
tNetworkSyncData *syncP = FindJoiningPlayer (missingObjFrames.nPlayer);
if (syncP && (missingObjFrames.nFrame > syncP->objs.missingFrames.nFrame)) {
#if 1
	networkThread.AbortSync ();
	syncP->objs.missingFrames = missingObjFrames; // restart at the first missing frame
	syncP->objs.nFrame = 0;
	syncP->objs.nCurrent = 0;				
	syncP->nState = 1;
	}
#else
else {
	if (syncP && (missingObjFrames.nFrame < syncP->objs.nCurrent)) {
		syncP->objs.nFrame = 0;
		syncP->objs.nCurrent = missingObjFrames.nFrame; // restarted at the first missing object
		syncP->nState = 1;
		}
	}
#endif
}

//------------------------------------------------------------------------------

