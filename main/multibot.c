/* $Id: multibot.c,v 1.6 2004/04/22 21:07:32 btb Exp $ */
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
 * Multiplayer robot code
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "vecmat.h"
#include "inferno.h"
#include "multibot.h"
#include "game.h"
#include "modem.h"
#include "network.h"
#include "multi.h"
#include "laser.h"
#include "error.h"
#include "mono.h"
#include "timer.h"
#include "text.h"
#include "ai.h"
#include "fireball.h"
#include "aistruct.h"
#include "robot.h"
#include "fuelcen.h"
#include "powerup.h"
#include "scores.h"
#include "gauges.h"
#include "fuelcen.h"
#include "morph.h"
#include "digi.h"
#include "sounds.h"
#include "effects.h"
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

#define STANDARD_EXPL_DELAY	(F1_0/4)
#define MIN_CONTROL_TIME		(F1_0*1)
#define ROBOT_TIMEOUT			(F1_0*2)

#define MIN_TO_ADD				60
#define MAX_TO_DELETE			61

#define MAX_ROBOT_POWERUPS		4

#define MIN_ROBOT_COM_GAP		(F1_0/12)

#define MULTI_ROBOT_PRIORITY(nObject, nPlayer) (((nObject % 4) + nPlayer) % gameData.multi.nPlayers)

extern void MultiSendStolenItems ();
extern int MultiPowerupIsAllowed (int);

//-----------------------------------------------------------------------------
// Determine whether or not I am allowed to move this robot.

int MultiCanRemoveRobot (int nObject, int agitation)
{
	int rval, nRemOwner = gameData.objs.objects [nObject].cType.aiInfo.REMOTE_OWNER;

	// Claim robot if necessary.
if (gameStates.app.bPlayerExploded)
	return 0;
#ifdef _DEBUG
if ((nObject < 0) || (nObject > gameData.objs.nLastObject)) {	
	Int3 ();
	rval = 0;
	}
else if (gameData.objs.objects [nObject].nType != OBJ_ROBOT) {
	Int3 ();
	rval = 0;
	}
#endif
else if ((gameData.bots.pInfo [gameData.objs.objects [nObject].id].bossFlag) && (gameData.boss.nDying == 1))
	return 0;
else if (nRemOwner == gameData.multi.nLocalPlayer) { // Already my robot!
	int slot_num = gameData.objs.objects [nObject].cType.aiInfo.REMOTE_SLOT_NUM;
   if ((slot_num < 0) || (slot_num >= MAX_ROBOTS_CONTROLLED))
		return 0;
	if (multiData.robots.fired [slot_num])
		rval = 0;
	else {
		multiData.robots.agitation [slot_num] = agitation;
		multiData.robots.lastMsgTime [slot_num] = gameData.time.xGame;
		rval = 1;
		}
	}
else if ((nRemOwner != -1) || (agitation < MIN_TO_ADD)) {
		if (agitation == ROBOT_FIRE_AGITATION) // Special case for firing at non-player
			rval = 1; // Try to vFire at player even tho we're not in control!
		else
			rval = 0;
	}
else
	rval = MultiAddControlledRobot (nObject, agitation);
return rval;
}

//-----------------------------------------------------------------------------

void MultiCheckRobotTimeout (void)
{
	static fix lastcheck = 0;
	int i, nRemOwner;

if (gameData.time.xGame > lastcheck + F1_0) {
	lastcheck = gameData.time.xGame;
	for (i = 0; i < MAX_ROBOTS_CONTROLLED; i++) {
		if ((multiData.robots.controlled [i] != -1) && 
			 (multiData.robots.lastSendTime [i] + ROBOT_TIMEOUT < gameData.time.xGame)) {
			nRemOwner = gameData.objs.objects [multiData.robots.controlled [i]].cType.aiInfo.REMOTE_OWNER;
			if (nRemOwner != gameData.multi.nLocalPlayer) {		
				multiData.robots.controlled [i] = -1;
				Int3 (); // Non-terminal but Rob is interesting, step over please...
				return;
				}
 			if (nRemOwner !=gameData.multi.nLocalPlayer)
				return;
			if (multiData.robots.sendPending [i])
				MultiSendRobotPosition (multiData.robots.controlled [i], 1);
			MultiSendReleaseRobot (multiData.robots.controlled [i]);
			}
		}
	}			
}

//-----------------------------------------------------------------------------

void MultiStripRobots (int playernum)
{
	// Grab all robots away from a player 
	// (player died or exited the game)

	int i;

if (gameData.app.nGameMode & GM_MULTI_ROBOTS) {
	if (playernum == gameData.multi.nLocalPlayer)
		for (i = 0; i < MAX_ROBOTS_CONTROLLED; i++)
			MultiDeleteControlledRobot (multiData.robots.controlled [i]);
	for (i = 1; i <= gameData.objs.nLastObject; i++)
		if ((gameData.objs.objects [i].nType == OBJ_ROBOT) && 
			 (gameData.objs.objects [i].cType.aiInfo.REMOTE_OWNER == playernum)) {
			Assert ((gameData.objs.objects [i].controlType == CT_AI) || 
				    (gameData.objs.objects [i].controlType == CT_NONE) || 
					 (gameData.objs.objects [i].controlType == CT_MORPH));
			gameData.objs.objects [i].cType.aiInfo.REMOTE_OWNER = -1;
			if (playernum == gameData.multi.nLocalPlayer)
				gameData.objs.objects [i].cType.aiInfo.REMOTE_SLOT_NUM = 4;
			else
				gameData.objs.objects [i].cType.aiInfo.REMOTE_SLOT_NUM = 0;
	  		}
	}
// Note -- only call this with playernum == gameData.multi.nLocalPlayer if all other players
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

if (gameData.bots.pInfo [gameData.objs.objects [nObject].id].bossFlag) // this is a boss, so make sure he gets a slot
	agitation= (agitation*3)+gameData.multi.nLocalPlayer;  
if (gameData.objs.objects [nObject].cType.aiInfo.REMOTE_SLOT_NUM > 0) {
	gameData.objs.objects [nObject].cType.aiInfo.REMOTE_SLOT_NUM -= 1;
	return 0;
	}
for (i = 0; i < MAX_ROBOTS_CONTROLLED; i++) {
	if ((multiData.robots.controlled [i] == -1) || (gameData.objs.objects [multiData.robots.controlled [i]].nType != OBJ_ROBOT)) {
		first_freeRobot = i;
		break;
		}
	if (multiData.robots.lastMsgTime [i] + ROBOT_TIMEOUT < gameData.time.xGame) {
		if (multiData.robots.sendPending [i])
			MultiSendRobotPosition (multiData.robots.controlled [i], 1);
		MultiSendReleaseRobot (multiData.robots.controlled [i]);
		first_freeRobot = i;
		break;
		}
	if ((multiData.robots.controlled [i] != -1) && 
		 (multiData.robots.agitation [i] < lowest_agitation) && 
		 (multiData.robots.controlledTime [i] + MIN_CONTROL_TIME < gameData.time.xGame)) {
			lowest_agitation = multiData.robots.agitation [i];
			lowest_agitated_bot = i;
		}
	}
if (first_freeRobot != -1)  // Room left for new robots
	i = first_freeRobot;
else if ((agitation > lowest_agitation)) {// && (lowest_agitation <= MAX_TO_DELETE)) // Replace some old robot with a more agitated one
	if (multiData.robots.sendPending [lowest_agitated_bot])
		MultiSendRobotPosition (multiData.robots.controlled [lowest_agitated_bot], 1);
	MultiSendReleaseRobot (multiData.robots.controlled [lowest_agitated_bot]);
	i = lowest_agitated_bot;
	}
else
	return 0; // Sorry, can't squeeze him in!
MultiSendClaimRobot (nObject);
multiData.robots.controlled [i] = nObject;
multiData.robots.agitation [i] = agitation;
gameData.objs.objects [nObject].cType.aiInfo.REMOTE_OWNER = gameData.multi.nLocalPlayer;
gameData.objs.objects [nObject].cType.aiInfo.REMOTE_SLOT_NUM = i;
multiData.robots.controlledTime [i] = gameData.time.xGame;
multiData.robots.lastSendTime [i] = multiData.robots.lastMsgTime [i] = gameData.time.xGame;
return 1;
}	

//-----------------------------------------------------------------------------
// Delete robot tObject number nObject from list of controlled robots because it is dead

void MultiDeleteControlledRobot (int nObject)
{
	int i;

if ((nObject < 0) || (nObject > gameData.objs.nLastObject))
	return;
for (i = 0; i < MAX_ROBOTS_CONTROLLED; i++)
	if (multiData.robots.controlled [i] == nObject) {
		if (gameData.objs.objects [nObject].cType.aiInfo.REMOTE_SLOT_NUM != i) {
			Int3 ();  // can't release this bot!
			return;
			}
		gameData.objs.objects [nObject].cType.aiInfo.REMOTE_OWNER = -1;
		gameData.objs.objects [nObject].cType.aiInfo.REMOTE_SLOT_NUM = 0;
		multiData.robots.controlled [i] = -1;
		multiData.robots.sendPending [i] = 0;
		multiData.robots.fired [i] = 0;
		}
}

//-----------------------------------------------------------------------------

void MultiSendClaimRobot (int nObject)
{
	short s;
	
if ((nObject < 0) || (nObject > gameData.objs.nLastObject)) {
	Int3 (); // See rob
	return;
	}
if (gameData.objs.objects [nObject].nType != OBJ_ROBOT) {
	Int3 (); // See rob
	return;
	}
// The AI tells us we should take control of this robot. 
multiData.msg.buf [0] = (char)MULTI_ROBOT_CLAIM;
multiData.msg.buf [1] = gameData.multi.nLocalPlayer;
s = ObjnumLocalToRemote (nObject, (sbyte *)&multiData.msg.buf [4]);
PUT_INTEL_SHORT (multiData.msg.buf+2, s);
MultiSendData (multiData.msg.buf, 5, 2);
MultiSendData (multiData.msg.buf, 5, 2);
MultiSendData (multiData.msg.buf, 5, 2);
}

//-----------------------------------------------------------------------------

void MultiSendReleaseRobot (int nObject)
{
	short s;
	
if ((nObject < 0) || (nObject > gameData.objs.nLastObject)) {
	Int3 (); // See rob
	return;
	}
if (gameData.objs.objects [nObject].nType != OBJ_ROBOT) {
	Int3 (); // See rob
	return;
	}
MultiDeleteControlledRobot (nObject);
	multiData.msg.buf [0] = (char)MULTI_ROBOT_RELEASE;
multiData.msg.buf [1] = gameData.multi.nLocalPlayer;
s = ObjnumLocalToRemote (nObject, (sbyte *)&multiData.msg.buf [4]);
PUT_INTEL_SHORT (multiData.msg.buf+2, s);
MultiSendData (multiData.msg.buf, 5, 2);
MultiSendData (multiData.msg.buf, 5, 2);
MultiSendData (multiData.msg.buf, 5, 2);
}

//-----------------------------------------------------------------------------

int MultiSendRobotFrame (int sent)
{
	static int last_sent = 0;

	int i, sending;
	int rval = 0;

for (i = 0; i < MAX_ROBOTS_CONTROLLED; i++) {
	sending = (last_sent + 1 + i) % MAX_ROBOTS_CONTROLLED;
	if ((multiData.robots.controlled [sending] != -1) && 
		 ((multiData.robots.sendPending [sending] > sent) || (multiData.robots.fired [sending] > sent))) {
		if (multiData.robots.sendPending [sending]) {
			multiData.robots.sendPending [sending] = 0;	
			MultiSendRobotPositionSub (multiData.robots.controlled [sending]);
			}
		if (multiData.robots.fired [sending]) {
			multiData.robots.fired [sending] = 0;
			MultiSendData (multiData.robots.fireBuf [sending], 18, 1);
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
	shortpos sp;
#endif

multiData.msg.buf [bufP++] = MULTI_ROBOT_POSITION;  								
multiData.msg.buf [bufP++] = gameData.multi.nLocalPlayer;											
s = ObjnumLocalToRemote (nObject, (sbyte *) (multiData.msg.buf + bufP + 2));
PUT_INTEL_SHORT (multiData.msg.buf + bufP, s);
bufP += 3;
#if ! (defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__))
CreateShortPos ((shortpos *) (multiData.msg.buf + bufP), gameData.objs.objects + nObject, 0);		
bufP += sizeof (shortpos);
#else
CreateShortPos (&sp, gameData.objs.objects+nObject, 1);
memcpy (multiData.msg.buf + bufP, (ubyte *) (sp.bytemat), 9);
bufP += 9;
memcpy (multiData.msg.buf + bufP, (ubyte *)& (sp.xo), 14);
bufP += 14;
#endif
MultiSendData ((char *) multiData.msg.buf, bufP, 1);
}

//-----------------------------------------------------------------------------
// Send robot position to other player (s).  Includes a byte
// value describing whether or not they fired a weapon

void MultiSendRobotPosition (int nObject, int force)
{
	int	i;

if (! (gameData.app.nGameMode & GM_MULTI))
	return;
if ((nObject < 0) || (nObject > gameData.objs.nLastObject)) {
	Int3 (); // See rob
	return;
	}
if (gameData.objs.objects [nObject].nType != OBJ_ROBOT) {
	Int3 (); // See rob
	return;
	}
if (gameData.objs.objects [nObject].cType.aiInfo.REMOTE_OWNER != gameData.multi.nLocalPlayer)
	return;
//	gameData.objs.objects [nObject].physInfo.drag = gameData.bots.pInfo [gameData.objs.objects [nObject].id].drag; // Set drag to normal
i = gameData.objs.objects [nObject].cType.aiInfo.REMOTE_SLOT_NUM;
multiData.robots.lastSendTime [i] = gameData.time.xGame;
multiData.robots.sendPending [i] = 1+force;
if (force & (gameData.app.nGameMode & GM_NETWORK))
	networkData.bPacketUrgent = 1;
return;
}

//-----------------------------------------------------------------------------

void MultiSendRobotFire (int nObject, int nGun, vmsVector *vFire)
{
	// Send robot vFire event
	int bufP = 0;
	short s;
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
	vmsVector swapped_vec;
#endif

multiData.msg.buf [bufP++] = MULTI_ROBOT_FIRE;						
multiData.msg.buf [bufP++] = gameData.multi.nLocalPlayer;								
s = ObjnumLocalToRemote (nObject, (sbyte *) (multiData.msg.buf + bufP + 2));
PUT_INTEL_SHORT (multiData.msg.buf + bufP, s);
bufP += 3;
multiData.msg.buf [bufP++] = nGun;									
#if ! (defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__))
memcpy (multiData.msg.buf + bufP, vFire, sizeof (vmsVector));         
bufP += sizeof (vmsVector); // 12
// --------------------------
//      Total = 18
#else
swapped_vec.x = (fix)INTEL_INT ((int)vFire->x);
swapped_vec.y = (fix)INTEL_INT ((int)vFire->y);
swapped_vec.z = (fix)INTEL_INT ((int)vFire->z);
memcpy (multiData.msg.buf + bufP, &swapped_vec, sizeof (vmsVector)); 
bufP += sizeof (vmsVector);
#endif
if (gameData.objs.objects [nObject].cType.aiInfo.REMOTE_OWNER == gameData.multi.nLocalPlayer) {
	int slot = gameData.objs.objects [nObject].cType.aiInfo.REMOTE_SLOT_NUM;
	if ((slot < 0) || (slot >= MAX_ROBOTS_CONTROLLED))
		return;
	if (multiData.robots.fired [slot] != 0)
		return;
	memcpy (multiData.robots.fireBuf [slot], multiData.msg.buf, bufP);
	multiData.robots.fired [slot] = 1;
	if (gameData.app.nGameMode & GM_NETWORK)
		networkData.bPacketUrgent = 1;
	}
else
	MultiSendData (multiData.msg.buf, bufP, 2); // Not our robot, send ASAP
}

//-----------------------------------------------------------------------------

void MultiSendRobotExplode (int nObject, int nKiller,char bIsThief)
{
	// Send robot explosion event to the other players

	int bufP = 0;
	short s;

multiData.msg.buf [bufP++] = MULTI_ROBOT_EXPLODE;					
multiData.msg.buf [bufP++] = gameData.multi.nLocalPlayer;								
s = (short)ObjnumLocalToRemote (nKiller, (sbyte *) (multiData.msg.buf + bufP + 2));
PUT_INTEL_SHORT (multiData.msg.buf + bufP, s);                       
bufP += 3;
s = (short)ObjnumLocalToRemote (nObject, (sbyte *) (multiData.msg.buf + bufP + 2));
PUT_INTEL_SHORT (multiData.msg.buf + bufP, s);                       
bufP += 3;
multiData.msg.buf [bufP++] = bIsThief;   
MultiSendData (multiData.msg.buf, bufP, 1);
MultiDeleteControlledRobot (nObject);
}

//-----------------------------------------------------------------------------

void MultiSendCreateRobot (int station, int nObject, int nType)
{
	// Send create robot information

	int bufP = 0;

multiData.msg.buf [bufP++] = MULTI_CREATE_ROBOT;						
multiData.msg.buf [bufP++] = gameData.multi.nLocalPlayer;								
multiData.msg.buf [bufP++] = (sbyte)station;                         
PUT_INTEL_SHORT (multiData.msg.buf + bufP, nObject);                  
bufP += 2;
multiData.msg.buf [bufP++] = nType;									
MapObjnumLocalToLocal ((short)nObject);
MultiSendData (multiData.msg.buf, bufP, 2);
}

//-----------------------------------------------------------------------------

void MultiSendBossActions (int bossobjnum, int action, int secondary, int nObject)
{
	// Send special boss behavior information

	int bufP = 0;
	
multiData.msg.buf [bufP++] = MULTI_BOSS_ACTIONS;						
multiData.msg.buf [bufP++] = gameData.multi.nLocalPlayer;	
PUT_INTEL_SHORT (multiData.msg.buf + bufP, bossobjnum);  // Which player is controlling the boss        
bufP += 2; // We won't network map this nObject since it's the boss
multiData.msg.buf [bufP++] = (sbyte)action;  // What is the boss doing?
multiData.msg.buf [bufP++] = (sbyte)secondary;  // More info for what he is doing
PUT_INTEL_SHORT (multiData.msg.buf + bufP, nObject);                  
bufP += 2; // Objnum of tObject created by gate-in action
if (action == 3) {
	PUT_INTEL_SHORT (multiData.msg.buf + bufP, gameData.objs.objects [nObject].nSegment); 
	bufP += 2; // Segment number tObject created in (for gate only)
	}
else 
	bufP += 2; // Dummy

if (action == 1) { // Teleport releases robot
	// Boss is up for grabs after teleporting
	Assert (
		 (gameData.objs.objects [bossobjnum].cType.aiInfo.REMOTE_SLOT_NUM >= 0) && 
		 (gameData.objs.objects [bossobjnum].cType.aiInfo.REMOTE_SLOT_NUM < MAX_ROBOTS_CONTROLLED));
	MultiDeleteControlledRobot (bossobjnum);
//		multiData.robots.controlled [gameData.objs.objects [bossobjnum].cType.aiInfo.REMOTE_SLOT_NUM] = -1;
//		gameData.objs.objects [bossobjnum].cType.aiInfo.REMOTE_OWNER = -1;
//		gameData.objs.objects [bossobjnum].cType.aiInfo.REMOTE_SLOT_NUM = 5; // Hands-off period!
	}
MultiSendData (multiData.msg.buf, bufP, 1);
}
			
//-----------------------------------------------------------------------------
// Send create robot information

void MultiSendCreateRobotPowerups (tObject *delObjP)
{
	int	bufP = 0, hBufP;
	int	i, j = 0;
	char	h, nContained;
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
	vmsVector swapped_vec;
#endif

multiData.msg.buf [bufP++] = MULTI_CREATE_ROBOT_POWERUPS;				
multiData.msg.buf [bufP++] = gameData.multi.nLocalPlayer;				
hBufP = bufP++;
multiData.msg.buf [bufP++] = delObjP->containsType; 					
multiData.msg.buf [bufP++] = delObjP->containsId;						
PUT_INTEL_SHORT (multiData.msg.buf + bufP, delObjP->nSegment);		        
bufP += 2;
#if !(defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__))
memcpy (multiData.msg.buf + bufP, &delObjP->pos, sizeof (vmsVector));	
#else
swapped_vec.x = (fix)INTEL_INT ((int) delObjP->pos.x);
swapped_vec.y = (fix)INTEL_INT ((int) delObjP->pos.y);
swapped_vec.z = (fix)INTEL_INT ((int) delObjP->pos.z);
memcpy (multiData.msg.buf + bufP, &swapped_vec, sizeof (vmsVector));     
#endif
bufP += 12;
multiData.create.nLoc = 0;
for (nContained = delObjP->containsCount; nContained; nContained -= h) {
	h = (nContained > MAX_ROBOT_POWERUPS) ? MAX_ROBOT_POWERUPS : nContained;
	multiData.msg.buf [hBufP] = h;
	memset (multiData.msg.buf + bufP, -1, MAX_ROBOT_POWERUPS * sizeof (short));
	for (i = 0; i < h; i++, j++) {
		PUT_INTEL_SHORT (multiData.msg.buf + bufP + 2 * i, multiData.create.nObjNums [j]);
		MapObjnumLocalToLocal (multiData.create.nObjNums [j]);
		}
	MultiSendData (multiData.msg.buf, 27, 2);
	}
}

//-----------------------------------------------------------------------------

void MultiDoClaimRobot (char *buf)
{
	short nRobot, nRemoteBot;
	char nPlayer = buf [1];

nRemoteBot = GET_INTEL_SHORT (buf + 2);
nRobot = ObjnumRemoteToLocal (nRemoteBot, (sbyte)buf [4]);
if ((nRobot > gameData.objs.nLastObject) || (nRobot < 0))
//		Int3 (); // See rob
	return;
if (gameData.objs.objects [nRobot].nType != OBJ_ROBOT)
	return;
if (gameData.objs.objects [nRobot].cType.aiInfo.REMOTE_OWNER != -1)
	if (MULTI_ROBOT_PRIORITY (nRemoteBot, nPlayer) <= MULTI_ROBOT_PRIORITY (nRemoteBot, gameData.objs.objects [nRobot].cType.aiInfo.REMOTE_OWNER))
		return;
// Perform the requested change
if (gameData.objs.objects [nRobot].cType.aiInfo.REMOTE_OWNER == gameData.multi.nLocalPlayer)
	MultiDeleteControlledRobot (nRobot);
gameData.objs.objects [nRobot].cType.aiInfo.REMOTE_OWNER = nPlayer;
gameData.objs.objects [nRobot].cType.aiInfo.REMOTE_SLOT_NUM = 0;
}

//-----------------------------------------------------------------------------

void MultiDoReleaseRobot (char *buf)
{
	short nRobot, nRemoteBot;
	char nPlayer = buf [1];

nRemoteBot = GET_INTEL_SHORT (buf + 2);
nRobot = ObjnumRemoteToLocal (nRemoteBot, (sbyte)buf [4]);
if ((nRobot < 0) || (nRobot > gameData.objs.nLastObject))
	return;
if (gameData.objs.objects [nRobot].nType != OBJ_ROBOT)
	return;
if (gameData.objs.objects [nRobot].cType.aiInfo.REMOTE_OWNER != nPlayer)
	return;
// Perform the requested change
gameData.objs.objects [nRobot].cType.aiInfo.REMOTE_OWNER = -1;
gameData.objs.objects [nRobot].cType.aiInfo.REMOTE_SLOT_NUM = 0;
}

//-----------------------------------------------------------------------------
// Process robot movement sent by another player

void MultiDoRobotPosition (char *buf)
{
#if defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__)
	shortpos sp;
#endif
	short nRobot, nRemoteBot;
	int bufP = 1;
	char nPlayer = buf [bufP++];

nRemoteBot = GET_INTEL_SHORT (buf + bufP);
nRobot = ObjnumRemoteToLocal (nRemoteBot, (sbyte)buf [bufP+2]); 
bufP += 3;
if ((nRobot < 0) || (nRobot > gameData.objs.nLastObject))
	return;
if ((gameData.objs.objects [nRobot].nType != OBJ_ROBOT) || 
	 (gameData.objs.objects [nRobot].flags & OF_EXPLODING)) 
	return;
if (gameData.objs.objects [nRobot].cType.aiInfo.REMOTE_OWNER != nPlayer) {	
	if (gameData.objs.objects [nRobot].cType.aiInfo.REMOTE_OWNER != -1)
		return;
		// Robot claim packet must have gotten lost, let this player claim it.
	else if (gameData.objs.objects [nRobot].cType.aiInfo.REMOTE_SLOT_NUM > 3) {
		gameData.objs.objects [nRobot].cType.aiInfo.REMOTE_OWNER = nPlayer;
		gameData.objs.objects [nRobot].cType.aiInfo.REMOTE_SLOT_NUM = 0;
		}
	else
		gameData.objs.objects [nRobot].cType.aiInfo.REMOTE_SLOT_NUM++;
	}
set_thrust_from_velocity (&gameData.objs.objects [nRobot]); // Try to smooth out movement
//	gameData.objs.objects [nRobot].physInfo.drag = gameData.bots.pInfo [gameData.objs.objects [nRobot].id].drag >> 4; // Set drag to low
#if ! (defined (WORDS_BIGENDIAN) || defined (__BIG_ENDIAN__))
ExtractShortPos (&gameData.objs.objects [nRobot], (shortpos *) (buf+bufP), 0);
#else
memcpy ((ubyte *) (sp.bytemat), (ubyte *) (buf + bufP), 9);		
bufP += 9;
memcpy ((ubyte *)& (sp.xo), (ubyte *) (buf + bufP), 14);
ExtractShortPos (&gameData.objs.objects [nRobot], &sp, 1);
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
	vmsVector	vFire, vGunPoint;
	tRobotInfo	*robotP;

nPlayer = buf [bufP++];												
nRemoteBot = GET_INTEL_SHORT (buf + bufP);
nRobot = ObjnumRemoteToLocal (nRemoteBot, (sbyte)buf [bufP+2]); 
bufP += 3;
nGun = (sbyte)buf [bufP++];                                      
memcpy (&vFire, buf+bufP, sizeof (vmsVector));
vFire.x = (fix)INTEL_INT ((int)vFire.x);
vFire.y = (fix)INTEL_INT ((int)vFire.y);
vFire.z = (fix)INTEL_INT ((int)vFire.z);
if ((nRobot < 0) || (nRobot > gameData.objs.nLastObject) || 
		 (gameData.objs.objects [nRobot].nType != OBJ_ROBOT) || 
		 (gameData.objs.objects [nRobot].flags & OF_EXPLODING))
	return;
// Do the firing
if (nGun == -1 || nGun==-2)
	// Drop proximity bombs
	VmVecAdd (&vGunPoint, &gameData.objs.objects [nRobot].pos, &vFire);
else 
	calc_gun_point (&vGunPoint, &gameData.objs.objects [nRobot], nGun);
robotP = &gameData.bots.pInfo [gameData.objs.objects [nRobot].id];
if (nGun == -1) 
	CreateNewLaserEasy (&vFire, &vGunPoint, nRobot, PROXIMITY_ID, 1);
else if (nGun == -2)
	CreateNewLaserEasy (&vFire, &vGunPoint, nRobot, SUPERPROX_ID, 1);
else	
	CreateNewLaserEasy (&vFire, &vGunPoint, nRobot, robotP->nWeaponType, 1);
}

//-----------------------------------------------------------------------------

extern void DropStolenItems (tObject *);

int MultiExplodeRobotSub (int nRobot, int nKiller,char bIsThief)
{
	tObject *robotP;

if ((nRobot < 0) || (nRobot > gameData.objs.nLastObject)) { // Objnum in range?
	Int3 (); // See rob
	return 0;
	}
if (gameData.objs.objects [nRobot].nType != OBJ_ROBOT) // Object is robotP?
	return 0;
	if (gameData.objs.objects [nRobot].flags & OF_EXPLODING) // Object not already exploding
		return 0;
// Data seems valid, explode the sucker
if (networkData.bSendObjects && NetworkObjnumIsPast (nRobot))
	networkData.nSendObjNum = -1;
robotP = &gameData.objs.objects [nRobot];
// Drop non-random KEY powerups locally only!
if ((robotP->containsCount > 0) && 
	 (robotP->containsType == OBJ_POWERUP) && 
	 (gameData.app.nGameMode & GM_MULTI_COOP) && 
	 (robotP->containsId >= POW_KEY_BLUE) && 
	 (robotP->containsId <= POW_KEY_GOLD))
	ObjectCreateEgg (robotP);
else if (robotP->cType.aiInfo.REMOTE_OWNER == gameData.multi.nLocalPlayer) {
	MultiDropRobotPowerups (OBJ_IDX (robotP));
	MultiDeleteControlledRobot (OBJ_IDX (robotP));
	}
else if (robotP->cType.aiInfo.REMOTE_OWNER == -1 && NetworkIAmMaster ()) {
	MultiDropRobotPowerups (OBJ_IDX (robotP));
	//MultiDeleteControlledRobot (OBJ_IDX (robotP));
	}
if (bIsThief || gameData.bots.pInfo [robotP->id].thief)
	DropStolenItems (robotP);
if (gameData.bots.pInfo [robotP->id].bossFlag) {
	if (gameData.boss.nDying)
		return 0;
	start_boss_death_sequence (robotP);	
	}
else if (gameData.bots.pInfo [robotP->id].death_roll)
	StartRobotDeathSequence (robotP);
else {
	if (robotP->id == SPECIAL_REACTOR_ROBOT)
		SpecialReactorStuff ();
	if (gameData.bots.pInfo [robotP->id].kamikaze)
		ExplodeObject (robotP,1);	//	Kamikaze, explode right away, IN YOUR FACE!
	else
		ExplodeObject (robotP,STANDARD_EXPL_DELAY);
   }
return 1;
}

//-----------------------------------------------------------------------------

void MultiDoRobotExplode (char *buf)
{
	// Explode robot controlled by other player

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
if ((nRobot < 0) || (nRobot > gameData.objs.nLastObject))
	return;
rval = MultiExplodeRobotSub (nRobot, nKiller,thief);
if (rval && (nKiller == gameData.multi.players [gameData.multi.nLocalPlayer].nObject))
	AddPointsToScore (gameData.bots.pInfo [gameData.objs.objects [nRobot].id].scoreValue);
}

//-----------------------------------------------------------------------------

void MultiDoCreateRobot (char *buf)
{
	
	int		fuelcen_num = buf [2];
	int		nPlayer = buf [1];
	short		nObject;
	ubyte		nType = buf [5];

	fuelcen_info *robotcen;
	vmsVector curObject_loc, direction;
	tObject *objP;

nObject = GET_INTEL_SHORT (buf + 3);
if ((nPlayer < 0) || (nObject < 0) || (fuelcen_num < 0) || 
	 (fuelcen_num >= gameData.matCens.nFuelCenters) || (nPlayer >= gameData.multi.nPlayers)) {
	Int3 (); // Bogus data
	return;
	}
robotcen = gameData.matCens.fuelCenters + fuelcen_num;
// Play effect and sound
COMPUTE_SEGMENT_CENTER_I (&curObject_loc, robotcen->nSegment);
objP = ObjectCreateExplosion ((short) robotcen->nSegment, &curObject_loc, i2f (10), VCLIP_MORPHING_ROBOT);
if (objP)
	ExtractOrientFromSegment (&objP->orient, &gameData.segs.segments [robotcen->nSegment]);
if (gameData.eff.vClips [0][VCLIP_MORPHING_ROBOT].nSound > -1)
	DigiLinkSoundToPos (gameData.eff.vClips [0][VCLIP_MORPHING_ROBOT].nSound, (short) robotcen->nSegment, 0, &curObject_loc, 0, F1_0);
// Set robot center flags, in case we become the master for the next one
robotcen->Flag = 0;
robotcen->Capacity -= gameData.matCens.xEnergyToCreateOneRobot;
robotcen->Timer = 0;
if (! (objP = CreateMorphRobot (gameData.segs.segments + robotcen->nSegment, &curObject_loc, nType)))
	return; // Cannot create tObject!
objP->matCenCreator = ((short) (robotcen - gameData.matCens.fuelCenters)) | 0x80;
//	ExtractOrientFromSegment (&objP->orient, &gameData.segs.segments [robotcen->nSegment]);
VmVecSub (&direction, &gameData.objs.console->pos, &objP->pos);
VmVector2Matrix (&objP->orient, &direction, &objP->orient.uvec, NULL);
MorphStart (objP);
MapObjnumLocalToRemote (OBJ_IDX (objP), nObject, nPlayer);
Assert (objP->cType.aiInfo.REMOTE_OWNER == -1);
}

//-----------------------------------------------------------------------------

void MultiDoBossActions (char *buf)
{
	// Code to handle remote-controlled boss actions

	tObject	*bossObjP;
	short		nBossObj, boss_index;
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
if ((nBossObj < 0) || (nBossObj > gameData.objs.nLastObject)) {
	Int3 ();  // See Rob
	return;
	}
bossObjP = gameData.objs.objects + nBossObj;
if ((bossObjP->nType != OBJ_ROBOT) || ! (gameData.bots.pInfo [bossObjP->id].bossFlag)) {
	Int3 (); // Got boss actions for a robot who's not a boss?
	return;
	}
for (boss_index = 0; boss_index < extraGameInfo [0].nBossCount; boss_index++)
	if (gameData.boss.objList [boss_index] == nBossObj)
		break;
if (gameData.boss.objList [boss_index] == nBossObj) {
	Int3 (); // Got boss actions for a robot who's not a boss?
	return;
	}
switch (action)  {
	case 1: // Teleport
		{	
		short nTeleportSeg;

		vmsVector boss_dir;
		if ((secondary < 0) || (secondary > gameData.boss.nTeleportSegs [boss_index])) {
			Int3 (); // Bad nSegment for boss teleport, ROB!!
			return;
			}
		nTeleportSeg = gameData.boss.teleportSegs [boss_index][secondary];
		if ((nTeleportSeg < 0) || (nTeleportSeg > gameData.segs.nLastSegment)) {
			Int3 ();  // See Rob
			return;
			}
		COMPUTE_SEGMENT_CENTER_I (&bossObjP->pos, nTeleportSeg);
		RelinkObject (OBJ_IDX (bossObjP), nTeleportSeg);
		gameData.boss.nLastTeleportTime = gameData.time.xGame;
		VmVecSub (&boss_dir, &gameData.objs.objects [gameData.multi.players [nPlayer].nObject].pos, &bossObjP->pos);
		VmVector2Matrix (&bossObjP->orient, &boss_dir, NULL, NULL);
		DigiLinkSoundToPos (gameData.eff.vClips [0][VCLIP_MORPHING_ROBOT].nSound, nTeleportSeg, 0, &bossObjP->pos, 0 , F1_0);
		DigiKillSoundLinkedToObject (OBJ_IDX (bossObjP));
		DigiLinkSoundToObject2 (SOUND_BOSS_SHARE_SEE, OBJ_IDX (bossObjP), 1, F1_0, F1_0*512);	//	F1_0*512 means play twice as loud
		gameData.ai.localInfo [OBJ_IDX (bossObjP)].next_fire = 0;
		if (bossObjP->cType.aiInfo.REMOTE_OWNER == gameData.multi.nLocalPlayer) {
			MultiDeleteControlledRobot (nBossObj);
//			multiData.robots.controlled [bossObjP->cType.aiInfo.REMOTE_SLOT_NUM] = -1;
			}
		bossObjP->cType.aiInfo.REMOTE_OWNER = -1; // Boss is up for grabs again!
		bossObjP->cType.aiInfo.REMOTE_SLOT_NUM = 0; // Available immediately!
		}
		break;

	case 2: // Cloak
		gameData.boss.nHitTime = -F1_0*10;
		gameData.boss.nCloakStartTime = gameData.time.xGame;
		gameData.boss.nCloakEndTime = gameData.time.xGame + gameData.boss.nCloakDuration;
		bossObjP->cType.aiInfo.CLOAKED = 1;
		break;

	case 3: // Gate in robots!
		// Do some validity checking
		if ((nRemoteObj >= MAX_OBJECTS) || (nRemoteObj < 0) || (nSegment < 0) || 
				(nSegment > gameData.segs.nLastSegment)) {
			Int3 (); // See Rob, bad data in boss gate action message
			return;
			}
		// Gate one in!
		if (GateInRobot (nBossObj, (ubyte) secondary, nSegment))
			MapObjnumLocalToRemote (multiData.create.nObjNums [0], nRemoteObj, nPlayer);
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
	tObject	delObjP;
	int		nPlayer, nEggObj, i, bufP = 1;
	short		s;

nPlayer = buf [bufP++];										
delObjP.containsCount = buf [bufP++];						
delObjP.containsType = buf [bufP++];						
delObjP.containsId = buf [bufP++]; 						
delObjP.nSegment = GET_INTEL_SHORT (buf + bufP);            
bufP += 2;
memcpy (&delObjP.pos, buf+bufP, sizeof (vmsVector));      
bufP += 12;
VmVecZero (&delObjP.mType.physInfo.velocity);
delObjP.pos.x = (fix)INTEL_INT ((int)delObjP.pos.x);
delObjP.pos.y = (fix)INTEL_INT ((int)delObjP.pos.y);
delObjP.pos.z = (fix)INTEL_INT ((int)delObjP.pos.z);
Assert ((nPlayer >= 0) && (nPlayer < gameData.multi.nPlayers));
Assert (nPlayer != gameData.multi.nLocalPlayer); // What? How'd we send ourselves this?
multiData.create.nLoc = 0;
d_srand (1245L);
nEggObj = ObjectCreateEgg (&delObjP);
if (nEggObj == -1)
	return; // Object buffer full
//	Assert (nEggObj > -1);
Assert ((multiData.create.nLoc > 0) && (multiData.create.nLoc <= MAX_ROBOT_POWERUPS));
for (i = 0; i < multiData.create.nLoc; i++) {
	s = GET_INTEL_SHORT (buf + bufP);
	if (s != -1)
		MapObjnumLocalToRemote ((short)multiData.create.nObjNums [i], s, nPlayer);
	else
		gameData.objs.objects [multiData.create.nObjNums [i]].flags |= OF_SHOULD_BE_DEAD; // Delete gameData.objs.objects other guy didn't create one of
	bufP += 2;
	}
}

//-----------------------------------------------------------------------------
// Code to handle dropped robot powerups in network mode ONLY!

void MultiDropRobotPowerups (int nObject)
{
	tObject		*delObjP;
	int			nEggObj = -1;
	tRobotInfo	*robotP; 

if ((nObject < 0) || (nObject > gameData.objs.nLastObject)) {
	Int3 ();  // See rob
	return;
	}
delObjP = &gameData.objs.objects [nObject];
if (delObjP->nType != OBJ_ROBOT) {
	Int3 (); // dropping powerups for non-robot, Rob's fault
	return;
	}
robotP = &gameData.bots.pInfo [delObjP->id];
multiData.create.nLoc = 0;
if (delObjP->containsCount > 0) { 
	//	If dropping a weapon that the player has, drop energy instead, unless it's vulcan, in which case drop vulcan ammo.
	if (delObjP->containsType == OBJ_POWERUP) {
		MaybeReplacePowerupWithEnergy (delObjP);
		if (!MultiPowerupIsAllowed (delObjP->containsId))
			delObjP->containsId=POW_SHIELD_BOOST;
		// No key drops in non-coop games!
		if (! (gameData.app.nGameMode & GM_MULTI_COOP)) {
			if ((delObjP->containsId >= POW_KEY_BLUE) && (delObjP->containsId <= POW_KEY_GOLD))
				delObjP->containsCount = 0;
			}
		}
	d_srand (1245L);
	if (delObjP->containsCount > 0)
		nEggObj = ObjectCreateEgg (delObjP);
	}
else if (delObjP->cType.aiInfo.REMOTE_OWNER == -1) // No random goodies for robots we weren't in control of
	return;
else if (robotP->containsCount) {
	d_srand (TimerGetApproxSeconds ());
	if (((d_rand () * 16) >> 15) < robotP->containsProb) {
		delObjP->containsCount = ((d_rand () * robotP->containsCount) >> 15) + 1;
		delObjP->containsType = robotP->containsType;
		delObjP->containsId = robotP->containsId;
		if (delObjP->containsType == OBJ_POWERUP) {
			MaybeReplacePowerupWithEnergy (delObjP);
			if (!MultiPowerupIsAllowed (delObjP->containsId))
				delObjP->containsId=POW_SHIELD_BOOST;
			 }
		d_srand (1245L);
		if (delObjP->containsCount > 0)
			nEggObj = ObjectCreateEgg (delObjP);
		}
	}
if (nEggObj >= 0) // Transmit the tObject creation to the other players	 	
	MultiSendCreateRobotPowerups (delObjP);
}

//	-----------------------------------------------------------------------------
//	Robot *robot got whacked by player player_num and requests permission to do something about it.
//	Note: This function will be called regardless of whether gameData.app.nGameMode is a multiplayer mode, so it
//	should quick-out if not in a multiplayer mode.  On the other hand, it only gets called when a
//	player or player weapon whacks a robot, so it happens rarely.
void MultiRobotRequestChange (tObject *robot, int player_num)
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
if ((multiData.robots.agitation [slot] < 70) || 
	 (MULTI_ROBOT_PRIORITY (nRemoteObj, player_num) > 
	  MULTI_ROBOT_PRIORITY (nRemoteObj, gameData.multi.nLocalPlayer)) || 
	  (d_rand () > 0x4400))	{
	if (multiData.robots.sendPending [slot])
		MultiSendRobotPosition (multiData.robots.controlled [slot], -1);
	MultiSendReleaseRobot (multiData.robots.controlled [slot]);
	robot->cType.aiInfo.REMOTE_SLOT_NUM = 5;  // Hands-off period
	}
}

//-----------------------------------------------------------------------------
//eof