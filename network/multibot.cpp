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

#include "inferno.h"
#include "multibot.h"
#include "network.h"
#include "network_lib.h"
#include "error.h"
#include "timer.h"
#include "text.h"
#include "gauges.h"
#include "dropobject.h"
#include "fireball.h"
#include "physics.h" 
#include "byteswap.h"
#include "gameseg.h"

int MultiAddControlledRobot (int nObject, int agitation);
void MultiSendReleaseRobot (int nObject);
void MultiDeleteControlledRobot (int nObject);
void MultiSendRobotPositionSub (int nObject);

//
// Code for controlling robots in multiplayer games
//

#define STANDARD_EXPL_DELAY	(I2X (1)/4)
#define MIN_CONTROL_TIME		(I2X (1))
#define ROBOT_TIMEOUT			(I2X (2))

#define MIN_TO_ADD				60

#define MAX_ROBOT_POWERUPS		4

#define MULTI_ROBOT_PRIORITY(nObject, nPlayer) (((nObject % 4) + nPlayer) % gameData.multiplayer.nPlayers)

extern void MultiSendStolenItems ();
extern int MultiPowerupIsAllowed (int);

//-----------------------------------------------------------------------------
// Determine whether or not I am allowed to move this robot.

int MultiCanRemoveRobot (int nObject, int agitation)
{
	int nRemOwner;
	CObject *objP = OBJECTS + nObject;

	// Claim robot if necessary.
if (gameStates.app.bPlayerExploded)
	return 0;
#if DBG
if ((nObject < 0) || (nObject > gameData.objs.nLastObject [0])) {
	Int3 ();
	return 0;
	}
else if (objP->info.nType != OBJ_ROBOT) {
	Int3 ();
	return 0;
	}
#endif
else if (ROBOTINFO (objP->info.nId).bossFlag) {
	int i = FindBoss (nObject);
	if ((i >= 0) && gameData.boss [i].nDying == 1)
		return 0;
	return 1;
	}
nRemOwner = objP->cType.aiInfo.REMOTE_OWNER;
if (nRemOwner == gameData.multiplayer.nLocalPlayer) { // Already my robot!
	int nSlot = objP->cType.aiInfo.REMOTE_SLOT_NUM;
   if ((nSlot < 0) || (nSlot >= MAX_ROBOTS_CONTROLLED))
		return 0;
	if (gameData.multigame.robots.fired [nSlot])
		return 0;
	gameData.multigame.robots.agitation [nSlot] = agitation;
	gameData.multigame.robots.lastMsgTime [nSlot] = gameData.time.xGame;
	return 1;
	}
if ((nRemOwner != -1) || (agitation < MIN_TO_ADD)) {
	if (agitation == ROBOT_FIRE_AGITATION) // Special case for firing at non-CPlayerData
		return 1; // Try to vFire at CPlayerData even tho we're not in control!
	return 0;
	}
return MultiAddControlledRobot (nObject, agitation);
}

//-----------------------------------------------------------------------------

void MultiCheckRobotTimeout (void)
{
	static fix lastcheck = 0;
	int i, nRemOwner;

if (gameData.time.xGame > lastcheck + I2X (1)) {
	lastcheck = gameData.time.xGame;
	for (i = 0; i < MAX_ROBOTS_CONTROLLED; i++) {
		if ((gameData.multigame.robots.controlled [i] != -1) && 
			 (gameData.multigame.robots.lastSendTime [i] + ROBOT_TIMEOUT < gameData.time.xGame)) {
			nRemOwner = OBJECTS [gameData.multigame.robots.controlled [i]].cType.aiInfo.REMOTE_OWNER;
			if (nRemOwner != gameData.multiplayer.nLocalPlayer) {	
				gameData.multigame.robots.controlled [i] = -1;
				Int3 (); // Non-terminal but Rob is interesting, step over please...
				return;
				}
 			if (nRemOwner !=gameData.multiplayer.nLocalPlayer)
				return;
			if (gameData.multigame.robots.sendPending [i])
				MultiSendRobotPosition (gameData.multigame.robots.controlled [i], 1);
			MultiSendReleaseRobot (gameData.multigame.robots.controlled [i]);
			}
		}
	}		
}

//-----------------------------------------------------------------------------

void MultiStripRobots (int nPlayer)
{
	// Grab all robots away from a CPlayerData 
	// (CPlayerData died or exited the game)

	int 		i;
	CObject	*objP;

if (gameData.app.nGameMode & GM_MULTI_ROBOTS) {
	if (nPlayer == gameData.multiplayer.nLocalPlayer)
		for (i = 0; i < MAX_ROBOTS_CONTROLLED; i++)
			MultiDeleteControlledRobot (gameData.multigame.robots.controlled [i]);
	FORALL_ROBOT_OBJS (objP, i)
		if (objP->cType.aiInfo.REMOTE_OWNER == nPlayer) {
			objP->cType.aiInfo.REMOTE_OWNER = -1;
			objP->cType.aiInfo.REMOTE_SLOT_NUM = (nPlayer == gameData.multiplayer.nLocalPlayer) ? 4 : 0;
	  		}
	}
// Note -- only call this with playernum == gameData.multiplayer.nLocalPlayer if all other players
// already know that we are clearing house.  This does not send a release
// message for each controlled robot!!
}

//-----------------------------------------------------------------------------

void MultiDumpRobots (void)
{
// Dump robot control info for debug purposes
if (! (gameData.app.nGameMode & GM_MULTI_ROBOTS))
	return;
}

//-----------------------------------------------------------------------------
// Try to add a new robot to the controlled list, return 1 if added, 0 if not.

int MultiAddControlledRobot (int nObject, int agitation)
{
	int i;
	int lowest_agitation = 0x7fffffff; // MAX POSITIVE INT
	int lowest_agitated_bot = -1;
	int first_freeRobot = -1;

if (ROBOTINFO (OBJECTS [nObject].info.nId).bossFlag) // this is a boss, so make sure he gets a slot
	agitation= (agitation*3)+gameData.multiplayer.nLocalPlayer;  
if (OBJECTS [nObject].cType.aiInfo.REMOTE_SLOT_NUM > 0) {
	OBJECTS [nObject].cType.aiInfo.REMOTE_SLOT_NUM -= 1;
	return 0;
	}
for (i = 0; i < MAX_ROBOTS_CONTROLLED; i++) {
	if ((gameData.multigame.robots.controlled [i] == -1) || (OBJECTS [gameData.multigame.robots.controlled [i]].info.nType != OBJ_ROBOT)) {
		first_freeRobot = i;
		break;
		}
	if (gameData.multigame.robots.lastMsgTime [i] + ROBOT_TIMEOUT < gameData.time.xGame) {
		if (gameData.multigame.robots.sendPending [i])
			MultiSendRobotPosition (gameData.multigame.robots.controlled [i], 1);
		MultiSendReleaseRobot (gameData.multigame.robots.controlled [i]);
		first_freeRobot = i;
		break;
		}
	if ((gameData.multigame.robots.controlled [i] != -1) && 
		 (gameData.multigame.robots.agitation [i] < lowest_agitation) && 
		 (gameData.multigame.robots.controlledTime [i] + MIN_CONTROL_TIME < gameData.time.xGame)) {
			lowest_agitation = gameData.multigame.robots.agitation [i];
			lowest_agitated_bot = i;
		}
	}
if (first_freeRobot != -1)  // Room left for new robots
	i = first_freeRobot;
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
OBJECTS [nObject].cType.aiInfo.REMOTE_OWNER = gameData.multiplayer.nLocalPlayer;
OBJECTS [nObject].cType.aiInfo.REMOTE_SLOT_NUM = i;
gameData.multigame.robots.controlledTime [i] = gameData.time.xGame;
gameData.multigame.robots.lastSendTime [i] = gameData.multigame.robots.lastMsgTime [i] = gameData.time.xGame;
return 1;
}

//-----------------------------------------------------------------------------
// Delete robot CObject number nObject from list of controlled robots because it is dead

void MultiDeleteControlledRobot (int nObject)
{
	int i;

if ((nObject < 0) || (nObject > gameData.objs.nLastObject [0]))
	return;
for (i = 0; i < MAX_ROBOTS_CONTROLLED; i++)
	if (gameData.multigame.robots.controlled [i] == nObject) {
		if (OBJECTS [nObject].cType.aiInfo.REMOTE_SLOT_NUM != i) {
			Int3 ();  // can't release this bot!
			return;
			}
		OBJECTS [nObject].cType.aiInfo.REMOTE_OWNER = -1;
		OBJECTS [nObject].cType.aiInfo.REMOTE_SLOT_NUM = 0;
		gameData.multigame.robots.controlled [i] = -1;
		gameData.multigame.robots.sendPending [i] = 0;
		gameData.multigame.robots.fired [i] = 0;
		}
}

//-----------------------------------------------------------------------------

void MultiSendClaimRobot (int nObject)
{
	short s;

if ((nObject < 0) || (nObject > gameData.objs.nLastObject [0])) {
	Int3 (); // See rob
	return;
	}
if (OBJECTS [nObject].info.nType != OBJ_ROBOT) {
	Int3 (); // See rob
	return;
	}
// The AI tells us we should take control of this robot. 
gameData.multigame.msg.buf [0] = (char)MULTI_ROBOT_CLAIM;
gameData.multigame.msg.buf [1] = gameData.multiplayer.nLocalPlayer;
s = ObjnumLocalToRemote (nObject, reinterpret_cast<sbyte*> (&gameData.multigame.msg.buf [4]));
PUT_INTEL_SHORT (gameData.multigame.msg.buf+2, s);
MultiSendData (gameData.multigame.msg.buf, 5, 2);
MultiSendData (gameData.multigame.msg.buf, 5, 2);
MultiSendData (gameData.multigame.msg.buf, 5, 2);
}

//-----------------------------------------------------------------------------

void MultiSendReleaseRobot (int nObject)
{
	short s;

if ((nObject < 0) || (nObject > gameData.objs.nLastObject [0])) {
	Int3 (); // See rob
	return;
	}
if (OBJECTS [nObject].info.nType != OBJ_ROBOT) {
	Int3 (); // See rob
	return;
	}
MultiDeleteControlledRobot (nObject);
	gameData.multigame.msg.buf [0] = (char)MULTI_ROBOT_RELEASE;
gameData.multigame.msg.buf [1] = gameData.multiplayer.nLocalPlayer;
s = ObjnumLocalToRemote (nObject, reinterpret_cast<sbyte*> (&gameData.multigame.msg.buf [4]));
PUT_INTEL_SHORT (gameData.multigame.msg.buf+2, s);
MultiSendData (gameData.multigame.msg.buf, 5, 2);
MultiSendData (gameData.multigame.msg.buf, 5, 2);
MultiSendData (gameData.multigame.msg.buf, 5, 2);
}

//-----------------------------------------------------------------------------

int MultiSendRobotFrame (int sent)
{
	static int last_sent = 0;

	int i, sending;
	int rval = 0;

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
			MultiSendData (reinterpret_cast<char*> (gameData.multigame.robots.fireBuf [sending]), 18, 1);
			}
		if (! (gameData.app.nGameMode & GM_NETWORK))
			sent += 1;
		last_sent = sending;
		rval++;
		}
	}
Assert ((last_sent >= 0) && (last_sent <= MAX_ROBOTS_CONTROLLED));
return (rval);
}

//-----------------------------------------------------------------------------

void MultiSendRobotPositionSub (int nObject)
{
	int bufP = 0;
	short s;
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
	tShortPos sp;
#endif

gameData.multigame.msg.buf [bufP++] = MULTI_ROBOT_POSITION;  							
gameData.multigame.msg.buf [bufP++] = gameData.multiplayer.nLocalPlayer;										
s = ObjnumLocalToRemote (nObject, reinterpret_cast<sbyte*> (gameData.multigame.msg.buf + bufP + 2));
PUT_INTEL_SHORT (gameData.multigame.msg.buf + bufP, s);
bufP += 3;
#if ! (defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__))
CreateShortPos (reinterpret_cast<tShortPos*> (gameData.multigame.msg.buf + bufP), OBJECTS + nObject, 0);	
bufP += sizeof (tShortPos);
#else
CreateShortPos (&sp, OBJECTS+nObject, 1);
memcpy (gameData.multigame.msg.buf + bufP, reinterpret_cast<ubyte*> (sp.bytemat), 9);
bufP += 9;
memcpy (gameData.multigame.msg.buf + bufP, reinterpret_cast<ubyte*> (&sp.xo), 14);
bufP += 14;
#endif
MultiSendData (reinterpret_cast<char*> (gameData.multigame.msg.buf), bufP, 1);
}

//-----------------------------------------------------------------------------
// Send robot position to other CPlayerData (s).  Includes a byte
// value describing whether or not they fired a weapon

void MultiSendRobotPosition (int nObject, int force)
{
	int	i;

if (! (gameData.app.nGameMode & GM_MULTI))
	return;
if ((nObject < 0) || (nObject > gameData.objs.nLastObject [0])) {
	Int3 (); // See rob
	return;
	}
if (OBJECTS [nObject].info.nType != OBJ_ROBOT) {
	Int3 (); // See rob
	return;
	}
if (OBJECTS [nObject].cType.aiInfo.REMOTE_OWNER != gameData.multiplayer.nLocalPlayer)
	return;
i = OBJECTS [nObject].cType.aiInfo.REMOTE_SLOT_NUM;
gameData.multigame.robots.lastSendTime [i] = gameData.time.xGame;
gameData.multigame.robots.sendPending [i] = 1+force;
if (force & (gameData.app.nGameMode & GM_NETWORK))
	networkData.bPacketUrgent = 1;
return;
}

//-----------------------------------------------------------------------------

void MultiSendRobotFire (int nObject, int nGun, CFixVector *vFire)
{
	// Send robot vFire event
	int bufP = 0;
	short s;
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
	CFixVector vSwapped;
#endif

gameData.multigame.msg.buf [bufP++] = MULTI_ROBOT_FIRE;					
gameData.multigame.msg.buf [bufP++] = gameData.multiplayer.nLocalPlayer;							
s = ObjnumLocalToRemote (nObject, reinterpret_cast<sbyte*> (gameData.multigame.msg.buf + bufP + 2));
PUT_INTEL_SHORT (gameData.multigame.msg.buf + bufP, s);
bufP += 3;
gameData.multigame.msg.buf [bufP++] = nGun;								
#if ! (defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__))
memcpy (gameData.multigame.msg.buf + bufP, vFire, sizeof (CFixVector));         
bufP += sizeof (CFixVector); // 12
// --------------------------
//      Total = 18
#else
vSwapped [X] = (fix) INTEL_INT ((int) (*vFire) [X]);
vSwapped [Y] = (fix) INTEL_INT ((int) (*vFire) [Y]);
vSwapped [Z] = (fix) INTEL_INT ((int) (*vFire) [Z]);
memcpy (gameData.multigame.msg.buf + bufP, &vSwapped, sizeof (CFixVector)); 
bufP += sizeof (CFixVector);
#endif
if (OBJECTS [nObject].cType.aiInfo.REMOTE_OWNER == gameData.multiplayer.nLocalPlayer) {
	int slot = OBJECTS [nObject].cType.aiInfo.REMOTE_SLOT_NUM;
	if ((slot < 0) || (slot >= MAX_ROBOTS_CONTROLLED))
		return;
	if (gameData.multigame.robots.fired [slot] != 0)
		return;
	memcpy (gameData.multigame.robots.fireBuf [slot], gameData.multigame.msg.buf, bufP);
	gameData.multigame.robots.fired [slot] = 1;
	if (gameData.app.nGameMode & GM_NETWORK)
		networkData.bPacketUrgent = 1;
	}
else
	MultiSendData (gameData.multigame.msg.buf, bufP, 2); // Not our robot, send ASAP
}

//-----------------------------------------------------------------------------

void MultiSendRobotExplode (int nObject, int nKiller,char bIsThief)
{
	// Send robot explosion event to the other players

	int bufP = 0;
	short s;

gameData.multigame.msg.buf [bufP++] = MULTI_ROBOT_EXPLODE;				
gameData.multigame.msg.buf [bufP++] = gameData.multiplayer.nLocalPlayer;							
s = (short)ObjnumLocalToRemote (nKiller, reinterpret_cast<sbyte*> (gameData.multigame.msg.buf + bufP + 2));
PUT_INTEL_SHORT (gameData.multigame.msg.buf + bufP, s);                       
bufP += 3;
s = (short)ObjnumLocalToRemote (nObject, reinterpret_cast<sbyte*> (gameData.multigame.msg.buf + bufP + 2));
PUT_INTEL_SHORT (gameData.multigame.msg.buf + bufP, s);                       
bufP += 3;
gameData.multigame.msg.buf [bufP++] = bIsThief;   
MultiSendData (gameData.multigame.msg.buf, bufP, 1);
MultiDeleteControlledRobot (nObject);
}

//-----------------------------------------------------------------------------

void MultiSendCreateRobot (int station, int nObject, int nType)
{
	// Send create robot information

	int bufP = 0;

gameData.multigame.msg.buf [bufP++] = MULTI_CREATE_ROBOT;					
gameData.multigame.msg.buf [bufP++] = gameData.multiplayer.nLocalPlayer;							
gameData.multigame.msg.buf [bufP++] = (sbyte)station;                         
PUT_INTEL_SHORT (gameData.multigame.msg.buf + bufP, nObject);                  
bufP += 2;
gameData.multigame.msg.buf [bufP++] = nType;								
MapObjnumLocalToLocal ((short)nObject);
MultiSendData (gameData.multigame.msg.buf, bufP, 2);
}

//-----------------------------------------------------------------------------

void MultiSendBossActions (int bossobjnum, int action, int secondary, int nObject)
{
	// Send special boss behavior information

	int bufP = 0;

gameData.multigame.msg.buf [bufP++] = MULTI_BOSS_ACTIONS;					
gameData.multigame.msg.buf [bufP++] = gameData.multiplayer.nLocalPlayer;
PUT_INTEL_SHORT (gameData.multigame.msg.buf + bufP, bossobjnum);  // Which CPlayerData is controlling the boss        
bufP += 2; // We won't network map this nObject since it's the boss
gameData.multigame.msg.buf [bufP++] = (sbyte)action;  // What is the boss doing?
gameData.multigame.msg.buf [bufP++] = (sbyte)secondary;  // More info for what he is doing
PUT_INTEL_SHORT (gameData.multigame.msg.buf + bufP, nObject);                  
bufP += 2; // Objnum of CObject created by gate-in action
if (action == 3) {
	PUT_INTEL_SHORT (gameData.multigame.msg.buf + bufP, OBJECTS [nObject].info.nSegment); 
	bufP += 2; // Segment number CObject created in (for gate only)
	}
else 
	bufP += 2; // Dummy

if (action == 1) { // Teleport releases robot
	// Boss is up for grabs after teleporting
#if 0
	Assert (
		 (OBJECTS [bossobjnum].cType.aiInfo.REMOTE_SLOT_NUM >= 0) && 
		 (OBJECTS [bossobjnum].cType.aiInfo.REMOTE_SLOT_NUM < MAX_ROBOTS_CONTROLLED));
#endif
	MultiDeleteControlledRobot (bossobjnum);
//		gameData.multigame.robots.controlled [OBJECTS [bossobjnum].cType.aiInfo.REMOTE_SLOT_NUM] = -1;
//		OBJECTS [bossobjnum].cType.aiInfo.REMOTE_OWNER = -1;
//		OBJECTS [bossobjnum].cType.aiInfo.REMOTE_SLOT_NUM = 5; // Hands-off period!
	}
MultiSendData (gameData.multigame.msg.buf, bufP, 1);
}
		
//-----------------------------------------------------------------------------
// Send create robot information

void MultiSendCreateRobotPowerups (CObject *delObjP)
{
	int	bufP = 0, hBufP;
	int	i, j = 0;
	char	h, nContained;
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
	CFixVector vSwapped;
#endif

gameData.multigame.msg.buf [bufP++] = MULTI_CREATE_ROBOT_POWERUPS;			
gameData.multigame.msg.buf [bufP++] = gameData.multiplayer.nLocalPlayer;			
hBufP = bufP++;
gameData.multigame.msg.buf [bufP++] = delObjP->info.contains.nType; 				
gameData.multigame.msg.buf [bufP++] = delObjP->info.contains.nId;					
PUT_INTEL_SHORT (gameData.multigame.msg.buf + bufP, delObjP->info.nSegment);		        
bufP += 2;
#if !(defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__))
memcpy (gameData.multigame.msg.buf + bufP, &delObjP->info.position.vPos, sizeof (CFixVector));
#else
vSwapped[X] = (fix)INTEL_INT ((int) delObjP->info.position.vPos[X]);
vSwapped[Y] = (fix)INTEL_INT ((int) delObjP->info.position.vPos[Y]);
vSwapped[Z] = (fix)INTEL_INT ((int) delObjP->info.position.vPos[Z]);
memcpy (gameData.multigame.msg.buf + bufP, &vSwapped, sizeof (CFixVector));     
#endif
bufP += 12;
gameData.multigame.create.nLoc = 0;
for (nContained = delObjP->info.contains.nCount; nContained; nContained -= h) {
	h = (nContained > MAX_ROBOT_POWERUPS) ? MAX_ROBOT_POWERUPS : nContained;
	gameData.multigame.msg.buf [hBufP] = h;
	memset (gameData.multigame.msg.buf + bufP, -1, MAX_ROBOT_POWERUPS * sizeof (short));
	for (i = 0; i < h; i++, j++) {
		PUT_INTEL_SHORT (gameData.multigame.msg.buf + bufP + 2 * i, gameData.multigame.create.nObjNums [j]);
		MapObjnumLocalToLocal (gameData.multigame.create.nObjNums [j]);
		}
	MultiSendData (gameData.multigame.msg.buf, 27, 2);
	}
}

//-----------------------------------------------------------------------------

void MultiDoClaimRobot (char *buf)
{
	short nRobot, nRemoteBot;
	char nPlayer = buf [1];

nRemoteBot = GET_INTEL_SHORT (buf + 2);
nRobot = ObjnumRemoteToLocal (nRemoteBot, (sbyte)buf [4]);
if ((nRobot > gameData.objs.nLastObject [0]) || (nRobot < 0))
//		Int3 (); // See rob
	return;
if (OBJECTS [nRobot].info.nType != OBJ_ROBOT)
	return;
if (OBJECTS [nRobot].cType.aiInfo.REMOTE_OWNER != -1)
	if (MULTI_ROBOT_PRIORITY (nRemoteBot, nPlayer) <= MULTI_ROBOT_PRIORITY (nRemoteBot, OBJECTS [nRobot].cType.aiInfo.REMOTE_OWNER))
		return;
// Perform the requested change
if (OBJECTS [nRobot].cType.aiInfo.REMOTE_OWNER == gameData.multiplayer.nLocalPlayer)
	MultiDeleteControlledRobot (nRobot);
OBJECTS [nRobot].cType.aiInfo.REMOTE_OWNER = nPlayer;
OBJECTS [nRobot].cType.aiInfo.REMOTE_SLOT_NUM = 0;
}

//-----------------------------------------------------------------------------

void MultiDoReleaseRobot (char *buf)
{
	short nRobot, nRemoteBot;
	char nPlayer = buf [1];

nRemoteBot = GET_INTEL_SHORT (buf + 2);
nRobot = ObjnumRemoteToLocal (nRemoteBot, (sbyte)buf [4]);
if ((nRobot < 0) || (nRobot > gameData.objs.nLastObject [0]))
	return;
if (OBJECTS [nRobot].info.nType != OBJ_ROBOT)
	return;
if (OBJECTS [nRobot].cType.aiInfo.REMOTE_OWNER != nPlayer)
	return;
// Perform the requested change
OBJECTS [nRobot].cType.aiInfo.REMOTE_OWNER = -1;
OBJECTS [nRobot].cType.aiInfo.REMOTE_SLOT_NUM = 0;
}

//-----------------------------------------------------------------------------
// Process robot movement sent by another CPlayerData

void MultiDoRobotPosition (char *buf)
{
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
	tShortPos sp;
#endif
	short nRobot, nRemoteBot;
	int bufP = 1;
	char nPlayer = buf [bufP++];

nRemoteBot = GET_INTEL_SHORT (buf + bufP);
nRobot = ObjnumRemoteToLocal (nRemoteBot, (sbyte)buf [bufP+2]); 
bufP += 3;
if ((nRobot < 0) || (nRobot > gameData.objs.nLastObject [0]))
	return;
if ((OBJECTS [nRobot].info.nType != OBJ_ROBOT) || 
	 (OBJECTS [nRobot].info.nFlags & OF_EXPLODING)) 
	return;
if (OBJECTS [nRobot].cType.aiInfo.REMOTE_OWNER != nPlayer) {
	if (OBJECTS [nRobot].cType.aiInfo.REMOTE_OWNER != -1)
		return;
		// Robot claim packet must have gotten lost, let this CPlayerData claim it.
	else if (OBJECTS [nRobot].cType.aiInfo.REMOTE_SLOT_NUM > 3) {
		OBJECTS [nRobot].cType.aiInfo.REMOTE_OWNER = nPlayer;
		OBJECTS [nRobot].cType.aiInfo.REMOTE_SLOT_NUM = 0;
		}
	else
		OBJECTS [nRobot].cType.aiInfo.REMOTE_SLOT_NUM++;
	}
OBJECTS [nRobot].SetThrustFromVelocity (); // Try to smooth out movement
#if ! (defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__))
ExtractShortPos (&OBJECTS [nRobot], reinterpret_cast<tShortPos*> (buf+bufP), 0);
#else
memcpy (reinterpret_cast<ubyte*> ((sp.bytemat), reinterpret_cast<ubyte*> (buf + bufP), 9);	
bufP += 9;
memcpy (reinterpret_cast<ubyte*> (& (sp.xo), reinterpret_cast<ubyte*> (buf + bufP), 14);
ExtractShortPos (&OBJECTS [nRobot], &sp, 1);
#endif
}

//-----------------------------------------------------------------------------

void MultiDoRobotFire (char *buf)
{
	// Send robot vFire event
	int			bufP = 1;
	short			nRobot;
	short			nRemoteBot;
	int			nPlayer, nGun;
	CFixVector	vFire, vGunPoint;
	tRobotInfo	*robotP;

nPlayer = buf [bufP++];											
nRemoteBot = GET_INTEL_SHORT (buf + bufP);
nRobot = ObjnumRemoteToLocal (nRemoteBot, (sbyte)buf [bufP+2]); 
bufP += 3;
nGun = (sbyte)buf [bufP++];                                      
memcpy (&vFire, buf+bufP, sizeof (CFixVector));
vFire [X] = (fix)INTEL_INT ((int)vFire [X]);
vFire [Y] = (fix)INTEL_INT ((int)vFire [Y]);
vFire [Z] = (fix)INTEL_INT ((int)vFire [Z]);
if ((nRobot < 0) || (nRobot > gameData.objs.nLastObject [0]) || 
		 (OBJECTS [nRobot].info.nType != OBJ_ROBOT) || 
		 (OBJECTS [nRobot].info.nFlags & OF_EXPLODING))
	return;
// Do the firing
if (nGun == -1 || nGun==-2)
	// Drop proximity bombs
	vGunPoint = OBJECTS [nRobot].info.position.vPos + vFire;
else 
	CalcGunPoint (&vGunPoint, &OBJECTS [nRobot], nGun);
robotP = &ROBOTINFO (OBJECTS [nRobot].info.nId);
if (nGun == -1) 
	CreateNewLaserEasy (&vFire, &vGunPoint, nRobot, PROXMINE_ID, 1);
else if (nGun == -2)
	CreateNewLaserEasy (&vFire, &vGunPoint, nRobot, SMARTMINE_ID, 1);
else
	CreateNewLaserEasy (&vFire, &vGunPoint, nRobot, robotP->nWeaponType, 1);
}

//-----------------------------------------------------------------------------

extern void DropStolenItems (CObject*);

int MultiExplodeRobotSub (int nRobot, int nKiller,char bIsThief)
{
	CObject *robotP;

if ((nRobot < 0) || (nRobot > gameData.objs.nLastObject [0])) { // Objnum in range?
	Int3 (); // See rob
	return 0;
	}
if (OBJECTS [nRobot].info.nType != OBJ_ROBOT) // Object is robotP?
	return 0;
	if (OBJECTS [nRobot].info.nFlags & OF_EXPLODING) // Object not already exploding
		return 0;
// Data seems valid, explode the sucker
NetworkResetObjSync (nRobot);
robotP = OBJECTS + nRobot;
// Drop non-Random KEY powerups locally only!
if ((robotP->info.contains.nCount > 0) && 
	 (robotP->info.contains.nType == OBJ_POWERUP) && 
	 (gameData.app.nGameMode & GM_MULTI_COOP) && 
	 (robotP->info.contains.nId >= POW_KEY_BLUE) && 
	 (robotP->info.contains.nId <= POW_KEY_GOLD))
	ObjectCreateEgg (robotP);
else if (robotP->cType.aiInfo.REMOTE_OWNER == gameData.multiplayer.nLocalPlayer) {
	MultiDropRobotPowerups (OBJ_IDX (robotP));
	MultiDeleteControlledRobot (OBJ_IDX (robotP));
	}
else if (robotP->cType.aiInfo.REMOTE_OWNER == -1 && NetworkIAmMaster ()) {
	MultiDropRobotPowerups (OBJ_IDX (robotP));
	//MultiDeleteControlledRobot (OBJ_IDX (robotP));
	}
if (bIsThief || ROBOTINFO (robotP->info.nId).thief)
	DropStolenItems (robotP);
if (ROBOTINFO (robotP->info.nId).bossFlag) {
	int i = FindBoss (nRobot);
	if ((i >= 0) && gameData.boss [i].nDying)
		return 0;
	StartBossDeathSequence (robotP);
	}
else if (ROBOTINFO (robotP->info.nId).bDeathRoll)
	StartRobotDeathSequence (robotP);
else {
	if (robotP->info.nId == SPECIAL_REACTOR_ROBOT)
		SpecialReactorStuff ();
	if (ROBOTINFO (robotP->info.nId).kamikaze)
		robotP->Explode (1);	//	Kamikaze, explode right away, IN YOUR FACE!
	else
		robotP->Explode (STANDARD_EXPL_DELAY);
   }
return 1;
}

//-----------------------------------------------------------------------------

void MultiDoRobotExplode (char *buf)
{
	// Explode robot controlled by other CPlayerData

	int	nPlayer, nRobot, rval, bufP = 1;
	short nRemoteBot, nKiller, nRemoteKiller;
	char	thief;

nPlayer = buf [bufP++]; 
nRemoteKiller = GET_INTEL_SHORT (buf + bufP);
nKiller = ObjnumRemoteToLocal (nRemoteKiller, (sbyte)buf [bufP+2]); 
bufP += 3;
nRemoteBot = GET_INTEL_SHORT (buf + bufP);
nRobot = ObjnumRemoteToLocal (nRemoteBot, (sbyte)buf [bufP+2]); 
bufP += 3;
thief = buf [bufP];
if ((nRobot < 0) || (nRobot > gameData.objs.nLastObject [0]))
	return;
rval = MultiExplodeRobotSub (nRobot, nKiller,thief);
if (rval && (nKiller == LOCALPLAYER.nObject))
	AddPointsToScore (ROBOTINFO (OBJECTS [nRobot].info.nId).scoreValue);
}

//-----------------------------------------------------------------------------

void MultiDoCreateRobot (char *buf)
{

	int		nFuelCen = buf [2];
	int		nPlayer = buf [1];
	short		nObject;
	ubyte		nType = buf [5];

	tFuelCenInfo *robotcen;
	CFixVector vObjPos, direction;
	CObject *objP;

nObject = GET_INTEL_SHORT (buf + 3);
if ((nPlayer < 0) || (nObject < 0) || (nFuelCen < 0) || 
	 (nFuelCen >= gameData.matCens.nFuelCenters) || (nPlayer >= gameData.multiplayer.nPlayers)) {
	Int3 (); // Bogus data
	return;
	}
robotcen = gameData.matCens.fuelCenters + nFuelCen;
// Play effect and sound
vObjPos = SEGMENTS [robotcen->nSegment].Center ();
objP = /*Object*/CreateExplosion ((short) robotcen->nSegment, vObjPos, I2X (10), VCLIP_MORPHING_ROBOT);
if (objP)
	ExtractOrientFromSegment (&objP->info.position.mOrient, &SEGMENTS [robotcen->nSegment]);
if (gameData.eff.vClips [0][VCLIP_MORPHING_ROBOT].nSound > -1)
	audio.CreateSegmentSound (gameData.eff.vClips [0][VCLIP_MORPHING_ROBOT].nSound, (short) robotcen->nSegment, 0, vObjPos, 0, I2X (1));
// Set robot center flags, in case we become the master for the next one
robotcen->bFlag = 0;
robotcen->xCapacity -= gameData.matCens.xEnergyToCreateOneRobot;
robotcen->xTimer = 0;
if (! (objP = CreateMorphRobot (SEGMENTS + robotcen->nSegment, &vObjPos, nType)))
	return; // Cannot create CObject!
objP->info.nCreator = ((short) (robotcen - gameData.matCens.fuelCenters)) | 0x80;
//	ExtractOrientFromSegment (&objP->info.position.mOrient, &SEGMENTS [robotcen->nSegment]);
direction = gameData.objs.consoleP->info.position.vPos - objP->info.position.vPos;
objP->info.position.mOrient = CFixMatrix::CreateFU(direction, objP->info.position.mOrient.UVec ());
//objP->info.position.mOrient = CFixMatrix::CreateFU(direction, &objP->info.position.mOrient.UVec (), NULL);
objP->MorphStart ();
MapObjnumLocalToRemote (objP->Index (), nObject, nPlayer);
Assert (objP->cType.aiInfo.REMOTE_OWNER == -1);
}

//-----------------------------------------------------------------------------

void MultiDoBossActions (char *buf)
{
	// Code to handle remote-controlled boss actions

	CObject	*bossObjP;
	short		nBossObj, nBossIdx;
	int		nPlayer;
	int		action;
	short		secondary;
	short		nRemoteObj, nSegment;
	int		bufP = 1;

nPlayer = buf [bufP++];
nBossObj = GET_INTEL_SHORT (buf + bufP);           
bufP += 2;
action = buf [bufP++];
secondary = buf [bufP++];
nRemoteObj = GET_INTEL_SHORT (buf + bufP);         
bufP += 2;
nSegment = GET_INTEL_SHORT (buf + bufP);                
bufP += 2;
if ((nBossObj < 0) || (nBossObj > gameData.objs.nLastObject [0])) {
	Int3 ();  // See Rob
	return;
	}
nBossIdx = FindBoss (nBossObj);
if (nBossIdx < 0)
	return;
bossObjP = OBJECTS + nBossObj;
if ((bossObjP->info.nType != OBJ_ROBOT) || !(ROBOTINFO (bossObjP->info.nId).bossFlag)) {
	Int3 (); // Got boss actions for a robot who's not a boss?
	return;
	}
switch (action)  {
	case 1: // Teleport
	 {
		short nTeleportSeg;

		CFixVector vBossDir;
		if ((secondary < 0) || (secondary > gameData.boss [nBossIdx].nTeleportSegs)) {
			Int3 (); // Bad nSegment for boss teleport, ROB!!
			return;
			}
		nTeleportSeg = gameData.boss [nBossIdx].teleportSegs[secondary];
		if ((nTeleportSeg < 0) || (nTeleportSeg > gameData.segs.nLastSegment)) {
			Int3 ();  // See Rob
			return;
			}
		bossObjP->info.position.vPos = SEGMENTS [nTeleportSeg].Center ();
		OBJECTS [nBossObj].RelinkToSeg (nTeleportSeg);
		gameData.boss [nBossIdx].nLastTeleportTime = gameData.time.xGame;
		vBossDir = OBJECTS [gameData.multiplayer.players [nPlayer].nObject].info.position.vPos - bossObjP->info.position.vPos;
/*
		bossObjP->info.position.mOrient = CFixMatrix::Create(vBossDir, NULL, NULL);
*/
		// TODO: MatrixCreateFCheck
		bossObjP->info.position.mOrient = CFixMatrix::CreateF(vBossDir);

		audio.CreateSegmentSound (gameData.eff.vClips [0][VCLIP_MORPHING_ROBOT].nSound, nTeleportSeg, 0, bossObjP->info.position.vPos, 0, I2X (1));
		audio.DestroyObjectSound (OBJ_IDX (bossObjP));
		audio.CreateObjectSound (SOUND_BOSS_SHARE_SEE, SOUNDCLASS_ROBOT, OBJ_IDX (bossObjP), 1, I2X (1), I2X (512));	//	I2X (5)12 means play twice as loud
		gameData.ai.localInfo [OBJ_IDX (bossObjP)].nextPrimaryFire = 0;
		if (bossObjP->cType.aiInfo.REMOTE_OWNER == gameData.multiplayer.nLocalPlayer) {
			MultiDeleteControlledRobot (nBossObj);
//			gameData.multigame.robots.controlled [bossObjP->cType.aiInfo.REMOTE_SLOT_NUM] = -1;
			}
		bossObjP->cType.aiInfo.REMOTE_OWNER = -1; // Boss is up for grabs again!
		bossObjP->cType.aiInfo.REMOTE_SLOT_NUM = 0; // Available immediately!
		}
		break;

	case 2: // Cloak
		gameData.boss [nBossIdx].nHitTime = -I2X (10);
		gameData.boss [nBossIdx].nCloakStartTime = gameData.time.xGame;
		gameData.boss [nBossIdx].nCloakEndTime = gameData.time.xGame + gameData.boss [nBossIdx].nCloakDuration;
		bossObjP->cType.aiInfo.CLOAKED = 1;
		break;

	case 3: // Gate in robots!
		// Do some validity checking
		if ((nRemoteObj >= LEVEL_OBJECTS) || (nRemoteObj < 0) || (nSegment < 0) || 
				(nSegment > gameData.segs.nLastSegment)) {
			Int3 (); // See Rob, bad data in boss gate action message
			return;
			}
		// Gate one in!
		if (GateInRobot (nBossObj, (ubyte) secondary, nSegment))
			MapObjnumLocalToRemote (gameData.multigame.create.nObjNums [0], nRemoteObj, nPlayer);
		break;

	case 4: // Start effect
		RestartEffect (BOSS_ECLIP_NUM);
		break;

	case 5:	// Stop effect
		StopEffect (BOSS_ECLIP_NUM);
		break;

	default:
		Int3 (); // Illegal nType to boss actions
	}
}

//-----------------------------------------------------------------------------
// Code to drop remote-controlled robot powerups

void MultiDoCreateRobotPowerups (char *buf)
{
	CObject	delObjP;
	int		nPlayer, nEggObj, i, bufP = 1;
	short		s;

nPlayer = buf [bufP++];									
delObjP.info.contains.nCount = buf [bufP++];					
delObjP.info.contains.nType = buf [bufP++];					
delObjP.info.contains.nId = buf [bufP++]; 					
delObjP.info.nSegment = GET_INTEL_SHORT (buf + bufP);            
bufP += 2;
memcpy (&delObjP.info.position.vPos, buf+bufP, sizeof (CFixVector));      
bufP += 12;
delObjP.mType.physInfo.velocity.SetZero();
delObjP.info.position.vPos [X] = (fix) INTEL_INT ((int) delObjP.info.position.vPos [X]);
delObjP.info.position.vPos [Y] = (fix) INTEL_INT ((int) delObjP.info.position.vPos [Y]);
delObjP.info.position.vPos [Z] = (fix) INTEL_INT ((int) delObjP.info.position.vPos [Z]);
Assert ((nPlayer >= 0) && (nPlayer < gameData.multiplayer.nPlayers));
Assert (nPlayer != gameData.multiplayer.nLocalPlayer); // What? How'd we send ourselves this?
gameData.multigame.create.nLoc = 0;
nEggObj = ObjectCreateEgg (&delObjP);
if (nEggObj == -1)
	return; // Object buffer full
//	Assert (nEggObj > -1);
Assert ((gameData.multigame.create.nLoc > 0) && (gameData.multigame.create.nLoc <= MAX_ROBOT_POWERUPS));
for (i = 0; i < gameData.multigame.create.nLoc; i++) {
	s = GET_INTEL_SHORT (buf + bufP);
	if (s != -1)
		MapObjnumLocalToRemote ((short)gameData.multigame.create.nObjNums [i], s, nPlayer);
	else
		OBJECTS [gameData.multigame.create.nObjNums [i]].Die (); // Delete OBJECTS other guy didn't create one of
	bufP += 2;
	}
}

//-----------------------------------------------------------------------------
// Code to handle dropped robot powerups in network mode ONLY!

void MultiDropRobotPowerups (int nObject)
{
	CObject		*delObjP;
	int			nEggObj = -1;
	tRobotInfo	*robotP; 

if ((nObject < 0) || (nObject > gameData.objs.nLastObject [0])) {
	Int3 ();  // See rob
	return;
	}
delObjP = &OBJECTS [nObject];
if (delObjP->info.nType != OBJ_ROBOT) {
	Int3 (); // dropping powerups for non-robot, Rob's fault
	return;
	}
robotP = &ROBOTINFO (delObjP->info.nId);
gameData.multigame.create.nLoc = 0;
if (delObjP->info.contains.nCount > 0) { 
	//	If dropping a weapon that the CPlayerData has, drop energy instead, unless it's vulcan, in which case drop vulcan ammo.
	if (delObjP->info.contains.nType == OBJ_POWERUP) {
		MaybeReplacePowerupWithEnergy (delObjP);
		if (!MultiPowerupIsAllowed (delObjP->info.contains.nId))
			delObjP->info.contains.nId=POW_SHIELD_BOOST;
		// No key drops in non-coop games!
		if (! (gameData.app.nGameMode & GM_MULTI_COOP)) {
			if ((delObjP->info.contains.nId >= POW_KEY_BLUE) && (delObjP->info.contains.nId <= POW_KEY_GOLD))
				delObjP->info.contains.nCount = 0;
			}
		}
	if (delObjP->info.contains.nCount > 0)
		nEggObj = ObjectCreateEgg (delObjP);
	}
else if (delObjP->cType.aiInfo.REMOTE_OWNER == -1) // No Random goodies for robots we weren't in control of
	return;
else if (robotP->containsCount) {
	if (((d_rand () * 16) >> 15) < robotP->containsProb) {
		delObjP->info.contains.nCount = ((d_rand () * robotP->containsCount) >> 15) + 1;
		delObjP->info.contains.nType = robotP->containsType;
		delObjP->info.contains.nId = robotP->containsId;
		if (delObjP->info.contains.nType == OBJ_POWERUP) {
			MaybeReplacePowerupWithEnergy (delObjP);
			if (!MultiPowerupIsAllowed (delObjP->info.contains.nId))
				delObjP->info.contains.nId=POW_SHIELD_BOOST;
			 }
		if (delObjP->info.contains.nCount > 0)
			nEggObj = ObjectCreateEgg (delObjP);
		}
	}
if (nEggObj >= 0) // Transmit the CObject creation to the other players	 
	MultiSendCreateRobotPowerups (delObjP);
}

//	-----------------------------------------------------------------------------
//	Robot *robot got whacked by CPlayerData player_num and requests permission to do something about it.
//	Note: This function will be called regardless of whether gameData.app.nGameMode is a multiplayer mode, so it
//	should quick-out if not in a multiplayer mode.  On the other hand, it only gets called when a
//	CPlayerData or CPlayerData weapon whacks a robot, so it happens rarely.
void MultiRobotRequestChange (CObject *robot, int player_num)
{
	int	slot, nRemoteObj;
	sbyte dummy;

if (!(gameData.app.nGameMode & GM_MULTI_ROBOTS))
	return;
slot = robot->cType.aiInfo.REMOTE_SLOT_NUM;
if ((slot < 0) || (slot >= MAX_ROBOTS_CONTROLLED)) {
	Int3 ();
	return;
	}
nRemoteObj = ObjnumLocalToRemote (OBJ_IDX (robot), &dummy);
if (nRemoteObj < 0)
	return;
if ((gameData.multigame.robots.agitation [slot] < 70) || 
	 (MULTI_ROBOT_PRIORITY (nRemoteObj, player_num) > 
	  MULTI_ROBOT_PRIORITY (nRemoteObj, gameData.multiplayer.nLocalPlayer)) || 
	  (d_rand () > 0x4400)) {
	if (gameData.multigame.robots.sendPending [slot])
		MultiSendRobotPosition (gameData.multigame.robots.controlled [slot], -1);
	MultiSendReleaseRobot (gameData.multigame.robots.controlled [slot]);
	robot->cType.aiInfo.REMOTE_SLOT_NUM = 5;  // Hands-off period
	}
}

//-----------------------------------------------------------------------------
//eof
