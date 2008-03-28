/* $Id: fuelcen.c,v 1.8 2003/10/04 03:30:27 btb Exp $ */
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
static char rcsid [] = "$Id: fuelcen.c,v 1.8 2003/10/04 03:30:27 btb Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "inferno.h"
#include "fuelcen.h"
#include "gameseg.h"
#include "game.h"		// For gameData.time.xFrame
#include "error.h"
#include "mono.h"
#include "gauges.h"
#include "vclip.h"
#include "fireball.h"
#include "robot.h"
#include "powerup.h"

#include "wall.h"
#include "sounds.h"
#include "morph.h"
#include "3d.h"
#include "bm.h"
#include "polyobj.h"
#include "ai.h"
#include "gamemine.h"
#include "gamesave.h"
#include "player.h"
#include "collide.h"
#include "laser.h"
#include "network.h"
#include "multi.h"
#include "multibot.h"
#include "escort.h"
#include "dropobject.h"

// The max number of fuel stations per mine.

// Every time a robot is created in the morphing code, it decreases capacity of the morpher
// by this amount... when capacity gets to 0, no more morphers...

#define	ROBOT_GEN_TIME (i2f (5))
#define	EQUIP_GEN_TIME (i2f (3) * (gameStates.app.nDifficultyLevel + 1))

#define MATCEN_HP_DEFAULT			F1_0*500; // Hitpoints
#define MATCEN_INTERVAL_DEFAULT	F1_0*5;	//  5 seconds

#ifdef EDITOR
char	Special_names [MAX_CENTER_TYPES][11] = {
	"NOTHING   ",
	"FUELCEN   ",
	"REPAIRCEN ",
	"CONTROLCEN",
	"ROBOTMAKER",
	"GOAL_RED  ",
	"GOAL_BLUE ",
};
#endif

//------------------------------------------------------------
// Resets all fuel center info
void FuelCenReset ()
{
	int i;

for (i = 0; i < MAX_SEGMENTS; i++)
	gameData.segs.segment2s [i].special = SEGMENT_IS_NOTHING;
gameData.matCens.nFuelCenters = 0;
gameData.matCens.nBotCenters = 0;
gameData.matCens.nEquipCenters = 0;
gameData.matCens.nRepairCenters = 0;
}

#ifdef _DEBUG		//this is sometimes called by people from the debugger
void reset_allRobot_centers ()
{
	int i;

	// Remove all materialization centers
for (i = 0; i < gameData.segs.nSegments; i++)
	if (gameData.segs.segment2s [i].special == SEGMENT_IS_ROBOTMAKER) {
		gameData.segs.segment2s [i].special = SEGMENT_IS_NOTHING;
		gameData.segs.segment2s [i].nMatCen = -1;
		}
}
#endif

//------------------------------------------------------------
// Turns a tSegment into a fully charged up fuel center...
void FuelCenCreate (tSegment *segP, int oldType)
{
	short			nSegment = SEG_IDX (segP);
	tSegment2	*seg2P = gameData.segs.segment2s + nSegment;
	int			i, stationType = seg2P->special;

switch (stationType)	{
	case SEGMENT_IS_NOTHING:
	case SEGMENT_IS_GOAL_BLUE:
	case SEGMENT_IS_GOAL_RED:
	case SEGMENT_IS_TEAM_BLUE:
	case SEGMENT_IS_TEAM_RED:
	case SEGMENT_IS_WATER:
	case SEGMENT_IS_LAVA:
	case SEGMENT_IS_SPEEDBOOST:
	case SEGMENT_IS_BLOCKED:
	case SEGMENT_IS_NODAMAGE:
	case SEGMENT_IS_SKYBOX:
	case SEGMENT_IS_OUTDOOR:
		return;
	case SEGMENT_IS_FUELCEN:
	case SEGMENT_IS_REPAIRCEN:
	case SEGMENT_IS_CONTROLCEN:
	case SEGMENT_IS_ROBOTMAKER:
	case SEGMENT_IS_EQUIPMAKER:
		break;
	default:
		Error ("Segment %d has invalid\nstation nType %d in fuelcen.c\n", nSegment, stationType);
	}

Assert ((seg2P != NULL));
if (seg2P == NULL) 
	return;

switch (oldType) {
	case SEGMENT_IS_FUELCEN:
	case SEGMENT_IS_REPAIRCEN:
	case SEGMENT_IS_ROBOTMAKER:
	case SEGMENT_IS_EQUIPMAKER:
		i = seg2P->value;
		break;
	default:
		Assert (gameData.matCens.nFuelCenters < MAX_FUEL_CENTERS);
		i = gameData.matCens.nFuelCenters;
	}

seg2P->value = i;
gameData.matCens.fuelCenters [i].nType = stationType;
gameData.matCens.origStationTypes [i] = (oldType == stationType) ? SEGMENT_IS_NOTHING : oldType;
gameData.matCens.fuelCenters [i].xMaxCapacity = gameData.matCens.xFuelMaxAmount;
gameData.matCens.fuelCenters [i].xCapacity = gameData.matCens.fuelCenters [i].xMaxCapacity;
gameData.matCens.fuelCenters [i].nSegment = nSegment;
gameData.matCens.fuelCenters [i].xTimer = -1;
gameData.matCens.fuelCenters [i].bFlag = 0;
//	gameData.matCens.fuelCenters [i].NextRobotType = -1;
//	gameData.matCens.fuelCenters [i].last_created_obj=NULL;
//	gameData.matCens.fuelCenters [i].last_created_sig = -1;
COMPUTE_SEGMENT_CENTER (&gameData.matCens.fuelCenters [i].vCenter, segP);
if (oldType == SEGMENT_IS_NOTHING)
	gameData.matCens.nFuelCenters++;
if (oldType == SEGMENT_IS_EQUIPMAKER)
	gameData.matCens.nEquipCenters++;
else if (oldType == SEGMENT_IS_ROBOTMAKER) {
	gameData.matCens.origStationTypes [i] = SEGMENT_IS_NOTHING;
	i = seg2P->nMatCen;
	if (i < --gameData.matCens.nBotCenters)
		memcpy (gameData.matCens.botGens + i, 
				  gameData.matCens.botGens + i + 1, 
				  (gameData.matCens.nBotCenters - i) * sizeof (tFuelCenInfo));
	}
}

//------------------------------------------------------------
// Adds a matcen that already is a special nType into the gameData.matCens.fuelCenters array.
// This function is separate from other fuelcens because we don't want values reset.
void BotGenCreate (tSegment *segP, int oldType)
{
	short			nSegment = SEG_IDX (segP);
	tSegment2	*seg2P = gameData.segs.segment2s + nSegment;
	int			i, stationType = seg2P->special;

Assert (stationType == SEGMENT_IS_ROBOTMAKER);
Assert (gameData.matCens.nFuelCenters > -1);
if (seg2P->nMatCen >= gameFileInfo.botGen.count) {
	seg2P->special = SEGMENT_IS_NOTHING;
	seg2P->nMatCen = -1;
	return;
	}
switch (oldType) {
	case SEGMENT_IS_FUELCEN:
	case SEGMENT_IS_REPAIRCEN:
	case SEGMENT_IS_ROBOTMAKER:
		i = seg2P->value;
		break;
	default:
		Assert (gameData.matCens.nFuelCenters < MAX_FUEL_CENTERS);
		i = gameData.matCens.nFuelCenters;
	}
seg2P->value = i;
gameData.matCens.fuelCenters [i].nType = stationType;
gameData.matCens.origStationTypes [i] = (oldType == stationType) ? SEGMENT_IS_NOTHING : oldType;
gameData.matCens.fuelCenters [i].xCapacity = i2f (gameStates.app.nDifficultyLevel + 3);
gameData.matCens.fuelCenters [i].xMaxCapacity = gameData.matCens.fuelCenters [i].xCapacity;
gameData.matCens.fuelCenters [i].nSegment = nSegment;
gameData.matCens.fuelCenters [i].xTimer = -1;
gameData.matCens.fuelCenters [i].bFlag = 0;
COMPUTE_SEGMENT_CENTER_I (&gameData.matCens.fuelCenters [i].vCenter, seg2P - gameData.segs.segment2s);
i = gameData.matCens.nBotCenters++;
gameData.matCens.botGens [i].xHitPoints = MATCEN_HP_DEFAULT;
gameData.matCens.botGens [i].xInterval = MATCEN_INTERVAL_DEFAULT;
gameData.matCens.botGens [i].nSegment = nSegment;
if (oldType == SEGMENT_IS_NOTHING)
	gameData.matCens.botGens [i].nFuelCen = gameData.matCens.nFuelCenters;
gameData.matCens.nFuelCenters++;
}

//------------------------------------------------------------

void SetEquipGenStates (void)
{
	int	i;

for (i = 0; i < gameData.matCens.nEquipCenters; i++)
	gameData.matCens.fuelCenters [gameData.matCens.equipGens [i].nFuelCen].bEnabled = 
		FindTriggerTarget (gameData.matCens.fuelCenters [i].nSegment, -1) == 0;
}

//------------------------------------------------------------
// Adds a matcen that already is a special nType into the gameData.matCens.fuelCenters array.
// This function is separate from other fuelcens because we don't want values reset.
void EquipGenCreate (tSegment *segP, int oldType)
{
	short			nSegment = SEG_IDX (segP);
	tSegment2	*seg2P = gameData.segs.segment2s  + nSegment;
	int			stationType = seg2P->special;
	int			i;

Assert (stationType == SEGMENT_IS_EQUIPMAKER);
Assert (gameData.matCens.nFuelCenters > -1);
if (seg2P->nMatCen >= gameFileInfo.equipGen.count) {
	seg2P->special = SEGMENT_IS_NOTHING;
	seg2P->nMatCen = -1;
	return;
	}
switch (oldType) {
	case SEGMENT_IS_FUELCEN:
	case SEGMENT_IS_REPAIRCEN:
	case SEGMENT_IS_ROBOTMAKER:
	case SEGMENT_IS_EQUIPMAKER:
		i = seg2P->value;
		break;
	default:
		Assert (gameData.matCens.nFuelCenters < MAX_FUEL_CENTERS);
		i = gameData.matCens.nFuelCenters;
	}
seg2P->value = i;
gameData.matCens.fuelCenters [i].nType = stationType;
gameData.matCens.origStationTypes [i] = (oldType == stationType) ? SEGMENT_IS_NOTHING : oldType;
gameData.matCens.fuelCenters [i].xCapacity = i2f (gameStates.app.nDifficultyLevel + 3);
gameData.matCens.fuelCenters [i].xMaxCapacity = gameData.matCens.fuelCenters [i].xCapacity;
gameData.matCens.fuelCenters [i].nSegment = nSegment;
gameData.matCens.fuelCenters [i].xTimer = -1;
gameData.matCens.fuelCenters [i].bFlag = 0;
//gameData.matCens.fuelCenters [i].bEnabled = FindTriggerTarget (nSegment, -1) == 0;
COMPUTE_SEGMENT_CENTER_I (&gameData.matCens.fuelCenters [i].vCenter, nSegment);
//seg2P->nMatCen = gameData.matCens.nEquipCenters;
i = seg2P->nMatCen;
gameData.matCens.equipGens [i].xHitPoints = MATCEN_HP_DEFAULT;
gameData.matCens.equipGens [i].xInterval = MATCEN_INTERVAL_DEFAULT;
gameData.matCens.equipGens [i].nSegment = nSegment;
if (oldType == SEGMENT_IS_NOTHING)
	gameData.matCens.equipGens [i].nFuelCen = gameData.matCens.nFuelCenters;
if (gameData.matCens.nEquipCenters <= i)
	 gameData.matCens.nEquipCenters = i + 1;
gameData.matCens.nFuelCenters++;
}

//------------------------------------------------------------
// Adds a tSegment that already is a special nType into the gameData.matCens.fuelCenters array.
void FuelCenActivate (tSegment * segP, int stationType)
{
	tSegment2	*seg2P = gameData.segs.segment2s + SEG_IDX (segP);

seg2P->special = stationType;
if (seg2P->special == SEGMENT_IS_ROBOTMAKER)
	BotGenCreate (segP, SEGMENT_IS_NOTHING);
else if (seg2P->special == SEGMENT_IS_EQUIPMAKER)
	EquipGenCreate (segP, SEGMENT_IS_NOTHING);
else {
	FuelCenCreate (segP, SEGMENT_IS_NOTHING);
	if (seg2P->special == SEGMENT_IS_REPAIRCEN)
		seg2P->nMatCen = gameData.matCens.nRepairCenters++;
	}
}

//	The lower this number is, the more quickly the center can be re-triggered.
//	If it's too low, it can mean all the robots won't be put out, but for about 5
//	robots, that's not real likely.
#define	MATCEN_LIFE (i2f (30-2*gameStates.app.nDifficultyLevel))

//------------------------------------------------------------
//	Trigger (enable) the materialization center in tSegment nSegment
int MatCenTrigger (short nSegment)
{
	// -- tSegment		*segP = &gameData.segs.segments [nSegment];
	tSegment2		*seg2P = &gameData.segs.segment2s [nSegment];
	vmsVector		pos, delta;
	tFuelCenInfo	*matCenP;
	int				nObject;

#if TRACE
con_printf (CONDBG, "Trigger matcen, tSegment %i\n", nSegment);
#endif
if (seg2P->special == SEGMENT_IS_EQUIPMAKER) {
	matCenP = gameData.matCens.fuelCenters + gameData.matCens.equipGens [seg2P->nMatCen].nFuelCen;
	return (matCenP->bEnabled = !matCenP->bEnabled) ? 1 : 2;
	}
Assert (seg2P->special == SEGMENT_IS_ROBOTMAKER);
Assert (seg2P->nMatCen < gameData.matCens.nFuelCenters);
Assert ((seg2P->nMatCen >= 0) && (seg2P->nMatCen <= gameData.segs.nLastSegment));

matCenP = gameData.matCens.fuelCenters + gameData.matCens.botGens [seg2P->nMatCen].nFuelCen;
if (matCenP->bEnabled)
	return 0;
if (!matCenP->nLives)
	return 0;
//	MK: 11/18/95, At insane, matcens work forever!
if (gameStates.app.nDifficultyLevel+1 < NDL)
	matCenP->nLives--;

matCenP->xTimer = F1_0*1000;	//	Make sure the first robot gets emitted right away.
matCenP->bEnabled = 1;			//	Say this center is enabled, it can create robots.
matCenP->xCapacity = i2f (gameStates.app.nDifficultyLevel + 3);
matCenP->xDisableTime = MATCEN_LIFE;

//	Create a bright tObject in the tSegment.
pos = matCenP->vCenter;
VmVecSub (&delta, gameData.segs.vertices + gameData.segs.segments [nSegment].verts [0], &matCenP->vCenter);
VmVecScaleInc (&pos, &delta, F1_0/2);
nObject = CreateObject (OBJ_LIGHT, 0, -1, nSegment, &pos, NULL, 0, CT_LIGHT, MT_NONE, RT_NONE, 1);
if (nObject != -1) {
	gameData.objs.objects [nObject].lifeleft = MATCEN_LIFE;
	gameData.objs.objects [nObject].cType.lightInfo.intensity = i2f (8);	//	Light cast by a fuelcen.
	} 
else {
#if TRACE
	con_printf (1, "Can't create invisible flare for matcen.\n");
#endif
	Int3 ();
	}
return 0;
}

//------------------------------------------------------------
//	Trigger (enable) the materialization center in tSegment nSegment
void SpawnBotTrigger (tObject *objP, short nSegment)
{
	tSegment2		*seg2P = &gameData.segs.segment2s [nSegment];
	tFuelCenInfo	*matCenP;
	short				nType;

if (nSegment < 0)
	nType = 255;
else {
	Assert (seg2P->special == SEGMENT_IS_ROBOTMAKER);
	Assert (seg2P->nMatCen < gameData.matCens.nFuelCenters);
	Assert ((seg2P->nMatCen >= 0) && (seg2P->nMatCen <= gameData.segs.nLastSegment));
	matCenP = gameData.matCens.fuelCenters + gameData.matCens.botGens [seg2P->nMatCen].nFuelCen;
	nType = GetMatCenObjType (matCenP, gameData.matCens.botGens [seg2P->nMatCen].objFlags);
	if (nType < 0)
		nType = 255;
	}
BossSpewRobot (objP, NULL, nType, 1);
}

#ifdef EDITOR
//------------------------------------------------------------
// Takes away a tSegment's fuel center properties.
//	Deletes the tSegment point entry in the tFuelCenInfo list.
void FuelCenDelete (tSegment * segP)
{
	tSegment2	*seg2P = &gameData.segs.segment2s [SEG_IDX (segP)];
	int i, j;

Restart: ;

seg2P->special = 0;

for (i = 0; i < gameData.matCens.nFuelCenters; i++) {
	if (gameData.matCens.fuelCenters [i].nSegment == SEG_IDX (segP)) {
		// If Robot maker is deleted, fix gameData.segs.segments and gameData.matCens.botGens.
		if (gameData.matCens.fuelCenters [i].nType == SEGMENT_IS_ROBOTMAKER) {
			gameData.matCens.nBotCenters--;
			Assert (gameData.matCens.nBotCenters >= 0);
			for (j = seg2P->nMatCen; j < gameData.matCens.nBotCenters; j++)
				gameData.matCens.botGens [j] = gameData.matCens.botGens [j+1];
			for (j = 0; j < gameData.matCens.nFuelCenters; j++) {
				if (gameData.matCens.fuelCenters [j].nType == SEGMENT_IS_ROBOTMAKER)
					if (gameData.segs.segment2s [gameData.matCens.fuelCenters [j].nSegment].nMatCen > seg2P->nMatCen)
						gameData.segs.segment2s [gameData.matCens.fuelCenters [j].nSegment].nMatCen--;
				}
			}
		//fix gameData.matCens.botGens so they point to correct fuelcenter
		for (j = 0; j < gameData.matCens.nBotCenters; j++)
			if (gameData.matCens.botGens [j].nFuelCen > i)		//this matCenPter's fuelcen is changing
				gameData.matCens.botGens [j].nFuelCen--;

		gameData.matCens.nFuelCenters--;
		Assert (gameData.matCens.nFuelCenters >= 0);
		for (j = i; j < gameData.matCens.nFuelCenters; j++) {
			gameData.matCens.fuelCenters [j] = gameData.matCens.fuelCenters [j+1];
			gameData.segs.segment2s [gameData.matCens.fuelCenters [j].nSegment].value = j;
			}
		goto Restart;
		}
	}
}
#endif

//	----------------------------------------------------------------------------------------------------------

tObject *CreateMorphRobot (tSegment *segP, vmsVector *vObjPosP, ubyte object_id)
{
	short			nObject;
	tObject		*objP;
	tRobotInfo	*botInfoP;
	ubyte			default_behavior;

LOCALPLAYER.numRobotsLevel++;
LOCALPLAYER.numRobotsTotal++;
nObject = CreateObject ((ubyte) OBJ_ROBOT, object_id, -1, SEG_IDX (segP), vObjPosP,
								&vmdIdentityMatrix, gameData.models.polyModels [ROBOTINFO (object_id).nModel].rad,
				 				(ubyte) CT_AI, (ubyte) MT_PHYSICS, (ubyte) RT_POLYOBJ, 1);
if (nObject < 0) {
#if TRACE
	con_printf (1, "Can't create morph robot.  Aborting morph.\n");
#endif
	Int3 ();
	return NULL;
	}
objP = gameData.objs.objects + nObject;
//Set polygon-tObject-specific data
botInfoP = &ROBOTINFO (objP->id);
objP->rType.polyObjInfo.nModel = botInfoP->nModel;
objP->rType.polyObjInfo.nSubObjFlags = 0;
//set Physics info
objP->mType.physInfo.mass = botInfoP->mass;
objP->mType.physInfo.drag = botInfoP->drag;
objP->mType.physInfo.flags |= (PF_LEVELLING);
objP->shields = RobotDefaultShields (objP);
default_behavior = botInfoP->behavior;
InitAIObject (OBJ_IDX (objP), default_behavior, -1);		//	Note, -1 = tSegment this robot goes to to hide, should probably be something useful
CreateNSegmentPath (objP, 6, -1);		//	Create a 6 tSegment path from creation point.
gameData.ai.localInfo [nObject].mode = AIBehaviorToMode (default_behavior);
return objP;
}

int Num_extryRobots = 15;

#ifdef _DEBUG
int	FrameCount_last_msg = 0;
#endif

//	----------------------------------------------------------------------------------------------------------

void CreateMatCenEffect (tFuelCenInfo *matCenP, ubyte nVideoClip)
{
	vmsVector	vPos;
	tObject		*objP;

COMPUTE_SEGMENT_CENTER_I (&vPos, matCenP->nSegment);
// HACK!!!The 10 under here should be something equal to the 1/2 the size of the tSegment.
objP = ObjectCreateExplosion ((short) matCenP->nSegment, &vPos, i2f (10), nVideoClip);
if (objP) {
	ExtractOrientFromSegment (&objP->position.mOrient, gameData.segs.segments + matCenP->nSegment);
	if (gameData.eff.vClips [0][nVideoClip].nSound > -1)
		DigiLinkSoundToPos (gameData.eff.vClips [0][nVideoClip].nSound, (short) matCenP->nSegment,
								  0, &vPos, 0, F1_0);
	matCenP->bFlag	= 1;
	matCenP->xTimer = 0;
	}
}

//	----------------------------------------------------------------------------------------------------------

int GetMatCenObjType (tFuelCenInfo *matCenP, int *objFlags)
{
	int				i, nObjIndex, nTypes = 0;
	unsigned int	flags;
	sbyte				objTypes [64];

memset (objTypes, 0, sizeof (objTypes));
for (i = 0; i < 3; i++) {
	nObjIndex = i * 32;
	flags = (unsigned int) objFlags [i];
	while (flags) {
		if (flags & 1)
			objTypes [nTypes++] = nObjIndex;
		flags >>= 1;
		nObjIndex++;
		}
	}
if (!nTypes)
	return -1;
if (nTypes == 1)
	return objTypes [0];
return objTypes [(d_rand () * nTypes) / 327
];
}

//	----------------------------------------------------------------------------------------------------------

void EquipGenHandler (tFuelCenInfo * matCenP)
{
	int			nObject, nMatCen, nType;
	tObject		*objP;
	vmsVector	vPos; 
	fix			topTime;

if (!matCenP->bEnabled)
	return;
nMatCen = gameData.segs.segment2s [matCenP->nSegment].nMatCen;
if (nMatCen == -1) {
#if TRACE
	con_printf (CONDBG, "Dysfunctional robot generator at %d\n", matCenP->nSegment);
#endif
	return;
	}
matCenP->xTimer += gameData.time.xFrame;
if (!matCenP->bFlag) {
	topTime = EQUIP_GEN_TIME;
	if (matCenP->xTimer < topTime)
		return;
	nObject = gameData.segs.segments [matCenP->nSegment].objects;
	while (nObject >= 0) {
		objP = gameData.objs.objects + nObject;
		if ((objP->nType == OBJ_POWERUP) || (objP->id == OBJ_PLAYER)) {
			matCenP->xTimer = 0;
			return;
			}
		nObject = objP->next;
		}
	CreateMatCenEffect (matCenP, VCLIP_MORPHING_ROBOT);
	}
else if (matCenP->bFlag == 1) {			// Wait until 1/2 second after VCLIP started.
	if (matCenP->xTimer < (gameData.eff.vClips [0][VCLIP_MORPHING_ROBOT].xTotalTime / 2))
		return;
	matCenP->bFlag = 0;
	matCenP->xTimer = 0;
	nType = GetMatCenObjType (matCenP, gameData.matCens.equipGens [nMatCen].objFlags);
	if (nType < 0)
		return;
	COMPUTE_SEGMENT_CENTER_I (&vPos, matCenP->nSegment);
	// If this is the first materialization, set to valid robot.
	nObject = CreateObject (OBJ_POWERUP, nType, -1, (short) matCenP->nSegment, &vPos, &vmdIdentityMatrix, 
									gameData.objs.pwrUp.info [nType].size, 
									CT_POWERUP, MT_PHYSICS, RT_POWERUP, 1);
	if (nObject < 0)
		return;
	objP = gameData.objs.objects + nObject;
	if (IsMultiGame) {
		gameData.multiplayer.maxPowerupsAllowed [nType]++;
		gameData.multigame.create.nObjNums [gameData.multigame.create.nLoc++] = nObject;
		}
	objP->rType.vClipInfo.nClipIndex = gameData.objs.pwrUp.info [objP->id].nClipIndex;
	objP->rType.vClipInfo.xFrameTime = gameData.eff.vClips [0][objP->rType.vClipInfo.nClipIndex].xFrameTime;
	objP->rType.vClipInfo.nCurFrame = 0;
	objP->matCenCreator = gameData.segs.xSegments [matCenP->nSegment].owner;
	objP->lifeleft = IMMORTAL_TIME;
	}
else {
	matCenP->bFlag = 0;
	matCenP->xTimer = 0;
	}
}

//	----------------------------------------------------------------------------------------------------------

void VirusGenHandler (tFuelCenInfo * matCenP)
{
	int			nObject, nMatCen;
	tObject		*objP;
	vmsVector	vPos; 
	fix			topTime;

if (gameStates.entropy.bExitSequence || (gameData.segs.xSegments [matCenP->nSegment].owner <= 0))
	return;
nMatCen = gameData.segs.segment2s [matCenP->nSegment].nMatCen;
if (nMatCen == -1) {
#if TRACE
	con_printf (CONDBG, "Dysfunctional robot generator at %d\n", matCenP->nSegment);
#endif
	return;
	}
matCenP->xTimer += gameData.time.xFrame;
if (!matCenP->bFlag) {
	topTime = i2f (extraGameInfo [1].entropy.nVirusGenTime);
	if (matCenP->xTimer < topTime)
		return;
	nObject = gameData.segs.segments [matCenP->nSegment].objects;
	while (nObject >= 0) {
		objP = gameData.objs.objects + nObject;
		if ((objP->nType == OBJ_POWERUP) && (objP->id == POW_ENTROPY_VIRUS)) {
			matCenP->xTimer = 0;
			return;
			}
		nObject = objP->next;
		}
	CreateMatCenEffect (matCenP, VCLIP_POWERUP_DISAPPEARANCE);
	}
else if (matCenP->bFlag == 1) {			// Wait until 1/2 second after VCLIP started.
	if (matCenP->xTimer < (gameData.eff.vClips [0][VCLIP_POWERUP_DISAPPEARANCE].xTotalTime / 2))
		return;
	matCenP->bFlag = 0;
	matCenP->xTimer = 0;
	COMPUTE_SEGMENT_CENTER_I (&vPos, matCenP->nSegment);
	// If this is the first materialization, set to valid robot.
	nObject = CreateObject (OBJ_POWERUP, POW_ENTROPY_VIRUS, -1, (short) matCenP->nSegment, &vPos, &vmdIdentityMatrix, 
									gameData.objs.pwrUp.info [POW_ENTROPY_VIRUS].size, 
									CT_POWERUP, MT_PHYSICS, RT_POWERUP, 1);
	if (nObject >= 0) {
		objP = gameData.objs.objects + nObject;
		if (IsMultiGame)
			gameData.multigame.create.nObjNums [gameData.multigame.create.nLoc++] = nObject;
		objP->rType.vClipInfo.nClipIndex = gameData.objs.pwrUp.info [objP->id].nClipIndex;
		objP->rType.vClipInfo.xFrameTime = gameData.eff.vClips [0][objP->rType.vClipInfo.nClipIndex].xFrameTime;
		objP->rType.vClipInfo.nCurFrame = 0;
		objP->matCenCreator = gameData.segs.xSegments [matCenP->nSegment].owner;
		objP->lifeleft = IMMORTAL_TIME;
		}
	}
else {
	matCenP->bFlag = 0;
	matCenP->xTimer = 0;
	}
}

//	----------------------------------------------------------------------------------------------------------

inline int VertigoObjFlags (tMatCenInfo *miP)
{
return miP->objFlags [2] = gameData.objs.nVertigoBotFlags;
}

//	----------------------------------------------------------------------------------------------------------

void BotGenHandler (tFuelCenInfo * matCenP)
{
	fix			xDistToPlayer;
	vmsVector	vPos, vDir; 
	int			nMatCen, nSegment, nObject;
	tObject		*objP;
	fix			topTime;
	int			nType, nMyStation, nCount, i;

if (gameStates.gameplay.bNoBotAI)
	return;
if (!matCenP->bEnabled)
	return;
if (matCenP->xDisableTime > 0) {
	matCenP->xDisableTime -= gameData.time.xFrame;
	if (matCenP->xDisableTime <= 0) {
#if TRACE
		con_printf (CONDBG, "Robot center #%i gets disabled due to time running out.\n", 
						FUELCEN_IDX (matCenP));
#endif
		matCenP->bEnabled = 0;
		}
	}
//	No robot making in multiplayer mode.
if (IsMultiGame && (!(gameData.app.nGameMode & GM_MULTI_ROBOTS) || !NetworkIAmMaster ()))
	return;
// Wait until transmorgafier has capacity to make a robot...
if (matCenP->xCapacity <= 0)
	return;
nMatCen = gameData.segs.segment2s [matCenP->nSegment].nMatCen;
if (nMatCen == -1) {
#if TRACE
	con_printf (CONDBG, "Dysfunctional robot generator at %d\n", matCenP->nSegment);
#endif
	return;
	}
if (!(gameData.matCens.botGens [nMatCen].objFlags [0] ||
		gameData.matCens.botGens [nMatCen].objFlags [1] ||
		VertigoObjFlags (gameData.matCens.botGens + nMatCen)))
	return;

// Wait until we have a D2_FREE slot for this puppy...
if ((LOCALPLAYER.numRobotsLevel - 
	  LOCALPLAYER.numKillsLevel) >= 
	 (nGameSaveOrgRobots + Num_extryRobots)) {
#ifdef _DEBUG
	if (gameData.app.nFrameCount > FrameCount_last_msg + 20) {
#if TRACE
		con_printf (CONDBG, "Cannot morph until you kill one!\n");
#endif
		FrameCount_last_msg = gameData.app.nFrameCount;
		}
#endif
	return;
	}
matCenP->xTimer += gameData.time.xFrame;
if (!matCenP->bFlag) {
	if (IsMultiGame)
		topTime = ROBOT_GEN_TIME;
	else {
		xDistToPlayer = VmVecDistQuick (&gameData.objs.console->position.vPos, &matCenP->vCenter);
		topTime = xDistToPlayer / 64 + d_rand () * 2 + F1_0*2;
		if (topTime > ROBOT_GEN_TIME)
			topTime = ROBOT_GEN_TIME + d_rand ();
		if (topTime < F1_0*2)
			topTime = F1_0*3/2 + d_rand ()*2;
		}
	if (matCenP->xTimer < topTime)
		return;
	nMyStation = FUELCEN_IDX (matCenP);

	//	Make sure this robotmaker hasn't put out its max without having any of them killed.
	for (i = 0, nCount = 0; i <= gameData.objs.nLastObject; i++)
		if ((gameData.objs.objects [i].nType == OBJ_ROBOT) &&
			 ((gameData.objs.objects [i].matCenCreator ^ 0x80) == nMyStation))
			nCount++;
	if (nCount > gameStates.app.nDifficultyLevel + 3) {
#if TRACE
		con_printf (CONDBG, "Cannot morph: center %i has already put out %i robots.\n", nMyStation, nCount);
#endif
		matCenP->xTimer /= 2;
		return;
		}
		//	Whack on any robot or tPlayer in the matcen tSegment.
	nCount = 0;
	nSegment = matCenP->nSegment;
	for (nObject = gameData.segs.segments [nSegment].objects;
		  nObject != -1;
		  nObject = gameData.objs.objects [nObject].next) {
		nCount++;
		if (nCount > MAX_OBJECTS) {
#if TRACE
			con_printf (CONDBG, "Object list in tSegment %d is circular.", nSegment);
#endif
			Int3 ();
			return;
			}
		if (gameData.objs.objects [nObject].nType == OBJ_ROBOT) {
			CollideRobotAndMatCen (gameData.objs.objects + nObject);
			matCenP->xTimer = topTime / 2;
			return;
			}
		else if (gameData.objs.objects [nObject].nType == OBJ_PLAYER) {
			CollidePlayerAndMatCen (gameData.objs.objects + nObject);
			matCenP->xTimer = topTime / 2;
			return;
			}
		}
	CreateMatCenEffect (matCenP, VCLIP_MORPHING_ROBOT);
	}
else if (matCenP->bFlag == 1) {			// Wait until 1/2 second after VCLIP started.
	if (matCenP->xTimer <= (gameData.eff.vClips [0][VCLIP_MORPHING_ROBOT].xTotalTime / 2))
		return;
	matCenP->xCapacity -= gameData.matCens.xEnergyToCreateOneRobot;
	matCenP->bFlag = 0;
	matCenP->xTimer = 0;
	COMPUTE_SEGMENT_CENTER_I (&vPos, matCenP->nSegment);
	// If this is the first materialization, set to valid robot.
	nType = (int) GetMatCenObjType (matCenP, gameData.matCens.botGens [nMatCen].objFlags);
	if (nType < 0)
		return;
#if TRACE
	con_printf (CONDBG, "Morph: (nType = %i) (seg = %i) (capacity = %08x)\n", nType, matCenP->nSegment, matCenP->xCapacity);
#endif
	if (!(objP = CreateMorphRobot (gameData.segs.segments + matCenP->nSegment, &vPos, nType))) {
#if TRACE
		con_printf (CONDBG, "Warning: CreateMorphRobot returned NULL (no gameData.objs.objects left?)\n");
#endif
		return;
		}
	if (IsMultiGame)
		MultiSendCreateRobot (FUELCEN_IDX (matCenP), OBJ_IDX (objP), nType);
	objP->matCenCreator = (FUELCEN_IDX (matCenP)) | 0x80;
	// Make object face player...
	VmVecSub (&vDir, &gameData.objs.console->position.vPos, &objP->position.vPos);
	VmVector2Matrix (&objP->position.mOrient, &vDir, &objP->position.mOrient.uVec, NULL);
	MorphStart (objP);
	}
else {
	matCenP->bFlag = 0;
	matCenP->xTimer = 0;
	}
}


//-------------------------------------------------------------
// Called once per frame, replenishes fuel supply.
void FuelcenUpdateAll ()
{
	int				i, t;
	tFuelCenInfo	*fuelCenP = gameData.matCens.fuelCenters;
	fix xAmountToReplenish = FixMul (gameData.time.xFrame,gameData.matCens.xFuelRefillSpeed);

for (i = 0; i < gameData.matCens.nFuelCenters; i++, fuelCenP++) {
	t = fuelCenP->nType;
	if (t == SEGMENT_IS_ROBOTMAKER) {
		if (IsMultiGame && (gameData.app.nGameMode & GM_ENTROPY))
			VirusGenHandler (gameData.matCens.fuelCenters + i);
		else if (!(gameStates.app.bGameSuspended & SUSP_ROBOTS))
			BotGenHandler (gameData.matCens.fuelCenters + i);
		}
	else if (t == SEGMENT_IS_EQUIPMAKER) {
		if (!(gameStates.app.bGameSuspended & SUSP_ROBOTS))
			EquipGenHandler (gameData.matCens.fuelCenters + i);
		}
	else if (t == SEGMENT_IS_CONTROLCEN) {
		//controlcen_proc (gameData.matCens.fuelCenters + i);
		}
	else if ((fuelCenP->xMaxCapacity > 0) && 
				(gameData.matCens.playerSegP != gameData.segs.segments + fuelCenP->nSegment)) {
		if (fuelCenP->xCapacity < fuelCenP->xMaxCapacity) {
 			fuelCenP->xCapacity += xAmountToReplenish;
			if (fuelCenP->xCapacity >= fuelCenP->xMaxCapacity) {
				fuelCenP->xCapacity = fuelCenP->xMaxCapacity;
				//gauge_message ("Fuel center is fully recharged!   ");
				}
			}
		}
	}
}

//--unused-- //-------------------------------------------------------------
//--unused-- // replenishes all fuel supplies.
//--unused-- void FuelCenReplenishAll ()
//--unused-- {
//--unused-- 	int i;
//--unused--
//--unused-- 	for (i=0; i<gameData.matCens.nFuelCenters; i++)	{
//--unused-- 		gameData.matCens.fuelCenters [i].xCapacity = gameData.matCens.fuelCenters [i].xMaxCapacity;
//--unused-- 	}
//--unused--
//--unused-- }

#define FUELCEN_SOUND_DELAY (f1_0/4)		//play every half second

//-------------------------------------------------------------

fix HostileRoomDamageShields (tSegment *segP, fix MaxAmountCanGive)
{
	static fix last_playTime=0;
	fix amount;
	xsegment	*xsegP;

if (!(gameData.app.nGameMode & GM_ENTROPY))
	return 0;
Assert (segP != NULL);
gameData.matCens.playerSegP = segP;
if (!segP)
	return 0;
xsegP = gameData.segs.xSegments + SEG_IDX (segP);
if ((xsegP->owner < 1) || (xsegP->owner == GetTeam (gameData.multiplayer.nLocalPlayer) + 1))
	return 0;
amount = FixMul (gameData.time.xFrame, extraGameInfo [1].entropy.nShieldDamageRate * F1_0);
if (amount > MaxAmountCanGive)
	amount = MaxAmountCanGive;
if (last_playTime > gameData.time.xGame)
	last_playTime = 0;
if (gameData.time.xGame > last_playTime+FUELCEN_SOUND_DELAY) {
	DigiPlaySample (SOUND_PLAYER_GOT_HIT, F1_0/2);
	if (IsMultiGame)
		MultiSendPlaySound (SOUND_PLAYER_GOT_HIT, F1_0/2);
	last_playTime = gameData.time.xGame;
	}
return amount;
}

//-------------------------------------------------------------

fix FuelCenGiveFuel (tSegment *segP, fix MaxAmountCanTake)
{
	short			nSegment = SEG_IDX (segP);
	tSegment2	*seg2P = gameData.segs.segment2s + nSegment;
	xsegment		*xsegp = gameData.segs.xSegments + nSegment;
	fix			amount;

	static fix last_playTime = 0;

Assert (segP != NULL);
gameData.matCens.playerSegP = segP;
if ((gameData.app.nGameMode & GM_ENTROPY) && ((xsegp->owner < 0) || 
	 ((xsegp->owner > 0) && (xsegp->owner != GetTeam (gameData.multiplayer.nLocalPlayer) + 1))))
	return 0;
if (!segP || (seg2P->special != SEGMENT_IS_FUELCEN))
	return 0;
DetectEscortGoalAccomplished (-4);	//	UGLY!Hack!-4 means went through fuelcen.
#if 0
if (gameData.matCens.fuelCenters [segP->value].xMaxCapacity <= 0) {
	HUDInitMessage ("Fuelcenter %d is destroyed.", segP->value);
	return 0;
	}
if (gameData.matCens.fuelCenters [segP->value].xCapacity <= 0) {
	HUDInitMessage ("Fuelcenter %d is empty.", segP->value);
	return 0;
	}
#endif
if (MaxAmountCanTake <= 0)
	return 0;
if (gameData.app.nGameMode & GM_ENTROPY)
	amount = FixMul (gameData.time.xFrame, gameData.matCens.xFuelGiveAmount * F1_0);
else
	amount = FixMul (gameData.time.xFrame, gameData.matCens.xFuelGiveAmount);
if (amount > MaxAmountCanTake)
	amount = MaxAmountCanTake;
if (last_playTime > gameData.time.xGame)
	last_playTime = 0;
if (gameData.time.xGame > last_playTime + FUELCEN_SOUND_DELAY) {
	DigiPlaySample (SOUND_REFUEL_STATION_GIVING_FUEL, F1_0 / 2);
	if (IsMultiGame)
		MultiSendPlaySound (SOUND_REFUEL_STATION_GIVING_FUEL, F1_0 / 2);
	last_playTime = gameData.time.xGame;
	}
//HUDInitMessage ("Fuelcen %d has %d/%d fuel", segP->value,f2i (gameData.matCens.fuelCenters [segP->value].xCapacity),f2i (gameData.matCens.fuelCenters [segP->value].xMaxCapacity));
return amount;
}

//-------------------------------------------------------------
// DM/050904
// Repair centers
// use same values as fuel centers
fix RepairCenGiveShields (tSegment *segP, fix MaxAmountCanTake)
{
	short		nSegment = SEG_IDX (segP);
	tSegment2	*seg2P = gameData.segs.segment2s + nSegment;
	xsegment	*xsegp = gameData.segs.xSegments + nSegment;
	static fix last_playTime=0;
	fix amount;

if (gameOpts->legacy.bFuelCens)
	return 0;
Assert (segP != NULL);
if (!segP)
	return 0;
gameData.matCens.playerSegP = segP;
if ((gameData.app.nGameMode & GM_ENTROPY) && ((xsegp->owner < 0) || 
	 ((xsegp->owner > 0) && (xsegp->owner != GetTeam (gameData.multiplayer.nLocalPlayer) + 1))))
	return 0;
if (seg2P->special != SEGMENT_IS_REPAIRCEN)
	return 0;
//		DetectEscortGoalAccomplished (-4);	//	UGLY!Hack!-4 means went through fuelcen.
//		if (gameData.matCens.fuelCenters [segP->value].xMaxCapacity<=0)	{
//			HUDInitMessage ("Repaircenter %d is destroyed.", segP->value);
//			return 0;
//		}
//		if (gameData.matCens.fuelCenters [segP->value].xCapacity<=0)	{
//			HUDInitMessage ("Repaircenter %d is empty.", segP->value);
//			return 0;
//		}
if (MaxAmountCanTake <= 0)	{
	return 0;
}
amount = FixMul (gameData.time.xFrame, extraGameInfo [IsMultiGame].entropy.nShieldFillRate * F1_0);
if (amount > MaxAmountCanTake)
	amount = MaxAmountCanTake;
if (last_playTime > gameData.time.xGame)
	last_playTime = 0;
if (gameData.time.xGame > last_playTime + FUELCEN_SOUND_DELAY) {
	DigiPlaySample (SOUND_REFUEL_STATION_GIVING_FUEL, F1_0/2);
	if (IsMultiGame)
		MultiSendPlaySound (SOUND_REFUEL_STATION_GIVING_FUEL, F1_0/2);
	last_playTime = gameData.time.xGame;
	}
return amount;
}

//--unused-- //-----------------------------------------------------------
//--unused-- // Damages a fuel center
//--unused-- void FuelCenDamage (tSegment *segP, fix damage)
//--unused-- {
//--unused-- 	//int i;
//--unused-- 	// int	station_num = segP->value;
//--unused--
//--unused-- 	Assert (segP != NULL);
//--unused-- 	if (segP == NULL) return;
//--unused--
//--unused-- 	switch (segP->special)	{
//--unused-- 	case SEGMENT_IS_NOTHING:
//--unused-- 		return;
//--unused-- 	case SEGMENT_IS_ROBOTMAKER:
//--unused-- //--		// Robotmaker hit by laser
//--unused-- //--		if (gameData.matCens.fuelCenters [station_num].xMaxCapacity<=0)	{
//--unused-- //--			// Shooting a already destroyed materializer
//--unused-- //--		} else {
//--unused-- //--			gameData.matCens.fuelCenters [station_num].xMaxCapacity -= damage;
//--unused-- //--			if (gameData.matCens.fuelCenters [station_num].xCapacity > gameData.matCens.fuelCenters [station_num].xMaxCapacity)	{
//--unused-- //--				gameData.matCens.fuelCenters [station_num].xCapacity = gameData.matCens.fuelCenters [station_num].xMaxCapacity;
//--unused-- //--			}
//--unused-- //--			if (gameData.matCens.fuelCenters [station_num].xMaxCapacity <= 0)	{
//--unused-- //--				gameData.matCens.fuelCenters [station_num].xMaxCapacity = 0;
//--unused-- //--				// Robotmaker dead
//--unused-- //--				for (i=0; i<6; i++)
//--unused-- //--					segP->sides [i].nOvlTex = 0;
//--unused-- //--			}
//--unused-- //--		}
//--unused-- 		break;
//--unused-- 	case SEGMENT_IS_FUELCEN:
//--unused-- //--		DigiPlaySample (SOUND_REFUEL_STATION_HIT);
//--unused-- //--		if (gameData.matCens.fuelCenters [station_num].xMaxCapacity>0)	{
//--unused-- //--			gameData.matCens.fuelCenters [station_num].xMaxCapacity -= damage;
//--unused-- //--			if (gameData.matCens.fuelCenters [station_num].xCapacity > gameData.matCens.fuelCenters [station_num].xMaxCapacity)	{
//--unused-- //--				gameData.matCens.fuelCenters [station_num].xCapacity = gameData.matCens.fuelCenters [station_num].xMaxCapacity;
//--unused-- //--			}
//--unused-- //--			if (gameData.matCens.fuelCenters [station_num].xMaxCapacity <= 0)	{
//--unused-- //--				gameData.matCens.fuelCenters [station_num].xMaxCapacity = 0;
//--unused-- //--				DigiPlaySample (SOUND_REFUEL_STATION_DESTROYED);
//--unused-- //--			}
//--unused-- //--		} else {
//--unused-- //--			gameData.matCens.fuelCenters [station_num].xMaxCapacity = 0;
//--unused-- //--		}
//--unused-- //--		HUDInitMessage ("Fuelcenter %d damaged", station_num);
//--unused-- 		break;
//--unused-- 	case SEGMENT_IS_REPAIRCEN:
//--unused-- 		break;
//--unused-- 	case SEGMENT_IS_CONTROLCEN:
//--unused-- 		break;
//--unused-- 	default:
//--unused-- 		Error ("Invalid nType in fuelcen.c");
//--unused-- 	}
//--unused-- }

//--unused-- //	----------------------------------------------------------------------------------------------------------
//--unused-- fixang my_delta_ang (fixang a,fixang b)
//--unused-- {
//--unused-- 	fixang delta0,delta1;
//--unused--
//--unused-- 	return (abs (delta0 = a - b) < abs (delta1 = b - a)) ? delta0 : delta1;
//--unused--
//--unused-- }

//--unused-- //	----------------------------------------------------------------------------------------------------------
//--unused-- //return though which tSide of seg0 is seg1
//--unused-- int john_find_connect_side (int seg0,int seg1)
//--unused-- {
//--unused-- 	tSegment *Seg=&gameData.segs.segments [seg0];
//--unused-- 	int i;
//--unused--
//--unused-- 	for (i=MAX_SIDES_PER_SEGMENT;i--;) if (Seg->children [i]==seg1) return i;
//--unused--
//--unused-- 	return -1;
//--unused-- }

//	----------------------------------------------------------------------------------------------------------
//--unused-- vmsAngVec start_angles, deltaAngles, goalAngles;
//--unused-- vmsVector start_pos, delta_pos, goal_pos;
//--unused-- int FuelStationSeg;
//--unused-- fix currentTime,deltaTime;
//--unused-- int next_side, side_index;
//--unused-- int * sidelist;

//--repair-- int Repairing;
//--repair-- vmsVector repair_save_uvec;		//the tPlayer's upvec when enter repaircen
//--repair-- tObject *RepairObj=NULL;		//which tObject getting repaired
//--repair-- int disable_repair_center=0;
//--repair-- fix repair_rate;
//--repair-- #define FULL_REPAIR_RATE i2f (10)

//--unused-- ubyte save_controlType,save_movementType;

//--unused-- int SideOrderBack [] = {WFRONT, WRIGHT, WTOP, WLEFT, WBOTTOM, WBACK};
//--unused-- int SideOrderFront [] =  {WBACK, WLEFT, WTOP, WRIGHT, WBOTTOM, WFRONT};
//--unused-- int SideOrderLeft [] =  { WRIGHT, WBACK, WTOP, WFRONT, WBOTTOM, WLEFT };
//--unused-- int SideOrderRight [] =  { WLEFT, WFRONT, WTOP, WBACK, WBOTTOM, WRIGHT };
//--unused-- int SideOrderTop [] =  { WBOTTOM, WLEFT, WBACK, WRIGHT, WFRONT, WTOP };
//--unused-- int SideOrderBottom [] =  { WTOP, WLEFT, WFRONT, WRIGHT, WBACK, WBOTTOM };

//--unused-- int SideUpVector [] = {WBOTTOM, WFRONT, WBOTTOM, WFRONT, WBOTTOM, WBOTTOM };

//--repair-- //	----------------------------------------------------------------------------------------------------------
//--repair-- void refuel_calc_deltas (tObject *objP, int next_side, int repair_seg)
//--repair-- {
//--repair-- 	vmsVector nextcenter, headfvec, *headuvec;
//--repair-- 	vmsMatrix goal_orient;
//--repair--
//--repair-- 	// Find time for this movement
//--repair-- 	deltaTime = F1_0;		// one second...
//--repair-- 	
//--repair-- 	// Find start and goal position
//--repair-- 	start_pos = objP->position.vPos;
//--repair-- 
//--repair-- 	// Find delta position to get to goal position
//--repair-- 	COMPUTE_SEGMENT_CENTER (&goal_pos,&gameData.segs.segments [repair_seg]);
//--repair-- 	VmVecSub (&delta_pos,&goal_pos,&start_pos);
//--repair-- 
//--repair-- 	// Find start angles
//--repair-- 	//angles_from_vector (&start_angles,&objP->position.mOrient.fVec);
//--repair-- 	VmExtractAnglesMatrix (&start_angles,&objP->position.mOrient);
//--repair-- 
//--repair-- 	// Find delta angles to get to goal orientation
//--repair-- 	med_compute_center_point_on_side (&nextcenter,&gameData.segs.segments [repair_seg],next_side);
//--repair-- 	VmVecSub (&headfvec,&nextcenter,&goal_pos);
//--repair--
//--repair-- 	if (next_side == 5)						//last tSide
//--repair-- 		headuvec = &repair_save_uvec;
//--repair-- 	else
//--repair-- 		headuvec = &gameData.segs.segments [repair_seg].sides [SideUpVector [next_side]].normals [0];
//--repair--
//--repair-- 	VmVector2Matrix (&goal_orient,&headfvec,headuvec,NULL);
//--repair-- 	VmExtractAnglesMatrix (&goalAngles,&goal_orient);
//--repair-- 	deltaAngles.p = my_delta_ang (start_angles.p,goalAngles.p);
//--repair-- 	deltaAngles.b = my_delta_ang (start_angles.b,goalAngles.b);
//--repair-- 	deltaAngles.h = my_delta_ang (start_angles.h,goalAngles.h);
//--repair-- 	currentTime = 0;
//--repair-- 	Repairing = 0;
//--repair-- }
//--repair--
//--repair-- //	----------------------------------------------------------------------------------------------------------
//--repair-- //if repairing, cut it short
//--repair-- abort_repair_center ()
//--repair-- {
//--repair-- 	if (!RepairObj || side_index==5)
//--repair-- 		return;
//--repair--
//--repair-- 	currentTime = 0;
//--repair-- 	side_index = 5;
//--repair-- 	next_side = sidelist [side_index];
//--repair-- 	refuel_calc_deltas (RepairObj, next_side, FuelStationSeg);
//--repair-- }
//--repair--
//--repair-- //	----------------------------------------------------------------------------------------------------------
//--repair-- void repair_ship_damage ()
//--repair-- {
//--repair-- }
//--repair--
//--repair-- //	----------------------------------------------------------------------------------------------------------
//--repair-- int refuel_do_repair_effect (tObject * objP, int firstTime, int repair_seg)	{
//--repair--
//--repair-- 	objP->mType.physInfo.velocity.x = 0;			
//--repair-- 	objP->mType.physInfo.velocity.y = 0;			
//--repair-- 	objP->mType.physInfo.velocity.z = 0;			
//--repair--
//--repair-- 	if (firstTime)	{
//--repair-- 		int entry_side;
//--repair-- 		currentTime = 0;
//--repair--
//--repair-- 		DigiPlaySample (SOUND_REPAIR_STATION_PLAYER_ENTERING, F1_0);
//--repair--
//--repair-- 		entry_side = john_find_connect_side (repair_seg,objP->nSegment);
//--repair-- 		Assert (entry_side > -1);
//--repair--
//--repair-- 		switch (entry_side)	{
//--repair-- 		case WBACK: sidelist = SideOrderBack; break;
//--repair-- 		case WFRONT: sidelist = SideOrderFront; break;
//--repair-- 		case WLEFT: sidelist = SideOrderLeft; break;
//--repair-- 		case WRIGHT: sidelist = SideOrderRight; break;
//--repair-- 		case WTOP: sidelist = SideOrderTop; break;
//--repair-- 		case WBOTTOM: sidelist = SideOrderBottom; break;
//--repair-- 		}
//--repair-- 		side_index = 0;
//--repair-- 		next_side = sidelist [side_index];
//--repair--
//--repair-- 		refuel_calc_deltas (objP,next_side, repair_seg);
//--repair-- 	}
//--repair--
//--repair-- 	//update shields
//--repair-- 	if (LOCALPLAYER.shields < MAX_SHIELDS) {	//if above max, don't mess with it
//--repair--
//--repair-- 		LOCALPLAYER.shields += FixMul (gameData.time.xFrame,repair_rate);
//--repair--
//--repair-- 		if (LOCALPLAYER.shields > MAX_SHIELDS)
//--repair-- 			LOCALPLAYER.shields = MAX_SHIELDS;
//--repair-- 	}
//--repair--
//--repair-- 	currentTime += gameData.time.xFrame;
//--repair--
//--repair-- 	if (currentTime >= deltaTime)	{
//--repair-- 		vmsAngVec av;
//--repair-- 		objP->position.vPos = goal_pos;
//--repair-- 		av	= goalAngles;
//--repair-- 		VmAngles2Matrix (&objP->position.mOrient,&av);
//--repair--
//--repair-- 		if (side_index >= 5)
//--repair-- 			return 1;		// Done being repaired...
//--repair--
//--repair-- 		if (Repairing==0)		{
//--repair-- 			//DigiPlaySample (SOUND_REPAIR_STATION_FIXING);
//--repair-- 			Repairing=1;
//--repair--
//--repair-- 			switch (next_side)	{
//--repair-- 			case 0:	DigiPlaySample (SOUND_REPAIR_STATION_FIXING_1,F1_0); break;
//--repair-- 			case 1:	DigiPlaySample (SOUND_REPAIR_STATION_FIXING_2,F1_0); break;
//--repair-- 			case 2:	DigiPlaySample (SOUND_REPAIR_STATION_FIXING_3,F1_0); break;
//--repair-- 			case 3:	DigiPlaySample (SOUND_REPAIR_STATION_FIXING_4,F1_0); break;
//--repair-- 			case 4:	DigiPlaySample (SOUND_REPAIR_STATION_FIXING_1,F1_0); break;
//--repair-- 			case 5:	DigiPlaySample (SOUND_REPAIR_STATION_FIXING_2,F1_0); break;
//--repair-- 			}
//--repair-- 	
//--repair-- 			repair_ship_damage ();
//--repair--
//--repair-- 		}
//--repair--
//--repair-- 		if (currentTime >= (deltaTime+ (F1_0/2)))	{
//--repair-- 			currentTime = 0;
//--repair-- 			// Find next tSide...
//--repair-- 			side_index++;
//--repair-- 			if (side_index >= 6) return 1;
//--repair-- 			next_side = sidelist [side_index];
//--repair-- 
//--repair-- 			refuel_calc_deltas (objP, next_side, repair_seg);
//--repair-- 		}
//--repair--
//--repair-- 	} else {
//--repair-- 		fix factor, p,b,h;
//--repair-- 		vmsAngVec av;
//--repair--
//--repair-- 		factor = FixDiv (currentTime,deltaTime);
//--repair--
//--repair-- 		// Find tObject's current position
//--repair-- 		objP->position.vPos = delta_pos;
//--repair-- 		VmVecScale (&objP->position.vPos, factor);
//--repair-- 		VmVecInc (&objP->position.vPos, &start_pos);
//--repair-- 		
//--repair-- 		// Find tObject's current orientation
//--repair-- 		p	= FixMul (deltaAngles.p,factor);
//--repair-- 		b	= FixMul (deltaAngles.b,factor);
//--repair-- 		h	= FixMul (deltaAngles.h,factor);
//--repair-- 		av.p = (fixang)p + start_angles.p;
//--repair-- 		av.b = (fixang)b + start_angles.b;
//--repair-- 		av.h = (fixang)h + start_angles.h;
//--repair-- 		VmAngles2Matrix (&objP->position.mOrient,&av);
//--repair--
//--repair-- 	}
//--repair--
//--repair-- 	UpdateObjectSeg (objP);		//update tSegment
//--repair--
//--repair-- 	return 0;
//--repair-- }
//--repair--
//--repair-- //	----------------------------------------------------------------------------------------------------------
//--repair-- //do the repair center for this frame
//--repair-- void do_repair_sequence (tObject *objP)
//--repair-- {
//--repair-- 	Assert (obj == RepairObj);
//--repair--
//--repair-- 	if (refuel_do_repair_effect (objP, 0, FuelStationSeg)) {
//--repair-- 		if (LOCALPLAYER.shields < MAX_SHIELDS)
//--repair-- 			LOCALPLAYER.shields = MAX_SHIELDS;
//--repair-- 		objP->controlType = save_controlType;
//--repair-- 		objP->movementType = save_movementType;
//--repair-- 		disable_repair_center=1;
//--repair-- 		RepairObj = NULL;
//--repair--
//--repair--
//--repair-- 		//the two lines below will spit the tPlayer out of the rapair center,
//--repair-- 		//but what happen is that the ship just bangs into the door
//--repair-- 		//if (objP->movementType == MT_PHYSICS)
//--repair-- 		//	VmVecCopyScale (&objP->mType.physInfo.velocity,&objP->position.mOrient.fVec,i2f (200);
//--repair-- 	}
//--repair--
//--repair-- }
//--repair--
//--repair-- //	----------------------------------------------------------------------------------------------------------
//--repair-- //see if we should start the repair center
//--repair-- void check_start_repair_center (tObject *objP)
//--repair-- {
//--repair-- 	if (RepairObj != NULL) return;		//already in repair center
//--repair--
//--repair-- 	if (Lsegments [objP->nSegment].specialType & SS_REPAIR_CENTER) {
//--repair--
//--repair-- 		if (!disable_repair_center) {
//--repair-- 			//have just entered repair center
//--repair--
//--repair-- 			RepairObj = obj;
//--repair-- 			repair_save_uvec = objP->position.mOrient.uVec;
//--repair--
//--repair-- 			repair_rate = FixMulDiv (FULL_REPAIR_RATE, (MAX_SHIELDS - LOCALPLAYER.shields),MAX_SHIELDS);
//--repair--
//--repair-- 			save_controlType = objP->controlType;
//--repair-- 			save_movementType = objP->movementType;
//--repair--
//--repair-- 			objP->controlType = CT_REPAIRCEN;
//--repair-- 			objP->movementType = MT_NONE;
//--repair--
//--repair-- 			FuelStationSeg	= Lsegments [objP->nSegment].special_segment;
//--repair-- 			Assert (FuelStationSeg != -1);
//--repair--
//--repair-- 			if (refuel_do_repair_effect (objP, 1, FuelStationSeg)) {
//--repair-- 				Int3 ();		//can this happen?
//--repair-- 				objP->controlType = CT_FLYING;
//--repair-- 				objP->movementType = MT_PHYSICS;
//--repair-- 			}
//--repair-- 		}
//--repair-- 	}
//--repair-- 	else
//--repair-- 		disable_repair_center=0;
//--repair--
//--repair-- }

//	--------------------------------------------------------------------------------------------

void DisableMatCens (void)
{
	int	i;

for (i = 0; i < gameData.matCens.nBotCenters; i++) {
	if (gameData.matCens.fuelCenters [i].nType != SEGMENT_IS_EQUIPMAKER) {
		gameData.matCens.fuelCenters [i].bEnabled = 0;
		gameData.matCens.fuelCenters [i].xDisableTime = 0;
		}
	}
}

//	--------------------------------------------------------------------------------------------
//	Initialize all materialization centers.
//	Give them all the right number of lives.
void InitAllMatCens (void)
{
	int	i;
#ifdef _DEBUG
	int	j, nFuelCen;
#endif

for (i = 0; i < gameData.matCens.nFuelCenters; i++)
	if (gameData.matCens.fuelCenters [i].nType == SEGMENT_IS_ROBOTMAKER) {
		 gameData.matCens.fuelCenters [i].nLives = 3;
		 gameData.matCens.fuelCenters [i].bEnabled = 0;
		 gameData.matCens.fuelCenters [i].xDisableTime = 0;
		 }

#ifdef _DEBUG

for (i = 0; i < gameData.matCens.nFuelCenters; i++)
	if (gameData.matCens.fuelCenters [i].nType == SEGMENT_IS_ROBOTMAKER) {
		//	Make sure this fuelcen is pointed at by a matcen.
		for (j = 0; j < gameData.matCens.nBotCenters; j++)
			if (gameData.matCens.botGens [j].nFuelCen == i)
				break;
#	if 1
		if (j == gameData.matCens.nBotCenters)
			j = j;
#	else
		Assert (j != gameData.matCens.nBotCenters);
#	endif
		}
	else if (gameData.matCens.fuelCenters [i].nType == SEGMENT_IS_EQUIPMAKER) {
		//	Make sure this fuelcen is pointed at by a matcen.
		for (j = 0; j < gameData.matCens.nEquipCenters; j++)
			if (gameData.matCens.equipGens [j].nFuelCen == i)
				break;
#	if 1
		if (j == gameData.matCens.nEquipCenters)
			j = j;
#	else
		Assert (j != gameData.matCens.nEquipCenters);
#	endif
		}

//	Make sure all matcens point at a fuelcen
for (i = 0; i < gameData.matCens.nBotCenters; i++) {
	nFuelCen = gameData.matCens.botGens [i].nFuelCen;
	Assert (nFuelCen < gameData.matCens.nFuelCenters);
#	if 1
	if (gameData.matCens.fuelCenters [nFuelCen].nType != SEGMENT_IS_ROBOTMAKER)
		i = i;
#	else
	Assert (gameData.matCens.fuelCenters [nFuelCen].nType == SEGMENT_IS_ROBOTMAKER);
#	endif
	}

for (i = 0; i < gameData.matCens.nEquipCenters; i++) {
	nFuelCen = gameData.matCens.equipGens [i].nFuelCen;
	Assert (nFuelCen < gameData.matCens.nFuelCenters);
#	if 1
	if (gameData.matCens.fuelCenters [nFuelCen].nType != SEGMENT_IS_EQUIPMAKER)
		i = i;
#	else
	Assert (gameData.matCens.fuelCenters [nFuelCen].nType == SEGMENT_IS_EQUIPMAKER);
#	endif
	}
#endif

}

//-----------------------------------------------------------------------------

short flagGoalList [MAX_SEGMENTS_D2X];
short flagGoalRoots [2] = {-1, -1};
short blueFlagGoals = -1;

// create linked lists of all segments with special nType blue and red goal

int GatherFlagGoals (void)
{
	int			h, i, j;
	tSegment2	*seg2P = gameData.segs.segment2s;

memset (flagGoalList, 0xff, sizeof (flagGoalList));
for (h = i = 0; i <= gameData.segs.nLastSegment; i++, seg2P++) {
	if (seg2P->special == SEGMENT_IS_GOAL_BLUE) {
		j = 0;
		h |= 1;
		}
	else if (seg2P->special == SEGMENT_IS_GOAL_RED) {
		h |= 2;
		j = 1;
		}
	else
		continue;
	flagGoalList [i] = flagGoalRoots [j];
	flagGoalRoots [j] = i;
	}
return h;
}

//--------------------------------------------------------------------

void MultiSendCaptureBonus (char);

int FlagAtHome (int nFlagId)
{
	int		i, j;
	tObject	*objP;

for (i = flagGoalRoots [nFlagId - POW_BLUEFLAG]; i >= 0; i = flagGoalList [i])
	for (j = gameData.segs.segments [i].objects; j >= 0; j = objP->next) {
		objP = gameData.objs.objects + j;
		if ((objP->nType == OBJ_POWERUP) && (objP->id == nFlagId))
			return 1;
		}
return 0;
}

//--------------------------------------------------------------------

int CheckFlagDrop (tSegment2 *pSeg, int nTeamId, int nFlagId, int nGoalId)
{
if (pSeg->special != nGoalId)
	return 0;
if (GetTeam (gameData.multiplayer.nLocalPlayer) != nTeamId)
	return 0;
if (!(LOCALPLAYER.flags & PLAYER_FLAGS_FLAG))
	return 0;
if (gameStates.app.bHaveExtraGameInfo [1] && extraGameInfo [1].bEnhancedCTF && 
	 !FlagAtHome ((nFlagId == POW_BLUEFLAG) ? POW_REDFLAG : POW_BLUEFLAG))
	return 0;
MultiSendCaptureBonus ((char) gameData.multiplayer.nLocalPlayer);
LOCALPLAYER.flags &= (~(PLAYER_FLAGS_FLAG));
MaybeDropNetPowerup (-1, nFlagId, FORCE_DROP);
return 1;
}

//--------------------------------------------------------------------

void FuelCenCheckForGoal (tSegment *segP)
{
	tSegment2	*seg2P = gameData.segs.segment2s + SEG_IDX (segP);

	Assert (segP != NULL);
	Assert (gameData.app.nGameMode & GM_CAPTURE);

#if 1
CheckFlagDrop (seg2P, TEAM_BLUE, POW_REDFLAG, SEGMENT_IS_GOAL_BLUE);
CheckFlagDrop (seg2P, TEAM_RED, POW_BLUEFLAG, SEGMENT_IS_GOAL_RED);
#else
if (!(LOCALPLAYER.flags & PLAYER_FLAGS_FLAG))
	return;
if (seg2P->special == SEGMENT_IS_GOAL_BLUE)	{
	if (GetTeam (gameData.multiplayer.nLocalPlayer) == TEAM_BLUE) && FlagAtHome (POW_BLUEFLAG)) {	
		MultiSendCaptureBonus (gameData.multiplayer.nLocalPlayer);
		LOCALPLAYER.flags &= (~(PLAYER_FLAGS_FLAG);
		MaybeDropNetPowerup (-1, POW_REDFLAG, FORCE_DROP);
		}
	}
else if (seg2P->special == SEGMENT_IS_GOAL_RED) {
	if (GetTeam (gameData.multiplayer.nLocalPlayer) == TEAM_RED) && FlagAtHome (POW_REDFLAG)) {	
		MultiSendCaptureBonus (gameData.multiplayer.nLocalPlayer);
		LOCALPLAYER.flags &= (~(PLAYER_FLAGS_FLAG);
		MaybeDropNetPowerup (-1, POW_BLUEFLAG, FORCE_DROP);
		}
	}
#endif
}

//--------------------------------------------------------------------

void FuelCenCheckForHoardGoal (tSegment *segP)
{
	tSegment2	*seg2P = &gameData.segs.segment2s [SEG_IDX (segP)];

Assert (segP != NULL);
Assert (gameData.app.nGameMode & GM_HOARD);
if (gameStates.app.bPlayerIsDead)
	return;
if ((seg2P->special != SEGMENT_IS_GOAL_BLUE) && (seg2P->special != SEGMENT_IS_GOAL_RED))
	return;
if (!LOCALPLAYER.secondaryAmmo [PROXMINE_INDEX])
	return;
#if TRACE
con_printf (CONDBG,"In orb goal!\n");
#endif
MultiSendOrbBonus ((char) gameData.multiplayer.nLocalPlayer);
LOCALPLAYER.flags &= (~(PLAYER_FLAGS_FLAG));
LOCALPLAYER.secondaryAmmo [PROXMINE_INDEX]=0;
}

//--------------------------------------------------------------------
#if 1//ndef FAST_FILE_IO /*permanently enabled for a reason!*/
/*
 * reads an old_tMatCenInfo structure from a CFILE
 */
void OldMatCenInfoRead (old_tMatCenInfo *mi, CFILE *fp)
{
mi->objFlags = CFReadInt (fp);
mi->xHitPoints = CFReadFix (fp);
mi->xInterval = CFReadFix (fp);
mi->nSegment = CFReadShort (fp);
mi->nFuelCen = CFReadShort (fp);
}

/*
 * reads a tMatCenInfo structure from a CFILE
 */
void MatCenInfoRead (tMatCenInfo *mi, CFILE *fp)
{
mi->objFlags [0] = CFReadInt (fp);
mi->objFlags [1] = CFReadInt (fp);
mi->xHitPoints = CFReadFix (fp);
mi->xInterval = CFReadFix (fp);
mi->nSegment = CFReadShort (fp);
mi->nFuelCen = CFReadShort (fp);
}
#endif
