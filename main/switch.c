/* $Id: switch.c,v 1.9 2003/10/04 03:14:48 btb Exp $ */
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
static char rcsid [] = "$Id: switch.c,v 1.9 2003/10/04 03:14:48 btb Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "inferno.h"
#include "gauges.h"
#include "newmenu.h"
#include "game.h"
#include "switch.h"
#include "segment.h"
#include "error.h"
#include "gameseg.h"
#include "mono.h"
#include "wall.h"
#include "texmap.h"
#include "fuelcen.h"
#include "cntrlcen.h"
#include "newdemo.h"
#include "player.h"
#include "endlevel.h"
#include "gameseq.h"
#include "multi.h"
#include "network.h"
#include "palette.h"
#include "robot.h"
#include "bm.h"
#include "timer.h"
#include "segment.h"
#include "kconfig.h"
#include "text.h"
#include "lighting.h"
#include "hudmsg.h"

#ifdef EDITOR
#include "editor/editor.h"
#endif

#define MAX_ORIENT_STEPS	10

#define TT_OPEN_DOOR        0   // Open a door
#define TT_CLOSE_DOOR       1   // Close a door
#define TT_MATCEN           2   // Activate a matcen
#define TT_EXIT             3   // End the level
#define TT_SECRET_EXIT      4   // Go to secret level
#define TT_ILLUSION_OFF     5   // Turn an illusion off
#define TT_ILLUSION_ON      6   // Turn an illusion on
#define TT_UNLOCK_DOOR      7   // Unlock a door
#define TT_LOCK_DOOR        8   // Lock a door
#define TT_OPEN_WALL        9   // Makes a wall open
#define TT_CLOSE_WALL       10  // Makes a wall closed
#define TT_ILLUSORY_WALL    11  // Makes a wall illusory
#define TT_LIGHT_OFF        12  // Turn a light off
#define TT_LIGHT_ON         13  // Turn s light on
#define TT_TELEPORT			 14
#define TT_SPEEDBOOST		 15
#define TT_CAMERA				 16
#define TT_SHIELD_DAMAGE	 17
#define TT_ENERGY_DRAIN		 18
#define NUM_TRIGGER_TYPES   19

int oppTrigTypes [] = {
	TT_CLOSE_DOOR,
	TT_OPEN_DOOR,
	TT_MATCEN,
	TT_EXIT,
	TT_SECRET_EXIT,
	TT_ILLUSION_ON,
	TT_ILLUSION_OFF,
	TT_LOCK_DOOR,
	TT_UNLOCK_DOOR,
	TT_CLOSE_WALL,
	TT_OPEN_WALL,
	TT_ILLUSORY_WALL,
	TT_LIGHT_ON,
	TT_LIGHT_OFF,
	TT_TELEPORT,
	TT_SPEEDBOOST,
	TT_CAMERA,
	TT_SHIELD_DAMAGE,
	TT_ENERGY_DRAIN
	};

//link Links [MAX_WALL_LINKS];
//int Num_links;

#ifdef EDITOR
fix triggerTime_count=F1_0;

//-----------------------------------------------------------------
// Initializes all the switches.
void trigger_init ()
{
	int i;

	gameData.trigs.nTriggers = 0;

for (i = 0; i < MAX_TRIGGERS; i++) {
	gameData.trigs.triggers [i].nType = 0;
	gameData.trigs.triggers [i].flags = 0;
	gameData.trigs.triggers [i].nLinks = 0;
	gameData.trigs.triggers [i].value = 0;
	gameData.trigs.triggers [i].time = -1;
	}
memset (gameData.trigs.delay, -1, sizeof (gameData.trigs.delay));
}
#endif

//-----------------------------------------------------------------
// Executes a link, attached to a tTrigger.
// Toggles all walls linked to the switch.
// Opens doors, Blasts blast walls, turns off illusions.
void DoLink (tTrigger *trigP)
{
	int i;

short *segs = trigP->nSegment;
short *sides = trigP->nSide;
for (i = trigP->nLinks; i; i--, segs++, sides++)
	WallToggle (gameData.segs.segments + *segs, *sides);
}

//------------------------------------------------------------------------------
//close a door
void DoCloseDoor (tTrigger *trigP)
{
	int i;

short *segs = trigP->nSegment;
short *sides = trigP->nSide;
for (i = trigP->nLinks; i; i--, segs++, sides++)
	WallCloseDoor (gameData.segs.segments+*segs, *sides);
}

//------------------------------------------------------------------------------
//turns lighting on.  returns true if lights were actually turned on. (they
//would not be if they had previously been shot out).
int DoLightOn (tTrigger *trigP)
{
	int i,ret=0;

short *segs = trigP->nSegment;
short *sides = trigP->nSide;
short nSegment,nSide;
for (i = trigP->nLinks; i; i--, segs++, sides++) {
	nSegment = *segs;
	nSide = *sides;

	//check if tmap2 casts light before turning the light on.  This
	//is to keep us from turning on blown-out lights
	if (gameData.pig.tex.pTMapInfo [gameData.segs.segments [nSegment].sides [nSide].nOvlTex].lighting) {
		ret |= AddLight (nSegment, nSide); 		//any light sets flag
		EnableFlicker (nSegment, nSide);
	}
}
return ret;
}

//------------------------------------------------------------------------------
//turns lighting off.  returns true if lights were actually turned off. (they
//would not be if they had previously been shot out).
int DoLightOff (tTrigger *trigP)
{
	int i,ret=0;

short *segs = trigP->nSegment;
short *sides = trigP->nSide;
short nSegment,nSide;
for (i = trigP->nLinks; i; i--, segs++, sides++) {
	nSegment = *segs;
	nSide = *sides;

	//check if tmap2 casts light before turning the light off.  This
	//is to keep us from turning off blown-out lights
	if (gameData.pig.tex.pTMapInfo [gameData.segs.segments [nSegment].sides [nSide].nOvlTex].lighting) {
		ret |= SubtractLight (nSegment, nSide); 	//any light sets flag
		DisableFlicker (nSegment, nSide);
	}
}
return ret;
}

//------------------------------------------------------------------------------
// Unlocks all doors linked to the switch.
void DoUnlockDoors (tTrigger *trigP)
{
	int i;

short *segs = trigP->nSegment;
short *sides = trigP->nSide;
short nSegment,nSide, nWall;
for (i = trigP->nLinks; i; i--, segs++, sides++) {
	nSegment = *segs;
	nSide = *sides;
	nWall=WallNumI (nSegment, nSide);
	gameData.walls.walls [nWall].flags &= ~WALL_DOOR_LOCKED;
	gameData.walls.walls [nWall].keys = KEY_NONE;
}
}

//------------------------------------------------------------------------------
// Return tTrigger number if door is controlled by a wall switch, else return -1.
int DoorIsWallSwitched (int nWall)
{
	int i, nTrigger;
	tTrigger *trigP = gameData.trigs.triggers;
	short *segs, *sides;

for (nTrigger=0; nTrigger < gameData.trigs.nTriggers; nTrigger++, trigP++) {
	segs = trigP->nSegment;
	sides = trigP->nSide;
	for (i = trigP->nLinks; i; i--, segs++, sides++) {
		if (WallNumI (*segs, *sides) == nWall) {
			return nTrigger;
			}
	  	}
	}
return -1;
}

//------------------------------------------------------------------------------

void FlagWallSwitchedDoors (void)
{
	int	i;

for (i = 0; i < gameData.walls.nWalls; i++)
	if (DoorIsWallSwitched (i))
		gameData.walls.walls [i].flags |= WALL_WALL_SWITCH;
}

//------------------------------------------------------------------------------
// Locks all doors linked to the switch.
void DoLockDoors (tTrigger *trigP)
{
	int i;
	short *segs = trigP->nSegment;
	short *sides = trigP->nSide;

for (i = trigP->nLinks; i; i--, segs++, sides++) {
	gameData.walls.walls [WallNumI (*segs, *sides)].flags |= WALL_DOOR_LOCKED;
}
}

//------------------------------------------------------------------------------
// Changes walls pointed to by a tTrigger. returns true if any walls changed
int DoChangeWalls (tTrigger *trigP)
{
	int 		i,ret=0;
	short 	*segs = trigP->nSegment;
	short 	*sides = trigP->nSide;
	short 	nSide,nConnSide,nWall,nConnWall;
	int 		nNewWallType;
	tSegment *segP,*cSegP;

for (i = trigP->nLinks; i; i--, segs++, sides++) {

	segP = gameData.segs.segments + *segs;
	nSide = *sides;

	if (segP->children [nSide] < 0) {
		if (gameOpts->legacy.bSwitches)
			Warning (TXT_TRIG_SINGLE, *segs, nSide, TRIG_IDX (trigP));
		cSegP = NULL;
		nConnSide = -1;
		}
	else {
		cSegP = gameData.segs.segments + segP->children [nSide];
		nConnSide = FindConnectedSide (segP, cSegP);
		}
	switch (trigP->nType) {
		case TT_OPEN_WALL:
			nNewWallType = WALL_OPEN; 
			break;
		case TT_CLOSE_WALL:		
			nNewWallType = WALL_CLOSED; 
			break;
		case TT_ILLUSORY_WALL:	
			nNewWallType = WALL_ILLUSION; 
			break;
		default:
			Assert (0); /* nNewWallType unset */
			return 0;
			break;
		}
	nWall = WallNumP (segP, nSide);
	if (!IS_WALL (nWall)) {
#ifdef _DEBUG
		LogErr ("WARNING: Wall trigger %d targets non-existant wall @ %d,%d\n", 
				  trigP - gameData.trigs.triggers, SEG_IDX (segP), nSide);
#endif
		continue;
		}
	nConnWall = (nConnSide < 0) ? NO_WALL : WallNumP (cSegP, nConnSide);
	if ((gameData.walls.walls [nWall].nType == nNewWallType) &&
		 (!IS_WALL (nConnWall) || (gameData.walls.walls [nConnWall].nType == nNewWallType)))
		continue;		//already in correct state, so skip
	ret = 1;
	switch (trigP->nType) {
		case TT_OPEN_WALL:
			if (!(gameData.pig.tex.pTMapInfo [segP->sides [nSide].nBaseTex].flags & TMI_FORCE_FIELD)) 
				StartWallCloak (segP,nSide);
			else {
				vmsVector pos;
				COMPUTE_SIDE_CENTER (&pos, segP, nSide);
				DigiLinkSoundToPos (SOUND_FORCEFIELD_OFF, SEG_IDX (segP), nSide, &pos, 0, F1_0);
				gameData.walls.walls [nWall].nType = nNewWallType;
				DigiKillSoundLinkedToSegment (SEG_IDX (segP),nSide,SOUND_FORCEFIELD_HUM);
				if (IS_WALL (nConnWall)) {
					gameData.walls.walls [nConnWall].nType = nNewWallType;
					DigiKillSoundLinkedToSegment (SEG_IDX (cSegP),nConnSide,SOUND_FORCEFIELD_HUM);
					}
				}
			ret = 1;
			break;

		case TT_CLOSE_WALL:
			if (!(gameData.pig.tex.pTMapInfo [segP->sides [nSide].nBaseTex].flags & TMI_FORCE_FIELD)) 
				StartWallDecloak (segP,nSide);
			else {
				vmsVector pos;
				COMPUTE_SIDE_CENTER (&pos, segP, nSide);
				DigiLinkSoundToPos (SOUND_FORCEFIELD_HUM, SEG_IDX (segP),nSide,&pos,1, F1_0/2);
				gameData.walls.walls [nWall].nType = nNewWallType;
				if (IS_WALL (nConnWall))
					gameData.walls.walls [nConnWall].nType = nNewWallType;
				}
			break;

		case TT_ILLUSORY_WALL:
			gameData.walls.walls [WallNumP (segP, nSide)].nType = nNewWallType;
			if (IS_WALL (nConnWall))
				gameData.walls.walls [nConnWall].nType = nNewWallType;
			break;
		}
	KillStuckObjects (WallNumP (segP, nSide));
	if (IS_WALL (nConnWall))
		KillStuckObjects (nConnWall);
  	}
return ret;
}

//------------------------------------------------------------------------------

void PrintTriggerMessage (int nPlayer,int trig,int shot,char *message)
 {
	char *pl;		//points to 's' or nothing for plural word

   if (nPlayer != gameData.multi.nLocalPlayer)
		return;

	pl = (gameData.trigs.triggers [trig].nLinks>1)?"s":"";

    if (!(gameData.trigs.triggers [trig].flags & TF_NO_MESSAGE) && shot)
     HUDInitMessage (message,pl);
 }


//------------------------------------------------------------------------------

void DoMatCen (tTrigger *trigP)
{
	int i;

short *segs = trigP->nSegment;
for (i = trigP->nLinks; i; i--, segs++)
	MatCenTrigger (*segs);
}

//------------------------------------------------------------------------------

void DoIllusionOn (tTrigger *trigP)
{
	int i;

short *segs = trigP->nSegment;
short *sides = trigP->nSide;
for (i = trigP->nLinks; i; i--, segs++, sides++) {
	WallIllusionOn (&gameData.segs.segments [*segs], *sides);
}
}

//------------------------------------------------------------------------------

void DoIllusionOff (tTrigger *trigP)
{
	int i;
	short *segs = trigP->nSegment;
	short *sides = trigP->nSide;
	tSegment *seg;

for (i = trigP->nLinks; i; i--, segs++, sides++) {
	vmsVector	cp;
	seg = gameData.segs.segments + *segs;
	WallIllusionOff (seg, *sides);
	COMPUTE_SIDE_CENTER (&cp, seg, *sides);
	DigiLinkSoundToPos (SOUND_WALL_REMOVED, SEG_IDX (seg), *sides, &cp, 0, F1_0);
  	}
}

//------------------------------------------------------------------------------

void TriggerSetObjOrient (short nObject, short nSegment, short nSide, int bSetPos, int nStep)
{
	vmsAngVec	ad, an, av;
	vmsVector	n, vel;
	vmsMatrix	rm;
	tObject		*objP = gameData.objs.objects + nObject;

if (nStep <= 0) {
	n = *gameData.segs.segments [nSegment].sides [nSide].normals;
	n.p.x = -n.p.x;
	n.p.y = -n.p.y;
	n.p.z = -n.p.z;
	gameStates.gameplay.vTgtDir = n;
	if (nStep < 0)
		nStep = MAX_ORIENT_STEPS;
	}
else
	n = gameStates.gameplay.vTgtDir;
// turn the ship so that it is facing the destination nSide of the destination tSegment
// invert the normal as it points into the tSegment
// compute angles from the normal
VmExtractAnglesVector (&an, &n);
// create new orientation matrix
if (!nStep)
	VmAngles2Matrix (&objP->position.mOrient, &an);
if (bSetPos)
	COMPUTE_SEGMENT_CENTER_I (&objP->position.vPos, + nSegment); 
// rotate the ships vel vector accordingly
VmExtractAnglesVector (&av, &objP->mType.physInfo.velocity);
av.p -= an.p;
av.b -= an.b;
av.h -= an.h;
if (nStep) {
	if (nStep > 1) {
		av.p /= nStep;
		av.b /= nStep;
		av.h /= nStep;
		VmExtractAnglesMatrix (&ad, &objP->position.mOrient);
		ad.p += (an.p - ad.p) / nStep;
		ad.b += (an.b - ad.b) / nStep;
		ad.h += (an.h - ad.h) / nStep;
		VmAngles2Matrix (&objP->position.mOrient, &ad);
		}
	else
		VmAngles2Matrix (&objP->position.mOrient, &an);
	}
VmAngles2Matrix (&rm, &av);
VmVecRotate (&vel, &objP->mType.physInfo.velocity, &rm);
objP->mType.physInfo.velocity = vel;
//StopPlayerMovement ();
}

//------------------------------------------------------------------------------

void TriggerSetObjPos (short nObject, short nSegment)
{
RelinkObject (nObject, nSegment);
}

//------------------------------------------------------------------------------

void DoTeleport (tTrigger *trigP, short nObject)
{
if (trigP->nLinks > 0) {
		int		i;
		short		nSegment, nSide;

	d_srand (TimerGetFixedSeconds ());
	i = d_rand () % trigP->nLinks;
	nSegment = trigP->nSegment [i];
	nSide = trigP->nSide [i];
	// set new tPlayer direction, facing the destination nSide
	TriggerSetObjOrient (nObject, nSegment, nSide, 1, 0);
	TriggerSetObjPos (nObject, nSegment);
	gameStates.render.bDoAppearanceEffect = 1;
	MultiSendTeleport ((char) nObject, nSegment, (char) nSide);
	}
}

//------------------------------------------------------------------------------

wall *TriggerParentWall (short nTrigger)
{
	int	i;

for (i = 0; i < gameData.walls.nWalls; i++)
	if (gameData.walls.walls [i].nTrigger == nTrigger)
		return gameData.walls.walls + i;
return NULL;
}

//------------------------------------------------------------------------------

fix			speedBoostSpeed = 0;

void SetSpeedBoostVelocity (short nObject, fix speed, 
									 short srcSegnum, short srcSidenum,
									 short destSegnum, short destSidenum,
									 vmsVector *pSrcPt, vmsVector *pDestPt,
									 int bSetOrient)
{
	vmsVector			n, h;
	tObject				*objP = gameData.objs.objects + nObject;
	int					v;
	tSpeedBoostData	sbd = gameData.objs.speedBoost [nObject];

if (speed < 0)
	speed = speedBoostSpeed;
if ((speed <= 0) || (speed > 10))
	speed = 10;
speedBoostSpeed = speed;
v = 60 + (COMPETITION ? 100 : extraGameInfo [IsMultiGame].nSpeedBoost) * 4 * speed;
if (sbd.bBoosted) {
	if (pSrcPt && pDestPt) {
		VmVecSub (&n, pDestPt, pSrcPt);
		VmVecNormalize (&n);
		}
	else if (srcSegnum >= 0) {
		COMPUTE_SIDE_CENTER (&sbd.vSrc, gameData.segs.segments + srcSegnum, srcSidenum);
		COMPUTE_SIDE_CENTER (&sbd.vDest, gameData.segs.segments + destSegnum, destSidenum);
		if (memcmp (&sbd.vSrc, &sbd.vDest, sizeof (vmsVector))) {
			VmVecSub (&n, &sbd.vDest, &sbd.vSrc);
			VmVecNormalize (&n);
			}
		else {
			Controls.vertical_thrustTime =
			Controls.forward_thrustTime =
			Controls.sideways_thrustTime = 0;
			memcpy (&n, gameData.segs.segments [destSegnum].sides [destSidenum].normals, sizeof (n));
		// turn the ship so that it is facing the destination nSide of the destination tSegment
		// invert the normal as it points into the tSegment
			n.p.x = -n.p.x;
			n.p.y = -n.p.y;
			n.p.z = -n.p.z;
			}
		}
	else {
		memcpy (&n, gameData.segs.segments [destSegnum].sides [destSidenum].normals, sizeof (n));
	// turn the ship so that it is facing the destination nSide of the destination tSegment
	// invert the normal as it points into the tSegment
		n.p.x = -n.p.x;
		n.p.y = -n.p.y;
		n.p.z = -n.p.z;
		}
	sbd.vVel.p.x = n.p.x * v;
	sbd.vVel.p.y = n.p.y * v;
	sbd.vVel.p.z = n.p.z * v;
#if 0
	d = (double) (labs (n.p.x) + labs (n.p.y) + labs (n.p.z)) / ((double) F1_0 * 60.0);
	h.p.x = n.p.x ? (fix) ((double) n.p.x / d) : 0;
	h.p.y = n.p.y ? (fix) ((double) n.p.y / d) : 0;
	h.p.z = n.p.z ? (fix) ((double) n.p.z / d) : 0;
#else
#	if 1
	h.p.x =
	h.p.y =
	h.p.z = F1_0 * 60;
#	else
	h.p.x = (n.p.x ? n.p.x : F1_0) * 60;
	h.p.y = (n.p.y ? n.p.y : F1_0) * 60;
	h.p.z = (n.p.z ? n.p.z : F1_0) * 60;
#	endif
#endif
	VmVecSub (&sbd.vMinVel, &sbd.vVel, &h);
/*
	if (!sbd.vMinVel.p.x)
		sbd.vMinVel.p.x = F1_0 * -60;
	if (!sbd.vMinVel.p.y)
		sbd.vMinVel.p.y = F1_0 * -60;
	if (!sbd.vMinVel.p.z)
		sbd.vMinVel.p.z = F1_0 * -60;
*/
	VmVecAdd (&sbd.vMaxVel, &sbd.vVel, &h);
/*
	if (!sbd.vMaxVel.p.x)
		sbd.vMaxVel.p.x = F1_0 * 60;
	if (!sbd.vMaxVel.p.y)
		sbd.vMaxVel.p.y = F1_0 * 60;
	if (!sbd.vMaxVel.p.z)
		sbd.vMaxVel.p.z = F1_0 * 60;
*/
	if (sbd.vMinVel.p.x > sbd.vMaxVel.p.x) {
		fix h = sbd.vMinVel.p.x;
		sbd.vMinVel.p.x = sbd.vMaxVel.p.x;
		sbd.vMaxVel.p.x = h;
		}
	if (sbd.vMinVel.p.y > sbd.vMaxVel.p.y) {
		fix h = sbd.vMinVel.p.y;
		sbd.vMinVel.p.y = sbd.vMaxVel.p.y;
		sbd.vMaxVel.p.y = h;
		}
	if (sbd.vMinVel.p.z > sbd.vMaxVel.p.z) {
		fix h = sbd.vMinVel.p.z;
		sbd.vMinVel.p.z = sbd.vMaxVel.p.z;
		sbd.vMaxVel.p.z = h;
		}
	objP->mType.physInfo.velocity = sbd.vVel;
	if (bSetOrient) {
		TriggerSetObjOrient (nObject, destSegnum, destSidenum, 0, -1);
		gameStates.gameplay.nDirSteps = MAX_ORIENT_STEPS - 1;
		}
	gameData.objs.speedBoost [nObject] = sbd;
	}
else {
	objP->mType.physInfo.velocity.p.x = objP->mType.physInfo.velocity.p.x / v * 60;
	objP->mType.physInfo.velocity.p.y = objP->mType.physInfo.velocity.p.y / v * 60;
	objP->mType.physInfo.velocity.p.z = objP->mType.physInfo.velocity.p.z / v * 60;
	}
}

//------------------------------------------------------------------------------

void UpdatePlayerOrient (void)
{
if (gameStates.app.b40fpsTick && gameStates.gameplay.nDirSteps)
	TriggerSetObjOrient (gameData.multi.players [gameData.multi.nLocalPlayer].nObject, -1, -1, 0, gameStates.gameplay.nDirSteps--);
}

//------------------------------------------------------------------------------

void DoSpeedBoost (tTrigger *trigP, short nObject)
{
if (COMPETITION || extraGameInfo [IsMultiGame].nSpeedBoost) {
	wall *w = TriggerParentWall (TRIG_IDX (trigP));
	gameData.objs.speedBoost [nObject].bBoosted = (trigP->value && (trigP->nLinks > 0));
	SetSpeedBoostVelocity ((short) nObject, trigP->value, 
								  (short) (w ? w->nSegment : -1), (short) (w ? w->nSide : -1),
								  trigP->nSegment [0], trigP->nSide [0], NULL, NULL, (trigP->flags & TF_SET_ORIENT) != 0);
	}
}

//------------------------------------------------------------------------------

extern void EnterSecretLevel (void);
extern void ExitSecretLevel (void);
extern int PSecretLevelDestroyed (void);

int WallIsForceField (tTrigger *trigP)
{
	int i;
	short *segs = trigP->nSegment;
	short *sides = trigP->nSide;

for (i = trigP->nLinks; i; i--, segs++, sides++)
	if ((gameData.pig.tex.pTMapInfo [gameData.segs.segments [*segs].sides [*sides].nBaseTex].flags & TMI_FORCE_FIELD))
		break;
return (i > 0);
}

//------------------------------------------------------------------------------

int CheckTriggerSub (short nObject, tTrigger *triggers, int nTriggerCount, int nTrigger, int nPlayer, int shot, int bBotTrigger)
{
	tTrigger	*trigP;
	tObject	*objP = gameData.objs.objects + nObject;
	ubyte		bIsPlayer = (objP->nType == OBJ_PLAYER);

if (nTrigger >= nTriggerCount)
	return 1;
trigP = triggers + nTrigger;
if (trigP->flags & TF_DISABLED)
	return 1;		//1 means don't send trigger hit to other players
if (bIsPlayer) {
	if (!IsMultiGame && (nObject != gameData.multi.players [gameData.multi.nLocalPlayer].nObject))
		return 1;
	}
else {
	nPlayer = -1;
	if ((trigP->nType != TT_TELEPORT) && (trigP->nType != TT_SPEEDBOOST)) {
		if (objP->nType != OBJ_ROBOT)
			return 1;
		if (!(bBotTrigger || gameData.bots.pInfo [objP->id].companion))
			return 1;
		}
	else
		if (objP->nType != OBJ_ROBOT)
			return 1;
		}
#if 1
if ((triggers == gameData.trigs.triggers) && 
	 (trigP->nType != TT_TELEPORT) && (trigP->nType != TT_SPEEDBOOST)) {
	long trigP = gameStates.app.nSDLTicks;
	if ((gameData.trigs.delay [nTrigger] >= 0) && (trigP - gameData.trigs.delay [nTrigger] < 750))
		return 1;
	gameData.trigs.delay [nTrigger] = trigP;
	}
#endif
if (trigP->flags & TF_ONE_SHOT)		//if this is a one-shot...
	trigP->flags |= TF_DISABLED;		//..then don'trigP let it happen again

switch (trigP->nType) {

	case TT_EXIT:
		if (nPlayer != gameData.multi.nLocalPlayer)
			break;
		DigiStopAll ();		//kill the sounds
		if ((gameData.missions.nCurrentLevel > 0) || gameStates.app.bD1Mission) {
			StartEndLevelSequence (0);
			} 
		else if (gameData.missions.nCurrentLevel < 0) {
			if ((gameData.multi.players [gameData.multi.nLocalPlayer].shields < 0) || 
					gameStates.app.bPlayerIsDead)
				break;
			ExitSecretLevel ();
			return 1;
			}
		else {
#ifdef EDITOR
				ExecMessageBox ("Yo!", 1, "You have hit the exit tTrigger!", "");
#else
				Int3 ();		//level num == 0, but no editor!
			#endif
			}
		return 1;
		break;

	case TT_SECRET_EXIT: {
		int	truth;

		if (nPlayer != gameData.multi.nLocalPlayer)
			break;
		if ((gameData.multi.players [gameData.multi.nLocalPlayer].shields < 0) || 
				gameStates.app.bPlayerIsDead)
			break;
		if (gameData.app.nGameMode & GM_MULTI) {
			HUDInitMessage (TXT_TELEPORT_MULTI);
			DigiPlaySample (SOUND_BAD_SELECTION, F1_0);
			break;
		}
		truth = PSecretLevelDestroyed ();

		if (gameData.demo.nState == ND_STATE_RECORDING)			// record whether we're really going to the secret level
			NDRecordSecretExitBlown (truth);

		if ((gameData.demo.nState != ND_STATE_PLAYBACK) && truth) {
			HUDInitMessage (TXT_SECRET_DESTROYED);
			DigiPlaySample (SOUND_BAD_SELECTION, F1_0);
			break;
		}

		if (gameData.demo.nState == ND_STATE_RECORDING)		// stop demo recording
			gameData.demo.nState = ND_STATE_PAUSED;
		DigiStopAll ();		//kill the sounds
		DigiPlaySample (SOUND_SECRET_EXIT, F1_0);
		GrPaletteFadeOut (NULL, 32, 0);
		EnterSecretLevel ();
		gameData.reactor.bDestroyed = 0;
		return 1;
		break;
	}

	case TT_OPEN_DOOR:
		DoLink (trigP);
		PrintTriggerMessage (nPlayer,nTrigger,shot,"Door%s opened!");
		break;

	case TT_CLOSE_DOOR:
		DoCloseDoor (trigP);
		PrintTriggerMessage (nPlayer,nTrigger,shot,"Door%s closed!");
		break;

	case TT_UNLOCK_DOOR:
		DoUnlockDoors (trigP);
		PrintTriggerMessage (nPlayer,nTrigger,shot,"Door%s unlocked!");
		break;

	case TT_LOCK_DOOR:
		DoLockDoors (trigP);
		PrintTriggerMessage (nPlayer,nTrigger,shot,"Door%s locked!");
		break;

	case TT_OPEN_WALL:
		if (DoChangeWalls (trigP))
		{
			if (WallIsForceField (trigP))
				PrintTriggerMessage (nPlayer,nTrigger,shot,"Force field%s deactivated!");
			else
				PrintTriggerMessage (nPlayer,nTrigger,shot,"Wall%s opened!");
		}
		break;

	case TT_CLOSE_WALL:
		if (DoChangeWalls (trigP))
		{
			if (WallIsForceField (trigP))
				PrintTriggerMessage (nPlayer,nTrigger,shot,"Force field%s activated!");
			else
				PrintTriggerMessage (nPlayer,nTrigger,shot,"Wall%s closed!");
		}
		break;

	case TT_ILLUSORY_WALL:
		//don'trigP know what to say, so say nothing
		DoChangeWalls (trigP);
		break;

	case TT_MATCEN:
		if (!(gameData.app.nGameMode & GM_MULTI) || (gameData.app.nGameMode & GM_MULTI_ROBOTS))
			DoMatCen (trigP);
		break;

	case TT_ILLUSION_ON:
		DoIllusionOn (trigP);
		PrintTriggerMessage (nPlayer,nTrigger,shot,"Illusion%s on!");
		break;

	case TT_ILLUSION_OFF:
		DoIllusionOff (trigP);
		PrintTriggerMessage (nPlayer,nTrigger,shot,"Illusion%s off!");
		break;

	case TT_LIGHT_OFF:
		if (DoLightOff (trigP))
			PrintTriggerMessage (nPlayer,nTrigger,shot,"Lights off!");
		break;

	case TT_LIGHT_ON:
		if (DoLightOn (trigP))
			PrintTriggerMessage (nPlayer,nTrigger,shot,"Lights on!");
		break;

	case TT_TELEPORT:
		if (bIsPlayer) {
			if (nPlayer != gameData.multi.nLocalPlayer)
				break;
			if ((gameData.multi.players [gameData.multi.nLocalPlayer].shields < 0) || 
					gameStates.app.bPlayerIsDead)
				break;
			}
		DigiPlaySample (SOUND_SECRET_EXIT, F1_0);
		DoTeleport (trigP, nObject);
		if (bIsPlayer)
			PrintTriggerMessage (nPlayer,nTrigger,shot,"Teleport!");
		break;

	case TT_SPEEDBOOST:
		if (bIsPlayer) {
			if (nPlayer != gameData.multi.nLocalPlayer)
				break;
			if ((gameData.multi.players [gameData.multi.nLocalPlayer].shields < 0) || 
				 gameStates.app.bPlayerIsDead)
				break;
			}
		DoSpeedBoost (trigP, nObject);
		if (bIsPlayer)
			PrintTriggerMessage (nPlayer,nTrigger, shot, "Speed Boost!");
		break;

	case TT_SHIELD_DAMAGE:
		gameData.multi.players [gameData.multi.nLocalPlayer].shields += gameData.trigs.triggers [nTrigger].value;
		break;

	case TT_ENERGY_DRAIN:
		gameData.multi.players [gameData.multi.nLocalPlayer].energy += gameData.trigs.triggers [nTrigger].value;
		break;

	default:
		Int3 ();
		break;
	}
if (trigP->flags & TF_ALTERNATE)
		trigP->nType = oppTrigTypes [trigP->nType];
return 0;
}

//------------------------------------------------------------------------------

void ExecObjTriggers (short nObject)
{
	short i = gameData.trigs.firstObjTrigger [nObject];

while (i >= 0) {
	CheckTriggerSub (nObject, gameData.trigs.objTriggers, gameData.trigs.nObjTriggers, i, -1, 1, 1);
	if (gameData.app.nGameMode & GM_MULTI)
		MultiSendObjTrigger (i);
	i = gameData.trigs.objTriggerRefs [i].next;
	}
}

//-----------------------------------------------------------------
// Checks for a tTrigger whenever an tObject hits a tTrigger nSide.
void CheckTrigger (tSegment *segP, short nSide, short nObject, int shot)
{
	int 		nWall;
	ubyte		nTrigger;	//, cnTrigger;
	tObject	*objP = gameData.objs.objects + nObject;

nWall = WallNumP (segP, nSide);
if (!IS_WALL (nWall)) 
	return;
nTrigger = gameData.walls.walls [nWall].nTrigger;
if (CheckTriggerSub (nObject, gameData.trigs.triggers, gameData.trigs.nTriggers, nTrigger, 
							(objP->nType == OBJ_PLAYER) ? objP->id : -1, shot, 0))
	return;
if (gameData.demo.nState == ND_STATE_RECORDING)
	NDRecordTrigger (SEG_IDX (segP), nSide, nObject, shot);
if (gameData.app.nGameMode & GM_MULTI)
	MultiSendTrigger (nTrigger, nObject);
}

//------------------------------------------------------------------------------

void TriggersFrameProcess ()
{
	int i;

for (i = 0; i < gameData.trigs.nTriggers; i++)
	if (gameData.trigs.triggers [i].time >= 0)
		gameData.trigs.triggers [i].time -= gameData.time.xFrame;
}

//------------------------------------------------------------------------------

static inline int TriggerHasTarget (tTrigger *triggerP, short nSegment, short nSide)
{
	int	i;
			
for (i = 0; i < triggerP->nLinks; i++)
	if ((triggerP->nSegment [i] == nSegment) && 
		 ((nSide < 0) || (triggerP->nSide [i] == nSide)))
	return 1;
return 0;
}

//------------------------------------------------------------------------------

int FindTriggerTarget (short nSegment, short nSide)
{
	int	i;

for (i = 0; i < gameData.trigs.nTriggers; i++)
	if (TriggerHasTarget (gameData.trigs.triggers + i, nSegment, nSide))
		return i + 1;
for (i = 0; i < gameData.trigs.nObjTriggers; i++)
	if (TriggerHasTarget (gameData.trigs.objTriggers + i, nSegment, nSide))
		return -i - 1;
return i;
}

//------------------------------------------------------------------------------

#ifndef FAST_FILE_IO
#if 0
	static char d2TriggerMap [10] = {
		TT_OPEN_DOOR,
		TT_SHIELD_DAMAGE,
		TT_ENERGY_DRAIN,
		TT_EXIT,
		-1,
		-1,
		TT_MATCEN,
		TT_ILLUSION_OFF,
		TT_ILLUSION_ON,
		TT_SECRET_EXIT
		};

	static char d2FlagMap [10] = {
		0,
		0,
		0,
		0,
		0,
		TF_ONE_SHOT,
		0,
		0,
		0,
		0
		};
#endif

/*
 * reads a v29_trigger structure from a CFILE
 */
extern void v29_trigger_read (v29_trigger *trigP, CFILE *fp)
{
	int	i;

trigP->nType = CFReadByte (fp);
trigP->flags = CFReadShort (fp);
trigP->value = CFReadFix (fp);
trigP->time = CFReadFix (fp);
trigP->link_num = CFReadByte (fp);
trigP->nLinks = CFReadShort (fp);
for (i=0; i<MAX_WALLS_PER_LINK; i++)
	trigP->nSegment [i] = CFReadShort (fp);
for (i=0; i<MAX_WALLS_PER_LINK; i++)
	trigP->nSide [i] = CFReadShort (fp);
/*
for (i = 0; i < 10; i++)
	if ((d2TriggerMap [i] >= 0) && (flags & (1 << i))) {
		trigP->nType = d2TriggerMap [i];
		break;
		}
trigP->flags = 0;
for (i = 0; i < 10; i++)
	if ((d2FlagMap [i] > 0) && (flags & (1 << i))) {
		trigP->flags |= d2FlagMap [i];
		break;
		}
*/
}

//------------------------------------------------------------------------------

/*
 * reads a v30_trigger structure from a CFILE
 */
extern void v30_trigger_read (v30_trigger *trigP, CFILE *fp)
{
	int i;

trigP->flags = CFReadShort (fp);
trigP->nLinks = CFReadByte (fp);
trigP->pad = CFReadByte (fp);
trigP->value = CFReadFix (fp);
trigP->time = CFReadFix (fp);
for (i=0; i<MAX_WALLS_PER_LINK; i++)
	trigP->nSegment [i] = CFReadShort (fp);
for (i=0; i<MAX_WALLS_PER_LINK; i++)
	trigP->nSide [i] = CFReadShort (fp);
}

//------------------------------------------------------------------------------

/*
 * reads a tTrigger structure from a CFILE
 */
extern void TriggerRead (tTrigger *trigP, CFILE *fp, int bObjTrigger)
{
	int i;

trigP->nType = CFReadByte (fp);
if (bObjTrigger)
	trigP->flags = (short) CFReadShort (fp);
else
	trigP->flags = (short) CFReadByte (fp);
trigP->nLinks = CFReadByte (fp);
CFReadByte (fp);
trigP->value = CFReadFix (fp);
trigP->time = CFReadFix (fp);
for (i = 0; i < MAX_WALLS_PER_LINK; i++)
	trigP->nSegment [i] = CFReadShort (fp);
for (i = 0; i < MAX_WALLS_PER_LINK; i++)
	trigP->nSide [i] = CFReadShort (fp);
}
#endif

//------------------------------------------------------------------------------
//eof
