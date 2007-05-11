/* $Id: fireball.c, v 1.4 2003/10/04 03:14:47 btb Exp $ */
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "error.h"
#include "fix.h"
#include "vecmat.h"
#include "gr.h"
#include "3d.h"

#include "inferno.h"
#include "object.h"
#include "vclip.h"
#include "game.h"
#include "mono.h"
#include "polyobj.h"
#include "sounds.h"
#include "player.h"
#include "gauges.h"
#include "powerup.h"
#include "bm.h"
#include "ai.h"
#include "weapon.h"
#include "fireball.h"
#include "collide.h"
#include "newmenu.h"
#include "network.h"
#include "gameseq.h"
#include "physics.h"
#include "scores.h"
#include "laser.h"
#include "wall.h"
#include "multi.h"
#include "endlevel.h"
#include "timer.h"
#include "fuelcen.h"
#include "cntrlcen.h"
#include "gameseg.h"
#include "gamesave.h"
#include "automap.h"
#include "kconfig.h"
#include "text.h"
#include "hudmsg.h"
//#define _DEBUG
#define EXPLOSION_SCALE (F1_0*5/2)		//explosion is the obj size times this 

//--unused-- ubyte	Frame_processed [MAX_OBJECTS];

int	PK1=1, PK2=8;

//------------------------------------------------------------------------------

tObject *ObjectCreateExplosionSub (
	tObject *objP, 
	short nSegment, 
	vmsVector * position, 
	fix size, 
	ubyte vclipType, 
	fix maxdamage, 
	fix maxdistance, 
	fix maxforce, 
	short parent)
{
	short nObject;
	tObject *explObjP;

nObject = CreateObject (OBJ_FIREBALL, vclipType, -1, nSegment, position, &vmdIdentityMatrix, size, 
							  CT_EXPLOSION, MT_NONE, RT_FIREBALL, 1);

	if (nObject < 0) {
#if TRACE
		con_printf (1, "Can't create tObject in ObjectCreateExplosionSub.\n");
#endif
		return NULL;
	}

	explObjP = gameData.objs.objects + nObject;
	//now set explosion-specific data
	explObjP->lifeleft = gameData.eff.vClips [0] [vclipType].xTotalTime;
	explObjP->cType.explInfo.nSpawnTime = -1;
	explObjP->cType.explInfo.nDeleteObj = -1;
	explObjP->cType.explInfo.nDeleteTime = -1;

	if (maxdamage > 0) {
		fix dist, force;
		vmsVector pos_hit, vForce;
		fix damage;
		int i, t, id;
		tObject * obj0P = gameData.objs.objects;
					 
		// -- now legal for xBadAss explosions on a tWall. Assert (objP != NULL);

		for (i=0; i<=gameData.objs.nLastObject; i++)	{
			t = obj0P->nType;
			id = obj0P->id;
			//	Weapons used to be affected by xBadAss explosions, but this introduces serious problems.
			//	When a smart bomb blows up, if one of its children goes right towards a nearby tWall, it will
			//	blow up, blowing up all the children.  So I remove it.  MK, 09/11/94
			if ((obj0P != objP) && !(obj0P->flags & OF_SHOULD_BE_DEAD) && 
			    ((t==OBJ_WEAPON && (id==PROXMINE_ID || id==SMARTMINE_ID || id==SMALLMINE_ID)) || 
			     (t == OBJ_CNTRLCEN) || 
			     (t==OBJ_PLAYER) || ((t==OBJ_ROBOT) && (parent >= 0) &&
			     ((gameData.objs.objects [parent].nType != OBJ_ROBOT) || (gameData.objs.objects [parent].id != id))))) {
				dist = VmVecDistQuick (&obj0P->position.vPos, &explObjP->position.vPos);
				// Make damage be from 'maxdamage' to 0.0, where 0.0 is 'maxdistance' away;
				if (dist < maxdistance) {
					if (ObjectToObjectVisibility (explObjP, obj0P, FQ_TRANSWALL)) {

						damage = maxdamage - FixMulDiv (dist, maxdamage, maxdistance);
						force = maxforce - FixMulDiv (dist, maxforce, maxdistance);

						// Find the force vector on the tObject
						VmVecNormalizedDirQuick (&vForce, &obj0P->position.vPos, &explObjP->position.vPos);
						VmVecScale (&vForce, force);
	
						// Find where the point of impact is... (pos_hit)
						VmVecSub (&pos_hit, &explObjP->position.vPos, &obj0P->position.vPos);
						VmVecScale (&pos_hit, FixDiv (obj0P->size, obj0P->size + dist));
	
						switch (obj0P->nType)	{
							case OBJ_WEAPON:
								PhysApplyForce (obj0P, &vForce);

								if (obj0P->id == PROXMINE_ID || obj0P->id == SMARTMINE_ID) {		//prox bombs have chance of blowing up
									if (FixMul (dist, force) > i2f (8000)) {
										KillObject (obj0P);
										ExplodeBadassWeapon (obj0P, &obj0P->position.vPos);
									}
								}
								break;

							case OBJ_ROBOT:
								{
								PhysApplyForce (obj0P, &vForce);

								//	If not a boss, stun for 2 seconds at 32 force, 1 second at 16 force
								if ((objP != NULL) && (!ROBOTINFO (obj0P->id).bossFlag) && (gameData.weapons.info [objP->id].flash)) {
									tAIStatic	*aip = &obj0P->cType.aiInfo;
									int			force_val = f2i (FixDiv (VmVecMagQuick (&vForce) * gameData.weapons.info [objP->id].flash, gameData.time.xFrame)/128) + 2;

									if (explObjP->cType.aiInfo.SKIP_AI_COUNT * gameData.time.xFrame < F1_0) {
										aip->SKIP_AI_COUNT += force_val;
										obj0P->mType.physInfo.rotThrust.p.x = ((d_rand () - 16384) * force_val)/16;
										obj0P->mType.physInfo.rotThrust.p.y = ((d_rand () - 16384) * force_val)/16;
										obj0P->mType.physInfo.rotThrust.p.z = ((d_rand () - 16384) * force_val)/16;
										obj0P->mType.physInfo.flags |= PF_USES_THRUST;

										//@@if (ROBOTINFO (obj0P->id).companion)
										//@@	BuddyMessage ("Daisy, Daisy, Give me...");
									} else
										aip->SKIP_AI_COUNT--;

								}

								//	When a robot gets whacked by a xBadAss force, he looks towards it because robots tend to get blasted from behind.
								{
									vmsVector vNegForce;
									vNegForce.p.x = vForce.p.x * -2 * (7 - gameStates.app.nDifficultyLevel)/8;
									vNegForce.p.y = vForce.p.y * -2 * (7 - gameStates.app.nDifficultyLevel)/8;
									vNegForce.p.z = vForce.p.z * -2 * (7 - gameStates.app.nDifficultyLevel)/8;
									PhysApplyRot (obj0P, &vNegForce);
								}
								if (obj0P->shields >= 0) {
									if (ROBOTINFO (obj0P->id).bossFlag)
										if (bossProps [gameStates.app.bD1Mission] [ROBOTINFO (obj0P->id).bossFlag-BOSS_D2].bInvulKinetic)
											damage /= 4;

									if (ApplyDamageToRobot (obj0P, damage, parent))
										if ((objP != NULL) && (parent == LOCALPLAYER.nObject))
											AddPointsToScore (ROBOTINFO (obj0P->id).scoreValue);
								}

								if ((objP != NULL) && (ROBOTINFO (obj0P->id).companion) && (!gameData.weapons.info [objP->id].flash)) {
									int	i, count;
									char	ouch_str [6*4 + 2];

									count = f2i (damage/8);
									if (count > 4)
										count = 4;
									else if (count <= 0)
										count = 1;
									ouch_str [0] = 0;
									for (i=0; i<count; i++) {
										strcat (ouch_str, TXT_BUDDY_OUCH);
										strcat (ouch_str, " ");
										}
									BuddyMessage (ouch_str);
								}
								break;
								}
							case OBJ_CNTRLCEN:
								if (obj0P->shields >= 0) {
									ApplyDamageToReactor (obj0P, damage, parent);
								}
								break;
							case OBJ_PLAYER:	{
								tObject * killer=NULL;
								vmsVector	vForce2;

								//	Hack!Warning!Test code!
								if ((objP != NULL) && gameData.weapons.info [objP->id].flash && obj0P->id==gameData.multiplayer.nLocalPlayer) {
									int	fe;

									fe = min (F1_0*4, force*gameData.weapons.info [objP->id].flash/32);	//	For four seconds or less

									if (objP->cType.laserInfo.nParentSig == gameData.objs.console->nSignature) {
										fe /= 2;
										force /= 2;
									}
									if (force > F1_0) {
										gameData.render.xFlashEffect = fe;
										PALETTE_FLASH_ADD (PK1 + f2i (PK2*force), PK1 + f2i (PK2*force), PK1 + f2i (PK2*force));
#if TRACE
										con_printf (CONDBG, "force = %7.3f, adding %i\n", f2fl (force), PK1 + f2i (PK2*force));
#endif
									}
								}

								if ((objP != NULL) && (gameData.app.nGameMode & GM_MULTI) && (objP->nType == OBJ_PLAYER)) {
									killer = objP;
								}
								vForce2 = vForce;
								if (parent > -1) {
									killer = gameData.objs.objects + parent;
									if (killer != gameData.objs.console)		// if someone else whacks you, cut force by 2x
										vForce2.p.x /= 2;	
										vForce2.p.y /= 2;	
										vForce2.p.z /= 2;
								}
								vForce2.p.x /= 2;	
								vForce2.p.y /= 2;	
								vForce2.p.z /= 2;

								PhysApplyForce (obj0P, &vForce);
								PhysApplyRot (obj0P, &vForce2);
								if (gameStates.app.nDifficultyLevel == 0)
									damage /= 4;
								if (obj0P->shields >= 0)
									ApplyDamageToPlayer (obj0P, killer, damage);
							}
								break;

							default:
								Int3 ();	//	Illegal tObject nType
						}	// end switch
					} else {
						;
					}	// end if (ObjectToObjectVisibility...
				}	// end if (dist < maxdistance)
			}
			obj0P++;
		}	// end for
	}	// end if (maxdamage...

	return explObjP;

}

//------------------------------------------------------------------------------

tObject *ObjectCreateMuzzleFlash (short nSegment, vmsVector * position, fix size, ubyte vclipType)
{
return ObjectCreateExplosionSub (NULL, nSegment, position, size, vclipType, 0, 0, 0, -1);
}

//------------------------------------------------------------------------------

tObject *ObjectCreateExplosion (short nSegment, vmsVector * position, fix size, ubyte vclipType)
{
return ObjectCreateExplosionSub (NULL, nSegment, position, size, vclipType, 0, 0, 0, -1);
}

//------------------------------------------------------------------------------

tObject *ObjectCreateBadassExplosion (
	tObject *objP, 
	short nSegment, 
	vmsVector *position, 
	fix size, 
	ubyte vclipType, 
	fix maxdamage, 
	fix maxdistance, 
	fix maxforce, 
	short parent)
{
	tObject	*rval = ObjectCreateExplosionSub (objP, nSegment, position, size, vclipType, maxdamage, maxdistance, maxforce, parent);

if ((objP != NULL) && (objP->nType == OBJ_WEAPON))
	CreateSmartChildren (objP, NUM_SMART_CHILDREN);
return rval;
}

//------------------------------------------------------------------------------
//blows up a xBadAss weapon, creating the xBadAss explosion
//return the explosion tObject
tObject *ExplodeBadassWeapon (tObject *objP, vmsVector *pos)
{
	tWeaponInfo *wi = &gameData.weapons.info [objP->id];

Assert (wi->damage_radius);
if ((objP->id == EARTHSHAKER_ID) || (objP->id == ROBOT_EARTHSHAKER_ID))
	ShakerRockStuff ();
DigiLinkSoundToObject (SOUND_BADASS_EXPLOSION, OBJ_IDX (objP), 0, F1_0);
return ObjectCreateBadassExplosion (objP, objP->nSegment, pos, 
                                    wi->impact_size, 
                                    wi->robot_hit_vclip, 
                                    wi->strength [gameStates.app.nDifficultyLevel], 
                                    wi->damage_radius, wi->strength [gameStates.app.nDifficultyLevel], 
                                    objP->cType.laserInfo.nParentObj);
}

//------------------------------------------------------------------------------

tObject *ExplodeBadassObject (tObject *objP, fix damage, fix distance, fix force)
{

	tObject 	*rval = ObjectCreateBadassExplosion (objP, objP->nSegment, &objP->position.vPos, objP->size, 
																 (ubyte) GetExplosionVClip (objP, 0), 
																 damage, distance, force, 
																 OBJ_IDX (objP));
if (rval)
	DigiLinkSoundToObject (SOUND_BADASS_EXPLOSION, OBJ_IDX (rval), 0, F1_0);
return rval;
}

//------------------------------------------------------------------------------
//blows up the tPlayer with a xBadAss explosion
//return the explosion tObject
tObject *ExplodeBadassPlayer (tObject *objP)
{
	return ExplodeBadassObject (objP, F1_0*50, F1_0*40, F1_0*150);
}

//------------------------------------------------------------------------------

inline double VectorVolume (vmsVector *vMin, vmsVector *vMax)
{
return fabs (f2fl (vMax->p.x - vMin->p.x)) *
		 fabs (f2fl (vMax->p.y - vMin->p.y)) *
		 fabs (f2fl (vMax->p.z - vMin->p.z));
}

//------------------------------------------------------------------------------

double ObjectVolume (tObject *objP)
{
	tPolyModel	*pm;
	int			i, j;
	double		size;

if (objP->renderType != RT_POLYOBJ)
	size = 4 * Pi * pow (f2fl (objP->size), 3) / 3;
else {
	size = 0;
	pm = gameData.models.polyModels + objP->rType.polyObjInfo.nModel;
	if (i = objP->rType.polyObjInfo.nSubObjFlags) {
		for (j = 0; i && (j < pm->nModels); i >>= 1, j++)
			if (i & 1)
				size += VectorVolume (pm->subModels.mins + j, pm->subModels.maxs + j);
		}
	else {
		for (j = 0; j < pm->nModels; j++)
			size += VectorVolume (pm->subModels.mins + j, pm->subModels.maxs + j);
		}
	}
return sqrt (size);
}

//------------------------------------------------------------------------------

#define DEBRIS_LIFE (f1_0 * 2)		//lifespan in seconds

fix nDebrisLife [] = {2, 15, 30, 60, 120, 180, 300};

tObject *ObjectCreateDebris (tObject *parentObjP, int nSubObj)
{
	int nObject;
	tObject *debrisP;
	tPolyModel *po;

Assert ((parentObjP->nType == OBJ_ROBOT) || (parentObjP->nType == OBJ_PLAYER));
nObject = CreateObject (
	OBJ_DEBRIS, 0, -1, parentObjP->nSegment, &parentObjP->position.vPos, &parentObjP->position.mOrient, 
	gameData.models.polyModels [parentObjP->rType.polyObjInfo.nModel].subModels.rads [nSubObj], 
	CT_DEBRIS, MT_PHYSICS, RT_POLYOBJ, 1);
if ((nObject < 0) && (gameData.objs.nLastObject >= MAX_OBJECTS - 1)) {
#if TRACE
	con_printf (1, "Can't create tObject in ObjectCreateDebris.\n");
#endif
	Int3 ();
	return NULL;
	}
if (nObject < 0)
	return NULL;				// Not enough debris slots!
debrisP = gameData.objs.objects + nObject;
Assert (nSubObj < 32);
//Set polygon-tObject-specific data
debrisP->rType.polyObjInfo.nModel = parentObjP->rType.polyObjInfo.nModel;
debrisP->rType.polyObjInfo.nSubObjFlags = 1 << nSubObj;
debrisP->rType.polyObjInfo.nTexOverride = parentObjP->rType.polyObjInfo.nTexOverride;
//Set physics data for this tObject
po = gameData.models.polyModels + debrisP->rType.polyObjInfo.nModel;
debrisP->mType.physInfo.velocity.p.x = RAND_MAX/2 - d_rand ();
debrisP->mType.physInfo.velocity.p.y = RAND_MAX/2 - d_rand ();
debrisP->mType.physInfo.velocity.p.z = RAND_MAX/2 - d_rand ();
VmVecScale (&debrisP->mType.physInfo.velocity, F1_0 * 10);
VmVecNormalizeQuick (&debrisP->mType.physInfo.velocity);
VmVecScale (&debrisP->mType.physInfo.velocity, i2f (10 + (30 * d_rand () / RAND_MAX)));
VmVecInc (&debrisP->mType.physInfo.velocity, &parentObjP->mType.physInfo.velocity);
// -- used to be: Notice, not random!VmVecMake (&debrisP->mType.physInfo.rotVel, 10*0x2000/3, 10*0x4000/3, 10*0x7000/3);
#if 0//def _DEBUG
VmVecZero (&debrisP->mType.physInfo.rotVel);
#else
VmVecMake (&debrisP->mType.physInfo.rotVel, d_rand () + 0x1000, d_rand ()*2 + 0x4000, d_rand ()*3 + 0x2000);
#endif
VmVecZero (&debrisP->mType.physInfo.rotThrust);
debrisP->lifeleft = nDebrisLife [gameOpts->render.nDebrisLife] * F1_0 + 3*DEBRIS_LIFE/4 + FixMul (d_rand (), DEBRIS_LIFE);	//	Some randomness, so they don't all go away at the same time.
debrisP->mType.physInfo.mass =
#if 0
	(fix) ((double) parentObjP->mType.physInfo.mass * ObjectVolume (debrisP) / ObjectVolume (parentObjP));
#else
	FixMulDiv (parentObjP->mType.physInfo.mass, debrisP->size, parentObjP->size);
#endif
debrisP->mType.physInfo.drag = gameOpts->render.nDebrisLife ? 256 : 0; //fl2f (0.2);		//parentObjP->mType.physInfo.drag;
if (gameOpts->render.nDebrisLife) {
	debrisP->mType.physInfo.flags |= PF_FREE_SPINNING;
	VmVecScaleFrac (&debrisP->mType.physInfo.rotVel, 1, 3);
	}
return debrisP;
}

// --------------------------------------------------------------------------------------------------------------------

void DrawFireball (tObject *objP)
{
if (objP->lifeleft > 0)
	DrawVClipObject (objP, objP->lifeleft, 0, objP->id, (objP->nType == OBJ_WEAPON) ? gameData.weapons.color + objP->id : NULL);
}

// --------------------------------------------------------------------------------------------------------------------
//	Return true if there is a door here and it is openable
//	It is assumed that the tPlayer has all keys.
int PlayerCanOpenDoor (tSegment *segP, short nSide)
{
	short	nWall, wallType;

	nWall = WallNumP (segP, nSide);

	if (!IS_WALL (nWall))
		return 0;						//	no tWall here.

	wallType = gameData.walls.walls [nWall].nType;
	//	Can't open locked doors.
	if (( (wallType == WALL_DOOR) && (gameData.walls.walls [nWall].flags & WALL_DOOR_LOCKED)) || (wallType == WALL_CLOSED))
		return 0;

	return 1;

}

#define	QUEUE_SIZE	64

// --------------------------------------------------------------------------------------------------------------------
//	Return a tSegment %i segments away from initial tSegment.
//	Returns -1 if can't find a tSegment that distance away.
int PickConnectedSegment (tObject *objP, int max_depth)
{
	int		i;
	int		cur_depth;
	int		start_seg;
	int		head, tail;
	int		seg_queue [QUEUE_SIZE*2];
	sbyte		bVisited [MAX_SEGMENTS_D2X];
	sbyte		depth [MAX_SEGMENTS_D2X];
	sbyte		side_rand [MAX_SIDES_PER_SEGMENT];

	start_seg = objP->nSegment;
	head = 0;
	tail = 0;
	seg_queue [head++] = start_seg;

	memset (bVisited, 0, gameData.segs.nLastSegment+1);
	memset (depth, 0, gameData.segs.nLastSegment+1);
	cur_depth = 0;

	for (i=0; i<MAX_SIDES_PER_SEGMENT; i++)
		side_rand [i] = i;

	//	Now, randomize a bit to start, so we don't always get started in the same direction.
	for (i=0; i<4; i++) {
		int	ind1, temp;

		ind1 = (d_rand () * MAX_SIDES_PER_SEGMENT) >> 15;
		temp = side_rand [ind1];
		side_rand [ind1] = side_rand [i];
		side_rand [i] = temp;
	}


	while (tail != head) {
		int		nSide, count;
		tSegment	*segP;
		int		ind1, ind2, temp;

		if (cur_depth >= max_depth) {
			return seg_queue [tail];
		}

		segP = gameData.segs.segments + seg_queue [tail++];
		tail &= QUEUE_SIZE-1;

		//	to make random, switch a pair of entries in side_rand.
		ind1 = (d_rand () * MAX_SIDES_PER_SEGMENT) >> 15;
		ind2 = (d_rand () * MAX_SIDES_PER_SEGMENT) >> 15;
		temp = side_rand [ind1];
		side_rand [ind1] = side_rand [ind2];
		side_rand [ind2] = temp;

		count = 0;
		for (nSide=ind1; count<MAX_SIDES_PER_SEGMENT; count++) {
			short	snrand, nWall;

			if (nSide == MAX_SIDES_PER_SEGMENT)
				nSide = 0;

			snrand = side_rand [nSide];
			nWall = WallNumP (segP, snrand);
			nSide++;

			if (( (!IS_WALL (nWall)) && (segP->children [snrand] > -1)) || PlayerCanOpenDoor (segP, snrand)) {
				if (!bVisited [segP->children [snrand]]) {
					seg_queue [head++] = segP->children [snrand];
					bVisited [segP->children [snrand]] = 1;
					depth [segP->children [snrand]] = cur_depth+1;
					head &= QUEUE_SIZE-1;
					if (head > tail) {
						if (head == tail + QUEUE_SIZE-1)
							Int3 ();	//	queue overflow.  Make it bigger!
					} else
						if (head+QUEUE_SIZE == tail + QUEUE_SIZE-1)
							Int3 ();	//	queue overflow.  Make it bigger!
				}
			}
		}
		if ((seg_queue [tail] < 0) || (seg_queue [tail] > gameData.segs.nLastSegment)) {
			// -- Int3 ();	//	Something bad has happened.  Queue is trashed.  --MK, 12/13/94
			return -1;
		}
		cur_depth = depth [seg_queue [tail]];
	}
#if TRACE
	con_printf (CONDBG, "...failed at depth %i, returning -1\n", cur_depth);
#endif
	return -1;
}

//------------------------------------------------------------------------------

#define	BASE_NET_DROP_DEPTH	8

int InitObjectCount (tObject *objP)
{
	int	nFree, nTotal, i, j, bFree;
	short	nType = objP->nType;
	short	id = objP->id; 

nFree = nTotal = 0;
for (i = 0, objP = gameData.objs.init; i < gameFileInfo.objects.count; i++, objP++) {
	if ((objP->nType != nType) || (objP->id != id))
		continue;
	nTotal++;
	for (bFree = 1, j = gameData.segs.segments [objP->nSegment].objects; j != -1; j = gameData.objs.objects [j].next)
		if ((gameData.objs.objects [j].nType == nType) && (gameData.objs.objects [j].id == id)) {
			bFree = 0;
			break;
			}
	if (bFree)
		nFree++;
	}
return nFree ? -nFree : nTotal;
}

//------------------------------------------------------------------------------

tObject *FindInitObject (tObject *objP)
{
	int	h, i, j, bUsed, 
			bUseFree, 
			objCount = InitObjectCount (objP);
	short	nType = objP->nType;
	short	id = objP->id; 

// due to gameData.objs.objects being deleted from the tObject list when picked up and recreated when dropped, 
// cannot determine exact respawn tSegment, so randomly chose one from all segments where powerups
// of this nType had initially been placed in the level.
if (!objCount)		//no gameData.objs.objects of this nType had initially been placed in the mine. 
	return NULL;	//can happen with missile packs
d_srand (TimerGetFixedSeconds ());
if ((bUseFree = (objCount < 0)))
	objCount = -objCount;
h = d_rand () % objCount + 1;	
for (i = 0, objP = gameData.objs.init; i < gameFileInfo.objects.count; i++, objP++) {
	if ((objP->nType != nType) || (objP->id != id))
		continue;
	// if the current tSegment does not contain a powerup of the nType being looked for, 
	// return that tSegment
	if (bUseFree) {
		for (bUsed = 0, j = gameData.segs.segments [objP->nSegment].objects; j != -1; j = gameData.objs.objects [j].next)
			if ((gameData.objs.objects [j].nType == nType) && (gameData.objs.objects [j].id == id)) {
				bUsed = 1;
				break;
				}
		if (bUsed)
			continue;
		}
	if (!--h)
		return objP;
	}
return NULL;
}

//	------------------------------------------------------------------------------------------------------

int ReturnFlagHome (tObject *objP)
{
	tObject	*initObjP;

if (gameStates.app.bHaveExtraGameInfo [1] && extraGameInfo [1].bEnhancedCTF) {
	if (gameData.segs.segment2s [objP->nSegment].special == ((objP->id == POW_REDFLAG) ? SEGMENT_IS_GOAL_RED : SEGMENT_IS_GOAL_BLUE))
		return objP->nSegment;
	if ((initObjP = FindInitObject (objP))) {
	//objP->nSegment = initObjP->nSegment;
		objP->position.vPos = initObjP->position.vPos;
		objP->position.mOrient = initObjP->position.mOrient;
		RelinkObject (OBJ_IDX (objP), initObjP->nSegment);
		HUDInitMessage (TXT_FLAG_RETURN);
		DigiPlaySample (SOUND_DROP_WEAPON, F1_0);
		MultiSendReturnFlagHome (OBJ_IDX (objP));
		}
	}
return objP->nSegment;
}

//	------------------------------------------------------------------------------------------------------

int FindFuelCen (int nSegment)
{
	int	i;

for (i = 0; i < gameData.matCens.nFuelCenters; i++)
	if (gameData.matCens.fuelCenters [i].nSegment == nSegment)
		return i;
return -1;
}

//-----------------------------------------------------------------------------

int CountRooms (void)
{
	int		i;
	xsegment	*segP = gameData.segs.xSegments;

memset (gameData.entropy.nRoomOwners, 0xFF, sizeof (gameData.entropy.nRoomOwners));
memset (gameData.entropy.nTeamRooms, 0, sizeof (gameData.entropy.nTeamRooms));
for (i = 0; i <= gameData.segs.nLastSegment; i++, segP++)
	if ((segP->owner >= 0) && (segP->group >= 0) && 
		 /* (segP->group <= N_MAX_ROOMS) &&*/ (gameData.entropy.nRoomOwners [segP->group] < 0))
		gameData.entropy.nRoomOwners [segP->group] = segP->owner;
for (i = 0; i < N_MAX_ROOMS; i++)
	if (gameData.entropy.nRoomOwners [i] >= 0) {
		gameData.entropy.nTeamRooms [gameData.entropy.nRoomOwners [i]]++;
		gameData.entropy.nTotalRooms++;
		}
return gameData.entropy.nTotalRooms;
}

//-----------------------------------------------------------------------------

void ConquerRoom (int newOwner, int oldOwner, int roomId)
{
	int			f, h, i, j, jj, k, kk, nObject;
	tSegment		*segP;
	xsegment		*xsegP;
	tObject		*objP;
	tFuelCenInfo	*fuelP;
	short			virusGens [MAX_FUEL_CENTERS];

// this loop with
// a) convert all segments with group 'roomId' to newOwner 'newOwner'
// b) count all virus centers newOwner 'newOwner' was owning already, 
//    j holding the total number of virus centers, jj holding the number of 
//    virus centers converted from fuel/repair centers
// c) count all virus centers newOwner 'newOwner' has just conquered, 
//    k holding the total number of conquered virus centers, kk holding the number of 
//    conquered virus centers converted from fuel/repair centers
// So if j > jj or k < kk, all virus centers placed in virusGens can be converted
// back to their original nType
gameData.entropy.nTeamRooms [oldOwner]--;
gameData.entropy.nTeamRooms [newOwner]++;
for (i = 0, j = jj = 0, k = kk = MAX_FUEL_CENTERS, segP = gameData.segs.segments, xsegP = gameData.segs.xSegments; 
	  i <= gameData.segs.nLastSegment; 
	  i++, segP++, xsegP++) {
	if ((xsegP->group == roomId) && (xsegP->owner == oldOwner)) {
		xsegP->owner = newOwner;
		ChangeSegmentTexture (i, oldOwner);
		if (gameData.segs.segment2s [i].special == SEGMENT_IS_ROBOTMAKER) {
			--k;
			if (extraGameInfo [1].entropy.bRevertRooms && (-1 < (f = FindFuelCen (i))) &&
				 (gameData.matCens.origStationTypes [f] != SEGMENT_IS_NOTHING))
				virusGens [--kk] = f;
			for (nObject = segP->objects; nObject >= 0; nObject = objP->next) {
				objP = gameData.objs.objects + nObject;
				if ((objP->nType == OBJ_POWERUP) && (objP->nType == POW_ENTROPY_VIRUS))
					objP->matCenCreator = newOwner;
				}
			}
		}
	else {
		if ((xsegP->owner == newOwner) && (gameData.segs.segment2s [i].special == SEGMENT_IS_ROBOTMAKER)) {
			j++;
			if (extraGameInfo [1].entropy.bRevertRooms && (-1 < (f = FindFuelCen (i))) &&
				 (gameData.matCens.origStationTypes [f] != SEGMENT_IS_NOTHING))
				virusGens [jj++] = f;
			}
		}
	}
// if the new owner has conquered a virus generator, turn all generators that had
// been turned into virus generators because the new owner had lost his last virus generators
// back to their original nType
if (extraGameInfo [1].entropy.bRevertRooms && (jj + (MAX_FUEL_CENTERS - kk)) && ((j > jj) || (k < kk))) {
	if (kk < MAX_FUEL_CENTERS) {
		memcpy (virusGens + jj, virusGens + kk, (MAX_FUEL_CENTERS - kk) * sizeof (short));
		jj += (MAX_FUEL_CENTERS - kk);
		for (j = 0; j < jj; j++) {
			fuelP = gameData.matCens.fuelCenters + virusGens [j];
			h = fuelP->nSegment;
			gameData.segs.segment2s [h].special = gameData.matCens.origStationTypes [fuelP - gameData.matCens.fuelCenters];
			FuelCenCreate (gameData.segs.segments + h, gameData.segs.segment2s [h].special);
			ChangeSegmentTexture (h, newOwner);
			}
		}
	}

// check if the other newOwner's last virus center has been conquered
// if so, find a fuel or repair center owned by that and turn it into a virus generator
// preferrably convert repair centers
for (i = 0, h = -1, xsegP = gameData.segs.xSegments; i <= gameData.segs.nLastSegment; i++, xsegP++)
	if (xsegP->owner == oldOwner) 
		switch (gameData.segs.segment2s [i].special) {
			case SEGMENT_IS_ROBOTMAKER:
				return;
			case SEGMENT_IS_FUELCEN:
				if (h < 0)
					h = i;
				break;
			case SEGMENT_IS_REPAIRCEN:
				if ((h < 0) || (gameData.segs.segment2s [h].special == SEGMENT_IS_FUELCEN))
					h = i;
			}
if (h < 0)
	return;
i = gameData.segs.segment2s [h].special;
gameData.segs.segment2s [h].special = SEGMENT_IS_ROBOTMAKER;
BotGenCreate (gameData.segs.segments + h, i);
ChangeSegmentTexture (h, newOwner);
}

//	------------------------------------------------------------------------------------------------------

void StartConquerWarning (void)
{
if (extraGameInfo [1].entropy.bDoConquerWarning) {
	gameStates.sound.nConquerWarningSoundChannel = DigiPlaySample (SOUND_CONTROL_CENTER_WARNING_SIREN, F3_0);
	MultiSendConquerWarning ();
	gameStates.entropy.bConquerWarning = 1;
	}
}

//	------------------------------------------------------------------------------------------------------

void StopConquerWarning (void)
{
if (gameStates.entropy.bConquerWarning) {
	if (gameStates.sound.nConquerWarningSoundChannel >= 0)
		DigiStopSound (gameStates.sound.nConquerWarningSoundChannel);
	MultiSendStopConquerWarning ();
	gameStates.entropy.bConquerWarning = 0;
	}
}

//	------------------------------------------------------------------------------------------------------

int CheckConquerRoom (xsegment *segP)
{
	tPlayer	*playerP = gameData.multiplayer.players + gameData.multiplayer.nLocalPlayer;
	int		team = GetTeam (gameData.multiplayer.nLocalPlayer) + 1;
	time_t	t;

gameStates.entropy.bConquering = 0;
if (!(gameData.app.nGameMode & GM_ENTROPY))
	return 0;
#if 0
if (gameStates.entropy.nTimeLastMoved < 0) {
	HUDMessage (0, "you are moving");
	StopConquerWarning ();
	return 0;
	}
if (playerP->shields < 0) {
	HUDMessage (0, "you are dead");
	StopConquerWarning ();
	return 0;
	}
if (playerP->secondaryAmmo [PROXIMITY_INDEX] < extraGameInfo [1].nCaptureVirusLimit) {
	HUDMessage (0, "too few viruses");
	StopConquerWarning ();
	return 0;
	}
if (segP->owner < 0) {
	HUDMessage (0, "neutral room");
	StopConquerWarning ();
	return 0;
	}
if (segP->owner == team) {
	HUDMessage (0, "own room");
	StopConquerWarning ();
	return 0;
	}
#else
if ((gameStates.entropy.nTimeLastMoved < 0) || 
	 (playerP->shields < 0) || 
	 (playerP->secondaryAmmo [PROXIMITY_INDEX] < extraGameInfo [1].entropy.nCaptureVirusLimit) ||
	 (segP->owner < 0) || 
	 (segP->owner == team)) {
	StopConquerWarning ();
	return 0;
	}
#endif
t = SDL_GetTicks ();
if (!gameStates.entropy.nTimeLastMoved)
	gameStates.entropy.nTimeLastMoved = (int) t;
if (t - gameStates.entropy.nTimeLastMoved < extraGameInfo [1].entropy.nCaptureTimeLimit * 1000) {
	gameStates.entropy.bConquering = 1;
	if (segP->owner > 0)
		StartConquerWarning ();
	return 0;
	}
StopConquerWarning ();
if (segP->owner)
	MultiSendCaptureBonus ((char) gameData.multiplayer.nLocalPlayer);
playerP->secondaryAmmo [PROXIMITY_INDEX] -= extraGameInfo [1].entropy.nCaptureVirusLimit;
if (playerP->secondaryAmmo [SMART_MINE_INDEX] > extraGameInfo [1].entropy.nBashVirusCapacity)
	playerP->secondaryAmmo [SMART_MINE_INDEX] -= extraGameInfo [1].entropy.nBashVirusCapacity;
else
	playerP->secondaryAmmo [SMART_MINE_INDEX] = 0;
MultiSendConquerRoom ((char) team, (char) segP->owner, (char) segP->group);
ConquerRoom ((char) team, (char) segP->owner, (char) segP->group);
return 1;
}

//	------------------------------------------------------------------------------------------------------
//	Choose tSegment to drop a powerup in.
//	For all active net players, try to create a N tSegment path from the player.  If possible, return that
//	tSegment.  If not possible, try another player.  After a few tries, use a random tSegment.
//	Don't drop if control center in tSegment.
int ChooseDropSegment (tObject *objP, int *pbFixedPos, int nDropState)
{
	int			pnum = 0;
	short			nSegment = -1;
	int			nCurDropDepth;
	int			special, count;
	short			player_seg;
	vmsVector	tempv, *player_pos;
	fix			nDist;
	int			bUseInitSgm = 
						objP &&
						(EGI_FLAG (bFixedRespawns, 0, 0, 0) || 
						 (EGI_FLAG (bEnhancedCTF, 0, 0, 0) && 
						 (objP->nType == OBJ_POWERUP) && ((objP->id == POW_BLUEFLAG) || (objP->id == POW_REDFLAG))));
#if TRACE
con_printf (CONDBG, "ChooseDropSegment:");
#endif
if (bUseInitSgm) {
	tObject *initObjP = FindInitObject (objP);
	if (initObjP) {
		*pbFixedPos = 1;
		objP->position.vPos = initObjP->position.vPos;
		objP->position.mOrient = initObjP->position.mOrient;
		return initObjP->nSegment;
		}
	}
if (pbFixedPos)
	*pbFixedPos = 0;
d_srand (TimerGetFixedSeconds ());
nCurDropDepth = BASE_NET_DROP_DEPTH + ((d_rand () * BASE_NET_DROP_DEPTH*2) >> 15);
player_pos = &gameData.objs.objects [LOCALPLAYER.nObject].position.vPos;
player_seg = gameData.objs.objects [LOCALPLAYER.nObject].nSegment;
while ((nSegment == -1) && (nCurDropDepth > BASE_NET_DROP_DEPTH/2)) {
	pnum = (d_rand () * gameData.multiplayer.nPlayers) >> 15;
	count = 0;
	while ((count < gameData.multiplayer.nPlayers) && 
				 ((gameData.multiplayer.players [pnum].connected == 0) || (pnum==gameData.multiplayer.nLocalPlayer) || ((gameData.app.nGameMode & (GM_TEAM|GM_CAPTURE|GM_ENTROPY)) && 
				 (GetTeam (pnum)==GetTeam (gameData.multiplayer.nLocalPlayer))))) {
		pnum = (pnum+1)%gameData.multiplayer.nPlayers;
		count++;
		}
	if (count == gameData.multiplayer.nPlayers) {
		//if can't valid non-tPlayer person, use the tPlayer
		pnum = gameData.multiplayer.nLocalPlayer;
		//return (d_rand () * gameData.segs.nLastSegment) >> 15;
		}
	nSegment = PickConnectedSegment (gameData.objs.objects + gameData.multiplayer.players [pnum].nObject, nCurDropDepth);
#if TRACE
	con_printf (CONDBG, " %d", nSegment);
#endif
	if (nSegment == -1) {
		nCurDropDepth--;
		continue;
		}
	special = gameData.segs.segment2s [nSegment].special;
	if ((special == SEGMENT_IS_CONTROLCEN) ||
		 (special == SEGMENT_IS_BLOCKED) ||
		 (special == SEGMENT_IS_GOAL_BLUE) ||
		 (special == SEGMENT_IS_GOAL_RED) ||
		 (special == SEGMENT_IS_TEAM_BLUE) ||
		 (special == SEGMENT_IS_TEAM_RED))
		nSegment = -1;
	else {	//don't drop in any children of control centers
		int i;
		for (i=0;i<6;i++) {
			int ch = gameData.segs.segments [nSegment].children [i];
			if (IS_CHILD (ch) && gameData.segs.segment2s [ch].special == SEGMENT_IS_CONTROLCEN) {
				nSegment = -1;
				break;
				}
			}
		}
	//bail if not far enough from original position
	if (nSegment != -1) {
		COMPUTE_SEGMENT_CENTER_I (&tempv, nSegment);
		nDist = FindConnectedDistance (player_pos, player_seg, &tempv, nSegment, -1, WID_FLY_FLAG);
		if ((nDist >= 0) && (nDist < i2f (20) * nCurDropDepth)) {
			nSegment = -1;
			}
		}
	nCurDropDepth--;
}
#if TRACE
if (nSegment != -1)
	con_printf (CONDBG, " dist=%x\n", nDist);
#endif
if (nSegment == -1) {
#if TRACE
	con_printf (1, "Warning: Unable to find a connected tSegment.  Picking a random one.\n");
#endif
	return (d_rand () * gameData.segs.nLastSegment) >> 15;
	}
else
	return nSegment;
}

//	------------------------------------------------------------------------------------------------------

void DropPowerups (void)
{
	short	h = gameData.objs.nFirstDropped, i;

while (h >= 0) {
	i = h;
	h = gameData.objs.dropInfo [i].nNextPowerup;
	if (!MaybeDropNetPowerup (i, gameData.objs.dropInfo [i].nPowerupType, CHECK_DROP))
		break;
	}
}

//	------------------------------------------------------------------------------------------------------

void RespawnDestroyedWeapon (short nObject)
{
	int	h = gameData.objs.nFirstDropped, i;

while (h >= 0) {
	i = h;
	h = gameData.objs.dropInfo [i].nNextPowerup;
	if ((gameData.objs.dropInfo [i].nObject == nObject) &&
		 (gameData.objs.dropInfo [i].nDropTime < 0)) {
		gameData.objs.dropInfo [i].nDropTime = 0;
		MaybeDropNetPowerup (i, gameData.objs.dropInfo [i].nPowerupType, CHECK_DROP);
		}
	}
}

//	------------------------------------------------------------------------------------------------------

static int AddDropInfo (void)
{
	int	h;

if (gameData.objs.nFreeDropped < 0)
	return -1;
h = gameData.objs.nFreeDropped;
gameData.objs.nFreeDropped = gameData.objs.dropInfo [h].nNextPowerup;
gameData.objs.dropInfo [h].nPrevPowerup = gameData.objs.nLastDropped;
gameData.objs.dropInfo [h].nNextPowerup = -1;
if (gameData.objs.nLastDropped >= 0)
	gameData.objs.dropInfo [gameData.objs.nLastDropped].nNextPowerup = h;
else
	gameData.objs.nFirstDropped =
	gameData.objs.nLastDropped = h;
gameData.objs.nDropped++;
return h;
}

//	------------------------------------------------------------------------------------------------------

static void DelDropInfo (int h)
{
	int	i, j;

if (h < 0)
	return;
i = gameData.objs.dropInfo [h].nPrevPowerup;
j = gameData.objs.dropInfo [h].nNextPowerup;
if (i < 0)
	gameData.objs.nFirstDropped = j;
else
	gameData.objs.dropInfo [i].nNextPowerup = j;
if (j < 0)
	gameData.objs.nLastDropped = i;
else
	gameData.objs.dropInfo [j].nPrevPowerup = i;
gameData.objs.dropInfo [h].nNextPowerup = gameData.objs.nFreeDropped;
gameData.objs.nFreeDropped = h;
gameData.objs.nDropped--;
}

//	------------------------------------------------------------------------------------------------------
//	Drop cloak powerup if in a network game.

int MaybeDropNetPowerup (short nObject, int nPowerupType, int nDropState)
{
if (EGI_FLAG (bImmortalPowerups, 0, 0, 0) || 
		 ((gameData.app.nGameMode & GM_MULTI) && !(gameData.app.nGameMode & GM_MULTI_COOP))) {
	short	nSegment;
	int h, bFixedPos = 0;
	vmsVector vNewPos;

	MultiSendWeapons (1);
#if 0
	if ((gameData.app.nGameMode & GM_NETWORK) && (nDropState < CHECK_DROP) && (nPowerupType >= 0)) {
		if (gameData.multiplayer.powerupsInMine [nPowerupType] >= gameData.multiplayer.maxPowerupsAllowed [nPowerupType])
			return 0;
		}
#endif
	if (gameData.reactor.bDestroyed || gameStates.app.bEndLevelSequence)
		return 0;
	gameData.multigame.create.nLoc = 0;
	if (gameStates.app.bHaveExtraGameInfo [IsMultiGame] && (extraGameInfo [IsMultiGame].nSpawnDelay != 0)) {
		if (nDropState == CHECK_DROP) {
			if ((gameData.objs.dropInfo [nObject].nDropTime < 0) ||
				 (gameStates.app.nSDLTicks - gameData.objs.dropInfo [nObject].nDropTime < 
				  extraGameInfo [IsMultiGame].nSpawnDelay))
				return 0;
			nDropState = EXEC_DROP;
			}
		else if (nDropState == INIT_DROP) {
			if (0 > (h = AddDropInfo ()))
				return 0;
			gameData.objs.dropInfo [h].nObject = nObject;
			gameData.objs.dropInfo [h].nPowerupType = nPowerupType;
			gameData.objs.dropInfo [h].nDropTime = 
				 (extraGameInfo [IsMultiGame].nSpawnDelay < 0) ? -1 : gameStates.app.nSDLTicks;
			return 0;
			}
		if (nDropState == EXEC_DROP) {
			DelDropInfo (nObject);
			}
		}
	nObject = CallObjectCreateEgg (gameData.objs.objects + LOCALPLAYER.nObject, 
											 1, OBJ_POWERUP, nPowerupType);
	if (nObject < 0)
		return 0;
	nSegment = ChooseDropSegment (gameData.objs.objects + nObject, &bFixedPos, nDropState);
	if (bFixedPos)
		vNewPos = gameData.objs.objects [nObject].position.vPos;
	else
		PickRandomPointInSeg (&vNewPos, nSegment);
	MultiSendCreatePowerup (nPowerupType, nSegment, nObject, &vNewPos);
	if (!bFixedPos)
		gameData.objs.objects [nObject].position.vPos = vNewPos;
	VmVecZero (&gameData.objs.objects [nObject].mType.physInfo.velocity);
	RelinkObject (nObject, nSegment);
	ObjectCreateExplosion (nSegment, &vNewPos, i2f (5), VCLIP_POWERUP_DISAPPEARANCE);
	return 1;
	}
return 0;
}

//	------------------------------------------------------------------------------------------------------
//	Return true if current tSegment contains some tObject.
int SegmentContainsObject (int objType, int obj_id, int nSegment)
{
	int	nObject;

if (nSegment == -1)
	return 0;
nObject = gameData.segs.segments [nSegment].objects;
while (nObject != -1)
	if ((gameData.objs.objects [nObject].nType == objType) && (gameData.objs.objects [nObject].id == obj_id))
		return 1;
	else
		nObject = gameData.objs.objects [nObject].next;
return 0;
}

//	------------------------------------------------------------------------------------------------------
int ObjectNearbyAux (int nSegment, int objectType, int object_id, int depth)
{
	int	i, seg2;

if (depth == 0)
	return 0;
if (SegmentContainsObject (objectType, object_id, nSegment))
	return 1;
for (i = 0; i < MAX_SIDES_PER_SEGMENT; i++) {
	seg2 = gameData.segs.segments [nSegment].children [i];
	if (seg2 != -1)
		if (ObjectNearbyAux (seg2, objectType, object_id, depth-1))
			return 1;
		}
	return 0;
}


//	------------------------------------------------------------------------------------------------------
//	Return true if some powerup is nearby (within 3 segments).
int WeaponNearby (tObject *objP, int weapon_id)
{
	return ObjectNearbyAux (objP->nSegment, OBJ_POWERUP, weapon_id, 3);
}

//	------------------------------------------------------------------------------------------------------

void MaybeReplacePowerupWithEnergy (tObject *delObjP)
{
	int	weapon_index=-1;

	if (delObjP->containsType != OBJ_POWERUP)
		return;

	if (delObjP->containsId == POW_CLOAK) {
		if (WeaponNearby (delObjP, delObjP->containsId)) {
#if TRACE
			con_printf (CONDBG, "Bashing cloak into nothing because there's one nearby.\n");
#endif
			delObjP->containsCount = 0;
		}
		return;
	}
	switch (delObjP->containsId) {
		case POW_VULCAN:			
			weapon_index = VULCAN_INDEX;		
			break;
		case POW_GAUSS:			
			weapon_index = GAUSS_INDEX;		
			break;
		case POW_SPREADFIRE:	
			weapon_index = SPREADFIRE_INDEX;	
			break;
		case POW_PLASMA:			
			weapon_index = PLASMA_INDEX;		
			break;
		case POW_FUSION:			
			weapon_index = FUSION_INDEX;		
			break;
		case POW_HELIX:			
			weapon_index = HELIX_INDEX;		
			break;
		case POW_PHOENIX:		
			weapon_index = PHOENIX_INDEX;		
			break;
		case POW_OMEGA:			
			weapon_index = OMEGA_INDEX;		
			break;

	}

	//	Don't drop vulcan ammo if tPlayer maxed out.
	if (( (weapon_index == VULCAN_INDEX) || (delObjP->containsId == POW_VULCAN_AMMO)) && (LOCALPLAYER.primaryAmmo [VULCAN_INDEX] >= VULCAN_AMMO_MAX))
		delObjP->containsCount = 0;
	else if (( (weapon_index == GAUSS_INDEX) || (delObjP->containsId == POW_VULCAN_AMMO)) && (LOCALPLAYER.primaryAmmo [VULCAN_INDEX] >= VULCAN_AMMO_MAX))
		delObjP->containsCount = 0;
	else if (weapon_index != -1) {
		if ((PlayerHasWeapon (weapon_index, 0, -1) & HAS_WEAPON_FLAG) || 
			 WeaponNearby (delObjP, delObjP->containsId)) {
			if (d_rand () > 16384) {
				delObjP->containsType = OBJ_POWERUP;
				if (weapon_index == VULCAN_INDEX) {
					delObjP->containsId = POW_VULCAN_AMMO;
				} else if (weapon_index == GAUSS_INDEX) {
					delObjP->containsId = POW_VULCAN_AMMO;
				} else {
					delObjP->containsId = POW_ENERGY;
				}
			} else {
				delObjP->containsType = OBJ_POWERUP;
				delObjP->containsId = POW_SHIELD_BOOST;
			}
		}
	} else if (delObjP->containsId == POW_QUADLASER)
		if ((LOCALPLAYER.flags & PLAYER_FLAGS_QUAD_LASERS) || WeaponNearby (delObjP, delObjP->containsId)) {
			if (d_rand () > 16384) {
				delObjP->containsType = OBJ_POWERUP;
				delObjP->containsId = POW_ENERGY;
			} else {
				delObjP->containsType = OBJ_POWERUP;
				delObjP->containsId = POW_SHIELD_BOOST;
			}
		}

	//	If this robot was gated in by the boss and it now contains energy, make it contain nothing, 
	//	else the room gets full of energy.
	if ((delObjP->matCenCreator == BOSS_GATE_MATCEN_NUM) && (delObjP->containsId == POW_ENERGY) && (delObjP->containsType == OBJ_POWERUP)) {
#if TRACE
		con_printf (CONDBG, "Converting energy powerup to nothing because robot %i gated in by boss.\n", OBJ_IDX (delObjP));
#endif
		delObjP->containsCount = 0;
	}

	// Change multiplayer extra-lives into invulnerability
	if ((gameData.app.nGameMode & GM_MULTI) && (delObjP->containsId == POW_EXTRA_LIFE))
	{
		delObjP->containsId = POW_INVUL;
	}
}

//------------------------------------------------------------------------------

int DropPowerup (ubyte nType, ubyte id, short owner, int num, vmsVector *init_vel, vmsVector *pos, short nSegment)
{
	short			nObject=-1;
	tObject		*objP;
	vmsVector	new_velocity, vNewPos;
	fix			old_mag;
   int			count;

switch (nType) {
	case OBJ_POWERUP:
		for (count = 0; count < num; count++) {
			int	rand_scale;
			new_velocity = *init_vel;
			old_mag = VmVecMagQuick (init_vel);

			//	We want powerups to move more in network mode.
			if ((gameData.app.nGameMode & GM_MULTI) && !(gameData.app.nGameMode & GM_MULTI_ROBOTS)) {
				rand_scale = 4;
				//	extra life powerups are converted to invulnerability in multiplayer, for what is an extra life, anyway?
				if (id == POW_EXTRA_LIFE)
					id = POW_INVUL;
				}
			else
				rand_scale = 2;
			new_velocity.p.x += FixMul (old_mag+F1_0*32, d_rand ()*rand_scale - 16384*rand_scale);
			new_velocity.p.y += FixMul (old_mag+F1_0*32, d_rand ()*rand_scale - 16384*rand_scale);
			new_velocity.p.z += FixMul (old_mag+F1_0*32, d_rand ()*rand_scale - 16384*rand_scale);
			// Give keys zero velocity so they can be tracked better in multi
			if ((gameData.app.nGameMode & GM_MULTI) && 
				 (( (id >= POW_KEY_BLUE) && (id <= POW_KEY_GOLD)) || (id == POW_MONSTERBALL)))
				VmVecZero (&new_velocity);
			vNewPos = *pos;

			if (gameData.app.nGameMode & GM_MULTI) {	
				if (gameData.multigame.create.nLoc >= MAX_NET_CREATE_OBJECTS) {
#if TRACE
					con_printf (1, "Not enough slots to drop all powerups!\n");
#endif
					return -1;
					}
#if 0
				if (TooManyPowerups (id)) {
#if TRACE
					con_printf (1, "More powerups of nType %d in mine than at start of game!\n", id);
#endif
					return -1;
					}
#endif					
				if ((gameData.app.nGameMode & GM_NETWORK) && networkData.nStatus == NETSTAT_ENDLEVEL)
					return -1;
				}
			nObject = CreateObject (nType, id, owner, nSegment, &vNewPos, &vmdIdentityMatrix, gameData.objs.pwrUp.info [id].size, 
										  CT_POWERUP, MT_PHYSICS, RT_POWERUP, 0);
			if (nObject < 0) {
#if TRACE
				con_printf (1, "Can't create tObject in ObjectCreateEgg.  Aborting.\n");
#endif
				Int3 ();
				return nObject;
				}
			if (gameData.app.nGameMode & GM_MULTI)
				gameData.multigame.create.nObjNums [gameData.multigame.create.nLoc++] = nObject;
			objP = gameData.objs.objects + nObject;
			objP->mType.physInfo.velocity = new_velocity;
			objP->mType.physInfo.drag = 512;	//1024;
			objP->mType.physInfo.mass = F1_0;
			objP->mType.physInfo.flags = PF_BOUNCE;
			objP->rType.vClipInfo.nClipIndex = gameData.objs.pwrUp.info [objP->id].nClipIndex;
			objP->rType.vClipInfo.xFrameTime = gameData.eff.vClips [0] [objP->rType.vClipInfo.nClipIndex].xFrameTime;
			objP->rType.vClipInfo.nCurFrame = 0;

			switch (objP->id) {
				case POW_CONCUSSION_1:
				case POW_CONCUSSION_4:
				case POW_SHIELD_BOOST:
				case POW_ENERGY:
					objP->lifeleft = (d_rand () + F1_0*3) * 64;		//	Lives for 3 to 3.5 binary minutes (a binary minute is 64 seconds)
					if (gameData.app.nGameMode & GM_MULTI)
						objP->lifeleft /= 2;
					break;
				default:
//						if (gameData.app.nGameMode & GM_MULTI)
//							objP->lifeleft = (d_rand () + F1_0*3) * 64;		//	Lives for 5 to 5.5 binary minutes (a binary minute is 64 seconds)
					break;
				}
			}
		break;

	case OBJ_ROBOT:
		for (count=0; count<num; count++) {
			int	rand_scale;
			new_velocity = *init_vel;
			old_mag = VmVecMagQuick (init_vel);
			VmVecNormalizeQuick (&new_velocity);
			//	We want powerups to move more in network mode.
			rand_scale = 2;
			new_velocity.p.x += (d_rand ()-16384)*2;
			new_velocity.p.y += (d_rand ()-16384)*2;
			new_velocity.p.z += (d_rand ()-16384)*2;
			VmVecNormalizeQuick (&new_velocity);
			VmVecScale (&new_velocity, (F1_0*32 + old_mag) * rand_scale);
			vNewPos = *pos;
			//	This is dangerous, could be outside mine.
//				vNewPos.x += (d_rand ()-16384)*8;
//				vNewPos.y += (d_rand ()-16384)*7;
//				vNewPos.z += (d_rand ()-16384)*6;
			nObject = CreateObject (OBJ_ROBOT, id, -1, nSegment, &vNewPos, &vmdIdentityMatrix, 
										 gameData.models.polyModels [ROBOTINFO (id).nModel].rad, 
										 CT_AI, MT_PHYSICS, RT_POLYOBJ, 1);
			if (nObject < 0) {
#if TRACE
				con_printf (1, "Can't create tObject in ObjectCreateEgg, robots.  Aborting.\n");
#endif
				Int3 ();
				return nObject;
				}
			if (gameData.app.nGameMode & GM_MULTI)
				gameData.multigame.create.nObjNums [gameData.multigame.create.nLoc++] = nObject;
			objP = &gameData.objs.objects [nObject];
			//Set polygon-tObject-specific data
			objP->rType.polyObjInfo.nModel = ROBOTINFO (objP->id).nModel;
			objP->rType.polyObjInfo.nSubObjFlags = 0;
			//set Physics info
			objP->mType.physInfo.velocity = new_velocity;
			objP->mType.physInfo.mass = ROBOTINFO (objP->id).mass;
			objP->mType.physInfo.drag = ROBOTINFO (objP->id).drag;
			objP->mType.physInfo.flags |= (PF_LEVELLING);
			objP->shields = ROBOTINFO (objP->id).strength;
			objP->cType.aiInfo.behavior = AIB_NORMAL;
			gameData.ai.localInfo [nObject].playerAwarenessType = PA_WEAPON_ROBOT_COLLISION;
			gameData.ai.localInfo [nObject].playerAwarenessTime = F1_0*3;
			objP->cType.aiInfo.CURRENT_STATE = AIS_LOCK;
			objP->cType.aiInfo.GOAL_STATE = AIS_LOCK;
			objP->cType.aiInfo.REMOTE_OWNER = -1;
			if (ROBOTINFO (id).bossFlag)
				AddBoss (nObject);
			}
		// At JasenW's request, robots which contain robots
		// sometimes drop shields.
		if (d_rand () > 16384)
			DropPowerup (OBJ_POWERUP, POW_SHIELD_BOOST, -1, 1, init_vel, pos, nSegment);
		break;

	default:
		Error ("Error: Illegal nType (%i) in tObject spawning.\n", nType);
	}
return nObject;
}

// ----------------------------------------------------------------------------
// Returns created tObject number.
// If tObject dropped by tPlayer, set flag.
int ObjectCreateEgg (tObject *objP)
{
	int	rval;

if (!(gameData.app.nGameMode & GM_MULTI) & (objP->nType != OBJ_PLAYER)) {
	if (objP->containsType == OBJ_POWERUP) {
		if (objP->containsId == POW_SHIELD_BOOST) {
			if (LOCALPLAYER.shields >= i2f (100)) {
				if (d_rand () > 16384) {
#if TRACE
					con_printf (CONDBG, "Not dropping shield!\n");
#endif
					return -1;
					}
				} 
			else  if (LOCALPLAYER.shields >= i2f (150)) {
				if (d_rand () > 8192) {
#if TRACE
					con_printf (CONDBG, "Not dropping shield!\n");
#endif
					return -1;
					}
				}
			} 
		else if (objP->containsId == POW_ENERGY) {
			if (LOCALPLAYER.energy >= i2f (100)) {
				if (d_rand () > 16384) {
#if TRACE
					con_printf (CONDBG, "Not dropping energy!\n");
#endif
					return -1;
					}
				} 
			else if (LOCALPLAYER.energy >= i2f (150)) {
				if (d_rand () > 8192) {
#if TRACE
					con_printf (CONDBG, "Not dropping energy!\n");
#endif
					return -1;
					}
				}
			}
		}
	}

rval = DropPowerup (
	objP->containsType, 
	 (ubyte) objP->containsId, 
	 (short) (
		 ((gameData.app.nGameMode & GM_ENTROPY) && 
		 (objP->containsType == OBJ_POWERUP) && 
		 (objP->containsId == POW_ENTROPY_VIRUS) && 
		 (objP->nType == OBJ_PLAYER)) ? 
		GetTeam (objP->id) + 1 : -1
		), 
	objP->containsCount, 
	&objP->mType.physInfo.velocity, 
	&objP->position.vPos, objP->nSegment);
if (rval >= 0) {
	if ((objP->nType == OBJ_PLAYER) && (objP->id == gameData.multiplayer.nLocalPlayer))
		gameData.objs.objects [rval].flags |= OF_PLAYER_DROPPED;
	if (objP->nType == OBJ_ROBOT && objP->containsType==OBJ_POWERUP) {
		if (objP->containsId==POW_VULCAN || objP->containsId==POW_GAUSS)
			gameData.objs.objects [rval].cType.powerupInfo.count = VULCAN_WEAPON_AMMO_AMOUNT;
		else if (objP->containsId==POW_OMEGA)
			gameData.objs.objects [rval].cType.powerupInfo.count = MAX_OMEGA_CHARGE;
		}
	}
return rval;
}

// -- extern int Items_destroyed;

//	-------------------------------------------------------------------------------------------------------
//	Put count gameData.objs.objects of nType nType (eg, powerup), id = id (eg, energy) into *objP, then drop them! Yippee!
//	Returns created tObject number.
int CallObjectCreateEgg (tObject *objP, int count, int nType, int id)
{
// -- 	if (!(gameData.app.nGameMode & GM_MULTI) && (objP == gameData.objs.console))
// -- 		if (d_rand () < 32767/6) {
// -- 			Items_destroyed++;
// -- 			return -1;
// -- 		}

if (count <= 0)
	return -1;
objP->containsCount = count;
objP->containsType = nType;
objP->containsId = id;
return ObjectCreateEgg (objP);
}

//------------------------------------------------------------------------------
//what tVideoClip does this explode with?
short GetExplosionVClip (tObject *objP, int stage)
{
if (objP->nType==OBJ_ROBOT) {
	if (stage==0 && ROBOTINFO (objP->id).nExp1VClip>-1)
		return ROBOTINFO (objP->id).nExp1VClip;
	else if (stage==1 && ROBOTINFO (objP->id).nExp2VClip>-1)
		return ROBOTINFO (objP->id).nExp2VClip;
	}
else if ((objP->nType == OBJ_PLAYER) && (gameData.pig.ship.player->expl_vclip_num >- 1))
	return gameData.pig.ship.player->expl_vclip_num;
return VCLIP_SMALL_EXPLOSION;		//default
}

//------------------------------------------------------------------------------
//blow up a polygon model
void ExplodePolyModel (tObject *obj)
{
Assert (obj->renderType == RT_POLYOBJ);
if (gameData.models.nDyingModels [obj->rType.polyObjInfo.nModel] != -1)
	obj->rType.polyObjInfo.nModel = gameData.models.nDyingModels [obj->rType.polyObjInfo.nModel];
if (gameData.models.polyModels [obj->rType.polyObjInfo.nModel].nModels > 1) {
	int i;
	for (i = 1; i < gameData.models.polyModels [obj->rType.polyObjInfo.nModel].nModels; i++)
		if ((obj->nType != OBJ_ROBOT) || (obj->id != 44) || (i != 5)) 	//energy sucker energy part
			ObjectCreateDebris (obj, i);
	//make parent tObject only draw center part
	obj->rType.polyObjInfo.nSubObjFlags = 1;
	}
}

//------------------------------------------------------------------------------
//if the tObject has a destroyed model, switch to it.  Otherwise, delete it.
void MaybeDeleteObject (tObject *delObjP)
{
if (gameData.models.nDeadModels [delObjP->rType.polyObjInfo.nModel] != -1) {
	delObjP->rType.polyObjInfo.nModel = gameData.models.nDeadModels [delObjP->rType.polyObjInfo.nModel];
	delObjP->flags |= OF_DESTROYED;
	}
else {		//normal, multi-stage explosion
	if (delObjP->nType == OBJ_PLAYER)
		delObjP->renderType = RT_NONE;
	else
		KillObject (delObjP);
	}
}

//	-------------------------------------------------------------------------------------------------------
//blow up an tObject.  Takes the tObject to destroy, and the point of impact
void ExplodeObject (tObject *hitObjP, fix delayTime)
{
if (hitObjP->flags & OF_EXPLODING) 
	return;
if (delayTime) {		//wait a little while before creating explosion
	int nObject;
	tObject *objP;
	//create a placeholder tObject to do the delay, with id==-1
	nObject = CreateObject (OBJ_FIREBALL, -1, -1, hitObjP->nSegment, &hitObjP->position.vPos, &vmdIdentityMatrix, 0, 
									CT_EXPLOSION, MT_NONE, RT_NONE, 1);
	if (nObject < 0) {
		MaybeDeleteObject (hitObjP);		//no explosion, die instantly
#if TRACE
		con_printf (1, "Couldn't start explosion, deleting tObject now\n");
#endif
		Int3 ();
		return;
		}
	objP = gameData.objs.objects + nObject;
	//now set explosion-specific data
	objP->lifeleft = delayTime;
	objP->cType.explInfo.nDeleteObj = OBJ_IDX (hitObjP);
#ifdef _DEBUG
	if (objP->cType.explInfo.nDeleteObj < 0)
		Int3 (); // See Rob!
#endif
	objP->cType.explInfo.nDeleteTime = -1;
	objP->cType.explInfo.nSpawnTime = 0;
	}
else {
	tObject	*explObjP;
	ubyte		nVClip;

	nVClip = (ubyte) GetExplosionVClip (hitObjP, 0);
	explObjP = ObjectCreateExplosion (hitObjP->nSegment, &hitObjP->position.vPos, FixMul (hitObjP->size, EXPLOSION_SCALE), nVClip);
	if (!explObjP) {
		MaybeDeleteObject (hitObjP);		//no explosion, die instantly
#if TRACE
		con_printf (CONDBG, "Couldn't start explosion, deleting tObject now\n");
#endif
		return;
		}
	//don't make debris explosions have physics, because they often
	//happen when the debris has hit the tWall, so the fireball is trying
	//to move into the tWall, which shows off FVI problems.   	
	if ((hitObjP->nType != OBJ_DEBRIS) && (hitObjP->movementType == MT_PHYSICS)) {
		explObjP->movementType = MT_PHYSICS;
		explObjP->mType.physInfo = hitObjP->mType.physInfo;
		}
	if ((hitObjP->renderType == RT_POLYOBJ) && (hitObjP->nType != OBJ_DEBRIS))
		ExplodePolyModel (hitObjP);
	MaybeDeleteObject (hitObjP);
	}
hitObjP->flags |= OF_EXPLODING;		//say that this is blowing up
hitObjP->controlType = CT_NONE;		//become inert while exploding
}

//------------------------------------------------------------------------------
//do whatever needs to be done for this piece of debris for this frame
void DoDebrisFrame (tObject *objP)
{
Assert (objP->controlType == CT_DEBRIS);
if (objP->lifeleft < 0)
	ExplodeObject (objP, 0);
}

//------------------------------------------------------------------------------

extern void DropStolenItems (tObject *objP);

//do whatever needs to be done for this explosion for this frame
void DoExplosionSequence (tObject *obj)
{
	int t;

Assert (obj->controlType == CT_EXPLOSION);
//See if we should die of old age
if (obj->lifeleft <= 0) 	{	// We died of old age
	KillObject (obj);
	obj->lifeleft = 0;
	}
//See if we should create a secondary explosion
if (obj->lifeleft <= obj->cType.explInfo.nSpawnTime) {
	tObject		*explObjP, *delObjP = gameData.objs.objects + obj->cType.explInfo.nDeleteObj;
	ubyte			nVClip;
	vmsVector	*vSpawnPos;
	fix			xBadAss = (fix) ROBOTINFO (delObjP->id).badass;
	if ((obj->cType.explInfo.nDeleteObj < 0) || 
		 (obj->cType.explInfo.nDeleteObj > gameData.objs.nLastObject)) {
#if TRACE
		con_printf (CONDBG, "Illegal value for nDeleteObj in fireball.c\n");
#endif
		Int3 (); // get Rob, please... thanks
		return;
		}
	vSpawnPos = &delObjP->position.vPos;
	t = delObjP->nType;
	if (((t != OBJ_ROBOT) && (t != OBJ_CLUTTER) && (t != OBJ_CNTRLCEN) && (t != OBJ_PLAYER)) || 
			(delObjP->nSegment == -1)) {
		Int3 ();	//pretty bad
		return;
		}
	nVClip = (ubyte) GetExplosionVClip (delObjP, 1);
	if ((delObjP->nType == OBJ_ROBOT) && xBadAss)
		explObjP = ObjectCreateBadassExplosion (
			NULL, 
			delObjP->nSegment, 
			vSpawnPos, 
			FixMul (delObjP->size, EXPLOSION_SCALE), 
			nVClip, 
			F1_0 * xBadAss, 
			i2f (4) * xBadAss, 
			i2f (35) * xBadAss, 
			-1);
	else
		explObjP = ObjectCreateExplosion (delObjP->nSegment, vSpawnPos, FixMul (delObjP->size, EXPLOSION_SCALE), nVClip);
	if ((delObjP->containsCount > 0) && !IsMultiGame) { // Multiplayer handled outside of this code!!
		//	If dropping a weapon that the tPlayer has, drop energy instead, unless it's vulcan, in which case drop vulcan ammo.
		if (delObjP->containsType == OBJ_POWERUP)
			MaybeReplacePowerupWithEnergy (delObjP);
		if ((delObjP->containsType != OBJ_ROBOT) || !(delObjP->flags & OF_ARMAGEDDON))
			ObjectCreateEgg (delObjP);
		}
	if ((delObjP->nType == OBJ_ROBOT) && !IsMultiGame) { // Multiplayer handled outside this code!!
		tRobotInfo	*botInfoP = &ROBOTINFO (delObjP->id);
		if (botInfoP->containsCount && ((botInfoP->containsType != OBJ_ROBOT) || !(delObjP->flags & OF_ARMAGEDDON))) {
			if (d_rand () % 16 + 1 < botInfoP->containsProb) {
				delObjP->containsCount = (d_rand () % botInfoP->containsCount) + 1;
				delObjP->containsType = botInfoP->containsType;
				delObjP->containsId = botInfoP->containsId;
				MaybeReplacePowerupWithEnergy (delObjP);
				ObjectCreateEgg (delObjP);
				}
			}
		if (botInfoP->thief)
			DropStolenItems (delObjP);
		if (botInfoP->companion) {
			DropBuddyMarker (delObjP);
		}
	}
	if (ROBOTINFO (delObjP->id).nExp2Sound > -1)
		DigiLinkSoundToPos (ROBOTINFO (delObjP->id).nExp2Sound, delObjP->nSegment, 0, vSpawnPos, 0, F1_0);
		//PLAY_SOUND_3D (ROBOTINFO (delObjP->id).nExp2Sound, vSpawnPos, delObjP->nSegment);
	obj->cType.explInfo.nSpawnTime = -1;
	//make debris
	if (delObjP->renderType == RT_POLYOBJ)
		ExplodePolyModel (delObjP);		//explode a polygon model
	//set some parm in explosion
	if (explObjP) {
		if (delObjP->movementType == MT_PHYSICS) {
			explObjP->movementType = MT_PHYSICS;
			explObjP->mType.physInfo = delObjP->mType.physInfo;
			}
		explObjP->cType.explInfo.nDeleteTime = explObjP->lifeleft/2;
		explObjP->cType.explInfo.nDeleteObj = OBJ_IDX (delObjP);
#ifdef _DEBUG
		if (obj->cType.explInfo.nDeleteObj < 0)
		  	Int3 (); // See Rob!
#endif
		}
	else {
		MaybeDeleteObject (delObjP);
#if TRACE
		con_printf (CONDBG, "Couldn't create secondary explosion, deleting tObject now\n");
#endif
		}
	}
	//See if we should delete an tObject
if (obj->lifeleft <= obj->cType.explInfo.nDeleteTime) {
	tObject *delObjP = gameData.objs.objects + obj->cType.explInfo.nDeleteObj;
	obj->cType.explInfo.nDeleteTime = -1;
	MaybeDeleteObject (delObjP);
	}
}

//------------------------------------------------------------------------------
#define EXPL_WALL_TIME					 (f1_0)
#define EXPL_WALL_TOTAL_FIREBALLS	32
#define EXPL_WALL_FIREBALL_SIZE 		 (0x48000*6/10)	//smallest size

void InitExplodingWalls ()
{
	int i;

for (i = 0; i < MAX_EXPLODING_WALLS; i++)
	gameData.walls.explWalls [i].nSegment = -1;
}

//------------------------------------------------------------------------------
//explode the given tWall
void ExplodeWall (short nSegment, short nSide)
{
	int i;
	vmsVector pos;

	//find a d_free slot

	for (i=0;i<MAX_EXPLODING_WALLS && gameData.walls.explWalls [i].nSegment != -1;i++);

	if (i==MAX_EXPLODING_WALLS) {		//didn't find slot.
#if TRACE
		con_printf (CONDBG, "Couldn't find d_free slot for exploding tWall!\n");
#endif
		Int3 ();
		return;
	}

	gameData.walls.explWalls [i].nSegment = nSegment;
	gameData.walls.explWalls [i].nSide = nSide;
	gameData.walls.explWalls [i].time = 0;

	//play one long sound for whole door tWall explosion
	COMPUTE_SIDE_CENTER (&pos, &gameData.segs.segments [nSegment], nSide);
	DigiLinkSoundToPos (SOUND_EXPLODING_WALL, nSegment, nSide, &pos, 0, F1_0);

}

//------------------------------------------------------------------------------
//handle walls for this frame
//note: this tWall code assumes the tWall is not triangulated
void DoExplodingWallFrame ()
{
	int i;

	for (i=0;i<MAX_EXPLODING_WALLS;i++) {
		short nSegment = gameData.walls.explWalls [i].nSegment;
		if (nSegment != -1) {
			short nSide = gameData.walls.explWalls [i].nSide;
			fix oldfrac, newfrac;
			int oldCount, newCount, e;		//n, 
			oldfrac = FixDiv (gameData.walls.explWalls [i].time, EXPL_WALL_TIME);
			gameData.walls.explWalls [i].time += gameData.time.xFrame;
			if (gameData.walls.explWalls [i].time > EXPL_WALL_TIME)
				gameData.walls.explWalls [i].time = EXPL_WALL_TIME;
			if (gameData.walls.explWalls [i].time> (EXPL_WALL_TIME*3)/4) {
				tSegment *seg, *csegp;
				short cside, n;
				ubyte	a;
				seg = gameData.segs.segments + nSegment;
				a = (ubyte) gameData.walls.walls [WallNumP (seg, nSide)].nClip;
				n = AnimFrameCount (gameData.walls.pAnims + a);
				csegp = gameData.segs.segments + seg->children [nSide];
				cside = FindConnectedSide (seg, csegp);
				WallSetTMapNum (seg, nSide, csegp, cside, a, n - 1);
				gameData.walls.walls [WallNumP (seg, nSide)].flags |= WALL_BLASTED;
				if (cside >= 0)
					gameData.walls.walls [WallNumP (csegp, cside)].flags |= WALL_BLASTED;
			}
			newfrac = FixDiv (gameData.walls.explWalls [i].time, EXPL_WALL_TIME);
			oldCount = f2i (EXPL_WALL_TOTAL_FIREBALLS * FixMul (oldfrac, oldfrac));
			newCount = f2i (EXPL_WALL_TOTAL_FIREBALLS * FixMul (newfrac, newfrac));
			//n = newCount - oldCount;
			//now create all the next explosions
			for (e=oldCount;e<newCount;e++) {
				short			vertnum_list [4];
				vmsVector	*v0, *v1, *v2;
				vmsVector	vv0, vv1, pos;
				fix			size;

				//calc expl position

				GetSideVerts (vertnum_list, nSegment, nSide);
				v0 = gameData.segs.vertices + vertnum_list [0];
				v1 = gameData.segs.vertices + vertnum_list [1];
				v2 = gameData.segs.vertices + vertnum_list [2];
				VmVecSub (&vv0, v0, v1);
				VmVecSub (&vv1, v2, v1);
				VmVecScaleAdd (&pos, v1, &vv0, d_rand ()*2);
				VmVecScaleInc (&pos, &vv1, d_rand ()*2);
				size = EXPL_WALL_FIREBALL_SIZE + (2*EXPL_WALL_FIREBALL_SIZE * e / EXPL_WALL_TOTAL_FIREBALLS);
				//fireballs start away from door, with subsequent ones getting closer
				VmVecScaleInc (&pos, gameData.segs.segments [nSegment].sides [nSide].normals, size* (EXPL_WALL_TOTAL_FIREBALLS-e)/EXPL_WALL_TOTAL_FIREBALLS);
				if (e & 3)		//3 of 4 are normal
					ObjectCreateExplosion ((short) gameData.walls.explWalls [i].nSegment, &pos, size, (ubyte) VCLIP_SMALL_EXPLOSION);
				else
					ObjectCreateBadassExplosion (NULL, (short) gameData.walls.explWalls [i].nSegment, &pos, 
					size, 
					 (ubyte) VCLIP_SMALL_EXPLOSION, 
					i2f (4), 		// damage strength
					i2f (20), 		//	damage radius
					i2f (50), 		//	damage force
					-1		//	parent id
					);
			}
			if (gameData.walls.explWalls [i].time >= EXPL_WALL_TIME)
				gameData.walls.explWalls [i].nSegment = -1;	//flag this slot as d_free
		}
	}
}

//------------------------------------------------------------------------------
//creates afterburner blobs behind the specified tObject
void DropAfterburnerBlobs (tObject *objP, int count, fix xSizeScale, fix xLifeTime, 
									tObject *pParent, int bThruster)
{
	vmsVector	vPos [2];
	short			i, nSegment;
	tObject		*blobObjP;

CalcShipThrusterPos (objP, vPos);
if (count == 1)
	VmVecAvg (vPos, vPos, vPos + 1);
for (i = 0; i < count; i++) {
	nSegment = FindSegByPoint (vPos + i, objP->nSegment);
	if (nSegment == -1)
		continue;
	if (!(blobObjP = ObjectCreateExplosion (nSegment, vPos + i, xSizeScale, VCLIP_AFTERBURNER_BLOB)))
		continue;
	if (xLifeTime != -1)
		blobObjP->lifeleft = xLifeTime;
	AddChildObjectP (pParent, blobObjP);
	blobObjP->renderType = RT_THRUSTER;
	if (bThruster) {
		blobObjP->mType.physInfo.flags |= PF_WIGGLE;
		}
	}
}

//------------------------------------------------------------------------------

void RemoveMonsterball (void)
{
if (gameData.hoard.monsterballP) {
	ReleaseObject (OBJ_IDX (gameData.hoard.monsterballP));
	gameData.hoard.monsterballP = NULL;
	}
}

//------------------------------------------------------------------------------

int CreateMonsterball (void)
{
	short			nDropSeg, nObject;
	vmsVector	vSegCenter;
	vmsVector	vInitVel = ZERO_VECTOR;

RemoveMonsterball ();
#ifdef _DEBUG
nDropSeg = gameData.hoard.nMonsterballSeg;
#else
nDropSeg = (gameData.hoard.nMonsterballSeg >= 0) ? 
			  gameData.hoard.nMonsterballSeg : ChooseDropSegment (NULL, NULL, EXEC_DROP);
#endif
if (nDropSeg >= 0) {
	COMPUTE_SEGMENT_CENTER_I (&vSegCenter, nDropSeg);
	nObject = DropPowerup (OBJ_POWERUP, POW_MONSTERBALL, -1, 1, &vInitVel, &vSegCenter, nDropSeg);
	if (nObject >= 0) {
		gameData.hoard.monsterballP = gameData.objs.objects + nObject;
		gameData.hoard.monsterballP->nType = OBJ_MONSTERBALL;
		gameData.hoard.monsterballP->mType.physInfo.mass = F1_0 * 10;
		gameData.hoard.nLastHitter = -1;
		CreatePlayerAppearanceEffect (gameData.hoard.monsterballP);
		return 1;
		}
	}
#ifndef _DEBUG
Warning (TXT_NO_MONSTERBALL);
#endif
gameData.app.nGameMode &= ~GM_MONSTERBALL;
return 0;
}

//------------------------------------------------------------------------------

int FindMonsterball (void)
{
	short		i;
	tObject	*objP;

gameData.hoard.monsterballP = NULL;
gameData.hoard.nMonsterballSeg = -1;
gameData.hoard.nLastHitter = -1;
for (i = 0, objP = gameData.objs.objects; i < gameData.objs.nObjects; i++, objP++)
	if ((objP->nType == OBJ_MONSTERBALL) || 
		 ((objP->nType == OBJ_POWERUP) && (objP->id == POW_MONSTERBALL))) {
		if (gameData.hoard.nMonsterballSeg < 0)
			gameData.hoard.nMonsterballSeg = objP->nSegment;
		ReleaseObject (i);
		}
#ifndef _DEBUG
if (!(NetworkIAmMaster () && IsMultiGame && (gameData.app.nGameMode & GM_MONSTERBALL)))
	return 0;
#endif
if (!CreateMonsterball ())
	return 0;
MultiSendMonsterball (1, 1);
return 1;
}

//	-----------------------------------------------------------------------------

int CheckMonsterballScore (void)
{
	ubyte	special;

if (!(gameData.app.nGameMode & GM_MONSTERBALL))
	return 0;
if (!gameData.hoard.monsterballP)
	return 0;
if (gameData.hoard.nLastHitter != LOCALPLAYER.nObject)
	return 0;
special = gameData.segs.segment2s [gameData.hoard.monsterballP->nSegment].special;
if ((special != SEGMENT_IS_GOAL_BLUE) && (special != SEGMENT_IS_GOAL_RED))
	return 0;
if ((GetTeam (gameData.multiplayer.nLocalPlayer) == TEAM_RED) == (special == SEGMENT_IS_GOAL_RED))
	MultiSendCaptureBonus (gameData.multiplayer.nLocalPlayer);
else
	MultiSendCaptureBonus (-gameData.multiplayer.nLocalPlayer - 1);
CreatePlayerAppearanceEffect (gameData.hoard.monsterballP);
RemoveMonsterball ();
CreateMonsterball ();
MultiSendMonsterball (1, 1);
return 1;
}

//------------------------------------------------------------------------------
//eof
