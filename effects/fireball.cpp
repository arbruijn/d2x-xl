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
#include "segmath.h"
#include "marker.h"
#include "text.h"
#include "interp.h"
#include "lightning.h"
#include "dropobject.h"
#include "cockpit.h"
#include "visibility.h"
#include "postprocessing.h"

#define EXPLOSION_SCALE (I2X (5)/2)		//explosion is the obj size times this

int32_t	PK1=1, PK2=8;

void DropStolenItems (CObject *pObj);

//------------------------------------------------------------------------------

CObject *CObject::CreateExplBlast (bool bForce)
{
#if 0 //DBG
return NULL;
#endif

	int16_t	nObject;
	CObject*	pObj;

if (!(bForce || (gameOpts->render.effects.bEnabled && gameOpts->render.effects.nShockwaves)))
	return NULL;
if (SPECTATOR (this))
	return NULL;
nObject = CreateFireball (0, info.nSegment, info.position.vPos, info.xSize, RT_EXPLBLAST);
pObj = OBJECT (nObject);
if (!pObj)
	return NULL;
pObj->SetLife (BLAST_LIFE);
pObj->cType.explInfo.nSpawnTime = -1;
pObj->cType.explInfo.nDestroyedObj = -1;
pObj->cType.explInfo.nDeleteTime = -1;
pObj->SetSize (info.xSize);
pObj->info.xSize /= 3;
if (IsMissile ()) {
	if ((Id () == EARTHSHAKER_ID) || (Id () == ROBOT_EARTHSHAKER_ID))
		pObj->SetSize (I2X (5) / 2);
	else if ((Id () == MEGAMSL_ID) || (Id () == ROBOT_MEGAMSL_ID) || (Id () == EARTHSHAKER_MEGA_ID))
		pObj->SetSize (I2X (2));
	else if ((Id () == SMARTMSL_ID) || (Id () == ROBOT_SMARTMSL_ID))
		pObj->SetSize (I2X (3) / 2);
	else
		pObj->SetSize (I2X (1));
	}
return pObj;
}

//------------------------------------------------------------------------------

CObject *CObject::CreateShockwave (void)
{
	int16_t	nObject;
	CObject	*pObj;

if (!(gameOpts->render.effects.bEnabled && gameOpts->render.effects.nShockwaves))
	return NULL;
if ((info.nType != OBJ_PLAYER) && (info.nType != OBJ_ROBOT) && (info.nType != OBJ_REACTOR))
	return NULL;
if (SPECTATOR (this))
	return NULL;
nObject = CreateFireball (0, info.nSegment, info.position.vPos, 10 * info.xSize, RT_SHOCKWAVE);
pObj = OBJECT (nObject);
if (!pObj)
	return NULL;
pObj->SetLife (SHOCKWAVE_LIFE);
pObj->cType.explInfo.nSpawnTime = -1;
pObj->cType.explInfo.nDestroyedObj = -1;
pObj->cType.explInfo.nDeleteTime = -1;
#if 0
pObj->Orientation () = Orientation ();
#elif 0
CAngleVector a = CAngleVector::Create (SRandShort (), SRandShort (), SRandShort ());
CFixMatrix mRotate = CFixMatrix::Create (a);
pObj->Orientation () = mRotate * Orientation ();
#else // always keep the same orientation towards the player, pitched down 45 degrees
CAngleVector a = CAngleVector::Create (-I2X (1) / 8, /*I2X (1) / 8 - SRandShort () % (I2X (1) / 4)*/0, 0);
CFixMatrix mOrient = CFixMatrix::Create (a);
pObj->info.position.mOrient = gameData.objData.pViewer->Orientation () * mOrient;
#endif

if (IsMissile ()) {
	if ((Id () == EARTHSHAKER_ID) || (Id () == ROBOT_EARTHSHAKER_ID))
		pObj->SetSize (I2X (5) / 2);
	else if ((Id () == MEGAMSL_ID) || (Id () == ROBOT_MEGAMSL_ID) || (Id () == EARTHSHAKER_MEGA_ID))
		pObj->SetSize (I2X (2));
	else if ((Id () == SMARTMSL_ID) || (Id () == ROBOT_SMARTMSL_ID))
		pObj->SetSize (I2X (3) / 2);
	else 
		pObj->SetSize (I2X (1));
	}
return pObj;
}

//------------------------------------------------------------------------------

CObject* CreateExplosion (CObject* pParent, int16_t nSegment, CFixVector& vPos, CFixVector& vImpact, fix xSize,
								  uint8_t nVClip, fix xMaxDamage, fix xMaxDistance, fix xMaxForce, int16_t nParent)
{
	int16_t		nObject;
	CObject		*pExplObj, *pObj;
	fix			dist, force, damage;
	CFixVector	vHit, vForce;
	int32_t		nType, id;
	CWeaponInfo	*pWeaponInfo = WEAPONINFO (pParent);
	int32_t		flash = (pParent && pWeaponInfo) ? static_cast<int32_t> (pWeaponInfo->flash) : 0;

nObject = CreateFireball (nVClip, nSegment, vPos, xSize, RT_FIREBALL);
pExplObj = OBJECT (nObject);
if (!pExplObj)
	return NULL;

//now set explosion-specific data
pExplObj->SetLife (gameData.effectData.animations [0][nVClip].xTotalTime);
if ((nVClip != ANIM_MORPHING_ROBOT) && 
	 //(nVClip != ANIM_PLAYER_APPEARANCE) &&
	 (nVClip != ANIM_POWERUP_DISAPPEARANCE) &&
	 (nVClip != ANIM_AFTERBURNER_BLOB)) {
	pExplObj->SetMoveDist (2 * pExplObj->info.xSize);
	pExplObj->SetMoveTime (pExplObj->info.xLifeLeft);
	}
pExplObj->cType.explInfo.nSpawnTime = -1;
pExplObj->cType.explInfo.nDestroyedObj = -1;
pExplObj->cType.explInfo.nDeleteTime = -1;

if ((pParent && (nVClip == ANIM_POWERUP_DISAPPEARANCE)) || (nVClip == ANIM_MORPHING_ROBOT))
	postProcessManager.Add (new CPostEffectShockwave (SDL_GetTicks (), pExplObj->LifeLeft () / 4, pExplObj->info.xSize, 1, pExplObj->Position ()));

if (SHOW_LIGHTNING (2)) {
	bool bDie = true;
	if (nVClip == ANIM_PLAYER_APPEARANCE)
		lightningManager.CreateForPlayerTeleport (pExplObj);
	else if (nVClip == ANIM_MORPHING_ROBOT)
		lightningManager.CreateForRobotTeleport (pExplObj);
	else if (nVClip == ANIM_POWERUP_DISAPPEARANCE)
		lightningManager.CreateForPowerupTeleport (pExplObj);
	else
		bDie = false;
	if (bDie)
		pExplObj->Die ();
	}

if (xMaxDamage <= 0)
	return pExplObj;
// -- now legal for xSplashDamage explosions on a CWall. Assert (this != NULL);
FORALL_OBJS (pObj) {
	nType = pObj->info.nType;
	id = pObj->info.nId;
	//	Weapons used to be affected by badass explosions, but this introduces serious problems.
	//	When a smart bomb blows up, if one of its children goes right towards a nearby wall, it will
	//	blow up, blowing up all the children.  So I remove it.  MK, 09/11/94
	if (pObj == pParent)
		continue;
	if (pObj->info.nFlags & OF_SHOULD_BE_DEAD)
		continue;
	if (nType == OBJ_WEAPON) {
		if (!pObj->IsMine ())
			continue;
		}
	else if (nType == OBJ_ROBOT) {
		if (nParent < 0)
			continue;
		CObject* pParent = OBJECT (nParent);
		if (pParent && pParent->IsRobot () && (pParent->Id () == id))
			continue;
		}
	else if ((nType != OBJ_REACTOR) && (nType != OBJ_PLAYER))
		continue;
	dist = CFixVector::Dist (OBJPOS (pObj)->vPos, vImpact);
	// Make damage be from 'xMaxDamage' to 0.0, where 0.0 is 'xMaxDistance' away;
	if (dist >= xMaxDistance)
		continue;
	if (!ObjectToObjectVisibility (pExplObj, pObj, FQ_TRANSWALL, -1.0f))
		continue;
	damage = xMaxDamage - FixMulDiv (dist, xMaxDamage, xMaxDistance);
	force = xMaxForce - FixMulDiv (dist, xMaxForce, xMaxDistance);
	// Find the force vector on the CObject
	CFixVector::NormalizedDir (vForce, pObj->info.position.vPos, vImpact);
	vForce *= force;
	// Find where the point of impact is... (vHit)
	vHit = vImpact - pObj->info.position.vPos;
	vHit *= (FixDiv (pObj->info.xSize, pObj->info.xSize + dist));
	if (nType == OBJ_WEAPON) {
		pObj->ApplyForce (vForce);
		if (pObj->IsMine () && (FixMul (dist, force) > I2X (8000))) {	//prox bombs have chance of blowing up
			pObj->Die ();
			pObj->ExplodeSplashDamageWeapon (pObj->info.position.vPos);
			}
		}
	else if (nType == OBJ_ROBOT) {
		CFixVector	vNegForce;
		fix			xScale = -2 * (7 - gameStates.app.nDifficultyLevel) / 8;

		pObj->ApplyForce (vForce);
		//	If not a boss, stun for 2 seconds at 32 force, 1 second at 16 force
		if (flash && !pObj->IsBoss ()) {
			tAIStaticInfo	*aip = &pObj->cType.aiInfo;
			int32_t				nForce = X2I (FixDiv (vForce.Mag () * flash, gameData.timeData.xFrame) / 128) + 2;

			if (pExplObj->cType.aiInfo.SKIP_AI_COUNT * gameData.timeData.xFrame >= I2X (1))
				aip->SKIP_AI_COUNT--;
			else {
				aip->SKIP_AI_COUNT += nForce;
				pObj->mType.physInfo.rotThrust.v.coord.x = (SRandShort () * nForce) / 16;
				pObj->mType.physInfo.rotThrust.v.coord.y = (SRandShort () * nForce) / 16;
				pObj->mType.physInfo.rotThrust.v.coord.z = (SRandShort () * nForce) / 16;
				pObj->mType.physInfo.flags |= PF_USES_THRUST;
				}
			}
#if 1
		vNegForce = vForce * xScale;
#else
		vNegForce.v.coord.x = vForce.v.coord.x * xScale;
		vNegForce.v.coord.y = vForce.v.coord.y * xScale;
		vNegForce.v.coord.z = vForce.v.coord.z * xScale;
#endif
		pObj->ApplyRotForce (vNegForce);
		if (pObj->info.xShield >= 0) {
			if (pObj->IsBoss () && bossProps [gameStates.app.bD1Mission][pObj->BossId () - BOSS_D2].bInvulKinetic)
				damage /= 4;
			if (pObj->ApplyDamageToRobot (damage, nParent)) {
#if DBG
				if (pParent && (nParent == LOCALPLAYER.nObject))
#else
				if (!(gameStates.app.bGameSuspended & SUSP_ROBOTS) && pParent && (nParent == LOCALPLAYER.nObject))
#endif
					cockpit->AddPointsToScore (ROBOTINFO (pObj)->scoreValue);
				}
			}
		if (!flash && pObj->IsGuideBot ())
			BuddyOuchMessage (damage);
		}
	else if (nType == OBJ_REACTOR) {
		if (pObj->info.xShield >= 0)
			pObj->ApplyDamageToReactor (damage, nParent);
		}
	else if (nType == OBJ_PLAYER) {
		CObject*		pKiller = NULL;
		CFixVector	vRotForce;

		//	Hack!Warning!Test code!
		if (flash && (pObj->info.nId == N_LOCALPLAYER)) {
			int32_t fe = Min (I2X (4), force * flash / 32);	//	For four seconds or less
			if (pParent->cType.laserInfo.parent.nSignature == gameData.objData.pConsole->info.nSignature) {
				fe /= 2;
				force /= 2;
				}
			if (force > I2X (1)) {
				force = PK1 + X2I (PK2 * force);
				paletteManager.BumpEffect (force, force, force);
				paletteManager.SetFadeDelay (fe);
#if TRACE
				console.printf (CON_DBG, "force = %7.3f, adding %i\n", X2F (force), PK1 + X2I (PK2*force));
#endif
				}
			}
		if (pParent && IsMultiGame && (pParent->info.nType == OBJ_PLAYER))
			pKiller = pParent;
		vRotForce = vForce;
		if (nParent > -1) {
			pKiller = OBJECT (nParent);
			if (pKiller != gameData.objData.pConsole)	{	// if someone else whacks you, cut force by 2x
				vRotForce.v.coord.x /= 2;
				vRotForce.v.coord.y /= 2;
				vRotForce.v.coord.z /= 2;
				}
			}
		vRotForce.v.coord.x /= 2;
		vRotForce.v.coord.y /= 2;
		vRotForce.v.coord.z /= 2;
		pObj->ApplyForce (vForce);
		pObj->ApplyRotForce (vRotForce);
		if (gameStates.app.nDifficultyLevel == 0)
			damage /= 4;
		if (pObj->info.xShield >= 0)
			pObj->ApplyDamageToPlayer (pKiller, damage);
		}
	}
return pExplObj;
}

//------------------------------------------------------------------------------

CObject* CreateSplashDamageExplosion (CObject* pObj, int16_t nSegment, CFixVector& position, CFixVector &impact, fix size, uint8_t nVClip,
												  fix maxDamage, fix maxDistance, fix maxForce, int16_t nParent)
{
CObject* pExplObj = CreateExplosion (pObj, nSegment, position, impact, size, nVClip, maxDamage, maxDistance, maxForce, nParent);
if (pExplObj) {
	if (!pObj ||
		 ((pObj->info.nType == OBJ_PLAYER) && !SPECTATOR (pObj)) || 
		 (pObj->info.nType == OBJ_ROBOT) || 
		 pObj->IsSplashDamageWeapon ())
		postProcessManager.Add (new CPostEffectShockwave (SDL_GetTicks (), BLAST_LIFE, pExplObj->info.xSize, 1, pExplObj->Position ()));
	if (pObj && (pObj->info.nType == OBJ_WEAPON))
		CreateSmartChildren (pObj, NUM_SMART_CHILDREN);
	}
return pExplObj;
}

//------------------------------------------------------------------------------
//blows up a xSplashDamage weapon, creating the xSplashDamage explosion
//return the explosion CObject
CObject* CObject::ExplodeSplashDamageWeapon (CFixVector& vImpact, CObject* pTarget)
{
	CWeaponInfo *pWeaponInfo = WEAPONINFO (info.nId);

if (!pWeaponInfo)
	return NULL;

Assert (pWeaponInfo->xDamageRadius);
// adjust the impact location in case it is inside the object
#if 1
if (pTarget) {
	CFixVector vRelImpact = vImpact - pTarget->Position (); // impact relative to object center
	fix xDist = vRelImpact.Mag ();
	if (xDist != pTarget->Size ()) {
		vRelImpact *= Size ();
		vRelImpact /= xDist;
		vImpact = pTarget->Position () + vRelImpact;
		}
	}
#endif
if ((info.nId == EARTHSHAKER_ID) || (info.nId == ROBOT_EARTHSHAKER_ID))
	ShakerRockStuff (&vImpact);
audio.CreateObjectSound (gameStates.app.bD1Mission ? SOUND_STANDARD_EXPLOSION : IsSplashDamageWeapon () ? SOUND_BADASS_EXPLOSION_WEAPON : SOUND_STANDARD_EXPLOSION, SOUNDCLASS_EXPLOSION, OBJ_IDX (this));
CFixVector vExplPos;
if (gameStates.render.bPerPixelLighting != 2) 
	vExplPos = vImpact;
else { //make sure explosion center is not behind some wall
	vExplPos = info.vLastPos - info.position.vPos;
	CFixVector::Normalize (vExplPos);
	//VmVecScale (&v, I2X (10));
	vExplPos += vImpact;
	}
return CreateSplashDamageExplosion (this, info.nSegment, vExplPos, vImpact, pWeaponInfo->xImpactSize, pWeaponInfo->nRobotHitAnimation,
												pWeaponInfo->strength [gameStates.app.nDifficultyLevel], pWeaponInfo->xDamageRadius, pWeaponInfo->strength [gameStates.app.nDifficultyLevel],
												cType.laserInfo.parent.nObject);
}

//------------------------------------------------------------------------------

CObject* CObject::ExplodeSplashDamage (fix damage, fix distance, fix force)
{

CObject* pExplObj = CreateSplashDamageExplosion (this, info.nSegment, info.position.vPos, info.position.vPos, info.xSize,
																 (uint8_t) GetExplosionVClip (this, 0), damage, distance, force, OBJ_IDX (this));
if (pExplObj)
	audio.CreateObjectSound (gameStates.app.bD1Mission ? SOUND_STANDARD_EXPLOSION : SOUND_BADASS_EXPLOSION_ACTOR, SOUNDCLASS_EXPLOSION, OBJ_IDX (pExplObj));
return pExplObj;
}

//------------------------------------------------------------------------------
//blows up the player with a xSplashDamage explosion
//return the explosion CObject
CObject* CObject::ExplodeSplashDamagePlayer (void)
{
return ExplodeSplashDamage (I2X (50), I2X (40), I2X (150));
}

//------------------------------------------------------------------------------

inline double VectorVolume (const CFixVector& vMin, const CFixVector& vMax)
{
return fabs (X2F (vMax.v.coord.x - vMin.v.coord.x)) *
		 fabs (X2F (vMax.v.coord.y - vMin.v.coord.y)) *
		 fabs (X2F (vMax.v.coord.z - vMin.v.coord.z));
}

//------------------------------------------------------------------------------

double ObjectVolume (CObject *pObj)
{
	CPolyModel	*pModel;
	int32_t		i, j;
	double		size;

if (pObj->info.renderType != RT_POLYOBJ)
	size = 4 * PI * pow (X2F (pObj->info.xSize), 3) / 3;
else {
	size = 0;
	pModel = gameData.modelData.polyModels [0] + pObj->ModelId ();
	if ((i = pObj->rType.polyObjInfo.nSubObjFlags)) {
		for (j = 0; i && (j < pModel->ModelCount ()); i >>= 1, j++)
			if (i & 1)
				size += VectorVolume(pModel->SubModels ().mins [j], pModel->SubModels ().maxs [j]);
		}
	else {
		for (j = 0; j < pModel->ModelCount (); j++)
			size += VectorVolume (pModel->SubModels ().mins [j], pModel->SubModels ().maxs [j]);
		}
	}
return sqrt (size);
}

//------------------------------------------------------------------------------

void CObject::SetupRandomMovement (void)
{
mType.physInfo.velocity.Set (SRandShort (), SRandShort (), SRandShort ());
//mType.physInfo.velocity *= (I2X (10));
CFixVector::Normalize (mType.physInfo.velocity);
mType.physInfo.velocity *= (I2X (10 + 30 * RandShort () / SHORT_RAND_MAX));
//mType.physInfo.velocity += mType.physInfo.velocity;
// -- used to be: Notice, not random!VmVecMake (&mType.physInfo.rotVel, 10*0x2000/3, 10*0x4000/3, 10*0x7000/3);
#if 0//DBG
VmVecZero (&mType.physInfo.rotVel);
#else
mType.physInfo.rotVel = CFixVector::Create (RandShort () + 0x1000, 2 * RandShort () + 0x4000, 3 * RandShort () + 0x2000);
#endif
mType.physInfo.rotThrust.SetZero ();
}

//------------------------------------------------------------------------------

#define DEBRIS_LIFE (I2X (1) * 2)		//lifespan in seconds

fix nDebrisLife [] = {2, 5, 10, 15, 30, 60, 120, 180, 300};

void CObject::SetupDebris (int32_t nSubObj, int32_t nId, int32_t nTexOverride)
{
Assert (nSubObj < 32);
info.nType = OBJ_DEBRIS;
//Set polygon-CObject-specific data
rType.polyObjInfo.nModel = nId;
rType.polyObjInfo.nSubObjFlags = 1 << nSubObj;
rType.polyObjInfo.nTexOverride = nTexOverride;
//Set physics data for this CObject
SetupRandomMovement ();
#if 0 //DBG
SetLife (I2X (nDebrisLife [8]) + 3 * DEBRIS_LIFE / 4 + FixMul (RandShort (), DEBRIS_LIFE));	//	Some randomness, so they don't all go away at the same time.
#else
SetLife (I2X (nDebrisLife [gameOpts->render.nDebrisLife]) + 3 * DEBRIS_LIFE / 4 + FixMul (RandShort (), DEBRIS_LIFE));	//	Some randomness, so they don't all go away at the same time.
if (nSubObj == 0)
	info.xLifeLeft *= 2;
#endif
mType.physInfo.mass =
#if 0
	(fix) ((double) mType.physInfo.mass * ObjectVolume (pDebris) / ObjectVolume (this));
#else
	FixMulDiv (mType.physInfo.mass, info.xSize, info.xSize);
#endif
mType.physInfo.drag = gameOpts->render.nDebrisLife ? 256 : 0; //F2X (0.2);		//mType.physInfo.drag;
if (gameOpts->render.nDebrisLife) {
	mType.physInfo.flags |= PF_FREE_SPINNING;
	mType.physInfo.rotVel /= 3;
	}
mType.physInfo.flags &= ~(PF_TURNROLL | PF_LEVELLING | PF_WIGGLE | PF_USES_THRUST);
}

//------------------------------------------------------------------------------

CObject* CObject::CreateDebris (int32_t nSubObj)
{
int32_t nObject = ::CreateDebris (this, nSubObj);
if ((nObject < 0) && (gameData.objData.nLastObject [0] >= LEVEL_OBJECTS - 1)) {
#if TRACE
	console.printf (1, "Can't create object in ObjectCreateDebris.\n");
#endif
	Int3 ();
	return NULL;
	}

CObject* pObj = OBJECT (nObject);
if (!pObj)
	return NULL;				// Not enough debris slots!
pObj->SetupDebris (nSubObj, ModelId (), rType.polyObjInfo.nTexOverride);
return pObj;
}

// --------------------------------------------------------------------------------------------------------------------

void DrawFireball (CObject *pObj)
{
if (pObj->info.xLifeLeft > 0)
	DrawVClipObject (pObj, pObj->info.xLifeLeft, 0, pObj->info.nId, (pObj->info.nType == OBJ_WEAPON) ? gameData.weaponData.color + pObj->info.nId : NULL);
}

//------------------------------------------------------------------------------
//what tAnimationInfo does this explode with?
int16_t GetExplosionVClip (CObject *pObj, int32_t stage)
{
if (pObj->info.nType == OBJ_ROBOT) {
	tRobotInfo *pRobotInfo = ROBOTINFO (pObj);
	if (pRobotInfo) {
		if ((stage == 0) && (pRobotInfo->nExp1VClip > -1))
			return pRobotInfo->nExp1VClip;
		else if ((stage == 1) && (pRobotInfo->nExp2VClip > -1))
			return pRobotInfo->nExp2VClip;
		}
	}
else if ((pObj->info.nType == OBJ_PLAYER) && (gameData.pigData.ship.player->nExplVClip >- 1))
	return gameData.pigData.ship.player->nExplVClip;
return ANIM_SMALL_EXPLOSION;		//default
}

//------------------------------------------------------------------------------
//blow up a polygon model
void CObject::ExplodePolyModel (void)
{
Assert (info.renderType == RT_POLYOBJ);

//CreateExplBlast (); TEST!!!
CreateShockwave ();
RequestEffects (EXPL_LIGHTNING | SHRAPNEL_SMOKE);
if (gameData.modelData.nDyingModels [ModelId ()] != -1)
	rType.polyObjInfo.nModel = gameData.modelData.nDyingModels [ModelId ()];

if ((info.nType == OBJ_ROBOT) || (info.nType == OBJ_PLAYER)) {
	int32_t nModels = gameData.modelData.polyModels [0][ModelId ()].ModelCount ();

	if (gameOpts->render.effects.bEnabled && gameOpts->render.effects.nShrapnels && (nModels > 1)) {
		int32_t j = (int32_t) FRound (X2F (info.xSize)) * (gameOpts->render.effects.nShrapnels + 1);
		for (int32_t i = 0; i < j; i++) {// "i = int32_t (j > 0)" => use the models fuselage only once
			int32_t h = i % nModels;
			if (/*((i == 0) || (h != 0)) &&*/
				 ((info.nType != OBJ_ROBOT) || (info.nId != 44) || (h != 5))) 	//energy sucker energy part
					CreateDebris (h ? h : Rand (nModels - 1) + 1);
			}
		}
	else {
		for (int32_t i = 1; i < nModels; i++) // "i = int32_t (j > 0)" => use the models fuselage only once
			if ((info.nType != OBJ_ROBOT) || (info.nId != 44) || (i != 5)) 	//energy sucker energy part
				CreateDebris (i);
		}
	//make parent CObject only draw center part
	if (info.nType != OBJ_REACTOR)
		SetupDebris (0, ModelId (), rType.polyObjInfo.nTexOverride);
	}
}

//------------------------------------------------------------------------------
//if the CObject has a destroyed model, switch to it.  Otherwise, delete it.
void CObject::MaybeDelete (void)
{
if (gameData.modelData.nDeadModels [ModelId ()] != -1) {
	rType.polyObjInfo.nModel = gameData.modelData.nDeadModels [ModelId ()];
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
	int32_t nObject = CreateFireball (uint8_t (-1), info.nSegment, info.position.vPos, 0, RT_NONE);
	CObject *pObj = OBJECT (nObject);
	if (!pObj) {
		MaybeDelete ();		//no explosion, die instantly
#if TRACE
		console.printf (1, "Couldn't start explosion, deleting object now\n");
#endif
		Int3 ();
		return;
		}
	//now set explosion-specific data
	pObj->UpdateLife (delayTime);
	pObj->cType.explInfo.nDestroyedObj = OBJ_IDX (this);
#if DBG
	if (pObj->cType.explInfo.nDestroyedObj < 0)
		Int3 (); // See Rob!
#endif
	pObj->cType.explInfo.nDeleteTime = -1;
	pObj->cType.explInfo.nSpawnTime = 0;
	}
else {
	CObject	*pExplObj;
	uint8_t		nVClip;

	nVClip = (uint8_t) GetExplosionVClip (this, 0);
	pExplObj = CreateExplosion (info.nSegment, info.position.vPos, FixMul (info.xSize, EXPLOSION_SCALE), nVClip);
	if (!pExplObj) {
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
		pExplObj->info.movementType = MT_PHYSICS;
		pExplObj->mType.physInfo = mType.physInfo;
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
void DoDebrisFrame (CObject *pObj)
{
Assert (pObj->info.controlType == CT_DEBRIS);
if (pObj->info.xLifeLeft < 0)
	pObj->Explode (0);
}

//------------------------------------------------------------------------------

//do whatever needs to be done for this explosion for this frame
void CObject::DoExplosionSequence (void)
{
	int32_t nType;

Assert (info.controlType == CT_EXPLOSION);
//See if we should die of old age
if (info.xLifeLeft <= 0) {	// We died of old age
	UpdateLife (0);
	Die ();
	}
if (info.renderType == RT_EXPLBLAST)
	return;
if (info.renderType == RT_SHOCKWAVE)
	return;
if (info.renderType == RT_SHRAPNELS) {
	//- moved to DoSmokeFrame() - UpdateShrapnels (this);
	return;
	}

if (m_xMoveTime) {
	CFixVector vOffset = gameData.objData.pViewer->Position () - Origin ();
	CFixVector::Normalize (vOffset);
	fix xOffset = fix (m_xMoveDist * double (m_xMoveTime - info.xLifeLeft) / double (m_xMoveTime));
	vOffset *= xOffset;
	Position () = Origin () + vOffset;
	}
//See if we should create a secondary explosion
if ((info.xLifeLeft <= cType.explInfo.nSpawnTime) && (cType.explInfo.nDestroyedObj >= 0)) {
	CObject*		pExplObj, *pDelObj;
	uint8_t		nVClip;
	CFixVector*	vSpawnPos;
	fix			xSplashDamage;

	if ((cType.explInfo.nDestroyedObj < 0) ||
		 (cType.explInfo.nDestroyedObj > gameData.objData.nLastObject [0])) {
#if TRACE
		console.printf (CON_DBG, "Warning: Illegal value for nDestroyedObj in fireball.c\n");
#endif
		Int3 (); // get Rob, please... thanks
		return;
		}
	pDelObj = OBJECT (cType.explInfo.nDestroyedObj);
	tRobotInfo* pRobotInfo = pDelObj->IsRobot () ? ROBOTINFO (pDelObj->info.nId) : NULL; 
	xSplashDamage = pRobotInfo ? (fix) pRobotInfo->splashDamage : 0;
	vSpawnPos = &pDelObj->info.position.vPos;
	nType = pDelObj->info.nType;
	if (((nType != OBJ_ROBOT) && (nType != OBJ_CLUTTER) && (nType != OBJ_REACTOR) && (nType != OBJ_PLAYER)) ||
			(pDelObj->info.nSegment == -1)) {
		Int3 ();	//pretty bad
		return;
		}
	nVClip = (uint8_t) GetExplosionVClip (pDelObj, 1);
	if ((pDelObj->info.nType == OBJ_ROBOT) && xSplashDamage)
		pExplObj = CreateSplashDamageExplosion (NULL, pDelObj->info.nSegment, *vSpawnPos, *vSpawnPos, FixMul (pDelObj->info.xSize, EXPLOSION_SCALE),
															 nVClip, I2X (xSplashDamage), I2X (4) * xSplashDamage, I2X (35) * xSplashDamage, -1);
	else
		pExplObj = CreateExplosion (pDelObj->info.nSegment, *vSpawnPos, FixMul (pDelObj->info.xSize, EXPLOSION_SCALE), nVClip);
	if (!IsMultiGame) { // Multiplayer handled outside of this code!!
		if (pDelObj->info.contains.nCount > 0) {
			//	If dropping a weapon that the player has, drop energy instead, unless it's vulcan, in which case drop vulcan ammo.
			if (pDelObj->info.contains.nType == OBJ_POWERUP)
				MaybeReplacePowerupWithEnergy (pDelObj);
			if ((pDelObj->info.contains.nType != OBJ_ROBOT) || !(pDelObj->info.nFlags & OF_ARMAGEDDON))
				pDelObj->CreateEgg ();
			}
		if ((pDelObj->info.nType == OBJ_ROBOT) && (!pDelObj->info.contains.nCount || gameStates.app.bD2XLevel)) {
			if (pRobotInfo->containsCount && ((pRobotInfo->containsType != OBJ_ROBOT) || !(pDelObj->info.nFlags & OF_ARMAGEDDON))) {
#if DBG
				int32_t nProb = Rand (16);
				if (nProb < pRobotInfo->containsProb) {
#else
				if (Rand (16) < pRobotInfo->containsProb) {
#endif
					pDelObj->info.contains.nCount = Rand (pRobotInfo->containsCount) + 1;
					pDelObj->info.contains.nType = pRobotInfo->containsType;
					pDelObj->info.contains.nId = pRobotInfo->containsId;
					MaybeReplacePowerupWithEnergy (pDelObj);
					pDelObj->CreateEgg ();
					}
				}
			if (pRobotInfo->thief)
				DropStolenItems (pDelObj);
#if !DBG
			if (pRobotInfo->companion)
				markerManager.DropForGuidebot (pDelObj);
#endif
			}
		}
	if (pRobotInfo && (pRobotInfo->nExp2Sound > -1))
		audio.CreateSegmentSound (pRobotInfo->nExp2Sound, pDelObj->info.nSegment, 0, *vSpawnPos, 0, I2X (1));
	cType.explInfo.nSpawnTime = -1;
	//make debris
	if (pDelObj->info.renderType == RT_POLYOBJ) {
		pDelObj->ExplodePolyModel ();		//explode a polygon model
#if 0
		if (pExplObj) {
			pExplObj->m_xMoveDist = (pDelObj->info.xSize < pExplObj->info.xSize) ? pExplObj->info.xSize : pDelObj->info.xSize;
			pExplObj->m_xMoveTime = pExplObj->info.xLifeLeft;
			}	
#endif
		}

	//set some parm in explosion
	if (pExplObj) {
		if (pDelObj->info.movementType == MT_PHYSICS) {
			pExplObj->info.movementType = MT_PHYSICS;
			pExplObj->mType.physInfo = pDelObj->mType.physInfo;
			}
		pExplObj->cType.explInfo.nDeleteTime = pExplObj->info.xLifeLeft / 2;
		pExplObj->cType.explInfo.nDestroyedObj = OBJ_IDX (pDelObj);
#if DBG
		if (cType.explInfo.nDestroyedObj < 0)
		  	Int3 (); // See Rob!
#endif
		}
	else {
		pDelObj->MaybeDelete ();
#if TRACE
		console.printf (CON_DBG, "Couldn'nType create secondary explosion, deleting CObject now\n");
#endif
		}
	}
	//See if we should delete an CObject
if ((info.xLifeLeft <= cType.explInfo.nDeleteTime) && (cType.explInfo.nDestroyedObj >= 0)) {
	CObject *pDelObj = OBJECT (cType.explInfo.nDestroyedObj);
	cType.explInfo.nDeleteTime = -1;
	pDelObj->MaybeDelete ();
	}
}

//------------------------------------------------------------------------------
//eof
