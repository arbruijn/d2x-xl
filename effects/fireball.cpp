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

#include "descent.h"
#include "cockpit.h"
#include "escort.h"
#include "fireball.h"
#include "collide.h"
#include "network.h"
#include "physics.h"
#include "timer.h"
#include "gameseg.h"
#include "marker.h"
#include "text.h"
#include "interp.h"
#include "lightning.h"
#include "dropobject.h"
#include "cockpit.h"

//#define _DEBUG
#define EXPLOSION_SCALE (I2X (5)/2)		//explosion is the obj size times this

int	PK1=1, PK2=8;

void DropStolenItems (CObject *objP);

//------------------------------------------------------------------------------

CObject *CObject::CreateExplBlast (void)
{
	short		nObject, id;
	CObject	*objP;

if (!gameOpts->render.effects.bExplBlasts)
	return NULL;
nObject = CreateFireball (0, info.nSegment, info.position.vPos, 2 * info.xSize, RT_EXPLBLAST);
if (nObject < 0)
	return NULL;
objP = OBJECTS + nObject;
objP->info.xLifeLeft = BLAST_LIFE;
objP->cType.explInfo.nSpawnTime = -1;
objP->cType.explInfo.nDeleteObj = -1;
objP->cType.explInfo.nDeleteTime = -1;
objP->info.xSize = info.xSize;
objP->info.xSize /= 3;
if ((info.nType == OBJ_WEAPON) && (gameData.objs.bIsMissile [id = info.nId])) {
	if ((id == EARTHSHAKER_ID) || (id == ROBOT_EARTHSHAKER_ID)) {
		objP->info.xSize = I2X (5) / 2;
//		objP->info.xLifeLeft = 3 * BLAST_LIFE / 2;
		}
	else if ((id == MEGAMSL_ID) || (id == ROBOT_MEGAMSL_ID) || (id == EARTHSHAKER_MEGA_ID))
		objP->info.xSize = I2X (2);
	else if ((id == SMARTMSL_ID) || (id == ROBOT_SMARTMSL_ID))
		objP->info.xSize = I2X (3) / 2;
	else {
		//objP->info.xLifeLeft /= 2;
		objP->info.xSize = I2X (1);
		}
	}
return objP;
}

//------------------------------------------------------------------------------

CObject *CreateLighting (CObject *parentObjP)
{
	short		nObject;
	CObject	*objP;

if (!EGI_FLAG (bUseLightning, 0, 0, 1))
	return NULL;
nObject = CreateFireball (0, parentObjP->info.nSegment, parentObjP->info.position.vPos, 2 * parentObjP->info.xSize, RT_LIGHTNING);
if (nObject < 0)
	return NULL;
objP = OBJECTS + nObject;
objP->info.xLifeLeft = IMMORTAL_TIME;
objP->cType.explInfo.nSpawnTime = -1;
objP->cType.explInfo.nDeleteObj = -1;
objP->cType.explInfo.nDeleteTime = -1;
objP->info.xSize = 1;
return objP;
}

//------------------------------------------------------------------------------

CObject* CreateExplosion (CObject* parentP, short nSegment, CFixVector& vPos, fix xSize,
								  ubyte nVClip, fix xMaxDamage, fix xMaxDistance, fix xMaxForce, short nParent)
{
	short			nObject;
	CObject		*explObjP, *objP;
	fix			dist, force, damage;
	CFixVector	vHit, vForce;
	int			nType, id;
	int			flash = parentP ? static_cast<int> (gameData.weapons.info [parentP->info.nId].flash) : 0;

nObject = CreateFireball (nVClip, nSegment, vPos, xSize, RT_FIREBALL);

if (nObject < 0) {
#if TRACE
	console.printf (1, "Can'nType create CObject in /*Object*/CreateExplosionSub.\n");
#endif
	return NULL;
	}

explObjP = OBJECTS + nObject;
//now set explosion-specific data
explObjP->info.xLifeLeft = gameData.eff.vClips [0][nVClip].xTotalTime;
explObjP->cType.explInfo.nSpawnTime = -1;
explObjP->cType.explInfo.nDeleteObj = -1;
explObjP->cType.explInfo.nDeleteTime = -1;

if (xMaxDamage <= 0)
	return explObjP;
// -- now legal for xBadAss explosions on a CWall. Assert (this != NULL);
FORALL_OBJS (objP, i) {
	nType = objP->info.nType;
	id = objP->info.nId;
	//	Weapons used to be affected by xBadAss explosions, but this introduces serious problems.
	//	When a smart bomb blows up, if one of its children goes right towards a nearby CWall, it will
	//	blow up, blowing up all the children.  So I remove it.  MK, 09/11/94
	if (objP == parentP)
		continue;
	if (objP->info.nFlags & OF_SHOULD_BE_DEAD)
		continue;
	if (nType == OBJ_WEAPON) {
		if (!WeaponIsMine (objP->info.nId))
		continue;
		}
	else if (nType == OBJ_ROBOT) {
		if (nParent < 0)
			continue;
		if ((OBJECTS [nParent].info.nType == OBJ_ROBOT) && (OBJECTS [nParent].info.nId == id))
			continue;
		}
	else if ((nType != OBJ_REACTOR) && (nType != OBJ_PLAYER))
		continue;
	dist = CFixVector::Dist (objP->info.position.vPos, explObjP->info.position.vPos);
	// Make damage be from 'xMaxDamage' to 0.0, where 0.0 is 'xMaxDistance' away;
	if (dist >= xMaxDistance)
		continue;
	if (!ObjectToObjectVisibility (explObjP, objP, FQ_TRANSWALL))
		continue;
	damage = xMaxDamage - FixMulDiv (dist, xMaxDamage, xMaxDistance);
	force = xMaxForce - FixMulDiv (dist, xMaxForce, xMaxDistance);
	// Find the force vector on the CObject
	CFixVector::NormalizedDir(vForce, objP->info.position.vPos, explObjP->info.position.vPos);
	vForce *= force;
	// Find where the point of impact is... (vHit)
	vHit = explObjP->info.position.vPos - objP->info.position.vPos;
	vHit *= (FixDiv (objP->info.xSize, objP->info.xSize + dist));
	if (nType == OBJ_WEAPON) {
		objP->ApplyForce (vForce);
		if (WeaponIsMine (objP->info.nId) && (FixMul (dist, force) > I2X (8000))) {	//prox bombs have chance of blowing up
			objP->Die ();
			objP->ExplodeBadassWeapon (objP->info.position.vPos);
			}
		}
	else if (nType == OBJ_ROBOT) {
		CFixVector	vNegForce;
		fix			xScale = -2 * (7 - gameStates.app.nDifficultyLevel) / 8;

		objP->ApplyForce (vForce);
		//	If not a boss, stun for 2 seconds at 32 force, 1 second at 16 force
		if (flash && !ROBOTINFO (objP->info.nId).bossFlag) {
			tAIStaticInfo	*aip = &objP->cType.aiInfo;
			int				nForce = X2I (FixDiv (vForce.Mag () * flash, gameData.time.xFrame) / 128) + 2;

			if (explObjP->cType.aiInfo.SKIP_AI_COUNT * gameData.time.xFrame >= I2X (1))
				aip->SKIP_AI_COUNT--;
			else {
				aip->SKIP_AI_COUNT += nForce;
				objP->mType.physInfo.rotThrust [X] = ((d_rand () - 16384) * nForce) / 16;
				objP->mType.physInfo.rotThrust [Y] = ((d_rand () - 16384) * nForce) / 16;
				objP->mType.physInfo.rotThrust [Z] = ((d_rand () - 16384) * nForce) / 16;
				objP->mType.physInfo.flags |= PF_USES_THRUST;
				}
			}
		vNegForce [X] = vForce [X] * xScale;
		vNegForce [Y] = vForce [Y] * xScale;
		vNegForce [Z] = vForce [Z] * xScale;
		objP->ApplyRotForce (vNegForce);
		if (objP->info.xShields >= 0) {
			if (ROBOTINFO (objP->info.nId).bossFlag &&
				 bossProps [gameStates.app.bD1Mission][ROBOTINFO (objP->info.nId).bossFlag - BOSS_D2].bInvulKinetic)
				damage /= 4;
			if (objP->ApplyDamageToRobot (damage, nParent)) {
				if (!gameStates.gameplay.bNoBotAI && parentP && (nParent == LOCALPLAYER.nObject))
					cockpit->AddPointsToScore (ROBOTINFO (objP->info.nId).scoreValue);
				}
			}
		if (!flash && ROBOTINFO (objP->info.nId).companion)
			BuddyOuchMessage (damage);
		}
	else if (nType == OBJ_REACTOR) {
		if (objP->info.xShields >= 0)
			objP->ApplyDamageToReactor (damage, nParent);
		}
	else if (nType == OBJ_PLAYER) {
		CObject*		killerP = NULL;
		CFixVector	vRotForce;

		//	Hack!Warning!Test code!
		if (flash && (objP->info.nId == gameData.multiplayer.nLocalPlayer)) {
			int fe = min (I2X (4), force * flash / 32);	//	For four seconds or less
			if (parentP->cType.laserInfo.parent.nSignature == gameData.objs.consoleP->info.nSignature) {
				fe /= 2;
				force /= 2;
				}
			if (force > I2X (1)) {
				paletteManager.SetFlashDuration (fe);
				force = PK1 + X2I (PK2 * force);
				paletteManager.BumpEffect (force, force, force);
#if TRACE
				console.printf (CON_DBG, "force = %7.3f, adding %i\n", X2F (force), PK1 + X2I (PK2*force));
#endif
				}
			}
		if (parentP && IsMultiGame && (parentP->info.nType == OBJ_PLAYER))
			killerP = parentP;
		vRotForce = vForce;
		if (nParent > -1) {
			killerP = OBJECTS + nParent;
			if (killerP != gameData.objs.consoleP)		// if someone else whacks you, cut force by 2x
				vRotForce [X] /= 2;
				vRotForce [Y] /= 2;
				vRotForce [Z] /= 2;
			}
		vRotForce [X] /= 2;
		vRotForce [Y] /= 2;
		vRotForce [Z] /= 2;
		objP->ApplyForce (vForce);
		objP->ApplyRotForce (vRotForce);
		if (gameStates.app.nDifficultyLevel == 0)
			damage /= 4;
		if (objP->info.xShields >= 0)
			objP->ApplyDamageToPlayer (killerP, damage);
		}
	}
return explObjP;
}

//------------------------------------------------------------------------------

CObject* CreateBadassExplosion (CObject* objP, short nSegment, CFixVector& position, fix size, ubyte nVClip,
										  fix maxDamage, fix maxDistance, fix maxForce, short parent)
{
CObject* explObjP = CreateExplosion (objP, nSegment, position, size, nVClip, maxDamage, maxDistance, maxForce, parent);
if (explObjP && objP && (objP->info.nType == OBJ_WEAPON))
	CreateSmartChildren (objP, NUM_SMART_CHILDREN);
return explObjP;
}

//------------------------------------------------------------------------------
//blows up a xBadAss weapon, creating the xBadAss explosion
//return the explosion CObject
CObject* CObject::ExplodeBadassWeapon (CFixVector& vPos)
{
	CWeaponInfo *wi = &gameData.weapons.info [info.nId];

Assert (wi->xDamageRadius);
if ((info.nId == EARTHSHAKER_ID) || (info.nId == ROBOT_EARTHSHAKER_ID))
	ShakerRockStuff ();
audio.CreateObjectSound (SOUND_BADASS_EXPLOSION, SOUNDCLASS_EXPLOSION, OBJ_IDX (this));
CFixVector v;
if (gameStates.render.bPerPixelLighting == 2) { //make sure explosion center is not behind some wall
	v = info.vLastPos - info.position.vPos;
	CFixVector::Normalize (v);
//VmVecScale (&v, I2X (10));
	v += vPos;
	}
else
	v = vPos;
return CreateBadassExplosion (this, info.nSegment, v, wi->xImpactSize, wi->nRobotHitVClip,
                              wi->strength [gameStates.app.nDifficultyLevel], 
										wi->xDamageRadius, wi->strength [gameStates.app.nDifficultyLevel],
                              cType.laserInfo.parent.nObject);
}

//------------------------------------------------------------------------------

CObject* CObject::ExplodeBadass (fix damage, fix distance, fix force)
{

CObject* explObjP = CreateBadassExplosion (this, info.nSegment, info.position.vPos, info.xSize,
													    (ubyte) GetExplosionVClip (this, 0), damage, distance, force, OBJ_IDX (this));
if (explObjP)
	audio.CreateObjectSound (SOUND_BADASS_EXPLOSION, SOUNDCLASS_EXPLOSION, OBJ_IDX (explObjP));
return explObjP;
}

//------------------------------------------------------------------------------
//blows up the CPlayerData with a xBadAss explosion
//return the explosion CObject
CObject* CObject::ExplodeBadassPlayer (void)
{
return ExplodeBadass (I2X (50), I2X (40), I2X (150));
}

//------------------------------------------------------------------------------

inline double VectorVolume (const CFixVector& vMin, const CFixVector& vMax)
{
return fabs (X2F (vMax[X] - vMin[X])) *
		 fabs (X2F (vMax[Y] - vMin[Y])) *
		 fabs (X2F (vMax[Z] - vMin[Z]));
}

//------------------------------------------------------------------------------

double ObjectVolume (CObject *objP)
{
	CPolyModel	*modelP;
	int			i, j;
	double		size;

if (objP->info.renderType != RT_POLYOBJ)
	size = 4 * Pi * pow (X2F (objP->info.xSize), 3) / 3;
else {
	size = 0;
	modelP = gameData.models.polyModels [0] + objP->rType.polyObjInfo.nModel;
	if ((i = objP->rType.polyObjInfo.nSubObjFlags)) {
		for (j = 0; i && (j < modelP->ModelCount ()); i >>= 1, j++)
			if (i & 1)
				size += VectorVolume(modelP->SubModels ().mins [j], modelP->SubModels ().maxs [j]);
		}
	else {
		for (j = 0; j < modelP->ModelCount (); j++)
			size += VectorVolume (modelP->SubModels ().mins [j], modelP->SubModels ().maxs [j]);
		}
	}
return sqrt (size);
}

//------------------------------------------------------------------------------

#define DEBRIS_LIFE (I2X (1) * 2)		//lifespan in seconds

fix nDebrisLife [] = {2, 5, 10, 15, 30, 60, 120, 180, 300};

CObject* CObject::CreateDebris (int nSubObj)
{
	int 			nObject;
	CObject 		*debrisP;
	CPolyModel 	*po;

Assert ((info.nType == OBJ_ROBOT) || (info.nType == OBJ_PLAYER));
nObject = ::CreateDebris (this, nSubObj);
if ((nObject < 0) && (gameData.objs.nLastObject [0] >= LEVEL_OBJECTS - 1)) {
#if TRACE
	console.printf (1, "Can't create CObject in ObjectCreateDebris.\n");
#endif
	Int3 ();
	return NULL;
	}
if (nObject < 0)
	return NULL;				// Not enough debris slots!
debrisP = OBJECTS + nObject;
Assert (nSubObj < 32);
//Set polygon-CObject-specific data
debrisP->rType.polyObjInfo.nModel = rType.polyObjInfo.nModel;
debrisP->rType.polyObjInfo.nSubObjFlags = 1 << nSubObj;
debrisP->rType.polyObjInfo.nTexOverride = rType.polyObjInfo.nTexOverride;
//Set physics data for this CObject
po = gameData.models.polyModels [0] + debrisP->rType.polyObjInfo.nModel;
debrisP->mType.physInfo.velocity[X] = RAND_MAX/2 - d_rand ();
debrisP->mType.physInfo.velocity[Y] = RAND_MAX/2 - d_rand ();
debrisP->mType.physInfo.velocity[Z] = RAND_MAX/2 - d_rand ();
debrisP->mType.physInfo.velocity *= (I2X (10));
CFixVector::Normalize (debrisP->mType.physInfo.velocity);
debrisP->mType.physInfo.velocity *= (I2X (10 + (30 * d_rand () / RAND_MAX)));
debrisP->mType.physInfo.velocity += mType.physInfo.velocity;
// -- used to be: Notice, not random!VmVecMake (&debrisP->mType.physInfo.rotVel, 10*0x2000/3, 10*0x4000/3, 10*0x7000/3);
#if 0//def _DEBUG
VmVecZero (&debrisP->mType.physInfo.rotVel);
#else
debrisP->mType.physInfo.rotVel = CFixVector::Create(d_rand () + 0x1000, d_rand ()*2 + 0x4000, d_rand ()*3 + 0x2000);
#endif
debrisP->mType.physInfo.rotThrust.SetZero ();
debrisP->info.xLifeLeft = I2X (nDebrisLife [gameOpts->render.nDebrisLife]) + 3 * DEBRIS_LIFE / 4 + FixMul (d_rand (), DEBRIS_LIFE);	//	Some randomness, so they don't all go away at the same time.
debrisP->mType.physInfo.mass =
#if 0
	(fix) ((double) mType.physInfo.mass * ObjectVolume (debrisP) / ObjectVolume (this));
#else
	FixMulDiv (mType.physInfo.mass, debrisP->info.xSize, info.xSize);
#endif
debrisP->mType.physInfo.drag = gameOpts->render.nDebrisLife ? 256 : 0; //F2X (0.2);		//mType.physInfo.drag;
if (gameOpts->render.nDebrisLife) {
	debrisP->mType.physInfo.flags |= PF_FREE_SPINNING;
	debrisP->mType.physInfo.rotVel /= 3;
}
return debrisP;
}

// --------------------------------------------------------------------------------------------------------------------

void DrawFireball (CObject *objP)
{
if (objP->info.xLifeLeft > 0)
	DrawVClipObject (objP, objP->info.xLifeLeft, 0, objP->info.nId, (objP->info.nType == OBJ_WEAPON) ? gameData.weapons.color + objP->info.nId : NULL);
}

//------------------------------------------------------------------------------
//what tVideoClip does this explode with?
short GetExplosionVClip (CObject *objP, int stage)
{
if (objP->info.nType == OBJ_ROBOT) {
	if ((stage == 0) && (ROBOTINFO (objP->info.nId).nExp1VClip > -1))
		return ROBOTINFO (objP->info.nId).nExp1VClip;
	else if ((stage == 1) && (ROBOTINFO (objP->info.nId).nExp2VClip > -1))
		return ROBOTINFO (objP->info.nId).nExp2VClip;
	}
else if ((objP->info.nType == OBJ_PLAYER) && (gameData.pig.ship.player->nExplVClip >- 1))
	return gameData.pig.ship.player->nExplVClip;
return VCLIP_SMALL_EXPLOSION;		//default
}

//------------------------------------------------------------------------------
//blow up a polygon model
void CObject::ExplodePolyModel (void)
{
Assert (info.renderType == RT_POLYOBJ);
CreateExplBlast ();
RequestEffects (EXPL_LIGHTNINGS | SHRAPNEL_SMOKE);
if (gameData.models.nDyingModels [rType.polyObjInfo.nModel] != -1)
	rType.polyObjInfo.nModel = gameData.models.nDyingModels [rType.polyObjInfo.nModel];
if (gameData.models.polyModels [0][rType.polyObjInfo.nModel].ModelCount () > 1) {
	for (int i = 1; i < gameData.models.polyModels [0][rType.polyObjInfo.nModel].ModelCount (); i++)
		if ((info.nType != OBJ_ROBOT) || (info.nId != 44) || (i != 5)) 	//energy sucker energy part
			CreateDebris (i);
	//make parent CObject only draw center part
	rType.polyObjInfo.nSubObjFlags = 1;
	}
}

//------------------------------------------------------------------------------
//if the CObject has a destroyed model, switch to it.  Otherwise, delete it.
void CObject::MaybeDelete (void)
{
if (gameData.models.nDeadModels [rType.polyObjInfo.nModel] != -1) {
	rType.polyObjInfo.nModel = gameData.models.nDeadModels [rType.polyObjInfo.nModel];
	info.nFlags |= OF_DESTROYED;
	}
else {		//Normal, multi-stage explosion
	if (info.nType == OBJ_PLAYER)
		info.renderType = RT_NONE;
	else
		Die ();
	}
}

//	-------------------------------------------------------------------------------------------------------
//blow up an CObject.  Takes the CObject to destroy, and the point of impact
void CObject::Explode (fix delayTime)
{
if (info.nFlags & OF_EXPLODING)
	return;
if (delayTime) {		//wait a little while before creating explosion
	//create a placeholder CObject to do the delay, with id==-1
	int nObject = CreateFireball (ubyte (-1), info.nSegment, info.position.vPos, 0, RT_NONE);
	if (nObject < 0) {
		MaybeDelete ();		//no explosion, die instantly
#if TRACE
		console.printf (1, "Couldn't start explosion, deleting object now\n");
#endif
		Int3 ();
		return;
		}
	CObject *objP = OBJECTS + nObject;
	//now set explosion-specific data
	objP->info.xLifeLeft = delayTime;
	objP->cType.explInfo.nDeleteObj = OBJ_IDX (this);
#if DBG
	if (objP->cType.explInfo.nDeleteObj < 0)
		Int3 (); // See Rob!
#endif
	objP->cType.explInfo.nDeleteTime = -1;
	objP->cType.explInfo.nSpawnTime = 0;
	}
else {
	CObject	*explObjP;
	ubyte		nVClip;

	nVClip = (ubyte) GetExplosionVClip (this, 0);
	explObjP = /*Object*/CreateExplosion (info.nSegment, info.position.vPos, FixMul (info.xSize, EXPLOSION_SCALE), nVClip);
	if (!explObjP) {
		MaybeDelete ();		//no explosion, die instantly
#if TRACE
		console.printf (CON_DBG, "Couldn't start explosion, deleting CObject now\n");
#endif
		return;
		}
	//don't make debris explosions have physics, because they often
	//happen when the debris has hit the CWall, so the fireball is trying
	//to move into the CWall, which shows off FVI problems.
	if ((info.nType != OBJ_DEBRIS) && (info.movementType == MT_PHYSICS)) {
		explObjP->info.movementType = MT_PHYSICS;
		explObjP->mType.physInfo = mType.physInfo;
		}
	if ((info.renderType == RT_POLYOBJ) && (info.nType != OBJ_DEBRIS))
		ExplodePolyModel ();
	MaybeDelete ();
	}
info.nFlags |= OF_EXPLODING;		//say that this is blowing up
info.controlType = CT_NONE;		//become inert while exploding
}

//------------------------------------------------------------------------------
//do whatever needs to be done for this piece of debris for this frame
void DoDebrisFrame (CObject *objP)
{
Assert (objP->info.controlType == CT_DEBRIS);
if (objP->info.xLifeLeft < 0)
	objP->Explode (0);
}

//------------------------------------------------------------------------------

//do whatever needs to be done for this explosion for this frame
void CObject::DoExplosionSequence (void)
{
	int nType;

Assert (info.controlType == CT_EXPLOSION);
//See if we should die of old age
if (info.xLifeLeft <= 0) {	// We died of old age
	Die ();
	info.xLifeLeft = 0;
	}
if (info.renderType == RT_EXPLBLAST)
	return;
if (info.renderType == RT_SHRAPNELS) {
	//- moved to DoSmokeFrame() - UpdateShrapnels (this);
	return;
	}
//See if we should create a secondary explosion
if ((info.xLifeLeft <= cType.explInfo.nSpawnTime) && (cType.explInfo.nDeleteObj >= 0)) {
	CObject		*explObjP, *delObjP;
	ubyte			nVClip;
	CFixVector*	vSpawnPos;
	fix			xBadAss;

	if ((cType.explInfo.nDeleteObj < 0) ||
		 (cType.explInfo.nDeleteObj > gameData.objs.nLastObject [0])) {
#if TRACE
		console.printf (CON_DBG, "Illegal value for nDeleteObj in fireball.c\n");
#endif
		Int3 (); // get Rob, please... thanks
		return;
		}
	delObjP = OBJECTS + cType.explInfo.nDeleteObj;
	xBadAss = (fix) ROBOTINFO (delObjP->info.nId).badass;
	vSpawnPos = &delObjP->info.position.vPos;
	nType = delObjP->info.nType;
	if (((nType != OBJ_ROBOT) && (nType != OBJ_CLUTTER) && (nType != OBJ_REACTOR) && (nType != OBJ_PLAYER)) ||
			(delObjP->info.nSegment == -1)) {
		Int3 ();	//pretty bad
		return;
		}
	nVClip = (ubyte) GetExplosionVClip (delObjP, 1);
	if ((delObjP->info.nType == OBJ_ROBOT) && xBadAss)
		explObjP = CreateBadassExplosion (NULL, delObjP->info.nSegment, *vSpawnPos, FixMul (delObjP->info.xSize, EXPLOSION_SCALE),
													 nVClip, I2X (xBadAss), I2X (4) * xBadAss, I2X (35) * xBadAss, -1);
	else
		explObjP = /*Object*/CreateExplosion (delObjP->info.nSegment, *vSpawnPos, FixMul (delObjP->info.xSize, EXPLOSION_SCALE), nVClip);
	if (!IsMultiGame) { // Multiplayer handled outside of this code!!
		if (delObjP->info.contains.nCount > 0) {
			//	If dropping a weapon that the player has, drop energy instead, unless it's vulcan, in which case drop vulcan ammo.
			if (delObjP->info.contains.nType == OBJ_POWERUP)
				MaybeReplacePowerupWithEnergy (delObjP);
			if ((delObjP->info.contains.nType != OBJ_ROBOT) || !(delObjP->info.nFlags & OF_ARMAGEDDON))
				ObjectCreateEgg (delObjP);
			}
		if (delObjP->info.nType == OBJ_ROBOT) {
			tRobotInfo	*botInfoP = &ROBOTINFO (delObjP->info.nId);
			if (botInfoP->containsCount && ((botInfoP->containsType != OBJ_ROBOT) || !(delObjP->info.nFlags & OF_ARMAGEDDON))) {
				if (d_rand () % 16 + 1 < botInfoP->containsProb) {
					delObjP->info.contains.nCount = (d_rand () % botInfoP->containsCount) + 1;
					delObjP->info.contains.nType = botInfoP->containsType;
					delObjP->info.contains.nId = botInfoP->containsId;
					MaybeReplacePowerupWithEnergy (delObjP);
					ObjectCreateEgg (delObjP);
					}
				}
			if (botInfoP->thief)
				DropStolenItems (delObjP);
#if !DBG
			if (botInfoP->companion)
				DropBuddyMarker (delObjP);
#endif
			}
		}
	if (ROBOTINFO (delObjP->info.nId).nExp2Sound > -1)
		audio.CreateSegmentSound (ROBOTINFO (delObjP->info.nId).nExp2Sound, delObjP->info.nSegment, 0, *vSpawnPos, 0, I2X (1));
		//PLAY_SOUND_3D (ROBOTINFO (delObjP->info.nId).nExp2Sound, vSpawnPos, delObjP->info.nSegment);
	cType.explInfo.nSpawnTime = -1;
	//make debris
	if (delObjP->info.renderType == RT_POLYOBJ)
		delObjP->ExplodePolyModel ();		//explode a polygon model
	//set some parm in explosion
	if (explObjP) {
		if (delObjP->info.movementType == MT_PHYSICS) {
			explObjP->info.movementType = MT_PHYSICS;
			explObjP->mType.physInfo = delObjP->mType.physInfo;
			}
		explObjP->cType.explInfo.nDeleteTime = explObjP->info.xLifeLeft / 2;
		explObjP->cType.explInfo.nDeleteObj = OBJ_IDX (delObjP);
#if DBG
		if (cType.explInfo.nDeleteObj < 0)
		  	Int3 (); // See Rob!
#endif
		}
	else {
		delObjP->MaybeDelete ();
#if TRACE
		console.printf (CON_DBG, "Couldn'nType create secondary explosion, deleting CObject now\n");
#endif
		}
	}
	//See if we should delete an CObject
if ((info.xLifeLeft <= cType.explInfo.nDeleteTime) && (cType.explInfo.nDeleteObj >= 0)) {
	CObject *delObjP = OBJECTS + cType.explInfo.nDeleteObj;
	cType.explInfo.nDeleteTime = -1;
	delObjP->MaybeDelete ();
	}
}

//------------------------------------------------------------------------------
//eof
