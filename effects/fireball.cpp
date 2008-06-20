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
#include "objeffects.h"
#include "objsmoke.h"
#include "objrender.h"
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
#include "escort.h"
#include "weapon.h"
#include "fireball.h"
#include "collide.h"
#include "newmenu.h"
#include "network.h"
#include "loadgame.h"
#include "physics.h"
#include "scores.h"
#include "laser.h"
#include "wall.h"
#include "multi.h"
#include "endlevel.h"
#include "timer.h"
#include "fuelcen.h"
#include "reactor.h"
#include "gameseg.h"
#include "gamesave.h"
#include "automap.h"
#include "marker.h"
#include "kconfig.h"
#include "text.h"
#include "hudmsg.h"
#include "interp.h"
#include "lightning.h"
#include "monsterball.h"
#include "entropy.h"
#include "dropobject.h"
//#define _DEBUG
#define EXPLOSION_SCALE (F1_0*5/2)		//explosion is the obj size times this 

//--unused-- ubyte	Frame_processed [MAX_OBJECTS];

int	PK1=1, PK2=8;

void DropStolenItems (tObject *objP);

//------------------------------------------------------------------------------

tObject *CreateExplBlast (tObject *parentObjP)
{
	short		nObject, id;
	tObject	*objP;

if (!gameOpts->render.effects.bExplBlasts)
	return NULL;
nObject = CreateObject (OBJ_FIREBALL, 0, -1, parentObjP->nSegment, &parentObjP->position.vPos, &vmdIdentityMatrix, 
								2 * parentObjP->size, CT_EXPLOSION, MT_NONE, RT_EXPLBLAST, 1);
if (nObject < 0)
	return NULL;
objP = OBJECTS + nObject;
objP->lifeleft = BLAST_LIFE;
objP->cType.explInfo.nSpawnTime = -1;
objP->cType.explInfo.nDeleteObj = -1;
objP->cType.explInfo.nDeleteTime = -1;
objP->size = parentObjP->size;
objP->size /= 3;
if ((parentObjP->nType == OBJ_WEAPON) && (gameData.objs.bIsMissile [id = parentObjP->id])) {
	if ((id == EARTHSHAKER_ID) || (id == ROBOT_EARTHSHAKER_ID)) {
		objP->size = 5 * F1_0 / 2;
//		objP->lifeleft = 3 * BLAST_LIFE / 2;
		}
	else if ((id == MEGAMSL_ID) || (id == ROBOT_MEGAMSL_ID) || (id == EARTHSHAKER_MEGA_ID))
		objP->size = 2 * F1_0;
	else if ((id == SMARTMSL_ID) || (id == ROBOT_SMARTMSL_ID))
		objP->size = 3 * F1_0 / 2;
	else {
		//objP->lifeleft /= 2;
		objP->size = F1_0;
		}
	}
return objP;
}

//------------------------------------------------------------------------------

tObject *CreateLighting (tObject *parentObjP)
{
	short		nObject;
	tObject	*objP;

if (!EGI_FLAG (bUseLightnings, 0, 0, 1))
	return NULL;
nObject = CreateObject (OBJ_FIREBALL, 0, -1, parentObjP->nSegment, &parentObjP->position.vPos, &vmdIdentityMatrix, 
								2 * parentObjP->size, CT_EXPLOSION, MT_NONE, RT_LIGHTNING, 1);
if (nObject < 0)
	return NULL;
objP = OBJECTS + nObject;
objP->lifeleft = IMMORTAL_TIME;
objP->cType.explInfo.nSpawnTime = -1;
objP->cType.explInfo.nDeleteObj = -1;
objP->cType.explInfo.nDeleteTime = -1;
objP->size = 1;
return objP;
}

//------------------------------------------------------------------------------

tObject *ObjectCreateExplosionSub (tObject *objP, short nSegment, vmsVector *vPos, fix xSize, 
											  ubyte nVClip, fix xMaxDamage, fix xMaxDistance, fix xMaxForce, short nParent)
{
	short			nObject;
	tObject		*explObjP, *obj0P;
	fix			dist, force, damage;
	vmsVector	pos_hit, vForce;
	int			i, t, id;

nObject = CreateObject (OBJ_FIREBALL, nVClip, -1, nSegment, vPos, &vmdIdentityMatrix, xSize, 
							   CT_EXPLOSION, MT_NONE, RT_FIREBALL, 1);

if (nObject < 0) {
#if TRACE
	con_printf (1, "Can't create tObject in ObjectCreateExplosionSub.\n");
#endif
	return NULL;
	}

explObjP = OBJECTS + nObject;
//now set explosion-specific data
explObjP->lifeleft = gameData.eff.vClips [0][nVClip].xTotalTime;
explObjP->cType.explInfo.nSpawnTime = -1;
explObjP->cType.explInfo.nDeleteObj = -1;
explObjP->cType.explInfo.nDeleteTime = -1;

if (xMaxDamage <= 0)
	return explObjP;
// -- now legal for xBadAss explosions on a tWall. Assert (objP != NULL);
for (i = 0, obj0P = OBJECTS; i <= gameData.objs.nLastObject [0]; i++, obj0P++) {
	t = obj0P->nType;
	id = obj0P->id;
	//	Weapons used to be affected by xBadAss explosions, but this introduces serious problems.
	//	When a smart bomb blows up, if one of its children goes right towards a nearby tWall, it will
	//	blow up, blowing up all the children.  So I remove it.  MK, 09/11/94
	if (obj0P == objP)
		continue;
	if (obj0P->flags & OF_SHOULD_BE_DEAD)
		continue;
	if (t == OBJ_WEAPON) {
		if (!WeaponIsMine (obj0P->id)) 
		continue;
		}
	else if (t == OBJ_ROBOT) {
		if (nParent < 0)
			continue;
		if ((OBJECTS [nParent].nType == OBJ_ROBOT) && (OBJECTS [nParent].id == id))
			continue;
		}
	else if ((t != OBJ_REACTOR) && (t != OBJ_PLAYER))
		continue;
	dist = VmVecDistQuick (&obj0P->position.vPos, &explObjP->position.vPos);
	// Make damage be from 'xMaxDamage' to 0.0, where 0.0 is 'xMaxDistance' away;
	if (dist >= xMaxDistance)
		continue;
	if (!ObjectToObjectVisibility (explObjP, obj0P, FQ_TRANSWALL))
		continue;
	damage = xMaxDamage - FixMulDiv (dist, xMaxDamage, xMaxDistance);
	force = xMaxForce - FixMulDiv (dist, xMaxForce, xMaxDistance);
	// Find the force vector on the tObject
	VmVecNormalizedDir (&vForce, &obj0P->position.vPos, &explObjP->position.vPos);
	VmVecScale (&vForce, force);
	// Find where the point of impact is... (pos_hit)
	VmVecSub (&pos_hit, &explObjP->position.vPos, &obj0P->position.vPos);
	VmVecScale (&pos_hit, FixDiv (obj0P->size, obj0P->size + dist));
	if (t == OBJ_WEAPON) {
		PhysApplyForce (obj0P, &vForce);
		if (WeaponIsMine (obj0P->id) && (FixMul (dist, force) > i2f (8000))) {	//prox bombs have chance of blowing up
			KillObject (obj0P);
			ExplodeBadassWeapon (obj0P, &obj0P->position.vPos);
			}
		}
	else if (t == OBJ_ROBOT) {
		vmsVector	vNegForce;
		fix			xScale = -2 * (7 - gameStates.app.nDifficultyLevel) / 8;

		PhysApplyForce (obj0P, &vForce);
		//	If not a boss, stun for 2 seconds at 32 force, 1 second at 16 force
		if (objP && (!ROBOTINFO (obj0P->id).bossFlag) && (gameData.weapons.info [objP->id].flash)) {
			tAIStatic	*aip = &obj0P->cType.aiInfo;
			int			force_val = f2i (FixDiv (VmVecMagQuick (&vForce) * gameData.weapons.info [objP->id].flash, gameData.time.xFrame)/128) + 2;

			if (explObjP->cType.aiInfo.SKIP_AI_COUNT * gameData.time.xFrame >= F1_0) 
				aip->SKIP_AI_COUNT--;
			else {
				aip->SKIP_AI_COUNT += force_val;
				obj0P->mType.physInfo.rotThrust.p.x = ((d_rand () - 16384) * force_val)/16;
				obj0P->mType.physInfo.rotThrust.p.y = ((d_rand () - 16384) * force_val)/16;
				obj0P->mType.physInfo.rotThrust.p.z = ((d_rand () - 16384) * force_val)/16;
				obj0P->mType.physInfo.flags |= PF_USES_THRUST;
				}
			}
		vNegForce.p.x = vForce.p.x * xScale;
		vNegForce.p.y = vForce.p.y * xScale;
		vNegForce.p.z = vForce.p.z * xScale;
		PhysApplyRot (obj0P, &vNegForce);
		if (obj0P->shields >= 0) {
			if (ROBOTINFO (obj0P->id).bossFlag &&
				 bossProps [gameStates.app.bD1Mission][ROBOTINFO (obj0P->id).bossFlag-BOSS_D2].bInvulKinetic)
				damage /= 4;
			if (ApplyDamageToRobot (obj0P, damage, nParent) && objP && (nParent == LOCALPLAYER.nObject))
				AddPointsToScore (ROBOTINFO (obj0P->id).scoreValue);
			}
		if (objP && (ROBOTINFO (obj0P->id).companion) && !gameData.weapons.info [objP->id].flash) {
			int	i, count;
			char	szOuch [6*4 + 2];

			count = f2i (damage / 8);
			if (count > 4)
				count = 4;
			else if (count <= 0)
				count = 1;
			szOuch [0] = 0;
			for (i = 0; i < count; i++) {
				strcat (szOuch, TXT_BUDDY_OUCH);
				strcat (szOuch, " ");
				}
			BuddyMessage (szOuch);
			}
		}
	else if (t == OBJ_REACTOR) {
		if (obj0P->shields >= 0)
			ApplyDamageToReactor (obj0P, damage, nParent);
		}
	else if (t == OBJ_PLAYER) {
		tObject		*killerP = NULL;
		vmsVector	vForce2;

		//	Hack!Warning!Test code!
		if (objP && gameData.weapons.info [objP->id].flash && obj0P->id==gameData.multiplayer.nLocalPlayer) {
			int fe = min (F1_0*4, force*gameData.weapons.info [objP->id].flash/32);	//	For four seconds or less
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
		if (objP && IsMultiGame && (objP->nType == OBJ_PLAYER))
			killerP = objP;
		vForce2 = vForce;
		if (nParent > -1) {
			killerP = OBJECTS + nParent;
			if (killerP != gameData.objs.console)		// if someone else whacks you, cut force by 2x
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
			ApplyDamageToPlayer (obj0P, killerP, damage);
		}
	}
return explObjP;
}

//------------------------------------------------------------------------------

tObject *ObjectCreateBadassExplosion (tObject *objP, short nSegment, vmsVector *position, fix size, ubyte nVClip, 
												  fix maxDamage, fix maxDistance, fix maxForce, short parent)
{
	tObject	*explObjP = ObjectCreateExplosionSub (objP, nSegment, position, size, nVClip, maxDamage, maxDistance, maxForce, parent);

if (objP && (objP->nType == OBJ_WEAPON))
	CreateSmartChildren (objP, NUM_SMART_CHILDREN);
return explObjP;
}

//------------------------------------------------------------------------------
//blows up a xBadAss weapon, creating the xBadAss explosion
//return the explosion tObject
tObject *ExplodeBadassWeapon (tObject *objP, vmsVector *vPos)
{
	tWeaponInfo *wi = &gameData.weapons.info [objP->id];

Assert (wi->damage_radius);
if ((objP->id == EARTHSHAKER_ID) || (objP->id == ROBOT_EARTHSHAKER_ID))
	ShakerRockStuff ();
DigiLinkSoundToObject (SOUND_BADASS_EXPLOSION, OBJ_IDX (objP), 0, F1_0, SOUNDCLASS_EXPLOSION);
vmsVector v;
if (gameStates.render.bPerPixelLighting == 2) { //make sure explosion center is not behind some wall
	VmVecSub (&v, &objP->vLastPos, &objP->position.vPos);
	VmVecNormalize (&v, &v);
//VmVecScale (&v, F1_0 * 10);
	VmVecInc (&v, vPos);
	}
else
	v = *vPos;
return ObjectCreateBadassExplosion (objP, objP->nSegment, &v, //objP->vLastPos, //vPos
                                    wi->impact_size, 
                                    wi->robot_hit_vclip, 
                                    wi->strength [gameStates.app.nDifficultyLevel], 
                                    wi->damage_radius, wi->strength [gameStates.app.nDifficultyLevel], 
                                    objP->cType.laserInfo.nParentObj);
}

//------------------------------------------------------------------------------

tObject *ExplodeBadassObject (tObject *objP, fix damage, fix distance, fix force)
{

	tObject 	*explObjP = ObjectCreateBadassExplosion (objP, objP->nSegment, &objP->position.vPos, objP->size, 
																 (ubyte) GetExplosionVClip (objP, 0), 
																 damage, distance, force, 
																 OBJ_IDX (objP));
if (explObjP)
	DigiLinkSoundToObject (SOUND_BADASS_EXPLOSION, OBJ_IDX (explObjP), 0, F1_0, SOUNDCLASS_EXPLOSION);
return explObjP;
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
	if ((i = objP->rType.polyObjInfo.nSubObjFlags)) {
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

fix nDebrisLife [] = {2, 5, 10, 15, 30, 60, 120, 180, 300};

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
if ((nObject < 0) && (gameData.objs.nLastObject [0] >= MAX_OBJECTS - 1)) {
#if TRACE
	con_printf (1, "Can't create tObject in ObjectCreateDebris.\n");
#endif
	Int3 ();
	return NULL;
	}
if (nObject < 0)
	return NULL;				// Not enough debris slots!
debrisP = OBJECTS + nObject;
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
VmVecNormalize (&debrisP->mType.physInfo.velocity);
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
else if ((objP->nType == OBJ_PLAYER) && (gameData.pig.ship.player->nExplVClip >- 1))
	return gameData.pig.ship.player->nExplVClip;
return VCLIP_SMALL_EXPLOSION;		//default
}

//------------------------------------------------------------------------------
//blow up a polygon model
void ExplodePolyModel (tObject *objP)
{
Assert (objP->renderType == RT_POLYOBJ);
CreateExplBlast (objP);
RequestEffects (objP, EXPL_LIGHTNINGS | SHRAPNEL_SMOKE);
if (gameData.models.nDyingModels [objP->rType.polyObjInfo.nModel] != -1)
	objP->rType.polyObjInfo.nModel = gameData.models.nDyingModels [objP->rType.polyObjInfo.nModel];
if (gameData.models.polyModels [objP->rType.polyObjInfo.nModel].nModels > 1) {
	int i;
	for (i = 1; i < gameData.models.polyModels [objP->rType.polyObjInfo.nModel].nModels; i++)
		if ((objP->nType != OBJ_ROBOT) || (objP->id != 44) || (i != 5)) 	//energy sucker energy part
			ObjectCreateDebris (objP, i);
	//make parent tObject only draw center part
	objP->rType.polyObjInfo.nSubObjFlags = 1;
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
		con_printf (1, "Couldn't start explosion, deleting object now\n");
#endif
		Int3 ();
		return;
		}
	objP = OBJECTS + nObject;
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

//do whatever needs to be done for this explosion for this frame
void DoExplosionSequence (tObject *objP)
{
	int t;

Assert (objP->controlType == CT_EXPLOSION);
//See if we should die of old age
if (objP->lifeleft <= 0) {	// We died of old age
	KillObject (objP);
	objP->lifeleft = 0;
	}
if (objP->renderType == RT_EXPLBLAST) 
	return;
if (objP->renderType == RT_SHRAPNELS) {
	//- moved to DoSmokeFrame() - UpdateShrapnels (objP);
	return;
	}
//See if we should create a secondary explosion
if ((objP->lifeleft <= objP->cType.explInfo.nSpawnTime) && (objP->cType.explInfo.nDeleteObj >= 0)) {
	tObject		*explObjP, *delObjP;
	ubyte			nVClip;
	vmsVector	*vSpawnPos;
	fix			xBadAss;

	if ((objP->cType.explInfo.nDeleteObj < 0) || 
		 (objP->cType.explInfo.nDeleteObj > gameData.objs.nLastObject [0])) {
#if TRACE
		con_printf (CONDBG, "Illegal value for nDeleteObj in fireball.c\n");
#endif
		Int3 (); // get Rob, please... thanks
		return;
		}
	delObjP = OBJECTS + objP->cType.explInfo.nDeleteObj;
	xBadAss = (fix) ROBOTINFO (delObjP->id).badass;
	vSpawnPos = &delObjP->position.vPos;
	t = delObjP->nType;
	if (((t != OBJ_ROBOT) && (t != OBJ_CLUTTER) && (t != OBJ_REACTOR) && (t != OBJ_PLAYER)) || 
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
#ifndef _DEBUG
		if (botInfoP->companion)
			DropBuddyMarker (delObjP);
#endif
		}
	if (ROBOTINFO (delObjP->id).nExp2Sound > -1)
		DigiLinkSoundToPos (ROBOTINFO (delObjP->id).nExp2Sound, delObjP->nSegment, 0, vSpawnPos, 0, F1_0);
		//PLAY_SOUND_3D (ROBOTINFO (delObjP->id).nExp2Sound, vSpawnPos, delObjP->nSegment);
	objP->cType.explInfo.nSpawnTime = -1;
	//make debris
	if (delObjP->renderType == RT_POLYOBJ)
		ExplodePolyModel (delObjP);		//explode a polygon model
	//set some parm in explosion
	if (explObjP) {
		if (delObjP->movementType == MT_PHYSICS) {
			explObjP->movementType = MT_PHYSICS;
			explObjP->mType.physInfo = delObjP->mType.physInfo;
			}
		explObjP->cType.explInfo.nDeleteTime = explObjP->lifeleft / 2;
		explObjP->cType.explInfo.nDeleteObj = OBJ_IDX (delObjP);
#ifdef _DEBUG
		if (objP->cType.explInfo.nDeleteObj < 0)
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
if ((objP->lifeleft <= objP->cType.explInfo.nDeleteTime) && (objP->cType.explInfo.nDeleteObj >= 0)) {
	tObject *delObjP = OBJECTS + objP->cType.explInfo.nDeleteObj;
	objP->cType.explInfo.nDeleteTime = -1;
	MaybeDeleteObject (delObjP);
	}
}

//------------------------------------------------------------------------------

#define EXPL_WALL_TIME					(f1_0)
#define EXPL_WALL_TOTAL_FIREBALLS	32
#define EXPL_WALL_FIREBALL_SIZE 		(0x48000*6/10)	//smallest size

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

	//find a D2_FREE slot

for (i = 0; (i < MAX_EXPLODING_WALLS) && (gameData.walls.explWalls [i].nSegment != -1); i++)
	;
if (i==MAX_EXPLODING_WALLS) {		//didn't find slot.
#if TRACE
	con_printf (CONDBG, "Couldn't find D2_FREE slot for exploding wall!\n");
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

for (i = 0; i < MAX_EXPLODING_WALLS; i++) {
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
			tSegment *seg = gameData.segs.segments + nSegment,
						*csegp = gameData.segs.segments + seg->children [nSide];
			ubyte	a = (ubyte) gameData.walls.walls [WallNumP (seg, nSide)].nClip;
			short n = AnimFrameCount (gameData.walls.pAnims + a);
			short cside = FindConnectedSide (seg, csegp);
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
		for (e = oldCount; e < newCount; e++) {
			short			vertnum_list [4];
			vmsVector	*v0, *v1, *v2;
			vmsVector	vv0, vv1, pos;
			fix			size;

			//calc expl position

			GetSideVertIndex (vertnum_list, nSegment, nSide);
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
			gameData.walls.explWalls [i].nSegment = -1;	//flag this slot as D2_FREE
		}
	}
}

//------------------------------------------------------------------------------
//eof
