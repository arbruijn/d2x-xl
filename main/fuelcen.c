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

// The max number of fuel stations per mine.

// Every time a robot is created in the morphing code, it decreases capacity of the morpher
// by this amount... when capacity gets to 0, no more morphers...
#define MATCEN_HP_DEFAULT			F1_0*500; // Hitpoints
#define MATCEN_INTERVAL_DEFAULT	F1_0*5;	//  5 seconds

#ifdef EDITOR
char	Special_names [MAX_CENTER_TYPES] [11] = {
	"NOTHING   ",
	"FUELCEN   ",
	"REPAIRCEN ",
	"CONTROLCEN",
	"ROBOTMAKER",
	"GOAL_RED",
	"GOAL_BLUE",
};
#endif

//------------------------------------------------------------
// Resets all fuel center info
void FuelCenReset ()
{
	int i;

	gameData.matCens.nFuelCenters = 0;
	for (i=0; i<MAX_SEGMENTS; i++)
		gameData.segs.segment2s [i].special = SEGMENT_IS_NOTHING;

	gameData.matCens.nRobotCenters = 0;

}

#ifndef NDEBUG		//this is sometimes called by people from the debugger
void reset_allRobot_centers ()
{
	int i;

	// Remove all materialization centers
	for (i=0; i<gameData.segs.nSegments; i++)
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
	segment2	*seg2p = gameData.segs.segment2s + (SEG_IDX (segP));

	int	stationType = seg2p->special;
	int	i;
	
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
		return;
	case SEGMENT_IS_FUELCEN:
	case SEGMENT_IS_REPAIRCEN:
	case SEGMENT_IS_CONTROLCEN:
	case SEGMENT_IS_ROBOTMAKER:
		break;
	default:
		Error ("Segment %d has invalid\nstation nType %d in fuelcen.c\n", SEG_IDX (segP), stationType);
	}

	Assert ((seg2p != NULL));
	if (seg2p == NULL) 
		return;

	switch (oldType) {
		case SEGMENT_IS_FUELCEN:
		case SEGMENT_IS_REPAIRCEN:
		case SEGMENT_IS_ROBOTMAKER:
			i = seg2p->value;
			break;
		default:
			Assert (gameData.matCens.nFuelCenters < MAX_FUEL_CENTERS);
			i = gameData.matCens.nFuelCenters;
		}

	seg2p->value = i;
	gameData.matCens.fuelCenters [i].Type = stationType;
	gameData.matCens.origStationTypes [i] = (oldType == stationType) ? SEGMENT_IS_NOTHING : oldType;
	gameData.matCens.fuelCenters [i].MaxCapacity = gameData.matCens.xFuelMaxAmount;
	gameData.matCens.fuelCenters [i].Capacity = gameData.matCens.fuelCenters [i].MaxCapacity;
	gameData.matCens.fuelCenters [i].nSegment = SEG2_IDX (seg2p);
	gameData.matCens.fuelCenters [i].Timer = -1;
	gameData.matCens.fuelCenters [i].Flag = 0;
//	gameData.matCens.fuelCenters [i].NextRobotType = -1;
//	gameData.matCens.fuelCenters [i].last_created_obj=NULL;
//	gameData.matCens.fuelCenters [i].last_created_sig = -1;
	COMPUTE_SEGMENT_CENTER (&gameData.matCens.fuelCenters [i].Center, segP);
	if (oldType == SEGMENT_IS_NOTHING)
		gameData.matCens.nFuelCenters++;
	else if (oldType == SEGMENT_IS_ROBOTMAKER) {
		gameData.matCens.origStationTypes [i] = SEGMENT_IS_NOTHING;
		i = seg2p->nMatCen;
		if (i < --gameData.matCens.nRobotCenters)
			memcpy (gameData.matCens.robotCenters + i, gameData.matCens.robotCenters + i + 1, (gameData.matCens.nRobotCenters - i) * sizeof (tFuelCenInfo));
		}
//	if (stationType == SEGMENT_IS_ROBOTMAKER)
//		gameData.matCens.fuelCenters [gameData.matCens.nFuelCenters].Capacity = i2f (gameStates.app.nDifficultyLevel + 3);
}

//------------------------------------------------------------
// Adds a matcen that already is a special nType into the gameData.matCens.fuelCenters array.
// This function is separate from other fuelcens because we don't want values reset.
void MatCenCreate (tSegment *segP, int oldType)
{
	segment2	*seg2p = &gameData.segs.segment2s [SEG_IDX (segP)];
	int	stationType = seg2p->special;
	int	i;

	Assert ((seg2p != NULL));
	Assert (stationType == SEGMENT_IS_ROBOTMAKER);
	if (seg2p == NULL) return;

	Assert (gameData.matCens.nFuelCenters > -1);
	switch (oldType) {
		case SEGMENT_IS_FUELCEN:
		case SEGMENT_IS_REPAIRCEN:
		case SEGMENT_IS_ROBOTMAKER:
			i = seg2p->value;
			break;
		default:
			Assert (gameData.matCens.nFuelCenters < MAX_FUEL_CENTERS);
			i = gameData.matCens.nFuelCenters;
		}
	seg2p->value = i;
	gameData.matCens.fuelCenters [i].Type = stationType;
	gameData.matCens.origStationTypes [i] = (oldType == stationType) ? SEGMENT_IS_NOTHING : oldType;
	gameData.matCens.fuelCenters [i].Capacity = i2f (gameStates.app.nDifficultyLevel + 3);
	gameData.matCens.fuelCenters [i].MaxCapacity = gameData.matCens.fuelCenters [i].Capacity;
	gameData.matCens.fuelCenters [i].nSegment = SEG2_IDX (seg2p);
	gameData.matCens.fuelCenters [i].Timer = -1;
	gameData.matCens.fuelCenters [i].Flag = 0;
//	gameData.matCens.fuelCenters [i].NextRobotType = -1;
//	gameData.matCens.fuelCenters [i].last_created_obj=NULL;
//	gameData.matCens.fuelCenters [i].last_created_sig = -1;
	COMPUTE_SEGMENT_CENTER_I (&gameData.matCens.fuelCenters [i].Center, seg2p-gameData.segs.segment2s);
	seg2p->nMatCen = gameData.matCens.nRobotCenters;
	gameData.matCens.robotCenters [gameData.matCens.nRobotCenters].hit_points = MATCEN_HP_DEFAULT;
	gameData.matCens.robotCenters [gameData.matCens.nRobotCenters].interval = MATCEN_INTERVAL_DEFAULT;
	gameData.matCens.robotCenters [gameData.matCens.nRobotCenters].nSegment = SEG2_IDX (seg2p);
	if (oldType == SEGMENT_IS_NOTHING)
		gameData.matCens.robotCenters [gameData.matCens.nRobotCenters].fuelcen_num = gameData.matCens.nFuelCenters;
	gameData.matCens.nRobotCenters++;
	gameData.matCens.nFuelCenters++;
}

//------------------------------------------------------------
// Adds a tSegment that already is a special nType into the gameData.matCens.fuelCenters array.
void FuelCenActivate (tSegment * segP, int stationType)
{
	segment2	*seg2p = &gameData.segs.segment2s [SEG_IDX (segP)];

	seg2p->special = stationType;
	if (seg2p->special == SEGMENT_IS_ROBOTMAKER)
		MatCenCreate (segP, SEGMENT_IS_NOTHING);
	else
		FuelCenCreate (segP, SEGMENT_IS_NOTHING);
}

//	The lower this number is, the more quickly the center can be re-triggered.
//	If it's too low, it can mean all the robots won't be put out, but for about 5
//	robots, that's not real likely.
#define	MATCEN_LIFE (i2f (30-2*gameStates.app.nDifficultyLevel))

//------------------------------------------------------------
//	Trigger (enable) the materialization center in tSegment nSegment
void MatCenTrigger (short nSegment)
{
	// -- tSegment		*segP = &gameData.segs.segments [nSegment];
	segment2		*seg2p = &gameData.segs.segment2s [nSegment];
	vmsVector	pos, delta;
	tFuelCenInfo	*robotcen;
	int			nObject;

#if TRACE
	con_printf (CON_DEBUG, "Trigger matcen, tSegment %i\n", nSegment);
#endif
	Assert (seg2p->special == SEGMENT_IS_ROBOTMAKER);
	Assert (seg2p->nMatCen < gameData.matCens.nFuelCenters);
	Assert ((seg2p->nMatCen >= 0) && (seg2p->nMatCen <= gameData.segs.nLastSegment));

	robotcen = gameData.matCens.fuelCenters + gameData.matCens.robotCenters [seg2p->nMatCen].fuelcen_num;

	if (robotcen->Enabled == 1)
		return;

	if (!robotcen->Lives)
		return;

	//	MK: 11/18/95, At insane, matcens work forever!
	if (gameStates.app.nDifficultyLevel+1 < NDL)
		robotcen->Lives--;

	robotcen->Timer = F1_0*1000;	//	Make sure the first robot gets emitted right away.
	robotcen->Enabled = 1;			//	Say this center is enabled, it can create robots.
	robotcen->Capacity = i2f (gameStates.app.nDifficultyLevel + 3);
	robotcen->DisableTime = MATCEN_LIFE;

	//	Create a bright tObject in the tSegment.
	pos = robotcen->Center;
	VmVecSub (&delta, gameData.segs.vertices + gameData.segs.segments [nSegment].verts [0], &robotcen->Center);
	VmVecScaleInc (&pos, &delta, F1_0/2);
	nObject = CreateObject (OBJ_LIGHT, 0, -1, nSegment, &pos, NULL, 0, CT_LIGHT, MT_NONE, RT_NONE, 1);
	if (nObject != -1) {
		gameData.objs.objects [nObject].lifeleft = MATCEN_LIFE;
		gameData.objs.objects [nObject].cType.lightInfo.intensity = i2f (8);	//	Light cast by a fuelcen.
	} else {
#if TRACE
		con_printf (1, "Can't create invisible flare for matcen.\n");
#endif
		Int3 ();
	}
}

#ifdef EDITOR
//------------------------------------------------------------
// Takes away a tSegment's fuel center properties.
//	Deletes the tSegment point entry in the tFuelCenInfo list.
void FuelCenDelete (tSegment * segP)
{
	segment2	*seg2p = &gameData.segs.segment2s [SEG_IDX (segP)];
	int i, j;

Restart: ;

	seg2p->special = 0;

	for (i=0; i<gameData.matCens.nFuelCenters; i++)	{
		if (gameData.matCens.fuelCenters [i].position.nSegment == SEG_IDX (segP))	{

			// If Robot maker is deleted, fix gameData.segs.segments and gameData.matCens.robotCenters.
			if (gameData.matCens.fuelCenters [i].Type == SEGMENT_IS_ROBOTMAKER) {
				gameData.matCens.nRobotCenters--;
				Assert (gameData.matCens.nRobotCenters >= 0);

				for (j=seg2p->nMatCen; j<gameData.matCens.nRobotCenters; j++)
					gameData.matCens.robotCenters [j] = gameData.matCens.robotCenters [j+1];

				for (j=0; j<gameData.matCens.nFuelCenters; j++) {
					if (gameData.matCens.fuelCenters [j].Type == SEGMENT_IS_ROBOTMAKER)
						if (gameData.segs.segment2s [gameData.matCens.fuelCenters [j].position.nSegment].nMatCen > seg2p->nMatCen)
							gameData.segs.segment2s [gameData.matCens.fuelCenters [j].position.nSegment].nMatCen--;
				}
			}

			//fix gameData.matCens.robotCenters so they point to correct fuelcenter
			for (j=0; j<gameData.matCens.nRobotCenters; j++)
				if (gameData.matCens.robotCenters [j].fuelcen_num > i)		//this robotcenter's fuelcen is changing
					gameData.matCens.robotCenters [j].fuelcen_num--;

			gameData.matCens.nFuelCenters--;
			Assert (gameData.matCens.nFuelCenters >= 0);
			for (j=i; j<gameData.matCens.nFuelCenters; j++)	{
				gameData.matCens.fuelCenters [j] = gameData.matCens.fuelCenters [j+1];
				gameData.segs.segment2s [gameData.matCens.fuelCenters [j].position.nSegment].value = j;
			}
			goto Restart;
		}
	}
}
#endif

#define	ROBOT_GEN_TIME (i2f (5))

tObject * CreateMorphRobot (tSegment *segP, vmsVector *vObjPosP, ubyte object_id)
{
	short		nObject;
	tObject	*objP;
	ubyte		default_behavior;

	gameData.multi.players [gameData.multi.nLocalPlayer].numRobotsLevel++;
	gameData.multi.players [gameData.multi.nLocalPlayer].numRobotsTotal++;

	nObject = CreateObject ((ubyte) OBJ_ROBOT, object_id, -1, SEG_IDX (segP), vObjPosP,
								  &vmdIdentityMatrix, gameData.models.polyModels [gameData.bots.pInfo [object_id].nModel].rad,
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
	objP->rType.polyObjInfo.nModel = gameData.bots.pInfo [objP->id].nModel;
	objP->rType.polyObjInfo.nSubObjFlags = 0;
	//set Physics info
	objP->mType.physInfo.mass = gameData.bots.pInfo [objP->id].mass;
	objP->mType.physInfo.drag = gameData.bots.pInfo [objP->id].drag;
	objP->mType.physInfo.flags |= (PF_LEVELLING);
	objP->shields = gameData.bots.pInfo [objP->id].strength;
	default_behavior = gameData.bots.pInfo [objP->id].behavior;
	InitAIObject (OBJ_IDX (objP), default_behavior, -1);		//	Note, -1 = tSegment this robot goes to to hide, should probably be something useful
	CreateNSegmentPath (objP, 6, -1);		//	Create a 6 tSegment path from creation point.
	gameData.ai.localInfo [nObject].mode = AIBehaviorToMode (default_behavior);
	return objP;
}

int Num_extryRobots = 15;

#ifndef NDEBUG
int	FrameCount_last_msg = 0;
#endif

//	----------------------------------------------------------------------------------------------------------

void MatCenHandler (tFuelCenInfo * robotcen)
{
	fix			dist_to_player;
	vmsVector	curObject_loc; //, direction;
	int			nMatCen, nSegment, nObject;
	tObject		*objP;
	fix			topTime;
	vmsVector	direction;
#if 0//def _DEBUG
	int			bMakeVirus = 1;
#else
	int			bMakeVirus = (gameData.app.nGameMode & GM_ENTROPY) != 0;
#endif
	ubyte			tVideoClip = bMakeVirus ? VCLIP_POWERUP_DISAPPEARANCE : VCLIP_MORPHING_ROBOT;

	if (bMakeVirus) {
#if 1//def RELEASE
		if (gameStates.entropy.bExitSequence || (gameData.segs.xSegments [robotcen->nSegment].owner <= 0))
			return;
#endif
		}
	else {
		if (!robotcen->Enabled)
			return;
		if (robotcen->DisableTime > 0) {
			robotcen->DisableTime -= gameData.time.xFrame;
			if (robotcen->DisableTime <= 0) {
#if TRACE
				con_printf (CON_DEBUG, "Robot center #%i gets disabled due to time running out.\n", 
								FUELCEN_IDX (robotcen));
#endif
				robotcen->Enabled = 0;
			}
		}
	}

	//	No robot making in multiplayer mode.
	if (!bMakeVirus && (gameData.app.nGameMode & GM_MULTI) && (!(gameData.app.nGameMode & GM_MULTI_ROBOTS) || !NetworkIAmMaster ()))
		return;
	// Wait until transmorgafier has capacity to make a robot...
	if (!bMakeVirus && (robotcen->Capacity <= 0)) {
		return;
	}

	nMatCen = gameData.segs.segment2s [robotcen->nSegment].nMatCen;

	if (nMatCen == -1) {
#if TRACE
		con_printf (CON_DEBUG, "Non-functional robotcen at %d\n", robotcen->nSegment);
#endif
		return;
	}

	if (!bMakeVirus && 
		 gameData.matCens.robotCenters [nMatCen].robotFlags [0]==0 && 
		 gameData.matCens.robotCenters [nMatCen].robotFlags [1]==0) {
		return;
	}

	// Wait until we have a d_free slot for this puppy...
   //	  <<<<<<<<<<<<<<<< Num robots in mine >>>>>>>>>>>>>>>>>>>>>>>>>>    <<<<<<<<<<<< Max robots in mine >>>>>>>>>>>>>>>
	if ((gameData.multi.players [gameData.multi.nLocalPlayer].numRobotsLevel - gameData.multi.players [gameData.multi.nLocalPlayer].numKillsLevel) >= 
		 (Gamesave_num_orgRobots + Num_extryRobots)) {
		#ifndef NDEBUG
		if (gameData.app.nFrameCount > FrameCount_last_msg + 20) {
#if TRACE
			con_printf (CON_DEBUG, "Cannot morph until you kill one!\n");
#endif
			FrameCount_last_msg = gameData.app.nFrameCount;
		}
		#endif
		return;
	}

	robotcen->Timer += gameData.time.xFrame;

	switch (robotcen->Flag)	{
	case 0:		// Wait until next robot can generate
		if (gameData.app.nGameMode & GM_MULTI) {
			topTime = bMakeVirus ? i2f (extraGameInfo [1].entropy.nVirusGenTime) : ROBOT_GEN_TIME;	
//			if (bMakeVirus)
//				topTime *= 2;
			}
		else {
			dist_to_player = VmVecDistQuick (&gameData.objs.console->position.vPos, &robotcen->Center);
			topTime = dist_to_player/64 + d_rand () * 2 + F1_0*2;
			if (topTime > ROBOT_GEN_TIME)
				topTime = ROBOT_GEN_TIME + d_rand ();
			if (topTime < F1_0*2)
				topTime = F1_0*3/2 + d_rand ()*2;
			}
		if (robotcen->Timer > topTime)	{
			int	count=0;
			int	i, my_station_num = FUELCEN_IDX (robotcen);

			//	Make sure this robotmaker hasn't put out its max without having any of them killed.
			if (bMakeVirus) {
				tObject *objP;
				int nObject = gameData.segs.segments [robotcen->nSegment].objects;

				while (nObject >= 0) {
					objP = gameData.objs.objects + nObject;
					if ((objP->nType == OBJ_POWERUP) && (objP->id == POW_ENTROPY_VIRUS)) {
						robotcen->Timer = 0;
						return;
						}
					nObject = objP->next;
					}
				}
			else {
				for (i=0; i<=gameData.objs.nLastObject; i++)
					if (gameData.objs.objects [i].nType == OBJ_ROBOT)
						if ((gameData.objs.objects [i].matCenCreator^0x80) == my_station_num)
							count++;
				if (count > gameStates.app.nDifficultyLevel + 3) {
#if TRACE
					con_printf (CON_DEBUG, "Cannot morph: center %i has already put out %i robots.\n", my_station_num, count);
#endif
					robotcen->Timer /= 2;
					return;
					}
				}

			//	Whack on any robot or tPlayer in the matcen tSegment.
			count=0;
			nSegment = robotcen->nSegment;
#if 1//def _DEBUG
		if (!bMakeVirus)
#endif
			for (nObject=gameData.segs.segments [nSegment].objects;nObject!=-1;nObject=gameData.objs.objects [nObject].next)	{
				count++;
				if (count > MAX_OBJECTS)	{
#if TRACE
					con_printf (CON_DEBUG, "Object list in tSegment %d is circular.", nSegment);
#endif
					Int3 ();
					return;
				}
				if (gameData.objs.objects [nObject].nType == OBJ_ROBOT) {
					CollideRobotAndMatCen (gameData.objs.objects + nObject);
					robotcen->Timer = topTime / 2;
					return;
					}
				else if (gameData.objs.objects [nObject].nType == OBJ_PLAYER) {
					CollidePlayerAndMatCen (gameData.objs.objects + nObject);
					robotcen->Timer = topTime / 2;
					return;
				}
			}

			COMPUTE_SEGMENT_CENTER_I (&curObject_loc, robotcen->nSegment);
			// HACK!!!The 10 under here should be something equal to the 1/2 the size of the tSegment.
			objP = ObjectCreateExplosion ((short) robotcen->nSegment, &curObject_loc, i2f (10), tVideoClip);

			if (objP)
				ExtractOrientFromSegment (&objP->position.mOrient,gameData.segs.segments + robotcen->nSegment);

			if (gameData.eff.vClips [0] [tVideoClip].nSound > -1) {
				DigiLinkSoundToPos (gameData.eff.vClips [0] [tVideoClip].nSound, (short) robotcen->nSegment,
												0, &curObject_loc, 0, F1_0);
			}
			robotcen->Flag	= 1;
			robotcen->Timer = 0;
		}
		break;

	case 1:			// Wait until 1/2 second after VCLIP started.
		if (robotcen->Timer > (gameData.eff.vClips [0] [tVideoClip].xTotalTime / 2))	{
			if (!bMakeVirus)
				robotcen->Capacity -= gameData.matCens.xEnergyToCreateOneRobot;
			robotcen->Flag = 0;
			robotcen->Timer = 0;
			COMPUTE_SEGMENT_CENTER_I (&curObject_loc, robotcen->nSegment);

			// If this is the first materialization, set to valid robot.
			if (bMakeVirus ||
				 gameData.matCens.robotCenters [nMatCen].robotFlags [0] != 0 || 
				 gameData.matCens.robotCenters [nMatCen].robotFlags [1] != 0) {
				ubyte	nType;
				uint	flags;
				sbyte legalTypes [64];   // 64 bits, the width of robotFlags [].
				int	numTypes, robot_index, i;

				if (!bMakeVirus) {
					numTypes = 0;
					for (i=0;i<2;i++) {
						robot_index = i*32;
						flags = gameData.matCens.robotCenters [nMatCen].robotFlags [i];
						while (flags) {
							if (flags & 1)
								legalTypes [numTypes++] = robot_index;
							flags >>= 1;
							robot_index++;
							}
						}

					if (numTypes == 1)
						nType = legalTypes [0];
					else
						nType = legalTypes [ (d_rand () * numTypes) / 32768];
#if TRACE
					con_printf (CON_DEBUG, "Morph: (nType = %i) (seg = %i) (capacity = %08x)\n", nType, robotcen->nSegment, robotcen->Capacity);
#endif
					}
				if (bMakeVirus) {
					int nObject = CreateObject (OBJ_POWERUP, POW_ENTROPY_VIRUS, -1, (short) robotcen->nSegment, &curObject_loc, &vmdIdentityMatrix, 
														gameData.objs.pwrUp.info [POW_ENTROPY_VIRUS].size, 
														CT_POWERUP, MT_PHYSICS, RT_POWERUP, 1);
					if (nObject >= 0) {
						objP = gameData.objs.objects + nObject;
						if (gameData.app.nGameMode & GM_MULTI)
							multiData.create.nObjNums [multiData.create.nLoc++] = nObject;
						objP->rType.vClipInfo.nClipIndex = gameData.objs.pwrUp.info [objP->id].nClipIndex;
						objP->rType.vClipInfo.xFrameTime = gameData.eff.vClips [0] [objP->rType.vClipInfo.nClipIndex].xFrameTime;
						objP->rType.vClipInfo.nCurFrame = 0;
						objP->matCenCreator = gameData.segs.xSegments [robotcen->nSegment].owner;
#if 1//def _DEBUG
						objP->lifeleft = IMMORTAL_TIME;
#endif
						}
					}
				else {
					objP = CreateMorphRobot (gameData.segs.segments + robotcen->nSegment, &curObject_loc, nType);
					if (objP == NULL) {
#if TRACE
						con_printf (CON_DEBUG, "Warning: CreateMorphRobot returned NULL (no gameData.objs.objects left?)\n");
#endif
						}
					else {
						if (gameData.app.nGameMode & GM_MULTI)
							MultiSendCreateRobot (FUELCEN_IDX (robotcen), OBJ_IDX (objP), nType);
						objP->matCenCreator = (FUELCEN_IDX (robotcen)) | 0x80;
						// Make tObject faces player...
						VmVecSub (&direction, &gameData.objs.console->position.vPos,&objP->position.vPos);
						VmVector2Matrix (&objP->position.mOrient, &direction, &objP->position.mOrient.uVec, NULL);
						MorphStart (objP);
						//robotcen->last_created_obj = obj;
						//robotcen->last_created_sig = robotcen->last_created_objP->nSignature;
						}
					}
				}
			}
		break;

	default:
		robotcen->Flag = 0;
		robotcen->Timer = 0;
	}
}


//-------------------------------------------------------------
// Called once per frame, replenishes fuel supply.
void FuelcenUpdateAll ()
{
	int i;
	fix xAmountToReplenish = FixMul (gameData.time.xFrame,gameData.matCens.xFuelRefillSpeed);

	for (i=0; i<gameData.matCens.nFuelCenters; i++)	{
		if (gameData.matCens.fuelCenters [i].Type == SEGMENT_IS_ROBOTMAKER)	{
			if (!(gameStates.app.bGameSuspended & SUSP_ROBOTS))
				MatCenHandler (&gameData.matCens.fuelCenters [i]);
		} else if (gameData.matCens.fuelCenters [i].Type == SEGMENT_IS_CONTROLCEN)	{
			//controlcen_proc (&gameData.matCens.fuelCenters [i]);
	
		} else if ((gameData.matCens.fuelCenters [i].MaxCapacity > 0) && 
					  (gameData.matCens.playerSegP != gameData.segs.segments + gameData.matCens.fuelCenters [i].nSegment)) {
			if (gameData.matCens.fuelCenters [i].Capacity < gameData.matCens.fuelCenters [i].MaxCapacity) {
 				gameData.matCens.fuelCenters [i].Capacity += xAmountToReplenish;
				if (gameData.matCens.fuelCenters [i].Capacity >= gameData.matCens.fuelCenters [i].MaxCapacity) {
					gameData.matCens.fuelCenters [i].Capacity = gameData.matCens.fuelCenters [i].MaxCapacity;
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
//--unused-- 		gameData.matCens.fuelCenters [i].Capacity = gameData.matCens.fuelCenters [i].MaxCapacity;
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
if ((xsegP->owner < 1) || (xsegP->owner == GetTeam (gameData.multi.nLocalPlayer) + 1))
	return 0;
amount = FixMul (gameData.time.xFrame, gameData.matCens.xFuelGiveAmount * extraGameInfo [1].entropy.nShieldDamageRate / 25);
if (amount > MaxAmountCanGive)
	amount = MaxAmountCanGive;
if (last_playTime > gameData.time.xGame)
	last_playTime = 0;
if (gameData.time.xGame > last_playTime+FUELCEN_SOUND_DELAY) {
	DigiPlaySample (SOUND_PLAYER_GOT_HIT, F1_0/2);
	if (gameData.app.nGameMode & GM_MULTI)
		MultiSendPlaySound (SOUND_PLAYER_GOT_HIT, F1_0/2);
	last_playTime = gameData.time.xGame;
	}
return amount;
}

//-------------------------------------------------------------

fix FuelCenGiveFuel (tSegment *segP, fix MaxAmountCanTake)
{
	short		nSegment = SEG_IDX (segP);
	segment2	*seg2p = gameData.segs.segment2s + nSegment;
	xsegment	*xsegp = gameData.segs.xSegments + nSegment;

	static fix last_playTime=0;

	Assert (segP != NULL);
	gameData.matCens.playerSegP = segP;
	if ((gameData.app.nGameMode & GM_ENTROPY) && ((xsegp->owner < 0) || ((xsegp->owner > 0) && (xsegp->owner != GetTeam (gameData.multi.nLocalPlayer) + 1))))
		return 0;
	if ((segP) && (seg2p->special==SEGMENT_IS_FUELCEN))	{
		fix amount;
		DetectEscortGoalAccomplished (-4);	//	UGLY!Hack!-4 means went through fuelcen.

//		if (gameData.matCens.fuelCenters [segP->value].MaxCapacity<=0)	{
//			HUDInitMessage ("Fuelcenter %d is destroyed.", segP->value);
//			return 0;
//		}

//		if (gameData.matCens.fuelCenters [segP->value].Capacity<=0)	{
//			HUDInitMessage ("Fuelcenter %d is empty.", segP->value);
//			return 0;
//		}

		if (MaxAmountCanTake <= 0)	{
			return 0;
		}
		if (gameData.app.nGameMode & GM_ENTROPY)
			amount = FixMul (gameData.time.xFrame, 
								  gameData.matCens.xFuelGiveAmount * 
								  extraGameInfo [IsMultiGame].entropy.nEnergyFillRate / 25);
		else
			amount = FixMul (gameData.time.xFrame, gameData.matCens.xFuelGiveAmount);
		if (amount > MaxAmountCanTake)
			amount = MaxAmountCanTake;
		if (last_playTime > gameData.time.xGame)
			last_playTime = 0;
		if (gameData.time.xGame > last_playTime+FUELCEN_SOUND_DELAY) {
			DigiPlaySample (SOUND_REFUEL_STATION_GIVING_FUEL, F1_0/2);
			if (gameData.app.nGameMode & GM_MULTI)
				MultiSendPlaySound (SOUND_REFUEL_STATION_GIVING_FUEL, F1_0/2);
			last_playTime = gameData.time.xGame;
		}


		//HUDInitMessage ("Fuelcen %d has %d/%d fuel", segP->value,f2i (gameData.matCens.fuelCenters [segP->value].Capacity),f2i (gameData.matCens.fuelCenters [segP->value].MaxCapacity));
		return amount;

	} else {
		return 0;
	}
}

//-------------------------------------------------------------
// DM/050904
// Repair centers
// use same values as fuel centers
fix RepairCenGiveShields (tSegment *segP, fix MaxAmountCanTake)
{
	short		nSegment = SEG_IDX (segP);
	segment2	*seg2p = gameData.segs.segment2s + nSegment;
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
	 ((xsegp->owner > 0) && (xsegp->owner != GetTeam (gameData.multi.nLocalPlayer) + 1))))
	return 0;
if (seg2p->special != SEGMENT_IS_REPAIRCEN)
	return 0;
//		DetectEscortGoalAccomplished (-4);	//	UGLY!Hack!-4 means went through fuelcen.
//		if (gameData.matCens.fuelCenters [segP->value].MaxCapacity<=0)	{
//			HUDInitMessage ("Repaircenter %d is destroyed.", segP->value);
//			return 0;
//		}
//		if (gameData.matCens.fuelCenters [segP->value].Capacity<=0)	{
//			HUDInitMessage ("Repaircenter %d is empty.", segP->value);
//			return 0;
//		}
if (MaxAmountCanTake <= 0)	{
	return 0;
}
amount = FixMul (gameData.time.xFrame, gameData.matCens.xFuelGiveAmount * extraGameInfo [IsMultiGame].entropy.nShieldFillRate / 25);
if (amount > MaxAmountCanTake)
	amount = MaxAmountCanTake;
if (last_playTime > gameData.time.xGame)
	last_playTime = 0;
if (gameData.time.xGame > last_playTime+FUELCEN_SOUND_DELAY) {
	DigiPlaySample (SOUND_REFUEL_STATION_GIVING_FUEL, F1_0/2);
	if (gameData.app.nGameMode & GM_MULTI)
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
//--unused-- //--		if (gameData.matCens.fuelCenters [station_num].MaxCapacity<=0)	{
//--unused-- //--			// Shooting a already destroyed materializer
//--unused-- //--		} else {
//--unused-- //--			gameData.matCens.fuelCenters [station_num].MaxCapacity -= damage;
//--unused-- //--			if (gameData.matCens.fuelCenters [station_num].Capacity > gameData.matCens.fuelCenters [station_num].MaxCapacity)	{
//--unused-- //--				gameData.matCens.fuelCenters [station_num].Capacity = gameData.matCens.fuelCenters [station_num].MaxCapacity;
//--unused-- //--			}
//--unused-- //--			if (gameData.matCens.fuelCenters [station_num].MaxCapacity <= 0)	{
//--unused-- //--				gameData.matCens.fuelCenters [station_num].MaxCapacity = 0;
//--unused-- //--				// Robotmaker dead
//--unused-- //--				for (i=0; i<6; i++)
//--unused-- //--					segP->sides [i].nOvlTex = 0;
//--unused-- //--			}
//--unused-- //--		}
//--unused-- 		break;
//--unused-- 	case SEGMENT_IS_FUELCEN:	
//--unused-- //--		DigiPlaySample (SOUND_REFUEL_STATION_HIT);
//--unused-- //--		if (gameData.matCens.fuelCenters [station_num].MaxCapacity>0)	{
//--unused-- //--			gameData.matCens.fuelCenters [station_num].MaxCapacity -= damage;
//--unused-- //--			if (gameData.matCens.fuelCenters [station_num].Capacity > gameData.matCens.fuelCenters [station_num].MaxCapacity)	{
//--unused-- //--				gameData.matCens.fuelCenters [station_num].Capacity = gameData.matCens.fuelCenters [station_num].MaxCapacity;
//--unused-- //--			}
//--unused-- //--			if (gameData.matCens.fuelCenters [station_num].MaxCapacity <= 0)	{
//--unused-- //--				gameData.matCens.fuelCenters [station_num].MaxCapacity = 0;
//--unused-- //--				DigiPlaySample (SOUND_REFUEL_STATION_DESTROYED);
//--unused-- //--			}
//--unused-- //--		} else {
//--unused-- //--			gameData.matCens.fuelCenters [station_num].MaxCapacity = 0;
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
//--repair-- 		entry_side = john_find_connect_side (repair_seg,objP->position.nSegment);
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
//--repair-- 	if (gameData.multi.players [gameData.multi.nLocalPlayer].shields < MAX_SHIELDS) {	//if above max, don't mess with it
//--repair--
//--repair-- 		gameData.multi.players [gameData.multi.nLocalPlayer].shields += FixMul (gameData.time.xFrame,repair_rate);
//--repair--
//--repair-- 		if (gameData.multi.players [gameData.multi.nLocalPlayer].shields > MAX_SHIELDS)
//--repair-- 			gameData.multi.players [gameData.multi.nLocalPlayer].shields = MAX_SHIELDS;
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
//--repair-- 		if (gameData.multi.players [gameData.multi.nLocalPlayer].shields < MAX_SHIELDS)
//--repair-- 			gameData.multi.players [gameData.multi.nLocalPlayer].shields = MAX_SHIELDS;
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
//--repair-- 	if (Lsegments [objP->position.nSegment].specialType & SS_REPAIR_CENTER) {
//--repair--
//--repair-- 		if (!disable_repair_center) {
//--repair-- 			//have just entered repair center
//--repair--
//--repair-- 			RepairObj = obj;
//--repair-- 			repair_save_uvec = objP->position.mOrient.uVec;
//--repair--
//--repair-- 			repair_rate = FixMulDiv (FULL_REPAIR_RATE, (MAX_SHIELDS - gameData.multi.players [gameData.multi.nLocalPlayer].shields),MAX_SHIELDS);
//--repair--
//--repair-- 			save_controlType = objP->controlType;
//--repair-- 			save_movementType = objP->movementType;
//--repair--
//--repair-- 			objP->controlType = CT_REPAIRCEN;
//--repair-- 			objP->movementType = MT_NONE;
//--repair--
//--repair-- 			FuelStationSeg	= Lsegments [objP->position.nSegment].special_segment;
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

	for (i=0; i<gameData.matCens.nRobotCenters; i++) {
		gameData.matCens.fuelCenters [i].Enabled = 0;
		gameData.matCens.fuelCenters [i].DisableTime = 0;
	}
}

//	--------------------------------------------------------------------------------------------
//	Initialize all materialization centers.
//	Give them all the right number of lives.
void InitAllMatCens (void)
{
	int	i;

	for (i=0; i<gameData.matCens.nFuelCenters; i++)
		if (gameData.matCens.fuelCenters [i].Type == SEGMENT_IS_ROBOTMAKER) {
			gameData.matCens.fuelCenters [i].Lives = 3;
			gameData.matCens.fuelCenters [i].Enabled = 0;
			gameData.matCens.fuelCenters [i].DisableTime = 0;
#ifndef NDEBUG
{
			//	Make sure this fuelcen is pointed at by a matcen.
			int	j;
			for (j=0; j<gameData.matCens.nRobotCenters; j++) {
				if (gameData.matCens.robotCenters [j].fuelcen_num == i)
					break;
			}
			Assert (j != gameData.matCens.nRobotCenters);
}
#endif

		}

#ifndef NDEBUG
	//	Make sure all matcens point at a fuelcen
	for (i=0; i<gameData.matCens.nRobotCenters; i++) {
		int	fuelcen_num = gameData.matCens.robotCenters [i].fuelcen_num;

		Assert (fuelcen_num < gameData.matCens.nFuelCenters);
		Assert (gameData.matCens.fuelCenters [fuelcen_num].Type == SEGMENT_IS_ROBOTMAKER);
	}
#endif

}

//-----------------------------------------------------------------------------

short flag_goal_list [MAX_SEGMENTS];
short flag_goal_roots [2] = {-1, -1};
short blueFlag_goals = -1;

// create linked lists of all segments with special nType blue and red goal

int GatherFlagGoals (void)
{
	int			h, i, j;
	segment2		*seg2P = gameData.segs.segment2s;

memset (flag_goal_list, 0xff, sizeof (flag_goal_list));
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
	flag_goal_list [i] = flag_goal_roots [j];
	flag_goal_roots [j] = i;
	}
return h;
}

//--------------------------------------------------------------------

void MultiSendCaptureBonus (char);

int FlagAtHome (int nFlagId)
{
	int		i, j;
	tObject	*objP;

for (i = flag_goal_roots [nFlagId - POW_BLUEFLAG]; i >= 0; i = flag_goal_list [i])
	for (j = gameData.segs.segments [i].objects; j >= 0; j = objP->next) {
		objP = gameData.objs.objects + j;
		if ((objP->nType == OBJ_POWERUP) && (objP->id == nFlagId))
			return 1;
		}
return 0;
}

//--------------------------------------------------------------------

int CheckFlagDrop (segment2 *pSeg, int nTeamId, int nFlagId, int nGoalId)
{
if (pSeg->special != nGoalId)
	return 0;
if (GetTeam (gameData.multi.nLocalPlayer) != nTeamId)
	return 0;
if (!(gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_FLAG))
	return 0;
if (gameStates.app.bHaveExtraGameInfo [1] && extraGameInfo [1].bEnhancedCTF && 
	 !FlagAtHome ((nFlagId == POW_BLUEFLAG) ? POW_REDFLAG : POW_BLUEFLAG))
	return 0;
MultiSendCaptureBonus ((char) gameData.multi.nLocalPlayer);
gameData.multi.players [gameData.multi.nLocalPlayer].flags &= (~ (PLAYER_FLAGS_FLAG));
MaybeDropNetPowerup (-1, nFlagId, FORCE_DROP);
return 1;
}

//--------------------------------------------------------------------

void FuelCenCheckForGoal (tSegment *segP)
{
	segment2	*seg2p = &gameData.segs.segment2s [SEG_IDX (segP)];

	Assert (segP != NULL);
	Assert (gameData.app.nGameMode & GM_CAPTURE);

#if 1
CheckFlagDrop (seg2p, TEAM_BLUE, POW_REDFLAG, SEGMENT_IS_GOAL_BLUE);
CheckFlagDrop (seg2p, TEAM_RED, POW_BLUEFLAG, SEGMENT_IS_GOAL_RED);
#else
if (seg2p->special==SEGMENT_IS_GOAL_BLUE)	{
	if ((GetTeam (gameData.multi.nLocalPlayer)==TEAM_BLUE) && 
		 (gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_FLAG))
		  FlagAtHome (POW_BLUEFLAG)) {		
		{
#if TRACE
		con_printf (CON_DEBUG,"In goal tSegment BLUE\n");
#endif
		MultiSendCaptureBonus (gameData.multi.nLocalPlayer);
		gameData.multi.players [gameData.multi.nLocalPlayer].flags &= (~ (PLAYER_FLAGS_FLAG);
		MaybeDropNetPowerup (-1, POW_REDFLAG, FORCE_DROP);
		}
	}
else if (seg2p->special==SEGMENT_IS_GOAL_RED) {
	if ((GetTeam (gameData.multi.nLocalPlayer)==TEAM_RED) && 
		 (gameData.multi.players [gameData.multi.nLocalPlayer].flags & PLAYER_FLAGS_FLAG)) &&
		 FlagAtHome (POW_REDFLAG)) {		
#if TRACE
		con_printf (CON_DEBUG,"In goal tSegment RED\n");
#endif
		MultiSendCaptureBonus (gameData.multi.nLocalPlayer);
		gameData.multi.players [gameData.multi.nLocalPlayer].flags &= (~ (PLAYER_FLAGS_FLAG);
		MaybeDropNetPowerup (-1, POW_BLUEFLAG, FORCE_DROP);
		}
	}
#endif
}

//--------------------------------------------------------------------

void FuelCenCheckForHoardGoal (tSegment *segP)
{
	segment2	*seg2p = &gameData.segs.segment2s [SEG_IDX (segP)];

	Assert (segP != NULL);
	Assert (gameData.app.nGameMode & GM_HOARD);

   if (gameStates.app.bPlayerIsDead)
		return;

	if (seg2p->special==SEGMENT_IS_GOAL_BLUE || seg2p->special==SEGMENT_IS_GOAL_RED )	
	{
		if (gameData.multi.players [gameData.multi.nLocalPlayer].secondaryAmmo [PROXIMITY_INDEX])
		{
#if TRACE
				con_printf (CON_DEBUG,"In orb goal!\n");
#endif
				MultiSendOrbBonus ((char) gameData.multi.nLocalPlayer);
				gameData.multi.players [gameData.multi.nLocalPlayer].flags &= (~ (PLAYER_FLAGS_FLAG));
				gameData.multi.players [gameData.multi.nLocalPlayer].secondaryAmmo [PROXIMITY_INDEX]=0;
      }
	}

}

//--------------------------------------------------------------------
#ifndef FAST_FILE_IO
/*
 * reads an old_tMatCenInfo structure from a CFILE
 */
void OldMatCenInfoRead (old_tMatCenInfo *mi, CFILE *fp)
{
	mi->robotFlags = CFReadInt (fp);
	mi->hit_points = CFReadFix (fp);
	mi->interval = CFReadFix (fp);
	mi->nSegment = CFReadShort (fp);
	mi->fuelcen_num = CFReadShort (fp);
}

/*
 * reads a tMatCenInfo structure from a CFILE
 */
void MatCenInfoRead (tMatCenInfo *mi, CFILE *fp)
{
	mi->robotFlags [0] = CFReadInt (fp);
	mi->robotFlags [1] = CFReadInt (fp);
	mi->hit_points = CFReadFix (fp);
	mi->interval = CFReadFix (fp);
	mi->nSegment = CFReadShort (fp);
	mi->fuelcen_num = CFReadShort (fp);
}
#endif
