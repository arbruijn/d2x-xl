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

#include "descent.h"
#include "pstypes.h"
#include "mono.h"
#include "byteswap.h"

#if defined(WORDS_BIGENDIAN) || defined(__BIG_ENDIAN__)

#include "segment.h"
#include "gameseg.h"
#include "network.h"
#include "network_lib.h"
#include "wall.h"

#include "ipx.h"
#include "multi.h"
#include "object.h"
#include "powerup.h"
#include "netmisc.h"
#include "error.h"

#endif

//------------------------------------------------------------------------------
// routine to calculate the checksum of the segments.  We add these specialized routines
// since the current way is byte order dependent.

ubyte* CalcCheckSum (ubyte* bufP, int len, uint& sum1, uint& sum2)
{
while(len--) {
	sum1 += *bufP++;
	if (sum1 >= 255) 
		sum1 -= 255;
	sum2 += sum1;
	}
return bufP;
}

//------------------------------------------------------------------------------

#if defined(WORDS_BIGENDIAN) || defined(__BIG_ENDIAN__)

static ubyte nmDataBuf [MAX_PACKETSIZE];    // used for tmp netgame packets as well as sending CObject data
static ubyte *nmBufP = NULL;

// if using the following macros in loops, the loop's body *must* be enclosed in curly braces,
// or the macros won't work as intended, as the buffer pointer nmBufI will only be incremented
// after the loop has been fully executed!

#define	BE_SET_INT(_src)						*(reinterpret_cast<int*> (nmBufP + nmBufI)) = INTEL_INT ((int) (_src)); nmBufI += 4
#define	BE_SET_SHORT(_src)					*(reinterpret_cast<short*> (nmBufP + nmBufI)) = INTEL_SHORT ((short) (_src)); nmBufI += 2
#define	BE_SET_BYTE(_src)						nmBufP [nmBufI++] = (ubyte) (_src)
#define	BE_SET_BYTES(_src,_srcSize)		memcpy (nmBufP + nmBufI, _src, _srcSize); nmBufI += (_srcSize)

#define	BE_GET_INT(_dest)						(_dest) = INTEL_INT (*(reinterpret_cast<int*> (nmBufP + nmBufI))); nmBufI += 4
#define	BE_GET_SHORT(_dest)					(_dest) = INTEL_SHORT (*(reinterpret_cast<short*> (nmBufP + nmBufI))); nmBufI += 2
#define	BE_GET_BYTE(_dest)					(_dest) = nmBufP [nmBufI++]
#define	BE_GET_BYTES(_dest,_destSize)		memcpy (_dest, nmBufP + nmBufI, _destSize); nmBufI += (_destSize)

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
BE_GET_BYTE (info->versionMajor);                            
BE_GET_BYTE (info->versionMinor);                            
BE_GET_BYTE (info->computerType);            
BE_GET_BYTE (info->connected);
//BE_GET_SHORT (info->socket);
info->socket = *(reinterpret_cast<short*> (data + nmBufI));	//don't swap!
nmBufI += 2;
BE_GET_BYTE (info->rank);     
}

//------------------------------------------------------------------------------

void BESendNetPlayersPacket (ubyte *server, ubyte *node)
{
	int i;
	int nmBufI = 0;

nmBufP = nmDataBuf;
#if DBG
memset (nmBufP, 0, IPX_DATALIMIT);	//this takes time and shouldn't be necessary
#endif
BE_SET_BYTE (netPlayers.nType);                            
BE_SET_INT (netPlayers.nSecurity);
for (i = 0; i < MAX_PLAYERS + 4; i++) {
	BE_SET_BYTES (netPlayers.players [i].callsign, CALLSIGN_LEN + 1); 
	BE_SET_BYTES (netPlayers.players [i].network.ipx.server, 4);    
	BE_SET_BYTES (netPlayers.players [i].network.ipx.node, 6);      
	BE_SET_BYTE (netPlayers.players [i].versionMajor);      
	BE_SET_BYTE (netPlayers.players [i].versionMinor);      
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
for (i = 0; i < MAX_PLAYERS + 4; i++)
	BEReceiveNetPlayerInfo (nmBufP, pinfo->players + i);
}

//------------------------------------------------------------------------------

void BESendSequencePacket (tSequencePacket seq, ubyte *server, ubyte *node, ubyte *netAddress)
{
	int nmBufI = 0;

nmBufP = nmDataBuf;
#if DBG
memset (nmBufP, 0, IPX_DATALIMIT);	//this takes time and shouldn't be necessary
#endif
BE_SET_BYTE (seq.nType);                                       
BE_SET_INT (seq.nSecurity);                           
nmBufI += 3;
BE_SET_BYTES (seq.player.callsign, CALLSIGN_LEN + 1);
BE_SET_BYTES (seq.player.network.ipx.server, 4);   
BE_SET_BYTES (seq.player.network.ipx.node, 6);     
BE_SET_BYTE (seq.player.versionMajor);                     
BE_SET_BYTE (seq.player.versionMinor);                     
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
BEReceiveNetPlayerInfo (data + nmBufI + 3, &(seq->player));	// +3 for pad bytes
}

//------------------------------------------------------------------------------

void BESendNetGamePacket (ubyte *server, ubyte *node, ubyte *netAddress, int bLiteData)
{
	int	i;
	short	h;
	int	nmBufI = 0;

nmBufP = nmDataBuf;
#if DBG
memset (nmBufP, 0, MAX_PACKETSIZE);	//this takes time and shouldn't be necessary
#endif
BE_SET_BYTE (netGame.nType);                 
BE_SET_INT (netGame.nSecurity);                           
BE_SET_BYTES (netGame.szGameName, NETGAME_NAME_LEN + 1);  
BE_SET_BYTES (netGame.szMissionTitle, MISSION_NAME_LEN + 1);  
BE_SET_BYTES (netGame.szMissionName, 9);            
BE_SET_INT (netGame.nLevel);
BE_SET_BYTE (netGame.gameMode);             
BE_SET_BYTE (netGame.bRefusePlayers);        
BE_SET_BYTE (netGame.difficulty);           
BE_SET_BYTE (netGame.gameStatus);          
BE_SET_BYTE (netGame.nNumPlayers);           
BE_SET_BYTE (netGame.nMaxPlayers);       
BE_SET_BYTE (netGame.nConnected);         
BE_SET_BYTE (netGame.gameFlags);           
BE_SET_BYTE (netGame.protocolVersion);     
BE_SET_BYTE (netGame.versionMajor);        
BE_SET_BYTE (netGame.versionMinor);        
BE_SET_BYTE (netGame.teamVector);          
if (bLiteData)
	goto do_send;
h = *(reinterpret_cast<ushort*> (&netGame.teamVector + 1));
BE_SET_SHORT (h);    // get the values for the first short bitfield
h = *(reinterpret_cast<ushort*> (&netGame.teamVector + 3));
BE_SET_SHORT (h);    // get the values for the first short bitfield
BE_SET_BYTES (netGame.szTeamName, 2 * (CALLSIGN_LEN + 1)); 
for (i = 0; i < MAX_PLAYERS; i++) {
	BE_SET_INT (netGame.locations [i]);
	}
int j;
for (i = 0; i < MAX_PLAYERS; i++) {
	for (j = 0; j < MAX_PLAYERS; j++) {
		BE_SET_SHORT (netGame.kills [i][j]);
		}
	}
BE_SET_SHORT (netGame.nSegmentCheckSum);
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
BE_SET_INT (netGame.controlInvulTime);
BE_SET_INT (netGame.monitorVector);
for (i = 0; i < MAX_PLAYERS; i++) {
	BE_SET_INT (netGame.playerScore[i]);
	}
BE_SET_BYTES (netGame.playerFlags, MAX_PLAYERS);
BE_SET_SHORT (PacketsPerSec ());
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
BE_GET_BYTES (netgame->szGameName, NETGAME_NAME_LEN + 1);   
BE_GET_BYTES (netgame->szMissionTitle, MISSION_NAME_LEN + 1); 
BE_GET_BYTES (netgame->szMissionName, 9);                 
BE_GET_INT (netgame->nLevel);                  
BE_GET_BYTE (netgame->gameMode);                  
BE_GET_BYTE (netgame->bRefusePlayers);             
BE_GET_BYTE (netgame->difficulty);                
BE_GET_BYTE (netgame->gameStatus);               
BE_GET_BYTE (netgame->nNumPlayers);                
BE_GET_BYTE (netgame->nMaxPlayers);            
BE_GET_BYTE (netgame->nConnected);              
BE_GET_BYTE (netgame->gameFlags);                
BE_GET_BYTE (netgame->protocolVersion);          
BE_GET_BYTE (netgame->versionMajor);             
BE_GET_BYTE (netgame->versionMinor);             
BE_GET_BYTE (netgame->teamVector);               
if (bLiteData)
	return;
BE_GET_SHORT (*reinterpret_cast<short*> ((reinterpret_cast<ubyte*> (&netgame->teamVector) + 1)));                             
BE_GET_SHORT (*reinterpret_cast<short*> ((reinterpret_cast<ubyte*> (&netgame->teamVector) + 3)));                             
BE_GET_BYTES (netgame->szTeamName, CALLSIGN_LEN + 1);   
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
BE_GET_SHORT (netgame->nSegmentCheckSum);         
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
BE_GET_INT (netgame->controlInvulTime);        
BE_GET_INT (netgame->monitorVector);            
for (i = 0; i < MAX_PLAYERS; i++) {
	BE_GET_INT (netgame->playerScore [i]);       
	}
BE_GET_BYTES (netgame->playerFlags, MAX_PLAYERS);
BE_GET_SHORT (netgame->nPacketsPerSec);             
BE_GET_BYTE (netgame->bShortPackets);              
}

//------------------------------------------------------------------------------

#define EGI_INTEL_SHORT_2BUF(_m) \
  *(reinterpret_cast<short*> (nmBufP) + (reinterpret_cast<char*> (&extraGameInfo [1]._m) - reinterpret_cast<char*> (&extraGameInfo [1]))) = INTEL_SHORT (extraGameInfo [1]._m);

#define EGI_INTEL_INT_2BUF(_m) \
	*(reinterpret_cast<int*> (nmBufP) + (reinterpret_cast<char*> (&extraGameInfo [1]._m) - reinterpret_cast<char*> (&extraGameInfo [1]))) = INTEL_INT (extraGameInfo [1]._m);

#define BUF2_EGI_INTEL_SHORT(_m) \
	extraGameInfo [1]._m = INTEL_SHORT (*reinterpret_cast<short*> (nmBufP + (reinterpret_cast<char*> (&extraGameInfo [1]._m) - reinterpret_cast<char*> (&extraGameInfo [1]))));

#define BUF2_EGI_INTEL_INT(_m) \
	extraGameInfo [1]._m = INTEL_INT (*reinterpret_cast<int*> (nmBufP + (reinterpret_cast<char*> (&extraGameInfo [1]._m) - reinterpret_cast<char*> (&extraGameInfo [1]))));

//------------------------------------------------------------------------------

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

void BESendMissingObjFrames(ubyte *server, ubyte *node, ubyte *netAddress)
{
	int	i;

memcpy (nmDataBuf, &networkData.sync [0].objs.missingFrames, sizeof (networkData.sync [0].objs.missingFrames));
reinterpret_cast<tMissingObjFrames*> (&nmDataBuf [0])->nFrame = INTEL_SHORT (networkData.sync [0].objs.missingFrames.nFrame);
i = 2 * sizeof (ubyte) + sizeof (ushort);
if (netAddress != NULL)
	IPXSendPacketData(nmDataBuf, i, server, node, netAddress);
else if ((server == NULL) && (node == NULL))
	IPXSendBroadcastData(nmDataBuf, i);
else
	IPXSendInternetPacketData(nmDataBuf, i, server, node);
}

//------------------------------------------------------------------------------

void BEReceiveMissingObjFrames(ubyte *data, tMissingObjFrames *missingObjFrames)
{
memcpy (missingObjFrames, nmDataBuf, sizeof (tMissingObjFrames));
missingObjFrames->nFrame = INTEL_SHORT (missingObjFrames->nFrame);
}

//------------------------------------------------------------------------------

void BESwapObject (CObject *objP)
{
// swap the short and int entries for this CObject
objP->info.nSignature = INTEL_INT (objP->info.nSignature);
objP->info.nNextInSeg = INTEL_SHORT (objP->info.nNextInSeg);
objP->info.nPrevInSeg = INTEL_SHORT (objP->info.nPrevInSeg);
objP->info.nSegment = INTEL_SHORT (objP->info.nSegment);
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
		objP->mType.physInfo.flags = INTEL_SHORT (objP->mType.physInfo.flags);
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
		objP->cType.laserInfo.xCreationTime = INTEL_INT (objP->cType.laserInfo.xCreationTime);
		objP->cType.laserInfo.nLastHitObj = INTEL_SHORT (objP->cType.laserInfo.nLastHitObj);
		if (objP->cType.laserInfo.nLastHitObj < 0)
			objP->cType.laserInfo.nLastHitObj = 0;
		else {
			gameData.objs.nHitObjects [OBJ_IDX (objP) * MAX_HIT_OBJECTS] = objP->cType.laserInfo.nLastHitObj;
			objP->cType.laserInfo.nLastHitObj = 1;
			}
		objP->cType.laserInfo.nHomingTarget = INTEL_SHORT (objP->cType.laserInfo.nHomingTarget);
		objP->cType.laserInfo.xScale = INTEL_INT (objP->cType.laserInfo.xScale);
		break;

	case CT_EXPLOSION:
		objP->cType.explInfo.nSpawnTime = INTEL_INT (objP->cType.explInfo.nSpawnTime);
		objP->cType.explInfo.nDeleteTime = INTEL_INT (objP->cType.explInfo.nDeleteTime);
		objP->cType.explInfo.nDeleteObj = INTEL_SHORT (objP->cType.explInfo.nDeleteObj);
		objP->cType.explInfo.attached.nParent = INTEL_SHORT (objP->cType.explInfo.attached.nParent);
		objP->cType.explInfo.attached.nPrev = INTEL_SHORT (objP->cType.explInfo.attached.nPrev);
		objP->cType.explInfo.attached.nNext = INTEL_SHORT (objP->cType.explInfo.attached.nNext);
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
		objP->cType.powerupInfo.nCount = INTEL_INT (objP->cType.powerupInfo.nCount);
		objP->cType.powerupInfo.xCreationTime = INTEL_INT (objP->cType.powerupInfo.xCreationTime);
		break;
	}

switch (objP->info.renderType) {
	case RT_MORPH:
	case RT_POLYOBJ: {
		int i;

		objP->rType.polyObjInfo.nModel = INTEL_INT (objP->rType.polyObjInfo.nModel);

		for (i = 0; i < MAX_SUBMODELS; i++)
			INTEL_ANGVEC (objP->rType.polyObjInfo.animAngles [i]);
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

void CSide::CheckSum (uint& sum1, uint& sum2)
{
	int				i, h;
	short				s;

CalcCheckSum (reinterpret_cast<ubyte*> (&m_nType), 1, sum1, sum2);
CalcCheckSum (reinterpret_cast<ubyte*> (&m_nFrame), 1, sum1, sum2);
s = INTEL_SHORT (WallNum ());
CalcCheckSum (reinterpret_cast<ubyte*> (&s), 2, sum1, sum2);
s = INTEL_SHORT (m_nBaseTex);
CalcCheckSum (reinterpret_cast<ubyte*> (&s), 2, sum1, sum2);
s = INTEL_SHORT ((m_nOvlOrient << 14) + m_nOvlTex);
CalcCheckSum (reinterpret_cast<ubyte*> (&s), 2, sum1, sum2);
for (i = 0; i < 4; i++) {
	h = INTEL_INT (int (m_uvls [i].u));
	CalcCheckSum (reinterpret_cast<ubyte*> (&h), 4, sum1, sum2);
	h = INTEL_INT (int (m_uvls [i].v));
	CalcCheckSum (reinterpret_cast<ubyte*> (&h), 4, sum1, sum2);
	h = INTEL_INT (int (m_uvls [i].l));
	CalcCheckSum (reinterpret_cast<ubyte*> (&h), 4, sum1, sum2);
	}
for (i = 0; i < 2; i++) {
	h = INTEL_INT (int (m_normals [i][X]));
	CalcCheckSum (reinterpret_cast<ubyte*> (&h), 4, sum1, sum2);
	h = INTEL_INT (int (m_normals [i][Y]));
	CalcCheckSum (reinterpret_cast<ubyte*> (&h), 4, sum1, sum2);
	h = INTEL_INT (int (m_normals [i][Z]));
	CalcCheckSum (reinterpret_cast<ubyte*> (&h), 4, sum1, sum2);
	}
}

//------------------------------------------------------------------------------

void CSegment::CheckSum (uint& sum1, uint& sum2)
{
	int	i;
	short	j;
	uint s1, s2;

for (i = 0; i < 6; i++) {
	m_sides [i].CheckSum (sum1, sum2);
	s1 = sum1;
	s2 = sum2;
	}
for (i = 0; i < MAX_SIDES_PER_SEGMENT; i++) {
	j = INTEL_SHORT (m_children [i]);
	CalcCheckSum (reinterpret_cast<ubyte*> (&j), 2, sum1, sum2);
	}
for (i = 0; i < MAX_VERTICES_PER_SEGMENT; i++) {
	j = INTEL_SHORT (m_verts [i]);
	CalcCheckSum (reinterpret_cast<ubyte*> (&j), 2, sum1, sum2);
	}
i = INTEL_INT (m_objects);
CalcCheckSum (reinterpret_cast<ubyte*> (&i), 4, sum1, sum2);
}

//------------------------------------------------------------------------------

ushort CalcSegmentCheckSum (void)
{
	uint sum1, sum2;

sum1 = sum2 = 0;
for (int i = 0; i < gameData.segs.nSegments; i++)
	SEGMENTS [i].CheckSum (sum1, sum2);
return sum1 * 256 + sum2 % 255;
}

//------------------------------------------------------------------------------
//eof
