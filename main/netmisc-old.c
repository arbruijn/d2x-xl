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

#ifdef RCS
static char rcsid[] = "$Id: netmisc.c,v 1.9 2003/10/04 19:13:32 btb Exp $";
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

ushort BECalcSegmentCheckSum()
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
		BEDoCheckSumCalc (&(sideP->nType), 1, &sum1, &sum2);
		BEDoCheckSumCalc (&(sideP->nFrame), 1, &sum1, &sum2);
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
			t = INTEL_INT (((int) normP->p.x));
			BEDoCheckSumCalc ((ubyte *) &t, 4, &sum1, &sum2);
			t = INTEL_INT (((int) normP->p.y));
			BEDoCheckSumCalc ((ubyte *) &t, 4, &sum1, &sum2);
			t = INTEL_INT (((int) normP->p.z));
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
	vptr = vptr;
	len = len;
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

sbyte out_buffer[MAX_PACKETSIZE];    // used for tmp netgame packets as well as sending tObject data

extern struct ipx_recv_data ipx_udpSrc;

void BEReceiveNetPlayerInfo(ubyte *data, tNetPlayerInfo *info)
{
	int loc = 0;

	memcpy(info->callsign, data + loc, CALLSIGN_LEN+1);       
	loc += CALLSIGN_LEN+1;
  memcpy(&(info->network.ipx.server), data + loc, 4);       
  loc += 4;
  memcpy(&(info->network.ipx.node), data + loc, 6);         
  loc += 6;
	info->version_major = data[loc];                            
	loc++;
	info->version_minor = data[loc];                            
	loc++;
	memcpy(&(info->computerType), data + loc, 1);            
	loc++;      // memcpy to avoid compile time warning about enum
	info->connected = data[loc];                                
	loc++;
	memcpy(&(info->socket), data + loc, 2);                   
	loc += 2;
	memcpy (&(info->rank),data + loc,1);                      
	loc++;
	// MWA don't think we need to swap this because we need it in high
	// order  info->socket = INTEL_SHORT(info->socket);
}

void BESendNetPlayersPacket(ubyte *server, ubyte *node)
{
	int i, tmpi;
	int loc = 0;
	short tmps;

	memset(out_buffer, 0, sizeof(out_buffer));
	out_buffer[0] = netPlayers.nType;                            loc++;
	tmpi = INTEL_INT (netPlayers.nSecurity);
	memcpy(out_buffer + loc, &tmpi, 4);                       loc += 4;
	for (i = 0; i < MAX_PLAYERS+4; i++) {
		memcpy(out_buffer + loc, netPlayers.players[i].callsign, CALLSIGN_LEN+1); 
		loc += CALLSIGN_LEN+1;
		memcpy(out_buffer + loc, netPlayers.players[i].network.ipx.server, 4);    
		loc += 4;
		memcpy(out_buffer + loc, netPlayers.players[i].network.ipx.node, 6);      
		loc += 6;
		memcpy(out_buffer + loc, &(netPlayers.players[i].version_major), 1);      
		loc++;
		memcpy(out_buffer + loc, &(netPlayers.players[i].version_minor), 1);      
		loc++;
		memcpy(out_buffer + loc, &(netPlayers.players[i].computerType), 1);      
		loc++;
		memcpy(out_buffer + loc, &(netPlayers.players[i].connected), 1);          
		loc++;
		tmps = INTEL_SHORT(netPlayers.players[i].socket);
		memcpy(out_buffer + loc, &tmps, 2);                                       
		loc += 2;
		memcpy(out_buffer + loc, &(netPlayers.players[i].rank), 1);               
		loc++;
	}

	if ((server == NULL) && (node == NULL))
		IPXSendBroadcastData(out_buffer, loc);
	else
		IPXSendInternetPacketData(out_buffer, loc, server, node);

}

void BEReceiveNetPlayersPacket(ubyte *data, tAllNetPlayersInfo *pinfo)
{
	int i, loc = 0;

	pinfo->nType = data[loc];                            
	loc++;
	memcpy(&(pinfo->nSecurity), data + loc, 4);        
	loc += 4;
	pinfo->nSecurity = INTEL_INT (pinfo->nSecurity);
	for (i = 0; i < MAX_PLAYERS+4; i++) {
		BEReceiveNetPlayerInfo(data + loc, &(pinfo->players[i]));
		loc += 26;          // sizeof(tNetPlayerInfo) on the PC
	}
}

void BESendSequencePacket(tSequencePacket seq, ubyte *server, ubyte *node, ubyte *netAddress)
{
	short tmps;
	int loc, tmpi;

	loc = 0;
	memset(out_buffer, 0, sizeof(out_buffer));
	out_buffer[0] = seq.nType;                                       
	loc++;
	tmpi = INTEL_INT (seq.nSecurity);
	memcpy(out_buffer + loc, &tmpi, 4);                           
	loc += 4;       
	loc += 3;
	memcpy(out_buffer + loc, seq.player.callsign, CALLSIGN_LEN+1);
	loc += CALLSIGN_LEN+1;
	memcpy(out_buffer + loc, seq.player.network.ipx.server, 4);   
	loc += 4;
	memcpy(out_buffer + loc, seq.player.network.ipx.node, 6);     
	loc += 6;
	out_buffer[loc] = seq.player.version_major;                     
	loc++;
	out_buffer[loc] = seq.player.version_minor;                     
	loc++;
	out_buffer[loc] = seq.player.computerType;                     
	loc++;
	out_buffer[loc] = seq.player.connected;                         
	loc++;
	tmps = INTEL_SHORT(seq.player.socket);
	memcpy(out_buffer + loc, &tmps, 2);                           
	loc += 2;
	out_buffer[loc]=seq.player.rank;                                
	loc++;      // for pad byte
	if (netAddress != NULL)
		IPXSendPacketData(out_buffer, loc, server, node, netAddress);
	else if (!server && !node)
		IPXSendBroadcastData(out_buffer, loc);
	else
		IPXSendInternetPacketData(out_buffer, loc, server, node);
}

void BEReceiveSequencePacket(ubyte *data, tSequencePacket *seq)
{
	int loc = 0;

	seq->nType = data[0];                        
	loc++;
	memcpy(&(seq->nSecurity), data + loc, 4);  loc += 4;   loc += 3;   // +3 for pad byte
	seq->nSecurity = INTEL_INT (seq->nSecurity);
	BEReceiveNetPlayerInfo(data + loc, &(seq->player));
}

void BESendNetGamePacket(ubyte *server, ubyte *node, ubyte *netAddress, int liteFlag)     // lite says shorter netgame packets
{
	uint tmpi;
	ushort tmps; // p;
	int i, j;
	int loc = 0;

	memset(out_buffer, 0, MAX_PACKETSIZE);
	memcpy(out_buffer + loc, &(netGame.nType), 1);                 
	loc++;
	tmpi = INTEL_INT (netGame.nSecurity);
	memcpy(out_buffer + loc, &tmpi, 4);                           
	loc += 4;
	memcpy(out_buffer + loc, netGame.szGameName, NETGAME_NAME_LEN+1);  
	loc += (NETGAME_NAME_LEN+1);
	memcpy(out_buffer + loc, netGame.szMissionTitle, MISSION_NAME_LEN+1);  
	loc += (MISSION_NAME_LEN+1);
	memcpy(out_buffer + loc, netGame.szMissionName, 9);            
	loc += 9;
	tmpi = INTEL_INT (netGame.nLevel);
	memcpy(out_buffer + loc, &tmpi, 4);                           
	loc += 4;
	memcpy(out_buffer + loc, &(netGame.gameMode), 1);             
	loc++;
	memcpy(out_buffer + loc, &(netGame.bRefusePlayers), 1);        
	loc++;
	memcpy(out_buffer + loc, &(netGame.difficulty), 1);           
	loc++;
	memcpy(out_buffer + loc, &(netGame.gameStatus), 1);          
	loc++;
	memcpy(out_buffer + loc, &(netGame.nNumPlayers), 1);           
	loc++;
	memcpy(out_buffer + loc, &(netGame.nMaxPlayers), 1);       
	loc++;
	memcpy(out_buffer + loc, &(netGame.nConnected), 1);         
	loc++;
	memcpy(out_buffer + loc, &(netGame.gameFlags), 1);           
	loc++;
	memcpy(out_buffer + loc, &(netGame.protocol_version), 1);     
	loc++;
	memcpy(out_buffer + loc, &(netGame.version_major), 1);        
	loc++;
	memcpy(out_buffer + loc, &(netGame.version_minor), 1);        
	loc++;
	memcpy(out_buffer + loc, &(netGame.teamVector), 1);          
	loc++;

	if (liteFlag)
		goto do_send;

// will this work -- damn bitfields -- totally bogus when trying to do
// this nType of stuff
// Watcom makes bitfields from left to right.  CW7 on the mac goes
// from right to left.  then they are endian swapped

	tmps = *(ushort *)((ubyte *)(&netGame.teamVector) + 1);    // get the values for the first short bitfield
	tmps = INTEL_SHORT(tmps);
	memcpy(out_buffer + loc, &tmps, 2);                           
	loc += 2;

	tmps = *(ushort *)((ubyte *)(&netGame.teamVector) + 3);    // get the values for the second short bitfield
	tmps = INTEL_SHORT(tmps);
	memcpy(out_buffer + loc, &tmps, 2);                           
	loc += 2;

#if 0       // removed since I reordered bitfields on mac
	p = *(ushort *)((ubyte *)(&netGame.teamVector) + 1);       // get the values for the first short bitfield
	tmps = 0;
	for (i = 15; i >= 0; i--) {
		if (p & (1 << i))
			tmps |= (1 << (15 - i);
	}
	tmps = INTEL_SHORT(tmps);
	memcpy(out_buffer + loc, &tmps, 2);                           
	loc += 2;
	p = *(ushort *)((ubyte *)(&netGame.teamVector) + 3);       // get the values for the second short bitfield
	tmps = 0;
	for (i = 15; i >= 0; i--) {
		if (p & (1 << i))
			tmps |= (1 << (15 - i);
	}
	tmps = INTEL_SHORT(tmps);
	memcpy(out_buffer + loc, &tmps, 2);                           
	loc += 2;
#endif

	memcpy(out_buffer + loc, netGame.team_name, 2*(CALLSIGN_LEN+1)); loc += 2*(CALLSIGN_LEN+1);
	for (i = 0; i < MAX_PLAYERS; i++) {
		tmpi = INTEL_INT (netGame.locations[i]);
		memcpy(out_buffer + loc, &tmpi, 4);       
		loc += 4;   // SWAP HERE!!!
	}

	for (i = 0; i < MAX_PLAYERS; i++) {
		for (j = 0; j < MAX_PLAYERS; j++) {
			tmps = INTEL_SHORT(netGame.kills[i][j]);
			memcpy(out_buffer + loc, &tmps, 2);   
			loc += 2;   // SWAP HERE!!!
		}
	}

	tmps = INTEL_SHORT(netGame.segments_checksum);
	memcpy(out_buffer + loc, &tmps, 2);           
	loc += 2;   // SWAP_HERE
	tmps = INTEL_SHORT(netGame.teamKills[0]);
	memcpy(out_buffer + loc, &tmps, 2);           
	loc += 2;   // SWAP_HERE
	tmps = INTEL_SHORT(netGame.teamKills[1]);
	memcpy(out_buffer + loc, &tmps, 2);           
	loc += 2;   // SWAP_HERE
	for (i = 0; i < MAX_PLAYERS; i++) {
		tmps = INTEL_SHORT(netGame.killed[i]);
		memcpy(out_buffer + loc, &tmps, 2);       
		loc += 2;   // SWAP HERE!!!
	}
	for (i = 0; i < MAX_PLAYERS; i++) {
		tmps = INTEL_SHORT(netGame.playerKills[i]);
		memcpy(out_buffer + loc, &tmps, 2);       
		loc += 2;   // SWAP HERE!!!
	}

	tmpi = INTEL_INT (netGame.KillGoal);
	memcpy(out_buffer + loc, &tmpi, 4);           
	loc += 4;   // SWAP_HERE
	tmpi = INTEL_INT (netGame.xPlayTimeAllowed);
	memcpy(out_buffer + loc, &tmpi, 4);           
	loc += 4;   // SWAP_HERE
	tmpi = INTEL_INT (netGame.xLevelTime);
	memcpy(out_buffer + loc, &tmpi, 4);           
	loc += 4;   // SWAP_HERE
	tmpi = INTEL_INT (netGame.control_invulTime);
	memcpy(out_buffer + loc, &tmpi, 4);           
	loc += 4;   // SWAP_HERE
	tmpi = INTEL_INT (netGame.monitor_vector);
	memcpy(out_buffer + loc, &tmpi, 4);           
	loc += 4;   // SWAP_HERE
	for (i = 0; i < MAX_PLAYERS; i++) {
		tmpi = INTEL_INT (netGame.player_score[i]);
		memcpy(out_buffer + loc, &tmpi, 4);       
		loc += 4;   // SWAP_HERE
	}
	for (i = 0; i < MAX_PLAYERS; i++) {
		memcpy(out_buffer + loc, &(netGame.playerFlags[i]), 1); loc++;
	}
	tmps = INTEL_SHORT(PacketsPerSec ());
	memcpy(out_buffer + loc, &tmps, 2);                   
	loc += 2;
	memcpy(out_buffer + loc, &(netGame.bShortPackets), 1); 
	loc++;

do_send:
	if (netAddress != NULL)
		IPXSendPacketData(out_buffer, loc, server, node, netAddress);
	else if ((server == NULL) && (node == NULL))
		IPXSendBroadcastData(out_buffer, loc);
	else
		IPXSendInternetPacketData(out_buffer, loc, server, node);
}

void BEReceiveNetGamePacket(ubyte *data, tNetgameInfo *netgame, int liteFlag)
{
	int i, j;
	int loc = 0;
	short bitfield; // new_field;

	memcpy(&(netgame->nType), data + loc, 1);                      
	loc++;
	memcpy(&(netgame->nSecurity), data + loc, 4);                  
	loc += 4;
	netgame->nSecurity = INTEL_INT (netgame->nSecurity);
	memcpy(netgame->szGameName, data + loc, NETGAME_NAME_LEN+1);   
	loc += (NETGAME_NAME_LEN+1);
	memcpy(netgame->szMissionTitle, data + loc, MISSION_NAME_LEN+1); 
	loc += (MISSION_NAME_LEN+1);
	memcpy(netgame->szMissionName, data + loc, 9);                 
	loc += 9;
	memcpy(&(netgame->nLevel), data + loc, 4);                  
	loc += 4;
	netgame->nLevel = INTEL_INT (netgame->nLevel);
	memcpy(&(netgame->gameMode), data + loc, 1);                  
	loc++;
	memcpy(&(netgame->bRefusePlayers), data + loc, 1);             
	loc++;
	memcpy(&(netgame->difficulty), data + loc, 1);                
	loc++;
	memcpy(&(netgame->gameStatus), data + loc, 1);               
	loc++;
	memcpy(&(netgame->nNumPlayers), data + loc, 1);                
	loc++;
	memcpy(&(netgame->nMaxPlayers), data + loc, 1);            
	loc++;
	memcpy(&(netgame->nConnected), data + loc, 1);              
	loc++;
	memcpy(&(netgame->gameFlags), data + loc, 1);                
	loc++;
	memcpy(&(netgame->protocol_version), data + loc, 1);          
	loc++;
	memcpy(&(netgame->version_major), data + loc, 1);             
	loc++;
	memcpy(&(netgame->version_minor), data + loc, 1);             
	loc++;
	memcpy(&(netgame->teamVector), data + loc, 1);               
	loc++;

	if (liteFlag)
		return;

	memcpy(&bitfield, data + loc, 2);                             
	loc += 2;
	bitfield = INTEL_SHORT(bitfield);
	memcpy(((ubyte *)(&netgame->teamVector) + 1), &bitfield, 2);

	memcpy(&bitfield, data + loc, 2);                             
	loc += 2;
	bitfield = INTEL_SHORT(bitfield);
	memcpy(((ubyte *)(&netgame->teamVector) + 3), &bitfield, 2);

#if 0       // not used since reordering mac bitfields
	memcpy(&bitfield, data + loc, 2);                             
	loc += 2;
	new_field = 0;
	for (i = 15; i >= 0; i--) {
		if (bitfield & (1 << i))
			new_field |= (1 << (15 - i);
	}
	new_field = INTEL_SHORT(new_field);
	memcpy(((ubyte *)(&netgame->teamVector) + 1), &new_field, 2);

	memcpy(&bitfield, data + loc, 2);                             
	loc += 2;
	new_field = 0;
	for (i = 15; i >= 0; i--) {
		if (bitfield & (1 << i))
			new_field |= (1 << (15 - i);
	}
	new_field = INTEL_SHORT(new_field);
	memcpy(((ubyte *)(&netgame->teamVector) + 3), &new_field, 2);
#endif

	memcpy(netgame->team_name, data + loc, 2*(CALLSIGN_LEN+1));   
	loc += 2*(CALLSIGN_LEN+1);
	for (i = 0; i < MAX_PLAYERS; i++) {
		memcpy(&(netgame->locations[i]), data + loc, 4);          
		loc += 4;
		netgame->locations[i] = INTEL_INT (netgame->locations[i]);
	}

	for (i = 0; i < MAX_PLAYERS; i++) {
		for (j = 0; j < MAX_PLAYERS; j++) {
			memcpy(&(netgame->kills[i][j]), data + loc, 2);       
			loc += 2;
			netgame->kills[i][j] = INTEL_SHORT(netgame->kills[i][j]);
		}
	}

	memcpy(&(netgame->segments_checksum), data + loc, 2);         
	loc += 2;
	netgame->segments_checksum = INTEL_SHORT(netgame->segments_checksum);
	memcpy(&(netgame->teamKills[0]), data + loc, 2);             
	loc += 2;
	netgame->teamKills[0] = INTEL_SHORT(netgame->teamKills[0]);
	memcpy(&(netgame->teamKills[1]), data + loc, 2);             
	loc += 2;
	netgame->teamKills[1] = INTEL_SHORT(netgame->teamKills[1]);
	for (i = 0; i < MAX_PLAYERS; i++) {
		memcpy(&(netgame->killed[i]), data + loc, 2);             
		loc += 2;
		netgame->killed[i] = INTEL_SHORT(netgame->killed[i]);
	}
	for (i = 0; i < MAX_PLAYERS; i++) {
		memcpy(&(netgame->playerKills[i]), data + loc, 2);       
		loc += 2;
		netgame->playerKills[i] = INTEL_SHORT(netgame->playerKills[i]);
	}
	memcpy(&(netgame->KillGoal), data + loc, 4);                  
	loc += 4;
	netgame->KillGoal = INTEL_INT (netgame->KillGoal);
	memcpy(&(netgame->xPlayTimeAllowed), data + loc, 4);           
	loc += 4;
	netgame->xPlayTimeAllowed = INTEL_INT (netgame->xPlayTimeAllowed);

	memcpy(&(netgame->xLevelTime), data + loc, 4);                
	loc += 4;
	netgame->xLevelTime = INTEL_INT (netgame->xLevelTime);
	memcpy(&(netgame->control_invulTime), data + loc, 4);        
	loc += 4;
	netgame->control_invulTime = INTEL_INT (netgame->control_invulTime);
	memcpy(&(netgame->monitor_vector), data + loc, 4);            
	loc += 4;
	netgame->monitor_vector = INTEL_INT (netgame->monitor_vector);
	for (i = 0; i < MAX_PLAYERS; i++) {
		memcpy(&(netgame->player_score[i]), data + loc, 4);       
		loc += 4;
		netgame->player_score[i] = INTEL_INT (netgame->player_score[i]);
	}
	for (i = 0; i < MAX_PLAYERS; i++) {
		memcpy(&(netgame->playerFlags[i]), data + loc, 1);       
		loc++;
	}
	memcpy(&(netgame->nPacketsPerSec), data + loc, 2);             
	loc += 2;
	netgame->nPacketsPerSec = INTEL_SHORT(netgame->nPacketsPerSec);
	memcpy(&(netgame->bShortPackets), data + loc, 1);              
	loc ++;

}

#define EGI_INTEL_SHORT_2BUF(_m) \
  *((short *) (out_buffer + ((char *) &extraGameInfo [1]. _m - (char *) &extraGameInfo [1]))) = INTEL_SHORT (extraGameInfo [1]. _m);

#define EGI_INTEL_INT_2BUF(_m) \
	*((int *) (out_buffer + ((char *) &extraGameInfo [1]. _m - (char *) &extraGameInfo [1]))) = INTEL_INT (extraGameInfo [1]. _m);

#define BUF2_EGI_INTEL_SHORT(_m) \
	extraGameInfo [1]. _m = INTEL_SHORT (*((short *) (out_buffer + ((char *) &extraGameInfo [1]. _m - (char *) &extraGameInfo [1]))));

#define BUF2_EGI_INTEL_INT(_m) \
	extraGameInfo [1]. _m = INTEL_INT (*((int *) (out_buffer + ((char *) &extraGameInfo [1]. _m - (char *) &extraGameInfo [1]))));

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


void BEReceiveExtraGameInfo(ubyte *data, tExtraGameInfo *extraGameInfo)
{
memcpy (&extraGameInfo [1], data, sizeof (tExtraGameInfo));
BUF2_EGI_INTEL_SHORT (entropy.nMaxVirusCapacity);
BUF2_EGI_INTEL_SHORT (entropy.nEnergyFillRate);
BUF2_EGI_INTEL_SHORT (entropy.nShieldFillRate);
BUF2_EGI_INTEL_SHORT (entropy.nShieldDamageRate);
BUF2_EGI_INTEL_INT (nSpawnDelay);
}


void BESendMissingObjFrames(ubyte *server, ubyte *node, ubyte *netAddress)
{
	int	i;
	
memcpy (out_buffer, &networkData.missingObjFrames, sizeof (networkData.missingObjFrames));
((tMissingObjFrames *) &out_buffer [0])->nMissing = INTEL_SHORT (networkData.missingObjFrames.nFrames);
((tMissingObjFrames *) &out_buffer [0])->nFrame = INTEL_SHORT (networkData.missingObjFrames.nFrame);
i = 2 * sizeof (ubyte) + sizeof (ushort);
if (netAddress != NULL)
	IPXSendPacketData(out_buffer, i, server, node, netAddress);
else if ((server == NULL) && (node == NULL))
	IPXSendBroadcastData(out_buffer, i);
else
	IPXSendInternetPacketData(out_buffer, i, server, node);
}


void BEReceiveMissingObjFrames(ubyte *server, ubyte *node, ubyte *netAddress)
{
	int	i;
	
memcpy (&networkData.missingObjFrames, out_buffer, sizeof (networkData.missingObjFrames));
networkData.missingObjFrames.nFrame = INTEL_SHORT (networkData.missingObjFrames.nFrame);
}


void BESwapObject(tObject *objP)
{
// swap the short and int entries for this tObject
objP->nSignature     = INTEL_INT (objP->nSignature);
objP->next          = INTEL_SHORT(objP->next);
objP->prev          = INTEL_SHORT(objP->prev);
objP->nSegment       = INTEL_SHORT(objP->nSegment);
objP->position.vPos.p.x         = INTEL_INT (objP->position.vPos.p.x);
objP->position.vPos.p.y         = INTEL_INT (objP->position.vPos.p.y);
objP->position.vPos.p.z         = INTEL_INT (objP->position.vPos.p.z);
objP->position.mOrient.rVec.p.x = INTEL_INT (objP->position.mOrient.rVec.p.x);
objP->position.mOrient.rVec.p.y = INTEL_INT (objP->position.mOrient.rVec.p.y);
objP->position.mOrient.rVec.p.z = INTEL_INT (objP->position.mOrient.rVec.p.z);
objP->position.mOrient.fVec.p.x = INTEL_INT (objP->position.mOrient.fVec.p.x);
objP->position.mOrient.fVec.p.y = INTEL_INT (objP->position.mOrient.fVec.p.y);
objP->position.mOrient.fVec.p.z = INTEL_INT (objP->position.mOrient.fVec.p.z);
objP->position.mOrient.uVec.p.x = INTEL_INT (objP->position.mOrient.uVec.p.x);
objP->position.mOrient.uVec.p.y = INTEL_INT (objP->position.mOrient.uVec.p.y);
objP->position.mOrient.uVec.p.z = INTEL_INT (objP->position.mOrient.uVec.p.z);
objP->size          = INTEL_INT (objP->size);
objP->shields       = INTEL_INT (objP->shields);
objP->vLastPos.p.x    = INTEL_INT (objP->vLastPos.p.x);
objP->vLastPos.p.y    = INTEL_INT (objP->vLastPos.p.y);
objP->vLastPos.p.z    = INTEL_INT (objP->vLastPos.p.z);
objP->lifeleft      = INTEL_INT (objP->lifeleft);
switch (objP->movementType) {
	case MT_PHYSICS:
		objP->mType.physInfo.velocity.p.x = INTEL_INT (objP->mType.physInfo.velocity.p.x);
		objP->mType.physInfo.velocity.p.y = INTEL_INT (objP->mType.physInfo.velocity.p.y);
		objP->mType.physInfo.velocity.p.z = INTEL_INT (objP->mType.physInfo.velocity.p.z);
		objP->mType.physInfo.thrust.p.x   = INTEL_INT (objP->mType.physInfo.thrust.p.x);
		objP->mType.physInfo.thrust.p.y   = INTEL_INT (objP->mType.physInfo.thrust.p.y);
		objP->mType.physInfo.thrust.p.z   = INTEL_INT (objP->mType.physInfo.thrust.p.z);
		objP->mType.physInfo.mass       = INTEL_INT (objP->mType.physInfo.mass);
		objP->mType.physInfo.drag       = INTEL_INT (objP->mType.physInfo.drag);
		objP->mType.physInfo.brakes     = INTEL_INT (objP->mType.physInfo.brakes);
		objP->mType.physInfo.rotVel.p.x   = INTEL_INT (objP->mType.physInfo.rotVel.p.x);
		objP->mType.physInfo.rotVel.p.y   = INTEL_INT (objP->mType.physInfo.rotVel.p.y);
		objP->mType.physInfo.rotVel.p.z   = INTEL_INT (objP->mType.physInfo.rotVel.p.z);
		objP->mType.physInfo.rotThrust.p.x = INTEL_INT (objP->mType.physInfo.rotThrust.p.x);
		objP->mType.physInfo.rotThrust.p.y = INTEL_INT (objP->mType.physInfo.rotThrust.p.y);
		objP->mType.physInfo.rotThrust.p.z = INTEL_INT (objP->mType.physInfo.rotThrust.p.z);
		objP->mType.physInfo.turnRoll   = INTEL_INT (objP->mType.physInfo.turnRoll);
		objP->mType.physInfo.flags      = INTEL_SHORT(objP->mType.physInfo.flags);
		break;

	case MT_SPINNING:
		objP->mType.spinRate.p.x = INTEL_INT (objP->mType.spinRate.p.x);
		objP->mType.spinRate.p.y = INTEL_INT (objP->mType.spinRate.p.y);
		objP->mType.spinRate.p.z = INTEL_INT (objP->mType.spinRate.p.z);
		break;
	}

switch (objP->controlType) {
	case CT_WEAPON:
		objP->cType.laserInfo.parentType = INTEL_SHORT(objP->cType.laserInfo.parentType);
		objP->cType.laserInfo.nParentObj = INTEL_SHORT(objP->cType.laserInfo.nParentObj);
		objP->cType.laserInfo.nParentSig  = INTEL_INT (objP->cType.laserInfo.nParentSig);
		objP->cType.laserInfo.creationTime = INTEL_INT (objP->cType.laserInfo.creationTime);
		objP->cType.laserInfo.nLastHitObj = INTEL_SHORT(objP->cType.laserInfo.nLastHitObj);
		if (objP->cType.laserInfo.nLastHitObj < 0)
			objP->cType.laserInfo.nLastHitObj = 0;
		else {
			gameData.objs.nHitObjects [OBJ_IDX (objP) * MAX_HIT_OBJECTS] = objP->cType.laserInfo.nLastHitObj;
			objP->cType.laserInfo.nLastHitObj = 1;
			}
		objP->cType.laserInfo.nMslLock = INTEL_SHORT(objP->cType.laserInfo.nMslLock);
		objP->cType.laserInfo.multiplier = INTEL_INT (objP->cType.laserInfo.multiplier);
		break;

	case CT_EXPLOSION:
		objP->cType.explInfo.nSpawnTime     = INTEL_INT (objP->cType.explInfo.nSpawnTime);
		objP->cType.explInfo.nDeleteTime    = INTEL_INT (objP->cType.explInfo.nDeleteTime);
		objP->cType.explInfo.nDeleteObj  = INTEL_SHORT(objP->cType.explInfo.nDeleteObj);
		objP->cType.explInfo.nAttachParent  = INTEL_SHORT(objP->cType.explInfo.nAttachParent);
		objP->cType.explInfo.nPrevAttach    = INTEL_SHORT(objP->cType.explInfo.nPrevAttach);
		objP->cType.explInfo.nNextAttach    = INTEL_SHORT(objP->cType.explInfo.nNextAttach);
		break;

	case CT_AI:
		objP->cType.aiInfo.nHideSegment         = INTEL_SHORT(objP->cType.aiInfo.nHideSegment);
		objP->cType.aiInfo.nHideIndex           = INTEL_SHORT(objP->cType.aiInfo.nHideIndex);
		objP->cType.aiInfo.nPathLength          = INTEL_SHORT(objP->cType.aiInfo.nPathLength);
		objP->cType.aiInfo.nDangerLaser     = INTEL_SHORT(objP->cType.aiInfo.nDangerLaser);
		objP->cType.aiInfo.nDangerLaserSig = INTEL_INT (objP->cType.aiInfo.nDangerLaserSig);
		objP->cType.aiInfo.xDyingStartTime     = INTEL_INT (objP->cType.aiInfo.xDyingStartTime);
		break;

	case CT_LIGHT:
		objP->cType.lightInfo.intensity = INTEL_INT (objP->cType.lightInfo.intensity);
		break;

	case CT_POWERUP:
		objP->cType.powerupInfo.count = INTEL_INT (objP->cType.powerupInfo.count);
		objP->cType.powerupInfo.creationTime = INTEL_INT (objP->cType.powerupInfo.creationTime);
		// Below commented out 5/2/96 by Matt.  I asked Allender why it was
		// here, and he didn't know, and it looks like it doesn't belong.
		// if (objP->id == POW_VULCAN)
		// objP->cType.powerupInfo.count = VULCAN_WEAPON_AMMO_AMOUNT;
		break;

	}

switch (objP->renderType) {
	case RT_MORPH:
	case RT_POLYOBJ: {
		int i;
		objP->rType.polyObjInfo.nModel      = INTEL_INT (objP->rType.polyObjInfo.nModel);
		for (i=0;i<MAX_SUBMODELS;i++) {
			objP->rType.polyObjInfo.animAngles[i].p = INTEL_INT (objP->rType.polyObjInfo.animAngles[i].p);
			objP->rType.polyObjInfo.animAngles[i].b = INTEL_INT (objP->rType.polyObjInfo.animAngles[i].b);
			objP->rType.polyObjInfo.animAngles[i].h = INTEL_INT (objP->rType.polyObjInfo.animAngles[i].h);
		}
		objP->rType.polyObjInfo.nSubObjFlags   = INTEL_INT (objP->rType.polyObjInfo.nSubObjFlags);
		objP->rType.polyObjInfo.nTexOverride  = INTEL_INT (objP->rType.polyObjInfo.nTexOverride);
		objP->rType.polyObjInfo.nAltTextures   = INTEL_INT (objP->rType.polyObjInfo.nAltTextures);
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

// needs to be recoded to actually work with big endian!
//--unused-- //Finds the difference between block1 and block2.  Fills in diff_buffer and
//--unused-- //returns the size of diff_buffer.
//--unused-- int netmisc_find_diff(void *block1, void *block2, int block_size, void *diff_buffer)
//--unused-- {
//--unused-- 	int mode;
//--unused-- 	ushort *c1, *c2, *diff_start, *c3;
//--unused-- 	int i, j, size, diff, n , same;
//--unused--
//--unused-- 	size=(block_size+1)/sizeof(ushort);
//--unused-- 	c1 = (ushort *)block1;
//--unused-- 	c2 = (ushort *)block2;
//--unused-- 	c3 = (ushort *)diff_buffer;
//--unused--
//--unused-- 	mode = same = diff = n = 0;
//--unused--
//--unused-- 	for (i=0; i<size; i++, c1++, c2++) {
//--unused-- 		if (*c1 != *c2) {
//--unused-- 			if (mode==0) {
//--unused-- 				mode = 1;
//--unused-- 				c3[n++] = same;
//--unused-- 				same=0; diff=0;
//--unused-- 				diff_start = c2;
//--unused-- 			}
//--unused-- 			*c1 = *c2;
//--unused-- 			diff++;
//--unused-- 			if (diff==65535) {
//--unused-- 				mode = 0;
//--unused-- 				// send how many diff ones.
//--unused-- 				c3[n++]=diff;
//--unused-- 				// send all the diff ones.
//--unused-- 				for (j=0; j<diff; j++)
//--unused-- 					c3[n++] = diff_start[j];
//--unused-- 				same=0; diff=0;
//--unused-- 				diff_start = c2;
//--unused-- 			}
//--unused-- 		} else {
//--unused-- 			if (mode==1) {
//--unused-- 				mode=0;
//--unused-- 				// send how many diff ones.
//--unused-- 				c3[n++]=diff;
//--unused-- 				// send all the diff ones.
//--unused-- 				for (j=0; j<diff; j++)
//--unused-- 					c3[n++] = diff_start[j];
//--unused-- 				same=0; diff=0;
//--unused-- 				diff_start = c2;
//--unused-- 			}
//--unused-- 			same++;
//--unused-- 			if (same==65535) {
//--unused-- 				mode=1;
//--unused-- 				// send how many the same
//--unused-- 				c3[n++] = same;
//--unused-- 				same=0; diff=0;
//--unused-- 				diff_start = c2;
//--unused-- 			}
//--unused-- 		}
//--unused--
//--unused-- 	}
//--unused-- 	if (mode==0) {
//--unused-- 		// send how many the same
//--unused-- 		c3[n++] = same;
//--unused-- 	} else {
//--unused-- 		// send how many diff ones.
//--unused-- 		c3[n++]=diff;
//--unused-- 		// send all the diff ones.
//--unused-- 		for (j=0; j<diff; j++)
//--unused-- 			c3[n++] = diff_start[j];
//--unused-- 	}
//--unused--
//--unused-- 	return n*2;
//--unused-- }

//--unused-- //Applies diff_buffer to block1 to create a new block1.  Returns the final
//--unused-- //size of block1.
//--unused-- int netmisc_apply_diff(void *block1, void *diff_buffer, int diff_size)
//--unused-- {
//--unused-- 	unsigned int i, j, n, size;
//--unused-- 	ushort *c1, *c2;
//--unused--
//--unused-- 	c1 = (ushort *)diff_buffer;
//--unused-- 	c2 = (ushort *)block1;
//--unused--
//--unused-- 	size = diff_size/2;
//--unused--
//--unused-- 	i=j=0;
//--unused-- 	while (1) {
//--unused-- 		j += c1[i];         // Same
//--unused-- 		i++;
//--unused-- 		if (i>=size) break;
//--unused-- 		n = c1[i];          // ndiff
//--unused-- 		i++;
//--unused-- 		if (n>0) {
//--unused-- 			//Assert(n* < 256);
//--unused-- 			memcpy(&c2[j], &c1[i], n*2);
//--unused-- 			i += n;
//--unused-- 			j += n;
//--unused-- 		}
//--unused-- 		if (i>=size) break;
//--unused-- 	}
//--unused-- 	return j*2;
//--unused-- }
