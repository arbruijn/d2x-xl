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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "descent.h"
#include "multibot.h"
#include "network.h"
#include "network_lib.h"
#include "error.h"
#include "timer.h"
#include "text.h"
#include "cockpit.h"
#include "dropobject.h"
#include "fireball.h"
#include "physics.h" 
#include "byteswap.h"
#include "segmath.h"

int32_t MultiAddControlledRobot (int32_t nObject, int32_t agitation);
void MultiSendReleaseRobot (int32_t nObject);
CObject* MultiDeleteControlledRobot (int32_t nObject);
void MultiSendRobotPositionSub (int32_t nObject);
void DropStolenItems (CObject*);


//
// Code for controlling robots in multiplayer games
//

#define STANDARD_EXPL_DELAY	(I2X (1)/4)
#define MIN_CONTROL_TIME		(I2X (1))
#define ROBOT_TIMEOUT			(I2X (2))

#define MIN_TO_ADD				60

#define MAX_ROBOT_POWERUPS		((gameStates.multi.nGameType == UDP_GAME) ? 15 : 4)

#define MULTI_ROBOT_PRIORITY(nObject, nPlayer) (((nObject % 4) + nPlayer) % N_PLAYERS)

void MultiSendStolenItems (void);
int32_t MultiPowerupIsAllowed (int32_t);

//-----------------------------------------------------------------------------

#if DBG

CObject* MultiRobot (int32_t nObject, const char* pszFile, const int32_t nLine)
{
CObject *pObj = OBJECT (nObject);
if (!pObj) {
	if (*pszFile)
		PrintLog (0, "Invalid multiplayer object reference in %s (%d)\n", pszFile, nLine);
	else
		PrintLog (0, "Invalid multiplayer object reference\n");
	return NULL;
	}
if (!pObj->IsRobot ()) {
	if (*pszFile)
		PrintLog (0, "Invalid multiplayer object type in %s (%d)\n", pszFile, nLine);
	else
		PrintLog (0, "Invalid multiplayer object type\n");
	return NULL;
	}
return pObj;
}

#else

CObject* MultiRobot (int32_t nObject)
{
CObject *pObj = OBJECT (nObject);
if (!pObj) 
	return NULL;
if (!pObj->IsRobot ()) 
	return NULL;
return pObj;
}

#endif

//-----------------------------------------------------------------------------
// Determine whether or not I am allowed to move this robot.

int32_t MultiCanControlRobot (int32_t nObject, int32_t agitation)
{
CObject *pObj = OBJECT (nObject);
if (!pObj)
	return 0;
	// Claim robot if necessary.
if (LOCALPLAYER.m_bExploded)
	return 0;
if (pObj->IsBoss ()) {
	int32_t i = gameData.bosses.Find (nObject);
	if ((i >= 0) && gameData.bosses [i].m_nDying == 1)
		return 0;
	return 1;
	}
int32_t nRemOwner = pObj->cType.aiInfo.REMOTE_OWNER;
if (nRemOwner == N_LOCALPLAYER) { // Already my robot!
	int32_t nSlot = pObj->cType.aiInfo.REMOTE_SLOT_NUM;
   if ((nSlot < 0) || (nSlot >= MAX_ROBOTS_CONTROLLED))
		return 0;
	if (gameData.multigame.robots.fired [nSlot])
		return 0;
	gameData.multigame.robots.agitation [nSlot] = agitation;
	gameData.multigame.robots.lastMsgTime [nSlot] = gameData.time.xGame;
	return 1;
	}
if ((nRemOwner != -1) || (agitation < MIN_TO_ADD)) {
	if (agitation == ROBOT_FIRE_AGITATION) // Special case for firing at non-player
		return 1; // Try to vFire at player even tho we're not in control!
	return 0;
	}
return MultiAddControlledRobot (nObject, agitation);
}

//-----------------------------------------------------------------------------

void MultiCheckRobotTimeout (void)
{
	static fix lastcheck = 0;
	int32_t i, nRemOwner;

if (gameData.time.xGame > lastcheck + I2X (1)) {
	lastcheck = gameData.time.xGame;
	for (i = 0; i < MAX_ROBOTS_CONTROLLED; i++) {
		if ((gameData.multigame.robots.controlled [i] != -1) && 
			 (gameData.multigame.robots.lastSendTime [i] + ROBOT_TIMEOUT < gameData.time.xGame)) {
			CObject* pObj = MULTIROBOT (gameData.multigame.robots.controlled [i]);
			if (pObj) {
				nRemOwner = pObj->cType.aiInfo.REMOTE_OWNER;
				if (nRemOwner != N_LOCALPLAYER) {	
					gameData.multigame.robots.controlled [i] = -1;
					return;
					}
 				if (nRemOwner != N_LOCALPLAYER)
					return;
				if (gameData.multigame.robots.sendPending [i])
					MultiSendRobotPosition (gameData.multigame.robots.controlled [i], 1);
				MultiSendReleaseRobot (gameData.multigame.robots.controlled [i]);
				}
			}
		}
	}		
}

//-----------------------------------------------------------------------------

void MultiStripRobots (int32_t nPlayer)
{
	// Grab all robots away from a player 
	// (player died or exited the game)

	int32_t 	i;
	CObject*	pObj;

if (gameData.app.GameMode (GM_MULTI_ROBOTS)) {
	if (nPlayer == N_LOCALPLAYER)
		for (i = 0; i < MAX_ROBOTS_CONTROLLED; i++)
			MultiDeleteControlledRobot (gameData.multigame.robots.controlled [i]);
	FORALL_ROBOT_OBJS (pObj)
		if (pObj->cType.aiInfo.REMOTE_OWNER == nPlayer) {
			pObj->cType.aiInfo.REMOTE_OWNER = -1;
			pObj->cType.aiInfo.REMOTE_SLOT_NUM = (nPlayer == N_LOCALPLAYER) ? 4 : 0;
	  		}
	}
// Note -- only call this with playernum == N_LOCALPLAYER if all other players
// already know that we are clearing house.  This does not send a release
// message for each controlled robot!!
}

//-----------------------------------------------------------------------------

void MultiDumpRobots (void)
{
// Dump robot control info for debug purposes
if (!gameData.app.GameMode (GM_MULTI_ROBOTS))
	return;
}

//-----------------------------------------------------------------------------
// Try to add a new robot to the controlled list, return 1 if added, 0 if not.

int32_t MultiAddControlledRobot (int32_t nObject, int32_t agitation)
{
CObject *pObj = MULTIROBOT (nObject);
if (!pObj)
	return 0;

	int32_t i;
	int32_t lowest_agitation = 0x7fffffff; // MAX POSITIVE INT
	int32_t lowest_agitated_bot = -1;
	int32_t firstFreeRobot = -1;

if (pObj->IsBoss ()) // this is a boss, so make sure he gets a slot
	agitation = (agitation * 3) + N_LOCALPLAYER;  
if (pObj->cType.aiInfo.REMOTE_SLOT_NUM > 0) {
	pObj->cType.aiInfo.REMOTE_SLOT_NUM -= 1;
	return 0;
	}
for (i = 0; i < MAX_ROBOTS_CONTROLLED; i++) {
	CObject* pRobot = MULTIROBOT (gameData.multigame.robots.controlled [i]);
	if (!pRobot || !pRobot->IsRobot ()) {
		firstFreeRobot = i;
		break;
		}
	if (gameData.multigame.robots.lastMsgTime [i] + ROBOT_TIMEOUT < gameData.time.xGame) {
		if (gameData.multigame.robots.sendPending [i])
			MultiSendRobotPosition (gameData.multigame.robots.controlled [i], 1);
		MultiSendReleaseRobot (gameData.multigame.robots.controlled [i]);
		firstFreeRobot = i;
		break;
		}
	if ((gameData.multigame.robots.controlled [i] != -1) && 
		 (gameData.multigame.robots.agitation [i] < lowest_agitation) && 
		 (gameData.multigame.robots.controlledTime [i] + MIN_CONTROL_TIME < gameData.time.xGame)) {
			lowest_agitation = gameData.multigame.robots.agitation [i];
			lowest_agitated_bot = i;
		}
	}
if (firstFreeRobot != -1)  // Room left for new robots
	i = firstFreeRobot;
else if ((agitation > lowest_agitation)) {// && (lowest_agitation <= MAX_TO_DELETE)) // Replace some old robot with a more agitated one
	if (gameData.multigame.robots.sendPending [lowest_agitated_bot])
		MultiSendRobotPosition (gameData.multigame.robots.controlled [lowest_agitated_bot], 1);
	MultiSendReleaseRobot (gameData.multigame.robots.controlled [lowest_agitated_bot]);
	i = lowest_agitated_bot;
	}
else
	return 0; // Sorry, can't squeeze him in!
MultiSendClaimRobot (nObject);
gameData.multigame.robots.controlled [i] = nObject;
gameData.multigame.robots.agitation [i] = agitation;
pObj->cType.aiInfo.REMOTE_OWNER = N_LOCALPLAYER;
pObj->cType.aiInfo.REMOTE_SLOT_NUM = i;
gameData.multigame.robots.controlledTime [i] = gameData.time.xGame;
gameData.multigame.robots.lastSendTime [i] = gameData.multigame.robots.lastMsgTime [i] = gameData.time.xGame;
return 1;
}

//-----------------------------------------------------------------------------
// Delete robot CObject number nObject from list of controlled robots because it is dead

CObject* MultiDeleteControlledRobot (int32_t nObject)
{
CObject *pObj = MULTIROBOT (nObject);
if (!pObj)
	return NULL;

for (int32_t i = 0; i < MAX_ROBOTS_CONTROLLED; i++)
	if (gameData.multigame.robots.controlled [i] == nObject) {
		if (pObj->cType.aiInfo.REMOTE_SLOT_NUM != i) 
			return NULL;
		pObj->cType.aiInfo.REMOTE_OWNER = -1;
		pObj->cType.aiInfo.REMOTE_SLOT_NUM = 0;
		gameData.multigame.robots.controlled [i] = -1;
		gameData.multigame.robots.sendPending [i] = 0;
		gameData.multigame.robots.fired [i] = 0;
		}
return pObj;
}

//-----------------------------------------------------------------------------

void MultiSendClaimRobot (int32_t nObject)
{
CObject *pObj = MULTIROBOT (nObject);
if (!pObj)
	return;
// The AI tells us we should take control of this robot. 
int32_t pBuffer = 0;

gameData.multigame.msg.buf [pBuffer++] = (uint8_t) MULTI_ROBOT_CLAIM;
ADD_MSG_ID
gameData.multigame.msg.buf [pBuffer++] = N_LOCALPLAYER;
int16_t nRemoteObject = GetRemoteObjNum (nObject, reinterpret_cast<int8_t&> (gameData.multigame.msg.buf [pBuffer + 2]));
PUT_INTEL_SHORT (gameData.multigame.msg.buf + pBuffer, nRemoteObject);
pBuffer += 3;
SET_MSG_ID
MultiSendData (gameData.multigame.msg.buf, pBuffer, 2);
}

//-----------------------------------------------------------------------------

void MultiSendReleaseRobot (int32_t nObject)
{
CObject *pObj = MultiDeleteControlledRobot (nObject);
if (!pObj)
	return;

int32_t pBuffer = 0;

gameData.multigame.msg.buf [pBuffer++] = (uint8_t) MULTI_ROBOT_RELEASE;
ADD_MSG_ID
gameData.multigame.msg.buf [pBuffer++] = N_LOCALPLAYER;
int16_t nRemoteObj = GetRemoteObjNum (nObject, reinterpret_cast<int8_t&> (gameData.multigame.msg.buf [pBuffer + 2]));
PUT_INTEL_SHORT (gameData.multigame.msg.buf + pBuffer, nRemoteObj);
pBuffer += 3;
SET_MSG_ID
MultiSendData (gameData.multigame.msg.buf, pBuffer, 2);
}

//-----------------------------------------------------------------------------

int32_t MultiSendRobotFrame (int32_t sent)
{
	static int32_t last_sent = 0;

	int32_t i, sending;
	int32_t rval = 0;

for (i = 0; i < MAX_ROBOTS_CONTROLLED; i++) {
	sending = (last_sent + 1 + i) % MAX_ROBOTS_CONTROLLED;
	if ((gameData.multigame.robots.controlled [sending] != -1) && 
		 ((gameData.multigame.robots.sendPending [sending] > sent) || (gameData.multigame.robots.fired [sending] > sent))) {
		if (gameData.multigame.robots.sendPending [sending]) {
			gameData.multigame.robots.sendPending [sending] = 0;
			MultiSendRobotPositionSub (gameData.multigame.robots.controlled [sending]);
			}
		if (gameData.multigame.robots.fired [sending]) {
			gameData.multigame.robots.fired [sending] = 0;
			MultiSendData (reinterpret_cast<uint8_t*> (gameData.multigame.robots.fireBuf [sending]), 18, 1);
			}
		if (!IsNetworkGame)
			sent += 1;
		last_sent = sending;
		rval++;
		}
	}
Assert ((last_sent >= 0) && (last_sent <= MAX_ROBOTS_CONTROLLED));
return rval;
}

//-----------------------------------------------------------------------------

void MultiSendRobotPositionSub (int32_t nObject)
{
CObject *pObj = MULTIROBOT (nObject);
if (!pObj)
	return;

	int32_t pBuffer = 0;
	int16_t s;
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
	tShortPos sp;
#endif

gameData.multigame.msg.buf [pBuffer++] = MULTI_ROBOT_POSITION;  							
gameData.multigame.msg.buf [pBuffer++] = N_LOCALPLAYER;										
s = GetRemoteObjNum (nObject, reinterpret_cast<int8_t&> (gameData.multigame.msg.buf [pBuffer + 2]));
PUT_INTEL_SHORT (gameData.multigame.msg.buf + pBuffer, s);
pBuffer += 3;
#if ! (defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__))
CreateShortPos (reinterpret_cast<tShortPos*> (gameData.multigame.msg.buf + pBuffer), pObj, 0);	
pBuffer += sizeof (tShortPos);
#else
CreateShortPos (&sp, pObj, 1);
memcpy (gameData.multigame.msg.buf + pBuffer, reinterpret_cast<uint8_t*> (sp.orient), 9);
pBuffer += 9;
memcpy (gameData.multigame.msg.buf + pBuffer, reinterpret_cast<uint8_t*> (&sp.coord), 14);
pBuffer += 14;
#endif
MultiSendData (reinterpret_cast<uint8_t*> (gameData.multigame.msg.buf), pBuffer, 1);
}

//-----------------------------------------------------------------------------
// Send robot position to other player (s).  Includes a byte
// value describing whether or not they fired a weapon

void MultiSendRobotPosition (int32_t nObject, int32_t bForce)
{
	int32_t	i;

if (! IsMultiGame)
	return;
CObject *pObj = MULTIROBOT (nObject);
if (!pObj)
	return;
if (pObj->cType.aiInfo.REMOTE_OWNER != N_LOCALPLAYER)
	return;
i = pObj->cType.aiInfo.REMOTE_SLOT_NUM;
gameData.multigame.robots.lastSendTime [i] = gameData.time.xGame;
gameData.multigame.robots.sendPending [i] = 1+bForce;
if (bForce & IsNetworkGame)
	NetworkFlushData ();
return;
}

//-----------------------------------------------------------------------------

void MultiSendRobotFire (int32_t nObject, int32_t nGun, CFixVector *vFire)
{
CObject *pObj = MULTIROBOT (nObject);
if (!pObj)
	return;
	// Send robot vFire event
	int32_t pBuffer = 0;
	int16_t s;
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
	CFixVector vSwapped;
#endif

gameData.multigame.msg.buf [pBuffer++] = MULTI_ROBOT_FIRE;					
gameData.multigame.msg.buf [pBuffer++] = N_LOCALPLAYER;							
s = GetRemoteObjNum (nObject, reinterpret_cast<int8_t&> (gameData.multigame.msg.buf [pBuffer + 2]));
PUT_INTEL_SHORT (gameData.multigame.msg.buf + pBuffer, s);
pBuffer += 3;
gameData.multigame.msg.buf [pBuffer++] = nGun;								
#if ! (defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__))
memcpy (gameData.multigame.msg.buf + pBuffer, vFire, sizeof (CFixVector));         
pBuffer += sizeof (CFixVector); // 12
// --------------------------
//      Total = 18
#else
vSwapped.dir.coord.x = (fix) INTEL_INT ((int32_t) (*vFire).dir.coord.x);
vSwapped.dir.coord.y = (fix) INTEL_INT ((int32_t) (*vFire).dir.coord.y);
vSwapped.dir.coord.z = (fix) INTEL_INT ((int32_t) (*vFire).dir.coord.z);
memcpy (gameData.multigame.msg.buf + pBuffer, &vSwapped, sizeof (CFixVector)); 
pBuffer += sizeof (CFixVector);
#endif
if (pObj->cType.aiInfo.REMOTE_OWNER == N_LOCALPLAYER) {
	int32_t slot = pObj->cType.aiInfo.REMOTE_SLOT_NUM;
	if ((slot < 0) || (slot >= MAX_ROBOTS_CONTROLLED))
		return;
	if (gameData.multigame.robots.fired [slot] != 0)
		return;
	memcpy (gameData.multigame.robots.fireBuf [slot], gameData.multigame.msg.buf, pBuffer);
	gameData.multigame.robots.fired [slot] = 1;
	if (IsNetworkGame)
		NetworkFlushData ();
	}
else
	MultiSendData (gameData.multigame.msg.buf, pBuffer, 2); // Not our robot, send ASAP
}

//-----------------------------------------------------------------------------

void MultiSendRobotExplode (int32_t nObject, int32_t nKiller, char bIsThief)
{
	// Send robot explosion event to the other players

	int32_t	pBuffer = 0;
	int16_t nRemoteObj;

gameData.multigame.msg.buf [pBuffer++] = MULTI_ROBOT_EXPLODE;				
ADD_MSG_ID
gameData.multigame.msg.buf [pBuffer++] = N_LOCALPLAYER;							
nRemoteObj = int16_t (GetRemoteObjNum (nKiller, reinterpret_cast<int8_t&> (gameData.multigame.msg.buf [pBuffer + 2])));
PUT_INTEL_SHORT (gameData.multigame.msg.buf + pBuffer, nRemoteObj);                       
pBuffer += 3;
nRemoteObj = int16_t (GetRemoteObjNum (nObject, reinterpret_cast<int8_t&> (gameData.multigame.msg.buf [pBuffer + 2])));
PUT_INTEL_SHORT (gameData.multigame.msg.buf + pBuffer, nRemoteObj);                       
pBuffer += 3;
gameData.multigame.msg.buf [pBuffer++] = bIsThief;   
SET_MSG_ID
MultiSendData (gameData.multigame.msg.buf, pBuffer, 1);
MultiDeleteControlledRobot (nObject);
}

//-----------------------------------------------------------------------------

void MultiSendCreateRobot (int32_t station, int32_t nObject, int32_t nType)
{
	// Send create robot information

	int32_t pBuffer = 0;

gameData.multigame.msg.buf [pBuffer++] = MULTI_CREATE_ROBOT;					
ADD_MSG_ID
gameData.multigame.msg.buf [pBuffer++] = N_LOCALPLAYER;	
gameData.multigame.msg.buf [pBuffer++] = (int8_t) station;                         
PUT_INTEL_SHORT (gameData.multigame.msg.buf + pBuffer, nObject);                  
pBuffer += 2;
gameData.multigame.msg.buf [pBuffer++] = nType;								
SetLocalObjNumMapping (int16_t (nObject));
SET_MSG_ID
MultiSendData (gameData.multigame.msg.buf, pBuffer, 2);
}

//-----------------------------------------------------------------------------

void MultiSendBossActions (int32_t nBossObj, int32_t action, int32_t secondary, int32_t nObject)
{
CObject *pObj = MULTIROBOT (nObject);
if (!pObj)
	return;
	// Send special boss behavior information

	int32_t pBuffer = 0;

gameData.multigame.msg.buf [pBuffer++] = MULTI_BOSS_ACTIONS;					
gameData.multigame.msg.buf [pBuffer++] = N_LOCALPLAYER;
PUT_INTEL_SHORT (gameData.multigame.msg.buf + pBuffer, nBossObj);  // Which player is controlling the boss        
pBuffer += 2; // We won't network map this nObject since it's the boss
gameData.multigame.msg.buf [pBuffer++] = (int8_t)action;  // What is the boss doing?
gameData.multigame.msg.buf [pBuffer++] = (int8_t)secondary;  // More info for what he is doing
PUT_INTEL_SHORT (gameData.multigame.msg.buf + pBuffer, nObject);                  
pBuffer += 2; // Objnum of CObject created by gate-in action
if (action == 3) {
	PUT_INTEL_SHORT (gameData.multigame.msg.buf + pBuffer, pObj->info.nSegment); 
	pBuffer += 2; // Segment number CObject created in (for gate only)
	}
else 
	pBuffer += 2; // Dummy

if (action == 1) { // Teleport releases robot
	// Boss is up for grabs after teleporting
	MultiDeleteControlledRobot (nBossObj);
	}
MultiSendData (gameData.multigame.msg.buf, pBuffer, 1);
}
		
//-----------------------------------------------------------------------------
// Send create robot information

void MultiSendCreateRobotPowerups (CObject *pDelObj)
{
	int32_t	pBuffer = 0, hBufP;
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
	CFixVector vSwapped;
#endif

gameData.multigame.msg.buf [pBuffer++] = MULTI_CREATE_ROBOT_POWERUPS;			
ADD_MSG_ID
gameData.multigame.msg.buf [pBuffer++] = N_LOCALPLAYER;			
hBufP = pBuffer++; // points to the buffer location where the number of powerups contained in the data packet is stored
gameData.multigame.msg.buf [pBuffer++] = pDelObj->info.contains.nType; 				
gameData.multigame.msg.buf [pBuffer++] = pDelObj->info.contains.nId;					
PUT_INTEL_SHORT (gameData.multigame.msg.buf + pBuffer, pDelObj->info.nSegment);
pBuffer += 2;
#if !(defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__))
memcpy (gameData.multigame.msg.buf + pBuffer, &pDelObj->info.position.vPos, sizeof (CFixVector));
#else
vSwapped.dir.coord.x = (fix)INTEL_INT ((int32_t) pDelObj->info.position.vPos.dir.coord.x);
vSwapped.dir.coord.y = (fix)INTEL_INT ((int32_t) pDelObj->info.position.vPos.dir.coord.y);
vSwapped.dir.coord.z = (fix)INTEL_INT ((int32_t) pDelObj->info.position.vPos.dir.coord.z);
memcpy (gameData.multigame.msg.buf + pBuffer, &vSwapped, sizeof (CFixVector));     
#endif
pBuffer += sizeof (CFixVector);
if (gameStates.multi.nGameType == UDP_GAME) {
	int8_t nOwner;
	int32_t nRemoteObj = GetRemoteObjNum (pDelObj->Index (), nOwner);
	PUT_INTEL_SHORT (gameData.multigame.msg.buf + pBuffer, nRemoteObj);
	pBuffer += 2;
	PUT_INTEL_INT (gameData.multigame.msg.buf + pBuffer, gameStates.app.nRandSeed);
	pBuffer += 4;
	}

// successively send all robot powerups just created (their count is in gameData.multigame.create.nCount)
while (gameData.multigame.create.nCount > MAX_ROBOT_POWERUPS) {
	CObject *pObj = OBJECT (gameData.multigame.create.nObjNums [--gameData.multigame.create.nCount]);
	if (pObj)
		pObj->Die ();
	}
gameData.multigame.msg.buf [hBufP] = (uint8_t) pDelObj->info.contains.nCount;

int32_t i;
for (i = 0; i < gameData.multigame.create.nCount; i++) {
	PUT_INTEL_SHORT (gameData.multigame.msg.buf + pBuffer + 2 * i, gameData.multigame.create.nObjNums [i]); // pBuffer must always point to the start of the object data list here!
	SetLocalObjNumMapping (gameData.multigame.create.nObjNums [i]);
	}
for (; i < MAX_ROBOT_POWERUPS; i++) 
	PUT_INTEL_SHORT (gameData.multigame.msg.buf + pBuffer + 2 * i, -1); // pBuffer must always point to the start of the object data list here!
pBuffer += 2 * MAX_ROBOT_POWERUPS;
SET_MSG_ID
MultiSendData (gameData.multigame.msg.buf, pBuffer, 2);
gameData.multigame.create.nCount = 0;
}

//-----------------------------------------------------------------------------

void MultiDoClaimRobot (uint8_t* buf)
{
	int32_t pBuffer = 1;

CHECK_MSG_ID

uint8_t nPlayer = buf [pBuffer++];
int16_t nRemoteBot = GET_INTEL_SHORT (buf + pBuffer);
pBuffer += 2;
int16_t nRobot = GetLocalObjNum (nRemoteBot, (int8_t)buf [pBuffer]);

CObject* pObj = MULTIROBOT (nRobot);
if (!pObj)
	return;
if (pObj->cType.aiInfo.REMOTE_OWNER != -1)
	if (MULTI_ROBOT_PRIORITY (nRemoteBot, nPlayer) <= MULTI_ROBOT_PRIORITY (nRemoteBot, pObj->cType.aiInfo.REMOTE_OWNER))
		return;
// Perform the requested change
if (pObj->cType.aiInfo.REMOTE_OWNER == N_LOCALPLAYER)
	MultiDeleteControlledRobot (nRobot);
pObj->cType.aiInfo.REMOTE_OWNER = nPlayer;
pObj->cType.aiInfo.REMOTE_SLOT_NUM = 0;
}

//-----------------------------------------------------------------------------

void MultiDoReleaseRobot (uint8_t* buf)
{
	int32_t pBuffer = 1;

CHECK_MSG_ID

uint8_t nPlayer = buf [pBuffer++];
int16_t nRemoteBot = GET_INTEL_SHORT (buf + pBuffer);
pBuffer += 2;
int16_t nRobot = GetLocalObjNum (nRemoteBot, (int8_t)buf [pBuffer]);
CObject* pObj = MULTIROBOT (nRobot);
if (!pObj)
	return;
if (pObj->cType.aiInfo.REMOTE_OWNER != nPlayer)
	return;
// Perform the requested change
pObj->cType.aiInfo.REMOTE_OWNER = -1;
pObj->cType.aiInfo.REMOTE_SLOT_NUM = 0;
}

//-----------------------------------------------------------------------------
// Process robot movement sent by another player

void MultiDoRobotPosition (uint8_t* buf)
{
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
	tShortPos sp;
#endif
	int16_t nRobot, nRemoteBot;
	int32_t pBuffer = 1;
	uint8_t nPlayer = buf [pBuffer++];

nRemoteBot = GET_INTEL_SHORT (buf + pBuffer);
nRobot = GetLocalObjNum (nRemoteBot, (int8_t)buf [pBuffer+2]); 
pBuffer += 3;
CObject* pObj = MULTIROBOT (nRobot);
if (!pObj)
	return;
if ((pObj->info.nType != OBJ_ROBOT) || (pObj->info.nFlags & OF_EXPLODING)) 
	return;
if (pObj->cType.aiInfo.REMOTE_OWNER != nPlayer) {
	if (pObj->cType.aiInfo.REMOTE_OWNER != -1)
		return;
		// Robot claim packet must have gotten lost, let this player claim it.
	else if (pObj->cType.aiInfo.REMOTE_SLOT_NUM > 3) {
		pObj->cType.aiInfo.REMOTE_OWNER = nPlayer;
		pObj->cType.aiInfo.REMOTE_SLOT_NUM = 0;
		}
	else
		pObj->cType.aiInfo.REMOTE_SLOT_NUM++;
	}
pObj->SetThrustFromVelocity (); // Try to smooth out movement
#if ! (defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__))
ExtractShortPos (pObj, reinterpret_cast<tShortPos*> (buf+pBuffer), 0);
#else
memcpy (reinterpret_cast<uint8_t*>(sp.orient), reinterpret_cast<uint8_t*>(buf + pBuffer), 9);	
pBuffer += 9;
memcpy (reinterpret_cast<uint8_t*> (&(sp.coord)), reinterpret_cast<uint8_t*>(buf + pBuffer), 14);
ExtractShortPos (pObj, &sp, 1);
#endif
}

//-----------------------------------------------------------------------------

void MultiDoRobotFire (uint8_t* buf)
{
	// Send robot vFire event
	int32_t		pBuffer = 2;
	int16_t		nRobot;
	int16_t		nRemoteBot;
	int32_t		nGun;
	CFixVector	vFire, vGunPoint;

nRemoteBot = GET_INTEL_SHORT (buf + pBuffer);
nRobot = GetLocalObjNum (nRemoteBot, (int8_t)buf [pBuffer+2]); 
CObject* pObj = MULTIROBOT (nRobot);
if (!pObj)
	return;

pBuffer += 3;
nGun = (int8_t)buf [pBuffer++];                                      
memcpy (&vFire, buf+pBuffer, sizeof (CFixVector));
vFire.v.coord.x = (fix)INTEL_INT ((int32_t)vFire.v.coord.x);
vFire.v.coord.y = (fix)INTEL_INT ((int32_t)vFire.v.coord.y);
vFire.v.coord.z = (fix)INTEL_INT ((int32_t)vFire.v.coord.z);
if ((nRobot < 0) || (nRobot > gameData.objData.nLastObject [0]) || 
	 (pObj->info.nType != OBJ_ROBOT) || 
	 (pObj->info.nFlags & OF_EXPLODING))
	return;
// Do the firing
if (nGun == -1 || nGun==-2)
	// Drop proximity bombs
	vGunPoint = pObj->info.position.vPos + vFire;
else 
	CalcGunPoint (&vGunPoint, pObj, nGun);
if (nGun == -1) 
	CreateNewWeaponSimple (&vFire, &vGunPoint, nRobot, PROXMINE_ID, 1);
else if (nGun == -2)
	CreateNewWeaponSimple (&vFire, &vGunPoint, nRobot, SMARTMINE_ID, 1);
else if (ROBOTINFO (pObj))
	CreateNewWeaponSimple (&vFire, &vGunPoint, nRobot, ROBOTINFO (pObj)->nWeaponType, 1);
}

//-----------------------------------------------------------------------------

int32_t MultiDestroyRobot (CObject* pRobot, char bIsThief)
{
	int16_t	nRobot = OBJ_IDX (pRobot);

// Drop non-Random KEY powerups locally only!
if (IsCoopGame && (pRobot->info.contains.nCount > 0) && (pRobot->info.contains.nType == OBJ_POWERUP) && 
	 (pRobot->info.contains.nId >= POW_KEY_BLUE) && (pRobot->info.contains.nId <= POW_KEY_GOLD))
	pRobot->CreateEgg ();
/*else*/ 
if ((pRobot->cType.aiInfo.REMOTE_OWNER == -1) && IAmGameHost ()) 
	pRobot->cType.aiInfo.REMOTE_OWNER = N_LOCALPLAYER;
if (pRobot->cType.aiInfo.REMOTE_OWNER == N_LOCALPLAYER) {
	MultiDropRobotPowerups (pRobot->Index ());
	pRobot->cType.aiInfo.REMOTE_OWNER = -1;
	MultiDeleteControlledRobot (nRobot);
	}

tRobotInfo* pRobotInfo = ROBOTINFO (pRobot);
if (!pRobotInfo)
	return 0;
if (bIsThief || pRobotInfo->thief)
	DropStolenItems (pRobot);
if (pRobotInfo->bossFlag) {
	int32_t i = gameData.bosses.Find (pRobot->Index ());
	if ((i >= 0) && gameData.bosses [i].m_nDying)
		return 0;
	StartBossDeathSequence (pRobot);
	}
else if (pRobotInfo->bDeathRoll)
	StartRobotDeathSequence (pRobot);
else {
	if (pRobot->info.nId == SPECIAL_REACTOR_ROBOT)
		SpecialReactorStuff ();
	if (pRobotInfo->kamikaze)
		pRobot->Explode (1);	//	Kamikaze, explode right away, IN YOUR FACE!
	else
		pRobot->Explode (STANDARD_EXPL_DELAY);
   }
return 1;
}

//-----------------------------------------------------------------------------

CObject* MultiExplodeRobot (int32_t nRobot, int32_t nKiller, char bIsThief)
{
CObject* pObj = MULTIROBOT (nRobot);
if (!pObj)
	return NULL;
if (pObj->info.nFlags & OF_EXPLODING) // Object already exploding
	return NULL;
if (pObj->cType.aiInfo.xDyingStartTime > 0)	// death sequence initiated
	return NULL;
// Data seems valid, explode the sucker
NetworkResetObjSync (nRobot);
return MultiDestroyRobot (pObj, bIsThief) ? pObj : NULL;
}

//-----------------------------------------------------------------------------

void MultiDoRobotExplode (uint8_t* buf)
{
	// Explode robot controlled by other player
	int32_t pBuffer = 1;

CHECK_MSG_ID

pBuffer++; // skip player id
int16_t nRemoteKiller = GET_INTEL_SHORT (buf + pBuffer);
int16_t nKiller = GetLocalObjNum (nRemoteKiller, int32_t (buf [pBuffer+2])); 
pBuffer += 3;
int16_t nRemoteBot = GET_INTEL_SHORT (buf + pBuffer);
int32_t nRobot = GetLocalObjNum (nRemoteBot, int32_t (buf [pBuffer+2])); 
pBuffer += 3;
CObject* pObj = MultiExplodeRobot (nRobot, nKiller, buf [pBuffer]);
if (!pObj)
	return;
if ((nKiller == LOCALPLAYER.nObject) && ROBOTINFO (pObj))
	cockpit->AddPointsToScore (ROBOTINFO (pObj)->scoreValue);
}

//-----------------------------------------------------------------------------

void MultiDoCreateRobot (uint8_t* buf)
{
	tProducerInfo*	pRobotMaker;
	CFixVector		vObjPos, direction;
	CObject*			pObj;
	int32_t			nPlayer;
	int32_t			nProducer;
	int16_t			nRemoteObj;
	uint8_t			nType;
	int32_t			pBuffer = 1;

CHECK_MSG_ID

nPlayer = buf [pBuffer++];
nProducer = buf [pBuffer++];
nRemoteObj = GET_INTEL_SHORT (buf + pBuffer);
pBuffer += 2;
nType = buf [pBuffer];
if ((nPlayer < 0) || (nRemoteObj < 0) || (nProducer < 0) || (nProducer >= gameData.producers.nProducers) || (nPlayer >= N_PLAYERS)) 
	return;
pRobotMaker = gameData.producers.producers + nProducer;
// Play effect and sound
vObjPos = SEGMENT (pRobotMaker->nSegment)->Center ();
pObj = CreateExplosion ((int16_t) pRobotMaker->nSegment, vObjPos, I2X (10), ANIM_MORPHING_ROBOT);
if (pObj)
	ExtractOrientFromSegment (&pObj->info.position.mOrient, SEGMENT (pRobotMaker->nSegment));
if (gameData.effects.animations [0][ANIM_MORPHING_ROBOT].nSound > -1)
	audio.CreateSegmentSound (gameData.effects.animations [0][ANIM_MORPHING_ROBOT].nSound, (int16_t) pRobotMaker->nSegment, 0, vObjPos, 0, I2X (1));
// Set robot center flags, in case we become the master for the next one
pRobotMaker->bFlag = 0;
pRobotMaker->xCapacity -= gameData.producers.xEnergyToCreateOneRobot;
pRobotMaker->xTimer = 0;
if (! (pObj = CreateMorphRobot (SEGMENT (pRobotMaker->nSegment), &vObjPos, nType)))
	return; // Cannot create CObject!
pObj->info.nCreator = ((int16_t) (pRobotMaker - gameData.producers.producers.Buffer ())) | 0x80;
//	ExtractOrientFromSegment (&pObj->info.position.mOrient, &SEGMENT (pRobotMaker->nSegment));
direction = gameData.objData.pConsole->info.position.vPos - pObj->info.position.vPos;
pObj->info.position.mOrient = CFixMatrix::CreateFU(direction, pObj->info.position.mOrient.m.dir.u);
//pObj->info.position.mOrient = CFixMatrix::CreateFU(direction, &pObj->info.position.mOrient.m.v.u, NULL);
pObj->MorphStart ();
SetObjNumMapping (pObj->Index (), nRemoteObj, nPlayer);
Assert (pObj->cType.aiInfo.REMOTE_OWNER == -1);
}

//-----------------------------------------------------------------------------

void MultiDoBossActions (uint8_t* buf)
{
	// Code to handle remote-controlled boss actions

	CObject	*pBossObj;
	int16_t		nBossObj, nBossIdx;
	int32_t		nPlayer;
	int32_t		action;
	int16_t		secondary;
	int16_t		nRemoteObj, nSegment;
	int32_t		pBuffer = 1;

nPlayer = buf [pBuffer++];
nBossObj = GET_INTEL_SHORT (buf + pBuffer);           
pBuffer += 2;
action = buf [pBuffer++];
secondary = buf [pBuffer++];
nRemoteObj = GET_INTEL_SHORT (buf + pBuffer);         
pBuffer += 2;
nSegment = GET_INTEL_SHORT (buf + pBuffer);                
pBuffer += 2;
pBossObj = OBJECT (nBossObj);
if (!pBossObj)
	return;
nBossIdx = gameData.bosses.Find (nBossObj);
if (nBossIdx < 0)
	return;
if (!pBossObj->IsBoss ())
	return;
switch (action)  {
	case 1: // Teleport
		{
		int16_t nTeleportSeg;

		CFixVector vBossDir;
		if ((secondary < 0) || (secondary > gameData.bosses [nBossIdx].m_nTeleportSegs)) 
			return;
		nTeleportSeg = gameData.bosses [nBossIdx].m_teleportSegs[secondary];
		if ((nTeleportSeg < 0) || (nTeleportSeg > gameData.segData.nLastSegment)) 
			return;
		pBossObj->info.position.vPos = SEGMENT (nTeleportSeg)->Center ();
		pBossObj->RelinkToSeg (nTeleportSeg);
		gameData.bosses [nBossIdx].m_nLastTeleportTime = gameData.time.xGame;
		vBossDir = PLAYEROBJECT (nPlayer)->info.position.vPos - pBossObj->info.position.vPos;
		pBossObj->info.position.mOrient = CFixMatrix::CreateF(vBossDir);

		audio.CreateSegmentSound (gameData.effects.animations [0][ANIM_MORPHING_ROBOT].nSound, nTeleportSeg, 0, pBossObj->info.position.vPos, 0, I2X (1));
		audio.DestroyObjectSound (OBJ_IDX (pBossObj));
		audio.CreateObjectSound (SOUND_BOSS_SHARE_SEE, SOUNDCLASS_ROBOT, OBJ_IDX (pBossObj), 1, I2X (1), I2X (512));	//	I2X (5)12 means play twice as loud
		gameData.ai.localInfo [OBJ_IDX (pBossObj)].pNextrimaryFire = 0;
		if (pBossObj->cType.aiInfo.REMOTE_OWNER == N_LOCALPLAYER) {
			MultiDeleteControlledRobot (nBossObj);
//			gameData.multigame.robots.controlled [pBossObj->cType.aiInfo.REMOTE_SLOT_NUM] = -1;
			}
		pBossObj->cType.aiInfo.REMOTE_OWNER = -1; // Boss is up for grabs again!
		pBossObj->cType.aiInfo.REMOTE_SLOT_NUM = 0; // Available immediately!
		}
		break;

	case 2: // Cloak
		gameData.bosses [nBossIdx].m_nHitTime = -I2X (10);
		gameData.bosses [nBossIdx].m_nCloakStartTime = gameData.time.xGame;
		gameData.bosses [nBossIdx].m_nCloakEndTime = gameData.time.xGame + gameData.bosses [nBossIdx].m_nCloakDuration;
		pBossObj->cType.aiInfo.CLOAKED = 1;
		break;

	case 3: // Gate in robots!
		// Do some validity checking
		if ((nRemoteObj >= LEVEL_OBJECTS) || (nRemoteObj < 0) || (nSegment < 0) || (nSegment > gameData.segData.nLastSegment)) 
			return;
		// Gate one in!
		if (GateInRobot (nBossObj, (uint8_t) secondary, nSegment))
			SetObjNumMapping (gameData.multigame.create.nObjNums [0], nRemoteObj, nPlayer);
		break;

	case 4: // Start effect
		RestartEffect (BOSS_ECLIP_NUM);
		break;

	case 5:	// Stop effect
		StopEffect (BOSS_ECLIP_NUM);
		break;

	default:
		BRP; // Illegal nType to boss actions
	}
}

//-----------------------------------------------------------------------------
// Code to drop remote-controlled robot powerups

void MultiDoCreateRobotPowerups (uint8_t* buf)
{
	CObject	pDelObj;
	int32_t	nPlayer, nEggObj, i;
	int16_t	s;
	int32_t	pBuffer = 1;

CHECK_MSG_ID

nPlayer = buf [pBuffer++];		
pDelObj.info.nType = OBJ_ROBOT;
pDelObj.info.contains.nCount = buf [pBuffer++];					
pDelObj.info.contains.nType = buf [pBuffer++];					
pDelObj.info.contains.nId = buf [pBuffer++]; 					
pDelObj.info.nSegment = GET_INTEL_SHORT (buf + pBuffer);            
pBuffer += 2;
memcpy (&pDelObj.info.position.vPos, buf + pBuffer, sizeof (CFixVector));      
pBuffer += sizeof (CFixVector);
pDelObj.mType.physInfo.velocity.SetZero ();
pDelObj.info.position.vPos.v.coord.x = (fix) INTEL_INT ((int32_t) pDelObj.info.position.vPos.v.coord.x);
pDelObj.info.position.vPos.v.coord.y = (fix) INTEL_INT ((int32_t) pDelObj.info.position.vPos.v.coord.y);
pDelObj.info.position.vPos.v.coord.z = (fix) INTEL_INT ((int32_t) pDelObj.info.position.vPos.v.coord.z);
if (gameStates.multi.nGameType != UDP_GAME)
	gameStates.app.nRandSeed = 1245L;
else {
	// Check whether that robot is still alive on this client (-> out of sync!) and destroy it if it is to re-sync
	// GetRemoteObjNum will return -1 if the corresponding local object does not exist
	// MultiExplodeRobot will handle the cases of the robot not existing or still existing but already about to being destroyed
	int8_t nOwner;
	MultiExplodeRobot (GetRemoteObjNum (GET_INTEL_SHORT (buf + pBuffer), nOwner), -1, 0); 
	pBuffer += 2;
	gameStates.app.nRandSeed = (uint32_t) GET_INTEL_INT (buf + pBuffer);
	pBuffer += 4;
	}
gameStates.app.SRand (gameStates.app.nRandSeed);
gameData.multigame.create.nCount = 0;
nEggObj = pDelObj.CreateEgg ();
if (nEggObj == -1)
	return; // Object buffer full
#if DBG
if (gameData.multigame.create.nCount > MAX_ROBOT_POWERUPS)
	BRP;
#endif
while (gameData.multigame.create.nCount > MAX_ROBOT_POWERUPS) {
	CObject* pObj = OBJECT (gameData.multigame.create.nObjNums [--gameData.multigame.create.nCount]);
	if (pObj)
		pObj->Die ();
	}
for (i = 0; i < gameData.multigame.create.nCount; i++) {
	CObject* pObj = OBJECT (gameData.multigame.create.nObjNums [i]);
	if (pObj) {
		s = GET_INTEL_SHORT (buf + pBuffer);
		if (s != -1)
			SetObjNumMapping ((int16_t)gameData.multigame.create.nObjNums [i], s, nPlayer);
		else
			pObj->Die (); // Delete OBJECTS other guy didn't create one of
		}
	pBuffer += 2;
	}
}

//-----------------------------------------------------------------------------
// Code to handle dropped robot powerups in network mode ONLY!

void MultiDropRobotPowerups (int32_t nObject)
{
	CObject		*pDelObj;
	int32_t		nEggObj = -1;
	tRobotInfo	*pRobotInfo; 

pDelObj = MULTIROBOT (nObject);
if (!pDelObj)
	return;
pRobotInfo = ROBOTINFO (pDelObj);
if (!pRobotInfo)
	return;
gameData.multigame.create.nCount = 0;

if (pDelObj->info.contains.nCount > 0) { 
	if (pDelObj->info.contains.nCount > MAX_ROBOT_POWERUPS)
		pDelObj->info.contains.nCount = MAX_ROBOT_POWERUPS;
	//	If dropping a weapon that the player has, drop energy instead, unless it's vulcan, in which case drop vulcan ammo.
	if (pDelObj->info.contains.nType == OBJ_POWERUP) {
		MaybeReplacePowerupWithEnergy (pDelObj);
		if (!MultiPowerupIsAllowed (pDelObj->info.contains.nId))
			pDelObj->info.contains.nId = POW_SHIELD_BOOST;
		// No key drops in non-coop games!
		if (!IsCoopGame) {
			if ((pDelObj->info.contains.nId >= POW_KEY_BLUE) && (pDelObj->info.contains.nId <= POW_KEY_GOLD))
				pDelObj->info.contains.nCount = 0;
			}
		}
	if (gameStates.multi.nGameType != UDP_GAME)
		gameStates.app.SRand (1245L);
	else
		gameStates.app.SRand ();
	if (pDelObj->info.contains.nCount > 0)
		nEggObj = pDelObj->CreateEgg ();
	}
else if (pDelObj->cType.aiInfo.REMOTE_OWNER == -1) // No Random goodies for robots we weren't in control of
	return;
else if (pRobotInfo->containsCount) {
	if (gameStates.multi.nGameType != UDP_GAME)
		gameStates.app.SRand ();
	if (Rand (15) + 1 <= pRobotInfo->containsProb) {
		pDelObj->info.contains.nCount = Rand (pRobotInfo->containsCount) + 1; // create at least one
		if (pDelObj->info.contains.nCount > MAX_ROBOT_POWERUPS)
			pDelObj->info.contains.nCount = MAX_ROBOT_POWERUPS;
		pDelObj->info.contains.nType = pRobotInfo->containsType;
		pDelObj->info.contains.nId = pRobotInfo->containsId;
		if (pDelObj->info.contains.nType == OBJ_POWERUP) {
			MaybeReplacePowerupWithEnergy (pDelObj);
			if (!MultiPowerupIsAllowed (pDelObj->info.contains.nId))
				pDelObj->info.contains.nId = POW_SHIELD_BOOST;
			 }
		if (gameStates.multi.nGameType != UDP_GAME)
			gameStates.app.SRand (1245L);
		else
			gameStates.app.SRand ();
		if (pDelObj->info.contains.nCount > 0)
			nEggObj = pDelObj->CreateEgg (false, true);
		}
	}
if (nEggObj >= 0) // Transmit the object creation to the other players	 
	MultiSendCreateRobotPowerups (pDelObj);
}

//	-----------------------------------------------------------------------------
//	Robot *robot got whacked by player player_num and requests permission to do something about it.
//	Note: This function will be called regardless of whether gameData.app.nGameMode is a multiplayer mode, so it
//	should quick-out if not in a multiplayer mode.  On the other hand, it only gets called when a
//	player or player weapon whacks a robot, so it happens rarely.
void MultiRobotRequestChange (CObject *robot, int32_t player_num)
{
	int32_t	slot, nRemoteObj;
	int8_t dummy;

if (!gameData.app.GameMode (GM_MULTI_ROBOTS))
	return;
slot = robot->cType.aiInfo.REMOTE_SLOT_NUM;
if ((slot < 0) || (slot >= MAX_ROBOTS_CONTROLLED))
	return;
nRemoteObj = GetRemoteObjNum (OBJ_IDX (robot), dummy);
if (nRemoteObj < 0)
	return;
if ((gameData.multigame.robots.agitation [slot] < 70) || 
	 (MULTI_ROBOT_PRIORITY (nRemoteObj, player_num) > 
	  MULTI_ROBOT_PRIORITY (nRemoteObj, N_LOCALPLAYER)) || 
	  (RandShort () > 0x4400)) {
	if (gameData.multigame.robots.sendPending [slot])
		MultiSendRobotPosition (gameData.multigame.robots.controlled [slot], -1);
	MultiSendReleaseRobot (gameData.multigame.robots.controlled [slot]);
	robot->cType.aiInfo.REMOTE_SLOT_NUM = 5;  // Hands-off period
	}
}

//-----------------------------------------------------------------------------
//eof
