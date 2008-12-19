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

#include "inferno.h"
#include "rle.h"
#include "stdlib.h"
#include "texmap.h"
#include "key.h"
#include "gameseg.h"
#include "lightning.h"
#include "physics.h"
#include "slew.h"
#include "render.h"
#include "fireball.h"
#include "hostage.h"
#include "gauges.h"
#include "scores.h"
#include "textures.h"
#include "newdemo.h"
#include "endlevel.h"
#include "multibot.h"
#include "text.h"
#include "automap.h"
#include "sphere.h"
#include "monsterball.h"

//#define _DEBUG

#ifdef TACTILE
#include "tactile.h"
#endif

#ifdef EDITOR
#include "editor/editor.h"
#endif

#include "collide.h"
#include "escort.h"

#define STANDARD_EXPL_DELAY (f1_0/4)

//##void CollideFireballAndWall (CObject *fireball, fix xHitSpeed, short nHitSeg, short nHitWall, CFixVector * vHitPt)	{
//##	return;
//##}

//	-------------------------------------------------------------------------------------------------------------
//	The only reason this routine is called (as of 10/12/94) is so Brain guys can open doors.
void CollideRobotAndWall (CObject *robotP, fix xHitSpeed, short nHitSeg, short nHitSide, CFixVector * vHitPt)
{
	tAILocalInfo	*ailP = gameData.ai.localInfo + OBJ_IDX (robotP);
	tRobotInfo		*botInfoP = &ROBOTINFO (robotP->info.nId);

if ((robotP->info.nId != ROBOT_BRAIN) &&
	 (robotP->cType.aiInfo.behavior != AIB_RUN_FROM) &&
	 !botInfoP->companion &&
	 (robotP->cType.aiInfo.behavior != AIB_SNIPE))
	return;

CWall *wallP = SEGMENTS [nHitSeg].Wall (nHitSide);
if (!wallP || (wallP->nType != WALL_DOOR))
	return;

if ((wallP->keys == KEY_NONE) && (wallP->state == WALL_DOOR_CLOSED) && !(wallP->flags & WALL_DOOR_LOCKED))
	WallOpenDoor (SEGMENTS + nHitSeg, nHitSide);
else if (botInfoP->companion) {
	if ((ailP->mode != AIM_GOTO_PLAYER) && (gameData.escort.nSpecialGoal != ESCORT_GOAL_SCRAM))
		return;
	if (!(wallP->flags & WALL_DOOR_LOCKED) || ((wallP->keys != KEY_NONE) && (wallP->keys & LOCALPLAYER.flags)))
		WallOpenDoor (SEGMENTS + nHitSeg, nHitSide);
	}
else if (botInfoP->thief) {		//	Thief allowed to go through doors to which CPlayerData has key.
	if ((wallP->keys != KEY_NONE) && (wallP->keys & LOCALPLAYER.flags))
		WallOpenDoor (SEGMENTS + nHitSeg, nHitSide);
	}
}

//##void CollideHostageAndWall (CObject *hostage, fix xHitSpeed, short nHitSeg, short nHitSide, CFixVector * vHitPt)	{
//##	return;
//##}

//	-------------------------------------------------------------------------------------------------------------

int ApplyDamageToClutter (CObject *clutter, fix damage)
{
if (clutter->info.nFlags & OF_EXPLODING)
	return 0;
if (clutter->info.xShields < 0)
	return 0;	//clutter already dead...
clutter->info.xShields -= damage;
if (clutter->info.xShields < 0) {
	ExplodeObject (clutter, 0);
	return 1;
	}
return 0;
}

//	-----------------------------------------------------------------------------

//given the specified vForce, apply damage from that vForce to an CObject
void ApplyForceDamage (CObject *objP, fix vForce, CObject *otherObjP)
{
	int	result;
	fix damage;

if (objP->info.nFlags & (OF_EXPLODING|OF_SHOULD_BE_DEAD))
	return;		//already exploding or dead
damage = FixDiv (vForce, objP->mType.physInfo.mass) / 8;
if ((otherObjP->info.nType == OBJ_PLAYER) && gameStates.app.cheats.bMonsterMode)
	damage = 0x7fffffff;
	switch (objP->info.nType) {
		case OBJ_ROBOT:
			if (ROBOTINFO (objP->info.nId).attackType == 1) {
				if (otherObjP->info.nType == OBJ_WEAPON)
					result = ApplyDamageToRobot (objP, damage/4, otherObjP->cType.laserInfo.parent.nObject);
				else
					result = ApplyDamageToRobot (objP, damage/4, OBJ_IDX (otherObjP));
				}
			else {
				if (otherObjP->info.nType == OBJ_WEAPON)
					result = ApplyDamageToRobot (objP, damage/2, otherObjP->cType.laserInfo.parent.nObject);
				else
					result = ApplyDamageToRobot (objP, damage/2, OBJ_IDX (otherObjP));
				}
			if (!gameStates.gameplay.bNoBotAI && result && (otherObjP->cType.laserInfo.parent.nSignature == gameData.objs.consoleP->info.nSignature))
				AddPointsToScore (ROBOTINFO (objP->info.nId).scoreValue);
			break;

		case OBJ_PLAYER:
			//	If colliding with a claw nType robotP, do damage proportional to gameData.time.xFrame because you can Collide with those
			//	bots every frame since they don't move.
			if ((otherObjP->info.nType == OBJ_ROBOT) && (ROBOTINFO (otherObjP->info.nId).attackType))
				damage = FixMul (damage, gameData.time.xFrame*2);
			//	Make trainee easier.
			if (gameStates.app.nDifficultyLevel == 0)
				damage /= 2;
			ApplyDamageToPlayer (objP, otherObjP, damage);
			break;

		case OBJ_CLUTTER:
			ApplyDamageToClutter (objP, damage);
			break;

		case OBJ_REACTOR:
			ApplyDamageToReactor (objP, damage, (short) OBJ_IDX (otherObjP));
			break;

		case OBJ_WEAPON:
			break;		//weapons don't take damage

		default:
			Int3 ();
		}
}

//	-----------------------------------------------------------------------------

void BumpThisObject (CObject *objP, CObject *otherObjP, CFixVector *vForce, int bDamage)
{
	fix			xForceMag;
	CFixVector	vRotForce;

if (!(objP->mType.physInfo.flags & PF_PERSISTENT)) {
	if (objP->info.nType == OBJ_PLAYER) {
		if (otherObjP->info.nType == OBJ_MONSTERBALL) {
			double mq;

			mq = (double) otherObjP->mType.physInfo.mass / ((double) objP->mType.physInfo.mass * (double) nMonsterballPyroForce);
			vRotForce[X] = (fix) ((double) (*vForce)[X] * mq);
			vRotForce[Y] = (fix) ((double) (*vForce)[Y] * mq);
			vRotForce[Z] = (fix) ((double) (*vForce)[Z] * mq);
			PhysApplyForce (objP, vForce);
			//PhysApplyRot (objP, &vRotForce);
			}
		else {
			CFixVector force2;
			force2[X] = (*vForce)[X] / 4;
			force2[Y] = (*vForce)[Y] / 4;
			force2[Z] = (*vForce)[Z] / 4;
			PhysApplyForce (objP, &force2);
#if 0
			HUDMessage (0, "%1.2f   %1.2f",
							VmVecMag (&objP->mType.physInfo.velocity) / 65536.0,
							VmVecMag (&force2) / 65536.0);
#endif
			if (bDamage && ((otherObjP->info.nType != OBJ_ROBOT) || !ROBOTINFO (otherObjP->info.nId).companion)) {
				xForceMag = force2.Mag();
				ApplyForceDamage (objP, xForceMag, otherObjP);
				}
			}
		}
	else {
		fix h = (4 + gameStates.app.nDifficultyLevel);
		if (objP->info.nType == OBJ_ROBOT) {
			if (ROBOTINFO (objP->info.nId).bossFlag)
				return;
			vRotForce[X] = (*vForce)[X] / h;
			vRotForce[Y] = (*vForce)[Y] / h;
			vRotForce[Z] = (*vForce)[Z] / h;
			PhysApplyForce (objP, vForce);
			PhysApplyRot (objP, &vRotForce);
			}
		else if ((objP->info.nType == OBJ_CLUTTER) || (objP->info.nType == OBJ_DEBRIS) || (objP->info.nType == OBJ_REACTOR)) {
			vRotForce[X] = (*vForce)[X] / h;
			vRotForce[Y] = (*vForce)[Y] / h;
			vRotForce[Z] = (*vForce)[Z] / h;
			PhysApplyForce (objP, vForce);
			PhysApplyRot (objP, &vRotForce);
			}
		else if (objP->info.nType == OBJ_MONSTERBALL) {
			double mq;

			if (otherObjP->info.nType == OBJ_PLAYER) {
				gameData.hoard.nLastHitter = OBJ_IDX (otherObjP);
				mq = ((double) otherObjP->mType.physInfo.mass * (double) nMonsterballPyroForce) / (double) objP->mType.physInfo.mass;
				}
			else {
				gameData.hoard.nLastHitter = otherObjP->cType.laserInfo.parent.nObject;
				mq = (double) nMonsterballForces [otherObjP->info.nId] * ((double) F1_0 / (double) otherObjP->mType.physInfo.mass) / 40.0;
				}
			vRotForce[X] = (fix) ((double) (*vForce)[X] * mq);
			vRotForce[Y] = (fix) ((double) (*vForce)[Y] * mq);
			vRotForce[Z] = (fix) ((double) (*vForce)[Z] * mq);
			PhysApplyForce (objP, &vRotForce);
			PhysApplyRot (objP, &vRotForce);
			if (gameData.hoard.nLastHitter == LOCALPLAYER.nObject)
				MultiSendMonsterball (1, 0);
			}
		else
			return;
		if (bDamage) {
			xForceMag = vForce->Mag();
			ApplyForceDamage (objP, xForceMag, otherObjP);
			}
		}
	}
}

//	-----------------------------------------------------------------------------
//deal with two OBJECTS bumping into each other.  Apply vForce from collision
//to each robotP.  The flags tells whether the objects should take damage from
//the collision.

int BumpTwoObjects (CObject *objP0, CObject *objP1, int bDamage, CFixVector *vHitPt)
{
	CFixVector	vForce, p0, p1, v0, v1, vh, vr, vn0, vn1;
	fix			mag, dot, m0, m1;
	CObject		*t;

if (objP0->info.movementType != MT_PHYSICS)
	t = objP1;
else if (objP1->info.movementType != MT_PHYSICS)
	t = objP0;
else
	t = NULL;
if (t) {
	Assert (t->info.movementType == MT_PHYSICS);
	vForce = t->mType.physInfo.velocity * (-t->mType.physInfo.mass);
#if 1//def _DEBUG
	if (!vForce.IsZero())
#endif
	PhysApplyForce (t, &vForce);
	return 1;
	}
#if DBG
//redo:
#endif
p0 = objP0->info.position.vPos;
p1 = objP1->info.position.vPos;
v0 = objP0->mType.physInfo.velocity;
v1 = objP1->mType.physInfo.velocity;
mag = CFixVector::Dot (v0, v1);
vn0 = v0;
m0 = CFixVector::Normalize (vn0);
vn1 = v1;
m1 = CFixVector::Normalize (vn1);
if (m0 && m1) {
	if (m0 > m1) {
		double d = (double)m1 / (double)m0;
		vn0 *= (fix)(d * 65536.0);
		vn1 *= (fix)(d * 65536.0);
//		VmVecScaleFrac (&vn0, m1, m0);
//		VmVecScaleFrac (&vn1, m1, m0);
		}
	else {
		double d = (double)m1 / (double)m0;
		vn0 *= (fix)(d * 65536.0);
		vn1 *= (fix)(d * 65536.0);
//		VmVecScaleFrac (&vn0, m0, m1);
//		VmVecScaleFrac (&vn1, m0, m1);
		}
	}
vh = p0 - p1;
if (!EGI_FLAG (nHitboxes, 0, 0, 0)) {
	m0 = vh.Mag();
	if (m0 > ((objP0->info.xSize + objP1->info.xSize) * 3) / 4) {
		p0 += vn0;
		p1 += vn1;
		vh = p0 - p1;
		m1 = vh.Mag();
		if (m1 > m0) {
#if 0//def _DEBUG
			HUDMessage (0, "moving away (%d, %d)", m0, m1);
#endif
			return 0;
			}
		}
	}
#if 0//def _DEBUG
HUDMessage (0, "colliding (%1.2f, %1.2f)", X2F (m0), X2F (m1));
#endif
vForce = v0 - v1;
m0 = objP0->mType.physInfo.mass;
m1 = objP1->mType.physInfo.mass;
if (!((m0 + m1) && FixMul (m0, m1))) {
#if 0//def _DEBUG
	HUDMessage (0, "Invalid Mass!!!");
#endif
	return 0;
	}
mag = vForce.Mag();
double d = (double)(2 * FixMul (m0, m1)) / (double)(m0 + m1);
vForce *= (fix)(d * 65536.0);
//VmVecScaleFrac (&vForce, 2 * FixMul (m0, m1), m0 + m1);
mag = vForce.Mag();
#if 0//def _DEBUG
if (fabs (X2F (mag) > 10000))
	;//goto redo;
HUDMessage (0, "bump force: %c%1.2f", (SIGN (vForce[X]) * SIGN (vForce[Y]) * SIGN (vForce[Z])) ? '-' : '+', X2F (mag));
#endif
if (mag < (m0 + m1) / 200) {
#if 0//def _DEBUG
	HUDMessage (0, "bump force too low");
#endif
	if (EGI_FLAG (bUseHitAngles, 0, 0, 0))
		return 0;	// don't bump if force too low
	}
#if 0//def _DEBUG
HUDMessage (0, "%d %d", mag, (objP0->mType.physInfo.mass + objP1->mType.physInfo.mass) / 200);
#endif
if (EGI_FLAG (bUseHitAngles, 0, 0, 0)) {
	// exert force in the direction of the hit point to the object's center
	vh = *vHitPt - objP1->info.position.vPos;
	if (CFixVector::Normalize (vh) > F1_0 / 16) {
		vr = vh;
		vh = -vh;
		vh *= mag;
		BumpThisObject (objP1, objP0, &vh, bDamage);
		// compute reflection vector. The vector from the other object's center to the hit point
		// serves as Normal.
		v1 = v0;
		CFixVector::Normalize (v1);
		dot = CFixVector::Dot (v1, vr);
		vr *= (2 * dot);
		//VmVecNegate (VmVecDec (&vr, &v0));
		CFixVector::Normalize (vr);
		vr *= mag;
		vr = -vr;
		BumpThisObject (objP0, objP1, &vr, bDamage);
		return 1;
		}
	}
BumpThisObject (objP1, objP0, &vForce, 0);
vForce = -vForce;
BumpThisObject (objP0, objP1, &vForce, 0);
return 1;
}

//	-----------------------------------------------------------------------------

void BumpOneObject (CObject *objP0, CFixVector *hit_dir, fix damage)
{
	CFixVector	hit_vec;

hit_vec = *hit_dir;
hit_vec *= damage;
PhysApplyForce (objP0, &hit_vec);
}

//	-----------------------------------------------------------------------------

#define DAMAGE_SCALE 		128	//	Was 32 before 8:55 am on Thursday, September 15, changed by MK, walls were hurting me more than robots!
#define DAMAGE_THRESHOLD 	 (F1_0/3)
#define WALL_LOUDNESS_SCALE (20)

fix force_force = I2X (50);

void CollidePlayerAndWall (CObject *playerObjP, fix xHitSpeed, short nHitSeg, short nHitSide, CFixVector * vHitPt)
{
	fix damage;
	char bForceFieldHit = 0;
	int nBaseTex, nOvlTex;

if (playerObjP->info.nId != gameData.multiplayer.nLocalPlayer) // Execute only for local CPlayerData
	return;
nBaseTex = SEGMENTS [nHitSeg].m_sides [nHitSide].m_nBaseTex;
//	If this CWall does damage, don't make *BONK* sound, we'll be making another sound.
if (gameData.pig.tex.tMapInfoP [nBaseTex].damage > 0)
	return;
if (gameData.pig.tex.tMapInfoP [nBaseTex].flags & TMI_FORCE_FIELD) {
	CFixVector vForce;
	paletteManager.BumpEffect (0, 0, 60);	//flash blue
	//knock CPlayerData around
	vForce [X] = 40 * (d_rand () - 16384);
	vForce [Y] = 40 * (d_rand () - 16384);
	vForce [Z] = 40 * (d_rand () - 16384);
	PhysApplyRot (playerObjP, &vForce);
#ifdef TACTILE
	if (TactileStick)
		Tactile_apply_force (&vForce, &playerObjP->info.position.mOrient);
#endif
	//make sound
	DigiLinkSoundToPos (SOUND_FORCEFIELD_BOUNCE_PLAYER, nHitSeg, 0, vHitPt, 0, f1_0);
	if (gameData.app.nGameMode & GM_MULTI)
		MultiSendPlaySound (SOUND_FORCEFIELD_BOUNCE_PLAYER, f1_0);
	bForceFieldHit=1;
	}
else {
#ifdef TACTILE
	CFixVector vForce;
	if (TactileStick) {
		vForce[X] = -playerObjP->mType.physInfo.velocity[X];
		vForce[Y] = -playerObjP->mType.physInfo.velocity[Y];
		vForce[Z] = -playerObjP->mType.physInfo.velocity[Z];
		Tactile_do_collide (&vForce, &playerObjP->info.position.mOrient);
	}
#endif
   WallHitProcess (SEGMENTS + nHitSeg, nHitSide, 20, playerObjP->info.nId, playerObjP);
	}
if (gameStates.app.bD2XLevel && (SEGMENTS [nHitSeg].m_nType == SEGMENT_IS_NODAMAGE))
	return;
//	** Damage from hitting CWall **
//	If the CPlayerData has less than 10% shields, don't take damage from bump
// Note: Does quad damage if hit a vForce field - JL
damage = (xHitSpeed / DAMAGE_SCALE) * (bForceFieldHit * 8 + 1);
nOvlTex = SEGMENTS [nHitSeg].m_sides [nHitSide].m_nOvlTex;
//don't do CWall damage and sound if hit lava or water
if ((gameData.pig.tex.tMapInfoP [nBaseTex].flags & (TMI_WATER|TMI_VOLATILE)) ||
		(nOvlTex && (gameData.pig.tex.tMapInfoP [nOvlTex].flags & (TMI_WATER|TMI_VOLATILE))))
	damage = 0;
if (damage >= DAMAGE_THRESHOLD) {
	int	volume = (xHitSpeed- (DAMAGE_SCALE*DAMAGE_THRESHOLD)) / WALL_LOUDNESS_SCALE ;
	CreateAwarenessEvent (playerObjP, PA_WEAPON_WALL_COLLISION);
	if (volume > F1_0)
		volume = F1_0;
	if (volume > 0 && !bForceFieldHit) {  // uhhhgly hack
		DigiLinkSoundToPos (SOUND_PLAYER_HIT_WALL, nHitSeg, 0, vHitPt, 0, volume);
		if (gameData.app.nGameMode & GM_MULTI)
			MultiSendPlaySound (SOUND_PLAYER_HIT_WALL, volume);
		}
	if (!(LOCALPLAYER.flags & PLAYER_FLAGS_INVULNERABLE))
		if (LOCALPLAYER.shields > f1_0*10 || bForceFieldHit)
			ApplyDamageToPlayer (playerObjP, playerObjP, damage);
	}
return;
}

//	-----------------------------------------------------------------------------

fix	xLastVolatileScrapeSoundTime = 0;

int CollideWeaponAndWall (CObject *weaponP, fix xHitSpeed, short nHitSeg, short nHitWall, CFixVector * vHitPt);
int CollideDebrisAndWall (CObject *debris, fix xHitSpeed, short nHitSeg, short nHitWall, CFixVector * vHitPt);

//see if CWall is volatile or water
//if volatile, cause damage to CPlayerData
//returns 1=lava, 2=water
int CheckVolatileWall (CObject *objP, int nSegment, int nSide, CFixVector *vHitPt)
{
	fix	d, water;
	int	nTexture;

Assert (objP->info.nType == OBJ_PLAYER);
nTexture = SEGMENTS [nSegment].m_sides [nSide].m_nBaseTex;
d = gameData.pig.tex.tMapInfoP [nTexture].damage;
water = (gameData.pig.tex.tMapInfoP [nTexture].flags & TMI_WATER);
if (d > 0 || water) {
	if (objP->info.nId == gameData.multiplayer.nLocalPlayer) {
		if (d > 0) {
			fix damage = FixMul (d, gameData.time.xFrame);
			if (gameStates.app.nDifficultyLevel == 0)
				damage /= 2;
			if (!(LOCALPLAYER.flags & PLAYER_FLAGS_INVULNERABLE))
				ApplyDamageToPlayer (objP, objP, damage);

#ifdef TACTILE
			if (TactileStick)
				Tactile_Xvibrate (50, 25);
#endif

			paletteManager.BumpEffect (X2I (damage*4), 0, 0);	//flash red
			}
		objP->mType.physInfo.rotVel[X] = (d_rand () - 16384)/2;
		objP->mType.physInfo.rotVel[Z] = (d_rand () - 16384)/2;
		}
	return (d > 0) ? 1 : 2;
	}
else {
#ifdef TACTILE
	if (TactileStick && !(gameData.app.nFrameCount & 15))
		 Tactile_Xvibrate_clear ();
#endif
	return 0;
	}
}

//	-----------------------------------------------------------------------------

int CheckVolatileSegment (CObject *objP, int nSegment)
{
	fix d;

//	Assert (objP->info.nType==OBJ_PLAYER);
if (!EGI_FLAG (bFluidPhysics, 1, 0, 0))
	return 0;
if (SEGMENTS [nSegment].m_nType == SEGMENT_IS_WATER)
	d = 0;
else if (SEGMENTS [nSegment].m_nType == SEGMENT_IS_LAVA)
	d = gameData.pig.tex.tMapInfo [0][404].damage / 2;
else {
#ifdef TACTILE
	if (TactileStick && !(gameData.app.nFrameCount & 15))
		Tactile_Xvibrate_clear ();
#endif
	return 0;
	}
if (d > 0) {
	fix damage = FixMul (d, gameData.time.xFrame) / 2;
	if (gameStates.app.nDifficultyLevel == 0)
		damage /= 2;
	if (objP->info.nType == OBJ_PLAYER) {
		if (!(gameData.multiplayer.players [objP->info.nId].flags & PLAYER_FLAGS_INVULNERABLE))
			ApplyDamageToPlayer (objP, objP, damage);
		}
	if (objP->info.nType == OBJ_ROBOT) {
		ApplyDamageToRobot (objP, objP->info.xShields + 1, OBJ_IDX (objP));
		}

#ifdef TACTILE
	if (TactileStick)
		Tactile_Xvibrate (50, 25);
#endif
	if ((objP->info.nType == OBJ_PLAYER) && (objP->info.nId == gameData.multiplayer.nLocalPlayer))
		paletteManager.BumpEffect (X2I (damage*4), 0, 0);	//flash red
	if ((objP->info.nType == OBJ_PLAYER) || (objP->info.nType == OBJ_ROBOT)) {
		objP->mType.physInfo.rotVel[X] = (d_rand () - 16384) / 4;
		objP->mType.physInfo.rotVel[Z] = (d_rand () - 16384) / 4;
		}
	return (d > 0) ? 1 : 2;
	}
if (((objP->info.nType == OBJ_PLAYER) || (objP->info.nType == OBJ_ROBOT)) &&
	 (objP->mType.physInfo.thrust[X] || objP->mType.physInfo.thrust[Y] || objP->mType.physInfo.thrust[Z])) {
	objP->mType.physInfo.rotVel[X] = (d_rand () - 16384) / 8;
	objP->mType.physInfo.rotVel[Z] = (d_rand () - 16384) / 8;
	}
return 0;
}

//	-----------------------------------------------------------------------------
//this gets called when an CObject is scraping along the CWall
void ScrapeObjectOnWall (CObject *objP, short nHitSeg, short hitside, CFixVector * vHitPt)
{
switch (objP->info.nType) {
	case OBJ_PLAYER:
		if (objP->info.nId == gameData.multiplayer.nLocalPlayer) {
			int nType = CheckVolatileWall (objP, nHitSeg, hitside, vHitPt);
			if (nType != 0) {
				CFixVector	vHit, vRand;

				if ((gameData.time.xGame > xLastVolatileScrapeSoundTime + F1_0/4) ||
						(gameData.time.xGame < xLastVolatileScrapeSoundTime)) {
					short sound = (nType == 1) ? SOUND_VOLATILE_WALL_HISS : SOUND_SHIP_IN_WATER;
					xLastVolatileScrapeSoundTime = gameData.time.xGame;
					DigiLinkSoundToPos (sound, nHitSeg, 0, vHitPt, 0, F1_0);
					if (IsMultiGame)
						MultiSendPlaySound (sound, F1_0);
					}
				vHit = SEGMENTS [nHitSeg].m_sides [hitside].m_normals [0];
				vRand = CFixVector::Random();
				vHit += vRand * (F1_0/8);
				CFixVector::Normalize (vHit);
				BumpOneObject (objP, &vHit, F1_0*8);
				}
			}
		break;

	//these two kinds of OBJECTS below shouldn't really slide, so
	//if this scrape routine gets called (which it might if the
	//CObject (such as a fusion blob) was created already poking
	//through the CWall) call the Collide routine.

	case OBJ_WEAPON:
		CollideWeaponAndWall (objP, 0, nHitSeg, hitside, vHitPt);
		break;

	case OBJ_DEBRIS:
		CollideDebrisAndWall (objP, 0, nHitSeg, hitside, vHitPt);
		break;
	}

}

//	-----------------------------------------------------------------------------
//if an effect is hit, and it can blow up, then blow it up
//returns true if it blew up
int CheckEffectBlowup (CSegment *segP, short nSide, CFixVector* vHit, CObject* blower, int bForceBlowup)
{
	int				tm, tmf, ec, nBitmap = 0;
	int				bOkToBlow = 0, nSwitchType = -1;
	short				nSound, bPermaTrigger;
	ubyte				vc;
	fix				u, v;
	fix				xDestSize;
	tEffectClip*	ecP = NULL;
	CBitmap*			bmP;
	CWall*			wallP;
	CTrigger*		trigP;
	//	If this CWall has a CTrigger and the blower-upper is not the CPlayerData or the buddy, abort!

if (blower->cType.laserInfo.parent.nType == OBJ_ROBOT)
	if (ROBOTINFO (OBJECTS [blower->cType.laserInfo.parent.nObject].info.nId).companion)
		bOkToBlow = 1;

if (!(bOkToBlow || (blower->cType.laserInfo.parent.nType == OBJ_PLAYER))) {
	if ((wallP = segP->Wall (nSide)) && (wallP->nTrigger < gameData.trigs.nTriggers))
		return 0;
	}

if (!(tm = segP->m_sides [nSide].m_nOvlTex))
	return 0;

tmf = segP->m_sides [nSide].m_nOvlOrient;		//tm flags
ec = gameData.pig.tex.tMapInfoP [tm].nEffectClip;
if (ec < 0) {
	if (gameData.pig.tex.tMapInfoP [tm].destroyed == -1)
		return 0;
	nBitmap = -1;
	nSwitchType = 0;
	}
else {
	ecP = gameData.eff.effectP + ec;
	if (ecP->flags & EF_ONE_SHOT)
		return 0;
	nBitmap = ecP->nDestBm;
	if (nBitmap < 0)
		return 0;
	nSwitchType = 1;
	}
//check if it's an animation (monitor) or casts light
bmP = gameData.pig.tex.bitmapP + gameData.pig.tex.bmIndexP [tm].index;
PIGGY_PAGE_IN (gameData.pig.tex.bmIndexP [tm].index, gameStates.app.bD1Data);
//this can be blown up...did we hit it?
if (!bForceBlowup) {
	segP->HitPointUV (nSide, &u, &v, NULL, *vHit, 0);	//evil: always say face zero
	bForceBlowup = !PixelTranspType (tm, tmf,  segP->m_sides [nSide].m_nFrame, u, v);
	}
if (!bForceBlowup)
	return 0;

if (IsMultiGame && netGame.bIndestructibleLights && !nSwitchType)
	return 0;
//note: this must get called before the texture changes,
//because we use the light value of the texture to change
//the static light in the CSegment
wallP = segP->Wall (nSide);
bPermaTrigger = (trigP = segP->Trigger (nSide)) && (trigP->flags & TF_PERMANENT);
if (!bPermaTrigger)
	SubtractLight (SEG_IDX (segP), nSide);
if (gameData.demo.nState == ND_STATE_RECORDING)
	NDRecordEffectBlowup (SEG_IDX (segP), nSide, vHit);
if (nSwitchType) {
	xDestSize = ecP->dest_size;
	vc = ecP->dest_vclip;
	}
else {
	xDestSize = I2X (20);
	vc = 3;
	}
ObjectCreateExplosion (SEG_IDX (segP), vHit, xDestSize, vc);
if (nSwitchType) {
	if ((nSound = gameData.eff.vClipP [vc].nSound) != -1)
		DigiLinkSoundToPos (nSound, SEG_IDX (segP), 0, vHit,  0, F1_0);
	if ((nSound = ecP->nSound) != -1)		//kill sound
		DigiKillSoundLinkedToSegment (SEG_IDX (segP), nSide, nSound);
	if (!bPermaTrigger && (ecP->dest_eclip != -1) && (gameData.eff.effectP [ecP->dest_eclip].nSegment == -1)) {
		tEffectClip	*newEcP = gameData.eff.effectP + ecP->dest_eclip;
		int nNewBm = newEcP->changingWallTexture;
		newEcP->time_left = EffectFrameTime (newEcP);
		newEcP->nCurFrame = 0;
		newEcP->nSegment = SEG_IDX (segP);
		newEcP->nSide = nSide;
		newEcP->flags |= EF_ONE_SHOT;
		newEcP->nDestBm = ecP->nDestBm;

		Assert ((nNewBm != 0) && (segP->m_sides [nSide].m_nOvlTex != 0));
		segP->m_sides [nSide].m_nOvlTex = nNewBm;		//replace with destoyed
		}
	else {
		Assert ((nBitmap != 0) && (segP->m_sides [nSide].m_nOvlTex != 0));
		if (!bPermaTrigger)
			segP->m_sides [nSide].m_nOvlTex = nBitmap;		//replace with destoyed
		}
	}
else {
	if (!bPermaTrigger)
		segP->m_sides [nSide].m_nOvlTex = gameData.pig.tex.tMapInfoP [tm].destroyed;
	//assume this is a light, and play light sound
	DigiLinkSoundToPos (SOUND_LIGHT_BLOWNUP, SEG_IDX (segP), 0, vHit,  0, F1_0);
	}
return 1;		//blew up!
}

//	Copied from laser.c!
#define	DESIRED_OMEGA_DIST	 (F1_0*5)		//	This is the desired distance between blobs.  For distances > MIN_OMEGA_BLOBS*DESIRED_OMEGA_DIST, but not very large, this will apply.
#define	MAX_OMEGA_BLOBS		16				//	No matter how far away the obstruction, this is the maximum number of blobs.
#define	MAX_OMEGA_DIST			 (MAX_OMEGA_BLOBS * DESIRED_OMEGA_DIST)		//	Maximum extent of lightning blobs.

//	-------------------------------------------------
//	Return true if ok to do Omega damage.
int OkToDoOmegaDamage (CObject *weaponP)
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

int CreateWeaponEffects (CObject *objP, int bExplBlast)
{
if ((objP->info.nType == OBJ_WEAPON) && gameData.objs.bIsMissile [objP->info.nId]) {
	if (bExplBlast)
		CreateExplBlast (objP);
	if ((objP->info.nId == EARTHSHAKER_ID) || (objP->info.nId == EARTHSHAKER_ID))
		RequestEffects (objP, MISSILE_LIGHTNINGS);
	else if ((objP->info.nId == EARTHSHAKER_MEGA_ID) || (objP->info.nId == ROBOT_SHAKER_MEGA_ID))
		RequestEffects (objP, MISSILE_LIGHTNINGS);
	else if ((objP->info.nId == MEGAMSL_ID) || (objP->info.nId == ROBOT_MEGAMSL_ID))
		RequestEffects (objP, MISSILE_LIGHTNINGS);
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

int CollideWeaponAndWall (CObject *weaponP, fix xHitSpeed, short nHitSeg, short nHitWall, CFixVector * vHitPt)
{
	CSegment		*segP = SEGMENTS + nHitSeg;
	CSide			*sideP = segP->m_sides + nHitWall;
	tWeaponInfo *wInfoP = gameData.weapons.info + weaponP->info.nId;
	CObject		*wObjP = OBJECTS + weaponP->cType.laserInfo.parent.nObject;

	int	bBounce, bBlewUp, bEscort, wallType, nPlayer;
	fix	nStrength = WI_strength (weaponP->info.nId, gameStates.app.nDifficultyLevel);

if (weaponP->info.nId == OMEGA_ID)
	if (!OkToDoOmegaDamage (weaponP))
		return 1;

//	If this is a guided missile and it strikes fairly directly, clear bounce flag.
if (weaponP->info.nId == GUIDEDMSL_ID) {
	fix dot = CFixVector::Dot (weaponP->info.position.mOrient.FVec (), sideP->m_normals[0]);
#if TRACE
	console.printf (CON_DBG, "Guided missile dot = %7.3f \n", X2F (dot));
#endif
	if (dot < -F1_0/6) {
#if TRACE
		console.printf (CON_DBG, "Guided missile loses bounciness. \n");
#endif
		weaponP->mType.physInfo.flags &= ~PF_BOUNCE;
		}
	else {
		CFixVector	vReflect;
		CAngleVector	va;
		vReflect = CFixVector::Reflect(weaponP->info.position.mOrient.FVec (), sideP->m_normals[0]);
		va = vReflect.ToAnglesVec();
		weaponP->info.position.mOrient = CFixMatrix::Create(va);
		}
	}

bBounce = (weaponP->mType.physInfo.flags & PF_BOUNCE) != 0;
if (!bBounce)
	CreateWeaponEffects (weaponP, 1);
//if an energy weaponP hits a forcefield, let it bounce
if ((gameData.pig.tex.tMapInfoP [sideP->m_nBaseTex].flags & TMI_FORCE_FIELD) &&
	 ((weaponP->info.nType != OBJ_WEAPON) || wInfoP->energy_usage)) {

	//make sound
	DigiLinkSoundToPos (SOUND_FORCEFIELD_BOUNCE_WEAPON, nHitSeg, 0, vHitPt, 0, f1_0);
	if (IsMultiGame)
		MultiSendPlaySound (SOUND_FORCEFIELD_BOUNCE_WEAPON, f1_0);
	return 1;	//bail here. physics code will bounce this CObject
	}

#if DBG
if (gameStates.input.keys.pressed [KEY_LAPOSTRO])
	if (weaponP->cType.laserInfo.parent.nObject == LOCALPLAYER.nObject) {
		//	MK: Real pain when you need to know a segP:CSide and you've got quad lasers.
#if TRACE
		console.printf (CON_DBG, "Your laser hit at CSegment = %i, CSide = %i \n", nHitSeg, nHitWall);
#endif
		//HUDInitMessage ("Hit at segment = %i, side = %i", nHitSeg, nHitWall);
		if (weaponP->info.nId < 4)
			SubtractLight (nHitSeg, nHitWall);
		else if (weaponP->info.nId == FLARE_ID)
			AddLight (nHitSeg, nHitWall);
		}
if (!(weaponP->mType.physInfo.velocity[X] ||
	   weaponP->mType.physInfo.velocity[Y] ||
	   weaponP->mType.physInfo.velocity[Z])) {
	Int3 ();	//	Contact Matt: This is impossible.  A weaponP with 0 velocity hit a CWall, which doesn't move.
	return 1;
	}
#endif
bBlewUp = CheckEffectBlowup (segP, nHitWall, vHitPt, weaponP, 0);
if ((weaponP->cType.laserInfo.parent.nType == OBJ_ROBOT) && ROBOTINFO (wObjP->info.nId).companion) {
	bEscort = 1;
	if (IsMultiGame) {
		Int3 ();  // Get Jason!
	   return 1;
	   }
	nPlayer = gameData.multiplayer.nLocalPlayer;		//if single CPlayerData, he's the CPlayerData's buddy
	}
else {
	bEscort = 0;
	nPlayer = (wObjP->info.nType == OBJ_PLAYER) ? wObjP->info.nId : -1;
	}
if (bBlewUp) {		//could be a CWall switch
	//for CWall triggers, always say that the CPlayerData shot it out.  This is
	//because robots can shoot out CWall triggers, and so the CTrigger better
	//take effect
	//	NO -- Changed by MK, 10/18/95.  We don't want robots blowing puzzles.  Only CPlayerData or buddy can open!
	CheckTrigger (segP, nHitWall, weaponP->cType.laserInfo.parent.nObject, 1);
	}
if (weaponP->info.nId == EARTHSHAKER_ID)
	ShakerRockStuff ();
wallType = WallHitProcess (segP, nHitWall, weaponP->info.xShields, nPlayer, weaponP);
// Wall is volatile if either tmap 1 or 2 is volatile
if ((gameData.pig.tex.tMapInfoP [sideP->m_nBaseTex].flags & TMI_VOLATILE) ||
	 (sideP->m_nOvlTex && (gameData.pig.tex.tMapInfoP [sideP->m_nOvlTex].flags & TMI_VOLATILE))) {
	ubyte tVideoClip;
	//we've hit a volatile CWall
	DigiLinkSoundToPos (SOUND_VOLATILE_WALL_HIT, nHitSeg, 0, vHitPt, 0, F1_0);
	//for most weapons, use volatile CWall hit.  For mega, use its special tVideoClip
	tVideoClip = (weaponP->info.nId == MEGAMSL_ID) ? wInfoP->robot_hit_vclip : VCLIP_VOLATILE_WALL_HIT;
	//	New by MK: If powerful badass, explode as badass, not due to lava, fixes megas being wimpy in lava.
	if (wInfoP->damage_radius >= VOLATILE_WALL_DAMAGE_RADIUS/2)
		ExplodeBadassWeapon (weaponP, vHitPt);
	else
		ObjectCreateBadassExplosion (weaponP, nHitSeg, vHitPt,
			wInfoP->impact_size + VOLATILE_WALL_IMPACT_SIZE,
			tVideoClip,
			nStrength / 4 + VOLATILE_WALL_EXPL_STRENGTH, 	//	diminished by mk on 12/08/94, i was doing 70 damage hitting lava on lvl 1.
			wInfoP->damage_radius+VOLATILE_WALL_DAMAGE_RADIUS,
			nStrength / 2 + VOLATILE_WALL_DAMAGE_FORCE,
			weaponP->cType.laserInfo.parent.nObject);
	KillObject (weaponP);		//make flares die in lava
	}
else if ((gameData.pig.tex.tMapInfoP [sideP->m_nBaseTex].flags & TMI_WATER) ||
			(sideP->m_nOvlTex && (gameData.pig.tex.tMapInfoP [sideP->m_nOvlTex].flags & TMI_WATER))) {
	//we've hit water
	//	MK: 09/13/95: Badass in water is 1/2 Normal intensity.
	if (wInfoP->matter) {
		DigiLinkSoundToPos (SOUNDMSL_HIT_WATER, nHitSeg, 0, vHitPt, 0, F1_0);
		if (wInfoP->damage_radius) {
			DigiLinkSoundToObject (SOUND_BADASS_EXPLOSION, OBJ_IDX (weaponP), 0, F1_0, SOUNDCLASS_EXPLOSION);
			//	MK: 09/13/95: Badass in water is 1/2 Normal intensity.
			ObjectCreateBadassExplosion (weaponP, nHitSeg, vHitPt,
				wInfoP->impact_size/2,
				wInfoP->robot_hit_vclip,
				nStrength / 4,
				wInfoP->damage_radius,
				nStrength / 2,
				weaponP->cType.laserInfo.parent.nObject);
			}
		else
			ObjectCreateExplosion (weaponP->info.nSegment, &weaponP->info.position.vPos, wInfoP->impact_size, wInfoP->wall_hit_vclip);
		}
	else {
		DigiLinkSoundToPos (SOUND_LASER_HIT_WATER, nHitSeg, 0, vHitPt, 0, F1_0);
		ObjectCreateExplosion (weaponP->info.nSegment, &weaponP->info.position.vPos, wInfoP->impact_size, VCLIP_WATER_HIT);
		}
	KillObject (weaponP);		//make flares die in water
	}
else {
	if (!bBounce) {
		//if it's not the CPlayerData's weaponP, or it is the CPlayerData's and there
		//is no CWall, and no blowing up monitor, then play sound
		if ((weaponP->cType.laserInfo.parent.nType != OBJ_PLAYER) ||
			 ((!sideP->IsWall () || wallType == WHP_NOT_SPECIAL) && !bBlewUp))
			if ((wInfoP->wall_hitSound > -1) && !(weaponP->info.nFlags & OF_SILENT))
				DigiLinkSoundToPos (wInfoP->wall_hitSound, weaponP->info.nSegment, 0, &weaponP->info.position.vPos, 0, F1_0);
		if (wInfoP->wall_hit_vclip > -1)	{
			if (wInfoP->damage_radius)
				ExplodeBadassWeapon (weaponP, vHitPt);
			else
				ObjectCreateExplosion (weaponP->info.nSegment, &weaponP->info.position.vPos, wInfoP->impact_size, wInfoP->wall_hit_vclip);
			}
		}
	}
//	If weaponP fired by CPlayerData or companion...
if ((weaponP->cType.laserInfo.parent.nType == OBJ_PLAYER) || bEscort) {
	if (!(weaponP->info.nFlags & OF_SILENT) &&
		 (weaponP->cType.laserInfo.parent.nObject == LOCALPLAYER.nObject))
		CreateAwarenessEvent (weaponP, PA_WEAPON_WALL_COLLISION);			// CObject "weaponP" can attract attention to CPlayerData

//		if (weaponP->info.nId != FLARE_ID) {
//	We now allow flares to open doors.

	if (!bBounce && ((weaponP->info.nId != FLARE_ID) || (weaponP->cType.laserInfo.parent.nType != OBJ_PLAYER))) {
		KillObject (weaponP);
		}

	//don't let flares stick in vForce fields
	if ((weaponP->info.nId == FLARE_ID) && (gameData.pig.tex.tMapInfoP [sideP->m_nBaseTex].flags & TMI_FORCE_FIELD)) {
		KillObject (weaponP);
		}
	if (!(weaponP->info.nFlags & OF_SILENT)) {
		switch (wallType) {
			case WHP_NOT_SPECIAL:
				//should be handled above
				//DigiLinkSoundToPos (wInfoP->wall_hitSound, weaponP->info.nSegment, 0, &weaponP->info.position.vPos, 0, F1_0);
				break;

			case WHP_NO_KEY:
				//play special hit door sound (if/when we get it)
				DigiLinkSoundToPos (SOUND_WEAPON_HIT_DOOR, weaponP->info.nSegment, 0, &weaponP->info.position.vPos, 0, F1_0);
			   if (gameData.app.nGameMode & GM_MULTI)
					MultiSendPlaySound (SOUND_WEAPON_HIT_DOOR, F1_0);
				break;

			case WHP_BLASTABLE:
				//play special blastable CWall sound (if/when we get it)
				if ((wInfoP->wall_hitSound > -1) && (!(weaponP->info.nFlags & OF_SILENT)))
					DigiLinkSoundToPos (SOUND_WEAPON_HIT_BLASTABLE, weaponP->info.nSegment, 0, &weaponP->info.position.vPos, 0, F1_0);
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
		KillObject (weaponP);
	}
return 1;
}

//	-----------------------------------------------------------------------------
//##void CollideCameraAndWall (CObject *camera, fix xHitSpeed, short nHitSeg, short nHitWall, CFixVector * vHitPt)	{
//##	return;
//##}

//##void CollidePowerupAndWall (CObject *powerup, fix xHitSpeed, short nHitSeg, short nHitWall, CFixVector * vHitPt)	{
//##	return;
//##}

int CollideDebrisAndWall (CObject *debris, fix xHitSpeed, short nHitSeg, short nHitWall, CFixVector * vHitPt)
{
if (gameOpts->render.nDebrisLife) {
	CFixVector	vDir = debris->mType.physInfo.velocity,
					vNormal = SEGMENTS [nHitSeg].m_sides [nHitWall].m_normals [0];
	debris->mType.physInfo.velocity = CFixVector::Reflect(vDir, vNormal);
	DigiLinkSoundToPos (SOUND_PLAYER_HIT_WALL, nHitSeg, 0, vHitPt, 0, F1_0 / 3);
	}
else
	ExplodeObject (debris, 0);
return 1;
}

//##void CollideFireballAndFireball (CObject *fireball1, CObject *fireball2, CFixVector *vHitPt) {
//##	return;
//##}

//##void CollideFireballAndRobot (CObject *fireball, CObject *robotP, CFixVector *vHitPt) {
//##	return;
//##}

//##void CollideFireballAndHostage (CObject *fireball, CObject *hostage, CFixVector *vHitPt) {
//##	return;
//##}

//##void CollideFireballAndPlayer (CObject *fireball, CObject *CPlayerData, CFixVector *vHitPt) {
//##	return;
//##}

//##void CollideFireballAndWeapon (CObject *fireball, CObject *weaponP, CFixVector *vHitPt) {
//##	//KillObject (weaponP);
//##	return;
//##}

//##void CollideFireballAndCamera (CObject *fireball, CObject *camera, CFixVector *vHitPt) {
//##	return;
//##}

//##void CollideFireballAndPowerup (CObject *fireball, CObject *powerup, CFixVector *vHitPt) {
//##	return;
//##}

//##void CollideFireballAndDebris (CObject *fireball, CObject *debris, CFixVector *vHitPt) {
//##	return;
//##}

//	-------------------------------------------------------------------------------------------------------------------

int CollideRobotAndRobot (CObject *robotP1, CObject *robotP2, CFixVector *vHitPt)
{
//		robot1-OBJECTS, X2I (robot1->info.position.vPos.x), X2I (robot1->info.position.vPos.y), X2I (robot1->info.position.vPos.z),
//		robot2-OBJECTS, X2I (robot2->info.position.vPos.x), X2I (robot2->info.position.vPos.y), X2I (robot2->info.position.vPos.z),
//		X2I (vHitPt->x), X2I (vHitPt->y), X2I (vHitPt->z));

BumpTwoObjects (robotP1, robotP2, 1, vHitPt);
return 1;
}

//	-----------------------------------------------------------------------------

int CollideRobotAndReactor (CObject *objP1, CObject *obj2, CFixVector *vHitPt)
{
if (objP1->info.nType == OBJ_ROBOT) {
	CFixVector vHit= obj2->info.position.vPos - objP1->info.position.vPos;
	CFixVector::Normalize (vHit);
	BumpOneObject (objP1, &vHit, 0);
	}
else {
	CFixVector vHit = objP1->info.position.vPos - obj2->info.position.vPos;
	CFixVector::Normalize (vHit);
	BumpOneObject (obj2, &vHit, 0);
	}
return 1;
}

//	-----------------------------------------------------------------------------
//##void CollideRobotAndHostage (CObject *robotP, CObject *hostage, CFixVector *vHitPt) {
//##	return;
//##}

//	-----------------------------------------------------------------------------

fix xLastThiefHitTime;

int  CollideRobotAndPlayer (CObject *robotP, CObject *playerObjP, CFixVector *vHitPt)
{
	int	bTheftAttempt = 0;
	short	nCollisionSeg;

if (robotP->info.nFlags & OF_EXPLODING)
	return 1;
nCollisionSeg = FindSegByPos (*vHitPt, playerObjP->info.nSegment, 1, 0);
if (nCollisionSeg != -1)
	ObjectCreateExplosion (nCollisionSeg, vHitPt, gameData.weapons.info [0].impact_size, gameData.weapons.info [0].wall_hit_vclip);
if (playerObjP->info.nId == gameData.multiplayer.nLocalPlayer) {
	if (ROBOTINFO (robotP->info.nId).companion)	//	Player and companion don't Collide.
		return 1;
	if (ROBOTINFO (robotP->info.nId).kamikaze) {
		ApplyDamageToRobot (robotP, robotP->info.xShields + 1, OBJ_IDX (playerObjP));
		if (!gameStates.gameplay.bNoBotAI && (playerObjP == gameData.objs.consoleP))
			AddPointsToScore (ROBOTINFO (robotP->info.nId).scoreValue);
		}
	if (ROBOTINFO (robotP->info.nId).thief) {
		if (gameData.ai.localInfo [OBJ_IDX (robotP)].mode == AIM_THIEF_ATTACK) {
			xLastThiefHitTime = gameData.time.xGame;
			AttemptToStealItem (robotP, playerObjP->info.nId);
			bTheftAttempt = 1;
			}
		else if (gameData.time.xGame - xLastThiefHitTime < F1_0*2)
			return 1;	//	ZOUNDS! BRILLIANT! Thief not Collide with CPlayerData if not stealing!
							// NO! VERY DUMB! makes thief look very stupid if CPlayerData hits him while cloaked!-AP
		else
			xLastThiefHitTime = gameData.time.xGame;
		}
	CreateAwarenessEvent (playerObjP, PA_PLAYER_COLLISION);			// CObject robotP can attract attention to CPlayerData
	if (USE_D1_AI) {
		DoD1AIRobotHitAttack (robotP, playerObjP, vHitPt);
		DoD1AIRobotHit (robotP, WEAPON_ROBOT_COLLISION);
		}
	else {
		DoAIRobotHitAttack (robotP, playerObjP, vHitPt);
		DoAIRobotHit (robotP, WEAPON_ROBOT_COLLISION);
		}
	}
else
	MultiRobotRequestChange (robotP, playerObjP->info.nId);
// added this if to remove the bump sound if it's the thief.
// A "steal" sound was added and it was getting obscured by the bump. -AP 10/3/95
//	Changed by MK to make this sound unless the robotP stole.
if (!(bTheftAttempt || ROBOTINFO (robotP->info.nId).energyDrain))
	DigiLinkSoundToPos (SOUND_ROBOT_HIT_PLAYER, playerObjP->info.nSegment, 0, vHitPt, 0, F1_0);
BumpTwoObjects (robotP, playerObjP, 1, vHitPt);
return 1;
}

//	-----------------------------------------------------------------------------
// Provide a way for network message to instantly destroy the control center
// without awarding points or anything.

//	if controlcen == NULL, that means don't do the explosion because the control center
//	was actually in another CObject.
int NetDestroyReactor (CObject *reactorP)
{
if (extraGameInfo [0].nBossCount && !gameData.reactor.bDestroyed) {
	extraGameInfo [0].nBossCount--;
	DoReactorDestroyedStuff (reactorP);
	if (reactorP && !(reactorP->info.nFlags & (OF_EXPLODING|OF_DESTROYED))) {
		DigiLinkSoundToPos (SOUND_CONTROL_CENTER_DESTROYED, reactorP->info.nSegment, 0, &reactorP->info.position.vPos, 0, F1_0);
		ExplodeObject (reactorP, 0);
		}
	return 1;
	}
return 0;
}

//	-----------------------------------------------------------------------------

void ApplyDamageToReactor (CObject *reactorP, fix xDamage, short nAttacker)
{
	int	whotype, i;

	//	Only allow a CPlayerData to xDamage the control center.

if ((nAttacker < 0) || (nAttacker > gameData.objs.nLastObject [0]))
	return;
whotype = OBJECTS [nAttacker].info.nType;
if (whotype != OBJ_PLAYER) {
#if TRACE
	console.printf (CON_DBG, "Damage to control center by CObject of nType %i prevented by MK! \n", whotype);
#endif
	return;
	}
if (IsMultiGame && !IsCoopGame && (LOCALPLAYER.timeLevel < netGame.controlInvulTime)) {
	if (OBJECTS [nAttacker].info.nId == gameData.multiplayer.nLocalPlayer) {
		int t = netGame.controlInvulTime - LOCALPLAYER.timeLevel;
		int secs = X2I (t) % 60;
		int mins = X2I (t) / 60;
		HUDInitMessage ("%s %d:%02d.", TXT_CNTRLCEN_INVUL, mins, secs);
		}
	return;
	}
if (OBJECTS [nAttacker].info.nId == gameData.multiplayer.nLocalPlayer) {
	if (0 >= (i = FindReactor (reactorP)))
		gameData.reactor.states [i].bHit = 1;
	AIDoCloakStuff ();
	}
if (reactorP->info.xShields >= 0)
	reactorP->info.xShields -= xDamage;
if ((reactorP->info.xShields < 0) && !(reactorP->info.nFlags & (OF_EXPLODING | OF_DESTROYED))) {
	/*if (gameStates.app.bD2XLevel && gameStates.gameplay.bMultiBosses)*/
	extraGameInfo [0].nBossCount--;
	DoReactorDestroyedStuff (reactorP);
	if (IsMultiGame) {
		if (!gameStates.gameplay.bNoBotAI && (nAttacker == LOCALPLAYER.nObject))
			AddPointsToScore (CONTROL_CEN_SCORE);
		MultiSendDestroyReactor (OBJ_IDX (reactorP), OBJECTS [nAttacker].info.nId);
		}
	else if (!gameStates.gameplay.bNoBotAI)
		AddPointsToScore (CONTROL_CEN_SCORE);
	DigiLinkSoundToPos (SOUND_CONTROL_CENTER_DESTROYED, reactorP->info.nSegment, 0, &reactorP->info.position.vPos, 0, F1_0);
	ExplodeObject (reactorP, 0);
	}
}

//	-----------------------------------------------------------------------------

int CollidePlayerAndReactor (CObject *reactorP, CObject *playerObjP, CFixVector *vHitPt)
{
	int	i;

if (playerObjP->info.nId == gameData.multiplayer.nLocalPlayer) {
	if (0 >= (i = FindReactor (reactorP)))
		gameData.reactor.states [i].bHit = 1;
	AIDoCloakStuff ();				//	In case CPlayerData cloaked, make control center know where he is.
	}
if (BumpTwoObjects (reactorP, playerObjP, 1, vHitPt))
	DigiLinkSoundToPos (SOUND_ROBOT_HIT_PLAYER, playerObjP->info.nSegment, 0, vHitPt, 0, F1_0);
return 1;
}

//	-----------------------------------------------------------------------------

int CollidePlayerAndMarker (CObject *markerP, CObject *playerObjP, CFixVector *vHitPt)
{
#if TRACE
console.printf (CON_DBG, "Collided with markerP %d! \n", markerP->info.nId);
#endif
if (playerObjP->info.nId==gameData.multiplayer.nLocalPlayer) {
	int drawn;

	if (IsMultiGame && !IsCoopGame)
		drawn = HUDInitMessage (TXT_MARKER_PLRMSG, gameData.multiplayer.players [markerP->info.nId/2].callsign, gameData.marker.szMessage [markerP->info.nId]);
	else
		if (gameData.marker.szMessage [markerP->info.nId][0])
			drawn = HUDInitMessage (TXT_MARKER_IDMSG, markerP->info.nId+1, gameData.marker.szMessage [markerP->info.nId]);
		else
			drawn = HUDInitMessage (TXT_MARKER_ID, markerP->info.nId+1);
	if (drawn)
		DigiPlaySample (SOUND_MARKER_HIT, F1_0);
	DetectEscortGoalAccomplished (OBJ_IDX (markerP));
   }
return 1;
}

//	-----------------------------------------------------------------------------
//	If a persistent weapon and other object is not a weaponP, weaken it, else kill it.
//	If both OBJECTS are weapons, weaken the weapon.
void MaybeKillWeapon (CObject *weaponP, CObject *otherObjP)
{
if (WeaponIsMine (weaponP->info.nId)) {
	KillObject (weaponP);
	return;
	}
//	Changed, 10/12/95, MK: Make weaponP-weaponP collisions always kill both weapons if not persistent.
//	Reason: Otherwise you can't use proxbombs to detonate incoming homing missiles (or mega missiles).
if (weaponP->mType.physInfo.flags & PF_PERSISTENT) {
	//	Weapons do a lot of damage to weapons, other OBJECTS do much less.
	if (!(otherObjP->mType.physInfo.flags & PF_PERSISTENT)) {
		weaponP->info.xShields -= otherObjP->info.xShields / ((otherObjP->info.nType == OBJ_WEAPON) ? 2 : 4);
		if (weaponP->info.xShields <= 0) {
			weaponP->info.xShields = 0;
			KillObject (weaponP);	// weaponP->info.xLifeLeft = 1;
			}
		}
	}
else {
	KillObject (weaponP);	// weaponP->info.xLifeLeft = 1;
	}
}

//	-----------------------------------------------------------------------------

int CollideWeaponAndReactor (CObject *weaponP, CObject *reactorP, CFixVector *vHitPt)
{
	int	i;

if (weaponP->info.nId == OMEGA_ID)
	if (!OkToDoOmegaDamage (weaponP))
		return 1;
if (weaponP->cType.laserInfo.parent.nType == OBJ_PLAYER) {
	fix damage = weaponP->info.xShields;
	if (OBJECTS [weaponP->cType.laserInfo.parent.nObject].info.nId == gameData.multiplayer.nLocalPlayer)
		if (0 <= (i = FindReactor (reactorP)))
			gameData.reactor.states [i].bHit = 1;
	if (WI_damage_radius (weaponP->info.nId))
		ExplodeBadassWeapon (weaponP, vHitPt);
	else
		ObjectCreateExplosion (reactorP->info.nSegment, vHitPt, reactorP->info.xSize*3/20, VCLIP_SMALL_EXPLOSION);
	DigiLinkSoundToPos (SOUND_CONTROL_CENTER_HIT, reactorP->info.nSegment, 0, vHitPt, 0, F1_0);
	damage = FixMul (damage, weaponP->cType.laserInfo.xScale);
	ApplyDamageToReactor (reactorP, damage, weaponP->cType.laserInfo.parent.nObject);
	MaybeKillWeapon (weaponP, reactorP);
	}
else {	//	If robotP weaponP hits control center, blow it up, make it go away, but do no damage to control center.
	ObjectCreateExplosion (reactorP->info.nSegment, vHitPt, reactorP->info.xSize*3/20, VCLIP_SMALL_EXPLOSION);
	MaybeKillWeapon (weaponP, reactorP);
	}
return 1;
}

//	-----------------------------------------------------------------------------

int CollideWeaponAndClutter (CObject *weaponP, CObject *clutterP, CFixVector *vHitPt)
{
ubyte exp_vclip = VCLIP_SMALL_EXPLOSION;
if (clutterP->info.xShields >= 0)
	clutterP->info.xShields -= weaponP->info.xShields;
DigiLinkSoundToPos (SOUND_LASER_HIT_CLUTTER, (short) weaponP->info.nSegment, 0, vHitPt, 0, F1_0);
ObjectCreateExplosion ((short) clutterP->info.nSegment, vHitPt, ((clutterP->info.xSize/3)*3)/4, exp_vclip);
if ((clutterP->info.xShields < 0) && !(clutterP->info.nFlags & (OF_EXPLODING|OF_DESTROYED)))
	ExplodeObject (clutterP, STANDARD_EXPL_DELAY);
MaybeKillWeapon (weaponP, clutterP);
return 1;
}

//--mk, 121094 -- extern void spinRobot (CObject *robotP, CFixVector *vHitPt);

fix	nFinalBossCountdownTime = 0;

//	------------------------------------------------------------------------------------------------------

void DoFinalBossFrame (void)
{
if (!gameStates.gameplay.bFinalBossIsDead)
	return;
if (!gameData.reactor.bDestroyed)
	return;
if (nFinalBossCountdownTime == 0)
	nFinalBossCountdownTime = F1_0*2;
nFinalBossCountdownTime -= gameData.time.xFrame;
if (nFinalBossCountdownTime > 0)
	return;
paletteManager.FadeOut ();
StartEndLevelSequence (0);		//pretend we hit the exit CTrigger
}

//	------------------------------------------------------------------------------------------------------
//	This is all the ugly stuff we do when you kill the final boss so that you don't die or something
//	which would ruin the logic of the cut sequence.
void DoFinalBossHacks (void)
{
if (gameStates.app.bPlayerIsDead) {
	Int3 ();		//	Uh-oh, CPlayerData is dead.  Try to rescue him.
	gameStates.app.bPlayerIsDead = 0;
	}
if (LOCALPLAYER.shields <= 0)
	LOCALPLAYER.shields = 1;
//	If you're not invulnerable, get invulnerable!
if (!(LOCALPLAYER.flags & PLAYER_FLAGS_INVULNERABLE)) {
	LOCALPLAYER.invulnerableTime = gameData.time.xGame;
	LOCALPLAYER.flags |= PLAYER_FLAGS_INVULNERABLE;
	SetupSpherePulse (gameData.multiplayer.spherePulse + gameData.multiplayer.nLocalPlayer, 0.02f, 0.5f);
	}
if (!(gameData.app.nGameMode & GM_MULTI))
	BuddyMessage ("Nice job, %s!", LOCALPLAYER.callsign);
gameStates.gameplay.bFinalBossIsDead = 1;
}

extern int MultiAllPlayersAlive ();
void MultiSendFinishGame ();

//	------------------------------------------------------------------------------------------------------
//	Return 1 if robotP died, else return 0
int ApplyDamageToRobot (CObject *robotP, fix damage, int nKillerObj)
{
	char		bIsThief, bIsBoss;
	char		tempStolen [MAX_STOLEN_ITEMS];
	CObject	*killerObjP = (nKillerObj < 0) ? NULL : OBJECTS + nKillerObj;

if (robotP->info.nFlags & OF_EXPLODING)
	return 0;
if (robotP->info.xShields < 0)
	return 0;	//robotP already dead...
if (gameData.time.xGame - robotP->CreationTime () < F1_0)
	return 0;
if (!(gameStates.app.cheats.bRobotsKillRobots || EGI_FLAG (bRobotsHitRobots, 0, 0, 0))) {
	// guidebot may kill other bots
	if (killerObjP && (killerObjP->info.nType == OBJ_ROBOT) && !ROBOTINFO (killerObjP->info.nId).companion)
		return 0;
	}
if ((bIsBoss = ROBOTINFO (robotP->info.nId).bossFlag)) {
	int i = FindBoss (OBJ_IDX (robotP));
	if (i >= 0) {
		gameData.boss [i].nHitTime = gameData.time.xGame;
		gameData.boss [i].bHasBeenHit = 1;
		}
	}

//	Buddy invulnerable on level 24 so he can give you his important messages.  Bah.
//	Also invulnerable if his cheat for firing weapons is in effect.
if (ROBOTINFO (robotP->info.nId).companion) {
//		if ((gameData.missions.nCurrentMission == gameData.missions.nBuiltinMission && gameData.missions.nCurrentLevel == gameData.missions.nLastLevel) || gameStates.app.cheats.bMadBuddy)
	if ((gameData.missions.nCurrentMission == gameData.missions.nBuiltinMission) &&
		 (gameData.missions.nCurrentLevel == gameData.missions.nLastLevel))
		return 0;
	}
robotP->SetTimeLastHit (gameStates.app.nSDLTicks);
robotP->info.xShields -= damage;
//	Do unspeakable hacks to make sure CPlayerData doesn't die after killing boss.  Or before, sort of.
if (bIsBoss) {
	if ((gameData.missions.nCurrentMission == gameData.missions.nBuiltinMission) &&
		 (gameData.missions.nCurrentLevel == gameData.missions.nLastLevel) &&
		 (robotP->info.xShields < 0) && (extraGameInfo [0].nBossCount == 1)) {
		if (IsMultiGame) {
			if (!MultiAllPlayersAlive ()) // everyones gotta be alive
				robotP->info.xShields = 1;
			else {
				MultiSendFinishGame ();
				DoFinalBossHacks ();
				}
			}
		else {	// NOTE LINK TO ABOVE!!!
			if ((LOCALPLAYER.shields < 0) || gameStates.app.bPlayerIsDead)
				robotP->info.xShields = 1;		//	Sorry, we can't allow you to kill the final boss after you've died.  Rough luck.
			else
				DoFinalBossHacks ();
			}
		}
	}

if (robotP->info.xShields >= 0) {
	if (killerObjP == gameData.objs.consoleP)
		ExecObjTriggers (OBJ_IDX (robotP), 1);
	return 0;
	}
if (IsMultiGame) {
	bIsThief = (ROBOTINFO (robotP->info.nId).thief != 0);
	if (bIsThief)
		memcpy (tempStolen, gameData.thief.stolenItems, sizeof (*tempStolen) * MAX_STOLEN_ITEMS);
	if (!MultiExplodeRobotSub (OBJ_IDX (robotP), nKillerObj, ROBOTINFO (robotP->info.nId).thief)) 
		return 0;
	if (bIsThief)
		memcpy (gameData.thief.stolenItems, tempStolen, sizeof (*tempStolen) * MAX_STOLEN_ITEMS);
	MultiSendRobotExplode (OBJ_IDX (robotP), nKillerObj, ROBOTINFO (robotP->info.nId).thief);
	if (bIsThief)
		memset (gameData.thief.stolenItems, 255, sizeof (gameData.thief.stolenItems));
	return 1;
	}

if (nKillerObj >= 0) {
	LOCALPLAYER.numKillsLevel++;
	LOCALPLAYER.numKillsTotal++;
	}

if (bIsBoss)
	StartBossDeathSequence (robotP);	//DoReactorDestroyedStuff (NULL);
else if (ROBOTINFO (robotP->info.nId).bDeathRoll)
	StartRobotDeathSequence (robotP);	//DoReactorDestroyedStuff (NULL);
else {
	if (robotP->info.nId == SPECIAL_REACTOR_ROBOT)
		SpecialReactorStuff ();
	ExplodeObject (robotP, ROBOTINFO (robotP->info.nId).kamikaze ? 1 : STANDARD_EXPL_DELAY);		//	Kamikaze, explode right away, IN YOUR FACE!
	}
return 1;
}

//	------------------------------------------------------------------------------------------------------

int	nBuddyGaveHintCount = 5;
fix	xLastTimeBuddyGameHint = 0;

//	Return true if damage done to boss, else return false.
int DoBossWeaponCollision (CObject *robotP, CObject *weaponP, CFixVector *vHitPt)
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
		int i = FindBoss (OBJ_IDX (robotP));
		if (i >= 0) {
			if (bossProps [gameStates.app.bD1Mission][d2BossIndex].bSpewMore && (d_rand () > 16384) &&
				 (BossSpewRobot (robotP, vHitPt, -1, 0) != -1))
				gameData.boss [i].nLastGateTime = gameData.time.xGame - gameData.boss [i].nGateInterval - 1;	//	Force allowing spew of another bot.
			BossSpewRobot (robotP, vHitPt, -1, 0);
			}
		}
	}

if (bossProps [gameStates.app.bD1Mission][d2BossIndex].bInvulSpot) {
	fix			dot;
	CFixVector	tvec1;

	//	Boss only vulnerable in back.  See if hit there.
	tvec1 = *vHitPt - robotP->info.position.vPos;
	CFixVector::Normalize (tvec1);	//	Note, if BOSS_INVULNERABLE_DOT is close to F1_0 (in magnitude), then should probably use non-quick version.
	dot = CFixVector::Dot (tvec1, robotP->info.position.mOrient.FVec ());
#if TRACE
	console.printf (CON_DBG, "Boss hit vec dot = %7.3f \n", X2F (dot));
#endif
	if (dot > gameData.physics.xBossInvulDot) {
		short	nSegment;

		nSegment = FindSegByPos (*vHitPt, robotP->info.nSegment, 1, 0);
		DigiLinkSoundToPos (SOUND_WEAPON_HIT_DOOR, nSegment, 0, vHitPt, 0, F1_0);
		bDamage = 0;

		if (xLastTimeBuddyGameHint == 0)
			xLastTimeBuddyGameHint = d_rand ()*32 + F1_0*16;

		if (nBuddyGaveHintCount) {
			if (xLastTimeBuddyGameHint + F1_0*20 < gameData.time.xGame) {
				int	sval;

				nBuddyGaveHintCount--;
				xLastTimeBuddyGameHint = gameData.time.xGame;
				sval = (d_rand ()*4) >> 15;
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
		if (!WI_matter (weaponP->info.nId)) {
			short nClone = CreateObject (weaponP->info.nType, weaponP->info.nId, -1, weaponP->info.nSegment, weaponP->info.position.vPos,
												  weaponP->info.position.mOrient, weaponP->info.xSize, 
												  weaponP->info.controlType, weaponP->info.movementType, weaponP->info.renderType);
			if (nClone != -1) {
				CObject	*cloneP = OBJECTS + nClone;
				if (weaponP->info.renderType == RT_POLYOBJ) {
					cloneP->rType.polyObjInfo.nModel = gameData.weapons.info [cloneP->info.nId].nModel;
					cloneP->info.xSize = FixDiv (gameData.models.polyModels [cloneP->rType.polyObjInfo.nModel].rad, 
															gameData.weapons.info [cloneP->info.nId].po_len_to_width_ratio);
					}
				cloneP->mType.physInfo.thrust.SetZero();
				cloneP->mType.physInfo.mass = WI_mass (weaponP->info.nType);
				cloneP->mType.physInfo.drag = WI_drag (weaponP->info.nType);
				CFixVector vImpulse = *vHitPt - robotP->info.position.vPos;
				CFixVector::Normalize (vImpulse);
				CFixVector vWeapon = weaponP->mType.physInfo.velocity;
				fix speed = CFixVector::Normalize (vWeapon);
				vImpulse += vWeapon * (-F1_0 * 2);
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

	nSegment = FindSegByPos (*vHitPt, robotP->info.nSegment, 1, 0);
	DigiLinkSoundToPos (SOUND_WEAPON_HIT_DOOR, nSegment, 0, vHitPt, 0, F1_0);
	bDamage = 0;
	}
return bDamage;
}

//	------------------------------------------------------------------------------------------------------

int FindHitObject (CObject *objP, short nObject)
{
	short	*p = gameData.objs.nHitObjects + OBJ_IDX (objP) * MAX_HIT_OBJECTS;
	int	i;

for (i = objP->cType.laserInfo.nLastHitObj; i; i--, p++)
	if (*p == nObject)
		return 1;
return 0;
}

//	------------------------------------------------------------------------------------------------------

int AddHitObject (CObject *objP, short nObject)
{
	short	*p;
	int	i;

if (FindHitObject (objP, nObject))
	return -1;
p = gameData.objs.nHitObjects + OBJ_IDX (objP) * MAX_HIT_OBJECTS;
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

int CollideRobotAndWeapon (CObject *robotP, CObject *weaponP, CFixVector *vHitPt)
{
	int			bDamage = 1;
	int			bInvulBoss = 0;
	fix			nStrength = WI_strength (weaponP->info.nId, gameStates.app.nDifficultyLevel);
	tRobotInfo	*botInfoP = &ROBOTINFO (robotP->info.nId);
	tWeaponInfo *wInfoP = gameData.weapons.info + weaponP->info.nId;

#if DBG
if (OBJ_IDX (weaponP) == nDbgObj)
	nDbgObj = nDbgObj;
#endif
if (weaponP->info.nId == PROXMINE_ID) {
	if (!COMPETITION && EGI_FLAG (bSmokeGrenades, 0, 0, 0))
		return 1;
	}
else if (weaponP->info.nId == OMEGA_ID) {
	if (!OkToDoOmegaDamage (weaponP))
		return 1;
	}
if (botInfoP->bossFlag) {
	int i = FindBoss (OBJ_IDX (robotP));
	if (i >= 0)
		gameData.boss [i].nHitTime = gameData.time.xGame;
	if (botInfoP->bossFlag >= BOSS_D2) {
		bDamage = DoBossWeaponCollision (robotP, weaponP, vHitPt);
		bInvulBoss = !bDamage;
		}
	}

//	Put in at request of Jasen (and Adam) because the Buddy-Bot gets in their way.
//	MK has so much fun whacking his butt around the mine he never cared...
if ((weaponP->cType.laserInfo.parent.nType == OBJ_ROBOT) && !gameStates.app.cheats.bRobotsKillRobots)
	return 1;
if (botInfoP->companion && (weaponP->cType.laserInfo.parent.nType != OBJ_ROBOT))
	return 1;
CreateWeaponEffects (weaponP, 1);
if (weaponP->info.nId == EARTHSHAKER_ID)
	ShakerRockStuff ();
//	If a persistent weaponP hit robotP most recently, quick abort, else we cream the same robotP many times,
//	depending on frame rate.
if (weaponP->mType.physInfo.flags & PF_PERSISTENT) {
	if (AddHitObject (weaponP, OBJ_IDX (robotP)) < 0)
		return 1;
	}
if (weaponP->cType.laserInfo.parent.nSignature == robotP->info.nSignature)
	return 1;
//	Changed, 10/04/95, put out blobs based on skill level and power of weaponP doing damage.
//	Also, only a weaponP hit from a CPlayerData weaponP causes smart blobs.
if ((weaponP->cType.laserInfo.parent.nType == OBJ_PLAYER) && botInfoP->energyBlobs)
	if ((robotP->info.xShields > 0) && bIsEnergyWeapon [weaponP->info.nId]) {
		fix xProb = (gameStates.app.nDifficultyLevel+2) * min (weaponP->info.xShields, robotP->info.xShields);
		xProb = botInfoP->energyBlobs * xProb / (NDL * 32);
		int nBlobs = xProb >> 16;
		if (2 * d_rand () < (xProb & 0xffff))
			nBlobs++;
		if (nBlobs)
			CreateSmartChildren (robotP, nBlobs);
		}

	//	Note: If weaponP hits an invulnerable boss, it will still do badass damage, including to the boss,
	//	unless this is trapped elsewhere.
	if (WI_damage_radius (weaponP->info.nId)) {
		if (bInvulBoss) {			//don't make badass sound
			//this code copied from ExplodeBadassWeapon ()
			ObjectCreateBadassExplosion (weaponP, weaponP->info.nSegment, vHitPt,
							wInfoP->impact_size,
							wInfoP->robot_hit_vclip,
							nStrength,
							wInfoP->damage_radius,
							nStrength,
							weaponP->cType.laserInfo.parent.nObject);

			}
		else		//Normal badass explosion
			ExplodeBadassWeapon (weaponP, vHitPt);
		}
	if (((weaponP->cType.laserInfo.parent.nType == OBJ_PLAYER) ||
		 ((weaponP->cType.laserInfo.parent.nType == OBJ_ROBOT) &&
		  (gameStates.app.cheats.bRobotsKillRobots || EGI_FLAG (bRobotsHitRobots, 0, 0, 0)))) &&
		 !(robotP->info.nFlags & OF_EXPLODING)) {
		CObject *explObjP = NULL;
		if (weaponP->cType.laserInfo.parent.nObject == LOCALPLAYER.nObject) {
			CreateAwarenessEvent (weaponP, WEAPON_ROBOT_COLLISION);			// object "weaponP" can attract attention to CPlayerData
			if (USE_D1_AI)
				DoD1AIRobotHit (robotP, WEAPON_ROBOT_COLLISION);
			else
				DoAIRobotHit (robotP, WEAPON_ROBOT_COLLISION);
			}
	  	else
			MultiRobotRequestChange (robotP, OBJECTS [weaponP->cType.laserInfo.parent.nObject].info.nId);
		if (botInfoP->nExp1VClip > -1)
			explObjP = ObjectCreateExplosion (weaponP->info.nSegment, vHitPt, (3 * robotP->info.xSize) / 8, (ubyte) botInfoP->nExp1VClip);
		else if (gameData.weapons.info [weaponP->info.nId].robot_hit_vclip > -1)
			explObjP = ObjectCreateExplosion (weaponP->info.nSegment, vHitPt, wInfoP->impact_size, (ubyte) wInfoP->robot_hit_vclip);
		if (explObjP)
			AttachObject (robotP, explObjP);
		if (bDamage && (botInfoP->nExp1Sound > -1))
			DigiLinkSoundToPos (botInfoP->nExp1Sound, robotP->info.nSegment, 0, vHitPt, 0, F1_0);
		if (!(weaponP->info.nFlags & OF_HARMLESS)) {
			fix damage = bDamage ? FixMul (weaponP->info.xShields, weaponP->cType.laserInfo.xScale) : 0;
			//	Cut Gauss damage on bosses because it just breaks the game.  Bosses are so easy to
			//	hit, and missing a robotP is what prevents the Gauss from being game-breaking.
			if (weaponP->info.nId == GAUSS_ID) {
				if (botInfoP->bossFlag)
					damage = (damage * (2 * NDL - gameStates.app.nDifficultyLevel)) / (2 * NDL);
				}
			else if (!COMPETITION && gameStates.app.bHaveExtraGameInfo [IsMultiGame] && (weaponP->info.nId == FUSION_ID))
				damage *= extraGameInfo [IsMultiGame].nFusionRamp / 2;
			if (!ApplyDamageToRobot (robotP, damage, weaponP->cType.laserInfo.parent.nObject))
				BumpTwoObjects (robotP, weaponP, 0, vHitPt);		//only bump if not dead. no damage from bump
			else if (!gameStates.gameplay.bNoBotAI && (weaponP->cType.laserInfo.parent.nSignature == gameData.objs.consoleP->info.nSignature)) {
				AddPointsToScore (botInfoP->scoreValue);
				DetectEscortGoalAccomplished (OBJ_IDX (robotP));
				}
			}
		//	If Gauss Cannon, spin robotP.
		if (robotP && !(botInfoP->companion || botInfoP->bossFlag) && (weaponP->info.nId == GAUSS_ID)) {
			tAIStaticInfo	*aip = &robotP->cType.aiInfo;

			if (aip->SKIP_AI_COUNT * gameData.time.xFrame < F1_0) {
				aip->SKIP_AI_COUNT++;
				robotP->mType.physInfo.rotThrust[X] = FixMul ((d_rand () - 16384), gameData.time.xFrame * aip->SKIP_AI_COUNT);
				robotP->mType.physInfo.rotThrust[Y] = FixMul ((d_rand () - 16384), gameData.time.xFrame * aip->SKIP_AI_COUNT);
				robotP->mType.physInfo.rotThrust[Z] = FixMul ((d_rand () - 16384), gameData.time.xFrame * aip->SKIP_AI_COUNT);
				robotP->mType.physInfo.flags |= PF_USES_THRUST;
				}
			}
		}
MaybeKillWeapon (weaponP, robotP);
return 1;
}

//##void CollideRobotAndCamera (CObject *robotP, CObject *camera, CFixVector *vHitPt) {
//##	return;
//##}

//##void CollideRobotAndPowerup (CObject *robotP, CObject *powerup, CFixVector *vHitPt) {
//##	return;
//##}

//##void CollideRobotAndDebris (CObject *robotP, CObject *debris, CFixVector *vHitPt) {
//##	return;
//##}

//##void CollideHostageAndHostage (CObject *hostage1, CObject *hostage2, CFixVector *vHitPt) {
//##	return;
//##}

//	-----------------------------------------------------------------------------

int CollideHostageAndPlayer (CObject *hostage, CObject *CPlayerData, CFixVector *vHitPt)
{
	// Give CPlayerData points, etc.
if (CPlayerData == gameData.objs.consoleP) {
	DetectEscortGoalAccomplished (OBJ_IDX (hostage));
	AddPointsToScore (HOSTAGE_SCORE);
	// Do effect
	RescueHostage (hostage->info.nId);
	// Remove the hostage CObject.
	KillObject (hostage);
	if (gameData.app.nGameMode & GM_MULTI)
		MultiSendRemObj (OBJ_IDX (hostage));
	}
return 1;
}

//--unused-- void CollideHostageAndWeapon (CObject *hostage, CObject *weaponP, CFixVector *vHitPt)
//--unused-- {
//--unused-- 	//	Cannot kill hostages, as per Matt's edict!
//--unused-- 	//	 (A fine edict, but in contradiction to the milestone: "Robots attack hostages.")
//--unused-- 	hostage->info.xShields -= weaponP->info.xShields/2;
//--unused--
//--unused-- 	CreateAwarenessEvent (weaponP, WEAPON_ROBOT_COLLISION);			// CObject "weapon" can attract attention to CPlayerData
//--unused--
//--unused-- 	//PLAY_SOUND_3D (SOUND_HOSTAGE_KILLED, vHitPt, hostage->info.nSegment);
//--unused-- 	DigiLinkSoundToPos (SOUND_HOSTAGE_KILLED, hostage->info.nSegment , 0, vHitPt, 0, F1_0);
//--unused--
//--unused--
//--unused-- 	if (hostage->info.xShields <= 0) {
//--unused-- 		ExplodeObject (hostage, 0);
//--unused-- 		KillObject (hostage);
//--unused-- 	}
//--unused--
//--unused-- 	if (WI_damage_radius (weaponP->info.nId))
//--unused-- 		ExplodeBadassWeapon (weaponP);
//--unused--
//--unused-- 	MaybeKillWeapon (weaponP, hostage);
//--unused--
//--unused-- }

//##void CollideHostageAndCamera (CObject *hostage, CObject *camera, CFixVector *vHitPt) {
//##	return;
//##}

//##void CollideHostageAndPowerup (CObject *hostage, CObject *powerup, CFixVector *vHitPt) {
//##	return;
//##}

//##void CollideHostageAndDebris (CObject *hostage, CObject *debris, CFixVector *vHitPt) {
//##	return;
//##}

//	-----------------------------------------------------------------------------

int CollidePlayerAndPlayer (CObject *player1, CObject *player2, CFixVector *vHitPt)
{
if (gameStates.app.bD2XLevel &&
	 (SEGMENTS [player1->info.nSegment].m_nType == SEGMENT_IS_NODAMAGE))
	return 1;
if (BumpTwoObjects (player1, player2, 1, vHitPt))
	DigiLinkSoundToPos (SOUND_ROBOT_HIT_PLAYER, player1->info.nSegment, 0, vHitPt, 0, F1_0);
return 1;
}

// -- removed, 09/06/95, MK -- void destroyPrimaryWeapon (int weapon_index)
// -- removed, 09/06/95, MK -- {
// -- removed, 09/06/95, MK -- 	if (weapon_index == MAX_PRIMARY_WEAPONS) {
// -- removed, 09/06/95, MK -- 		HUDInitMessage ("Quad lasers destroyed!");
// -- removed, 09/06/95, MK -- 		LOCALPLAYER.flags &= ~PLAYER_FLAGS_QUAD_LASERS;
// -- removed, 09/06/95, MK -- 		update_laserWeapon_info ();
// -- removed, 09/06/95, MK -- 	} else if (weapon_index == 0) {
// -- removed, 09/06/95, MK -- 		Assert (LOCALPLAYER.laserLevel > 0);
// -- removed, 09/06/95, MK -- 		HUDInitMessage ("%s degraded!", Text_string [104+weapon_index]);		//	Danger!Danger!Use of literal! Danger!
// -- removed, 09/06/95, MK -- 		LOCALPLAYER.laserLevel--;
// -- removed, 09/06/95, MK -- 		update_laserWeapon_info ();
// -- removed, 09/06/95, MK -- 	} else {
// -- removed, 09/06/95, MK -- 		HUDInitMessage ("%s destroyed!", Text_string [104+weapon_index]);		//	Danger!Danger!Use of literal! Danger!
// -- removed, 09/06/95, MK -- 		LOCALPLAYER.primaryWeaponFlags &= ~ (1 << weapon_index);
// -- removed, 09/06/95, MK -- 		AutoSelectWeapon (0);
// -- removed, 09/06/95, MK -- 	}
// -- removed, 09/06/95, MK --
// -- removed, 09/06/95, MK -- }
// -- removed, 09/06/95, MK --
// -- removed, 09/06/95, MK -- void destroySecondaryWeapon (int weapon_index)
// -- removed, 09/06/95, MK -- {
// -- removed, 09/06/95, MK -- 	if (LOCALPLAYER.secondaryAmmo <= 0)
// -- removed, 09/06/95, MK -- 		return;
// -- removed, 09/06/95, MK --
// -- removed, 09/06/95, MK -- 	HUDInitMessage ("%s destroyed!", Text_string [114+weapon_index]);		//	Danger!Danger!Use of literal! Danger!
// -- removed, 09/06/95, MK -- 	if (--LOCALPLAYER.secondaryAmmo [weapon_index] == 0)
// -- removed, 09/06/95, MK -- 		AutoSelectWeapon (1);
// -- removed, 09/06/95, MK --
// -- removed, 09/06/95, MK -- }
// -- removed, 09/06/95, MK --
// -- removed, 09/06/95, MK -- #define	LOSE_WEAPON_THRESHOLD	 (F1_0*30)

//	-----------------------------------------------------------------------------

void ApplyDamageToPlayer (CObject *playerObjP, CObject *killerObjP, fix damage)
{
CPlayerData *playerP = gameData.multiplayer.players + playerObjP->info.nId;
CPlayerData *killerP = (killerObjP && (killerObjP->info.nType == OBJ_PLAYER)) ? gameData.multiplayer.players + killerObjP->info.nId : NULL;
if (gameStates.app.bPlayerIsDead)
	return;

if (gameStates.app.bD2XLevel && (SEGMENTS [playerObjP->info.nSegment].m_nType == SEGMENT_IS_NODAMAGE))
	return;
if (LOCALPLAYER.flags & PLAYER_FLAGS_INVULNERABLE)
	return;
if (killerObjP && (killerObjP->info.nType == OBJ_ROBOT) && ROBOTINFO (killerObjP->info.nId).companion)
	return;
if (killerObjP == playerObjP) {
	if (!COMPETITION && gameStates.app.bHaveExtraGameInfo [1] && extraGameInfo [1].bInhibitSuicide)
		return;
	}
else if (killerP && gameStates.app.bHaveExtraGameInfo [1] &&
			!(COMPETITION || extraGameInfo [1].bFriendlyFire)) {
	if (gameData.app.nGameMode & GM_TEAM) {
		if (GetTeam (playerObjP->info.nId) == GetTeam (killerObjP->info.nId))
			return;
		}
	else if (gameData.app.nGameMode & GM_MULTI_COOP)
		return;
	}
if (gameStates.app.bEndLevelSequence)
	return;

gameData.multiplayer.bWasHit [playerObjP->info.nId] = -1;
//for the CPlayerData, the 'real' shields are maintained in the gameData.multiplayer.players []
//array.  The shields value in the CPlayerData's CObject are, I think, not
//used anywhere.  This routine, however, sets the OBJECTS shields to
//be a mirror of the value in the Player structure.

if (playerObjP->info.nId == gameData.multiplayer.nLocalPlayer) {		//is this the local CPlayerData?
	if ((gameData.app.nGameMode & GM_ENTROPY) && extraGameInfo [1].entropy.bPlayerHandicap && killerP) {
		double h = (double) playerP->netKillsTotal / (double) (killerP->netKillsTotal + 1);
		if (h < 0.5)
			h = 0.5;
		else if (h > 1.0)
			h = 1.0;
		if (!(damage = (fix) ((double) damage * h)))
			damage = 1;
		}
	playerP->shields -= damage;
	MultiSendShields ();
	paletteManager.BumpEffect (X2I (damage)*4, -X2I (damage/2), -X2I (damage/2));	//flash red
	if (playerP->shields < 0)	{
  		playerP->nKillerObj = OBJ_IDX (killerObjP);
		KillObject (playerObjP);
		if (gameData.escort.nObjNum != -1)
			if (killerObjP && (killerObjP->info.nType == OBJ_ROBOT) && (ROBOTINFO (killerObjP->info.nId).companion))
				gameData.escort.xSorryTime = gameData.time.xGame;
		}
	playerObjP->info.xShields = playerP->shields;		//mirror
	}
}

//	-----------------------------------------------------------------------------

int CollidePlayerAndWeapon (CObject *playerObjP, CObject *weaponP, CFixVector *vHitPt)
{
	fix		damage = weaponP->info.xShields;
	CObject *killer = NULL;

	//	In multiplayer games, only do damage to another CPlayerData if in first frame.
	//	This is necessary because in multiplayer, due to varying framerates, omega blobs actually
	//	have a bit of a lifetime.  But they start out with a lifetime of ONE_FRAME_TIME, and this
	//	gets bashed to 1/4 second in laser_doWeapon_sequence.  This bashing occurs for visual purposes only.
if (gameStates.app.bD2XLevel && (SEGMENTS [playerObjP->info.nSegment].m_nType == SEGMENT_IS_NODAMAGE))
	return 1;
if ((weaponP->info.nId == PROXMINE_ID) && !COMPETITION && EGI_FLAG (bSmokeGrenades, 0, 0, 0))
	return 1;
if (weaponP->info.nId == OMEGA_ID)
	if (!OkToDoOmegaDamage (weaponP))
		return 1;
//	Don't Collide own smart mines unless direct hit.
if ((weaponP->info.nId == SMARTMINE_ID) &&
	 (OBJ_IDX (playerObjP) == weaponP->cType.laserInfo.parent.nObject) &&
	 (CFixVector::Dist (*vHitPt, playerObjP->info.position.vPos) > playerObjP->info.xSize))
	return 1;
gameData.multiplayer.bWasHit [playerObjP->info.nId] = -1;
CreateWeaponEffects (weaponP, 1);
if (weaponP->info.nId == EARTHSHAKER_ID)
	ShakerRockStuff ();
damage = FixMul (damage, weaponP->cType.laserInfo.xScale);
if (!COMPETITION && gameStates.app.bHaveExtraGameInfo [IsMultiGame] && (weaponP->info.nId == FUSION_ID))
	damage *= extraGameInfo [IsMultiGame].nFusionRamp / 2;
if (IsMultiGame)
	damage = FixMul (damage, gameData.weapons.info [weaponP->info.nId].multi_damage_scale);
if (weaponP->mType.physInfo.flags & PF_PERSISTENT) {
	if (AddHitObject (weaponP, OBJ_IDX (playerObjP)) < 0)
		return 1;
}
if (playerObjP->info.nId == gameData.multiplayer.nLocalPlayer) {
	if (!(LOCALPLAYER.flags & PLAYER_FLAGS_INVULNERABLE)) {
		DigiLinkSoundToPos (SOUND_PLAYER_GOT_HIT, playerObjP->info.nSegment, 0, vHitPt, 0, F1_0);
		if (IsMultiGame)
			MultiSendPlaySound (SOUND_PLAYER_GOT_HIT, F1_0);
		}
	else {
		DigiLinkSoundToPos (SOUND_WEAPON_HIT_DOOR, playerObjP->info.nSegment, 0, vHitPt, 0, F1_0);
		if (gameData.app.nGameMode & GM_MULTI)
			MultiSendPlaySound (SOUND_WEAPON_HIT_DOOR, F1_0);
		}
	}
ObjectCreateExplosion (playerObjP->info.nSegment, vHitPt, I2X (10)/2, VCLIP_PLAYER_HIT);
if (WI_damage_radius (weaponP->info.nId))
	ExplodeBadassWeapon (weaponP, vHitPt);
MaybeKillWeapon (weaponP, playerObjP);
BumpTwoObjects (playerObjP, weaponP, 0, vHitPt);	//no damage from bump
if (!WI_damage_radius (weaponP->info.nId)) {
	if (weaponP->cType.laserInfo.parent.nObject > -1)
		killer = OBJECTS + weaponP->cType.laserInfo.parent.nObject;
	if (!(weaponP->info.nFlags & OF_HARMLESS))
		ApplyDamageToPlayer (playerObjP, killer, damage);
}
//	Robots become aware of you if you get hit.
AIDoCloakStuff ();
return 1;
}

//	-----------------------------------------------------------------------------
//	Nasty robots are the ones that attack you by running into you and doing lots of damage.
int CollidePlayerAndNastyRobot (CObject *playerObjP, CObject *robotP, CFixVector *vHitPt)
{
//	if (!(ROBOTINFO (objP->info.nId).energyDrain && gameData.multiplayer.players [playerObjP->info.nId].energy))
ObjectCreateExplosion (playerObjP->info.nSegment, vHitPt, I2X (10) / 2, VCLIP_PLAYER_HIT);
if (BumpTwoObjects (playerObjP, robotP, 0, vHitPt))	{//no damage from bump
	DigiLinkSoundToPos (ROBOTINFO (robotP->info.nId).clawSound, playerObjP->info.nSegment, 0, vHitPt, 0, F1_0);
	ApplyDamageToPlayer (playerObjP, robotP, F1_0* (gameStates.app.nDifficultyLevel+1));
	}
return 1;
}

//	-----------------------------------------------------------------------------

int CollidePlayerAndMatCen (CObject *objP)
{
	CFixVector vExitDir;

DigiLinkSoundToPos (SOUND_PLAYER_GOT_HIT, objP->info.nSegment, 0, &objP->info.position.vPos, 0, F1_0);
//	DigiPlaySample (SOUND_PLAYER_GOT_HIT, F1_0);
ObjectCreateExplosion (objP->info.nSegment, &objP->info.position.vPos, I2X (10)/2, VCLIP_PLAYER_HIT);
if (objP->info.nId != gameData.multiplayer.nLocalPlayer)
	return 1;
CSegment* segP = SEGMENTS + objP->info.nSegment;
for (short nSide = 0; nSide < MAX_SIDES_PER_SEGMENT; nSide++)
	if (segP->IsDoorWay (nSide, objP) & WID_FLY_FLAG) {
		vExitDir = segP->SideCenter (nSide) - objP->info.position.vPos;
		CFixVector::Normalize (vExitDir);
		CFixVector vRand = CFixVector::Random();
		vRand [X] /= 4;
		vRand [Y] /= 4;
		vRand [Z] /= 4;
		vExitDir += vRand;
		CFixVector::Normalize (vExitDir);
		}
BumpOneObject (objP, &vExitDir, 64*F1_0);
ApplyDamageToPlayer (objP, objP, 4*F1_0);	//	Changed, MK, 2/19/96, make killer the CPlayerData, so if you die in matcen, will say you killed yourself
return 1;
}

//	-----------------------------------------------------------------------------

int CollideRobotAndMatCen (CObject *objP)
{
	CFixVector	vExitDir;
	CSegment*	segP = SEGMENTS + objP->info.nSegment;

DigiLinkSoundToPos (SOUND_ROBOT_HIT, objP->info.nSegment, 0, &objP->info.position.vPos, 0, F1_0);
//	DigiPlaySample (SOUND_ROBOT_HIT, F1_0);

if (ROBOTINFO (objP->info.nId).nExp1VClip > -1)
	ObjectCreateExplosion ((short) objP->info.nSegment, &objP->info.position.vPos, (objP->info.xSize/2*3)/4, (ubyte) ROBOTINFO (objP->info.nId).nExp1VClip);
for (short nSide = 0; nSide < MAX_SIDES_PER_SEGMENT; nSide++)
	if (segP->IsDoorWay (nSide, NULL) & WID_FLY_FLAG) {
		vExitDir = segP->SideCenter (nSide) - objP->info.position.vPos;
		CFixVector::Normalize (vExitDir);
		}
BumpOneObject (objP, &vExitDir, 8*F1_0);
ApplyDamageToRobot (objP, F1_0, -1);
return 1;
}

//##void CollidePlayerAndCamera (CObject *playerObjP, CObject *camera, CFixVector *vHitPt) {
//##	return;
//##}

//	-----------------------------------------------------------------------------

int CollidePlayerAndPowerup (CObject *playerObjP, CObject *powerupP, CFixVector *vHitPt)
{
if (!gameStates.app.bEndLevelSequence && !gameStates.app.bPlayerIsDead &&
	(playerObjP->info.nId == gameData.multiplayer.nLocalPlayer)) {
	int bPowerupUsed = DoPowerup (powerupP, playerObjP->info.nId);
	if (bPowerupUsed) {
		KillObject (powerupP);
		if (IsMultiGame)
			MultiSendRemObj (OBJ_IDX (powerupP));
		}
	}
else if (IsCoopGame && (playerObjP->info.nId != gameData.multiplayer.nLocalPlayer)) {
	switch (powerupP->info.nId) {
		case POW_KEY_BLUE:
			gameData.multiplayer.players [playerObjP->info.nId].flags |= PLAYER_FLAGS_BLUE_KEY;
			break;
		case POW_KEY_RED:
			gameData.multiplayer.players [playerObjP->info.nId].flags |= PLAYER_FLAGS_RED_KEY;
			break;
		case POW_KEY_GOLD:
			gameData.multiplayer.players [playerObjP->info.nId].flags |= PLAYER_FLAGS_GOLD_KEY;
			break;
		default:
			break;
		}
	}
DetectEscortGoalAccomplished (OBJ_IDX (powerupP));
return 1;
}

//	-----------------------------------------------------------------------------

int CollidePlayerAndMonsterball (CObject *playerObjP, CObject *monsterball, CFixVector *vHitPt)
{
if (!gameStates.app.bEndLevelSequence && !gameStates.app.bPlayerIsDead &&
	(playerObjP->info.nId == gameData.multiplayer.nLocalPlayer)) {
	if (BumpTwoObjects (playerObjP, monsterball, 0, vHitPt))
		DigiLinkSoundToPos (SOUND_ROBOT_HIT_PLAYER, playerObjP->info.nSegment, 0, vHitPt, 0, F1_0);
	}
return 1;
}

//	-----------------------------------------------------------------------------
//##void CollidePlayerAndDebris (CObject *playerObjP, CObject *debris, CFixVector *vHitPt) {
//##	return;
//##}

int CollideActorAndClutter (CObject *actor, CObject *clutter, CFixVector *vHitPt)
{
if (gameStates.app.bD2XLevel &&
	 (SEGMENTS [actor->info.nSegment].m_nType == SEGMENT_IS_NODAMAGE))
	return 1;
if (!(actor->info.nFlags & OF_EXPLODING) && BumpTwoObjects (clutter, actor, 1, vHitPt))
	DigiLinkSoundToPos (SOUND_ROBOT_HIT_PLAYER, actor->info.nSegment, 0, vHitPt, 0, F1_0);
return 1;
}

//	-----------------------------------------------------------------------------
//	See if weapon1 creates a badass explosion.  If so, create the explosion
//	Return true if weapon does proximity (as opposed to only contact) damage when it explodes.
int MaybeDetonateWeapon (CObject *weapon1, CObject *weapon2, CFixVector *vHitPt)
{
	fix	dist;

if (!gameData.weapons.info [weapon1->info.nId].damage_radius)
	return 0;

dist = CFixVector::Dist (weapon1->info.position.vPos, weapon2->info.position.vPos);
if (dist >= F1_0*5)
	weapon1->info.xLifeLeft = min (dist/64, F1_0);
else {
	MaybeKillWeapon (weapon1, weapon2);
	if (weapon1->info.nFlags & OF_SHOULD_BE_DEAD) {
		CreateWeaponEffects (weapon1, 0);
		ExplodeBadassWeapon (weapon1, vHitPt);
		DigiLinkSoundToPos (gameData.weapons.info [weapon1->info.nId].robot_hitSound, weapon1->info.nSegment , 0, vHitPt, 0, F1_0);
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
if (!gameData.objs.bIsMissile [nTarget])
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

int CollideWeaponAndWeapon (CObject *weapon1, CObject *weapon2, CFixVector *vHitPt)
{
	int	id1 = weapon1->info.nId;
	int	id2 = weapon2->info.nId;
	int	bKill1, bKill2;

if (id1 == SMALLMINE_ID && id2 == SMALLMINE_ID)
	return 1;		//these can't blow each other up
if ((id1 == PROXMINE_ID || id2 == PROXMINE_ID) && !COMPETITION && EGI_FLAG (bSmokeGrenades, 0, 0, 0))
	return 1;
if (((id1 == OMEGA_ID) && !OkToDoOmegaDamage (weapon1)) ||
    ((id2 == OMEGA_ID) && !OkToDoOmegaDamage (weapon2)))
	return 1;
bKill1 = DestroyWeapon (id1, id2);
bKill2 = DestroyWeapon (id2, id1);
if (bKill1 || bKill2) {
	//	Bug reported by Adam Q. Pletcher on September 9, 1994, smart bomb homing missiles were toasting each other.
	if ((id1 == id2) && (weapon1->cType.laserInfo.parent.nObject == weapon2->cType.laserInfo.parent.nObject))
		return 1;
	if (bKill1)
		if (MaybeDetonateWeapon (weapon1, weapon2, vHitPt))
			MaybeDetonateWeapon (weapon2, weapon1, vHitPt);
	if (bKill2)
		if (MaybeDetonateWeapon (weapon2, weapon1, vHitPt))
			MaybeDetonateWeapon (weapon1, weapon2, vHitPt);
	}
return 1;
}

//	-----------------------------------------------------------------------------

int CollideWeaponAndMonsterball (CObject *weaponP, CObject *powerup, CFixVector *vHitPt)
{
if (weaponP->cType.laserInfo.parent.nType == OBJ_PLAYER) {
	DigiLinkSoundToPos (SOUND_ROBOT_HIT, weaponP->info.nSegment , 0, vHitPt, 0, F1_0);
	if (weaponP->info.nId == EARTHSHAKER_ID)
		ShakerRockStuff ();
	if (weaponP->mType.physInfo.flags & PF_PERSISTENT) {
		if (AddHitObject (weaponP, OBJ_IDX (powerup)) < 0)
			return 1;
		}
	ObjectCreateExplosion (powerup->info.nSegment, vHitPt, I2X (10)/2, VCLIP_PLAYER_HIT);
	if (WI_damage_radius (weaponP->info.nId))
		ExplodeBadassWeapon (weaponP, vHitPt);
	MaybeKillWeapon (weaponP, powerup);
	BumpTwoObjects (weaponP, powerup, 1, vHitPt);
	}
return 1;
}

//	-----------------------------------------------------------------------------
//##void CollideWeaponAndCamera (CObject *weaponP, CObject *camera, CFixVector *vHitPt) {
//##	return;
//##}

//	-----------------------------------------------------------------------------

int CollideWeaponAndDebris (CObject *weaponP, CObject *debris, CFixVector *vHitPt)
{
//	Hack! Prevent debris from causing bombs spewed at CPlayerData death to detonate!
if (WeaponIsMine (weaponP->info.nId)) {
	if (weaponP->cType.laserInfo.xCreationTime + F1_0/2 > gameData.time.xGame)
		return 1;
	}
if ((weaponP->cType.laserInfo.parent.nType==OBJ_PLAYER) && !(debris->info.nFlags & OF_EXPLODING))	{
	DigiLinkSoundToPos (SOUND_ROBOT_HIT, weaponP->info.nSegment , 0, vHitPt, 0, F1_0);
	ExplodeObject (debris, 0);
	if (WI_damage_radius (weaponP->info.nId))
		ExplodeBadassWeapon (weaponP, vHitPt);
	MaybeKillWeapon (weaponP, debris);
	KillObject (weaponP);
	}
return 1;
}

//##void CollideCameraAndCamera (CObject *camera1, CObject *camera2, CFixVector *vHitPt) {
//##	return;
//##}

//##void CollideCameraAndPowerup (CObject *camera, CObject *powerup, CFixVector *vHitPt) {
//##	return;
//##}

//##void CollideCameraAndDebris (CObject *camera, CObject *debris, CFixVector *vHitPt) {
//##	return;
//##}

//##void CollidePowerupAndPowerup (CObject *powerup1, CObject *powerup2, CFixVector *vHitPt) {
//##	return;
//##}

//##void CollidePowerupAndDebris (CObject *powerup, CObject *debris, CFixVector *vHitPt) {
//##	return;
//##}

//##void CollideDebrisAndDebris (CObject *debris1, CObject *debris2, CFixVector *vHitPt) {
//##	return;
//##}


/* DPH: Put these macros on one long line to avoid CR/LF problems on linux */
#define	COLLISION_OF(a, b) (((a)<<8) + (b))

#define	DO_COLLISION(type1, type2, collisionHandler) \
			case COLLISION_OF ((type1), (type2)): \
				return (collisionHandler) ((A), (B), vHitPt); \
			case COLLISION_OF ((type2), (type1)): \
				return (collisionHandler) ((B), (A), vHitPt);

#define	DO_SAME_COLLISION(type1, type2, collisionHandler) \
				case COLLISION_OF ((type1), (type1)): \
					return (collisionHandler) ((A), (B), vHitPt);

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

int CollideTwoObjects (CObject *A, CObject *B, CFixVector *vHitPt)
{
	int collisionType = COLLISION_OF (A->info.nType, B->info.nType);

switch (collisionType)	{
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
	NO_COLLISION		(OBJ_EXPLOSION, OBJ_ROBOT, 		CollideFireballAndRobot)
	NO_COLLISION		(OBJ_EXPLOSION, OBJ_HOSTAGE, 	CollideFireballAndHostage)
	NO_COLLISION 		(OBJ_EXPLOSION, OBJ_PLAYER, 	CollideFireballAndPlayer)
	NO_COLLISION 		(OBJ_EXPLOSION, OBJ_WEAPON, 	CollideFireballAndWeapon)
	NO_COLLISION		(OBJ_EXPLOSION, OBJ_CAMERA, 	CollideFireballAndCamera)
	NO_COLLISION		(OBJ_EXPLOSION, OBJ_POWERUP, 	CollideFireballAndPowerup)
	NO_COLLISION		(OBJ_EXPLOSION, OBJ_DEBRIS, 	CollideFireballAndDebris)
	NO_COLLISION		(OBJ_ROBOT, 	OBJ_HOSTAGE, 	CollideRobotAndHostage)
	DO_COLLISION		(OBJ_ROBOT, 	OBJ_PLAYER, 	CollideRobotAndPlayer)
	DO_COLLISION		(OBJ_ROBOT, 	OBJ_WEAPON, 	CollideRobotAndWeapon)
	NO_COLLISION		(OBJ_ROBOT, 	OBJ_CAMERA, 	CollideRobotAndCamera)
	NO_COLLISION		(OBJ_ROBOT, 	OBJ_POWERUP, 	CollideRobotAndPowerup)
	DO_COLLISION		(OBJ_ROBOT, 	OBJ_DEBRIS, 	CollideActorAndClutter)
	DO_COLLISION		(OBJ_ROBOT, 	OBJ_REACTOR, 	CollideRobotAndReactor)
	DO_COLLISION		(OBJ_HOSTAGE, 	OBJ_PLAYER, 	CollideHostageAndPlayer)
	NO_COLLISION		(OBJ_HOSTAGE, 	OBJ_WEAPON, 	CollideHostageAndWeapon)
	NO_COLLISION		(OBJ_HOSTAGE, 	OBJ_CAMERA, 	CollideHostageAndCamera)
	NO_COLLISION		(OBJ_HOSTAGE, 	OBJ_POWERUP, 	CollideHostageAndPowerup)
	NO_COLLISION		(OBJ_HOSTAGE, 	OBJ_DEBRIS, 	CollideHostageAndDebris)
	DO_COLLISION		(OBJ_PLAYER, 	OBJ_WEAPON, 	CollidePlayerAndWeapon)
	NO_COLLISION		(OBJ_PLAYER, 	OBJ_CAMERA, 	CollidePlayerAndCamera)
	DO_COLLISION		(OBJ_PLAYER, 	OBJ_POWERUP, 	CollidePlayerAndPowerup)
	DO_COLLISION		(OBJ_PLAYER, 	OBJ_DEBRIS, 	CollideActorAndClutter)
	DO_COLLISION		(OBJ_PLAYER, 	OBJ_REACTOR, 	CollidePlayerAndReactor)
	DO_COLLISION		(OBJ_PLAYER, 	OBJ_CLUTTER, 	CollideActorAndClutter)
	DO_COLLISION		(OBJ_PLAYER, 	OBJ_MONSTERBALL, 	CollidePlayerAndMonsterball)
	NO_COLLISION		(OBJ_WEAPON, 	OBJ_CAMERA, 	CollideWeaponAndCamera)
	NO_COLLISION		(OBJ_WEAPON, 	OBJ_POWERUP, 	CollideWeaponAndPowerup)
	DO_COLLISION		(OBJ_WEAPON, 	OBJ_DEBRIS, 	CollideWeaponAndDebris)
	DO_COLLISION		(OBJ_WEAPON, 	OBJ_REACTOR, 	CollideWeaponAndReactor)
	DO_COLLISION		(OBJ_WEAPON, 	OBJ_CLUTTER, 	CollideWeaponAndClutter)
	DO_COLLISION		(OBJ_WEAPON, 	OBJ_MONSTERBALL, 	CollideWeaponAndMonsterball)
	NO_COLLISION		(OBJ_CAMERA, 	OBJ_POWERUP, 	CollideCameraAndPowerup)
	NO_COLLISION		(OBJ_CAMERA, 	OBJ_DEBRIS, 	CollideCameraAndDebris)
	NO_COLLISION		(OBJ_POWERUP, 	OBJ_DEBRIS, 	CollidePowerupAndDebris)

	DO_COLLISION		(OBJ_MARKER, 	OBJ_PLAYER, 	CollidePlayerAndMarker)
	NO_COLLISION		(OBJ_MARKER, 	OBJ_ROBOT, 		NULL)
	NO_COLLISION		(OBJ_MARKER, 	OBJ_HOSTAGE, 	NULL)
	NO_COLLISION		(OBJ_MARKER, 	OBJ_WEAPON, 	NULL)
	NO_COLLISION		(OBJ_MARKER, 	OBJ_CAMERA, 	NULL)
	NO_COLLISION		(OBJ_MARKER, 	OBJ_POWERUP, 	NULL)
	NO_COLLISION		(OBJ_MARKER, 	OBJ_DEBRIS, 	NULL)

	DO_COLLISION		(OBJ_CAMBOT, 	OBJ_WEAPON, 	CollideRobotAndWeapon)
	DO_COLLISION		(OBJ_CAMBOT, 	OBJ_PLAYER, 	CollideRobotAndPlayer)
	NO_COLLISION		(OBJ_FIREBALL, OBJ_CAMBOT, 	CollideFireballAndRobot)
	default:
		Int3 ();	//Error ("Unhandled collisionType in Collide.c! \n");
	}
return 1;
}

//	-----------------------------------------------------------------------------

void CollideInit ()
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

int CollideObjectWithWall (CObject *objP, fix xHitSpeed, short nHitSeg, short nHitWall, CFixVector * vHitPt)
{
switch (objP->info.nType)	{
	case OBJ_NONE:
		Error ("An object of type NONE hit a wall! \n");
		break;
	case OBJ_PLAYER:
		CollidePlayerAndWall (objP, xHitSpeed, nHitSeg, nHitWall, vHitPt);
		break;
	case OBJ_WEAPON:
		CollideWeaponAndWall (objP, xHitSpeed, nHitSeg, nHitWall, vHitPt);
		break;
	case OBJ_DEBRIS:
		CollideDebrisAndWall (objP, xHitSpeed, nHitSeg, nHitWall, vHitPt);
		break;
	case OBJ_FIREBALL:
		break;	//CollideFireballAndWall (objP, xHitSpeed, nHitSeg, nHitWall, vHitPt);
	case OBJ_ROBOT:
		CollideRobotAndWall (objP, xHitSpeed, nHitSeg, nHitWall, vHitPt);
		break;
	case OBJ_HOSTAGE:
		break;	//CollideHostageAndWall (objP, xHitSpeed, nHitSeg, nHitWall, vHitPt);
	case OBJ_CAMERA:
		break;	//CollideCameraAndWall (objP, xHitSpeed, nHitSeg, nHitWall, vHitPt);
	case OBJ_EFFECT:
		break;	//CollideSmokeAndWall (objP, xHitSpeed, nHitSeg, nHitWall, vHitPt);
	case OBJ_POWERUP:
		break;	//CollidePowerupAndWall (objP, xHitSpeed, nHitSeg, nHitWall, vHitPt);
	case OBJ_GHOST:
		break;	//do nothing
	case OBJ_MONSTERBALL:
#if DBG
		objP = objP;
#endif
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
