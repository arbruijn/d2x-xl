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

#include "descent.h"
#include "rle.h"
#include "stdlib.h"
#include "texmap.h"
#include "key.h"
#include "segmath.h"
#include "lightning.h"
#include "physics.h"
#include "slew.h"
#include "rendermine.h"
#include "fireball.h"
#include "hostage.h"
#include "cockpit.h"
#include "scores.h"
#include "textures.h"
#include "newdemo.h"
#include "endlevel.h"
#include "multibot.h"
#include "text.h"
#include "automap.h"
#include "sphere.h"
#include "monsterball.h"
#include "marker.h"

#ifdef FORCE_FEEDBACK
#include "tactile.h"
#endif

#include "collide.h"
#include "escort.h"

#define STANDARD_EXPL_DELAY (I2X (1)/4)

//##void CollideFireballAndWall (CObject* fireball, fix xHitSpeed, int16_t nHitSeg, int16_t nHitWall, CFixVector& vHitPt) {
//##	return;
//##}

//	-----------------------------------------------------------------------------
//	The only reason this routine is called (as of 10/12/94) is so Brain guys can open doors.
void CObject::CollideRobotAndWall (fix xHitSpeed, int16_t nHitSeg, int16_t nHitSide, CFixVector& vHitPt)
{
	tAILocalInfo	*pLocalInfo = gameData.ai.localInfo + OBJ_IDX (this);
	tRobotInfo		*pRobotInfo = ROBOTINFO (info.nId);

if (!pRobotInfo)
	return;

if ((info.nId != ROBOT_BRAIN) &&
	 (cType.aiInfo.behavior != AIB_RUN_FROM) &&
	 !(pRobotInfo && pRobotInfo->companion) &&
	 (cType.aiInfo.behavior != AIB_SNIPE))
	return;

CWall *pWall = SEGMENT (nHitSeg)->Wall (nHitSide);
if (!pWall || (pWall->nType != WALL_DOOR))
	return;

if ((pWall->keys == KEY_NONE) && (pWall->state == WALL_DOOR_CLOSED) && !(pWall->flags & WALL_DOOR_LOCKED))
	SEGMENT (nHitSeg)->OpenDoor (nHitSide);
else if (pRobotInfo && pRobotInfo->companion) {
	if ((pLocalInfo->mode != AIM_GOTO_PLAYER) && (gameData.escort.nSpecialGoal != ESCORT_GOAL_SCRAM))
		return;
	if (!(pWall->flags & WALL_DOOR_LOCKED) || ((pWall->keys != KEY_NONE) && (pWall->keys & LOCALPLAYER.flags)))
		SEGMENT (nHitSeg)->OpenDoor (nHitSide);
	}
else if (pRobotInfo && pRobotInfo->thief) {		//	Thief allowed to go through doors to which player has key.
	if ((pWall->keys != KEY_NONE) && (pWall->keys & LOCALPLAYER.flags))
		SEGMENT (nHitSeg)->OpenDoor (nHitSide);
	}
}

//##void CollideHostageAndWall (CObject* hostage, fix xHitSpeed, int16_t nHitSeg, int16_t nHitSide,   CFixVector& vHitPt) {
//##	return;
//##}

//	-----------------------------------------------------------------------------

int32_t CObject::ApplyDamageToClutter (fix xDamage)
{
if (info.nFlags & OF_EXPLODING)
	return 0;
if (info.xShield < 0)
	return 0;	//clutter already dead...
info.xShield -= xDamage;
if (info.xShield < 0) {
	Explode (0);
	return 1;
	}
return 0;
}

//	-----------------------------------------------------------------------------

//given the specified vForce, apply damage from that vForce to an CObject
void CObject::ApplyForceDamage (fix vForce, CObject* pOtherObj)
{
	int32_t		result;
	fix			xDamage;
	tRobotInfo* pRobotInfo;

if (info.nFlags & (OF_EXPLODING | OF_SHOULD_BE_DEAD))
	return;		//already exploding or dead
xDamage = FixDiv (vForce, mType.physInfo.mass) / 8;
if ((pOtherObj->info.nType == OBJ_PLAYER) && gameStates.app.cheats.bMonsterMode)
	xDamage = 0x7fffffff;
	switch (info.nType) {
		case OBJ_ROBOT:
			pRobotInfo = ROBOTINFO (info.nId);
			if (pRobotInfo) {
				if (pRobotInfo->attackType == 1) {
					if (pOtherObj->info.nType == OBJ_WEAPON)
						result = ApplyDamageToRobot (xDamage / 4, pOtherObj->cType.laserInfo.parent.nObject);
					else
						result = ApplyDamageToRobot (xDamage / 4, OBJ_IDX (pOtherObj));
					}
				else {
					if (pOtherObj->info.nType == OBJ_WEAPON)
						result = ApplyDamageToRobot (xDamage / 2, pOtherObj->cType.laserInfo.parent.nObject);
					else
						result = ApplyDamageToRobot (xDamage / 2, OBJ_IDX (pOtherObj));
					}
#if DBG
				if (result && (pOtherObj->cType.laserInfo.parent.nSignature == gameData.objData.pConsole->info.nSignature))
#else
				if (!(gameStates.app.bGameSuspended & SUSP_ROBOTS) && result && (pOtherObj->cType.laserInfo.parent.nSignature == gameData.objData.pConsole->info.nSignature))
#endif
					cockpit->AddPointsToScore (pRobotInfo->scoreValue);
				}
			break;

		case OBJ_PLAYER:
			//	If colliding with a claw nType pRobot, do xDamage proportional to gameData.time.xFrame because you can Collide with those
			//	bots every frame since they don't move.
			pRobotInfo = ROBOTINFO (pOtherObj);
			if (pRobotInfo && pRobotInfo->attackType)
				xDamage = FixMul (xDamage, gameData.time.xFrame*2);
			//	Make trainee easier.
			if (gameStates.app.nDifficultyLevel == 0)
				xDamage /= 2;
			ApplyDamageToPlayer (pOtherObj, xDamage);
			break;

		case OBJ_CLUTTER:
			ApplyDamageToClutter (xDamage);
			break;

		case OBJ_REACTOR:
			ApplyDamageToReactor (xDamage, (int16_t) OBJ_IDX (pOtherObj));
			break;

		case OBJ_WEAPON:
			break;		//weapons don't take xDamage

		default:
			Int3 ();
		}
}

//	-----------------------------------------------------------------------------

void CObject::Bump (CObject* pOtherObj, CFixVector vForce, int32_t bDamage)
{
#if DBG
if (vForce.Mag () > I2X (1) * 1000)
	return;
#endif
if (!(mType.physInfo.flags & PF_PERSISTENT)) {
	if (info.nType == OBJ_PLAYER) {
		if (pOtherObj->info.nType == OBJ_MONSTERBALL) {
			float mq;

			mq = float (pOtherObj->mType.physInfo.mass) / (float (mType.physInfo.mass) * float (nMonsterballPyroForce));
			vForce *= mq;
			ApplyForce (vForce);
			}
		else {
			vForce *= 0.25f;
			ApplyForce (vForce);
			if (bDamage && !pOtherObj->IsGuideBot ()) 
				ApplyForceDamage (vForce.Mag (), pOtherObj);
			}
		}
	else {
		float h = 1.0f / float (4 + gameStates.app.nDifficultyLevel);
		if (info.nType == OBJ_MONSTERBALL) {
			float mq;

			if (pOtherObj->info.nType == OBJ_PLAYER) {
				gameData.hoard.nLastHitter = OBJ_IDX (pOtherObj);
				mq = float (pOtherObj->mType.physInfo.mass) / float (mType.physInfo.mass) * float (nMonsterballPyroForce) / 10.0f;
				}
			else {
				gameData.hoard.nLastHitter = pOtherObj->cType.laserInfo.parent.nObject;
				mq = float (I2X (nMonsterballForces [pOtherObj->info.nId]) / 100) / float (mType.physInfo.mass);
				}
			vForce *= mq;
			if (gameData.hoard.nLastHitter == LOCALPLAYER.nObject)
				MultiSendMonsterball (1, 0);
			}
		else if (info.nType == OBJ_ROBOT) {
			if (IsBoss ())
				return;
			}
		else if ((info.nType != OBJ_CLUTTER) && (info.nType != OBJ_DEBRIS) && (info.nType != OBJ_REACTOR))
			return;
		ApplyForce (vForce);
		vForce *= h;
		ApplyRotForce (vForce);
		if (bDamage)
			ApplyForceDamage (vForce.Mag (), pOtherObj);
		}
	}
}

//	-----------------------------------------------------------------------------

void CObject::Bump (CObject* pOtherObj, CFixVector vForce, CFixVector vRotForce, int32_t bDamage)
{
if (mType.physInfo.flags & PF_PERSISTENT)
	return;
if (IsStatic ())
	return;
if (info.nType == OBJ_PLAYER) {
	if ((this == gameData.objData.pConsole) && gameData.objData.speedBoost [OBJ_IDX (this)].bBoosted)
		return;
	vRotForce *= (I2X (1) / 4);
	}
else if (info.nType == OBJ_MONSTERBALL)
	gameData.hoard.nLastHitter = (pOtherObj->info.nType == OBJ_PLAYER) ? OBJ_IDX (pOtherObj) : pOtherObj->cType.laserInfo.parent.nObject;
mType.physInfo.velocity = vForce;
ApplyRotForce (vRotForce);
//TurnTowardsVector (vRotForce, I2X (1));
}

//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------

class CBumpForces {
	public:
		CObject*	m_pObj;
		CFixVector	m_vPos, m_vVel, m_vForce, m_vRotForce;
		fix m_mass;

	explicit CBumpForces (CObject* pObj = NULL) { 
		if ((m_pObj = pObj)) {
			m_vVel = pObj->mType.physInfo.velocity;
			m_vPos = pObj->info.position.vPos;
			m_mass = pObj->mType.physInfo.mass;
			}
		}
	inline void Compute (CFixVector& vDist, CFixVector* vNormal, CObject* pObj);
	inline void Bump (CBumpForces& fOther, fix massSum, fix massDiff, int32_t bDamage);
};

//	-----------------------------------------------------------------------------

inline void CBumpForces::Compute (CFixVector& vDist, CFixVector* vNormal, CObject* pObj)
{
if (m_vVel.IsZero ())
	m_vForce.SetZero (), m_vRotForce.SetZero ();
else {
	CFixVector vDistNorm, vVelNorm;

	if (vNormal && pObj->IsStatic ())
		vDistNorm = *vNormal;
	else {
		vDistNorm = vDist;
		CFixVector::Normalize (vDistNorm);
		}
	vVelNorm = m_vVel;
	fix mag = CFixVector::Normalize (vVelNorm);
	fix dot = CFixVector::Dot (vVelNorm, vDistNorm);	// angle between objects movement vector and vector to other object
	if (dot < 0)	// moving parallel to or away from other object
		m_vForce.SetZero (), m_vRotForce.SetZero ();
	else {
		if (dot > I2X (1))
			dot = I2X (1);
		m_vForce = vDistNorm * FixMul (dot, mag);	// scale objects movement vector with the angle to calculate the impulse on the other object
		m_vRotForce = vVelNorm * FixMul (I2X (1) - dot, mag);
		m_vVel -= m_vForce;
		}
	}
}

//	-----------------------------------------------------------------------------

inline void CBumpForces::Bump (CBumpForces& fOther, fix massSum, fix massDiff, int32_t bDamage)
{
CFixVector vRes = (m_vForce * massDiff + fOther.m_vForce * (2 * fOther.m_mass)) / massSum;
// don't divide by the total mass here or ApplyRotForce() will scale down the forces too much
CFixVector vRot = (m_vRotForce * massDiff + fOther.m_vRotForce * (2 * fOther.m_mass)) /*/ massSum*/;
if (m_pObj->info.nType == OBJ_PLAYER)
	vRes *= (I2X (1) / 4);
m_pObj->Bump (fOther.m_pObj, m_vVel + vRes, vRot, bDamage);
}

//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------
//deal with two OBJECTS bumping into each other.  Apply vForce from collision
//to each pRobot.  The flags tells whether the objects should take damage from
//the collision.

int32_t BumpTwoObjects (CObject* pThis, CObject* pOther, int32_t bDamage, CFixVector& vHitPt, CFixVector* vNormal = NULL)
{
	CObject* t;

if ((pThis->info.movementType != MT_PHYSICS) && !pThis->IsStatic ())
	t = pOther;
else if ((pOther->info.movementType != MT_PHYSICS) && !pOther->IsStatic ())
	t = pThis;
else
	t = NULL;
if (t) {
	Assert (t->info.movementType == MT_PHYSICS);
	CFixVector vForce = t->mType.physInfo.velocity * (-2 * t->mType.physInfo.mass);
	if (!vForce.IsZero ())
		t->ApplyForce (vForce);
	return 1;
	}

CBumpForces f0 (pThis), f1 (pOther);
CFixVector vDist = f1.m_vPos - f0.m_vPos;

#if 0
if (CFixVector::Dot (f0.m_vVel, f1.m_vVel) <= 0)
#else
if ((CFixVector::Dot (f0.m_vVel, vDist) < 0) && (CFixVector::Dot (f1.m_vVel, vDist) > 0))
#endif
	return 0;	//objects separating already

if (!CollisionModel () &&
	 ((pThis->info.nType == OBJ_PLAYER) || (pThis->info.nType == OBJ_ROBOT) || (pThis->info.nType == OBJ_REACTOR)) &&
	 ((pOther->info.nType == OBJ_PLAYER) || (pOther->info.nType == OBJ_ROBOT) || (pOther->info.nType == OBJ_REACTOR))) {
	fix dist = vDist.Mag ();
	fix intrusion = (pThis->info.xSize + pOther->info.xSize) - dist;
	if (intrusion > 0) {
#if 0 //DBG
		HUDMessage (0, "Unsticking objects (dist = %1.2f)", X2F (dist));
#endif
		fix speed0 = f0.m_vVel.Mag ();
		fix speed1 = f1.m_vVel.Mag ();
		if (speed0 + speed1 == 0)
			return 0;
		float d = float (speed0) / float (speed0 + speed1);
		fix offset0 = F2X (d);
		fix offset1 = I2X (1) - offset0;
		fix scale = FixDiv (intrusion, dist);
		f0.m_vPos -= vDist * FixMul (offset0, scale);
		f1.m_vPos += vDist * FixMul (offset1, scale);
		OBJPOS (pThis)->vPos = f0.m_vPos;
		pThis->RelinkToSeg (FindSegByPos (f0.m_vPos, pThis->info.nSegment, 0, 0));
		OBJPOS (pOther)->vPos = f1.m_vPos;
		pOther->RelinkToSeg (FindSegByPos (f1.m_vPos, pOther->info.nSegment, 0, 0));
		}
	}

// check if objects are penetrating and move apart
if ((EGI_FLAG (bUseHitAngles, 0, 0, 0) || (pOther->info.nType == OBJ_MONSTERBALL)) || pThis->IsStatic ()) { //&& !pThis->IsStatic ()) {
	f0.m_mass = pThis->mType.physInfo.mass;
	f1.m_mass = pOther->mType.physInfo.mass;

	if (pOther->info.nType == OBJ_MONSTERBALL) {
		if (pThis->info.nType == OBJ_WEAPON)
			f0.m_mass = I2X (nMonsterballForces [pThis->info.nId]) / 100;
		else if (pThis->info.nType == OBJ_PLAYER)
			f0.m_mass *= nMonsterballPyroForce;
		}

#if 1
	f0.Compute (vDist, vNormal, pOther);
	vDist.Neg ();
	f1.Compute (vDist, vNormal, pThis);
#else
	CFixVector	vDistNorm, vVelNorm;

	if (f0.m_vVel.IsZero ())
		f0.m_vForce.SetZero (), f0.m_vRotForce.SetZero ();
	else {
		if (vNormal && pOther->IsStatic ())
			vDistNorm = *vNormal;
		else {
			vDistNorm = vDist;
			CFixVector::Normalize (vDistNorm);
			}
		vVelNorm = f0.m_vVel;
		fix mag = CFixVector::Normalize (vVelNorm);
		fix dot = CFixVector::Dot (vVelNorm, vDistNorm);	// angle between objects movement vector and vector to other object
		if (dot < 0)	// moving parallel to or away from other object
			f0.m_vForce.SetZero (), f0.m_vRotForce.SetZero ();
		else {
			if (dot > I2X (1))
				dot = I2X (1);
			f0.m_vForce = vDistNorm * FixMul (dot, mag);	// scale objects movement vector with the angle to calculate the impulse on the other object
			f0.m_vRotForce = vVelNorm * FixMul (I2X (1) - dot, mag);
			f0.m_vVel -= f0.m_vForce;
			}
		}

	if (f1.m_vVel.IsZero ())
		f1.m_vForce.SetZero (), f1.m_vRotForce.SetZero ();
	else {
		if (vNormal && pThis->IsStatic ())
			vDistNorm = *vNormal;
		else {
			vDistNorm = vDist;
			CFixVector::Normalize (vDistNorm);
			}
		vDistNorm.Neg ();
		vVelNorm = f1.m_vVel;
		fix mag = CFixVector::Normalize (vVelNorm);
		fix dot = CFixVector::Dot (vVelNorm, vDistNorm);
		if (dot < 0)
			f1.m_vForce.SetZero (), f1.m_vRotForce.SetZero ();
		else {
			if (dot > I2X (1))
				dot = I2X (1);
			f1.m_vForce = vDistNorm * FixMul (dot, mag); 
			f1.m_vRotForce = vVelNorm * FixMul (I2X (1) - dot, mag);
			f1.m_vVel -= f1.m_vForce;
			}
		}
#endif

	fix massSum = f0.m_mass + f1.m_mass, massDiff = f0.m_mass - f1.m_mass;

#if 1
	f0.Bump (f1, massSum, massDiff, bDamage);
	f1.Bump (f0, massSum, -massDiff, bDamage);
#else
	CFixVector vRes0 = (f0.m_vForce * massDiff + f1.m_vForce * (2 * f1.m_mass)) / massSum;
	CFixVector vRes1 = (f1.m_vForce * -massDiff + f0.m_vForce * (2 * f0.m_mass)) / massSum;
	// don't divide by the total mass here or ApplyRotForce() will scale down the forces too much
	CFixVector vRot0 = (f0.m_vRotForce * massDiff + f1.m_vRotForce * (2 * f1.m_mass)) /*/ massSum*/;
	CFixVector vRot1 = (f1.m_vRotForce * -massDiff + f0.m_vRotForce * (2 * f0.m_mass)) /*/ massSum*/;
	if (pThis->info.nType == OBJ_PLAYER)
		vRes0 *= (I2X (1) / 4);
	else if (pOther->info.nType == OBJ_PLAYER)
		vRes1 *= (I2X (1) / 4);
	pThis->Bump (pOther, f0.m_vVel + vRes0, vRot0, bDamage);
	pOther->Bump (pThis, f1.m_vVel + vRes1, vRot1, bDamage);
#endif
	}
else {
	CFixVector vForce = f0.m_vVel - f1.m_vVel;
#if 0
	fix f0.m_mass = pThis->mType.physInfo.mass;
	fix f1.m_mass = pOther->mType.physInfo.mass;
	fix massProd = FixMul (f0.m_mass, f1.m_mass) * 2;
	fix massTotal = f0.m_mass + f1.m_mass;
	fix impulse = FixDiv (massProd, massTotal);
	if (impulse == 0)
		return 0;
#else
	float mass0 = X2F (pThis->mType.physInfo.mass);
	float mass1 = X2F (pOther->mType.physInfo.mass);
	float impulse = 2.0f * (mass0 * mass1) / (mass0 + mass1);
	if (impulse == 0.0f)
		return 0;
#endif
	vForce *= impulse;
	pOther->Bump (pThis, vForce, bDamage);
	vForce.Neg ();
	pThis->Bump (pOther, vForce, bDamage);
	}

return 1;
}

//	-----------------------------------------------------------------------------

void CObject::Bump (CFixVector vForce, fix xDamage)
{
vForce *= xDamage;
ApplyForce (vForce);
}

//	-----------------------------------------------------------------------------

#define DAMAGE_SCALE 		128	//	Was 32 before 8:55 am on Thursday, September 15, changed by MK, walls were hurting me more than robots!
#define DAMAGE_THRESHOLD 	 (I2X (1)/3)
#define WALL_LOUDNESS_SCALE (20)

fix force_force = I2X (50);

void CObject::CollidePlayerAndWall (fix xHitSpeed, int16_t nHitSeg, int16_t nHitSide, CFixVector& vHitPt)
{
	fix damage;
	char bForceFieldHit = 0;
	int32_t nBaseTex, nOvlTex;

if (info.nId != N_LOCALPLAYER) // Execute only for local player
	return;
nBaseTex = SEGMENT (nHitSeg)->m_sides [nHitSide].m_nBaseTex;
//	If this CWall does damage, don't make *BONK* sound, we'll be making another sound.
if (gameData.pig.tex.pTexMapInfo [nBaseTex].damage > 0)
	return;
if (gameData.pig.tex.pTexMapInfo [nBaseTex].flags & TMI_FORCE_FIELD) {
	CFixVector vForce;
	paletteManager.BumpEffect (0, 0, 60);	//flash blue
	//knock player around
	vForce.v.coord.x = 40 * SRandShort ();
	vForce.v.coord.y = 40 * SRandShort ();
	vForce.v.coord.z = 40 * SRandShort ();
	ApplyRotForce (vForce);
#ifdef FORCE_FEEDBACK
	if (TactileStick)
		Tactile_apply_force (&vForce, &info.position.mOrient);
#endif
	//make sound
	audio.CreateSegmentSound (SOUND_FORCEFIELD_BOUNCE_PLAYER, nHitSeg, 0, vHitPt);
	if (IsMultiGame)
		MultiSendPlaySound (SOUND_FORCEFIELD_BOUNCE_PLAYER, I2X (1));
	bForceFieldHit = 1;
	}
else {
#ifdef FORCE_FEEDBACK
	CFixVector vForce;
	if (TactileStick) {
		vForce.dir.coord.x = -mType.physInfo.velocity.dir.coord.x;
		vForce.dir.coord.y = -mType.physInfo.velocity.dir.coord.y;
		vForce.dir.coord.z = -mType.physInfo.velocity.dir.coord.z;
		Tactile_do_collide (&vForce, &info.position.mOrient);
	}
#endif
   SEGMENT (nHitSeg)->ProcessWallHit (nHitSide, 20, info.nId, this);
	}
if (gameStates.app.bD2XLevel && (SEGMENT (nHitSeg)->HasNoDamageProp ()))
	return;
//	** Damage from hitting CWall **
//	If the player has less than 10% shield, don't take damage from bump
// Note: Does quad damage if hit a vForce field - JL
damage = (xHitSpeed / DAMAGE_SCALE) * (bForceFieldHit * 8 + 1);
nOvlTex = SEGMENT (nHitSeg)->m_sides [nHitSide].m_nOvlTex;
//don't do CWall damage and sound if hit lava or water
if ((gameData.pig.tex.pTexMapInfo [nBaseTex].flags & (TMI_WATER|TMI_VOLATILE)) ||
		(nOvlTex && (gameData.pig.tex.pTexMapInfo [nOvlTex].flags & (TMI_WATER|TMI_VOLATILE))))
	damage = 0;
if (damage >= DAMAGE_THRESHOLD) {
	int32_t volume = (xHitSpeed - (DAMAGE_SCALE * DAMAGE_THRESHOLD)) / WALL_LOUDNESS_SCALE;
	CreateAwarenessEvent (this, PA_WEAPON_WALL_COLLISION);
	if (volume > I2X (1))
		volume = I2X (1);
	if (volume > 0 && !bForceFieldHit) {  // uhhhgly hack
		audio.CreateSegmentSound (SOUND_PLAYER_HIT_WALL, nHitSeg, 0, vHitPt, 0, volume);
		if (IsMultiGame)
			MultiSendPlaySound (SOUND_PLAYER_HIT_WALL, volume);
		}
	if (!(LOCALPLAYER.flags & PLAYER_FLAGS_INVULNERABLE))
		if (LOCALPLAYER.Shield () > I2X (1) * 10 || bForceFieldHit)
			ApplyDamageToPlayer (this, damage);
	}
return;
}

//	-----------------------------------------------------------------------------

fix	xLastVolatileScrapeSoundTime = 0;


//see if CWall is volatile or water
//if volatile, damage player ship
//returns 1=lava, 2=water
int32_t CObject::ApplyWallPhysics (int16_t nSegment, int16_t nSide)
{
	fix	xDamage = 0;
	int32_t	nType;

if (!((nType = SEGMENT (nSegment)->Physics (nSide, xDamage)) || 
		(nType = SEGMENT (nSegment)->Physics (xDamage))))
	return 0;
if (SEGMENT (nSegment)->HasNoDamageProp ())
	 xDamage = 0;
if (info.nId == N_LOCALPLAYER) {
	if (xDamage > 0) {
		xDamage = FixMul (xDamage, gameData.time.xFrame);
		if (gameStates.app.nDifficultyLevel == 0)
			xDamage /= 2;
		if (!(LOCALPLAYER.flags & PLAYER_FLAGS_INVULNERABLE))
			ApplyDamageToPlayer (this, xDamage);

#ifdef FORCE_FEEDBACK
		if (TactileStick)
			Tactile_Xvibrate (50, 25);
#endif

		paletteManager.BumpEffect (X2I (xDamage * 4), 0, 0);	//flash red
		}
	if (xDamage || !mType.physInfo.thrust.IsZero ()) {
		mType.physInfo.rotVel.v.coord.x = SRandShort () / 2;
		mType.physInfo.rotVel.v.coord.z = SRandShort () / 2;
		}
	}
return nType;
}

//	-----------------------------------------------------------------------------

int32_t CObject::CheckSegmentPhysics (void)
{
	fix xDamage;
	int32_t nType;

//	Assert (info.nType==OBJ_PLAYER);
if (!EGI_FLAG (bFluidPhysics, 1, 0, 0))
	return 0;
if (!(nType = SEGMENT (info.nSegment)->Physics (xDamage)))
	return 0;
if (xDamage > 0) {
	xDamage = FixMul (xDamage, gameData.time.xFrame) / 2;
	if (gameStates.app.nDifficultyLevel == 0)
		xDamage /= 2;
	if (info.nType == OBJ_PLAYER) {
		if (!(PLAYER (info.nId).flags & PLAYER_FLAGS_INVULNERABLE))
			ApplyDamageToPlayer (this, xDamage);
		}
	if (info.nType == OBJ_ROBOT) {
		ApplyDamageToRobot (info.xShield + 1, OBJ_IDX (this));
		}

#ifdef FORCE_FEEDBACK
	if (TactileStick)
		Tactile_Xvibrate (50, 25);
#endif
	if ((info.nType == OBJ_PLAYER) && (info.nId == N_LOCALPLAYER))
		paletteManager.BumpEffect (X2I (xDamage * 4), 0, 0);	//flash red
	if ((info.nType == OBJ_PLAYER) || (info.nType == OBJ_ROBOT)) {
		mType.physInfo.rotVel.v.coord.x = SRandShort () / 4;
		mType.physInfo.rotVel.v.coord.z = SRandShort () / 4;
		}
	return nType;
	}
if (((info.nType == OBJ_PLAYER) || (info.nType == OBJ_ROBOT)) && !mType.physInfo.thrust.IsZero ()) {
	mType.physInfo.rotVel.v.coord.x = SRandShort () / 8;
	mType.physInfo.rotVel.v.coord.z = SRandShort () / 8;
	}
return nType;
}

//	-----------------------------------------------------------------------------
//this gets called when an CObject is scraping along the CWall
void CObject::ScrapeOnWall (int16_t nHitSeg, int16_t nHitSide, CFixVector& vHitPt)
{
if (info.nType == OBJ_PLAYER) {
	if (info.nId == N_LOCALPLAYER) {
		int32_t nType = ApplyWallPhysics (nHitSeg, nHitSide);
		if (nType != 0) {
			CFixVector	vHit, vRand;

			if ((gameData.time.xGame > xLastVolatileScrapeSoundTime + I2X (1)/4) ||
					(gameData.time.xGame < xLastVolatileScrapeSoundTime)) {
				int16_t sound = (nType & 1) ? SOUND_VOLATILE_WALL_HISS : SOUND_SHIP_IN_WATER;
				xLastVolatileScrapeSoundTime = gameData.time.xGame;
				audio.CreateSegmentSound (sound, nHitSeg, 0, vHitPt);
				if (IsMultiGame)
					MultiSendPlaySound (sound, I2X (1));
				}
			vHit = SEGMENT (nHitSeg)->m_sides [nHitSide].m_normals [0];
			vRand = CFixVector::Random();
			vHit += vRand * (I2X (1)/8);
			CFixVector::Normalize (vHit);
			Bump (vHit, I2X (8));
			}
		}
	}
else if (info.nType == OBJ_WEAPON)
	CollideWeaponAndWall (0, nHitSeg, nHitSide, vHitPt);
else if (info.nType == OBJ_DEBRIS)
	CollideDebrisAndWall (0, nHitSeg, nHitSide, vHitPt);
}

//	Copied from laser.c!
#define	DESIRED_OMEGA_DIST	 (I2X (5))		//	This is the desired distance between blobs.  For distances > MIN_OMEGA_BLOBS*DESIRED_OMEGA_DIST, but not very large, this will apply.
#define	MAX_OMEGA_BLOBS		16				//	No matter how far away the obstruction, this is the maximum number of blobs.
#define	MAX_OMEGA_DIST			 (MAX_OMEGA_BLOBS * DESIRED_OMEGA_DIST)		//	Maximum extent of lightning blobs.

//	-----------------------------------------------------------------------------
//	Return true if ok to do Omega damage.
int32_t OkToDoOmegaDamage (CObject* pWeapon)
{
if (!IsMultiGame)
	return 1;
int32_t nParentSig = pWeapon->cType.laserInfo.parent.nSignature;
CObject *pParent = OBJECT (pWeapon->cType.laserInfo.parent.nObject);
if (pParent->info.nSignature != nParentSig)
	return 1;
fix dist = CFixVector::Dist (pParent->info.position.vPos, pWeapon->info.position.vPos);
if (dist > MAX_OMEGA_DIST)
	return 0;
return 1;
}

//	-----------------------------------------------------------------------------

int32_t CObject::CreateWeaponEffects (int32_t bExplBlast)
{
if ((info.nType == OBJ_WEAPON) && IsMissile ()) {
	if (bExplBlast) {
		CreateExplBlast ();
		CreateShockwave ();
		}
	if (info.nId == EARTHSHAKER_ID)
		RequestEffects (MISSILE_LIGHTNING);
	else if ((info.nId == EARTHSHAKER_MEGA_ID) || (info.nId == ROBOT_SHAKER_MEGA_ID))
		RequestEffects (MISSILE_LIGHTNING);
	else if ((info.nId == MEGAMSL_ID) || (info.nId == ROBOT_MEGAMSL_ID))
		RequestEffects (MISSILE_LIGHTNING);
	else
		return 0;
	return 1;
	}
return 0;
}

//	-----------------------------------------------------------------------------
//these gets added to the weapon's values when the weapon hits a volitle CWall
#define VOLATILE_WALL_EXPL_STRENGTH I2X (10)
#define VOLATILE_WALL_IMPACT_SIZE	I2X (3)
#define VOLATILE_WALL_DAMAGE_FORCE	I2X (5)
#define VOLATILE_WALL_DAMAGE_RADIUS	I2X (30)

// int32_t Show_segAnd_side = 0;

int32_t CObject::CollideWeaponAndWall (fix xHitSpeed, int16_t nHitSeg, int16_t nHitSide, CFixVector& vHitPt)
{
	CSegment*		pSeg = SEGMENT (nHitSeg);

if (!pSeg)
	return 1;

	CSide*			pSide = (nHitSide < 0) ? NULL : pSeg->m_sides + nHitSide;
	CWeaponInfo*	pWeaponInfo = gameData.weapons.info + info.nId;
	CObject*			pParentObj = OBJECT (cType.laserInfo.parent.nObject);

	int32_t			nPlayer;
	fix				nStrength = WI_strength (info.nId, gameStates.app.nDifficultyLevel);

if (info.nId == OMEGA_ID)
	if (!OkToDoOmegaDamage (this))
		return 1;

//	If this is a guided missile and it strikes fairly directly, clear bounce flag.
if (info.nId == GUIDEDMSL_ID) {
	fix dot = (nHitSide < 0) ? -1 : CFixVector::Dot (info.position.mOrient.m.dir.f, pSide->m_normals [0]);
#if TRACE
	console.printf (CON_DBG, "Guided missile dot = %7.3f \n", X2F (dot));
#endif
	if (dot < -I2X (1) / 6) {
#if TRACE
		console.printf (CON_DBG, "Guided missile loses bounciness. \n");
#endif
		mType.physInfo.flags &= ~PF_BOUNCES;
		}
	else {
		CFixVector vReflect = CFixVector::Reflect (info.position.mOrient.m.dir.f, pSide->m_normals [0]);
		CAngleVector va = vReflect.ToAnglesVec ();
		info.position.mOrient = CFixMatrix::Create (va);
		}
	}

int32_t bBounce = (mType.physInfo.flags & PF_BOUNCES) != 0;
if (!bBounce)
	CreateWeaponEffects (1);
//if an energy this hits a forcefield, let it bounce
if (pSide && (gameData.pig.tex.pTexMapInfo [pSide->m_nBaseTex].flags & TMI_FORCE_FIELD) &&
	 ((info.nType != OBJ_WEAPON) || pWeaponInfo->xEnergyUsage)) {

	//make sound
	audio.CreateSegmentSound (SOUND_FORCEFIELD_BOUNCE_WEAPON, nHitSeg, 0, vHitPt);
	if (IsMultiGame)
		MultiSendPlaySound (SOUND_FORCEFIELD_BOUNCE_WEAPON, I2X (1));
	return 1;	//bail here. physics code will bounce this CObject
	}

int32_t bEscort = pParentObj && pParentObj->IsGuideBot ();
if (bEscort) {
	if (IsMultiGame) {
		Int3 ();  // Get Jason!
	   return 1;
	   }
	nPlayer = N_LOCALPLAYER;		//if single player, he's the players's buddy
	pParentObj = OBJECT (LOCALPLAYER.nObject);
	}
else {
	nPlayer = (pParentObj && pParentObj->IsPlayer ()) ? pParentObj->info.nId : -1;
	}

int32_t bBlewUp = (nHitSide < 0) ? 0 : pSeg->BlowupTexture (nHitSide, vHitPt, this, 0);
if (bBlewUp) {		//could be a wall switch - only player or guidebot can activate it
	pSeg->OperateTrigger (nHitSide, pParentObj, 1);
	}
if (info.nId == EARTHSHAKER_ID)
	ShakerRockStuff (&Position ());
int32_t wallType = (nHitSide < 0) ? WHP_NOT_SPECIAL : pSeg->ProcessWallHit (nHitSide, info.xShield, nPlayer, this);
// Wall is volatile if either tmap 1 or 2 is volatile
if (pSide && ((gameData.pig.tex.pTexMapInfo [pSide->m_nBaseTex].flags & TMI_VOLATILE) ||
	           (pSide->m_nOvlTex && (gameData.pig.tex.pTexMapInfo [pSide->m_nOvlTex].flags & TMI_VOLATILE)))) {
	uint8_t tAnimationInfo;
	//we've hit a volatile CWall
	audio.CreateSegmentSound (SOUND_VOLATILE_WALL_HIT, nHitSeg, 0, vHitPt);
	//for most weapons, use volatile CWall hit.  For mega, use its special tAnimationInfo
	tAnimationInfo = (info.nId == MEGAMSL_ID) ? pWeaponInfo->nRobotHitAnimation : ANIM_VOLATILE_WALL_HIT;
	//	New by MK: If powerful splash damager, explode with splash damage, not due to lava, fixes megas being wimpy in lava.
	if (pWeaponInfo->xDamageRadius >= VOLATILE_WALL_DAMAGE_RADIUS / 2)
		ExplodeSplashDamageWeapon (vHitPt);
	else
		CreateSplashDamageExplosion (this, nHitSeg, vHitPt, vHitPt, pWeaponInfo->xImpactSize + VOLATILE_WALL_IMPACT_SIZE, tAnimationInfo,
											  nStrength / 4 + VOLATILE_WALL_EXPL_STRENGTH, pWeaponInfo->xDamageRadius+VOLATILE_WALL_DAMAGE_RADIUS,
											  nStrength / 2 + VOLATILE_WALL_DAMAGE_FORCE, cType.laserInfo.parent.nObject);
	Die ();		//make flares die in lava
	}
else if (pSide && ((gameData.pig.tex.pTexMapInfo [pSide->m_nBaseTex].flags & TMI_WATER) ||
			          (pSide->m_nOvlTex && (gameData.pig.tex.pTexMapInfo [pSide->m_nOvlTex].flags & TMI_WATER)))) {
	//we've hit water
	//	MK: 09/13/95: SplashDamage in water is 1/2 Normal intensity.
	if (pWeaponInfo->matter) {
		audio.CreateSegmentSound (SOUNDMSL_HIT_WATER, nHitSeg, 0, vHitPt);
		if (pWeaponInfo->xDamageRadius) {
			audio.CreateObjectSound (IsSplashDamageWeapon () ? SOUND_BADASS_EXPLOSION_WEAPON : SOUND_STANDARD_EXPLOSION, SOUNDCLASS_EXPLOSION, OBJ_IDX (this));
			//	MK: 09/13/95: SplashDamage in water is 1/2 Normal intensity.
			CreateSplashDamageExplosion (this, nHitSeg, vHitPt, vHitPt, pWeaponInfo->xImpactSize/2, pWeaponInfo->nRobotHitAnimation,
												  nStrength / 4, pWeaponInfo->xDamageRadius, nStrength / 2, cType.laserInfo.parent.nObject);
			}
		else
			CreateExplosion (info.nSegment, info.position.vPos, pWeaponInfo->xImpactSize, pWeaponInfo->nWallHitAnimation);
		}
	else {
		audio.CreateSegmentSound (SOUND_LASER_HIT_WATER, nHitSeg, 0, vHitPt);
		CreateExplosion (info.nSegment, info.position.vPos, pWeaponInfo->xImpactSize, ANIM_WATER_HIT);
		}
	Die ();		//make flares die in water
	}
else {
	if (!bBounce) {
		//if it's not the player's this, or it is the player's and there
		//is no CWall, and no blowing up monitor, then play sound
		if ((cType.laserInfo.parent.nType != OBJ_PLAYER) ||
			 (((wallType == WHP_NOT_SPECIAL)) && !bBlewUp))
			if ((pWeaponInfo->nWallHitSound > -1) && !(info.nFlags & OF_SILENT))
				CreateSound (pWeaponInfo->nWallHitSound);
		if (pWeaponInfo->nWallHitAnimation > -1) {
			if (pWeaponInfo->xDamageRadius)
				ExplodeSplashDamageWeapon (vHitPt);
			else
				CreateExplosion (info.nSegment, info.position.vPos, pWeaponInfo->xImpactSize, pWeaponInfo->nWallHitAnimation);
			}
		}
	}
//	If this fired by player or companion...
if ((cType.laserInfo.parent.nType == OBJ_PLAYER) || bEscort) {
	if (!(info.nFlags & OF_SILENT) &&
		 (cType.laserInfo.parent.nObject == LOCALPLAYER.nObject))
		CreateAwarenessEvent (this, PA_WEAPON_WALL_COLLISION);			// CObject "this" can attract attention to player

//		if (info.nId != FLARE_ID) {
//	We now allow flares to open doors.

	if (!bBounce && ((info.nId != FLARE_ID) || (cType.laserInfo.parent.nType != OBJ_PLAYER))) {
		Die ();
		}

	//don't let flares stick in vForce fields
	if ((info.nId == FLARE_ID) && (!pSide || (gameData.pig.tex.pTexMapInfo [pSide->m_nBaseTex].flags & TMI_FORCE_FIELD))) {
		Die ();
		}
	if (!(info.nFlags & OF_SILENT)) {
		switch (wallType) {
			case WHP_NOT_SPECIAL:
				//should be handled above
				//audio.CreateSegmentSound (pWeaponInfo->nWallHitSound, info.nSegment, 0, &info.position.vPos, 0, I2X (1));
				break;

			case WHP_NO_KEY:
				//play special hit door sound (if/when we get it)
				CreateSound (SOUND_WEAPON_HIT_DOOR);
			   if (IsMultiGame)
					MultiSendPlaySound (SOUND_WEAPON_HIT_DOOR, I2X (1));
				break;

			case WHP_BLASTABLE:
				//play special blastable CWall sound (if/when we get it)
				if ((pWeaponInfo->nWallHitSound > -1) && (!(info.nFlags & OF_SILENT)))
					CreateSound (SOUND_WEAPON_HIT_BLASTABLE);
				break;

			case WHP_DOOR:
				//don't play anything, since door open sound will play
				break;
			}
		}
	}
else {
	// This is a pRobot's laser
	if (!bBounce)
		Die ();
	}
return 1;
}

//	-----------------------------------------------------------------------------
//##void CollideCameraAndWall (CObject* camera, fix xHitSpeed, int16_t nHitSeg, int16_t nHitWall,   CFixVector& vHitPt) {
//##	return;
//##}

//##void CollidePowerupAndWall (CObject* powerup, fix xHitSpeed, int16_t nHitSeg, int16_t nHitWall,   CFixVector& vHitPt) {
//##	return;
//##}

int32_t CObject::CollideDebrisAndWall (fix xHitSpeed, int16_t nHitSeg, int16_t nHitWall, CFixVector& vHitPt)
{
if (gameOpts->render.nDebrisLife) {
	CFixVector	vDir = mType.physInfo.velocity,
					vNormal = SEGMENT (nHitSeg)->m_sides [nHitWall].m_normals [0];
	mType.physInfo.velocity = CFixVector::Reflect(vDir, vNormal);
	audio.CreateSegmentSound (SOUND_PLAYER_HIT_WALL, nHitSeg, 0, vHitPt, 0, I2X (1) / 3);
	}
else
	Explode (0);
return 1;
}

//##void CollideFireballAndFireball (CObject* fireball1, CObject* fireball2, CFixVector& vHitPt) {
//##	return;
//##}

//##void CollideFireballAndRobot (CObject* fireball, CObject* pRobot, CFixVector& vHitPt) {
//##	return;
//##}

//##void CollideFireballAndHostage (CObject* fireball, CObject* hostage, CFixVector& vHitPt) {
//##	return;
//##}

//##void CollideFireballAndPlayer (CObject* fireball, CObject* CPlayerData, CFixVector& vHitPt) {
//##	return;
//##}

//##void CollideFireballAndWeapon (CObject* fireball, CObject* pWeapon, CFixVector& vHitPt) {
//##	//pWeapon->Die ();
//##	return;
//##}

//##void CollideFireballAndCamera (CObject* fireball, CObject* camera, CFixVector& vHitPt) {
//##	return;
//##}

//##void CollideFireballAndPowerup (CObject* fireball, CObject* powerup, CFixVector& vHitPt) {
//##	return;
//##}

//##void CollideFireballAndDebris (CObject* fireball, CObject* debris, CFixVector& vHitPt) {
//##	return;
//##}

//	-------------------------------------------------------------------------------------------------------------------

int32_t CObject::CollideRobotAndRobot (CObject* other, CFixVector& vHitPt, CFixVector* vNormal)
{
//		robot1-OBJECTS, X2I (robot1->info.position.vPos.x), X2I (robot1->info.position.vPos.y), X2I (robot1->info.position.vPos.z),
//		robot2-OBJECTS, X2I (robot2->info.position.vPos.x), X2I (robot2->info.position.vPos.y), X2I (robot2->info.position.vPos.z),
//		X2I (vHitPt->x), X2I (vHitPt->y), X2I (vHitPt->z));

BumpTwoObjects (this, other, 1, vHitPt);
return 1;
}

//	-----------------------------------------------------------------------------

int32_t CObject::CollideRobotAndReactor (CObject* pReactor, CFixVector& vHitPt, CFixVector* vNormal)
{
if (info.nType == OBJ_ROBOT) {
	CFixVector vHit = pReactor->info.position.vPos - info.position.vPos;
	CFixVector::Normalize (vHit);
	Bump (vHit, 0);
	}
else {
	pReactor->CollideRobotAndReactor (this, vHitPt);
	}
return 1;
}

//	-----------------------------------------------------------------------------
//##void CollideRobotAndHostage (CObject* pRobot, CObject* hostage, CFixVector& vHitPt) {
//##	return;
//##}

//	-----------------------------------------------------------------------------

int32_t CObject::CollideRobotAndPlayer (CObject* pPlayerObj, CFixVector& vHitPt, CFixVector* vNormal)
{
if (!IsStatic ()) {
		int32_t	bTheftAttempt = 0;
		int16_t	nCollisionSeg;

	if (info.nFlags & OF_EXPLODING)
		return 1;
	nCollisionSeg = FindSegByPos (vHitPt, pPlayerObj->info.nSegment, 1, 0);
	if (nCollisionSeg != -1)
		CreateExplosion (nCollisionSeg, vHitPt, gameData.weapons.info [0].xImpactSize, gameData.weapons.info [0].nWallHitAnimation);
	if (pPlayerObj->info.nId == N_LOCALPLAYER) {
		if (IsGuideBot ())
			return 0; //	Player and companion don't collide.
		tRobotInfo *pRobotInfo = ROBOTINFO (info.nId);
		if (pRobotInfo && pRobotInfo->kamikaze) {
			ApplyDamageToRobot (info.xShield + 1, OBJ_IDX (pPlayerObj));
	#if DBG
			if (pPlayerObj == gameData.objData.pConsole)
	#else
			if (!(gameStates.app.bGameSuspended & SUSP_ROBOTS) && (pPlayerObj == gameData.objData.pConsole))
	#endif
				cockpit->AddPointsToScore (pRobotInfo->scoreValue);
			}
		if (pRobotInfo && pRobotInfo->thief) {
			if (gameData.ai.localInfo [OBJ_IDX (this)].mode == AIM_THIEF_ATTACK) {
				gameData.time.xLastThiefHitTime = gameData.time.xGame;
				AttemptToStealItem (this, pPlayerObj->info.nId);
				bTheftAttempt = 1;
				}
			else if (gameData.time.xGame - gameData.time.xLastThiefHitTime < I2X (2))
				return 1;	//	ZOUNDS! BRILLIANT! Thief not Collide with player if not stealing!
								// NO! VERY DUMB! makes thief look very stupid if player hits him while cloaked!-AP
			else
				gameData.time.xLastThiefHitTime = gameData.time.xGame;
			}
		CreateAwarenessEvent (pPlayerObj, PA_PLAYER_COLLISION);			// CObject this can attract attention to player
		if (USE_D1_AI) {
			DoD1AIRobotHitAttack (this, pPlayerObj, &vHitPt);
			DoD1AIRobotHit (this, WEAPON_ROBOT_COLLISION);
			}
		else {
			DoAIRobotHitAttack (this, pPlayerObj, &vHitPt);
			DoAIRobotHit (this, WEAPON_ROBOT_COLLISION);
			}
		}
	else
		MultiRobotRequestChange (this, pPlayerObj->info.nId);
	// added this if to remove the bump sound if it's the thief.
	// A "steal" sound was added and it was getting obscured by the bump. -AP 10/3/95
	//	Changed by MK to make this sound unless the this stole.
	if (!(bTheftAttempt || ROBOTINFO (info.nId)->energyDrain))
		audio.CreateSegmentSound (SOUND_ROBOT_HIT_PLAYER, pPlayerObj->info.nSegment, 0, vHitPt);
	}
BumpTwoObjects (this, pPlayerObj, 1, vHitPt, vNormal);
return 1;
}

//	-----------------------------------------------------------------------------
// Provide a way for network message to instantly destroy the control center
// without awarding points or anything.

//	if controlcen == NULL, that means don't do the explosion because the control center
//	was actually in another CObject.
int32_t NetDestroyReactor (CObject* pReactor)
{
if (extraGameInfo [0].nBossCount [0] && !gameData.reactor.bDestroyed) {
	--extraGameInfo [0].nBossCount [0];
	--extraGameInfo [0].nBossCount [1];
	DoReactorDestroyedStuff (pReactor);
	if (pReactor && !(pReactor->info.nFlags & (OF_EXPLODING|OF_DESTROYED))) {
		audio.CreateSegmentSound (SOUND_CONTROL_CENTER_DESTROYED, pReactor->info.nSegment, 0, pReactor->info.position.vPos);
		pReactor->Explode (0);
		}
	return 1;
	}
return 0;
}

//	-----------------------------------------------------------------------------

void CObject::ApplyDamageToReactor (fix xDamage, int16_t nAttacker)
{
	int32_t	i;

	//	Only allow a player to xDamage the control center.
CObject *pAttacker = OBJECT (nAttacker);
if (!pAttacker)
	return;
int32_t attackerType = pAttacker->info.nType;
if (attackerType != OBJ_PLAYER) {
#if TRACE
	console.printf (CON_DBG, "Damage to control center by CObject of nType %i prevented by MK! \n", whotype);
#endif
	return;
	}
if (IsMultiGame && !IsCoopGame && (LOCALPLAYER.timeLevel < netGameInfo.GetControlInvulTime ())) {
	if (pAttacker->info.nId == N_LOCALPLAYER) {
		int32_t t = netGameInfo.GetControlInvulTime () - LOCALPLAYER.timeLevel;
		int32_t secs = X2I (t) % 60;
		int32_t mins = X2I (t) / 60;
		HUDInitMessage ("%s %d:%02d.", TXT_CNTRLCEN_INVUL, mins, secs);
		}
	return;
	}
if (pAttacker->info.nId == N_LOCALPLAYER) {
	if (0 >= (i = FindReactor (this)))
		gameData.reactor.states [i].bHit = 1;
	AIDoCloakStuff ();
	}
if (info.xShield >= 0)
	info.xShield -= xDamage;
if ((info.xShield < 0) && !(info.nFlags & (OF_EXPLODING | OF_DESTROYED))) {
	/*if (gameStates.app.bD2XLevel && gameStates.gameplay.bMultiBosses)*/
	--extraGameInfo [0].nBossCount [0];
	--extraGameInfo [0].nBossCount [1];
	DoReactorDestroyedStuff (this);
	if (IsMultiGame) {
#if DBG
		if (nAttacker == LOCALPLAYER.nObject)
#else
		if (!(gameStates.app.bGameSuspended & SUSP_ROBOTS) && (nAttacker == LOCALPLAYER.nObject))
#endif
			cockpit->AddPointsToScore (CONTROL_CEN_SCORE);
		MultiSendDestroyReactor (OBJ_IDX (this), OBJECT (nAttacker)->info.nId);
		}
#if DBG
	else
#else
	else if (!(gameStates.app.bGameSuspended & SUSP_ROBOTS))
#endif
		cockpit->AddPointsToScore (CONTROL_CEN_SCORE);
	CreateSound (SOUND_CONTROL_CENTER_DESTROYED);
	Explode (0);
	}
}

//	-----------------------------------------------------------------------------

int32_t CObject::CollidePlayerAndReactor (CObject* pReactor, CFixVector& vHitPt, CFixVector* vNormal)
{
	int32_t	i;

if (info.nId == N_LOCALPLAYER) {
	if (0 >= (i = FindReactor (pReactor)))
		gameData.reactor.states [i].bHit = 1;
	AIDoCloakStuff ();				//	In case player cloaked, make control center know where he is.
	}
if (BumpTwoObjects (pReactor, this, 1, vHitPt))
	audio.CreateSegmentSound (SOUND_ROBOT_HIT_PLAYER, info.nSegment, 0, vHitPt);
return 1;
}

//	-----------------------------------------------------------------------------

int32_t CObject::CollidePlayerAndMarker (CObject* pMarker, CFixVector& vHitPt, CFixVector* vNormal)
{
#if TRACE
console.printf (CON_DBG, "Collided with pMarker %d! \n", pMarker->info.nId);
#endif
if (info.nId == N_LOCALPLAYER) {
	int32_t bDrawn;

	if (IsMultiGame && !IsCoopGame)
		bDrawn = HUDInitMessage (TXT_MARKER_PLRMSG, PLAYER (pMarker->info.nId / 2).callsign, markerManager.Message (pMarker->info.nId));
	else {
		if (*markerManager.Message (pMarker->info.nId))
			bDrawn = HUDInitMessage (TXT_MARKER_IDMSG, pMarker->info.nId + 1, markerManager.Message (pMarker->info.nId));
		else
			bDrawn = HUDInitMessage (TXT_MARKER_ID, pMarker->info.nId + 1);
		}
	if (bDrawn)
		audio.PlaySound (SOUND_MARKER_HIT);
	DetectEscortGoalAccomplished (OBJ_IDX (pMarker));
   }
return 1;
}

//	-----------------------------------------------------------------------------
//	If a persistent weapon and other object is not a pWeapon, weaken it, else kill it.
//	If both OBJECTS are weapons, weaken the weapon.
void CObject::MaybeKillWeapon (CObject* pOtherObj)
{
if (IsMine ()) {
	Die ();
	return;
	}
if (mType.physInfo.flags & PF_PERSISTENT) {
	//	Weapons do a lot of damage to weapons, other OBJECTS do much less.
	if (!(pOtherObj->mType.physInfo.flags & PF_PERSISTENT)) {
		info.xShield -= pOtherObj->info.xShield / ((pOtherObj->info.nType == OBJ_WEAPON) ? 2 : 4);
		if (info.xShield <= 0) {
			SetShield (0);
			Die ();	// info.xLifeLeft = 1;
			}
		}
	}
else {
	Die ();	// info.xLifeLeft = 1;
	}
}

//	-----------------------------------------------------------------------------

int32_t CObject::CollideWeaponAndReactor (CObject* pReactor, CFixVector& vHitPt, CFixVector* vNormal)
{
	int32_t	i;

if (info.nId == OMEGA_ID)
	if (!OkToDoOmegaDamage (this))
		return 1;
if (cType.laserInfo.parent.nType == OBJ_PLAYER) {
	fix damage = info.xShield;
	CObject* pParent = OBJECT (cType.laserInfo.parent.nObject);
	if (pParent && (pParent->info.nId == N_LOCALPLAYER))
		if (0 <= (i = FindReactor (pReactor)))
			gameData.reactor.states [i].bHit = 1;
	if (WI_damage_radius (info.nId))
		ExplodeSplashDamageWeapon (vHitPt, pReactor);
	else
		CreateExplosion (pReactor->info.nSegment, vHitPt, 3 * pReactor->info.xSize / 20, ANIM_SMALL_EXPLOSION);
	audio.CreateSegmentSound (SOUND_CONTROL_CENTER_HIT, pReactor->info.nSegment, 0, vHitPt);
	damage = FixMul (damage, cType.laserInfo.xScale);
	pReactor->ApplyDamageToReactor (damage, cType.laserInfo.parent.nObject);
	MaybeKillWeapon (pReactor);
	}
else {	//	If pRobot this hits control center, blow it up, make it go away, but do no damage to control center.
	CreateExplosion (pReactor->info.nSegment, vHitPt, 3 * pReactor->info.xSize / 20, ANIM_SMALL_EXPLOSION);
	MaybeKillWeapon (pReactor);
	}
return 1;
}

//	-----------------------------------------------------------------------------

int32_t CObject::CollideWeaponAndClutter (CObject* pClutter, CFixVector& vHitPt, CFixVector* vNormal)
{
uint8_t exp_vclip = ANIM_SMALL_EXPLOSION;
if (pClutter->info.xShield >= 0)
	pClutter->info.xShield -= info.xShield;
audio.CreateSegmentSound (SOUND_LASER_HIT_CLUTTER, (int16_t) info.nSegment, 0, vHitPt);
CreateExplosion ((int16_t) pClutter->info.nSegment, vHitPt, ((pClutter->info.xSize / 3) * 3) / 4, exp_vclip);
if ((pClutter->info.xShield < 0) && !(pClutter->info.nFlags & (OF_EXPLODING | OF_DESTROYED)))
	pClutter->Explode (STANDARD_EXPL_DELAY);
MaybeKillWeapon (pClutter);
return 1;
}

//--mk, 121094 -- extern void spinRobot (CObject* pRobot, CFixVector& vHitPt);

fix	nFinalBossCountdownTime = 0;

//	------------------------------------------------------------------------------------------------------

void DoFinalBossFrame (void)
{
if (!gameStates.gameplay.bFinalBossIsDead)
	return;
if (!gameData.reactor.bDestroyed)
	return;
if (nFinalBossCountdownTime == 0)
	nFinalBossCountdownTime = I2X (2);
nFinalBossCountdownTime -= gameData.time.xFrame;
if (nFinalBossCountdownTime > 0)
	return;
paletteManager.DisableEffect ();
StartEndLevelSequence (0);		//pretend we hit the exit CTrigger
}

//	------------------------------------------------------------------------------------------------------
//	This is all the ugly stuff we do when you kill the final boss so that you don't die or something
//	which would ruin the logic of the cut sequence.
void DoFinalBossHacks (void)
{
if (gameStates.app.bPlayerIsDead) {
	Int3 ();		//	Uh-oh, player is dead.  Try to rescue him.
	gameStates.app.bPlayerIsDead = 0;
	}
if (LOCALPLAYER.Shield () <= 0)
	LOCALPLAYER.SetShield (1);
//	If you're not invulnerable, get invulnerable!
if (!(LOCALPLAYER.flags & PLAYER_FLAGS_INVULNERABLE)) {
	LOCALPLAYER.invulnerableTime = gameData.time.xGame;
	LOCALPLAYER.flags |= PLAYER_FLAGS_INVULNERABLE;
	gameData.multiplayer.spherePulse [N_LOCALPLAYER].Setup (0.02f, 0.5f);
	}
if (!IsMultiGame)
	BuddyMessage ("Nice job, %s!", LOCALPLAYER.callsign);
missionManager.AdvanceLevel ();
gameStates.gameplay.bFinalBossIsDead = 1;
}

extern int32_t MultiAllPlayersAlive ();
void MultiSendFinishGame ();

//	------------------------------------------------------------------------------------------------------

bool CObject::Indestructible (void)
{
tRobotInfo* pRobotInfo = ROBOTINFO (info.nId);
return pRobotInfo ? pRobotInfo->strength <= 0 : false; // indestructible static object
}

//	------------------------------------------------------------------------------------------------------
//	Return 1 if this died, else return 0
int32_t CObject::ApplyDamageToRobot (fix xDamage, int32_t nKillerObj)
{
	char		bIsThief, bIsBoss;
	char		tempStolen [MAX_STOLEN_ITEMS];
	CObject	*pKillerObj = (nKillerObj < 0) ? NULL : OBJECT (nKillerObj);

tRobotInfo* pRobotInfo = ROBOTINFO (info.nId);
if (!pRobotInfo)
	return 0;

if (info.nFlags & OF_EXPLODING)
	return 0;
if (info.xShield < 0)
	return 0;	//this already dead...
if (Indestructible ()) // indestructible static object
	return 0; 
if (gameData.time.xGame - CreationTime () < I2X (1))
	return 0;
if (!AttacksRobots ()) {
	// guidebot may kill other bots
	if (pKillerObj && (pKillerObj->info.nType == OBJ_ROBOT) && !pKillerObj->IsGuideBot ())
		return 0;
	}
if ((bIsBoss = IsBoss ())) {
	int32_t i = gameData.bosses.Find (OBJ_IDX (this));
	if (i >= 0) {
		gameData.bosses [i].m_nHitTime = gameData.time.xGame;
		gameData.bosses [i].m_bHasBeenHit = 1;
		}
	}

//	Buddy invulnerable on level 24 so he can give you his important messages.  Bah.
//	Also invulnerable if his cheat for firing weapons is in effect.
if (IsGuideBot ()) {
	if ((missionManager.nCurrentMission == missionManager.nBuiltInMission [0]) &&
		 (missionManager.nCurrentLevel == missionManager.nLastLevel))
		return 0;
	}
SetTimeLastHit (gameStates.app.nSDLTicks [0]);
info.xShield -= xDamage;
//	Do unspeakable hacks to make sure player doesn't die after killing boss.  Or before, sort of.
if (bIsBoss) {
	if ((missionManager.nCurrentMission == missionManager.nBuiltInMission [0]) &&
		 (missionManager.nCurrentLevel == missionManager.nLastLevel) &&
		 (info.xShield < 0) && (extraGameInfo [0].nBossCount [0] == 1)) {
		if (IsMultiGame) {
			if (!MultiAllPlayersAlive ()) // everyones gotta be alive
				SetShield (1);
			else {
				MultiSendFinishGame ();
				DoFinalBossHacks ();
				}
			}
		else {	// NOTE LINK TO ABOVE!!!
			if ((LOCALPLAYER.Shield () < 0) || gameStates.app.bPlayerIsDead)
				SetShield (1);		//	Sorry, we can't allow you to kill the final boss after you've died.  Rough luck.
			else
				DoFinalBossHacks ();
			}
		}
	}

if (info.xShield >= 0) {
	if (pKillerObj == gameData.objData.pConsole)
		ExecObjTriggers (OBJ_IDX (this), 1);
	return 0;
	}
if (IsMultiGame) {
	bIsThief = IsThief ();
	if (bIsThief)
		memcpy (tempStolen, &gameData.thief.stolenItems [0], gameData.thief.stolenItems.Size ());
	if (IsMultiGame)
		gameStates.app.SRand (); // required for sync'ing the stuff the robot drops/spawns on the clients 
	if (!MultiExplodeRobot (OBJ_IDX (this), nKillerObj, bIsThief)) 
		return 0;
	if (bIsThief)
		memcpy (&gameData.thief.stolenItems [0], tempStolen, gameData.thief.stolenItems.Size ());
	MultiSendRobotExplode (OBJ_IDX (this), nKillerObj, bIsThief);
	if (bIsThief)
		gameData.thief.stolenItems.Clear (char (0xff));
	return 1;
	}

if (nKillerObj >= 0) {
	LOCALPLAYER.numKillsLevel++;
	LOCALPLAYER.numKillsTotal++;
	}

if (bIsBoss)
	StartBossDeathSequence (this);	//DoReactorDestroyedStuff (NULL);
else if (pRobotInfo->bDeathRoll)
	StartRobotDeathSequence (this);	//DoReactorDestroyedStuff (NULL);
else {
	if (info.nId == SPECIAL_REACTOR_ROBOT)
		SpecialReactorStuff ();
	Explode (pRobotInfo->kamikaze ? 1 : STANDARD_EXPL_DELAY);		//	Kamikaze, explode right away, IN YOUR FACE!
	}
return 1;
}

//	------------------------------------------------------------------------------------------------------

int32_t	nBuddyGaveHintCount = 5;
fix	xLastTimeBuddyGameHint = 0;

//	Return true if damage done to boss, else return false.
int32_t DoBossWeaponCollision (CObject* pRobot, CObject* pWeapon, CFixVector& vHitPt)
{
	int32_t	d2BossIndex;
	int32_t	bDamage = 1;
	int32_t	bKinetic = WI_matter (pWeapon->info.nId);

tRobotInfo* pRobotInfo = ROBOTINFO (pRobot);
if (!pRobotInfo)
	return 0;
d2BossIndex = pRobotInfo->bossFlag - BOSS_D2;
Assert ((d2BossIndex >= 0) && (d2BossIndex < NUM_D2_BOSSES));

//	See if should spew a bot.
if (pWeapon->cType.laserInfo.parent.nType == OBJ_PLAYER) {
	if ((bKinetic && bossProps [gameStates.app.bD1Mission][d2BossIndex].bSpewBotsKinetic) ||
		 (!bKinetic && bossProps [gameStates.app.bD1Mission][d2BossIndex].bSpewBotsEnergy)) {
		int32_t i = gameData.bosses.Find (OBJ_IDX (pRobot));
		if (i >= 0) {
			if (bossProps [gameStates.app.bD1Mission][d2BossIndex].bSpewMore && (RandShort () > SHORT_RAND_MAX / 2) &&
				 (pRobot->BossSpewRobot (&vHitPt, -1, 0) != -1))
				gameData.bosses [i].m_nLastGateTime = gameData.time.xGame - gameData.bosses [i].m_nGateInterval - 1;	//	Force allowing spew of another bot.
			pRobot->BossSpewRobot (&vHitPt, -1, 0);
			}
		}
	}

if (bossProps [gameStates.app.bD1Mission][d2BossIndex].bInvulSpot) {
	fix			dot;
	CFixVector	tvec1;

	//	Boss only vulnerable in back.  See if hit there.
	tvec1 = vHitPt - pRobot->info.position.vPos;
	CFixVector::Normalize (tvec1);	//	Note, if BOSS_INVULNERABLE_DOT is close to I2X (1) (in magnitude), then should probably use non-quick version.
	dot = CFixVector::Dot (tvec1, pRobot->info.position.mOrient.m.dir.f);
#if TRACE
	console.printf (CON_DBG, "Boss hit vec dot = %7.3f \n", X2F (dot));
#endif
	if (dot > gameData.physics.xBossInvulDot) {
		int16_t	nSegment = FindSegByPos (vHitPt, pRobot->info.nSegment, 1, 0);
		audio.CreateSegmentSound (SOUND_WEAPON_HIT_DOOR, nSegment, 0, vHitPt);
		bDamage = 0;

		if (xLastTimeBuddyGameHint == 0)
			xLastTimeBuddyGameHint = RandShort () * 32 + I2X (16);

		if (nBuddyGaveHintCount) {
			if (xLastTimeBuddyGameHint + I2X (20) < gameData.time.xGame) {
				int32_t	sval;

				nBuddyGaveHintCount--;
				xLastTimeBuddyGameHint = gameData.time.xGame;
				sval = (RandShort () * 4) >> 15;
				switch (sval) {
					case 0:
						BuddyMessage (TXT_BOSS_HIT_BACK);
						break;
					case 1:
						BuddyMessage (TXT_BOSS_VULNERABLE);
						break;
					case 2:
						BuddyMessage (TXT_BOSS_GET_BEHIND);
						break;
					case 3:
					default:
						BuddyMessage (TXT_BOSS_GLOW_SPOT);
						break;
					}
				}
			}

		//	Cause weapon to bounce.
		//	Make a copy of this pWeapon, because the physics wants to destroy it.
		if ((pWeapon->info.nType == OBJ_WEAPON) && !WI_matter (pWeapon->info.nId)) {
			int16_t nClone = CreateObject (pWeapon->info.nType, pWeapon->info.nId, -1, pWeapon->info.nSegment, pWeapon->info.position.vPos,
												    pWeapon->info.position.mOrient, pWeapon->info.xSize, 
												    pWeapon->info.controlType, pWeapon->info.movementType, pWeapon->info.renderType);
			if (nClone != -1) {
				CObject	*pClone = OBJECT (nClone);
				if (pWeapon->info.renderType == RT_POLYOBJ) {
					pClone->rType.polyObjInfo.nModel = gameData.weapons.info [pClone->info.nId].nModel;
					pClone->AdjustSize (0, gameData.weapons.info [pClone->info.nId].poLenToWidthRatio);
					}
				pClone->mType.physInfo.thrust.SetZero ();
				pClone->mType.physInfo.mass = WI_mass (pWeapon->info.nType);
				pClone->mType.physInfo.drag = WI_drag (pWeapon->info.nType);
				CFixVector vImpulse = vHitPt - pRobot->info.position.vPos;
				CFixVector::Normalize (vImpulse);
				CFixVector vWeapon = pWeapon->mType.physInfo.velocity;
				fix speed = CFixVector::Normalize (vWeapon);
				vImpulse += vWeapon * (-I2X (2));
				vImpulse *= (speed / 4);
				pClone->mType.physInfo.velocity = vImpulse;
				pClone->info.nFlags |= PF_BOUNCED_ONCE;
				}
			}
		}
	}
else if ((bKinetic && bossProps [gameStates.app.bD1Mission][d2BossIndex].bInvulKinetic) ||
		   (!bKinetic && bossProps [gameStates.app.bD1Mission][d2BossIndex].bInvulEnergy)) {
	int16_t	nSegment;

	nSegment = FindSegByPos (vHitPt, pRobot->info.nSegment, 1, 0);
	audio.CreateSegmentSound (SOUND_WEAPON_HIT_DOOR, nSegment, 0, vHitPt);
	bDamage = 0;
	}
return bDamage;
}

//	------------------------------------------------------------------------------------------------------

int32_t FindHitObject (CObject* pObj, int16_t nObject)
{
	int16_t	*p = gameData.objData.nHitObjects + pObj->Index () * MAX_HIT_OBJECTS;
	int32_t	i;

for (i = pObj->cType.laserInfo.nLastHitObj; i; i--, p++)
	if (*p == nObject)
		return 1;
return 0;
}

//	------------------------------------------------------------------------------------------------------

int32_t AddHitObject (CObject* pObj, int16_t nObject)
{
	int16_t	*p;
	int32_t	i;

if (FindHitObject (pObj, nObject))
	return -1;
p = gameData.objData.nHitObjects + pObj->Index () * MAX_HIT_OBJECTS;
i = pObj->cType.laserInfo.nLastHitObj;
if (i >= MAX_HIT_OBJECTS) {
	memcpy (p + 1, p, (MAX_HIT_OBJECTS - 1) * sizeof (*p));
	p [i - 1] = nObject;
	}
else {
	p [i] = nObject;
	pObj->cType.laserInfo.nLastHitObj++;
	}
return 1;
}

//	------------------------------------------------------------------------------------------------------

int32_t CObject::CollideWeaponAndRobot (CObject* pRobot, CFixVector& vHitPt, CFixVector* vNormal)
{
if (pRobot->IsGeometry ())
	return CollideWeaponAndWall (WI_speed (info.nId, gameStates.app.nDifficultyLevel), pRobot->Segment (), -1, vHitPt);

	tRobotInfo	*pRobotInfo = ROBOTINFO (pRobot);

if (!pRobotInfo && (pRobot->Type () != OBJ_CAMBOT)) {
	PrintLog (0, "invalid robot reference in CollideWeaponAndRobot (type = %d, id = %d)\n", pRobot->Type (), pRobot->Id ());
	return 1;
	}

	int32_t		bDamage = 1;
	int32_t		bInvulBoss = 0;
	fix			nStrength = WI_strength (info.nId, gameStates.app.nDifficultyLevel);
	CObject		*pParent = (cType.laserInfo.parent.nType != OBJ_ROBOT) ? NULL : OBJECT (cType.laserInfo.parent.nObject);
	CWeaponInfo *pWeaponInfo = gameData.weapons.info + info.nId;
	bool			bAttackRobots = pParent ? pParent->AttacksRobots () || (EGI_FLAG (bRobotsHitRobots, 0, 0, 0) && gameStates.app.cheats.bRobotsKillRobots) : false;

#if DBG
if (OBJ_IDX (this) == nDbgObj)
	BRP;
#endif
if (info.nId == PROXMINE_ID) {
	if (IsMultiGame && !COMPETITION && EGI_FLAG (bSmokeGrenades, 0, 0, 0))
		return 1;
	}
else if (info.nId == OMEGA_ID) {
	if (!OkToDoOmegaDamage (this))
		return 1;
	}
if (pRobotInfo && pRobotInfo->bossFlag) {
	int32_t i = gameData.bosses.Find (OBJ_IDX (pRobot));
	if (i >= 0)
		gameData.bosses [i].m_nHitTime = gameData.time.xGame;
	if (pRobotInfo->bossFlag >= BOSS_D2) {
		bDamage = DoBossWeaponCollision (pRobot, this, vHitPt);
		bInvulBoss = !bDamage;
		}
	}

//	Put in at request of Jasen (and Adam) because the Buddy-Bot gets in their way.
//	MK has so much fun whacking his butt around the mine he never cared...
if ((cType.laserInfo.parent.nType == OBJ_ROBOT) && !bAttackRobots)
	return 1;
if (pRobotInfo && pRobotInfo->companion && (cType.laserInfo.parent.nType != OBJ_ROBOT))
	return 1;
CreateWeaponEffects (1);
if (info.nId == EARTHSHAKER_ID)
	ShakerRockStuff (&Position ());
//	If a persistent this hit pRobot most recently, quick abort, else we cream the same pRobot many times,
//	depending on frame rate.
if (mType.physInfo.flags & PF_PERSISTENT) {
	if (AddHitObject (this, OBJ_IDX (pRobot)) < 0)
		return 1;
	}
if (cType.laserInfo.parent.nSignature == pRobot->info.nSignature)
	return 1;
//	Changed, 10/04/95, put out blobs based on skill level and power of this doing damage.
//	Also, only a this hit from a tPlayer this causes smart blobs.
if (pRobotInfo && pRobotInfo->energyBlobs && (cType.laserInfo.parent.nType == OBJ_PLAYER))
	if (!pRobot->IsStatic () && (pRobot->info.xShield > 0) && IsEnergyProjectile ()) {
		fix xProb = (gameStates.app.nDifficultyLevel+2) * Min (info.xShield, pRobot->info.xShield);
		xProb = pRobotInfo->energyBlobs * xProb / (DIFFICULTY_LEVEL_COUNT * 32);
		int32_t nBlobs = xProb >> 16;
		if (2 * RandShort () < (xProb & 0xffff))
			nBlobs++;
		if (nBlobs)
			CreateSmartChildren (pRobot, nBlobs);
		}

	//	Note: If this hits an invulnerable boss, it will still do splash damage, including to the boss,
	//	unless this is trapped elsewhere.
	if (WI_damage_radius (info.nId)) {
		if (bInvulBoss) {			//don't make badass sound
			//this code copied from ExplodeSplashDamageWeapon ()
			CreateSplashDamageExplosion (this, info.nSegment, vHitPt, vHitPt, pWeaponInfo->xImpactSize, pWeaponInfo->nRobotHitAnimation, 
												  nStrength, pWeaponInfo->xDamageRadius, nStrength, cType.laserInfo.parent.nObject);

			}
		else		//Normal splash damage explosion
			ExplodeSplashDamageWeapon (vHitPt, pRobot);
		}
	if (((cType.laserInfo.parent.nType == OBJ_PLAYER) || bAttackRobots) && !(pRobot->info.nFlags & OF_EXPLODING)) {
		CObject *pParent, *pExplObj = NULL;
		if (cType.laserInfo.parent.nObject == LOCALPLAYER.nObject) {
			CreateAwarenessEvent (this, WEAPON_ROBOT_COLLISION);			// object "this" can attract attention to tPlayer
			if (USE_D1_AI)
				DoD1AIRobotHit (pRobot, WEAPON_ROBOT_COLLISION);
			else
				DoAIRobotHit (pRobot, WEAPON_ROBOT_COLLISION);
			}
	  	else if ((pParent = OBJECT (cType.laserInfo.parent.nObject)))
			MultiRobotRequestChange (pRobot, pParent->info.nId);
		if (pRobotInfo && (pRobotInfo->nExp1VClip > -1))
			pExplObj = CreateExplosion (info.nSegment, vHitPt, (3 * pRobot->info.xSize) / 8, (uint8_t) pRobotInfo->nExp1VClip);
		else if (gameData.weapons.info [info.nId].nRobotHitAnimation > -1)
			pExplObj = CreateExplosion (info.nSegment, vHitPt, pWeaponInfo->xImpactSize, (uint8_t) pWeaponInfo->nRobotHitAnimation);
		if (pExplObj)
			AttachObject (pRobot, pExplObj);
		if (bDamage && pRobotInfo && (pRobotInfo->nExp1Sound > -1))
			audio.CreateSegmentSound (pRobotInfo->nExp1Sound, pRobot->info.nSegment, 0, vHitPt);
		if (!(info.nFlags & OF_HARMLESS)) {
			fix xDamage = bDamage ? FixMul (info.xShield, cType.laserInfo.xScale) : 0;
			//	Cut Gauss xDamage on bosses because it just breaks the game.  Bosses are so easy to
			//	hit, and missing a pRobot is what prevents the Gauss from being game-breaking.
			if (info.nId == GAUSS_ID) {
				if (pRobotInfo && pRobotInfo->bossFlag)
					xDamage = (xDamage * (2 * DIFFICULTY_LEVEL_COUNT - gameStates.app.nDifficultyLevel)) / (2 * DIFFICULTY_LEVEL_COUNT);
				}
			else if (info.nId == FUSION_ID) {
				xDamage = gameData.FusionDamage (xDamage);
				}
			if (!pRobot->ApplyDamageToRobot (xDamage, cType.laserInfo.parent.nObject))
				BumpTwoObjects (pRobot, this, 0, vHitPt);		//only bump if not dead. no xDamage from bump
#if DBG
			else if (cType.laserInfo.parent.nSignature == gameData.objData.pConsole->info.nSignature) {
#else
			else if (pRobotInfo && !(gameStates.app.bGameSuspended & SUSP_ROBOTS) && (cType.laserInfo.parent.nSignature == gameData.objData.pConsole->info.nSignature)) {
#endif
				cockpit->AddPointsToScore (pRobotInfo->scoreValue);
				DetectEscortGoalAccomplished (OBJ_IDX (pRobot));
				}
			}
		//	If Gauss Cannon, spin pRobot.
		if (pRobot && (!pRobotInfo || !(pRobotInfo->companion || pRobotInfo->bossFlag)) && (info.nId == GAUSS_ID)) {
			tAIStaticInfo	*aip = &pRobot->cType.aiInfo;

			if (aip->SKIP_AI_COUNT * gameData.time.xFrame < I2X (1)) {
				aip->SKIP_AI_COUNT++;
				pRobot->mType.physInfo.rotThrust.v.coord.x = FixMul (SRandShort (), gameData.time.xFrame * aip->SKIP_AI_COUNT);
				pRobot->mType.physInfo.rotThrust.v.coord.y = FixMul (SRandShort (), gameData.time.xFrame * aip->SKIP_AI_COUNT);
				pRobot->mType.physInfo.rotThrust.v.coord.z = FixMul (SRandShort (), gameData.time.xFrame * aip->SKIP_AI_COUNT);
				pRobot->mType.physInfo.flags |= PF_USES_THRUST;
				}
			}
		}
MaybeKillWeapon (pRobot);
return 1;
}
//	-----------------------------------------------------------------------------

int32_t CObject::CollidePlayerAndHostage (CObject* pHostage, CFixVector& vHitPt, CFixVector* vNormal)
{
if (this == gameData.objData.pConsole) {
	DetectEscortGoalAccomplished (OBJ_IDX (pHostage));
	cockpit->AddPointsToScore (HOSTAGE_SCORE);
	// Do effect
	RescueHostage (pHostage->info.nId);
	// Remove the hostage CObject.
	pHostage->Die ();
	if (IsMultiGame)
		MultiSendRemoveObject (OBJ_IDX (pHostage));
	}
return 1;
}

//	-----------------------------------------------------------------------------

int32_t CObject::CollidePlayerAndPlayer (CObject* pOther, CFixVector& vHitPt, CFixVector* vNormal)
{
if (gameStates.app.bD2XLevel &&
	 (SEGMENT (info.nSegment)->HasNoDamageProp ()))
	return 1;
if (BumpTwoObjects (this, pOther, 1, vHitPt))
	audio.CreateSegmentSound (SOUND_ROBOT_HIT_PLAYER, info.nSegment, 0, vHitPt);
return 1;
}

//	-----------------------------------------------------------------------------

void CObject::ApplyDamageToPlayer (CObject* pAttackerObj, fix xDamage)
{
if (gameStates.app.bPlayerIsDead) {
	// PrintLog (0, "ApplyDamageToPlayer: Player is already dead\n");
	return;
	}
if (gameStates.app.bD2XLevel && (SEGMENT (info.nSegment)->HasNoDamageProp ())) {
	// PrintLog (0, "ApplyDamageToPlayer: No damage segment\n");
	return;
	}
if ((info.nId == N_LOCALPLAYER) && (LOCALPLAYER.flags & PLAYER_FLAGS_INVULNERABLE)) {
	// PrintLog (0, "ApplyDamageToPlayer: Player is invulnerable\n");
	return;
	}

CPlayerData *pAttacker; 

if (!pAttackerObj) 
	pAttacker = NULL;
else {
	if (pAttackerObj->IsGuideBot ()) {
		// PrintLog (0, "ApplyDamageToPlayer: Player was hit by Guidebot\n");
		return;
		}
	pAttacker = (pAttackerObj->info.nType == OBJ_PLAYER) ? gameData.multiplayer.players + pAttackerObj->info.nId : NULL;
	if (pAttacker)
		// PrintLog (0, "ApplyDamageToPlayer: Damage was inflicted by %s\n", pAttacker->callsign);
	if (gameStates.app.bHaveExtraGameInfo [1]) {
		if ((pAttackerObj == this) && !COMPETITION && extraGameInfo [1].bInhibitSuicide) {
			// PrintLog (0, "ApplyDamageToPlayer: Suicide inhibited\n");
			return;
			}
		else if (pAttacker && !(COMPETITION || extraGameInfo [1].bFriendlyFire)) {
			if (IsTeamGame) {
				if (GetTeam (info.nId) == GetTeam (pAttackerObj->info.nId)) {
					// PrintLog (0, "ApplyDamageToPlayer: Friendly fire suppressed (team game)\n");
					return;
					}
				}
			else if (IsCoopGame) {
				// PrintLog (0, "ApplyDamageToPlayer: Friendly fire suppressed (coop game)\n");
				return;
				}
			}
		}
	}
if (gameStates.app.bEndLevelSequence)
	return;

gameData.multiplayer.bWasHit [info.nId] = -1;

if (info.nId == N_LOCALPLAYER) {		//is this the local player?
	// PrintLog (0, "ApplyDamageToPlayer: Processing local player damage %d\n", xDamage);
	CPlayerData *pPlayer = gameData.multiplayer.players + info.nId;
	if (IsEntropyGame && extraGameInfo [1].entropy.bPlayerHandicap && pAttacker) {
		double h = (double) pPlayer->netKillsTotal / (double) (pAttacker->netKillsTotal + 1);
		if (h < 0.5)
			h = 0.5;
		else if (h > 1.0)
			h = 1.0;
		if (!(xDamage = (fix) ((double) xDamage * h)))
			xDamage = 1;
		// PrintLog (0, "ApplyDamageToPlayer: Applying player handicap (resulting damage = %d)\n", xDamage);
		}
	pPlayer->UpdateShield (-xDamage);
	paletteManager.BumpEffect (X2I (xDamage) * 4, -X2I (xDamage / 2), -X2I (xDamage / 2));	//flash red
	if (pPlayer->Shield () < 0) {
  		pPlayer->nKillerObj = OBJ_IDX (pAttackerObj);
		Die ();
		if (gameData.escort.nObjNum != -1)
			if (pAttackerObj && pAttackerObj->IsGuideBot ())
				gameData.escort.xSorryTime = gameData.time.xGame;
		}
	}
}

//	-----------------------------------------------------------------------------

int32_t CObject::CollideWeaponAndPlayer (CObject* pPlayerObj, CFixVector& vHitPt, CFixVector* vNormal)
{
	fix xDamage = info.xShield;

	//	In multiplayer games, only do xDamage to another player if in first frame.
	//	This is necessary because in multiplayer, due to varying framerates, omega blobs actually
	//	have a bit of a lifetime.  But they start out with a lifetime of ONE_FRAME_TIME, and this
	//	gets bashed to 1/4 second in laser_doWeapon_sequence.  This bashing occurs for visual purposes only.
if (gameStates.app.bD2XLevel && (SEGMENT (pPlayerObj->info.nSegment)->HasNoDamageProp ()))
	return 1;
if ((info.nId == PROXMINE_ID) && IsMultiGame && !COMPETITION && EGI_FLAG (bSmokeGrenades, 0, 0, 0))
	return 1;
if ((info.nId == OMEGA_ID) && !OkToDoOmegaDamage (this))
	return 1;
//	Don't Collide own smart mines unless direct hit.
if ((info.nId == SMARTMINE_ID) &&
	 (OBJ_IDX (pPlayerObj) == cType.laserInfo.parent.nObject) &&
	 (CFixVector::Dist (vHitPt, OBJPOS (pPlayerObj)->vPos) > pPlayerObj->info.xSize))
	return 1;
gameData.multiplayer.bWasHit [pPlayerObj->info.nId] = -1;
CreateWeaponEffects (1);
if (info.nId == EARTHSHAKER_ID)
	ShakerRockStuff (&Position ());
xDamage = FixMul (xDamage, cType.laserInfo.xScale);
if (info.nId == FUSION_ID)
	xDamage = gameData.FusionDamage (xDamage);
if (IsMultiGame) {
	if (gameData.weapons.info [info.nId].xMultiDamageScale <= 0) {
		PrintLog (0, "invalid multiplayer damage scale for weapon %d!\n", info.nId);
		gameData.weapons.info [info.nId].xMultiDamageScale = I2X (1);
		}
	xDamage = FixMul (xDamage, gameData.weapons.info [info.nId].xMultiDamageScale);
	}
if (mType.physInfo.flags & PF_PERSISTENT) {
	if (AddHitObject (this, OBJ_IDX (pPlayerObj)) < 0)
		return 1;
}
if (pPlayerObj->info.nId == N_LOCALPLAYER) {
	if (!(LOCALPLAYER.flags & PLAYER_FLAGS_INVULNERABLE)) {
		audio.CreateSegmentSound (SOUND_PLAYER_GOT_HIT, OBJSEG (pPlayerObj), 0, vHitPt);
		if (IsMultiGame)
			MultiSendPlaySound (SOUND_PLAYER_GOT_HIT, I2X (1));
		}
	else {
		audio.CreateSegmentSound (SOUND_WEAPON_HIT_DOOR, OBJSEG (pPlayerObj), 0, vHitPt);
		if (IsMultiGame)
			MultiSendPlaySound (SOUND_WEAPON_HIT_DOOR, I2X (1));
		}
	}
CreateExplosion (OBJSEG (pPlayerObj), vHitPt, I2X (10)/2, ANIM_PLAYER_HIT);
if (WI_damage_radius (info.nId))
	ExplodeSplashDamageWeapon (vHitPt, pPlayerObj);
MaybeKillWeapon (pPlayerObj);
BumpTwoObjects (pPlayerObj, this, 0, vHitPt);	//no xDamage from bump
CObject* pParent;
if (!WI_damage_radius (info.nId) && !(info.nFlags & OF_HARMLESS) && (pParent = OBJECT (cType.laserInfo.parent.nObject)))
	pPlayerObj->ApplyDamageToPlayer (pParent, xDamage);
//	Robots become aware of you if you get hit.
AIDoCloakStuff ();
return 1;
}

//	-----------------------------------------------------------------------------
//	Nasty robots are the ones that attack you by running into you and doing lots of damage.
int32_t CObject::CollidePlayerAndNastyRobot (CObject* pRobot, CFixVector& vHitPt, CFixVector* vNormal)
{
//	if (!(ROBOTINFO (pObj)->energyDrain && PLAYER (info.nId).energy))
tRobotInfo* pRobotInfo = ROBOTINFO (pRobot);
if (!pRobotInfo)
	return 0;
CreateExplosion (info.nSegment, vHitPt, I2X (10) / 2, ANIM_PLAYER_HIT);
if (BumpTwoObjects (this, pRobot, 0, vHitPt)) {//no damage from bump
	audio.CreateSegmentSound (pRobotInfo->clawSound, info.nSegment, 0, vHitPt);
	ApplyDamageToPlayer (pRobot, I2X (gameStates.app.nDifficultyLevel+1));
	}
return 1;
}

//	-----------------------------------------------------------------------------

int32_t CObject::CollidePlayerAndObjProducer (void)
{
	CFixVector vExitDir;

CreateSound (SOUND_PLAYER_GOT_HIT);
//	audio.PlaySound (SOUND_PLAYER_GOT_HIT);
CreateExplosion (info.nSegment, info.position.vPos, I2X (10) / 2, ANIM_PLAYER_HIT);
if (info.nId != N_LOCALPLAYER)
	return 1;
CSegment* pSeg = SEGMENT (info.nSegment);
for (int16_t nSide = 0; nSide < SEGMENT_SIDE_COUNT; nSide++)
	if (pSeg->IsPassable (nSide, this) & WID_PASSABLE_FLAG) {
		vExitDir = pSeg->SideCenter (nSide) - info.position.vPos;
		CFixVector::Normalize (vExitDir);
		CFixVector vRand = CFixVector::Random();
		vRand.v.coord.x /= 4;
		vRand.v.coord.y /= 4;
		vRand.v.coord.z /= 4;
		vExitDir += vRand;
		CFixVector::Normalize (vExitDir);
		}
Bump (vExitDir, I2X (64));
ApplyDamageToPlayer (this, I2X (4));	
return 1;
}

//	-----------------------------------------------------------------------------

int32_t CObject::CollideRobotAndObjProducer (void)
{
	CFixVector	vExitDir;
	CSegment*	pSeg = SEGMENT (info.nSegment);

tRobotInfo* pRobotInfo = ROBOTINFO (info.nId);
if (!pRobotInfo)
	return 0;
CreateSound (SOUND_ROBOT_HIT);
//	audio.PlaySound (SOUND_ROBOT_HIT);
if (pRobotInfo->nExp1VClip > -1)
	CreateExplosion ((int16_t) info.nSegment, info.position.vPos, 3 * info.xSize / 8, (uint8_t) pRobotInfo->nExp1VClip);
vExitDir.SetZero ();
for (int16_t nSide = 0; nSide < SEGMENT_SIDE_COUNT; nSide++)
	if (pSeg->IsPassable (nSide, NULL) & WID_PASSABLE_FLAG) {
		vExitDir = pSeg->SideCenter (nSide) - info.position.vPos;
		CFixVector::Normalize (vExitDir);
		}
if (!vExitDir.IsZero ())
	Bump (vExitDir, I2X (8));
ApplyDamageToRobot (I2X (1), -1);
return 1;
}

//##void CollidePlayerAndCamera (CObject* pPlayerObj, CObject* camera, CFixVector& vHitPt) {
//##	return;
//##}

//	-----------------------------------------------------------------------------

int32_t CObject::CollidePlayerAndPowerup (CObject* pPowerup, CFixVector& vHitPt, CFixVector* vNormal)
{
if (gameStates.app.bGameSuspended & SUSP_POWERUPS)
	return 1;
if (!gameStates.app.bEndLevelSequence && !gameStates.app.bPlayerIsDead && (info.nId == N_LOCALPLAYER)) {
	int32_t bPowerupUsed = DoPowerup (pPowerup, info.nId);
	if (bPowerupUsed) {
		pPowerup->Die ();
		if (IsMultiGame)
			MultiSendRemoveObject (OBJ_IDX (pPowerup));
		}
	}
else if (IsCoopGame && (info.nId != N_LOCALPLAYER)) {
	switch (pPowerup->info.nId) {
		case POW_KEY_BLUE:
			PLAYER (info.nId).flags |= PLAYER_FLAGS_BLUE_KEY;
			break;
		case POW_KEY_RED:
			PLAYER (info.nId).flags |= PLAYER_FLAGS_RED_KEY;
			break;
		case POW_KEY_GOLD:
			PLAYER (info.nId).flags |= PLAYER_FLAGS_GOLD_KEY;
			break;
		default:
			break;
		}
	}
DetectEscortGoalAccomplished (OBJ_IDX (pPowerup));
return 1;
}

//	-----------------------------------------------------------------------------

int32_t CObject::CollidePlayerAndMonsterball (CObject* monsterball, CFixVector& vHitPt, CFixVector* vNormal)
{
if (!gameStates.app.bEndLevelSequence && !gameStates.app.bPlayerIsDead &&
	(info.nId == N_LOCALPLAYER)) {
	if (BumpTwoObjects (this, monsterball, 0, vHitPt))
		audio.CreateSegmentSound (SOUND_ROBOT_HIT_PLAYER, info.nSegment, 0, vHitPt);
	}
return 1;
}

//	-----------------------------------------------------------------------------
//##void CollidePlayerAndDebris (CObject* pPlayerObj, CObject* debris, CFixVector& vHitPt) {
//##	return;
//##}

int32_t CObject::CollideActorAndClutter (CObject* clutter, CFixVector& vHitPt, CFixVector* vNormal)
{
if (gameStates.app.bD2XLevel &&
	 (SEGMENT (info.nSegment)->HasNoDamageProp ()))
	return 1;
if (!(info.nFlags & OF_EXPLODING) && BumpTwoObjects (clutter, this, 1, vHitPt))
	audio.CreateSegmentSound (SOUND_ROBOT_HIT_PLAYER, info.nSegment, 0, vHitPt);
return 1;
}

//	-----------------------------------------------------------------------------
//	See if pOther causes this weapon to create a splash damage explosion.  If so, create the explosion
//	Return true if weapon does proximity (as opposed to only contact) damage when it explodes.
int32_t CObject::MaybeDetonateWeapon (CObject* pOther, CFixVector& vHitPt)
{
if (!gameData.weapons.info [info.nId].xDamageRadius)
	return 0;
fix xDist = CFixVector::Dist (info.position.vPos, pOther->info.position.vPos);
if (xDist >= I2X (5))
	UpdateLife (Min (xDist / 64, I2X (1)));
else {
	MaybeKillWeapon (pOther);
	if (info.nFlags & OF_SHOULD_BE_DEAD) {
		CreateWeaponEffects (0);
		ExplodeSplashDamageWeapon (vHitPt, pOther);
		audio.CreateSegmentSound (gameData.weapons.info [info.nId].nRobotHitSound, info.nSegment, 0, vHitPt);
		}
	}
return 1;
}

//	-----------------------------------------------------------------------------

inline int32_t DestroyWeapon (int32_t nTarget, int32_t nWeapon)
{
if (WI_destructible (nTarget))
	return 1;
if (COMPETITION)
	return 0;
if (!CObject::IsMissile (nTarget))
	return 0;
if (EGI_FLAG (bKillMissiles, 0, 0, 0) == 2)
	return 1;
if (nWeapon != OMEGA_ID)
	return 0;
if (EGI_FLAG (bKillMissiles, 0, 0, 0) == 1)
	return 1;
return 0;
}

//	-----------------------------------------------------------------------------

int32_t CObject::CollideWeaponAndWeapon (CObject* other, CFixVector& vHitPt, CFixVector* vNormal)
{
	int32_t	id1 = info.nId;
	int32_t	id2 = other->info.nId;
	int32_t	bKill1, bKill2;

if (id1 == SMALLMINE_ID && id2 == SMALLMINE_ID)
	return 1;		//these can't blow each other up
if ((id1 == PROXMINE_ID || id2 == PROXMINE_ID) && IsMultiGame && !COMPETITION && EGI_FLAG (bSmokeGrenades, 0, 0, 0))
	return 1;
if (((id1 == OMEGA_ID) && !OkToDoOmegaDamage (this)) ||
    ((id2 == OMEGA_ID) && !OkToDoOmegaDamage (other)))
	return 1;
bKill1 = DestroyWeapon (id1, id2);
bKill2 = DestroyWeapon (id2, id1);
if (bKill1 || bKill2) {
	//	Bug reported by Adam Q. Pletcher on September 9, 1994, smart bomb homing missiles were toasting each other.
	if ((id1 == id2) && (cType.laserInfo.parent.nObject == other->cType.laserInfo.parent.nObject))
		return 1;
	if (bKill1)
		if (MaybeDetonateWeapon (other, vHitPt))
			other->MaybeDetonateWeapon (this, vHitPt);
	if (bKill2)
		if (other->MaybeDetonateWeapon (this, vHitPt))
			MaybeDetonateWeapon (other, vHitPt);
	}
return 1;
}

//	-----------------------------------------------------------------------------

int32_t CObject::CollideWeaponAndMonsterball (CObject* pMonsterball, CFixVector& vHitPt, CFixVector* vNormal)
{
if (cType.laserInfo.parent.nType == OBJ_PLAYER) {
	audio.CreateSegmentSound (SOUND_ROBOT_HIT, info.nSegment, 0, vHitPt);
	if (info.nId == EARTHSHAKER_ID)
		ShakerRockStuff (&Position ());
	if (mType.physInfo.flags & PF_PERSISTENT) {
		if (AddHitObject (this, OBJ_IDX (pMonsterball)) < 0)
			return 1;
		}
	CreateExplosion (pMonsterball->info.nSegment, vHitPt, I2X (5), ANIM_PLAYER_HIT);
	if (WI_damage_radius (info.nId))
		ExplodeSplashDamageWeapon (vHitPt, pMonsterball);
	MaybeKillWeapon (pMonsterball);
	BumpTwoObjects (this, pMonsterball, 1, vHitPt);
	}
return 1;
}

//	-----------------------------------------------------------------------------
//##void CollideWeaponAndCamera (CObject* pWeapon, CObject* camera, CFixVector& vHitPt) {
//##	return;
//##}

//	-----------------------------------------------------------------------------

int32_t CObject::CollideWeaponAndDebris (CObject* pDebris, CFixVector& vHitPt, CFixVector* vNormal)
{
//	Hack! Prevent pDebris from causing bombs spewed at player death to detonate!
if (IsMine ()) {
	if (cType.laserInfo.xCreationTime + I2X (1)/2 > gameData.time.xGame)
		return 1;
	}
if ((cType.laserInfo.parent.nType == OBJ_PLAYER) && !(pDebris->info.nFlags & OF_EXPLODING)) {
	audio.CreateSegmentSound (SOUND_ROBOT_HIT, info.nSegment, 0, vHitPt);
	pDebris->Explode (0);
	if (WI_damage_radius (info.nId))
		ExplodeSplashDamageWeapon (vHitPt, pDebris);
	MaybeKillWeapon (pDebris);
	Die ();
	}
return 1;
}

//##void CollideCameraAndCamera (CObject* camera1, CObject* camera2, CFixVector& vHitPt) {
//##	return;
//##}

//##void CollideCameraAndPowerup (CObject* camera, CObject* powerup, CFixVector& vHitPt) {
//##	return;
//##}

//##void CollideCameraAndDebris (CObject* camera, CObject* debris, CFixVector& vHitPt) {
//##	return;
//##}

//##void CollidePowerupAndPowerup (CObject* powerup1, CObject* powerup2, CFixVector& vHitPt) {
//##	return;
//##}

//##void CollidePowerupAndDebris (CObject* powerup, CObject* debris, CFixVector& vHitPt) {
//##	return;
//##}

//##void CollideDebrisAndDebris (CObject* debris1, CObject* debris2, CFixVector& vHitPt) {
//##	return;
//##}


/* DPH: Put these macros on one long line to avoid CR/LF problems on linux */
#define	COLLISION_OF(a, b) (((a)<<8) + (b))

#define	DO_COLLISION(type1, type2, collisionHandler) \
			case COLLISION_OF ((type1), (type2)): \
				return ((objA)->collisionHandler) ((objB), vHitPt, vNormal); \
			case COLLISION_OF ((type2), (type1)): \
				return ((objB)->collisionHandler) ((objA), vHitPt, vNormal);

#define	DO_SAME_COLLISION(type1, type2, collisionHandler) \
				case COLLISION_OF ((type1), (type1)): \
					return ((objA)->collisionHandler) ((objB), vHitPt);

//these next two macros define a case that does nothing
#define	NO_COLLISION(type1, type2, collisionHandler) \
				case COLLISION_OF ((type1), (type2)): \
					break; \
				case COLLISION_OF ((type2), (type1)): \
					break;

#define	NO_SAME_COLLISION(type1, type2, collisionHandler) \
				case COLLISION_OF ((type1), (type1)): \
					break;

//	-----------------------------------------------------------------------------

int32_t CollideTwoObjects (CObject* objA, CObject* objB, CFixVector& vHitPt, CFixVector* vNormal)
{
	int32_t collisionType = COLLISION_OF (objA->info.nType, objB->info.nType);

switch (collisionType) {
	NO_SAME_COLLISION (OBJ_FIREBALL, OBJ_FIREBALL,  CollideFireballAndFireball)
	DO_SAME_COLLISION (OBJ_ROBOT, 	OBJ_ROBOT, 		CollideRobotAndRobot)
	NO_SAME_COLLISION (OBJ_HOSTAGE, 	OBJ_HOSTAGE, 	CollideHostageAndHostage)
	DO_SAME_COLLISION (OBJ_PLAYER, 	OBJ_PLAYER, 	CollidePlayerAndPlayer)
	DO_SAME_COLLISION (OBJ_WEAPON, 	OBJ_WEAPON, 	CollideWeaponAndWeapon)
	NO_SAME_COLLISION (OBJ_CAMERA, 	OBJ_CAMERA, 	CollideCameraAndCamera)
	NO_SAME_COLLISION (OBJ_POWERUP, 	OBJ_POWERUP, 	collidePowerupAndPowerup)
	NO_SAME_COLLISION (OBJ_DEBRIS, 	OBJ_DEBRIS, 	collideDebrisAndDebris)

	NO_SAME_COLLISION (OBJ_MARKER, 	OBJ_MARKER, 	NULL)

	NO_COLLISION		(OBJ_FIREBALL, OBJ_ROBOT, 		CollideFireballAndRobot)
	NO_COLLISION		(OBJ_FIREBALL, OBJ_HOSTAGE, 	CollideFireballAndHostage)
	NO_COLLISION 		(OBJ_FIREBALL, OBJ_PLAYER, 	CollideFireballAndPlayer)
	NO_COLLISION 		(OBJ_FIREBALL, OBJ_WEAPON, 	CollideFireballAndWeapon)
	NO_COLLISION		(OBJ_FIREBALL, OBJ_CAMERA, 	CollideFireballAndCamera)
	NO_COLLISION		(OBJ_FIREBALL, OBJ_POWERUP, 	CollideFireballAndPowerup)
	NO_COLLISION		(OBJ_FIREBALL, OBJ_DEBRIS, 	CollideFireballAndDebris)

	NO_COLLISION		(OBJ_EXPLOSION, OBJ_ROBOT, 	CollideFireballAndRobot)
	NO_COLLISION		(OBJ_EXPLOSION, OBJ_HOSTAGE, 	CollideFireballAndHostage)
	NO_COLLISION 		(OBJ_EXPLOSION, OBJ_PLAYER, 	CollideFireballAndPlayer)
	NO_COLLISION 		(OBJ_EXPLOSION, OBJ_WEAPON, 	CollideFireballAndWeapon)
	NO_COLLISION		(OBJ_EXPLOSION, OBJ_CAMERA, 	CollideFireballAndCamera)
	NO_COLLISION		(OBJ_EXPLOSION, OBJ_POWERUP, 	CollideFireballAndPowerup)
	NO_COLLISION		(OBJ_EXPLOSION, OBJ_DEBRIS, 	CollideFireballAndDebris)

	NO_COLLISION		(OBJ_ROBOT, 	OBJ_HOSTAGE, 	CollideRobotAndHostage)
	DO_COLLISION		(OBJ_ROBOT, 	OBJ_PLAYER, 	CollideRobotAndPlayer)
	NO_COLLISION		(OBJ_ROBOT, 	OBJ_CAMERA, 	CollideRobotAndCamera)
	NO_COLLISION		(OBJ_ROBOT, 	OBJ_POWERUP, 	CollideRobotAndPowerup)
	DO_COLLISION		(OBJ_ROBOT, 	OBJ_DEBRIS, 	CollideActorAndClutter)
	DO_COLLISION		(OBJ_ROBOT, 	OBJ_REACTOR, 	CollideRobotAndReactor)

	NO_COLLISION		(OBJ_HOSTAGE, 	OBJ_WEAPON, 	CollideHostageAndWeapon)
	NO_COLLISION		(OBJ_HOSTAGE, 	OBJ_CAMERA, 	CollideHostageAndCamera)
	NO_COLLISION		(OBJ_HOSTAGE, 	OBJ_POWERUP, 	CollideHostageAndPowerup)
	NO_COLLISION		(OBJ_HOSTAGE, 	OBJ_DEBRIS, 	CollideHostageAndDebris)

	NO_COLLISION		(OBJ_PLAYER, 	OBJ_CAMERA, 	CollidePlayerAndCamera)
	DO_COLLISION		(OBJ_PLAYER, 	OBJ_POWERUP, 	CollidePlayerAndPowerup)
	DO_COLLISION		(OBJ_PLAYER, 	OBJ_DEBRIS, 	CollideActorAndClutter)
	DO_COLLISION		(OBJ_PLAYER, 	OBJ_REACTOR, 	CollidePlayerAndReactor)
	DO_COLLISION		(OBJ_PLAYER, 	OBJ_CLUTTER, 	CollideActorAndClutter)
	DO_COLLISION		(OBJ_PLAYER, 	OBJ_MONSTERBALL, 	CollidePlayerAndMonsterball)
	DO_COLLISION		(OBJ_PLAYER, 	OBJ_HOSTAGE,	CollidePlayerAndHostage)
	DO_COLLISION		(OBJ_PLAYER, 	OBJ_MARKER,		CollidePlayerAndMarker)

	DO_COLLISION		(OBJ_WEAPON,	OBJ_ROBOT, 		CollideWeaponAndRobot)
	DO_COLLISION		(OBJ_WEAPON, 	OBJ_PLAYER,		CollideWeaponAndPlayer)
	NO_COLLISION		(OBJ_WEAPON, 	OBJ_CAMERA, 	CollideWeaponAndCamera)
	NO_COLLISION		(OBJ_WEAPON, 	OBJ_POWERUP, 	CollideWeaponAndPowerup)
	DO_COLLISION		(OBJ_WEAPON, 	OBJ_DEBRIS, 	CollideWeaponAndDebris)
	DO_COLLISION		(OBJ_WEAPON, 	OBJ_REACTOR, 	CollideWeaponAndReactor)
	DO_COLLISION		(OBJ_WEAPON, 	OBJ_CLUTTER, 	CollideWeaponAndClutter)
	DO_COLLISION		(OBJ_WEAPON, 	OBJ_MONSTERBALL, 	CollideWeaponAndMonsterball)

	NO_COLLISION		(OBJ_CAMERA, 	OBJ_POWERUP, 	CollideCameraAndPowerup)
	NO_COLLISION		(OBJ_CAMERA, 	OBJ_DEBRIS, 	CollideCameraAndDebris)

	NO_COLLISION		(OBJ_POWERUP, 	OBJ_DEBRIS, 	CollidePowerupAndDebris)

	NO_COLLISION		(OBJ_MARKER, 	OBJ_ROBOT, 		NULL)
	NO_COLLISION		(OBJ_MARKER, 	OBJ_HOSTAGE, 	NULL)
	NO_COLLISION		(OBJ_MARKER, 	OBJ_WEAPON, 	NULL)
	NO_COLLISION		(OBJ_MARKER, 	OBJ_CAMERA, 	NULL)
	NO_COLLISION		(OBJ_MARKER, 	OBJ_POWERUP, 	NULL)
	NO_COLLISION		(OBJ_MARKER, 	OBJ_DEBRIS, 	NULL)

	DO_COLLISION		(OBJ_WEAPON, 	OBJ_CAMBOT, 	CollideWeaponAndRobot)
	DO_COLLISION		(OBJ_CAMBOT, 	OBJ_PLAYER, 	CollideRobotAndPlayer)
	NO_COLLISION		(OBJ_FIREBALL, OBJ_CAMBOT, 	CollideFireballAndRobot)
	default:
		Int3 ();	//Error ("Unhandled collisionType in Collide.c! \n");
	}
return 1;
}

//	-----------------------------------------------------------------------------

void CollideInit (void)
{
	int32_t i, j;

for (i = 0; i < MAX_OBJECT_TYPES; i++)
	for (j = 0; j < MAX_OBJECT_TYPES; j++)
		gameData.objData.collisionResult [i][j] = RESULT_NOTHING;

ENABLE_COLLISION (OBJ_WALL, OBJ_ROBOT);
ENABLE_COLLISION (OBJ_WALL, OBJ_WEAPON);
ENABLE_COLLISION (OBJ_WALL, OBJ_PLAYER);
ENABLE_COLLISION (OBJ_WALL, OBJ_MONSTERBALL);
DISABLE_COLLISION (OBJ_FIREBALL, OBJ_FIREBALL);

ENABLE_COLLISION (OBJ_ROBOT, OBJ_ROBOT);
//	DISABLE_COLLISION (OBJ_ROBOT, OBJ_ROBOT);	//	ALERT: WARNING: HACK: MK = RESPONSIBLE!TESTING!!

DISABLE_COLLISION (OBJ_HOSTAGE, OBJ_HOSTAGE);
ENABLE_COLLISION  (OBJ_PLAYER, OBJ_PLAYER);
ENABLE_COLLISION  (OBJ_PLAYER, OBJ_WEAPON);
DISABLE_COLLISION (OBJ_PLAYER, OBJ_CAMERA);
ENABLE_COLLISION  (OBJ_PLAYER, OBJ_POWERUP);
DISABLE_COLLISION (OBJ_PLAYER, OBJ_DEBRIS);
ENABLE_COLLISION  (OBJ_PLAYER, OBJ_REACTOR)
ENABLE_COLLISION  (OBJ_PLAYER, OBJ_CLUTTER)
ENABLE_COLLISION  (OBJ_PLAYER, OBJ_MARKER);
ENABLE_COLLISION  (OBJ_PLAYER, OBJ_CAMBOT);
DISABLE_COLLISION (OBJ_PLAYER, OBJ_EFFECT);
ENABLE_COLLISION  (OBJ_PLAYER, OBJ_MONSTERBALL);
DISABLE_COLLISION (OBJ_FIREBALL, OBJ_ROBOT);
DISABLE_COLLISION (OBJ_FIREBALL, OBJ_HOSTAGE);
DISABLE_COLLISION (OBJ_FIREBALL, OBJ_PLAYER);
DISABLE_COLLISION (OBJ_FIREBALL, OBJ_WEAPON);
DISABLE_COLLISION (OBJ_FIREBALL, OBJ_CAMERA);
DISABLE_COLLISION (OBJ_FIREBALL, OBJ_POWERUP);
DISABLE_COLLISION (OBJ_FIREBALL, OBJ_DEBRIS);
ENABLE_COLLISION  (OBJ_FIREBALL, OBJ_CAMBOT);
DISABLE_COLLISION  (OBJ_FIREBALL, OBJ_EFFECT);
DISABLE_COLLISION (OBJ_ROBOT, OBJ_HOSTAGE);
ENABLE_COLLISION  (OBJ_ROBOT, OBJ_PLAYER);
ENABLE_COLLISION  (OBJ_ROBOT, OBJ_WEAPON);
DISABLE_COLLISION (OBJ_ROBOT, OBJ_CAMERA);
DISABLE_COLLISION (OBJ_ROBOT, OBJ_POWERUP);
DISABLE_COLLISION (OBJ_ROBOT, OBJ_DEBRIS);
ENABLE_COLLISION  (OBJ_ROBOT, OBJ_REACTOR)
ENABLE_COLLISION  (OBJ_HOSTAGE, OBJ_PLAYER);
ENABLE_COLLISION  (OBJ_HOSTAGE, OBJ_WEAPON);
DISABLE_COLLISION (OBJ_HOSTAGE, OBJ_CAMERA);
DISABLE_COLLISION (OBJ_HOSTAGE, OBJ_POWERUP);
DISABLE_COLLISION (OBJ_HOSTAGE, OBJ_DEBRIS);
DISABLE_COLLISION (OBJ_WEAPON, OBJ_CAMERA);
DISABLE_COLLISION (OBJ_WEAPON, OBJ_POWERUP);
ENABLE_COLLISION  (OBJ_WEAPON, OBJ_DEBRIS);
ENABLE_COLLISION  (OBJ_WEAPON, OBJ_WEAPON);
ENABLE_COLLISION  (OBJ_WEAPON, OBJ_REACTOR)
ENABLE_COLLISION  (OBJ_WEAPON, OBJ_CLUTTER)
ENABLE_COLLISION  (OBJ_WEAPON, OBJ_CAMBOT);
DISABLE_COLLISION  (OBJ_WEAPON, OBJ_EFFECT);
ENABLE_COLLISION  (OBJ_WEAPON, OBJ_MONSTERBALL);
DISABLE_COLLISION (OBJ_CAMERA, OBJ_CAMERA);
DISABLE_COLLISION (OBJ_CAMERA, OBJ_POWERUP);
DISABLE_COLLISION (OBJ_CAMERA, OBJ_DEBRIS);
DISABLE_COLLISION (OBJ_POWERUP, OBJ_POWERUP);
DISABLE_COLLISION (OBJ_POWERUP, OBJ_DEBRIS);
ENABLE_COLLISION  (OBJ_POWERUP, OBJ_WALL);
DISABLE_COLLISION (OBJ_DEBRIS, OBJ_DEBRIS);
ENABLE_COLLISION  (OBJ_ROBOT, OBJ_CAMBOT);
DISABLE_COLLISION  (OBJ_ROBOT, OBJ_EFFECT);
}

//	-----------------------------------------------------------------------------

int32_t CObject::CollideObjectAndWall (fix xHitSpeed, int16_t nHitSeg, int16_t nHitWall, CFixVector& vHitPt)
{
switch (info.nType) {
	case OBJ_NONE:
		Error ("An object of type NONE hit a wall! \n");
		break;
	case OBJ_PLAYER:
		CollidePlayerAndWall (xHitSpeed, nHitSeg, nHitWall, vHitPt);
		break;
	case OBJ_WEAPON:
		CollideWeaponAndWall (xHitSpeed, nHitSeg, nHitWall, vHitPt);
		break;
	case OBJ_DEBRIS:
		CollideDebrisAndWall (xHitSpeed, nHitSeg, nHitWall, vHitPt);
		break;
	case OBJ_FIREBALL:
		break;	//CollideFireballAndWall (xHitSpeed, nHitSeg, nHitWall, vHitPt);
	case OBJ_ROBOT:
		CollideRobotAndWall (xHitSpeed, nHitSeg, nHitWall, vHitPt);
		break;
	case OBJ_HOSTAGE:
		break;	//CollideHostageAndWall (xHitSpeed, nHitSeg, nHitWall, vHitPt);
	case OBJ_CAMERA:
		break;	//CollideCameraAndWall (xHitSpeed, nHitSeg, nHitWall, vHitPt);
	case OBJ_EFFECT:
		break;	//CollideSmokeAndWall (xHitSpeed, nHitSeg, nHitWall, vHitPt);
	case OBJ_POWERUP:
		break;	//CollidePowerupAndWall (xHitSpeed, nHitSeg, nHitWall, vHitPt);
	case OBJ_GHOST:
		break;	//do nothing
	case OBJ_MONSTERBALL:
		break;	//CollidePowerupAndWall (pObj, xHitSpeed, nHitSeg, nHitWall, vHitPt);
	default:
		Error ("Unhandled CObject nType hit CWall in Collide.c \n");
	}
return 1;
}

//	-----------------------------------------------------------------------------

void SetDebrisCollisions (void)
{
	int32_t	h = gameOpts->render.nDebrisLife ? RESULT_CHECK : RESULT_NOTHING;

SET_COLLISION (OBJ_PLAYER, OBJ_DEBRIS, h);
SET_COLLISION (OBJ_ROBOT, OBJ_DEBRIS, h);
}

//	-----------------------------------------------------------------------------
// eof
