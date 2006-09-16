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

/*
 *
 * Misc routines for network.
 *
 * Old Log:
 * Revision 1.1  1995/05/16  15:28:41  allender
 * Initial revision
 *
 * Revision 2.0  1995/02/27  11:27:24  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.3  1994/11/19  15:19:34  mike
 * rip out unused code and data.
 *
 * Revision 1.2  1994/08/09  19:31:53  john
 * Networking changes.
 *
 * Revision 1.1  1994/08/08  11:06:07  john
 * Initial revision
 *
 *
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

#if defined(WORDS_BIGENDIAN) || defined(__BIG_ENDIAN__)

#include "byteswap.h"
#include "segment.h"
#include "gameseg.h"
#include "network.h"
#include "wall.h"

#include "ipx.h"
#include "multi.h"
#ifdef NETWORK
#include "network.h"
#endif
#include "object.h"
#include "powerup.h"
#include "netmisc.h"
#include "error.h"

static ubyte nmDataBuf [IPX_MAX_DATA_SIZE];    // used for tmp netgame packets as well as sending object data
static ubyte *nmBufP = NULL;

extern struct ipx_recv_data ipx_udpSrc;

// if using the following macros in loops, the loop's body *must* be enclosed in curly braces,
// or the macros won't work as intended, as the buffer pointer nmBufI will only be incremented
// after the loop has been fully executed!

#define	BE_SET_INT(_src)						*((int *) (nmBufP + nmBufI)) = INTEL_INT ((int) (_src)); nmBufI += 4
#define	BE_SET_SHORT(_src)					*((short *) (nmBufP + nmBufI)) = INTEL_SHORT ((short) (_src)); nmBufI += 2
#define	BE_SET_BYTE(_src)						nmBufP [nmBufI++] = (ubyte) (_src)
#define	BE_SET_BYTES(_src,_srcSize)		memcpy (nmBufP + nmBufI, _src, _srcSize); nmBufI += (_srcSize)

#define	BE_GET_INT(_dest)						(_dest) = INTEL_INT (*((int *) (nmBufP + nmBufI))); nmBufI += 4
#define	BE_GET_SHORT(_dest)					(_dest) = INTEL_SHORT (*((short *) (nmBufP + nmBufI))); nmBufI += 2
#define	BE_GET_BYTE(_dest)					(_dest) = nmBufP [nmBufI++]
#define	BE_GET_BYTES(_dest,_destSize)		memcpy (_dest, nmBufP + nmBufI, _destSize); nmBufI += (_destSize)

//------------------------------------------------------------------------------
// routine to calculate the checksum of the segments.  We add these specialized routines
// since the current way is byte order dependent.

void BEDoCheckSumCalc (ubyte *b, int len, unsigned int *ps1, unsigned int *ps2)
{
	int	s1 = *ps1;
	int	s2 = *ps2;

while(len--) {
	s1 += *b++;
	if (s1 >= 255) 
		s1 -= 255;
	s2 += s1;
	}
*ps1 = s1;
*ps2 = s2;
}

//------------------------------------------------------------------------------

ushort BECalcSegmentCheckSum (void)
{
	int i, j, k;
	unsigned int sum1, sum2;
	short s;
	int t;

sum1 = sum2 = 0;
for (i = 0; i < gameData.segs.nLastSegment + 1; i++) {
	for (j = 0; j < MAX_SIDES_PER_SEGMENT; j++) {
		BEDoCheckSumCalc (&(gameData.segs.segments [i].sides [j].type), 1, &sum1, &sum2);
		BEDoCheckSumCalc (&(gameData.segs.segments [i].sides [j].frame_num), 1, &sum1, &sum2);
		s = INTEL_SHORT (WallNumI (i, j));
		BEDoCheckSumCalc ((ubyte *)&s, 2, &sum1, &sum2);
		s = INTEL_SHORT (gameData.segs.segments [i].sides [j].tmap_num);
		BEDoCheckSumCalc ((ubyte *)&s, 2, &sum1, &sum2);
		s = INTEL_SHORT (gameData.segs.segments [i].sides [j].tmap_num2);
		BEDoCheckSumCalc ((ubyte *)&s, 2, &sum1, &sum2);
		for (k = 0; k < 4; k++) {
			t = INTEL_INT (((int) gameData.segs.segments [i].sides [j].uvls [k].u));
			BEDoCheckSumCalc ((ubyte *)&t, 4, &sum1, &sum2);
			t = INTEL_INT (((int) gameData.segs.segments [i].sides [j].uvls [k].v));
			BEDoCheckSumCalc ((ubyte *)&t, 4, &sum1, &sum2);
			t = INTEL_INT (((int) gameData.segs.segments [i].sides [j].uvls [k].l));
			BEDoCheckSumCalc ((ubyte *)&t, 4, &sum1, &sum2);
			}
		for (k = 0; k < 2; k++) {
			t = INTEL_INT (((int) gameData.segs.segments [i].sides [j].normals [k].x));
			BEDoCheckSumCalc ((ubyte *)&t, 4, &sum1, &sum2);
			t = INTEL_INT (((int) gameData.segs.segments [i].sides [j].normals [k].y));
			BEDoCheckSumCalc ((ubyte *)&t, 4, &sum1, &sum2);
			t = INTEL_INT (((int) gameData.segs.segments [i].sides [j].normals [k].z));
			BEDoCheckSumCalc ((ubyte *)&t, 4, &sum1, &sum2);
			}
		}
	for (j = 0; j < MAX_SIDES_PER_SEGMENT; j++) {
		s = INTEL_SHORT (gameData.segs.segments [i].children[j]);
		BEDoCheckSumCalc ((ubyte *)&s, 2, &sum1, &sum2);
		}
	for (j = 0; j < MAX_VERTICES_PER_SEGMENT; j++) {
		s = INTEL_SHORT (gameData.segs.segments [i].verts [j]);
		BEDoCheckSumCalc ((ubyte *)&s, 2, &sum1, &sum2);
		}
	t = INTEL_INT (gameData.segs.segments [i].objects);
	BEDoCheckSumCalc ((ubyte *)&t, 4, &sum1, &sum2);
	}
sum2 %= 255;
return ((sum1 << 8) + sum2);
}

//------------------------------------------------------------------------------
// following are routine for big endian hardware that will swap the elements of
// structures send through the networking code.  The structures and
// this code must be kept in total sync

void BEReceiveNetPlayerInfo (ubyte *data, netplayer_info *info)
{
	int nmBufI = 0;

nmBufP = data;
BE_GET_BYTES (info->callsign, CALLSIGN_LEN + 1);       
BE_GET_BYTES (info->network.ipx.server, 4);       
BE_GET_BYTES (info->network.ipx.node, 6);         
BE_GET_BYTE (info->version_major);                            
BE_GET_BYTE (info->version_minor);                            
BE_GET_BYTE (info->computer_type);            
BE_GET_BYTE (info->connected);
//BE_GET_SHORT (info->socket);
info->socket = *((short *) (data + nmBufI));	//don't swap!
BE_GET_BYTE (info->rank);                      
}

//------------------------------------------------------------------------------

void BESendNetPlayersPacket (ubyte *server, ubyte *node)
{
	int i;
	int nmBufI = 0;

nmBufP = nmDataBuf;
#ifdef _DEBUG
memset (nmBufP, 0, IPX_MAX_DATA_SIZE);	//this takes time and shouldn't be necessary
#endif
BE_SET_BYTE (netPlayers.type);                            
BE_SET_INT (netPlayers.Security);
for (i = 0; i < MAX_PLAYERS + 4; i++) {
	BE_SET_BYTES (netPlayers.players [i].callsign, CALLSIGN_LEN + 1); 
	BE_SET_BYTES (netPlayers.players [i].network.ipx.server, 4);    
	BE_SET_BYTES (netPlayers.players [i].network.ipx.node, 6);      
	BE_SET_BYTE (netPlayers.players [i].version_major);      
	BE_SET_BYTE (netPlayers.players [i].version_minor);      
	BE_SET_BYTE (netPlayers.players [i].computer_type);      
	BE_SET_BYTE (netPlayers.players [i].connected);          
	BE_SET_SHORT (netPlayers.players [i].socket);
	BE_SET_BYTE (netPlayers.players [i].rank);               
	}
if (!server && !node)
	IPXSendBroadcastData (nmBufP, nmBufI);
else
	IPXSendInternetPacketData (nmBufP, nmBufI, server, node);
}

//------------------------------------------------------------------------------

void BEReceiveNetPlayersPacket (ubyte *data, allNetPlayers_info *pinfo)
{
	int i, nmBufI = 0;

nmBufP = data;
BE_GET_BYTE (pinfo->type);                            
BE_GET_INT (pinfo->Security);        
for (i = 0; i < MAX_PLAYERS + 4; i++) {
	BEReceiveNetPlayerInfo (data + nmBufI, pinfo->players + i);
	nmBufI += 26;          // sizeof(netplayer_info) on the PC
	}
}

//------------------------------------------------------------------------------

void BESendSequencePacket (sequence_packet seq, ubyte *server, ubyte *node, ubyte *net_address)
{
	int nmBufI = 0;

nmBufP = nmDataBuf;
#ifdef _DEBUG
memset (nmBufP, 0, IPX_MAX_DATA_SIZE);	//this takes time and shouldn't be necessary
#endif
BE_SET_BYTE (seq.type);                                       
BE_SET_INT (seq.Security);                           
nmBufI += 3;
BE_SET_BYTES (seq.player.callsign, CALLSIGN_LEN + 1);
BE_SET_BYTES (seq.player.network.ipx.server, 4);   
BE_SET_BYTES (seq.player.network.ipx.node, 6);     
BE_SET_BYTE (seq.player.version_major);                     
BE_SET_BYTE (seq.player.version_minor);                     
BE_SET_BYTE (seq.player.computer_type);                     
BE_SET_BYTE (seq.player.connected);                         
BE_SET_SHORT (seq.player.socket);                           
BE_SET_BYTE (seq.player.rank);                                
if (net_address)
	IPXSendPacketData (nmBufP, nmBufI, server, node, net_address);
else if (!server && !node)
	IPXSendBroadcastData (nmBufP, nmBufI);
else
	IPXSendInternetPacketData (nmBufP, nmBufI, server, node);
}

//------------------------------------------------------------------------------

void BEReceiveSequencePacket (ubyte *data, sequence_packet *seq)
{
	int nmBufI = 0;

nmBufP = data;
BE_GET_BYTE (seq->type);                        
BE_GET_INT (seq->Security);  
nmBufI += 3;   // +3 for pad bytes
BEReceiveNetPlayerInfo (data + nmBufI, &(seq->player));
}

//------------------------------------------------------------------------------

void BESendNetGamePacket (ubyte *server, ubyte *node, ubyte *net_address, int bLiteData)
{
	int	i;
	short	*ps;
	int	nmBufI = 0;

nmBufP = nmDataBuf;
#ifdef _DEBUG
memset (nmBufP, 0, IPX_MAX_DATA_SIZE);	//this takes time and shouldn't be necessary
#endif
BE_SET_BYTE (netGame.type);                 
BE_SET_INT (netGame.Security);                           
BE_SET_BYTES (netGame.game_name, NETGAME_NAME_LEN + 1);  
BE_SET_BYTES (netGame.mission_title, MISSION_NAME_LEN + 1);  
BE_SET_BYTES (netGame.mission_name, 9);            
BE_SET_INT (netGame.levelnum);
BE_SET_BYTE (netGame.gamemode);             
BE_SET_BYTE (netGame.RefusePlayers);        
BE_SET_BYTE (netGame.difficulty);           
BE_SET_BYTE (netGame.game_status);          
BE_SET_BYTE (netGame.numplayers);           
BE_SET_BYTE (netGame.max_numplayers);       
BE_SET_BYTE (netGame.numconnected);         
BE_SET_BYTE (netGame.game_flags);           
BE_SET_BYTE (netGame.protocol_version);     
BE_SET_BYTE (netGame.version_major);        
BE_SET_BYTE (netGame.version_minor);        
BE_SET_BYTE (netGame.team_vector);          
if (bLiteData)
	goto do_send;
BE_SET_SHORT (((ubyte *) &netGame.team_vector) + 1);    // get the values for the first short bitfield
BE_SET_SHORT (((ubyte *) &netGame.team_vector) + 3);    // get the values for the first short bitfield
BE_SET_BYTES (netGame.team_name, 2 * (CALLSIGN_LEN + 1)); 
for (i = 0; i < MAX_PLAYERS; i++) {
	BE_SET_INT (netGame.locations [i]);
	}
#if 1
for (i = MAX_PLAYERS * MAX_PLAYERS, ps = &netGame.kills [0][0]; i; i--, ps++) {
	BE_SET_SHORT (*ps);
	}
#else
{
int j;
for (i = 0; i < MAX_PLAYERS; i++) {
	for (j = 0; j < MAX_PLAYERS; j++) {
		BE_SET_SHORT (netGame.kills [i][j]);
		}
	}
}
#endif
BE_SET_SHORT (netGame.segments_checksum);
BE_SET_SHORT (netGame.team_kills [0]);
BE_SET_SHORT (netGame.team_kills [1]);
for (i = 0; i < MAX_PLAYERS; i++) {
	BE_SET_SHORT (netGame.killed [i]);
	}
for (i = 0; i < MAX_PLAYERS; i++) {
	BE_SET_SHORT (netGame.player_kills [i]);
	}
BE_SET_INT (netGame.KillGoal);
BE_SET_INT (netGame.PlayTimeAllowed);
BE_SET_INT (netGame.level_time);
BE_SET_INT (netGame.control_invul_time);
BE_SET_INT (netGame.monitor_vector);
for (i = 0; i < MAX_PLAYERS; i++) {
	BE_SET_INT (netGame.player_score[i]);
	}
BE_SET_BYTES (netGame.player_flags, MAX_PLAYERS);
BE_SET_SHORT (netGame.nPacketsPerSec);
BE_SET_BYTE (netGame.bShortPackets); 

do_send:

if (net_address)
	IPXSendPacketData(nmBufP, nmBufI, server, node, net_address);
else if (!server && !node)
	IPXSendBroadcastData(nmBufP, nmBufI);
else
	IPXSendInternetPacketData(nmBufP, nmBufI, server, node);
}

//------------------------------------------------------------------------------

void BEReceiveNetGamePacket (ubyte *data, netgame_info *netgame, int bLiteData)
{
	int	i;
	short	*ps;
	int	nmBufI = 0;

nmBufP = data;
BE_GET_BYTE (netgame->type);                      
BE_GET_INT (netgame->Security);                  
BE_GET_BYTES (netgame->game_name, NETGAME_NAME_LEN + 1);   
BE_GET_BYTES (netgame->mission_title, MISSION_NAME_LEN + 1); 
BE_GET_BYTES (netgame->mission_name, 9);                 
BE_GET_INT (netgame->levelnum);                  
BE_GET_BYTE (netgame->gamemode);                  
BE_GET_BYTE (netgame->RefusePlayers);             
BE_GET_BYTE (netgame->difficulty);                
BE_GET_BYTE (netgame->game_status);               
BE_GET_BYTE (netgame->numplayers);                
BE_GET_BYTE (netgame->max_numplayers);            
BE_GET_BYTE (netgame->numconnected);              
BE_GET_BYTE (netgame->game_flags);                
BE_GET_BYTE (netgame->protocol_version);          
BE_GET_BYTE (netgame->version_major);             
BE_GET_BYTE (netgame->version_minor);             
BE_GET_BYTE (netgame->team_vector);               
if (bLiteData)
	return;
BE_GET_SHORT (*((short *) (((ubyte *) &netgame->team_vector) + 1)));                             
BE_GET_SHORT (*((short *) (((ubyte *) &netgame->team_vector) + 3)));                             
BE_GET_BYTES (netgame->team_name, CALLSIGN_LEN + 1);   
for (i = 0; i < MAX_PLAYERS; i++) {
	BE_GET_INT (netgame->locations [i]);          
	}
#if 1
for (i = MAX_PLAYERS * MAX_PLAYERS, ps = &netgame->kills [0][0]; i; i--, ps++) {
	BE_GET_SHORT (*ps);       
	}
#else
{
int j;
for (i = 0; i < MAX_PLAYERS; i++)
	for (j = 0; j < MAX_PLAYERS; j++) {
		BE_GET_SHORT (netgame->kills [i][j]);       
		}
	}
#endif
BE_GET_SHORT (netgame->segments_checksum);         
BE_GET_SHORT (netgame->team_kills [0]);             
BE_GET_SHORT (netgame->team_kills [1]);             
for (i = 0; i < MAX_PLAYERS; i++) {
	BE_GET_SHORT (netgame->killed [i]);             
	}
for (i = 0; i < MAX_PLAYERS; i++) {
	BE_GET_SHORT (netgame->player_kills [i]);       
	}
BE_GET_INT (netgame->KillGoal);                  
BE_GET_INT (netgame->PlayTimeAllowed);           
BE_GET_INT (netgame->level_time);                
BE_GET_INT (netgame->control_invul_time);        
BE_GET_INT (netgame->monitor_vector);            
for (i = 0; i < MAX_PLAYERS; i++) {
	BE_GET_INT (netgame->player_score [i]);       
	}
BE_GET_BYTES (netgame->player_flags, MAX_PLAYERS);
BE_GET_SHORT (netgame->nPacketsPerSec);             
BE_GET_BYTE (netgame->bShortPackets);              
}

//------------------------------------------------------------------------------

#define EGI_INTEL_SHORT_2BUF(_m) \
  *((short *) (nmBufP + ((char *) &extraGameInfo [1]._m - (char *) &extraGameInfo [1]))) = INTEL_SHORT (extraGameInfo [1]._m);

#define EGI_INTEL_INT_2BUF(_m) \
	*((int *) (nmBufP + ((char *) &extraGameInfo [1]._m - (char *) &extraGameInfo [1]))) = INTEL_INT (extraGameInfo [1]._m);

#define BUF2_EGI_INTEL_SHORT(_m) \
	extraGameInfo [1]._m = INTEL_SHORT (*((short *) (nmBufP + ((char *) &extraGameInfo [1]._m - (char *) &extraGameInfo [1]))));

#define BUF2_EGI_INTEL_INT(_m) \
	extraGameInfo [1]._m = INTEL_INT (*((int *) (nmBufP + ((char *) &extraGameInfo [1]._m - (char *) &extraGameInfo [1]))));
	
void BESendExtraGameInfo (ubyte *server, ubyte *node, ubyte *net_address)
{
	int	i = 0;

nmBufP = nmDataBuf;
memcpy (nmBufP, &extraGameInfo [1], sizeof (extraGameInfo [0]));
EGI_INTEL_SHORT_2BUF (entropy.nMaxVirusCapacity);
EGI_INTEL_SHORT_2BUF (entropy.nEnergyFillRate);
EGI_INTEL_SHORT_2BUF (entropy.nShieldFillRate);
EGI_INTEL_SHORT_2BUF (entropy.nShieldDamageRate);
EGI_INTEL_INT_2BUF (nSpawnDelay);
if (net_address)
	IPXSendPacketData (nmBufP, sizeof (extraGameInfo [0]), server, node, net_address);
else if (!server && !node)
	IPXSendBroadcastData (nmBufP, sizeof (extraGameInfo [0]));
else
	IPXSendInternetPacketData (nmBufP, sizeof (extraGameInfo [0]), server, node);
}

//------------------------------------------------------------------------------

void BEReceiveExtraGameInfo (ubyte *data, extra_gameinfo *extraGameInfo)
{
	int	i;

nmBufP = data;
memcpy (&extraGameInfo [1], nmBufP, sizeof (extraGameInfo [0]));
BUF2_EGI_INTEL_SHORT (entropy.nMaxVirusCapacity);
BUF2_EGI_INTEL_SHORT (entropy.nEnergyFillRate);
BUF2_EGI_INTEL_SHORT (entropy.nShieldFillRate);
BUF2_EGI_INTEL_SHORT (entropy.nShieldDamageRate);
BUF2_EGI_INTEL_INT (nSpawnDelay);
}

//------------------------------------------------------------------------------

void BESwapObject ( object *objP)
{
// swap the short and int entries for this object
objP->signature = INTEL_INT (objP->signature);
objP->next = INTEL_SHORT (objP->next);
objP->prev = INTEL_SHORT (objP->prev);
objP->segnum = INTEL_SHORT (objP->segnum);
INTEL_VECTOR (&objP->pos);
INTEL_MATRIX (&objP->orient);
objP->size = INTEL_INT (objP->size);
objP->shields = INTEL_INT (objP->shields);
INTEL_VECTOR (&objP->last_pos);
objP->lifeleft = INTEL_INT (objP->lifeleft);
switch (objP->movement_type) {
	case MT_PHYSICS:
		INTEL_VECTOR (&objP->mtype.phys_info.velocity);
		INTEL_VECTOR (&objP->mtype.phys_info.thrust);
		objP->mtype.phys_info.mass = INTEL_INT (objP->mtype.phys_info.mass);
		objP->mtype.phys_info.drag = INTEL_INT (objP->mtype.phys_info.drag);
		objP->mtype.phys_info.brakes = INTEL_INT (objP->mtype.phys_info.brakes);
		INTEL_VECTOR (&objP->mtype.phys_info.rotvel);
		INTEL_VECTOR (&objP->mtype.phys_info.rotthrust);
		objP->mtype.phys_info.turnroll = INTEL_INT (objP->mtype.phys_info.turnroll);
		objP->mtype.phys_info.flags = INTEL_SHORT (objP->mtype.phys_info.flags);
		break;

	case MT_SPINNING:
		INTEL_VECTOR (&objP->mtype.spin_rate);
		break;
	}

switch (objP->control_type) {
	case CT_WEAPON:
		objP->ctype.laser_info.parent_type = INTEL_SHORT (objP->ctype.laser_info.parent_type);
		objP->ctype.laser_info.parent_num = INTEL_SHORT (objP->ctype.laser_info.parent_num);
		objP->ctype.laser_info.parent_signature = INTEL_INT (objP->ctype.laser_info.parent_signature);
		objP->ctype.laser_info.creation_time = INTEL_INT (objP->ctype.laser_info.creation_time);
		objP->ctype.laser_info.last_hitobj = INTEL_SHORT (objP->ctype.laser_info.last_hitobj);
		objP->ctype.laser_info.track_goal = INTEL_SHORT (objP->ctype.laser_info.track_goal);
		objP->ctype.laser_info.multiplier = INTEL_INT (objP->ctype.laser_info.multiplier);
		break;

	case CT_EXPLOSION:
		objP->ctype.expl_info.spawn_time = INTEL_INT (objP->ctype.expl_info.spawn_time);
		objP->ctype.expl_info.delete_time = INTEL_INT (objP->ctype.expl_info.delete_time);
		objP->ctype.expl_info.delete_objnum = INTEL_SHORT (objP->ctype.expl_info.delete_objnum);
		objP->ctype.expl_info.attach_parent = INTEL_SHORT (objP->ctype.expl_info.attach_parent);
		objP->ctype.expl_info.prev_attach = INTEL_SHORT (objP->ctype.expl_info.prev_attach);
		objP->ctype.expl_info.next_attach = INTEL_SHORT (objP->ctype.expl_info.next_attach);
		break;

	case CT_AI:
		objP->ctype.ai_info.hide_segment = INTEL_SHORT (objP->ctype.ai_info.hide_segment);
		objP->ctype.ai_info.hide_index = INTEL_SHORT (objP->ctype.ai_info.hide_index);
		objP->ctype.ai_info.path_length = INTEL_SHORT (objP->ctype.ai_info.path_length);
		objP->ctype.ai_info.danger_laser_num = INTEL_SHORT (objP->ctype.ai_info.danger_laser_num);
		objP->ctype.ai_info.danger_laser_signature = INTEL_INT (objP->ctype.ai_info.danger_laser_signature);
		objP->ctype.ai_info.dying_start_time = INTEL_INT (objP->ctype.ai_info.dying_start_time);
		break;

	case CT_LIGHT:
		objP->ctype.light_info.intensity = INTEL_INT (objP->ctype.light_info.intensity);
		break;

	case CT_POWERUP:
		objP->ctype.powerup_info.count = INTEL_INT (objP->ctype.powerup_info.count);
		objP->ctype.powerup_info.creation_time = INTEL_INT (objP->ctype.powerup_info.creation_time);
		break;
	}

switch (objP->render_type) {
	case RT_MORPH:
	case RT_POLYOBJ: {
		int i;

		objP->rtype.pobj_info.model_num = INTEL_INT (objP->rtype.pobj_info.model_num);

		for (i = 0; i < MAX_SUBMODELS; i++)
			INTEL_ANGVEC (objP->rtype.pobj_info.anim_angles + i);
		objP->rtype.pobj_info.subobj_flags = INTEL_INT (objP->rtype.pobj_info.subobj_flags);
		objP->rtype.pobj_info.tmap_override = INTEL_INT (objP->rtype.pobj_info.tmap_override);
		objP->rtype.pobj_info.alt_textures = INTEL_INT (objP->rtype.pobj_info.alt_textures);
		break;
	}

	case RT_WEAPON_VCLIP:
	case RT_HOSTAGE:
	case RT_POWERUP:
	case RT_FIREBALL:
	case RT_THRUSTER:
		objP->rtype.vclip_info.nClipIndex = INTEL_INT (objP->rtype.vclip_info.nClipIndex);
		objP->rtype.vclip_info.xFrameTime = INTEL_INT (objP->rtype.vclip_info.xFrameTime);
		break;

	case RT_LASER:
		break;
	}
}

#endif /* WORDS_BIGENDIAN */

//------------------------------------------------------------------------------

// Calculates the checksum of a block of memory.
ushort NetMiscCalcCheckSum (void * vptr, int len)
{
#if defined(WORDS_BIGENDIAN) || defined(__BIG_ENDIAN__)
return BECalcSegmentCheckSum ();
#else
	ubyte *ptr = (ubyte *) vptr;
	unsigned int sum1, sum2;

sum1 = sum2 = 0;
for (; len; len--) {
	sum1 += *ptr++;
#if 1
	sum1 %= 255;
#else
	if (sum1 >= 255)
		sum1 -= 255;
#endif
	sum2 += sum1;
	}
return (sum1 * 256 + sum2 % 255);
#endif
}

//------------------------------------------------------------------------------
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
//--unused-- 			//AsserT (n* < 256);
//--unused-- 			memcpy(&c2[j], &c1[i], n*2);
//--unused-- 			i += n;
//--unused-- 			j += n;
//--unused-- 		}
//--unused-- 		if (i>=size) break;
//--unused-- 	}
//--unused-- 	return j*2;
//--unused-- }
