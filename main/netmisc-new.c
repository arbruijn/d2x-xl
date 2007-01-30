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
#include "network.h"
#include "object.h"
#include "powerup.h"
#include "netmisc.h"
#include "error.h"

static ubyte nmDataBuf [IPX_MAX_DATA_SIZE];    // used for tmp netgame packets as well as sending tObject data
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
		BEDoCheckSumCalc (&(gameData.segs.segments [i].sides [j].nType), 1, &sum1, &sum2);
		BEDoCheckSumCalc (&(gameData.segs.segments [i].sides [j].nFrame), 1, &sum1, &sum2);
		s = INTEL_SHORT (WallNumI (i, j));
		BEDoCheckSumCalc ((ubyte *)&s, 2, &sum1, &sum2);
		s = INTEL_SHORT (gameData.segs.segments [i].sides [j].nBaseTex);
		BEDoCheckSumCalc ((ubyte *)&s, 2, &sum1, &sum2);
		s = INTEL_SHORT (gameData.segs.segments [i].sides [j].nOvlTex);
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

void BEReceiveNetPlayerInfo (ubyte *data, tNetPlayerInfo *info)
{
	int nmBufI = 0;

nmBufP = data;
BE_GET_BYTES (info->callsign, CALLSIGN_LEN + 1);       
BE_GET_BYTES (info->network.ipx.server, 4);       
BE_GET_BYTES (info->network.ipx.node, 6);         
BE_GET_BYTE (info->version_major);                            
BE_GET_BYTE (info->version_minor);                            
BE_GET_BYTE (info->computerType);            
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
BE_SET_BYTE (netPlayers.nType);                            
BE_SET_INT (netPlayers.nSecurity);
for (i = 0; i < MAX_PLAYERS + 4; i++) {
	BE_SET_BYTES (netPlayers.players [i].callsign, CALLSIGN_LEN + 1); 
	BE_SET_BYTES (netPlayers.players [i].network.ipx.server, 4);    
	BE_SET_BYTES (netPlayers.players [i].network.ipx.node, 6);      
	BE_SET_BYTE (netPlayers.players [i].version_major);      
	BE_SET_BYTE (netPlayers.players [i].version_minor);      
	BE_SET_BYTE (netPlayers.players [i].computerType);      
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

void BEReceiveNetPlayersPacket (ubyte *data, tAllNetPlayersInfo *pinfo)
{
	int i, nmBufI = 0;

nmBufP = data;
BE_GET_BYTE (pinfo->nType);                            
BE_GET_INT (pinfo->nSecurity);        
for (i = 0; i < MAX_PLAYERS + 4; i++) {
	BEReceiveNetPlayerInfo (data + nmBufI, pinfo->players + i);
	nmBufI += 26;          // sizeof(tNetPlayerInfo) on the PC
	}
}

//------------------------------------------------------------------------------

void BESendSequencePacket (tSequencePacket seq, ubyte *server, ubyte *node, ubyte *netAddress)
{
	int nmBufI = 0;

nmBufP = nmDataBuf;
#ifdef _DEBUG
memset (nmBufP, 0, IPX_MAX_DATA_SIZE);	//this takes time and shouldn't be necessary
#endif
BE_SET_BYTE (seq.nType);                                       
BE_SET_INT (seq.nSecurity);                           
nmBufI += 3;
BE_SET_BYTES (seq.player.callsign, CALLSIGN_LEN + 1);
BE_SET_BYTES (seq.player.network.ipx.server, 4);   
BE_SET_BYTES (seq.player.network.ipx.node, 6);     
BE_SET_BYTE (seq.player.version_major);                     
BE_SET_BYTE (seq.player.version_minor);                     
BE_SET_BYTE (seq.player.computerType);                     
BE_SET_BYTE (seq.player.connected);                         
BE_SET_SHORT (seq.player.socket);                           
BE_SET_BYTE (seq.player.rank);                                
if (netAddress)
	IPXSendPacketData (nmBufP, nmBufI, server, node, netAddress);
else if (!server && !node)
	IPXSendBroadcastData (nmBufP, nmBufI);
else
	IPXSendInternetPacketData (nmBufP, nmBufI, server, node);
}

//------------------------------------------------------------------------------

void BEReceiveSequencePacket (ubyte *data, tSequencePacket *seq)
{
	int nmBufI = 0;

nmBufP = data;
BE_GET_BYTE (seq->nType);                        
BE_GET_INT (seq->nSecurity);  
nmBufI += 3;   // +3 for pad bytes
BEReceiveNetPlayerInfo (data + nmBufI, &(seq->player));
}

//------------------------------------------------------------------------------

void BESendNetGamePacket (ubyte *server, ubyte *node, ubyte *netAddress, int bLiteData)
{
	int	i;
	short	*ps;
	int	nmBufI = 0;

nmBufP = nmDataBuf;
#ifdef _DEBUG
memset (nmBufP, 0, IPX_MAX_DATA_SIZE);	//this takes time and shouldn't be necessary
#endif
BE_SET_BYTE (netGame.nType);                 
BE_SET_INT (netGame.nSecurity);                           
BE_SET_BYTES (netGame.game_name, NETGAME_NAME_LEN + 1);  
BE_SET_BYTES (netGame.mission_title, MISSION_NAME_LEN + 1);  
BE_SET_BYTES (netGame.szMissionName, 9);            
BE_SET_INT (netGame.levelnum);
BE_SET_BYTE (netGame.gameMode);             
BE_SET_BYTE (netGame.RefusePlayers);        
BE_SET_BYTE (netGame.difficulty);           
BE_SET_BYTE (netGame.gameStatus);          
BE_SET_BYTE (netGame.numplayers);           
BE_SET_BYTE (netGame.max_numplayers);       
BE_SET_BYTE (netGame.numconnected);         
BE_SET_BYTE (netGame.gameFlags);           
BE_SET_BYTE (netGame.protocol_version);     
BE_SET_BYTE (netGame.version_major);        
BE_SET_BYTE (netGame.version_minor);        
BE_SET_BYTE (netGame.teamVector);          
if (bLiteData)
	goto do_send;
BE_SET_SHORT (((ubyte *) &netGame.teamVector) + 1);    // get the values for the first short bitfield
BE_SET_SHORT (((ubyte *) &netGame.teamVector) + 3);    // get the values for the first short bitfield
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
BE_SET_SHORT (netGame.teamKills [0]);
BE_SET_SHORT (netGame.teamKills [1]);
for (i = 0; i < MAX_PLAYERS; i++) {
	BE_SET_SHORT (netGame.killed [i]);
	}
for (i = 0; i < MAX_PLAYERS; i++) {
	BE_SET_SHORT (netGame.playerKills [i]);
	}
BE_SET_INT (netGame.KillGoal);
BE_SET_INT (netGame.xPlayTimeAllowed);
BE_SET_INT (netGame.xLevelTime);
BE_SET_INT (netGame.control_invulTime);
BE_SET_INT (netGame.monitor_vector);
for (i = 0; i < MAX_PLAYERS; i++) {
	BE_SET_INT (netGame.player_score[i]);
	}
BE_SET_BYTES (netGame.playerFlags, MAX_PLAYERS);
BE_SET_SHORT (netGame.nPacketsPerSec);
BE_SET_BYTE (netGame.bShortPackets); 

do_send:

if (netAddress)
	IPXSendPacketData(nmBufP, nmBufI, server, node, netAddress);
else if (!server && !node)
	IPXSendBroadcastData(nmBufP, nmBufI);
else
	IPXSendInternetPacketData(nmBufP, nmBufI, server, node);
}

//------------------------------------------------------------------------------

void BEReceiveNetGamePacket (ubyte *data, tNetgameInfo *netgame, int bLiteData)
{
	int	i;
	short	*ps;
	int	nmBufI = 0;

nmBufP = data;
BE_GET_BYTE (netgame->nType);                      
BE_GET_INT (netgame->nSecurity);                  
BE_GET_BYTES (netgame->game_name, NETGAME_NAME_LEN + 1);   
BE_GET_BYTES (netgame->mission_title, MISSION_NAME_LEN + 1); 
BE_GET_BYTES (netgame->szMissionName, 9);                 
BE_GET_INT (netgame->levelnum);                  
BE_GET_BYTE (netgame->gameMode);                  
BE_GET_BYTE (netgame->RefusePlayers);             
BE_GET_BYTE (netgame->difficulty);                
BE_GET_BYTE (netgame->gameStatus);               
BE_GET_BYTE (netgame->numplayers);                
BE_GET_BYTE (netgame->max_numplayers);            
BE_GET_BYTE (netgame->numconnected);              
BE_GET_BYTE (netgame->gameFlags);                
BE_GET_BYTE (netgame->protocol_version);          
BE_GET_BYTE (netgame->version_major);             
BE_GET_BYTE (netgame->version_minor);             
BE_GET_BYTE (netgame->teamVector);               
if (bLiteData)
	return;
BE_GET_SHORT (*((short *) (((ubyte *) &netgame->teamVector) + 1)));                             
BE_GET_SHORT (*((short *) (((ubyte *) &netgame->teamVector) + 3)));                             
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
BE_GET_SHORT (netgame->teamKills [0]);             
BE_GET_SHORT (netgame->teamKills [1]);             
for (i = 0; i < MAX_PLAYERS; i++) {
	BE_GET_SHORT (netgame->killed [i]);             
	}
for (i = 0; i < MAX_PLAYERS; i++) {
	BE_GET_SHORT (netgame->playerKills [i]);       
	}
BE_GET_INT (netgame->KillGoal);                  
BE_GET_INT (netgame->xPlayTimeAllowed);           
BE_GET_INT (netgame->xLevelTime);                
BE_GET_INT (netgame->control_invulTime);        
BE_GET_INT (netgame->monitor_vector);            
for (i = 0; i < MAX_PLAYERS; i++) {
	BE_GET_INT (netgame->player_score [i]);       
	}
BE_GET_BYTES (netgame->playerFlags, MAX_PLAYERS);
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
	
void BESendExtraGameInfo (ubyte *server, ubyte *node, ubyte *netAddress)
{
nmBufP = nmDataBuf;
memcpy (nmBufP, &extraGameInfo [1], sizeof (extraGameInfo [0]));
EGI_INTEL_SHORT_2BUF (entropy.nMaxVirusCapacity);
EGI_INTEL_SHORT_2BUF (entropy.nEnergyFillRate);
EGI_INTEL_SHORT_2BUF (entropy.nShieldFillRate);
EGI_INTEL_SHORT_2BUF (entropy.nShieldDamageRate);
EGI_INTEL_INT_2BUF (nSpawnDelay);
if (netAddress)
	IPXSendPacketData (nmBufP, sizeof (extraGameInfo [0]), server, node, netAddress);
else if (!server && !node)
	IPXSendBroadcastData (nmBufP, sizeof (extraGameInfo [0]));
else
	IPXSendInternetPacketData (nmBufP, sizeof (extraGameInfo [0]), server, node);
}

//------------------------------------------------------------------------------

void BEReceiveExtraGameInfo (ubyte *data, tExtraGameInfo *extraGameInfo)
{
nmBufP = data;
memcpy (&extraGameInfo [1], nmBufP, sizeof (extraGameInfo [0]));
BUF2_EGI_INTEL_SHORT (entropy.nMaxVirusCapacity);
BUF2_EGI_INTEL_SHORT (entropy.nEnergyFillRate);
BUF2_EGI_INTEL_SHORT (entropy.nShieldFillRate);
BUF2_EGI_INTEL_SHORT (entropy.nShieldDamageRate);
BUF2_EGI_INTEL_INT (nSpawnDelay);
}

//------------------------------------------------------------------------------

void BESwapObject ( tObject *objP)
{
// swap the short and int entries for this tObject
objP->nSignature = INTEL_INT (objP->nSignature);
objP->next = INTEL_SHORT (objP->next);
objP->prev = INTEL_SHORT (objP->prev);
objP->nSegment = INTEL_SHORT (objP->nSegment);
INTEL_VECTOR (&objP->position.vPos);
INTEL_MATRIX (&objP->position.mOrient);
objP->size = INTEL_INT (objP->size);
objP->shields = INTEL_INT (objP->shields);
INTEL_VECTOR (&objP->vLastPos);
objP->lifeleft = INTEL_INT (objP->lifeleft);
switch (objP->movementType) {
	case MT_PHYSICS:
		INTEL_VECTOR (&objP->mType.physInfo.velocity);
		INTEL_VECTOR (&objP->mType.physInfo.thrust);
		objP->mType.physInfo.mass = INTEL_INT (objP->mType.physInfo.mass);
		objP->mType.physInfo.drag = INTEL_INT (objP->mType.physInfo.drag);
		objP->mType.physInfo.brakes = INTEL_INT (objP->mType.physInfo.brakes);
		INTEL_VECTOR (&objP->mType.physInfo.rotVel);
		INTEL_VECTOR (&objP->mType.physInfo.rotThrust);
		objP->mType.physInfo.turnRoll = INTEL_INT (objP->mType.physInfo.turnRoll);
		objP->mType.physInfo.flags = INTEL_SHORT (objP->mType.physInfo.flags);
		break;

	case MT_SPINNING:
		INTEL_VECTOR (&objP->mType.spinRate);
		break;
	}

switch (objP->controlType) {
	case CT_WEAPON:
		objP->cType.laserInfo.parentType = INTEL_SHORT (objP->cType.laserInfo.parentType);
		objP->cType.laserInfo.nParentObj = INTEL_SHORT (objP->cType.laserInfo.nParentObj);
		objP->cType.laserInfo.nParentSig = INTEL_INT (objP->cType.laserInfo.nParentSig);
		objP->cType.laserInfo.creationTime = INTEL_INT (objP->cType.laserInfo.creationTime);
		objP->cType.laserInfo.nLastHitObj = INTEL_SHORT (objP->cType.laserInfo.nLastHitObj);
		objP->cType.laserInfo.nTrackGoal = INTEL_SHORT (objP->cType.laserInfo.nTrackGoal);
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
		break;
	}

switch (objP->renderType) {
	case RT_MORPH:
	case RT_POLYOBJ: {
		int i;

		objP->rType.polyObjInfo.nModel = INTEL_INT (objP->rType.polyObjInfo.nModel);

		for (i = 0; i < MAX_SUBMODELS; i++)
			INTEL_ANGVEC (objP->rType.polyObjInfo.animAngles + i);
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
