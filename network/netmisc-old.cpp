/* $Id: netmisc.c,v 1.9 2003/10/04 19:13:32 btb Exp $ */
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

#include <stdio.h>
#include <string.h>

#include "inferno.h"
#include "pstypes.h"
#include "mono.h"

//#define WORDS_BIGENDIAN

#if defined(WORDS_BIGENDIAN) || defined(__BIG_ENDIAN__)

#include "byteswap.h"
#include "segment.h"
#include "gameseg.h"
#include "network.h"
#include "network_lib.h"
#include "wall.h"

// routine to calculate the checksum of the segments.  We add these specialized routines
// since the current way is byte order dependent.

void BEDoCheckSumCalc(ubyte *b, int len, unsigned int *s1, unsigned int *s2)
{

	while(len--) {
		*s1 += *b++;
		if (*s1 >= 255) *s1 -= 255;
		*s2 += *s1;
	}
}

ushort BECalcSegmentCheckSum (void)
{
	int				i, j, k, t;
	unsigned int	sum1,sum2;
	short				s;
	tSegment			*segP;
	tSide				*sideP;
	tUVL				*uvlP;
	vmsVector		*normP;

sum1 = sum2 = 0;
for (i = 0, segP = gameData.segs.segments; i < gameData.segs.nSegments; i++, segP++) {
	for (j = 0, sideP = segP->sides; j < MAX_SIDES_PER_SEGMENT; j++, sideP++) {
		BEDoCheckSumCalc ((ubyte *) &(sideP->nType), 1, &sum1, &sum2);
		BEDoCheckSumCalc ((ubyte *) &(sideP->nFrame), 1, &sum1, &sum2);
		s = INTEL_SHORT (WallNumI (i, j));
		BEDoCheckSumCalc ((ubyte *) &s, 2, &sum1, &sum2);
		s = INTEL_SHORT (sideP->nBaseTex);
		BEDoCheckSumCalc ((ubyte *) &s, 2, &sum1, &sum2);
		s = INTEL_SHORT (sideP->nOvlOrient + (((short) sideP->nOvlTex) << 2));
		BEDoCheckSumCalc ((ubyte *) &s, 2, &sum1, &sum2);
		for (k = 0, uvlP = sideP->uvls; k < 4; k++, uvlP++) {
			t = INTEL_INT (((int) uvlP->u));
			BEDoCheckSumCalc ((ubyte *) &t, 4, &sum1, &sum2);
			t = INTEL_INT (((int) uvlP->v));
			BEDoCheckSumCalc ((ubyte *) &t, 4, &sum1, &sum2);
			t = INTEL_INT (((int) uvlP->l));
			BEDoCheckSumCalc ((ubyte *) &t, 4, &sum1, &sum2);
			}
		for (k = 0, normP = sideP->normals; k < 2; k++, normP++) {
			t = INTEL_INT ((int) (*normP) [X]);
			BEDoCheckSumCalc ((ubyte *) &t, 4, &sum1, &sum2);
			t = INTEL_INT ((int) (*normP) [Y]);
			BEDoCheckSumCalc ((ubyte *) &t, 4, &sum1, &sum2);
			t = INTEL_INT ((int) (*normP) [Z]);
			BEDoCheckSumCalc ((ubyte *) &t, 4, &sum1, &sum2);
			}
		}
	for (j = 0; j < MAX_SIDES_PER_SEGMENT; j++) {
		s = INTEL_SHORT (segP->children [j]);
		BEDoCheckSumCalc ((ubyte *) &s, 2, &sum1, &sum2);
	}
	for (j = 0; j < MAX_VERTICES_PER_SEGMENT; j++) {
		s = INTEL_SHORT (segP->verts [j]);
		BEDoCheckSumCalc ((ubyte *) &s, 2, &sum1, &sum2);
	}
	t = INTEL_INT (segP->objects);
	BEDoCheckSumCalc((ubyte *) &t, 4, &sum1, &sum2);
}
sum2 %= 255;
return ((sum1<<8)+ sum2);
}

// this routine totally and completely relies on the fact that the network
//  checksum must be calculated on the segments!!!!!

ushort NetMiscCalcCheckSum(void * vptr, int len)
{
return BECalcSegmentCheckSum();
}

// following are routine for macintosh only that will swap the elements of
// structures send through the networking code.  The structures and
// this code must be kept in total sync

#include "ipx.h"
#include "multi.h"
#include "object.h"
#include "powerup.h"
#include "error.h"

ubyte out_buffer [MAX_PACKETSIZE];    // used for tmp netgame packets as well as sending tObject data

//------------------------------------------------------------------------------

void BEReceiveNetPlayerInfo(ubyte *data, tNetPlayerInfo *info)
{
	int bufI = 0;

memcpy(info->callsign, data + bufI, CALLSIGN_LEN+1);
bufI += CALLSIGN_LEN+1;
memcpy(&(info->network.ipx.server), data + bufI, 4);
bufI += 4;
memcpy(&(info->network.ipx.node), data + bufI, 6);
bufI += 6;
info->versionMajor = data [bufI++];
info->versionMinor = data [bufI++];
memcpy(&(info->computerType), data + bufI, 1);
bufI++;      // memcpy to avoid compile time warning about enum
info->connected = data [bufI++];
memcpy(&(info->socket), data + bufI, 2);
bufI += 2;
memcpy (&(info->rank), data + bufI++, 1);
// MWA don't think we need to swap this because we need it in high
// order  info->socket = INTEL_SHORT(info->socket);
}

//------------------------------------------------------------------------------

void BESendNetPlayersPacket(ubyte *server, ubyte *node)
{
	int i, tmpi;
	int bufI = 0;
	short tmps;

memset(out_buffer, 0, sizeof(out_buffer));
out_buffer [0] = netPlayers.nType;
bufI++;
tmpi = INTEL_INT (netPlayers.nSecurity);
memcpy(out_buffer + bufI, &tmpi, 4);
bufI += 4;
for (i = 0; i < MAX_PLAYERS+4; i++) {
	memcpy(out_buffer + bufI, netPlayers.players [i].callsign, CALLSIGN_LEN+1);
	bufI += CALLSIGN_LEN+1;
	memcpy(out_buffer + bufI, netPlayers.players [i].network.ipx.server, 4);
	bufI += 4;
	memcpy(out_buffer + bufI, netPlayers.players [i].network.ipx.node, 6);
	bufI += 6;
	memcpy(out_buffer + bufI, &(netPlayers.players [i].versionMajor), 1);
	bufI++;
	memcpy(out_buffer + bufI, &(netPlayers.players [i].versionMinor), 1);
	bufI++;
	memcpy(out_buffer + bufI, &(netPlayers.players [i].computerType), 1);
	bufI++;
	memcpy(out_buffer + bufI, &(netPlayers.players [i].connected), 1);
	bufI++;
	tmps = INTEL_SHORT(netPlayers.players [i].socket);
	memcpy(out_buffer + bufI, &tmps, 2);
	bufI += 2;
	memcpy(out_buffer + bufI, &(netPlayers.players [i].rank), 1);
	bufI++;
	}
if ((server == NULL) && (node == NULL))
	IPXSendBroadcastData(out_buffer, bufI);
else
	IPXSendInternetPacketData(out_buffer, bufI, server, node);
}

//------------------------------------------------------------------------------

void BEReceiveNetPlayersPacket(ubyte *data, tAllNetPlayersInfo *pinfo)
{
	int i, bufI = 0;

pinfo->nType = data [bufI];
bufI++;
memcpy(&(pinfo->nSecurity), data + bufI, 4);
bufI += 4;
pinfo->nSecurity = INTEL_INT (pinfo->nSecurity);
for (i = 0; i < MAX_PLAYERS+4; i++) {
	BEReceiveNetPlayerInfo(data + bufI, &(pinfo->players [i]));
	bufI += 26;          // sizeof(tNetPlayerInfo) on the PC
	}
}

//------------------------------------------------------------------------------

void BESendSequencePacket(tSequencePacket seq, ubyte *server, ubyte *node, ubyte *netAddress)
{
	short tmps;
	int bufI, tmpi;

bufI = 0;
memset(out_buffer, 0, sizeof(out_buffer));
out_buffer [0] = seq.nType;
bufI++;
tmpi = INTEL_INT (seq.nSecurity);
memcpy(out_buffer + bufI, &tmpi, 4);
bufI += 4;
bufI += 3;
memcpy(out_buffer + bufI, seq.player.callsign, CALLSIGN_LEN+1);
bufI += CALLSIGN_LEN+1;
memcpy(out_buffer + bufI, seq.player.network.ipx.server, 4);
bufI += 4;
memcpy(out_buffer + bufI, seq.player.network.ipx.node, 6);
bufI += 6;
out_buffer [bufI] = seq.player.versionMajor;
bufI++;
out_buffer [bufI] = seq.player.versionMinor;
bufI++;
out_buffer [bufI] = seq.player.computerType;
bufI++;
out_buffer [bufI] = seq.player.connected;
bufI++;
tmps = INTEL_SHORT(seq.player.socket);
memcpy(out_buffer + bufI, &tmps, 2);
bufI += 2;
out_buffer [bufI]=seq.player.rank;
bufI++;      // for pad byte
if (netAddress != NULL)
	IPXSendPacketData(out_buffer, bufI, server, node, netAddress);
else if (!server && !node)
	IPXSendBroadcastData(out_buffer, bufI);
else
	IPXSendInternetPacketData(out_buffer, bufI, server, node);
}

//------------------------------------------------------------------------------

void BEReceiveSequencePacket(ubyte *data, tSequencePacket *seq)
{
	int bufI = 0;

	seq->nType = data [0];
	bufI++;
	memcpy(&(seq->nSecurity), data + bufI, 4);  
	bufI += 7;   // +3 for pad byte
	seq->nSecurity = INTEL_INT (seq->nSecurity);
	BEReceiveNetPlayerInfo(data + bufI, &(seq->player));
}

//------------------------------------------------------------------------------

void BESendNetGamePacket(ubyte *server, ubyte *node, ubyte *netAddress, int liteFlag)     // lite says shorter netgame packets
{
	uint tmpi;
	ushort tmps; // p;
	int i, j;
	int bufI = 0;

memset(out_buffer, 0, MAX_PACKETSIZE);
memcpy(out_buffer + bufI, &(netGame.nType), 1);
bufI++;
tmpi = INTEL_INT (netGame.nSecurity);
memcpy(out_buffer + bufI, &tmpi, 4);
bufI += 4;
memcpy(out_buffer + bufI, netGame.szGameName, NETGAME_NAME_LEN+1);
bufI += (NETGAME_NAME_LEN+1);
memcpy(out_buffer + bufI, netGame.szMissionTitle, MISSION_NAME_LEN+1);
bufI += (MISSION_NAME_LEN+1);
memcpy(out_buffer + bufI, netGame.szMissionName, 9);
bufI += 9;
tmpi = INTEL_INT (netGame.nLevel);
memcpy(out_buffer + bufI, &tmpi, 4);
bufI += 4;
memcpy(out_buffer + bufI, &(netGame.gameMode), 1);
bufI++;
memcpy(out_buffer + bufI, &(netGame.bRefusePlayers), 1);
bufI++;
memcpy(out_buffer + bufI, &(netGame.difficulty), 1);
bufI++;
memcpy(out_buffer + bufI, &(netGame.gameStatus), 1);
bufI++;
memcpy(out_buffer + bufI, &(netGame.nNumPlayers), 1);
bufI++;
memcpy(out_buffer + bufI, &(netGame.nMaxPlayers), 1);
bufI++;
memcpy(out_buffer + bufI, &(netGame.nConnected), 1);
bufI++;
memcpy(out_buffer + bufI, &(netGame.gameFlags), 1);
bufI++;
memcpy(out_buffer + bufI, &(netGame.protocolVersion), 1);
bufI++;
memcpy(out_buffer + bufI, &(netGame.versionMajor), 1);
bufI++;
memcpy(out_buffer + bufI, &(netGame.versionMinor), 1);
bufI++;
memcpy(out_buffer + bufI, &(netGame.teamVector), 1);
bufI++;

if (liteFlag)
	goto do_send;

// will this work -- damn bitfields -- totally bogus when trying to do
// this nType of stuff
// Watcom makes bitfields from left to right.  CW7 on the mac goes
// from right to left.  then they are endian swapped

tmps = *(ushort *)(&netGame.teamVector + 1);    // get the values for the first short bitfield
tmps = INTEL_SHORT(tmps);
memcpy(out_buffer + bufI, &tmps, 2);
bufI += 2;

tmps = *(ushort *)((ubyte *)(&netGame.teamVector) + 3);    // get the values for the second short bitfield
tmps = INTEL_SHORT(tmps);
memcpy(out_buffer + bufI, &tmps, 2);
bufI += 2;

memcpy(out_buffer + bufI, netGame.szTeamName, 2*(CALLSIGN_LEN+1)); 
bufI += 2*(CALLSIGN_LEN+1);
for (i = 0; i < MAX_PLAYERS; i++) {
	tmpi = INTEL_INT (netGame.locations [i]);
	memcpy(out_buffer + bufI, &tmpi, 4);
	bufI += 4;   // SWAP HERE!!!
	}

for (i = 0; i < MAX_PLAYERS; i++) {
	for (j = 0; j < MAX_PLAYERS; j++) {
		tmps = INTEL_SHORT(netGame.kills [i][j]);
		memcpy(out_buffer + bufI, &tmps, 2);
		bufI += 2;   // SWAP HERE!!!
		}
	}

tmps = INTEL_SHORT(netGame.nSegmentCheckSum);
memcpy(out_buffer + bufI, &tmps, 2);
bufI += 2;   // SWAP_HERE
tmps = INTEL_SHORT(netGame.teamKills [0]);
memcpy(out_buffer + bufI, &tmps, 2);
bufI += 2;   // SWAP_HERE
tmps = INTEL_SHORT(netGame.teamKills [1]);
memcpy(out_buffer + bufI, &tmps, 2);
bufI += 2;   // SWAP_HERE
for (i = 0; i < MAX_PLAYERS; i++) {
	tmps = INTEL_SHORT(netGame.killed [i]);
	memcpy(out_buffer + bufI, &tmps, 2);
	bufI += 2;   // SWAP HERE!!!
	}
for (i = 0; i < MAX_PLAYERS; i++) {
	tmps = INTEL_SHORT(netGame.playerKills [i]);
	memcpy(out_buffer + bufI, &tmps, 2);
	bufI += 2;   // SWAP HERE!!!
	}

tmpi = INTEL_INT (netGame.KillGoal);
memcpy(out_buffer + bufI, &tmpi, 4);
bufI += 4;   // SWAP_HERE
tmpi = INTEL_INT (netGame.xPlayTimeAllowed);
memcpy(out_buffer + bufI, &tmpi, 4);
bufI += 4;   // SWAP_HERE
tmpi = INTEL_INT (netGame.xLevelTime);
memcpy(out_buffer + bufI, &tmpi, 4);
bufI += 4;   // SWAP_HERE
tmpi = INTEL_INT (netGame.controlInvulTime);
memcpy(out_buffer + bufI, &tmpi, 4);
bufI += 4;   // SWAP_HERE
tmpi = INTEL_INT (netGame.monitorVector);
memcpy(out_buffer + bufI, &tmpi, 4);
bufI += 4;   // SWAP_HERE
for (i = 0; i < MAX_PLAYERS; i++) {
	tmpi = INTEL_INT (netGame.playerScore [i]);
	memcpy(out_buffer + bufI, &tmpi, 4);
	bufI += 4;   // SWAP_HERE
	}
for (i = 0; i < MAX_PLAYERS; i++) {
	memcpy(out_buffer + bufI, &(netGame.playerFlags [i]), 1); bufI++;
	}
tmps = INTEL_SHORT(PacketsPerSec ());
memcpy(out_buffer + bufI, &tmps, 2);
bufI += 2;
memcpy(out_buffer + bufI, &(netGame.bShortPackets), 1);
bufI++;

do_send:

if (netAddress != NULL)
	IPXSendPacketData(out_buffer, bufI, server, node, netAddress);
else if ((server == NULL) && (node == NULL))
	IPXSendBroadcastData(out_buffer, bufI);
else
	IPXSendInternetPacketData(out_buffer, bufI, server, node);
}

//------------------------------------------------------------------------------

void BEReceiveNetGamePacket(ubyte *data, tNetgameInfo *netgame, int liteFlag)
{
	int i, j;
	int bufI = 0;
	short bitfield; // new_field;

memcpy(&(netgame->nType), data + bufI, 1);
bufI++;
memcpy(&(netgame->nSecurity), data + bufI, 4);
bufI += 4;
netgame->nSecurity = INTEL_INT (netgame->nSecurity);
memcpy(netgame->szGameName, data + bufI, NETGAME_NAME_LEN+1);
bufI += (NETGAME_NAME_LEN+1);
memcpy(netgame->szMissionTitle, data + bufI, MISSION_NAME_LEN+1);
bufI += (MISSION_NAME_LEN+1);
memcpy(netgame->szMissionName, data + bufI, 9);
bufI += 9;
memcpy(&(netgame->nLevel), data + bufI, 4);
bufI += 4;
netgame->nLevel = INTEL_INT (netgame->nLevel);
memcpy(&(netgame->gameMode), data + bufI, 1);
bufI++;
memcpy(&(netgame->bRefusePlayers), data + bufI, 1);
bufI++;
memcpy(&(netgame->difficulty), data + bufI, 1);
bufI++;
memcpy(&(netgame->gameStatus), data + bufI, 1);
bufI++;
memcpy(&(netgame->nNumPlayers), data + bufI, 1);
bufI++;
memcpy(&(netgame->nMaxPlayers), data + bufI, 1);
bufI++;
memcpy(&(netgame->nConnected), data + bufI, 1);
bufI++;
memcpy(&(netgame->gameFlags), data + bufI, 1);
bufI++;
memcpy(&(netgame->protocolVersion), data + bufI, 1);
bufI++;
memcpy(&(netgame->versionMajor), data + bufI, 1);
bufI++;
memcpy(&(netgame->versionMinor), data + bufI, 1);
bufI++;
memcpy(&(netgame->teamVector), data + bufI, 1);
bufI++;

if (liteFlag)
	return;

memcpy(&bitfield, data + bufI, 2);
bufI += 2;
bitfield = INTEL_SHORT(bitfield);
memcpy(((ubyte *)(&netgame->teamVector) + 1), &bitfield, 2);

memcpy(&bitfield, data + bufI, 2);
bufI += 2;
bitfield = INTEL_SHORT(bitfield);
memcpy(((ubyte *)(&netgame->teamVector) + 3), &bitfield, 2);

#if 0       // not used since reordering mac bitfields
memcpy(&bitfield, data + bufI, 2);
bufI += 2;
new_field = 0;
for (i = 15; i >= 0; i--) {
	if (bitfield & (1 << i))
		new_field |= (1 << (15 - i);
	}
new_field = INTEL_SHORT(new_field);
memcpy(((ubyte *)(&netgame->teamVector) + 1), &new_field, 2);

memcpy(&bitfield, data + bufI, 2);
bufI += 2;
new_field = 0;
for (i = 15; i >= 0; i--) {
	if (bitfield & (1 << i))
		new_field |= (1 << (15 - i);
	}
new_field = INTEL_SHORT(new_field);
memcpy(((ubyte *)(&netgame->teamVector) + 3), &new_field, 2);
#endif

memcpy(netgame->szTeamName, data + bufI, 2*(CALLSIGN_LEN+1));
bufI += 2*(CALLSIGN_LEN+1);
for (i = 0; i < MAX_PLAYERS; i++) {
	memcpy(&(netgame->locations [i]), data + bufI, 4);
	bufI += 4;
	netgame->locations [i] = INTEL_INT (netgame->locations [i]);
	}

for (i = 0; i < MAX_PLAYERS; i++) {
	for (j = 0; j < MAX_PLAYERS; j++) {
		memcpy(&(netgame->kills [i][j]), data + bufI, 2);
		bufI += 2;
		netgame->kills [i][j] = INTEL_SHORT(netgame->kills [i][j]);
		}
	}

memcpy(&(netgame->nSegmentCheckSum), data + bufI, 2);
bufI += 2;
netgame->nSegmentCheckSum = INTEL_SHORT(netgame->nSegmentCheckSum);
memcpy(&(netgame->teamKills [0]), data + bufI, 2);
bufI += 2;
netgame->teamKills [0] = INTEL_SHORT(netgame->teamKills [0]);
memcpy(&(netgame->teamKills [1]), data + bufI, 2);
bufI += 2;
netgame->teamKills [1] = INTEL_SHORT(netgame->teamKills [1]);
for (i = 0; i < MAX_PLAYERS; i++) {
	memcpy(&(netgame->killed [i]), data + bufI, 2);
	bufI += 2;
	netgame->killed [i] = INTEL_SHORT(netgame->killed [i]);
	}
for (i = 0; i < MAX_PLAYERS; i++) {
	memcpy(&(netgame->playerKills [i]), data + bufI, 2);
	bufI += 2;
	netgame->playerKills [i] = INTEL_SHORT(netgame->playerKills [i]);
	}
memcpy(&(netgame->KillGoal), data + bufI, 4);
bufI += 4;
netgame->KillGoal = INTEL_INT (netgame->KillGoal);
memcpy(&(netgame->xPlayTimeAllowed), data + bufI, 4);
bufI += 4;
netgame->xPlayTimeAllowed = INTEL_INT (netgame->xPlayTimeAllowed);

memcpy(&(netgame->xLevelTime), data + bufI, 4);
bufI += 4;
netgame->xLevelTime = INTEL_INT (netgame->xLevelTime);
memcpy(&(netgame->controlInvulTime), data + bufI, 4);
bufI += 4;
netgame->controlInvulTime = INTEL_INT (netgame->controlInvulTime);
memcpy(&(netgame->monitorVector), data + bufI, 4);
bufI += 4;
netgame->monitorVector = INTEL_INT (netgame->monitorVector);
for (i = 0; i < MAX_PLAYERS; i++) {
	memcpy(&(netgame->playerScore [i]), data + bufI, 4);
	bufI += 4;
	netgame->playerScore [i] = INTEL_INT (netgame->playerScore [i]);
	}
for (i = 0; i < MAX_PLAYERS; i++) {
	memcpy(&(netgame->playerFlags [i]), data + bufI, 1);
	bufI++;
	}
memcpy(&(netgame->nPacketsPerSec), data + bufI, 2);
bufI += 2;
netgame->nPacketsPerSec = INTEL_SHORT(netgame->nPacketsPerSec);
memcpy(&(netgame->bShortPackets), data + bufI, 1);
bufI ++;
}

//------------------------------------------------------------------------------

#define EGI_INTEL_SHORT_2BUF(_m) \
  *((short *) (out_buffer + ((char *) &extraGameInfo [1]. _m - (char *) &extraGameInfo [1]))) = INTEL_SHORT (extraGameInfo [1]. _m);

#define EGI_INTEL_INT_2BUF(_m) \
	*((int *) (out_buffer + ((char *) &extraGameInfo [1]. _m - (char *) &extraGameInfo [1]))) = INTEL_INT (extraGameInfo [1]. _m);

#define BUF2_EGI_INTEL_SHORT(_m) \
	extraGameInfo [1]. _m = INTEL_SHORT (*((short *) (out_buffer + ((char *) &extraGameInfo [1]. _m - (char *) &extraGameInfo [1]))));

#define BUF2_EGI_INTEL_INT(_m) \
	extraGameInfo [1]. _m = INTEL_INT (*((int *) (out_buffer + ((char *) &extraGameInfo [1]. _m - (char *) &extraGameInfo [1]))));

//------------------------------------------------------------------------------

void BESendExtraGameInfo(ubyte *server, ubyte *node, ubyte *netAddress)
{
memcpy (out_buffer, &extraGameInfo [1], sizeof (tExtraGameInfo));
EGI_INTEL_SHORT_2BUF (entropy.nMaxVirusCapacity);
EGI_INTEL_SHORT_2BUF (entropy.nEnergyFillRate);
EGI_INTEL_SHORT_2BUF (entropy.nShieldFillRate);
EGI_INTEL_SHORT_2BUF (entropy.nShieldDamageRate);
EGI_INTEL_INT_2BUF (nSpawnDelay);
if (netAddress != NULL)
	IPXSendPacketData(out_buffer, sizeof (tExtraGameInfo), server, node, netAddress);
else if ((server == NULL) && (node == NULL))
	IPXSendBroadcastData(out_buffer, sizeof (tExtraGameInfo));
else
	IPXSendInternetPacketData(out_buffer, sizeof (tExtraGameInfo), server, node);
}

//------------------------------------------------------------------------------

void BEReceiveExtraGameInfo(ubyte *data, tExtraGameInfo *extraGameInfo)
{
memcpy (&extraGameInfo [1], data, sizeof (tExtraGameInfo));
BUF2_EGI_INTEL_SHORT (entropy.nMaxVirusCapacity);
BUF2_EGI_INTEL_SHORT (entropy.nEnergyFillRate);
BUF2_EGI_INTEL_SHORT (entropy.nShieldFillRate);
BUF2_EGI_INTEL_SHORT (entropy.nShieldDamageRate);
BUF2_EGI_INTEL_INT (nSpawnDelay);
}

//------------------------------------------------------------------------------

void BESendMissingObjFrames(ubyte *server, ubyte *node, ubyte *netAddress)
{
	int	i;

memcpy (out_buffer, &networkData.sync [0].objs.missingFrames, sizeof (networkData.sync [0].objs.missingFrames));
((tMissingObjFrames *) &out_buffer [0])->nFrame = INTEL_SHORT (networkData.sync [0].objs.missingFrames.nFrame);
i = 2 * sizeof (ubyte) + sizeof (ushort);
if (netAddress != NULL)
	IPXSendPacketData(out_buffer, i, server, node, netAddress);
else if ((server == NULL) && (node == NULL))
	IPXSendBroadcastData(out_buffer, i);
else
	IPXSendInternetPacketData(out_buffer, i, server, node);
}

//------------------------------------------------------------------------------

void BEReceiveMissingObjFrames(ubyte *data, tMissingObjFrames *missingObjFrames)
{
memcpy (missingObjFrames, out_buffer, sizeof (tMissingObjFrames));
missingObjFrames->nFrame = INTEL_SHORT (missingObjFrames->nFrame);
}

//------------------------------------------------------------------------------

void BESwapObject(tObject *objP)
{
// swap the short and int entries for this tObject
objP->info.nSignature = INTEL_INT (objP->info.nSignature);
objP->info.nNextInSeg = INTEL_SHORT(objP->info.nNextInSeg);
objP->info.nPrevInSeg = INTEL_SHORT(objP->info.nPrevInSeg);
objP->info.nSegment = INTEL_SHORT(objP->info.nSegment);
INTEL_VECTOR (objP->info.position.vPos);
INTEL_MATRIX (objP->info.position.mOrient);
objP->info.xSize = INTEL_INT (objP->info.xSize);
objP->info.xShields = INTEL_INT (objP->info.xShields);
INTEL_VECTOR (objP->info.vLastPos);
objP->info.xLifeLeft = INTEL_INT (objP->info.xLifeLeft);
switch (objP->info.movementType) {
	case MT_PHYSICS:
		INTEL_VECTOR (objP->mType.physInfo.velocity);
		INTEL_VECTOR (objP->mType.physInfo.thrust);
		objP->mType.physInfo.mass = INTEL_INT (objP->mType.physInfo.mass);
		objP->mType.physInfo.drag = INTEL_INT (objP->mType.physInfo.drag);
		objP->mType.physInfo.brakes = INTEL_INT (objP->mType.physInfo.brakes);
		INTEL_VECTOR (objP->mType.physInfo.rotVel);
		INTEL_VECTOR (objP->mType.physInfo.rotThrust);
		objP->mType.physInfo.turnRoll = INTEL_INT (objP->mType.physInfo.turnRoll);
		objP->mType.physInfo.flags = INTEL_SHORT(objP->mType.physInfo.flags);
		break;

	case MT_SPINNING:
		INTEL_VECTOR (objP->mType.spinRate);
		break;
	}

switch (objP->info.controlType) {
	case CT_WEAPON:
		objP->cType.laserInfo.parent.nType = INTEL_SHORT (objP->cType.laserInfo.parent.nType);
		objP->cType.laserInfo.parent.nObject = INTEL_SHORT (objP->cType.laserInfo.parent.nObject);
		objP->cType.laserInfo.parent.nSignature = INTEL_INT (objP->cType.laserInfo.parent.nSignature);
		objP->cType.laserInfo.creationTime = INTEL_INT (objP->cType.laserInfo.creationTime);
		objP->cType.laserInfo.nLastHitObj = INTEL_SHORT(objP->cType.laserInfo.nLastHitObj);
		if (objP->cType.laserInfo.nLastHitObj < 0)
			objP->cType.laserInfo.nLastHitObj = 0;
		else {
			gameData.objs.nHitObjects [OBJ_IDX (objP) * MAX_HIT_OBJECTS] = objP->cType.laserInfo.nLastHitObj;
			objP->cType.laserInfo.nLastHitObj = 1;
			}
		objP->cType.laserInfo.nHomingTarget = INTEL_SHORT(objP->cType.laserInfo.nHomingTarget);
		objP->cType.laserInfo.multiplier = INTEL_INT (objP->cType.laserInfo.multiplier);
		break;

	case CT_EXPLOSION:
		objP->cType.explInfo.nSpawnTime = INTEL_INT (objP->cType.explInfo.nSpawnTime);
		objP->cType.explInfo.nDeleteTime = INTEL_INT (objP->cType.explInfo.nDeleteTime);
		objP->cType.explInfo.nDeleteObj = INTEL_SHORT (objP->cType.explInfo.nDeleteObj);
		objP->cType.explInfo.nAttachParent = INTEL_SHORT (objP->cType.explInfo.nAttachParent);
		objP->cType.explInfo.nPrevAttach = INTEL_SHORT (objP->cType.explInfo.nPrevAttach);
		objP->cType.explInfo.nNextAttach = INTEL_SHORT (objP->cType.explInfo.nNextAttach);
		break;

	case CT_AI:
		objP->cType.aiInfo.nHideSegment = INTEL_SHORT (objP->cType.aiInfo.nHideSegment);
		objP->cType.aiInfo.nHideIndex = INTEL_SHORT (objP->cType.aiInfo.nHideIndex);
		objP->cType.aiInfo.nPathLength = INTEL_SHORT (objP->cType.aiInfo.nPathLength);
		objP->cType.aiInfo.nDangerLaser = INTEL_SHORT (objP->cType.aiInfo.nDangerLaser);
		objP->cType.aiInfo.nDangerLaserSig = INTEL_INT (objP->cType.aiInfo.nDangerLaserSig);
		objP->cType.aiInfo.xDyingStartTime = INTEL_INT (objP->cType.aiInfo.xDyingStartTime);
		break;

	case CT_LIGHT:
		objP->cType.lightInfo.intensity = INTEL_INT (objP->cType.lightInfo.intensity);
		break;

	case CT_POWERUP:
		objP->cType.powerupInfo.count = INTEL_INT (objP->cType.powerupInfo.count);
		objP->cType.powerupInfo.creationTime = INTEL_INT (objP->cType.powerupInfo.creationTime);
		// Below commented out 5/2/96 by Matt.  I asked Allender why it was
		// here, and he didn't know, and it looks like it doesn't belong.
		// if (objP->info.nId == POW_VULCAN)
		// objP->cType.powerupInfo.count = VULCAN_WEAPON_AMMO_AMOUNT;
		break;

	}

switch (objP->info.renderType) {
	case RT_MORPH:
	case RT_POLYOBJ: {
		int i;
		objP->rType.polyObjInfo.nModel = INTEL_INT (objP->rType.polyObjInfo.nModel);
		for (i = 0; i < MAX_SUBMODELS; i++) {
			INTEL_ANGVEC (objP->rType.polyObjInfo.animAngles [i]);
		}
		objP->rType.polyObjInfo.nSubObjFlags = INTEL_INT (objP->rType.polyObjInfo.nSubObjFlags);
		objP->rType.polyObjInfo.nTexOverride = INTEL_INT (objP->rType.polyObjInfo.nTexOverride);
		objP->rType.polyObjInfo.nAltTextures = INTEL_INT (objP->rType.polyObjInfo.nAltTextures);
		break;
	}

	case RT_WEAPON_VCLIP:
	case RT_HOSTAGE:
	case RT_POWERUP:
	case RT_FIREBALL:
	case RT_THRUSTER:
		objP->rType.vClipInfo.nClipIndex = INTEL_INT (objP->rType.vClipInfo.nClipIndex);
		objP->rType.vClipInfo.xFrameTime = INTEL_INT (objP->rType.vClipInfo.xFrameTime);
		break;

	case RT_LASER:
		break;
	}
//  END OF SWAPPING OBJECT STRUCTURE
}

#else /* !WORDS_BIGENDIAN */

//------------------------------------------------------------------------------
// Calculates the checksum of a block of memory.
ushort NetMiscCalcCheckSum(void * vptr, int len)
{
	ubyte *ptr = (ubyte *)vptr;
	unsigned int sum1,sum2;

	sum1 = sum2 = 0;

	for (; len; len--) {
		sum1 += *ptr++;
		if (sum1 >= 255)
			sum1 -= 255;
		sum2 += sum1;
	}
return (sum1 * 256 + sum2 % 255);
}

#endif /* WORDS_BIGENDIAN */

//------------------------------------------------------------------------------
//eof
