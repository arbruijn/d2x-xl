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

//##void CollideFireballAndWall (CObject* fireball, fix xHitSpeed, short nHitSeg, short nHitWall, CFixVector& vHitPt) {
//##	return;
//##}

//	-----------------------------------------------------------------------------
//	The only reason this routine is called (as of 10/12/94) is so Brain guys can open doors.
void CObject::CollideRobotAndWall (fix xHitSpeed, short nHitSeg, short nHitSide, CFixVector& vHitPt)
{
	tAILocalInfo	*ailP = gameData.ai.localInfo + OBJ_IDX (this);
	tRobotInfo		*botInfoP = &ROBOTINFO (info.nId);

if ((info.nId != ROBOT_BRAIN) &&
	 (cType.aiInfo.behavior != AIB_RUN_FROM) &&
	 !botInfoP->companion &&
	 (cType.aiInfo.behavior != AIB_SNIPE))
	return;

CWall *wallP = SEGMENTS [nHitSeg].Wall (nHitSide);
if (!wallP || (wallP->nType != WALL_DOOR))
	return;

if ((wallP->keys == KEY_NONE) && (wallP->state == WALL_DOOR_CLOSED) && !(wallP->flags & WALL_DOOR_LOCKED))
	SEGMENTS [nHitSeg].OpenDoor (nHitSide);
else if (botInfoP->companion) {
	if ((ailP->mode != AIM_GOTO_PLAYER) && (gameData.escort.nSpecialGoal != ESCORT_GOAL_SCRAM))
		return;
	if (!(wallP->flags & WALL_DOOR_LOCKED) || ((wallP->keys != KEY_NONE) && (wallP->keys & LOCALPLAYER.flags)))
		SEGMENTS [nHitSeg].OpenDoor (nHitSide);
	}
else if (botInfoP->thief) {		//	Thief allowed to go through doors to which player has key.
	if ((wallP->keys != KEY_NONE) && (wallP->keys & LOCALPLAYER.flags))
		SEGMENTS [nHitSeg].OpenDoor (nHitSide);
	}
}

//##void CollideHostageAndWall (CObject* hostage, fix xHitSpeed, short nHitSeg, short nHitSide,   CFixVector& vHitPt) {
//##	return;
//##}

//	-----------------------------------------------------------------------------

int CObject::ApplyDamageToClutter (fix xDamage)
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
void CObject::ApplyForceDamage (fix vForce, CObject* otherObjP)
{
	int	result;
	fix	xDamage;

if (info.nFlags & (OF_EXPLODING | OF_SHOULD_BE_DEAD))
	return;		//already exploding or dead
xDamage = FixDiv (vForce, mType.physInfo.mass) / 8;
if ((otherObjP->info.nType == OBJ_PLAYER) && gameStates.app.cheats.bMonsterMode)
	xDamage = 0x7fffffff;
	switch (info.nType) {
		case OBJ_ROBOT:
			if (ROBOTINFO (info.nId).attackType == 1) {
				if (otherObjP->info.nType == OBJ_WEAPON)
					result = ApplyDamageToRobot (xDamage / 4, otherObjP->cType.laserInfo.parent.nObject);
				else
					result = ApplyDamageToRobot (xDamage / 4, OBJ_IDX (otherObjP));
				}
			else {
				if (otherObjP->info.nType == OBJ_WEAPON)
					result = ApplyDamageToRobot (xDamage / 2, otherObjP->cType.laserInfo.parent.nObject);
				else
					result = ApplyDamageToRobot (xDamage / 2, OBJ_IDX (otherObjP));
				}
#if DBG
			if (result && (otherObjP->cType.laserInfo.parent.nSignature == gameData.objs.consoleP->info.nSignature))
#else
			if (!(gameStates.app.bGameSuspended & SUSP_ROBOTS) && result && (otherObjP->cType.laserInfo.parent.nSignature == gameData.objs.consoleP->info.nSignature))
#endif
				cockpit->AddPointsToScore (ROBOTINFO (info.nId).scoreValue);
			break;

		case OBJ_PLAYER:
			//	If colliding with a claw nType robotP, do xDamage proportional to gameData.time.xFrame because you can Collide with those
			//	bots every frame since they don't move.
			if ((otherObjP->info.nType == OBJ_ROBOT) && (ROBOTINFO (otherObjP->info.nId).attackType))
				xDamage = FixMul (xDamage, gameData.time.xFrame*2);
			//	Make trainee easier.
			if (gameStates.app.nDifficultyLevel == 0)
				xDamage /= 2;
			ApplyDamageToPlayer (otherObjP, xDamage);
			break;

		case OBJ_CLUTTER:
			ApplyDamageToClutter (xDamage);
			break;

		case OBJ_REACTOR:
			ApplyDamageToReactor (xDamage, (short) OBJ_IDX (otherObjP));
			break;

		case OBJ_WEAPON:
			break;		//weapons don't take xDamage

		default:
			Int3 ();
		}
}

//	-----------------------------------------------------------------------------

void CObject::Bump (CObject* otherObjP, CFixVector vForce, int bDamage)
{
#if DBG
if (vForce.Mag () > I2X (1) * 1000)
	return;
#endif
if (!(mType.physInfo.flags & PF_PERSISTENT)) {
	if (info.nType == OBJ_PLAYER) {
		if (otherObjP->info.nType == OBJ_MONSTERBALL) {
			float mq;

			mq = float (otherObjP->mType.physInfo.mass) / (float (mType.physInfo.mass) * float (nMonsterballPyroForce));
			vForce *= mq;
			ApplyForce (vForce);
			}
		else {
			vForce *= 0.25f;
			ApplyForce (vForce);
			if (bDamage && ((otherObjP->info.nType != OBJ_ROBOT) || !ROBOTINFO (otherObjP->info.nId).companion)) 
				ApplyForceDamage (vForce.Mag (), otherObjP);
			}
		}
	else {
		float h = 1.0f / float (4 + gameStates.app.nDifficultyLevel);
		if (info.nType == OBJ_MONSTERBALL) {
			float mq;

			if (otherObjP->info.nType == OBJ_PLAYER) {
				gameData.hoard.nLastHitter = OBJ_IDX (otherObjP);
				mq = float (otherObjP->mType.physInfo.mass) / float (mType.physInfo.mass) * float (nMonsterballPyroForce) / 10.0f;
				}
			else {
				gameData.hoard.nLastHitter = otherObjP->cType.laserInfo.parent.nObject;
				mq = float (I2X (nMonsterballForces [otherObjP->info.nId]) / 100) / float (mType.physInfo.mass);
				}
			vForce *= mq;
			if (gameData.hoard.nLastHitter == LOCALPLAYER.nObject)
				MultiSendMonsterball (1, 0);
			}
		else if (info.nType == OBJ_ROBOT) {
			if (ROBOTINFO (info.nId).bossFlag)
				return;
			}
		else if ((info.nType != OBJ_CLUTTER) && (info.nType != OBJ_DEBRIS) && (info.nType != OBJ_REACTOR))
			return;
		ApplyForce (vForce);
		vForce *= h;
		ApplyRotForce (vForce);
		if (bDamage)
			ApplyForceDamage (vForce.Mag (), otherObjP);
		}
	}
}

//	-----------------------------------------------------------------------------

void CObject::Bump (CObject* otherObjP, CFixVector vForce, CFixVector vRotForce, int bDamage)
{
if (mType.physInfo.flags & PF_PERSISTENT)
	return;
if (IsStatic ())
	return;
if (info.nType == OBJ_PLAYER) {
	if ((this == gameData.objs.consoleP) && gameData.objs.speedBoost [OBJ_IDX (this)].bBoosted)
		return;
	vRotForce *= (I2X (1) / 4);
	}
else if (info.nType == OBJ_MONSTERBALL)
	gameData.hoard.nLastHitter = (otherObjP->info.nType == OBJ_PLAYER) ? OBJ_IDX (otherObjP) : otherObjP->cType.laserInfo.parent.nObject;
mType.physInfo.velocity = vForce;
ApplyRotForce (vRotForce);
//TurnTowardsVector (vRotForce, I2X (1));
}

//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------

class CBumpForces {
	public:
		CObject*	m_objP;
		CFixVector	m_vPos, m_vVel, m_vForce, m_vRotForce;
		fix m_mass;

	explicit CBumpForces (CObject* objP = NULL) { 
		if ((m_BRP)) {
			m_vVel = objP->mType.physInfo.velocity;
			m_vPos = objP->info.position.vPos;
			m_mass = objP->mType.physInfo.mass;
			}
		}
	inline void Compute (CFixVector& vDist, CFixVector* vNormal, CObject* objP);
	inline void Bump (CBumpForces& fOther, fix massSum, fix massDiff, int bDamage);
};

//	-----------------------------------------------------------------------------

inline void CBumpForces::Compute (CFixVector& vDist, CFixVector* vNormal, CObject* objP)
{
if (m_vVel.IsZero ())
	m_vForce.SetZero (), m_vRotForce.SetZero ();
else {
	CFixVector vDistNorm, vVelNorm;

	if (vNormal && objP->IsStatic ())
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

inline void CBumpForces::Bump (CBumpForces& fOther, fix massSum, fix massDiff, int bDamage)
{
CFixVector vRes = (m_vForce * massDiff + fOther.m_vForce * (2 * fOther.m_mass)) / massSum;
// don't divide by the total mass here or ApplyRotForce() will scale down the forces too much
CFixVector vRot = (m_vRotForce * massDiff + fOther.m_vRotForce * (2 * fOther.m_mass)) /*/ massSum*/;
if (m_objP->info.nType == OBJ_PLAYER)
	vRes *= (I2X (1) / 4);
m_objP->Bump (fOther.m_objP, m_vVel + vRes, vRot, bDamage);
}

//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------
//	-----------------------------------------------------------------------------
//deal with two OBJECTS bumping into each other.  Apply vForce from collision
//to each robotP.  The flags tells whether the objects should take damage from
//the collision.

int BumpTwoObjects (CObject* thisP, CObject* otherP, int bDamage, CFixVector& vHitPt, CFixVector* vNormal = NULL)
{
	CObject* t;

if ((thisP->info.movementType != MT_PHYSICS) && !thisP->IsStatic ())
	t = otherP;
else if ((otherP->info.movementType != MT_PHYSICS) && !otherP->IsStatic ())
	t = thisP;
else
	t = NULL;
if (t) {
	Assert (t->info.movementType == MT_PHYSICS);
	CFixVector vForce = t->mType.physInfo.velocity * (-2 * t->mType.physInfo.mass);
	if (!vForce.IsZero ())
		t->ApplyForce (vForce);
	return 1;
	}

CBumpForces f0 (thisP), f1 (otherP);
CFixVector vDist = f1.m_vPos - f0.m_vPos;

#if 0
if (CFixVector::Dot (f0.m_vVel, f1.m_vVel) <= 0)
#else
if ((CFixVector::Dot (f0.m_vVel, vDist) < 0) && (CFixVector::Dot (f1.m_vVel, vDist) > 0))
#endif
	return 0;	//objects separating already

if (!CollisionModel () &&
	 ((thisP->info.nType == OBJ_PLAYER) || (thisP->info.nType == OBJ_ROBOT) || (thisP->info.nType == OBJ_REACTOR)) &&
	 ((otherP->info.nType == OBJ_PLAYER) || (otherP->info.nType == OBJ_ROBOT) || (otherP->info.nType == OBJ_REACTOR))) {
	fix dist = vDist.Mag ();
	fix intrusion = (thisP->info.xSize + otherP->info.xSize) - dist;
	if (intrusion > 0) {
#if DBG
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
		OBJPOS (thisP)->vPos = f0.m_vPos;
		thisP->RelinkToSeg (FindSegByPos (f0.m_vPos, thisP->info.nSegment, 0, 0));
		OBJPOS (otherP)->vPos = f1.m_vPos;
		otherP->RelinkToSeg (FindSegByPos (f1.m_vPos, otherP->info.nSegment, 0, 0));
		}
	}

// check if objects are penetrating and move apart
if ((EGI_FLAG (bUseHitAngles, 0, 0, 0) || (otherP->info.nType == OBJ_MONSTERBALL)) || thisP->IsStatic ()) { //&& !thisP->IsStatic ()) {
	f0.m_mass = thisP->mType.physInfo.mass;
	f1.m_mass = otherP->mType.physInfo.mass;

	if (otherP->info.nType == OBJ_MONSTERBALL) {
		if (thisP->info.nType == OBJ_WEAPON)
			f0.m_mass = I2X (nMonsterballForces [thisP->info.nId]) / 100;
		else if (thisP->info.nType == OBJ_PLAYER)
			f0.m_mass *= nMonsterballPyroForce;
		}

#if 1
	f0.Compute (vDist, vNormal, otherP);
	vDist.Neg ();
	f1.Compute (vDist, vNormal, thisP);
#else
	CFixVector	vDistNorm, vVelNorm, f0.m_vForce, f1.m_vForce, f0.m_vRotForce, f1.m_vRotForce;

	if (f0.m_vVel.IsZero ())
		f0.m_vForce.SetZero (), f0.m_vRotForce.SetZero ();
	else {
		if (vNormal && otherP->IsStatic ())
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
		if (vNormal && thisP->IsStatic ())
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
	if (thisP->info.nType == OBJ_PLAYER)
		vRes0 *= (I2X (1) / 4);
	else if (otherP->info.nType == OBJ_PLAYER)
		vRes1 *= (I2X (1) / 4);
	thisP->Bump (otherP, f0.m_vVel + vRes0, vRot0, bDamage);
	otherP->Bump (thisP, f1.m_vVel + vRes1, vRot1, bDamage);
#endif
	}
else {
	CFixVector vForce = f0.m_vVel - f1.m_vVel;
#if 0
	fix f0.m_mass = thisP->mType.physInfo.mass;
	fix f1.m_mass = otherP->mType.physInfo.mass;
	fix massProd = FixMul (f0.m_mass, f1.m_mass) * 2;
	fix massTotal = f0.m_mass + f1.m_mass;
	fix impulse = FixDiv (massProd, massTotal);
	if (impulse == 0)
		return 0;
#else
	float mass0 = X2F (thisP->mType.physInfo.mass);
	float mass1 = X2F (otherP->mType.physInfo.mass);
	float impulse = 2.0f * (mass0 * mass1) / (mass0 + mass1);
	if (impulse == 0.0f)
		return 0;
#endif
	vForce *= impulse;
	otherP->Bump (thisP, vForce, bDamage);
	vForce.Neg ();
	thisP->Bump (otherP, vForce, bDamage);
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

void CObject::CollidePlayerAndWall (fix xHitSpeed, short nHitSeg, short nHitSide, CFixVector& vHitPt)
{
	fix damage;
	char bForceFieldHit = 0;
	int nBaseTex, nOvlTex;

if (info.nId != N_LOCALPLAYER) // Execute only for local player
	return;
nBaseTex = SEGMENTS [nHitSeg].m_sides [nHitSide].m_nBaseTex;
//	If this CWall does damage, don't make *BONK* sound, we'll be making another sound.
if (gameData.pig.tex.tMapInfoP [nBaseTex].damage > 0)
	return;
if (gameData.pig.tex.tMapInfoP [nBaseTex].flags & TMI_FORCE_FIELD) {
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
   SEGMENTS [nHitSeg].ProcessWallHit (nHitSide, 20, info.nId, this);
	}
if (gameStates.app.bD2XLevel && (SEGMENTS [nHitSeg].HasNoDamageProp ()))
	return;
//	** Damage from hitting CWall **
//	If the player has less than 10% shield, don't take damage from bump
// Note: Does quad damage if hit a vForce field - JL
damage = (xHitSpeed / DAMAGE_SCALE) * (bForceFieldHit * 8 + 1);
nOvlTex = SEGMENTS [nHitSeg].m_sides [nHitSide].m_nOvlTex;
//don't do CWall damage and sound if hit lava or water
if ((gameData.pig.tex.tMapInfoP [nBaseTex].flags & (TMI_WATER|TMI_VOLATILE)) ||
		(nOvlTex && (gameData.pig.tex.tMapInfoP [nOvlTex].flags & (TMI_WATER|TMI_VOLATILE))))
	damage = 0;
if (damage >= DAMAGE_THRESHOLD) {
	int volume = (xHitSpeed - (DAMAGE_SCALE * DAMAGE_THRESHOLD)) / WALL_LOUDNESS_SCALE;
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
int CObject::ApplyWallPhysics (short nSegment, short nSide)
{
	fix	xDamage = 0;
	int	nType;

if (!((nType = SEGMENTS [nSegment].Physics (nSide, xDamage)) || 
		(nType = SEGMENTS [nSegment].Physics (xDamage))))
	return 0;
if (SEGMENTS [nSegment].HasNoDamageProp ())
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

int CObject::CheckSegmentPhysics (void)
{
	fix xDamage;
	int nType;

//	Assert (info.nType==OBJ_PLAYER);
if (!EGI_FLAG (bFluidPhysics, 1, 0, 0))
	return 0;
if (!(nType = SEGMENTS [info.nSegment].Physics (xDamage)))
	return 0;
if (xDamage > 0) {
	xDamage = FixMul (xDamage, gameData.time.xFrame) / 2;
	if (gameStates.app.nDifficultyLevel == 0)
		xDamage /= 2;
	if (info.nType == OBJ_PLAYER) {
		if (!(gameData.multiplayer.players [info.nId].flags & PLAYER_FLAGS_INVULNERABLE))
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
void CObject::ScrapeOnWall (short nHitSeg, short nHitSide, CFixVector& vHitPt)
{
if (info.nType == OBJ_PLAYER) {
	if (info.nId == N_LOCALPLAYER) {
		int nType = ApplyWallPhysics (nHitSeg, nHitSide);
		if (nType != 0) {
			CFixVector	vHit, vRand;

			if ((gameData.time.xGame > xLastVolatileScrapeSoundTime + I2X (1)/4) ||
					(gameData.time.xGame < xLastVolatileScrapeSoundTime)) {
				short sound = (nType & 1) ? SOUND_VOLATILE_WALL_HISS : SOUND_SHIP_IN_WATER;
				xLastVolatileScrapeSoundTime = gameData.time.xGame;
				audio.CreateSegmentSound (sound, nHitSeg, 0, vHitPt);
				if (IsMultiGame)
					MultiSendPlaySound (sound, I2X (1));
				}
			vHit = SEGMENTS [nHitSeg].m_sides [nHitSide].m_normals [0];
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
int OkToDoOmegaDamage (CObject* weaponP)
{
if (!IsMultiGame)
	return 1;
int nParentSig = weaponP->cType.laserInfo.parent.nSignature;
int nParentObj = weaponP->cType.laserInfo.parent.nObject;
if (OBJECTS [nParentObj].info.nSignature != nParentSig)
	return 1;
fix dist = CFixVector::Dist (OBJECTS [nParentObj].info.position.vPos, weaponP->info.position.vPos);
if (dist > MAX_OMEGA_DIST)
	return 0;
return 1;
}

//	-----------------------------------------------------------------------------

int CObject::CreateWeaponEffects (int bExplBlast)
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

// int Show_segAnd_side = 0;

int CObject::CollideWeaponAndWall (fix xHitSpeed, short nHitSeg, short nHitSide, CFixVector& vHitPt)
{
	CSegment*		segP = SEGMENTS + nHitSeg;
	CSide*			sideP = (nHitSide < 0) ? NULL : segP->m_sides + nHitSide;
	CWeaponInfo*	weaponInfoP = gameData.weapons.info + info.nId;
	CObject*			parentObjP = OBJECTS + cType.laserInfo.parent.nObject;

	int	nPlayer;
	fix	nStrength = WI_strength (info.nId, gameStates.app.nDifficultyLevel);

if (info.nId == OMEGA_ID)
	if (!OkToDoOmegaDamage (this))
		return 1;

//	If this is a guided missile and it strikes fairly directly, clear bounce flag.
if (info.nId == GUIDEDMSL_ID) {
	fix dot = (nHitSide < 0) ? -1 : CFixVector::Dot (info.position.mOrient.m.dir.f, sideP->m_normals [0]);
#if TRACE
	console.printf (CON_DBG, "Guided missile dot = %7.3f \n", X2F (dot));
#endif
	if (dot < -I2X (1)/6) {
#if TRACE
		console.printf (CON_DBG, "Guided missile loses bounciness. \n");
#endif
		mType.physInfo.flags &= ~PF_BOUNCE;
		}
	else {
		CFixVector vReflect = CFixVector::Reflect (info.position.mOrient.m.dir.f, sideP->m_normals[0]);
		CAngleVector va = vReflect.ToAnglesVec ();
		info.position.mOrient = CFixMatrix::Create (va);
		}
	}

int bBounce = (mType.physInfo.flags & PF_BOUNCE) != 0;
if (!bBounce)
	CreateWeaponEffects (1);
//if an energy this hits a forcefield, let it bounce
if (sideP && (gameData.pig.tex.tMapInfoP [sideP->m_nBaseTex].flags & TMI_FORCE_FIELD) &&
	 ((info.nType != OBJ_WEAPON) || weaponInfoP->xEnergyUsage)) {

	//make sound
	audio.CreateSegmentSound (SOUND_FORCEFIELD_BOUNCE_WEAPON, nHitSeg, 0, vHitPt);
	if (IsMultiGame)
		MultiSendPlaySound (SOUND_FORCEFIELD_BOUNCE_WEAPON, I2X (1));
	return 1;	//bail here. physics code will bounce this CObject
	}

#if DBG
if (gameStates.input.keys.pressed [KEY_LAPOSTRO])
	if (cType.laserInfo.parent.nObject == LOCALPLAYER.nObject) {
		//	MK: Real pain when you need to know a segP:CSide and you've got quad lasers.
#if TRACE
		console.printf (CON_DBG, "Your laser hit at CSegment = %i, CSide = %i \n", nHitSeg, nHitSide);
#endif
		//HUDInitMessage ("Hit at segment = %i, side = %i", nHitSeg, nHitSide);
		if (info.nId < 4)
			SubtractLight (nHitSeg, nHitSide);
		else if (info.nId == FLARE_ID)
			AddLight (nHitSeg, nHitSide);
		}
if (mType.physInfo.velocity.IsZero ()) {
	Int3 ();	//	Contact Matt: This is impossible.  A this with 0 velocity hit a CWall, which doesn't move.
	return 1;
	}
#endif
int bBlewUp = (nHitSide < 0) ? 0 : segP->CheckEffectBlowup (nHitSide, vHitPt, this, 0);
int bEscort = parentObjP->IsGuideBot ();
if (bEscort) {
	if (IsMultiGame) {
		Int3 ();  // Get Jason!
	   return 1;
	   }
	nPlayer = N_LOCALPLAYER;		//if single player, he's the players's buddy
	parentObjP = OBJECTS + LOCALPLAYER.nObject;
	}
else {
	nPlayer = parentObjP->IsPlayer () ? parentObjP->info.nId : -1;
	}
if (bBlewUp) {		//could be a wall switch - only player or guidebot can activate it
	segP->OperateTrigger (nHitSide, parentObjP, 1);
	}
if (info.nId == EARTHSHAKER_ID)
	ShakerRockStuff (&Position ());
int wallType = (nHitSide < 0) ? WHP_NOT_SPECIAL : segP->ProcessWallHit (nHitSide, info.xShield, nPlayer, this);
// Wall is volatile if either tmap 1 or 2 is volatile
if (sideP && ((gameData.pig.tex.tMapInfoP [sideP->m_nBaseTex].flags & TMI_VOLATILE) ||
	           (sideP->m_nOvlTex && (gameData.pig.tex.tMapInfoP [sideP->m_nOvlTex].flags & TMI_VOLATILE)))) {
	ubyte tVideoClip;
	//we've hit a volatile CWall
	audio.CreateSegmentSound (SOUND_VOLATILE_WALL_HIT, nHitSeg, 0, vHitPt);
	//for most weapons, use volatile CWall hit.  For mega, use its special tVideoClip
	tVideoClip = (info.nId == MEGAMSL_ID) ? weaponInfoP->nRobotHitVClip : VCLIP_VOLATILE_WALL_HIT;
	//	New by MK: If powerful splash damager, explode with splash damage, not due to lava, fixes megas being wimpy in lava.
	if (weaponInfoP->xDamageRadius >= VOLATILE_WALL_DAMAGE_RADIUS / 2)
		ExplodeSplashDamageWeapon (vHitPt);
	else
		CreateSplashDamageExplosion (this, nHitSeg, vHitPt, weaponInfoP->xImpactSize + VOLATILE_WALL_IMPACT_SIZE, tVideoClip,
											  nStrength / 4 + VOLATILE_WALL_EXPL_STRENGTH, weaponInfoP->xDamageRadius+VOLATILE_WALL_DAMAGE_RADIUS,
											  nStrength / 2 + VOLATILE_WALL_DAMAGE_FORCE, cType.laserInfo.parent.nObject);
	Die ();		//make flares die in lava
	}
else if (sideP && ((gameData.pig.tex.tMapInfoP [sideP->m_nBaseTex].flags & TMI_WATER) ||
			          (sideP->m_nOvlTex && (gameData.pig.tex.tMapInfoP [sideP->m_nOvlTex].flags & TMI_WATER)))) {
	//we've hit water
	//	MK: 09/13/95: SplashDamage in water is 1/2 Normal intensity.
	if (weaponInfoP->matter) {
		audio.CreateSegmentSound (SOUNDMSL_HIT_WATER, nHitSeg, 0, vHitPt);
		if (weaponInfoP->xDamageRadius) {
			audio.CreateObjectSound (IsSplashDamageWeapon () ? SOUND_BADASS_EXPLOSION_WEAPON : SOUND_STANDARD_EXPLOSION, SOUNDCLASS_EXPLOSION, OBJ_IDX (this));
			//	MK: 09/13/95: SplashDamage in water is 1/2 Normal intensity.
			CreateSplashDamageExplosion (this, nHitSeg, vHitPt, weaponInfoP->xImpactSize/2, weaponInfoP->nRobotHitVClip,
												  nStrength / 4, weaponInfoP->xDamageRadius, nStrength / 2, cType.laserInfo.parent.nObject);
			}
		else
			CreateExplosion (info.nSegment, info.position.vPos, weaponInfoP->xImpactSize, weaponInfoP->nWallHitVClip);
		}
	else {
		audio.CreateSegmentSound (SOUND_LASER_HIT_WATER, nHitSeg, 0, vHitPt);
		CreateExplosion (info.nSegment, info.position.vPos, weaponInfoP->xImpactSize, VCLIP_WATER_HIT);
		}
	Die ();		//make flares die in water
	}
else {
	if (!bBounce) {
		//if it's not the player's this, or it is the player's and there
		//is no CWall, and no blowing up monitor, then play sound
		if ((cType.laserInfo.parent.nType != OBJ_PLAYER) ||
			 (((wallType == WHP_NOT_SPECIAL)) && !bBlewUp))
			if ((weaponInfoP->nWallHitSound > -1) && !(info.nFlags & OF_SILENT))
				CreateSound (weaponInfoP->nWallHitSound);
		if (weaponInfoP->nWallHitVClip > -1) {
			if (weaponInfoP->xDamageRadius)
				ExplodeSplashDamageWeapon (vHitPt);
			else
				CreateExplosion (info.nSegment, info.position.vPos, weaponInfoP->xImpactSize, weaponInfoP->nWallHitVClip);
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
	if ((info.nId == FLARE_ID) && (!sideP || (gameData.pig.tex.tMapInfoP [sideP->m_nBaseTex].flags & TMI_FORCE_FIELD))) {
		Die ();
		}
	if (!(info.nFlags & OF_SILENT)) {
		switch (wallType) {
			case WHP_NOT_SPECIAL:
				//should be handled above
				//audio.CreateSegmentSound (weaponInfoP->nWallHitSound, info.nSegment, 0, &info.position.vPos, 0, I2X (1));
				break;

			case WHP_NO_KEY:
				//play special hit door sound (if/when we get it)
				CreateSound (SOUND_WEAPON_HIT_DOOR);
			   if (IsMultiGame)
					MultiSendPlaySound (SOUND_WEAPON_HIT_DOOR, I2X (1));
				break;

			case WHP_BLASTABLE:
				//play special blastable CWall sound (if/when we get it)
				if ((weaponInfoP->nWallHitSound > -1) && (!(info.nFlags & OF_SILENT)))
					CreateSound (SOUND_WEAPON_HIT_BLASTABLE);
				break;

			case WHP_DOOR:
				//don't play anything, since door open sound will play
				break;
			}
		}
	}
else {
	// This is a robotP's laser
	if (!bBounce)
		Die ();
	}
return 1;
}

//	-----------------------------------------------------------------------------
//##void CollideCameraAndWall (CObject* camera, fix xHitSpeed, short nHitSeg, short nHitWall,   CFixVector& vHitPt) {
//##	return;
//##}

//##void CollidePowerupAndWall (CObject* powerup, fix xHitSpeed, short nHitSeg, short nHitWall,   CFixVector& vHitPt) {
//##	return;
//##}

int CObject::CollideDebrisAndWall (fix xHitSpeed, short nHitSeg, short nHitWall, CFixVector& vHitPt)
{
if (gameOpts->render.nDebrisLife) {
	CFixVector	vDir = mType.physInfo.velocity,
					vNormal = SEGMENTS [nHitSeg].m_sides [nHitWall].m_normals [0];
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

//##void CollideFireballAndRobot (CObject* fireball, CObject* robotP, CFixVector& vHitPt) {
//##	return;
//##}

//##void CollideFireballAndHostage (CObject* fireball, CObject* hostage, CFixVector& vHitPt) {
//##	return;
//##}

//##void CollideFireballAndPlayer (CObject* fireball, CObject* CPlayerData, CFixVector& vHitPt) {
//##	return;
//##}

//##void CollideFireballAndWeapon (CObject* fireball, CObject* weaponP, CFixVector& vHitPt) {
//##	//weaponP->Die ();
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

int CObject::CollideRobotAndRobot (CObject* other, CFixVector& vHitPt, CFixVector* vNormal)
{
//		robot1-OBJECTS, X2I (robot1->info.position.vPos.x), X2I (robot1->info.position.vPos.y), X2I (robot1->info.position.vPos.z),
//		robot2-OBJECTS, X2I (robot2->info.position.vPos.x), X2I (robot2->info.position.vPos.y), X2I (robot2->info.position.vPos.z),
//		X2I (vHitPt->x), X2I (vHitPt->y), X2I (vHitPt->z));

BumpTwoObjects (this, other, 1, vHitPt);
return 1;
}

//	-----------------------------------------------------------------------------

int CObject::CollideRobotAndReactor (CObject* reactorP, CFixVector& vHitPt, CFixVector* vNormal)
{
if (info.nType == OBJ_ROBOT) {
	CFixVector vHit = reactorP->info.position.vPos - info.position.vPos;
	CFixVector::Normalize (vHit);
	Bump (vHit, 0);
	}
else {
	reactorP->CollideRobotAndReactor (this, vHitPt);
	}
return 1;
}

//	-----------------------------------------------------------------------------
//##void CollideRobotAndHostage (CObject* robotP, CObject* hostage, CFixVector& vHitPt) {
//##	return;
//##}

//	-----------------------------------------------------------------------------

int CObject::CollideRobotAndPlayer (CObject* playerObjP, CFixVector& vHitPt, CFixVector* vNormal)
{
if (!IsStatic ()) {
		int	bTheftAttempt = 0;
		short	nCollisionSeg;

	if (info.nFlags & OF_EXPLODING)
		return 1;
	nCollisionSeg = FindSegByPos (vHitPt, playerObjP->info.nSegment, 1, 0);
	if (nCollisionSeg != -1)
		CreateExplosion (nCollisionSeg, vHitPt, gameData.weapons.info [0].xImpactSize, gameData.weapons.info [0].nWallHitVClip);
	if (playerObjP->info.nId == N_LOCALPLAYER) {
		if (ROBOTINFO (info.nId).companion)	//	Player and companion don't Collide.
			return 1;
		if (ROBOTINFO (info.nId).kamikaze) {
			ApplyDamageToRobot (info.xShield + 1, OBJ_IDX (playerObjP));
	#if DBG
			if (playerObjP == gameData.objs.consoleP)
	#else
			if (!(gameStates.app.bGameSuspended & SUSP_ROBOTS) && (playerObjP == gameData.objs.consoleP))
	#endif
				cockpit->AddPointsToScore (ROBOTINFO (info.nId).scoreValue);
			}
		if (ROBOTINFO (info.nId).thief) {
			if (gameData.ai.localInfo [OBJ_IDX (this)].mode == AIM_THIEF_ATTACK) {
				gameData.time.xLastThiefHitTime = gameData.time.xGame;
				AttemptToStealItem (this, playerObjP->info.nId);
				bTheftAttempt = 1;
				}
			else if (gameData.time.xGame - gameData.time.xLastThiefHitTime < I2X (2))
				return 1;	//	ZOUNDS! BRILLIANT! Thief not Collide with player if not stealing!
								// NO! VERY DUMB! makes thief look very stupid if player hits him while cloaked!-AP
			else
				gameData.time.xLastThiefHitTime = gameData.time.xGame;
			}
		CreateAwarenessEvent (playerObjP, PA_PLAYER_COLLISION);			// CObject this can attract attention to player
		if (USE_D1_AI) {
			DoD1AIRobotHitAttack (this, playerObjP, &vHitPt);
			DoD1AIRobotHit (this, WEAPON_ROBOT_COLLISION);
			}
		else {
			DoAIRobotHitAttack (this, playerObjP, &vHitPt);
			DoAIRobotHit (this, WEAPON_ROBOT_COLLISION);
			}
		}
	else
		MultiRobotRequestChange (this, playerObjP->info.nId);
	// added this if to remove the bump sound if it's the thief.
	// A "steal" sound was added and it was getting obscured by the bump. -AP 10/3/95
	//	Changed by MK to make this sound unless the this stole.
	if (!(bTheftAttempt || ROBOTINFO (info.nId).energyDrain))
		audio.CreateSegmentSound (SOUND_ROBOT_HIT_PLAYER, playerObjP->info.nSegment, 0, vHitPt);
	}
BumpTwoObjects (this, playerObjP, 1, vHitPt, vNormal);
return 1;
}

//	-----------------------------------------------------------------------------
// Provide a way for network message to instantly destroy the control center
// without awarding points or anything.

//	if controlcen == NULL, that means don't do the explosion because the control center
//	was actually in another CObject.
int NetDestroyReactor (CObject* reactorP)
{
if (extraGameInfo [0].nBossCount [0] && !gameData.reactor.bDestroyed) {
	--extraGameInfo [0].nBossCount [0];
	--extraGameInfo [0].nBossCount [1];
	DoReactorDestroyedStuff (reactorP);
	if (reactorP && !(reactorP->info.nFlags & (OF_EXPLODING|OF_DESTROYED))) {
		audio.CreateSegmentSound (SOUND_CONTROL_CENTER_DESTROYED, reactorP->info.nSegment, 0, reactorP->info.position.vPos);
		reactorP->Explode (0);
		}
	return 1;
	}
return 0;
}

//	-----------------------------------------------------------------------------

void CObject::ApplyDamageToReactor (fix xDamage, short nAttacker)
{
	int	whotype, i;

	//	Only allow a player to xDamage the control center.

if ((nAttacker < 0) || (nAttacker > gameData.objs.nLastObject [0]))
	return;
whotype = OBJECTS [nAttacker].info.nType;
if (whotype != OBJ_PLAYER) {
#if TRACE
	console.printf (CON_DBG, "Damage to control center by CObject of nType %i prevented by MK! \n", whotype);
#endif
	return;
	}
if (IsMultiGame && !IsCoopGame && (LOCALPLAYER.timeLevel < netGame.GetControlInvulTime ())) {
	if (OBJECTS [nAttacker].info.nId == N_LOCALPLAYER) {
		int t = netGame.GetControlInvulTime () - LOCALPLAYER.timeLevel;
		int secs = X2I (t) % 60;
		int mins = X2I (t) / 60;
		HUDInitMessage ("%s %d:%02d.", TXT_CNTRLCEN_INVUL, mins, secs);
		}
	return;
	}
if (OBJECTS [nAttacker].info.nId == N_LOCALPLAYER) {
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
		MultiSendDestroyReactor (OBJ_IDX (this), OBJECTS [nAttacker].info.nId);
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

int CObject::CollidePlayerAndReactor (CObject* reactorP, CFixVector& vHitPt, CFixVector* vNormal)
{
	int	i;

if (info.nId == N_LOCALPLAYER) {
	if (0 >= (i = FindReactor (reactorP)))
		gameData.reactor.states [i].bHit = 1;
	AIDoCloakStuff ();				//	In case player cloaked, make control center know where he is.
	}
if (BumpTwoObjects (reactorP, this, 1, vHitPt))
	audio.CreateSegmentSound (SOUND_ROBOT_HIT_PLAYER, info.nSegment, 0, vHitPt);
return 1;
}

//	-----------------------------------------------------------------------------

int CObject::CollidePlayerAndMarker (CObject* markerP, CFixVector& vHitPt, CFixVector* vNormal)
{
#if TRACE
console.printf (CON_DBG, "Collided with markerP %d! \n", markerP->info.nId);
#endif
if (info.nId == N_LOCALPLAYER) {
	int bDrawn;

	if (IsMultiGame && !IsCoopGame)
		bDrawn = HUDInitMessage (TXT_MARKER_PLRMSG, gameData.multiplayer.players [markerP->info.nId / 2].callsign, markerManager.Message (markerP->info.nId));
	else {
		if (*markerManager.Message (markerP->info.nId))
			bDrawn = HUDInitMessage (TXT_MARKER_IDMSG, markerP->info.nId + 1, markerManager.Message (markerP->info.nId));
		else
			bDrawn = HUDInitMessage (TXT_MARKER_ID, markerP->info.nId + 1);
		}
	if (bDrawn)
		audio.PlaySound (SOUND_MARKER_HIT);
	DetectEscortGoalAccomplished (OBJ_IDX (markerP));
   }
return 1;
}

//	-----------------------------------------------------------------------------
//	If a persistent weapon and other object is not a weaponP, weaken it, else kill it.
//	If both OBJECTS are weapons, weaken the weapon.
void CObject::MaybeKillWeapon (CObject* otherObjP)
{
if (IsMine ()) {
	Die ();
	return;
	}
if (mType.physInfo.flags & PF_PERSISTENT) {
	//	Weapons do a lot of damage to weapons, other OBJECTS do much less.
	if (!(otherObjP->mType.physInfo.flags & PF_PERSISTENT)) {
		info.xShield -= otherObjP->info.xShield / ((otherObjP->info.nType == OBJ_WEAPON) ? 2 : 4);
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

int CObject::CollideWeaponAndReactor (CObject* reactorP, CFixVector& vHitPt, CFixVector* vNormal)
{
	int	i;

if (info.nId == OMEGA_ID)
	if (!OkToDoOmegaDamage (this))
		return 1;
if (cType.laserInfo.parent.nType == OBJ_PLAYER) {
	fix damage = info.xShield;
	if (OBJECTS [cType.laserInfo.parent.nObject].info.nId == N_LOCALPLAYER)
		if (0 <= (i = FindReactor (reactorP)))
			gameData.reactor.states [i].bHit = 1;
	if (WI_damage_radius (info.nId))
		ExplodeSplashDamageWeapon (vHitPt);
	else
		CreateExplosion (reactorP->info.nSegment, vHitPt, 3 * reactorP->info.xSize / 20, VCLIP_SMALL_EXPLOSION);
	audio.CreateSegmentSound (SOUND_CONTROL_CENTER_HIT, reactorP->info.nSegment, 0, vHitPt);
	damage = FixMul (damage, cType.laserInfo.xScale);
	reactorP->ApplyDamageToReactor (damage, cType.laserInfo.parent.nObject);
	MaybeKillWeapon (reactorP);
	}
else {	//	If robotP this hits control center, blow it up, make it go away, but do no damage to control center.
	CreateExplosion (reactorP->info.nSegment, vHitPt, 3 * reactorP->info.xSize / 20, VCLIP_SMALL_EXPLOSION);
	MaybeKillWeapon (reactorP);
	}
return 1;
}

//	-----------------------------------------------------------------------------

int CObject::CollideWeaponAndClutter (CObject* clutterP, CFixVector& vHitPt, CFixVector* vNormal)
{
ubyte exp_vclip = VCLIP_SMALL_EXPLOSION;
if (clutterP->info.xShield >= 0)
	clutterP->info.xShield -= info.xShield;
audio.CreateSegmentSound (SOUND_LASER_HIT_CLUTTER, (short) info.nSegment, 0, vHitPt);
CreateExplosion ((short) clutterP->info.nSegment, vHitPt, ((clutterP->info.xSize / 3) * 3) / 4, exp_vclip);
if ((clutterP->info.xShield < 0) && !(clutterP->info.nFlags & (OF_EXPLODING | OF_DESTROYED)))
	clutterP->Explode (STANDARD_EXPL_DELAY);
MaybeKillWeapon (clutterP);
return 1;
}

//--mk, 121094 -- extern void spinRobot (CObject* robotP, CFixVector& vHitPt);

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
	SetupSpherePulse (gameData.multiplayer.spherePulse + N_LOCALPLAYER, 0.02f, 0.5f);
	}
if (!IsMultiGame)
	BuddyMessage ("Nice job, %s!", LOCALPLAYER.callsign);
missionManager.AdvanceLevel ();
gameStates.gameplay.bFinalBossIsDead = 1;
}

extern int MultiAllPlayersAlive ();
void MultiSendFinishGame ();

//	------------------------------------------------------------------------------------------------------

bool CObject::Indestructible (void)
{
return ROBOTINFO (info.nId).strength <= 0; // indestructible static object
}

//	------------------------------------------------------------------------------------------------------
//	Return 1 if this died, else return 0
int CObject::ApplyDamageToRobot (fix xDamage, int nKillerObj)
{
	char		bIsThief, bIsBoss;
	char		tempStolen [MAX_STOLEN_ITEMS];
	CObject	*killerObjP = (nKillerObj < 0) ? NULL : OBJECTS + nKillerObj;

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
	if (killerObjP && (killerObjP->info.nType == OBJ_ROBOT) && !ROBOTINFO (killerObjP->info.nId).companion)
		return 0;
	}
if ((bIsBoss = ROBOTINFO (info.nId).bossFlag)) {
	int i = gameData.bosses.Find (OBJ_IDX (this));
	if (i >= 0) {
		gameData.bosses [i].m_nHitTime = gameData.time.xGame;
		gameData.bosses [i].m_bHasBeenHit = 1;
		}
	}

//	Buddy invulnerable on level 24 so he can give you his important messages.  Bah.
//	Also invulnerable if his cheat for firing weapons is in effect.
if (ROBOTINFO (info.nId).companion) {
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
	if (killerObjP == gameData.objs.consoleP)
		ExecObjTriggers (OBJ_IDX (this), 1);
	return 0;
	}
if (IsMultiGame) {
	bIsThief = (ROBOTINFO (info.nId).thief != 0);
	if (bIsThief)
		memcpy (tempStolen, &gameData.thief.stolenItems [0], gameData.thief.stolenItems.Size ());
	if (!MultiExplodeRobotSub (OBJ_IDX (this), nKillerObj, ROBOTINFO (info.nId).thief)) 
		return 0;
	if (bIsThief)
		memcpy (&gameData.thief.stolenItems [0], tempStolen, gameData.thief.stolenItems.Size ());
	MultiSendRobotExplode (OBJ_IDX (this), nKillerObj, ROBOTINFO (info.nId).thief);
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
else if (ROBOTINFO (info.nId).bDeathRoll)
	StartRobotDeathSequence (this);	//DoReactorDestroyedStuff (NULL);
else {
	if (info.nId == SPECIAL_REACTOR_ROBOT)
		SpecialReactorStuff ();
	Explode (ROBOTINFO (info.nId).kamikaze ? 1 : STANDARD_EXPL_DELAY);		//	Kamikaze, explode right away, IN YOUR FACE!
	}
return 1;
}

//	------------------------------------------------------------------------------------------------------

int	nBuddyGaveHintCount = 5;
fix	xLastTimeBuddyGameHint = 0;

//	Return true if damage done to boss, else return false.
int DoBossWeaponCollision (CObject* robotP, CObject* weaponP, CFixVector& vHitPt)
{
	int	d2BossIndex;
	int	bDamage = 1;
	int	bKinetic = WI_matter (weaponP->info.nId);

d2BossIndex = ROBOTINFO (robotP->info.nId).bossFlag - BOSS_D2;
Assert ((d2BossIndex >= 0) && (d2BossIndex < NUM_D2_BOSSES));

//	See if should spew a bot.
if (weaponP->cType.laserInfo.parent.nType == OBJ_PLAYER) {
	if ((bKinetic && bossProps [gameStates.app.bD1Mission][d2BossIndex].bSpewBotsKinetic) ||
		 (!bKinetic && bossProps [gameStates.app.bD1Mission][d2BossIndex].bSpewBotsEnergy)) {
		int i = gameData.bosses.Find (OBJ_IDX (robotP));
		if (i >= 0) {
			if (bossProps [gameStates.app.bD1Mission][d2BossIndex].bSpewMore && (RandShort () > SHORT_RAND_MAX / 2) &&
				 (robotP->BossSpewRobot (&vHitPt, -1, 0) != -1))
				gameData.bosses [i].m_nLastGateTime = gameData.time.xGame - gameData.bosses [i].m_nGateInterval - 1;	//	Force allowing spew of another bot.
			robotP->BossSpewRobot (&vHitPt, -1, 0);
			}
		}
	}

if (bossProps [gameStates.app.bD1Mission][d2BossIndex].bInvulSpot) {
	fix			dot;
	CFixVector	tvec1;

	//	Boss only vulnerable in back.  See if hit there.
	tvec1 = vHitPt - robotP->info.position.vPos;
	CFixVector::Normalize (tvec1);	//	Note, if BOSS_INVULNERABLE_DOT is close to I2X (1) (in magnitude), then should probably use non-quick version.
	dot = CFixVector::Dot (tvec1, robotP->info.position.mOrient.m.dir.f);
#if TRACE
	console.printf (CON_DBG, "Boss hit vec dot = %7.3f \n", X2F (dot));
#endif
	if (dot > gameData.physics.xBossInvulDot) {
		short	nSegment = FindSegByPos (vHitPt, robotP->info.nSegment, 1, 0);
		audio.CreateSegmentSound (SOUND_WEAPON_HIT_DOOR, nSegment, 0, vHitPt);
		bDamage = 0;

		if (xLastTimeBuddyGameHint == 0)
			xLastTimeBuddyGameHint = RandShort () * 32 + I2X (16);

		if (nBuddyGaveHintCount) {
			if (xLastTimeBuddyGameHint + I2X (20) < gameData.time.xGame) {
				int	sval;

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
		//	Make a copy of this weaponP, because the physics wants to destroy it.
		if ((weaponP->info.nType == OBJ_WEAPON) && !WI_matter (weaponP->info.nId)) {
			short nClone = CreateObject (weaponP->info.nType, weaponP->info.nId, -1, weaponP->info.nSegment, weaponP->info.position.vPos,
												  weaponP->info.position.mOrient, weaponP->info.xSize, 
												  weaponP->info.controlType, weaponP->info.movementType, weaponP->info.renderType);
			if (nClone != -1) {
				CObject	*cloneP = OBJECTS + nClone;
				if (weaponP->info.renderType == RT_POLYOBJ) {
					cloneP->rType.polyObjInfo.nModel = gameData.weapons.info [cloneP->info.nId].nModel;
					cloneP->SetSizeFromModel (0, gameData.weapons.info [cloneP->info.nId].poLenToWidthRatio);
					}
				cloneP->mType.physInfo.thrust.SetZero ();
				cloneP->mType.physInfo.mass = WI_mass (weaponP->info.nType);
				cloneP->mType.physInfo.drag = WI_drag (weaponP->info.nType);
				CFixVector vImpulse = vHitPt - robotP->info.position.vPos;
				CFixVector::Normalize (vImpulse);
				CFixVector vWeapon = weaponP->mType.physInfo.velocity;
				fix speed = CFixVector::Normalize (vWeapon);
				vImpulse += vWeapon * (-I2X (2));
				vImpulse *= (speed / 4);
				cloneP->mType.physInfo.velocity = vImpulse;
				cloneP->info.nFlags |= PF_HAS_BOUNCED;
				}
			}
		}
	}
else if ((bKinetic && bossProps [gameStates.app.bD1Mission][d2BossIndex].bInvulKinetic) ||
		   (!bKinetic && bossProps [gameStates.app.bD1Mission][d2BossIndex].bInvulEnergy)) {
	short	nSegment;

	nSegment = FindSegByPos (vHitPt, robotP->info.nSegment, 1, 0);
	audio.CreateSegmentSound (SOUND_WEAPON_HIT_DOOR, nSegment, 0, vHitPt);
	bDamage = 0;
	}
return bDamage;
}

//	------------------------------------------------------------------------------------------------------

int FindHitObject (CObject* objP, short nObject)
{
	short	*p = gameData.objs.nHitObjects + objP->Index () * MAX_HIT_OBJECTS;
	int	i;

for (i = objP->cType.laserInfo.nLastHitObj; i; i--, p++)
	if (*p == nObject)
		return 1;
return 0;
}

//	------------------------------------------------------------------------------------------------------

int AddHitObject (CObject* objP, short nObject)
{
	short	*p;
	int	i;

if (FindHitObject (objP, nObject))
	return -1;
p = gameData.objs.nHitObjects + objP->Index () * MAX_HIT_OBJECTS;
i = objP->cType.laserInfo.nLastHitObj;
if (i >= MAX_HIT_OBJECTS) {
	memcpy (p + 1, p, (MAX_HIT_OBJECTS - 1) * sizeof (*p));
	p [i - 1] = nObject;
	}
else {
	p [i] = nObject;
	objP->cType.laserInfo.nLastHitObj++;
	}
return 1;
}

//	------------------------------------------------------------------------------------------------------

int CObject::CollideWeaponAndRobot (CObject* robotP, CFixVector& vHitPt, CFixVector* vNormal)
{
if (robotP->IsGeometry ())
	return CollideWeaponAndWall (WI_speed (info.nId, gameStates.app.nDifficultyLevel), robotP->Segment (), -1, vHitPt);

	int			bDamage = 1;
	int			bInvulBoss = 0;
	fix			nStrength = WI_strength (info.nId, gameStates.app.nDifficultyLevel);
	CObject		*parentP = ((cType.laserInfo.parent.nType != OBJ_ROBOT) || (cType.laserInfo.parent.nObject < 0)) ? NULL : OBJECTS + cType.laserInfo.parent.nObject;
	tRobotInfo	*botInfoP = &ROBOTINFO (robotP->info.nId);
	CWeaponInfo *wInfoP = gameData.weapons.info + info.nId;
	bool			bAttackRobots = parentP ? parentP->AttacksRobots () || (EGI_FLAG (bRobotsHitRobots, 0, 0, 0) && gameStates.app.cheats.bRobotsKillRobots) : false;

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
if (botInfoP->bossFlag) {
	int i = gameData.bosses.Find (OBJ_IDX (robotP));
	if (i >= 0)
		gameData.bosses [i].m_nHitTime = gameData.time.xGame;
	if (botInfoP->bossFlag >= BOSS_D2) {
		bDamage = DoBossWeaponCollision (robotP, this, vHitPt);
		bInvulBoss = !bDamage;
		}
	}

//	Put in at request of Jasen (and Adam) because the Buddy-Bot gets in their way.
//	MK has so much fun whacking his butt around the mine he never cared...
if ((cType.laserInfo.parent.nType == OBJ_ROBOT) && !bAttackRobots)
	return 1;
if (botInfoP->companion && (cType.laserInfo.parent.nType != OBJ_ROBOT))
	return 1;
CreateWeaponEffects (1);
if (info.nId == EARTHSHAKER_ID)
	ShakerRockStuff (&Position ());
//	If a persistent this hit robotP most recently, quick abort, else we cream the same robotP many times,
//	depending on frame rate.
if (mType.physInfo.flags & PF_PERSISTENT) {
	if (AddHitObject (this, OBJ_IDX (robotP)) < 0)
		return 1;
	}
if (cType.laserInfo.parent.nSignature == robotP->info.nSignature)
	return 1;
//	Changed, 10/04/95, put out blobs based on skill level and power of this doing damage.
//	Also, only a this hit from a tPlayer this causes smart blobs.
if ((cType.laserInfo.parent.nType == OBJ_PLAYER) && botInfoP->energyBlobs)
	if (!robotP->IsStatic () && (robotP->info.xShield > 0) && IsEnergyWeapon ()) {
		fix xProb = (gameStates.app.nDifficultyLevel+2) * min (info.xShield, robotP->info.xShield);
		xProb = botInfoP->energyBlobs * xProb / (NDL * 32);
		int nBlobs = xProb >> 16;
		if (2 * RandShort () < (xProb & 0xffff))
			nBlobs++;
		if (nBlobs)
			CreateSmartChildren (robotP, nBlobs);
		}

	//	Note: If this hits an invulnerable boss, it will still do splash damage, including to the boss,
	//	unless this is trapped elsewhere.
	if (WI_damage_radius (info.nId)) {
		if (bInvulBoss) {			//don't make badass sound
			//this code copied from ExplodeSplashDamageWeapon ()
			CreateSplashDamageExplosion (this, info.nSegment, vHitPt, wInfoP->xImpactSize, wInfoP->nRobotHitVClip, 
												  nStrength, wInfoP->xDamageRadius, nStrength, cType.laserInfo.parent.nObject);

			}
		else		//Normal splash damage explosion
			ExplodeSplashDamageWeapon (vHitPt);
		}
	if (((cType.laserInfo.parent.nType == OBJ_PLAYER) || bAttackRobots) && !(robotP->info.nFlags & OF_EXPLODING)) {
		CObject* explObjP = NULL;
		if (cType.laserInfo.parent.nObject == LOCALPLAYER.nObject) {
			CreateAwarenessEvent (this, WEAPON_ROBOT_COLLISION);			// object "this" can attract attention to tPlayer
			if (USE_D1_AI)
				DoD1AIRobotHit (robotP, WEAPON_ROBOT_COLLISION);
			else
				DoAIRobotHit (robotP, WEAPON_ROBOT_COLLISION);
			}
	  	else
			MultiRobotRequestChange (robotP, OBJECTS [cType.laserInfo.parent.nObject].info.nId);
		if (botInfoP->nExp1VClip > -1)
			explObjP = CreateExplosion (info.nSegment, vHitPt, (3 * robotP->info.xSize) / 8, (ubyte) botInfoP->nExp1VClip);
		else if (gameData.weapons.info [info.nId].nRobotHitVClip > -1)
			explObjP = CreateExplosion (info.nSegment, vHitPt, wInfoP->xImpactSize, (ubyte) wInfoP->nRobotHitVClip);
		if (explObjP)
			AttachObject (robotP, explObjP);
		if (bDamage && (botInfoP->nExp1Sound > -1))
			audio.CreateSegmentSound (botInfoP->nExp1Sound, robotP->info.nSegment, 0, vHitPt);
		if (!(info.nFlags & OF_HARMLESS)) {
			fix xDamage = bDamage ? FixMul (info.xShield, cType.laserInfo.xScale) : 0;
			//	Cut Gauss xDamage on bosses because it just breaks the game.  Bosses are so easy to
			//	hit, and missing a robotP is what prevents the Gauss from being game-breaking.
			if (info.nId == GAUSS_ID) {
				if (botInfoP->bossFlag)
					xDamage = (xDamage * (2 * NDL - gameStates.app.nDifficultyLevel)) / (2 * NDL);
				}
			else if (info.nId == FUSION_ID) {
				xDamage = gameData.FusionDamage (xDamage);
				}
			if (!robotP->ApplyDamageToRobot (xDamage, cType.laserInfo.parent.nObject))
				BumpTwoObjects (robotP, this, 0, vHitPt);		//only bump if not dead. no xDamage from bump
#if DBG
			else if (cType.laserInfo.parent.nSignature == gameData.objs.consoleP->info.nSignature) {
#else
			else if (!(gameStates.app.bGameSuspended & SUSP_ROBOTS) && (cType.laserInfo.parent.nSignature == gameData.objs.consoleP->info.nSignature)) {
#endif
				cockpit->AddPointsToScore (botInfoP->scoreValue);
				DetectEscortGoalAccomplished (OBJ_IDX (robotP));
				}
			}
		//	If Gauss Cannon, spin robotP.
		if (robotP && !(botInfoP->companion || botInfoP->bossFlag) && (info.nId == GAUSS_ID)) {
			tAIStaticInfo	*aip = &robotP->cType.aiInfo;

			if (aip->SKIP_AI_COUNT * gameData.time.xFrame < I2X (1)) {
				aip->SKIP_AI_COUNT++;
				robotP->mType.physInfo.rotThrust.v.coord.x = FixMul (SRandShort (), gameData.time.xFrame * aip->SKIP_AI_COUNT);
				robotP->mType.physInfo.rotThrust.v.coord.y = FixMul (SRandShort (), gameData.time.xFrame * aip->SKIP_AI_COUNT);
				robotP->mType.physInfo.rotThrust.v.coord.z = FixMul (SRandShort (), gameData.time.xFrame * aip->SKIP_AI_COUNT);
				robotP->mType.physInfo.flags |= PF_USES_THRUST;
				}
			}
		}
MaybeKillWeapon (robotP);
return 1;
}
//	-----------------------------------------------------------------------------

int CObject::CollidePlayerAndHostage (CObject* hostageP, CFixVector& vHitPt, CFixVector* vNormal)
{
if (this == gameData.objs.consoleP) {
	DetectEscortGoalAccomplished (OBJ_IDX (hostageP));
	cockpit->AddPointsToScore (HOSTAGE_SCORE);
	// Do effect
	RescueHostage (hostageP->info.nId);
	// Remove the hostage CObject.
	hostageP->Die ();
	if (IsMultiGame)
		MultiSendRemoveObj (OBJ_IDX (hostageP));
	}
return 1;
}

//	-----------------------------------------------------------------------------

int CObject::CollidePlayerAndPlayer (CObject* otherP, CFixVector& vHitPt, CFixVector* vNormal)
{
if (gameStates.app.bD2XLevel &&
	 (SEGMENTS [info.nSegment].HasNoDamageProp ()))
	return 1;
if (BumpTwoObjects (this, otherP, 1, vHitPt))
	audio.CreateSegmentSound (SOUND_ROBOT_HIT_PLAYER, info.nSegment, 0, vHitPt);
return 1;
}

//	-----------------------------------------------------------------------------

void CObject::ApplyDamageToPlayer (CObject* killerObjP, fix xDamage)
{
if (gameStates.app.bPlayerIsDead) {
	// PrintLog (0, "ApplyDamageToPlayer: Player is already dead\n");
	return;
	}
if (gameStates.app.bD2XLevel && (SEGMENTS [info.nSegment].HasNoDamageProp ())) {
	// PrintLog (0, "ApplyDamageToPlayer: No damage segment\n");
	return;
	}
if ((info.nId == N_LOCALPLAYER) && (LOCALPLAYER.flags & PLAYER_FLAGS_INVULNERABLE)) {
	// PrintLog (0, "ApplyDamageToPlayer: Player is invulnerable\n");
	return;
	}

CPlayerData *killerP; 

if (!killerObjP) 
	killerP = NULL;
else {
	if ((killerObjP->info.nType == OBJ_ROBOT) && ROBOTINFO (killerObjP->info.nId).companion) {
		// PrintLog (0, "ApplyDamageToPlayer: Player was hit by Guidebot\n");
		return;
		}
	killerP = (killerObjP->info.nType == OBJ_PLAYER) ? gameData.multiplayer.players + killerObjP->info.nId : NULL;
	if (killerP)
		// PrintLog (0, "ApplyDamageToPlayer: Damage was inflicted by %s\n", killerP->callsign);
	if (gameStates.app.bHaveExtraGameInfo [1]) {
		if ((killerObjP == this) && !COMPETITION && extraGameInfo [1].bInhibitSuicide) {
			// PrintLog (0, "ApplyDamageToPlayer: Suicide inhibited\n");
			return;
			}
		else if (killerP && !(COMPETITION || extraGameInfo [1].bFriendlyFire)) {
			if (IsTeamGame) {
				if (GetTeam (info.nId) == GetTeam (killerObjP->info.nId)) {
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
	CPlayerData *playerP = gameData.multiplayer.players + info.nId;
	if (IsEntropyGame && extraGameInfo [1].entropy.bPlayerHandicap && killerP) {
		double h = (double) playerP->netKillsTotal / (double) (killerP->netKillsTotal + 1);
		if (h < 0.5)
			h = 0.5;
		else if (h > 1.0)
			h = 1.0;
		if (!(xDamage = (fix) ((double) xDamage * h)))
			xDamage = 1;
		// PrintLog (0, "ApplyDamageToPlayer: Applying player handicap (resulting damage = %d)\n", xDamage);
		}
	playerP->UpdateShield (-xDamage);
	paletteManager.BumpEffect (X2I (xDamage) * 4, -X2I (xDamage / 2), -X2I (xDamage / 2));	//flash red
	if (playerP->Shield () < 0) {
  		playerP->nKillerObj = OBJ_IDX (killerObjP);
		Die ();
		if (gameData.escort.nObjNum != -1)
			if (killerObjP && (killerObjP->info.nType == OBJ_ROBOT) && (ROBOTINFO (killerObjP->info.nId).companion))
				gameData.escort.xSorryTime = gameData.time.xGame;
		}
	}
}

//	-----------------------------------------------------------------------------

int CObject::CollideWeaponAndPlayer (CObject* playerObjP, CFixVector& vHitPt, CFixVector* vNormal)
{
	fix xDamage = info.xShield;

	//	In multiplayer games, only do xDamage to another player if in first frame.
	//	This is necessary because in multiplayer, due to varying framerates, omega blobs actually
	//	have a bit of a lifetime.  But they start out with a lifetime of ONE_FRAME_TIME, and this
	//	gets bashed to 1/4 second in laser_doWeapon_sequence.  This bashing occurs for visual purposes only.
if (gameStates.app.bD2XLevel && (SEGMENTS [playerObjP->info.nSegment].HasNoDamageProp ()))
	return 1;
if ((info.nId == PROXMINE_ID) && IsMultiGame && !COMPETITION && EGI_FLAG (bSmokeGrenades, 0, 0, 0))
	return 1;
if ((info.nId == OMEGA_ID) && !OkToDoOmegaDamage (this))
	return 1;
//	Don't Collide own smart mines unless direct hit.
if ((info.nId == SMARTMINE_ID) &&
	 (OBJ_IDX (playerObjP) == cType.laserInfo.parent.nObject) &&
	 (CFixVector::Dist (vHitPt, playerObjP->info.position.vPos) > playerObjP->info.xSize))
	return 1;
gameData.multiplayer.bWasHit [playerObjP->info.nId] = -1;
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
	if (AddHitObject (this, OBJ_IDX (playerObjP)) < 0)
		return 1;
}
if (playerObjP->info.nId == N_LOCALPLAYER) {
	if (!(LOCALPLAYER.flags & PLAYER_FLAGS_INVULNERABLE)) {
		audio.CreateSegmentSound (SOUND_PLAYER_GOT_HIT, playerObjP->info.nSegment, 0, vHitPt);
		if (IsMultiGame)
			MultiSendPlaySound (SOUND_PLAYER_GOT_HIT, I2X (1));
		}
	else {
		audio.CreateSegmentSound (SOUND_WEAPON_HIT_DOOR, playerObjP->info.nSegment, 0, vHitPt);
		if (IsMultiGame)
			MultiSendPlaySound (SOUND_WEAPON_HIT_DOOR, I2X (1));
		}
	}
CreateExplosion (playerObjP->info.nSegment, vHitPt, I2X (10)/2, VCLIP_PLAYER_HIT);
if (WI_damage_radius (info.nId))
	ExplodeSplashDamageWeapon (vHitPt);
MaybeKillWeapon (playerObjP);
BumpTwoObjects (playerObjP, this, 0, vHitPt);	//no xDamage from bump
if (!WI_damage_radius (info.nId) && (cType.laserInfo.parent.nObject > -1) && !(info.nFlags & OF_HARMLESS))
	playerObjP->ApplyDamageToPlayer (OBJECTS + cType.laserInfo.parent.nObject, xDamage);
//	Robots become aware of you if you get hit.
AIDoCloakStuff ();
return 1;
}

//	-----------------------------------------------------------------------------
//	Nasty robots are the ones that attack you by running into you and doing lots of damage.
int CObject::CollidePlayerAndNastyRobot (CObject* robotP, CFixVector& vHitPt, CFixVector* vNormal)
{
//	if (!(ROBOTINFO (objP->info.nId).energyDrain && gameData.multiplayer.players [info.nId].energy))
CreateExplosion (info.nSegment, vHitPt, I2X (10) / 2, VCLIP_PLAYER_HIT);
if (BumpTwoObjects (this, robotP, 0, vHitPt)) {//no damage from bump
	audio.CreateSegmentSound (ROBOTINFO (robotP->info.nId).clawSound, info.nSegment, 0, vHitPt);
	ApplyDamageToPlayer (robotP, I2X (gameStates.app.nDifficultyLevel+1));
	}
return 1;
}

//	-----------------------------------------------------------------------------

int CObject::CollidePlayerAndObjProducer (void)
{
	CFixVector vExitDir;

CreateSound (SOUND_PLAYER_GOT_HIT);
//	audio.PlaySound (SOUND_PLAYER_GOT_HIT);
CreateExplosion (info.nSegment, info.position.vPos, I2X (10) / 2, VCLIP_PLAYER_HIT);
if (info.nId != N_LOCALPLAYER)
	return 1;
CSegment* segP = SEGMENTS + info.nSegment;
for (short nSide = 0; nSide < SEGMENT_SIDE_COUNT; nSide++)
	if (segP->IsDoorWay (nSide, this) & WID_PASSABLE_FLAG) {
		vExitDir = segP->SideCenter (nSide) - info.position.vPos;
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

int CObject::CollideRobotAndObjProducer (void)
{
	CFixVector	vExitDir;
	CSegment*	segP = SEGMENTS + info.nSegment;

CreateSound (SOUND_ROBOT_HIT);
//	audio.PlaySound (SOUND_ROBOT_HIT);
if (ROBOTINFO (info.nId).nExp1VClip > -1)
	CreateExplosion ((short) info.nSegment, info.position.vPos, (info.xSize/2*3)/4, (ubyte) ROBOTINFO (info.nId).nExp1VClip);
vExitDir.SetZero ();
for (short nSide = 0; nSide < SEGMENT_SIDE_COUNT; nSide++)
	if (segP->IsDoorWay (nSide, NULL) & WID_PASSABLE_FLAG) {
		vExitDir = segP->SideCenter (nSide) - info.position.vPos;
		CFixVector::Normalize (vExitDir);
		}
if (!vExitDir.IsZero ())
	Bump (vExitDir, I2X (8));
ApplyDamageToRobot (I2X (1), -1);
return 1;
}

//##void CollidePlayerAndCamera (CObject* playerObjP, CObject* camera, CFixVector& vHitPt) {
//##	return;
//##}

//	-----------------------------------------------------------------------------

int CObject::CollidePlayerAndPowerup (CObject* powerupP, CFixVector& vHitPt, CFixVector* vNormal)
{
if (gameStates.app.bGameSuspended & SUSP_POWERUPS)
	return 1;
if (!gameStates.app.bEndLevelSequence && !gameStates.app.bPlayerIsDead &&
	(info.nId == N_LOCALPLAYER)) {
	int bPowerupUsed = DoPowerup (powerupP, info.nId);
	if (bPowerupUsed) {
		powerupP->Die ();
		if (IsMultiGame)
			MultiSendRemoveObj (OBJ_IDX (powerupP));
		}
	}
else if (IsCoopGame && (info.nId != N_LOCALPLAYER)) {
	switch (powerupP->info.nId) {
		case POW_KEY_BLUE:
			gameData.multiplayer.players [info.nId].flags |= PLAYER_FLAGS_BLUE_KEY;
			break;
		case POW_KEY_RED:
			gameData.multiplayer.players [info.nId].flags |= PLAYER_FLAGS_RED_KEY;
			break;
		case POW_KEY_GOLD:
			gameData.multiplayer.players [info.nId].flags |= PLAYER_FLAGS_GOLD_KEY;
			break;
		default:
			break;
		}
	}
DetectEscortGoalAccomplished (OBJ_IDX (powerupP));
return 1;
}

//	-----------------------------------------------------------------------------

int CObject::CollidePlayerAndMonsterball (CObject* monsterball, CFixVector& vHitPt, CFixVector* vNormal)
{
if (!gameStates.app.bEndLevelSequence && !gameStates.app.bPlayerIsDead &&
	(info.nId == N_LOCALPLAYER)) {
	if (BumpTwoObjects (this, monsterball, 0, vHitPt))
		audio.CreateSegmentSound (SOUND_ROBOT_HIT_PLAYER, info.nSegment, 0, vHitPt);
	}
return 1;
}

//	-----------------------------------------------------------------------------
//##void CollidePlayerAndDebris (CObject* playerObjP, CObject* debris, CFixVector& vHitPt) {
//##	return;
//##}

int CObject::CollideActorAndClutter (CObject* clutter, CFixVector& vHitPt, CFixVector* vNormal)
{
if (gameStates.app.bD2XLevel &&
	 (SEGMENTS [info.nSegment].HasNoDamageProp ()))
	return 1;
if (!(info.nFlags & OF_EXPLODING) && BumpTwoObjects (clutter, this, 1, vHitPt))
	audio.CreateSegmentSound (SOUND_ROBOT_HIT_PLAYER, info.nSegment, 0, vHitPt);
return 1;
}

//	-----------------------------------------------------------------------------
//	See if otherP causes this weapon to create a splash damage explosion.  If so, create the explosion
//	Return true if weapon does proximity (as opposed to only contact) damage when it explodes.
int CObject::MaybeDetonateWeapon (CObject* otherP, CFixVector& vHitPt)
{
if (!gameData.weapons.info [info.nId].xDamageRadius)
	return 0;
fix xDist = CFixVector::Dist (info.position.vPos, otherP->info.position.vPos);
if (xDist >= I2X (5))
	SetLife (min (xDist / 64, I2X (1)));
else {
	MaybeKillWeapon (otherP);
	if (info.nFlags & OF_SHOULD_BE_DEAD) {
		CreateWeaponEffects (0);
		ExplodeSplashDamageWeapon (vHitPt);
		audio.CreateSegmentSound (gameData.weapons.info [info.nId].nRobotHitSound, info.nSegment, 0, vHitPt);
		}
	}
return 1;
}

//	-----------------------------------------------------------------------------

inline int DestroyWeapon (int nTarget, int nWeapon)
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

int CObject::CollideWeaponAndWeapon (CObject* other, CFixVector& vHitPt, CFixVector* vNormal)
{
	int	id1 = info.nId;
	int	id2 = other->info.nId;
	int	bKill1, bKill2;

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

int CObject::CollideWeaponAndMonsterball (CObject* mBallP, CFixVector& vHitPt, CFixVector* vNormal)
{
if (cType.laserInfo.parent.nType == OBJ_PLAYER) {
	audio.CreateSegmentSound (SOUND_ROBOT_HIT, info.nSegment, 0, vHitPt);
	if (info.nId == EARTHSHAKER_ID)
		ShakerRockStuff (&Position ());
	if (mType.physInfo.flags & PF_PERSISTENT) {
		if (AddHitObject (this, OBJ_IDX (mBallP)) < 0)
			return 1;
		}
	CreateExplosion (mBallP->info.nSegment, vHitPt, I2X (10)/2, VCLIP_PLAYER_HIT);
	if (WI_damage_radius (info.nId))
		ExplodeSplashDamageWeapon (vHitPt);
	MaybeKillWeapon (mBallP);
	BumpTwoObjects (this, mBallP, 1, vHitPt);
	}
return 1;
}

//	-----------------------------------------------------------------------------
//##void CollideWeaponAndCamera (CObject* weaponP, CObject* camera, CFixVector& vHitPt) {
//##	return;
//##}

//	-----------------------------------------------------------------------------

int CObject::CollideWeaponAndDebris (CObject* debrisP, CFixVector& vHitPt, CFixVector* vNormal)
{
//	Hack! Prevent debrisP from causing bombs spewed at player death to detonate!
if (IsMine ()) {
	if (cType.laserInfo.xCreationTime + I2X (1)/2 > gameData.time.xGame)
		return 1;
	}
if ((cType.laserInfo.parent.nType == OBJ_PLAYER) && !(debrisP->info.nFlags & OF_EXPLODING)) {
	audio.CreateSegmentSound (SOUND_ROBOT_HIT, info.nSegment, 0, vHitPt);
	debrisP->Explode (0);
	if (WI_damage_radius (info.nId))
		ExplodeSplashDamageWeapon (vHitPt);
	MaybeKillWeapon (debrisP);
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

int CollideTwoObjects (CObject* objA, CObject* objB, CFixVector& vHitPt, CFixVector* vNormal)
{
	int collisionType = COLLISION_OF (objA->info.nType, objB->info.nType);

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
	int i, j;

for (i = 0; i < MAX_OBJECT_TYPES; i++)
	for (j = 0; j < MAX_OBJECT_TYPES; j++)
		gameData.objs.collisionResult [i][j] = RESULT_NOTHING;

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

int CObject::CollideObjectAndWall (fix xHitSpeed, short nHitSeg, short nHitWall, CFixVector& vHitPt)
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
		break;	//CollidePowerupAndWall (objP, xHitSpeed, nHitSeg, nHitWall, vHitPt);
	default:
		Error ("Unhandled CObject nType hit CWall in Collide.c \n");
	}
return 1;
}

//	-----------------------------------------------------------------------------

void SetDebrisCollisions (void)
{
	int	h = gameOpts->render.nDebrisLife ? RESULT_CHECK : RESULT_NOTHING;

SET_COLLISION (OBJ_PLAYER, OBJ_DEBRIS, h);
SET_COLLISION (OBJ_ROBOT, OBJ_DEBRIS, h);
}

//	-----------------------------------------------------------------------------
// eof
