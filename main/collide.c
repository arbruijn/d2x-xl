/* $Id: Collide.c, v 1.12 2003/03/27 01:23:18 btb Exp $ */
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
#include <math.h>

#include "rle.h"
#include "inferno.h"
#include "game.h"
#include "gr.h"
#include "stdlib.h"
#include "bm.h"
//#include "error.h"
#include "mono.h"
#include "3d.h"
#include "segment.h"
#include "texmap.h"
#include "laser.h"
#include "key.h"
#include "gameseg.h"
#include "object.h"
#include "physics.h"
#include "slew.h"		
#include "render.h"
#include "wall.h"
#include "vclip.h"
#include "polyobj.h"
#include "fireball.h"
#include "laser.h"
#include "error.h"
#include "ai.h"
#include "hostage.h"
#include "fuelcen.h"
#include "sounds.h"
#include "robot.h"
#include "weapon.h"
#include "player.h"
#include "gauges.h"
#include "powerup.h"
#include "network.h"
#include "newmenu.h"
#include "scores.h"
#include "effects.h"
#include "textures.h"
#include "multi.h"
#include "cntrlcen.h"
#include "fireball.h"
#include "newdemo.h"
#include "endlevel.h"
#include "multibot.h"
#include "piggy.h"
#include "text.h"
#include "automap.h"
#include "switch.h"
#include "palette.h"
#include "object.h"
#include "sphere.h"
#include "text.h"

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

//##void CollideFireballAndWall (tObject *fireball, fix hitspeed, short hitseg, short hitwall, vmsVector * vHitPt)	{
//##	return; 
//##}

//	-------------------------------------------------------------------------------------------------------------
//	The only reason this routine is called (as of 10/12/94) is so Brain guys can open doors.
void CollideRobotAndWall (tObject * robot, fix hitspeed, short hitseg, short hitwall, vmsVector * vHitPt)
{
	tAILocal		*ailp = gameData.ai.localInfo + OBJ_IDX (robot);
	tRobotInfo	*botInfoP = &ROBOTINFO (robot->id);

if ((robot->id == ROBOT_BRAIN) || 
	 (robot->cType.aiInfo.behavior == AIB_RUN_FROM) || 
	 botInfoP->companion || 
	 (robot->cType.aiInfo.behavior == AIB_SNIPE)) {
	int	nWall = WallNumI (hitseg, hitwall);
	if (nWall != -1) {
		tWall *wallP = gameData.walls.walls + nWall;
		if ((wallP->nType == WALL_DOOR) &&
			 (wallP->keys == KEY_NONE) && 
			 (wallP->state == WALL_DOOR_CLOSED) && 
			 !(wallP->flags & WALL_DOOR_LOCKED)) {
			WallOpenDoor (gameData.segs.segments + hitseg, hitwall);
		// -- Changed from this, 10/19/95, MK: Don't want buddy getting stranded from tPlayer
		//-- } else if (botInfoP->companion && (gameData.walls.walls [nWall].nType == WALL_DOOR) && (gameData.walls.walls [nWall].keys != KEY_NONE) && (gameData.walls.walls [nWall].state == WALL_DOOR_CLOSED) && !(gameData.walls.walls [nWall].flags & WALL_DOOR_LOCKED)) {
			} 
		else if (botInfoP->companion && (wallP->nType == WALL_DOOR)) {
			if ((ailp->mode == AIM_GOTO_PLAYER) || (gameData.escort.nSpecialGoal == ESCORT_GOAL_SCRAM)) {
				if (wallP->keys != KEY_NONE) {
					if (wallP->keys & LOCALPLAYER.flags)
						WallOpenDoor (gameData.segs.segments + hitseg, hitwall);
					} 
				else if (!(wallP->flags & WALL_DOOR_LOCKED))
					WallOpenDoor (gameData.segs.segments + hitseg, hitwall);
				}
			} 
		else if (botInfoP->thief) {		//	Thief allowed to go through doors to which tPlayer has key.
			if (wallP->keys != KEY_NONE)
				if (wallP->keys & LOCALPLAYER.flags)
					WallOpenDoor (gameData.segs.segments + hitseg, hitwall);
			}
		}
	}

	return;
}

//##void CollideHostageAndWall (tObject * hostage, fix hitspeed, short hitseg, short hitwall, vmsVector * vHitPt)	{
//##	return;
//##}

//	-------------------------------------------------------------------------------------------------------------

int ApplyDamageToClutter (tObject *clutter, fix damage)
{
if (clutter->flags & OF_EXPLODING)
	return 0;
if (clutter->shields < 0) 
	return 0;	//clutter already dead...
clutter->shields -= damage;
if (clutter->shields < 0) {
	ExplodeObject (clutter, 0);
	return 1;
	}
return 0;
}

//	-----------------------------------------------------------------------------

//given the specified vForce, apply damage from that vForce to an tObject
void ApplyForceDamage (tObject *objP, fix vForce, tObject *otherObjP)
{
	int	result;
	fix damage;

if (objP->flags & (OF_EXPLODING|OF_SHOULD_BE_DEAD))
	return;		//already exploding or dead
damage = FixDiv (vForce, objP->mType.physInfo.mass) / 8;
if ((otherObjP->nType == OBJ_PLAYER) && gameStates.app.cheats.bMonsterMode)
	damage = 0x7fffffff;
	switch (objP->nType) {
		case OBJ_ROBOT:
			if (ROBOTINFO (objP->id).attackType == 1) {
				if (otherObjP->nType == OBJ_WEAPON)
					result = ApplyDamageToRobot (objP, damage/4, otherObjP->cType.laserInfo.nParentObj);
				else
					result = ApplyDamageToRobot (objP, damage/4, OBJ_IDX (otherObjP));
				}
			else {
				if (otherObjP->nType == OBJ_WEAPON)
					result = ApplyDamageToRobot (objP, damage/2, otherObjP->cType.laserInfo.nParentObj);
				else
					result = ApplyDamageToRobot (objP, damage/2, OBJ_IDX (otherObjP));
				}		
			if (result && (otherObjP->cType.laserInfo.nParentSig == gameData.objs.console->nSignature))
				AddPointsToScore (ROBOTINFO (objP->id).scoreValue);
			break;

		case OBJ_PLAYER:
			//	If colliding with a claw nType robot, do damage proportional to gameData.time.xFrame because you can Collide with those
			//	bots every frame since they don't move.
			if ((otherObjP->nType == OBJ_ROBOT) && (ROBOTINFO (otherObjP->id).attackType))
				damage = FixMul (damage, gameData.time.xFrame*2);
			//	Make trainee easier.
			if (gameStates.app.nDifficultyLevel == 0)
				damage /= 2;
			ApplyDamageToPlayer (objP, otherObjP, damage);
			break;

		case OBJ_CLUTTER:
			ApplyDamageToClutter (objP, damage);
			break;

		case OBJ_CNTRLCEN:
			ApplyDamageToReactor (objP, damage, (short) OBJ_IDX (otherObjP));
			break;

		case OBJ_WEAPON:
			break;		//weapons don't take damage

		default:
			Int3 ();
		}
}

//	-----------------------------------------------------------------------------

short nMonsterballForces [100];

short nMonsterballPyroForce;

void SetMonsterballForces (void)
{
	int	i;
	tMonsterballForce *forceP = extraGameInfo [IsMultiGame].monsterball.forces;

memset (nMonsterballForces, 0, sizeof (nMonsterballForces));
for (i = 0; i < MAX_MONSTERBALL_FORCES - 1; i++, forceP++)
	nMonsterballForces [forceP->nWeaponId] = 	forceP->nForce;
nMonsterballPyroForce = forceP->nForce;
gameData.objs.pwrUp.info [POW_MONSTERBALL].size = 
	(gameData.objs.pwrUp.info [POW_SHIELD_BOOST].size * extraGameInfo [IsMultiGame].monsterball.nSizeMod) / 2;
}

//	-----------------------------------------------------------------------------

void BumpThisObject (tObject *objP, tObject *otherObjP, vmsVector *vForce, int bDamage)
{
	fix			xForceMag;
	vmsVector	vRotForce;

if (!(objP->mType.physInfo.flags & PF_PERSISTENT)) {
	if (objP->nType == OBJ_PLAYER) {
		if (otherObjP->nType == OBJ_MONSTERBALL) {
			double mq;
			
			mq = (double) otherObjP->mType.physInfo.mass / ((double) objP->mType.physInfo.mass * (double) nMonsterballPyroForce);
			vRotForce.p.x = (fix) ((double) vForce->p.x * mq);
			vRotForce.p.y = (fix) ((double) vForce->p.y * mq);
			vRotForce.p.z = (fix) ((double) vForce->p.z * mq);
			PhysApplyForce (objP, vForce);
			//PhysApplyRot (objP, &vRotForce);
			}
		else {
			vmsVector force2;
			force2.p.x = vForce->p.x / 4;
			force2.p.y = vForce->p.y / 4;
			force2.p.z = vForce->p.z / 4;
			PhysApplyForce (objP, &force2);
#if 0
			HUDMessage (0, "%1.2f   %1.2f", 
							VmVecMag (&objP->mType.physInfo.velocity) / 65536.0, 
							VmVecMag (&force2) / 65536.0);
#endif
			if (bDamage && ((otherObjP->nType != OBJ_ROBOT) || !ROBOTINFO (otherObjP->id).companion)) {
				xForceMag = VmVecMagQuick (&force2);
				ApplyForceDamage (objP, xForceMag, otherObjP);
				}
			}
		}
	else {
		fix h = (4 + gameStates.app.nDifficultyLevel);
		if (objP->nType == OBJ_ROBOT) {
			if (ROBOTINFO (objP->id).bossFlag)
				return;
			vRotForce.p.x = vForce->p.x / h;
			vRotForce.p.y = vForce->p.y / h;
			vRotForce.p.z = vForce->p.z / h;
			PhysApplyForce (objP, vForce);
			PhysApplyRot (objP, &vRotForce);
			}
		else if ((objP->nType == OBJ_CLUTTER) || (objP->nType == OBJ_DEBRIS) || (objP->nType == OBJ_CNTRLCEN)) {
			vRotForce.p.x = vForce->p.x / h;
			vRotForce.p.y = vForce->p.y / h;
			vRotForce.p.z = vForce->p.z / h;
			PhysApplyForce (objP, vForce);
			PhysApplyRot (objP, &vRotForce);
			}	
		else if (objP->nType == OBJ_MONSTERBALL) {
			double mq;
			
			if (otherObjP->nType == OBJ_PLAYER) {
				gameData.hoard.nLastHitter = OBJ_IDX (otherObjP);
				mq = ((double) otherObjP->mType.physInfo.mass * (double) nMonsterballPyroForce) / (double) objP->mType.physInfo.mass;
				}
			else {
				gameData.hoard.nLastHitter = otherObjP->cType.laserInfo.nParentObj;
				mq = (double) nMonsterballForces [otherObjP->id] * ((double) F1_0 / (double) otherObjP->mType.physInfo.mass) / 40.0;
				}
			vRotForce.p.x = (fix) ((double) vForce->p.x * mq);
			vRotForce.p.y = (fix) ((double) vForce->p.y * mq);
			vRotForce.p.z = (fix) ((double) vForce->p.z * mq);
			PhysApplyForce (objP, &vRotForce);
			PhysApplyRot (objP, &vRotForce);
			if (gameData.hoard.nLastHitter == LOCALPLAYER.nObject)
				MultiSendMonsterball (1, 0);
			}
		else
			return;
		if (bDamage) {
			xForceMag = VmVecMagQuick (vForce);
			ApplyForceDamage (objP, xForceMag, otherObjP);
			}
		}
	}
}

//	-----------------------------------------------------------------------------
//deal with two gameData.objs.objects bumping into each other.  Apply vForce from collision
//to each robot.  The flags tells whether the objects should take damage from
//the collision.

#define SIGN(_i)	((_i) ? (_i) / labs (_i) : 1)

int BumpTwoObjects (tObject *objP0, tObject *objP1, int bDamage, vmsVector *vHitPt)
{
	vmsVector	vForce, p0, p1, v0, v1, vh, vr, vn0, vn1;
	fix			mag, dot, m0, m1;
	tObject		*t;

if (objP0->movementType != MT_PHYSICS)
	t = objP1;
else if (objP1->movementType != MT_PHYSICS)
	t = objP0;
else
	t = NULL;
if (t) {
	Assert (t->movementType == MT_PHYSICS);
	VmVecCopyScale (&vForce, &t->mType.physInfo.velocity, -t->mType.physInfo.mass);
#if 1//def _DEBUG
	if (vForce.p.x || vForce.p.y || vForce.p.z)
#endif
	PhysApplyForce (t, &vForce);
	return 1;
	}
#ifdef _DEBUG
//redo:
#endif
p0 = objP0->position.vPos;
p1 = objP1->position.vPos;
v0 = objP0->mType.physInfo.velocity;
v1 = objP1->mType.physInfo.velocity;
mag = VmVecDot (&v0, &v1);
m0 = VmVecCopyNormalize (&vn0, &v0);
m1 = VmVecCopyNormalize (&vn1, &v1);
if (m0 && m1) {
	if (m0 > m1) {
		VmVecScaleFrac (&vn0, m1, m0);
		VmVecScaleFrac (&vn1, m1, m0);
		}
	else {
		VmVecScaleFrac (&vn0, m0, m1);
		VmVecScaleFrac (&vn1, m0, m1);
		}
	}
VmVecSub (&vh, &p0, &p1);
if (!EGI_FLAG (nHitboxes, 0, 0, 0)) {
	m0 = VmVecMag (&vh);
	if (m0 > ((objP0->size + objP1->size) * 3) / 4) {
		VmVecInc (&p0, &vn0);
		VmVecInc (&p1, &vn1);
		VmVecSub (&vh, &p0, &p1);
		m1 = VmVecMag (&vh);
		if (m1 > m0) {
#if 0//def _DEBUG
			HUDMessage (0, "moving away (%d, %d)", m0, m1);
#endif
			return 0;
			}
		}
	}
#if 0//def _DEBUG
HUDMessage (0, "colliding (%1.2f, %1.2f)", f2fl (m0), f2fl (m1));
#endif
VmVecSub (&vForce, &v0, &v1);
m0 = objP0->mType.physInfo.mass;
m1 = objP1->mType.physInfo.mass;
if (!((m0 + m1) && FixMul (m0, m1))) {
#if 0//def _DEBUG
	HUDMessage (0, "Invalid Mass!!!");
#endif
	return 0;
	}
mag = VmVecMag (&vForce);
VmVecScaleFrac (&vForce, 2 * FixMul (m0, m1), m0 + m1);
mag = VmVecMag (&vForce);
#if 0//def _DEBUG
if (fabs (f2fl (mag) > 10000))
	;//goto redo;
HUDMessage (0, "bump force: %c%1.2f", (SIGN (vForce.p.x) * SIGN (vForce.p.y) * SIGN (vForce.p.z)) ? '-' : '+', f2fl (mag));
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
	VmVecSub (&vh, vHitPt, &objP1->position.vPos);
	if (VmVecNormalize (&vh) > F1_0 / 16) {
		vr = vh;
		VmVecNegate (&vh);
		VmVecScale (&vh, mag);
		BumpThisObject (objP1, objP0, &vh, bDamage);
		// compute reflection vector. The vector from the other object's center to the hit point 
		// serves as normal.
		v1 = v0;
		VmVecNormalize (&v1);
		dot = VmVecDot (&v1, &vr);
		VmVecScale (&vr, 2 * dot);
		//VmVecNegate (VmVecDec (&vr, &v0));
		VmVecNormalize (&vr);
		VmVecScale (&vr, mag);
		VmVecNegate (&vr);
		BumpThisObject (objP0, objP1, &vr, bDamage);
		return 1;
		}
	}
BumpThisObject (objP1, objP0, &vForce, 0);
VmVecNegate (&vForce);
BumpThisObject (objP0, objP1, &vForce, 0);
return 1;
}

//	-----------------------------------------------------------------------------

void BumpOneObject (tObject *objP0, vmsVector *hit_dir, fix damage)
{
	vmsVector	hit_vec;

hit_vec = *hit_dir;
VmVecScale (&hit_vec, damage);
PhysApplyForce (objP0, &hit_vec);
}

//	-----------------------------------------------------------------------------

#define DAMAGE_SCALE 		128	//	Was 32 before 8:55 am on Thursday, September 15, changed by MK, walls were hurting me more than robots!
#define DAMAGE_THRESHOLD 	 (F1_0/3)
#define WALL_LOUDNESS_SCALE (20)

fix force_force = i2f (50);

void CollidePlayerAndWall (tObject * playerObjP, fix hitspeed, short hitseg, short hitwall, vmsVector * vHitPt)
{
	fix damage;
	char ForceFieldHit=0;
	int nBaseTex, nOvlTex;

if (playerObjP->id != gameData.multiplayer.nLocalPlayer) // Execute only for local tPlayer
	return;
nBaseTex = gameData.segs.segments [hitseg].sides [hitwall].nBaseTex;
//	If this tWall does damage, don't make *BONK* sound, we'll be making another sound.
if (gameData.pig.tex.pTMapInfo [nBaseTex].damage > 0)
	return;
if (gameData.pig.tex.pTMapInfo [nBaseTex].flags & TMI_FORCE_FIELD) {
	vmsVector vForce;
	PALETTE_FLASH_ADD (0, 0, 60);	//flash blue
	//knock tPlayer around
	vForce.p.x = 40 * (d_rand () - 16384);
	vForce.p.y = 40 * (d_rand () - 16384);
	vForce.p.z = 40 * (d_rand () - 16384);
	PhysApplyRot (playerObjP, &vForce);
#ifdef TACTILE
	if (TactileStick)
		Tactile_apply_force (&vForce, &playerObjP->position.mOrient);
#endif
	//make sound
	DigiLinkSoundToPos (SOUND_FORCEFIELD_BOUNCE_PLAYER, hitseg, 0, vHitPt, 0, f1_0);
	if (gameData.app.nGameMode & GM_MULTI)
		MultiSendPlaySound (SOUND_FORCEFIELD_BOUNCE_PLAYER, f1_0);
	ForceFieldHit=1;
	} 
else {
#ifdef TACTILE
	vmsVector vForce;
	if (TactileStick) {
		vForce.p.x = -playerObjP->mType.physInfo.velocity.p.x;
		vForce.p.y = -playerObjP->mType.physInfo.velocity.p.y;
		vForce.p.z = -playerObjP->mType.physInfo.velocity.p.z;
		Tactile_do_collide (&vForce, &playerObjP->position.mOrient);
	}
#endif
   WallHitProcess (gameData.segs.segments + hitseg, hitwall, 20, playerObjP->id, playerObjP);
	}
if (gameStates.app.bD2XLevel && (gameData.segs.segment2s [hitseg].special == SEGMENT_IS_NODAMAGE))
	return;
//	** Damage from hitting tWall **
//	If the tPlayer has less than 10% shields, don't take damage from bump
// Note: Does quad damage if hit a vForce field - JL
damage = (hitspeed / DAMAGE_SCALE) * (ForceFieldHit * 8 + 1);
nOvlTex = gameData.segs.segments [hitseg].sides [hitwall].nOvlTex;
//don't do tWall damage and sound if hit lava or water
if ((gameData.pig.tex.pTMapInfo [nBaseTex].flags & (TMI_WATER|TMI_VOLATILE)) || 
		(nOvlTex && (gameData.pig.tex.pTMapInfo [nOvlTex].flags & (TMI_WATER|TMI_VOLATILE))))
	damage = 0;
if (damage >= DAMAGE_THRESHOLD) {
	int	volume = (hitspeed- (DAMAGE_SCALE*DAMAGE_THRESHOLD)) / WALL_LOUDNESS_SCALE ;
	CreateAwarenessEvent (playerObjP, PA_WEAPON_WALL_COLLISION);
	if (volume > F1_0)
		volume = F1_0;
	if (volume > 0 && !ForceFieldHit) {  // uhhhgly hack
		DigiLinkSoundToPos (SOUND_PLAYER_HIT_WALL, hitseg, 0, vHitPt, 0, volume);
		if (gameData.app.nGameMode & GM_MULTI)
			MultiSendPlaySound (SOUND_PLAYER_HIT_WALL, volume);	
		}
	if (!(LOCALPLAYER.flags & PLAYER_FLAGS_INVULNERABLE))
		if (LOCALPLAYER.shields > f1_0*10 || ForceFieldHit)
			ApplyDamageToPlayer (playerObjP, playerObjP, damage);
	}
return;
}

//	-----------------------------------------------------------------------------

fix	xLastVolatileScrapeSoundTime = 0;

int CollideWeaponAndWall (tObject * weapon, fix hitspeed, short hitseg, short hitwall, vmsVector * vHitPt);
int CollideDebrisAndWall (tObject * debris, fix hitspeed, short hitseg, short hitwall, vmsVector * vHitPt);

//see if tWall is volatile or water
//if volatile, cause damage to tPlayer  
//returns 1=lava, 2=water
int CheckVolatileWall (tObject *objP, int nSegment, int nSide, vmsVector *vHitPt)
{
	fix	d, water;
	int	nTexture;

Assert (objP->nType == OBJ_PLAYER);
nTexture = gameData.segs.segments [nSegment].sides [nSide].nBaseTex;
d = gameData.pig.tex.pTMapInfo [nTexture].damage;
water = (gameData.pig.tex.pTMapInfo [nTexture].flags & TMI_WATER);
if (d > 0 || water) {
	if (objP->id == gameData.multiplayer.nLocalPlayer) {
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

			PALETTE_FLASH_ADD (f2i (damage*4), 0, 0);	//flash red
			}
		objP->mType.physInfo.rotVel.p.x = (d_rand () - 16384)/2;
		objP->mType.physInfo.rotVel.p.z = (d_rand () - 16384)/2;
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

int CheckVolatileSegment (tObject *objP, int nSegment)
{
	fix d;

//	Assert (objP->nType==OBJ_PLAYER);

	if (!EGI_FLAG (bFluidPhysics, 1, 0, 0))
		return 0;
	if (gameData.segs.segment2s [nSegment].special == SEGMENT_IS_WATER)
		d = 0;
	else if (gameData.segs.segment2s [nSegment].special == SEGMENT_IS_LAVA)
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
		if (objP->nType == OBJ_PLAYER) {
			if (!(gameData.multiplayer.players [objP->id].flags & PLAYER_FLAGS_INVULNERABLE))
				ApplyDamageToPlayer (objP, objP, damage);
			}
		if (objP->nType == OBJ_ROBOT) {
			ApplyDamageToRobot (objP, objP->shields + 1, OBJ_IDX (objP));
			}

#ifdef TACTILE
		if (TactileStick)
			Tactile_Xvibrate (50, 25);
#endif
	if ((objP->nType == OBJ_PLAYER) && (objP->id == gameData.multiplayer.nLocalPlayer))
		PALETTE_FLASH_ADD (f2i (damage*4), 0, 0);	//flash red
	if ((objP->nType == OBJ_PLAYER) || (objP->nType == OBJ_ROBOT)) {
		objP->mType.physInfo.rotVel.p.x = (d_rand () - 16384) / 2;
		objP->mType.physInfo.rotVel.p.z = (d_rand () - 16384) / 2;
		}
	return (d > 0) ? 1 : 2;
	}
if (((objP->nType == OBJ_PLAYER) || (objP->nType == OBJ_ROBOT)) && 
	 (objP->mType.physInfo.thrust.p.x || 
	  objP->mType.physInfo.thrust.p.y ||
	  objP->mType.physInfo.thrust.p.z)) {
	objP->mType.physInfo.rotVel.p.x = (d_rand () - 16384) / 4;
	objP->mType.physInfo.rotVel.p.z = (d_rand () - 16384) / 4;
	}
return 0;
}

//	-----------------------------------------------------------------------------
//this gets called when an tObject is scraping along the tWall
void ScrapeObjectOnWall (tObject *objP, short hitseg, short hitside, vmsVector * vHitPt)
{
switch (objP->nType) {
	case OBJ_PLAYER:
		if (objP->id == gameData.multiplayer.nLocalPlayer) {
			int nType = CheckVolatileWall (objP, hitseg, hitside, vHitPt);
			if (nType != 0) {
				vmsVector	vHit, vRand;

				if ((gameData.time.xGame > xLastVolatileScrapeSoundTime + F1_0/4) || 
						(gameData.time.xGame < xLastVolatileScrapeSoundTime)) {
					short sound = (nType == 1) ? SOUND_VOLATILE_WALL_HISS : SOUND_SHIP_IN_WATER;
					xLastVolatileScrapeSoundTime = gameData.time.xGame;
					DigiLinkSoundToPos (sound, hitseg, 0, vHitPt, 0, F1_0);
					if (IsMultiGame)
						MultiSendPlaySound (sound, F1_0);
					}
				vHit = gameData.segs.segments [hitseg].sides [hitside].normals [0];
				MakeRandomVector (&vRand);
				VmVecScaleInc (&vHit, &vRand, F1_0/8);
				VmVecNormalizeQuick (&vHit);
				BumpOneObject (objP, &vHit, F1_0*8);
				}
			}
		break;

	//these two kinds of gameData.objs.objects below shouldn't really slide, so
	//if this scrape routine gets called (which it might if the
	//tObject (such as a fusion blob) was created already poking
	//through the tWall) call the Collide routine.

	case OBJ_WEAPON:
		CollideWeaponAndWall (objP, 0, hitseg, hitside, vHitPt); 
		break;

	case OBJ_DEBRIS:		
		CollideDebrisAndWall (objP, 0, hitseg, hitside, vHitPt); 
		break;
	}

}

//	-----------------------------------------------------------------------------
//if an effect is hit, and it can blow up, then blow it up
//returns true if it blew up
int CheckEffectBlowup (tSegment *segP, short nSide, vmsVector *pnt, tObject *blower, int bForceBlowup)
{
	int			tm, tmf, ec, nBitmap = 0;
	int			nWall, nTrigger;
	int			bOkToBlow = 0, nSwitchType = -1;
	//int			x = 0, y = 0;
	short			nSound, bPermaTrigger;
	ubyte			vc;
	fix			u, v;
	fix			xDestSize;
	tEffectClip			*ecP = NULL;
	grsBitmap	*bmP;
	//	If this tWall has a tTrigger and the blower-upper is not the tPlayer or the buddy, abort!

if (blower->cType.laserInfo.parentType == OBJ_ROBOT)
	if (ROBOTINFO (gameData.objs.objects [blower->cType.laserInfo.nParentObj].id).companion)
		bOkToBlow = 1;

if (!(bOkToBlow || (blower->cType.laserInfo.parentType == OBJ_PLAYER))) {
	int nWall = WallNumP (segP, nSide);
	if (IS_WALL (nWall)&& 
		 (gameData.walls.walls [nWall].nTrigger < gameData.trigs.nTriggers))
		return 0;
	}

if (!(tm = segP->sides [nSide].nOvlTex))
	return 0;

tmf = segP->sides [nSide].nOvlOrient;		//tm flags
ec = gameData.pig.tex.pTMapInfo [tm].eclip_num;
if (ec < 0) {
	if (gameData.pig.tex.pTMapInfo [tm].destroyed == -1)
		return 0;
	nBitmap = -1;
	nSwitchType = 0;
	}
else {
	ecP = gameData.eff.pEffects + ec;
	if (ecP->flags & EF_ONE_SHOT)
		return 0;
	nBitmap = ecP->nDestBm;
	if (nBitmap < 0)
		return 0;
	nSwitchType = 1;
	}
//check if it's an animation (monitor) or casts light
bmP = gameData.pig.tex.pBitmaps + gameData.pig.tex.pBmIndex [tm].index;
PIGGY_PAGE_IN (gameData.pig.tex.pBmIndex [tm], gameStates.app.bD1Data);
//this can be blown up...did we hit it?
if (!bForceBlowup) {
	FindHitPointUV (&u, &v, NULL, pnt, segP, nSide, 0);	//evil: always say face zero
	bForceBlowup = !PixelTranspType (tm, tmf,  segP->sides [nSide].nFrame, u, v);
	}
if (!bForceBlowup) 
	return 0;

if (IsMultiGame && netGame.AlwaysLighting && !nSwitchType)
	return 0;
//note: this must get called before the texture changes, 
//because we use the light value of the texture to change
//the static light in the tSegment
nWall = WallNumP (segP, nSide);
bPermaTrigger = 
	IS_WALL (nWall) && 
	((nTrigger = gameData.walls.walls [nWall].nTrigger) != NO_TRIGGER) &&
	(gameData.trigs.triggers [nTrigger].flags & TF_PERMANENT);
if (!bPermaTrigger)
	SubtractLight (SEG_IDX (segP), nSide);
if (gameData.demo.nState == ND_STATE_RECORDING)
	NDRecordEffectBlowup (SEG_IDX (segP), nSide, pnt);
if (nSwitchType) {
	xDestSize = ecP->dest_size;
	vc = ecP->dest_vclip;
	}
else {
	xDestSize = i2f (20);
	vc = 3;
	}
ObjectCreateExplosion (SEG_IDX (segP), pnt, xDestSize, vc);
if (nSwitchType) {
	if ((nSound = gameData.eff.pVClips [vc].nSound) != -1)
		DigiLinkSoundToPos (nSound, SEG_IDX (segP), 0, pnt,  0, F1_0);
	if ((nSound = ecP->nSound) != -1)		//kill sound
		DigiKillSoundLinkedToSegment (SEG_IDX (segP), nSide, nSound);
	if (!bPermaTrigger && (ecP->dest_eclip != -1) && (gameData.eff.pEffects [ecP->dest_eclip].nSegment == -1)) {
		tEffectClip	*newEcP = gameData.eff.pEffects + ecP->dest_eclip;
		int nNewBm = newEcP->changingWallTexture;
		newEcP->time_left = EffectFrameTime (newEcP);
		newEcP->nCurFrame = 0;
		newEcP->nSegment = SEG_IDX (segP);
		newEcP->nSide = nSide;
		newEcP->flags |= EF_ONE_SHOT;
		newEcP->nDestBm = ecP->nDestBm;

		Assert ((nNewBm != 0) && (segP->sides [nSide].nOvlTex != 0));
		segP->sides [nSide].nOvlTex = nNewBm;		//replace with destoyed
		}
	else {
		Assert ((nBitmap != 0) && (segP->sides [nSide].nOvlTex != 0));
		if (!bPermaTrigger)
			segP->sides [nSide].nOvlTex = nBitmap;		//replace with destoyed
		}
	}
else {
	if (!bPermaTrigger)
		segP->sides [nSide].nOvlTex = gameData.pig.tex.pTMapInfo [tm].destroyed;
	//assume this is a light, and play light sound
	DigiLinkSoundToPos (SOUND_LIGHT_BLOWNUP, SEG_IDX (segP), 0, pnt,  0, F1_0);
	}
return 1;		//blew up!
}

//	Copied from laser.c!
#define	MIN_OMEGA_BLOBS		3				//	No matter how close the obstruction, at this many blobs created.
#define	MIN_OMEGA_DIST			 (F1_0*3)		//	At least this distance between blobs, unless doing so would violate MIN_OMEGA_BLOBS
#define	DESIRED_OMEGA_DIST	 (F1_0*5)		//	This is the desired distance between blobs.  For distances > MIN_OMEGA_BLOBS*DESIRED_OMEGA_DIST, but not very large, this will apply.
#define	MAX_OMEGA_BLOBS		16				//	No matter how far away the obstruction, this is the maximum number of blobs.
#define	MAX_OMEGA_DIST			 (MAX_OMEGA_BLOBS * DESIRED_OMEGA_DIST)		//	Maximum extent of lightning blobs.

//	-------------------------------------------------
//	Return true if ok to do Omega damage.
int OkToDoOmegaDamage (tObject *weapon)
{
	int	parent_sig = weapon->cType.laserInfo.nParentSig;
	int	nParentObj = weapon->cType.laserInfo.nParentObj;
	fix	dist;

if (!IsMultiGame)
	return 1;
if (gameData.objs.objects [nParentObj].nSignature != parent_sig) {
#if TRACE
	con_printf (CONDBG, "Parent of omega blob not consistent with tObject information. \n");
#endif
	return 1;
	}
dist = VmVecDistQuick (&gameData.objs.objects [nParentObj].position.vPos, &weapon->position.vPos);
if (dist > MAX_OMEGA_DIST)
	return 0;
return 1;
}

//	-----------------------------------------------------------------------------
//these gets added to the weapon's values when the weapon hits a volitle tWall
#define VOLATILE_WALL_EXPL_STRENGTH i2f (10)
#define VOLATILE_WALL_IMPACT_SIZE	i2f (3)
#define VOLATILE_WALL_DAMAGE_FORCE	i2f (5)
#define VOLATILE_WALL_DAMAGE_RADIUS	i2f (30)

// int Show_segAnd_side = 0;

int CollideWeaponAndWall (
	tObject * weaponP, fix hitspeed, short hitseg, short hitwall, vmsVector * vHitPt)
{
	tSegment *segP = gameData.segs.segments + hitseg;
	tSide *sideP = segP->sides + hitwall;
	tWeaponInfo *wInfoP = gameData.weapons.info + weaponP->id;
	tObject *wObjP = gameData.objs.objects + weaponP->cType.laserInfo.nParentObj;

	int bBlewUp;
	int wallType;
	int playernum;
	int robot_escort;
	fix nStrength = WI_strength (weaponP->id, gameStates.app.nDifficultyLevel);

if (weaponP->id == OMEGA_ID)
	if (!OkToDoOmegaDamage (weaponP))
		return 1;

if (bIsMissile [weaponP->id])
	CreateBlast (weaponP);
//	If this is a guided missile and it strikes fairly directly, clear bounce flag.
if (weaponP->id == GUIDEDMSL_ID) {
	fix dot = VmVecDot (&weaponP->position.mOrient.fVec, sideP->normals);
#if TRACE
	con_printf (CONDBG, "Guided missile dot = %7.3f \n", f2fl (dot));
#endif
	if (dot < -F1_0/6) {
#if TRACE
		con_printf (CONDBG, "Guided missile loses bounciness. \n");
#endif
		weaponP->mType.physInfo.flags &= ~PF_BOUNCE;
		}
	}

//if an energy weaponP hits a forcefield, let it bounce
if ((gameData.pig.tex.pTMapInfo [sideP->nBaseTex].flags & TMI_FORCE_FIELD) &&
	 ((weaponP->nType != OBJ_WEAPON) || wInfoP->energy_usage)) {

	//make sound
	DigiLinkSoundToPos (SOUND_FORCEFIELD_BOUNCE_WEAPON, hitseg, 0, vHitPt, 0, f1_0);
	if (gameData.app.nGameMode & GM_MULTI)
		MultiSendPlaySound (SOUND_FORCEFIELD_BOUNCE_WEAPON, f1_0);
	return 1;	//bail here. physics code will bounce this tObject
	}

#ifdef _DEBUG
if (keyd_pressed [KEY_LAPOSTRO])
	if (weaponP->cType.laserInfo.nParentObj == LOCALPLAYER.nObject) {
		//	MK: Real pain when you need to know a segP:tSide and you've got quad lasers.
#if TRACE
		con_printf (CONDBG, "Your laser hit at tSegment = %i, tSide = %i \n", hitseg, hitwall);
#endif
		//HUDInitMessage ("Hit at segment = %i, side = %i", hitseg, hitwall);
		if (weaponP->id < 4)
			SubtractLight (hitseg, hitwall);
		else if (weaponP->id == FLARE_ID)
			AddLight (hitseg, hitwall);
		}
if ((weaponP->mType.physInfo.velocity.p.x == 0) && 
	 (weaponP->mType.physInfo.velocity.p.y == 0) && 
	 (weaponP->mType.physInfo.velocity.p.z == 0)) {
	Int3 ();	//	Contact Matt: This is impossible.  A weaponP with 0 velocity hit a tWall, which doesn't move.
	return 1;
	}
#endif
bBlewUp = CheckEffectBlowup (segP, hitwall, vHitPt, weaponP, 0);
if ((weaponP->cType.laserInfo.parentType == OBJ_ROBOT) && ROBOTINFO (wObjP->id).companion) {
	robot_escort = 1;
	if (gameData.app.nGameMode & GM_MULTI) {
		Int3 ();  // Get Jason!
	   return 1;
	   }	
	playernum = gameData.multiplayer.nLocalPlayer;		//if single tPlayer, he's the tPlayer's buddy
	}
else {
	robot_escort = 0;
	if (wObjP->nType == OBJ_PLAYER)
		playernum = wObjP->id;
	else
		playernum = -1;		//not a tPlayer (thus a robot)
	}
if (bBlewUp) {		//could be a tWall switch
	//for tWall triggers, always say that the tPlayer shot it out.  This is
	//because robots can shoot out tWall triggers, and so the tTrigger better
	//take effect  
	//	NO -- Changed by MK, 10/18/95.  We don't want robots blowing puzzles.  Only tPlayer or buddy can open!
	CheckTrigger (segP, hitwall, weaponP->cType.laserInfo.nParentObj, 1);
	}
if (weaponP->id == EARTHSHAKER_ID)
	ShakerRockStuff ();
wallType = WallHitProcess (segP, hitwall, weaponP->shields, playernum, weaponP);
// Wall is volatile if either tmap 1 or 2 is volatile
if ((gameData.pig.tex.pTMapInfo [sideP->nBaseTex].flags & TMI_VOLATILE) || 
		(sideP->nOvlTex && 
		(gameData.pig.tex.pTMapInfo [sideP->nOvlTex].flags & TMI_VOLATILE))) {
	ubyte tVideoClip;
	//we've hit a volatile tWall
	DigiLinkSoundToPos (SOUND_VOLATILE_WALL_HIT, hitseg, 0, vHitPt, 0, F1_0);
	//for most weapons, use volatile tWall hit.  For mega, use its special tVideoClip
	tVideoClip = (weaponP->id == MEGAMSL_ID) ? wInfoP->robot_hit_vclip : VCLIP_VOLATILE_WALL_HIT;
	//	New by MK: If powerful badass, explode as badass, not due to lava, fixes megas being wimpy in lava.
	if (wInfoP->damage_radius >= VOLATILE_WALL_DAMAGE_RADIUS/2)
		ExplodeBadassWeapon (weaponP, vHitPt);
	else
		ObjectCreateBadassExplosion (weaponP, hitseg, vHitPt, 
			wInfoP->impact_size + VOLATILE_WALL_IMPACT_SIZE, 
			tVideoClip, 
			nStrength / 4 + VOLATILE_WALL_EXPL_STRENGTH, 	//	diminished by mk on 12/08/94, i was doing 70 damage hitting lava on lvl 1.
			wInfoP->damage_radius+VOLATILE_WALL_DAMAGE_RADIUS, 
			nStrength / 2 + VOLATILE_WALL_DAMAGE_FORCE, 
			weaponP->cType.laserInfo.nParentObj);
	KillObject (weaponP);		//make flares die in lava
	}
else if ((gameData.pig.tex.pTMapInfo [sideP->nBaseTex].flags & TMI_WATER) || 
			(sideP->nOvlTex && (gameData.pig.tex.pTMapInfo [sideP->nOvlTex].flags & TMI_WATER))) {
	//we've hit water
	//	MK: 09/13/95: Badass in water is 1/2 normal intensity.
	if (wInfoP->matter) {
		DigiLinkSoundToPos (SOUNDMSL_HIT_WATER, hitseg, 0, vHitPt, 0, F1_0);
		if (wInfoP->damage_radius) {
			DigiLinkSoundToObject (SOUND_BADASS_EXPLOSION, OBJ_IDX (weaponP), 0, F1_0);
			//	MK: 09/13/95: Badass in water is 1/2 normal intensity.
			ObjectCreateBadassExplosion (weaponP, hitseg, vHitPt, 
				wInfoP->impact_size/2, 
				wInfoP->robot_hit_vclip, 
				nStrength / 4, 
				wInfoP->damage_radius, 
				nStrength / 2, 
				weaponP->cType.laserInfo.nParentObj);
			}
		else
			ObjectCreateExplosion (weaponP->nSegment, &weaponP->position.vPos, wInfoP->impact_size, wInfoP->wall_hit_vclip);
		} 
	else {
		DigiLinkSoundToPos (SOUND_LASER_HIT_WATER, hitseg, 0, vHitPt, 0, F1_0);
		ObjectCreateExplosion (weaponP->nSegment, &weaponP->position.vPos, wInfoP->impact_size, VCLIP_WATER_HIT);
		}
	KillObject (weaponP);		//make flares die in water
	}
else {
	if (weaponP->mType.physInfo.flags & PF_BOUNCE) {
		//do special bound sound & effect
		}
	else {
		//if it's not the tPlayer's weaponP, or it is the tPlayer's and there
		//is no tWall, and no blowing up monitor, then play sound
		if ((weaponP->cType.laserInfo.parentType != OBJ_PLAYER) ||	
				((!IS_WALL (WallNumS (sideP)) || wallType==WHP_NOT_SPECIAL) && !bBlewUp))
			if ((wInfoP->wall_hitSound > -1) && (!(weaponP->flags & OF_SILENT)))
			DigiLinkSoundToPos (wInfoP->wall_hitSound, weaponP->nSegment, 0, &weaponP->position.vPos, 0, F1_0);
		if (wInfoP->wall_hit_vclip > -1)	{
			if (wInfoP->damage_radius)
				ExplodeBadassWeapon (weaponP, vHitPt);
			else
				ObjectCreateExplosion (weaponP->nSegment, &weaponP->position.vPos, wInfoP->impact_size, wInfoP->wall_hit_vclip);
			}
		}
	}
//	If weaponP fired by tPlayer or companion...
if ((weaponP->cType.laserInfo.parentType == OBJ_PLAYER) || robot_escort) {
	if (!(weaponP->flags & OF_SILENT) && 
		 (weaponP->cType.laserInfo.nParentObj == LOCALPLAYER.nObject))
		CreateAwarenessEvent (weaponP, PA_WEAPON_WALL_COLLISION);			// tObject "weaponP" can attract attention to tPlayer

//		if (weaponP->id != FLARE_ID) {
//	We now allow flares to open doors.

	if (((weaponP->id != FLARE_ID) || (weaponP->cType.laserInfo.parentType != OBJ_PLAYER)) && 
		 !(weaponP->mType.physInfo.flags & PF_BOUNCE)) {
		KillObject (weaponP);
		}

	//don't let flares stick in vForce fields
	if ((weaponP->id == FLARE_ID) && (gameData.pig.tex.pTMapInfo [sideP->nBaseTex].flags & TMI_FORCE_FIELD)) {
		KillObject (weaponP);
		}
	if (!(weaponP->flags & OF_SILENT)) {
		switch (wallType) {
			case WHP_NOT_SPECIAL:
				//should be handled above
				//DigiLinkSoundToPos (wInfoP->wall_hitSound, weaponP->nSegment, 0, &weaponP->position.vPos, 0, F1_0);
				break;

			case WHP_NO_KEY:
				//play special hit door sound (if/when we get it)
				DigiLinkSoundToPos (SOUND_WEAPON_HIT_DOOR, weaponP->nSegment, 0, &weaponP->position.vPos, 0, F1_0);
			   if (gameData.app.nGameMode & GM_MULTI)
					MultiSendPlaySound (SOUND_WEAPON_HIT_DOOR, F1_0);
				break;

			case WHP_BLASTABLE:
				//play special blastable tWall sound (if/when we get it)
				if ((wInfoP->wall_hitSound > -1) && (!(weaponP->flags & OF_SILENT)))
					DigiLinkSoundToPos (SOUND_WEAPON_HIT_BLASTABLE, weaponP->nSegment, 0, &weaponP->position.vPos, 0, F1_0);
				break;

			case WHP_DOOR:
				//don't play anything, since door open sound will play
				break;
			}
		} 
	}
else {
	// This is a robot's laser
	if (!(weaponP->mType.physInfo.flags & PF_BOUNCE))
		KillObject (weaponP);
	}
return 1;
}

//	-----------------------------------------------------------------------------
//##void CollideCameraAndWall (tObject * camera, fix hitspeed, short hitseg, short hitwall, vmsVector * vHitPt)	{
//##	return;
//##}

//##void CollidePowerupAndWall (tObject * powerup, fix hitspeed, short hitseg, short hitwall, vmsVector * vHitPt)	{
//##	return;
//##}

int CollideDebrisAndWall (tObject * debris, fix hitspeed, short hitseg, short hitwall, vmsVector * vHitPt)	
{	
if (gameOpts->render.nDebrisLife) {
	vmsVector	vDir = debris->mType.physInfo.velocity,
					vNormal = gameData.segs.segments [hitseg].sides [hitwall].normals [0];
	VmVecReflect (&debris->mType.physInfo.velocity, &vDir, &vNormal);
	DigiLinkSoundToPos (SOUND_PLAYER_HIT_WALL, hitseg, 0, vHitPt, 0, F1_0 / 3);
	}
else
	ExplodeObject (debris, 0);
return 1;
}

//##void CollideFireballAndFireball (tObject * fireball1, tObject * fireball2, vmsVector *vHitPt) {
//##	return; 
//##}

//##void CollideFireballAndRobot (tObject * fireball, tObject * robot, vmsVector *vHitPt) {
//##	return; 
//##}

//##void CollideFireballAndHostage (tObject * fireball, tObject * hostage, vmsVector *vHitPt) {
//##	return; 
//##}

//##void CollideFireballAndPlayer (tObject * fireball, tObject * tPlayer, vmsVector *vHitPt) {
//##	return; 
//##}

//##void CollideFireballAndWeapon (tObject * fireball, tObject * weapon, vmsVector *vHitPt) { 
//##	//KillObject (weapon);
//##	return; 
//##}

//##void CollideFireballAndCamera (tObject * fireball, tObject * camera, vmsVector *vHitPt) { 
//##	return; 
//##}

//##void CollideFireballAndPowerup (tObject * fireball, tObject * powerup, vmsVector *vHitPt) { 
//##	return; 
//##}

//##void CollideFireballAndDebris (tObject * fireball, tObject * debris, vmsVector *vHitPt) { 
//##	return; 
//##}

//	-------------------------------------------------------------------------------------------------------------------

int CollideRobotAndRobot (tObject * robotP1, tObject * robotP2, vmsVector *vHitPt) 
{ 
//		robot1-gameData.objs.objects, f2i (robot1->position.vPos.x), f2i (robot1->position.vPos.y), f2i (robot1->position.vPos.z), 
//		robot2-gameData.objs.objects, f2i (robot2->position.vPos.x), f2i (robot2->position.vPos.y), f2i (robot2->position.vPos.z), 
//		f2i (vHitPt->x), f2i (vHitPt->y), f2i (vHitPt->z));

BumpTwoObjects (robotP1, robotP2, 1, vHitPt);
return 1; 
}

//	-----------------------------------------------------------------------------

int CollideRobotAndReactor (tObject * objP1, tObject * obj2, vmsVector *vHitPt)
{
if (objP1->nType == OBJ_ROBOT) {
	vmsVector	hitvec;
	VmVecSub (&hitvec, &obj2->position.vPos, &objP1->position.vPos);
	VmVecNormalizeQuick (&hitvec);
	BumpOneObject (objP1, &hitvec, 0);
	} 
else {
	vmsVector	hitvec;
	VmVecSub (&hitvec, &objP1->position.vPos, &obj2->position.vPos);
	VmVecNormalizeQuick (&hitvec);
	BumpOneObject (obj2, &hitvec, 0);
	}
return 1;
}

//	-----------------------------------------------------------------------------
//##void CollideRobotAndHostage (tObject * robot, tObject * hostage, vmsVector *vHitPt) { 
//##	return; 
//##}

//	-----------------------------------------------------------------------------

fix xLastThiefHitTime;

int  CollideRobotAndPlayer (tObject *robot, tObject *playerObjP, vmsVector *vHitPt)
{ 
	int	bTheftAttempt = 0;
	short	nCollisionSeg;

if (robot->flags & OF_EXPLODING)
	return 1;
nCollisionSeg = FindSegByPoint (vHitPt, playerObjP->nSegment);
if (nCollisionSeg != -1)
	ObjectCreateExplosion (nCollisionSeg, vHitPt, gameData.weapons.info [0].impact_size, gameData.weapons.info [0].wall_hit_vclip);
if (playerObjP->id == gameData.multiplayer.nLocalPlayer) {
	if (ROBOTINFO (robot->id).companion)	//	Player and companion don't Collide.
		return 1;
	if (ROBOTINFO (robot->id).kamikaze) {
		ApplyDamageToRobot (robot, robot->shields + 1, OBJ_IDX (playerObjP));
		if (playerObjP == gameData.objs.console)
			AddPointsToScore (ROBOTINFO (robot->id).scoreValue);
		}
	if (ROBOTINFO (robot->id).thief) {
		if (gameData.ai.localInfo [OBJ_IDX (robot)].mode == AIM_THIEF_ATTACK) {
			xLastThiefHitTime = gameData.time.xGame;
			AttemptToStealItem (robot, playerObjP->id);
			bTheftAttempt = 1;
			} 
		else if (gameData.time.xGame - xLastThiefHitTime < F1_0*2)
			return 1;	//	ZOUNDS! BRILLIANT! Thief not Collide with tPlayer if not stealing!
							// NO! VERY DUMB! makes thief look very stupid if tPlayer hits him while cloaked!-AP
		else
			xLastThiefHitTime = gameData.time.xGame;
		}
	CreateAwarenessEvent (playerObjP, PA_PLAYER_COLLISION);			// tObject robot can attract attention to tPlayer
	DoAiRobotHitAttack (robot, playerObjP, vHitPt);
	DoAiRobotHit (robot, PA_WEAPON_ROBOT_COLLISION);
	} 
else
	MultiRobotRequestChange (robot, playerObjP->id);
// added this if to remove the bump sound if it's the thief.
// A "steal" sound was added and it was getting obscured by the bump. -AP 10/3/95
//	Changed by MK to make this sound unless the robot stole.
if (!(bTheftAttempt || ROBOTINFO (robot->id).energyDrain))
	DigiLinkSoundToPos (SOUND_ROBOT_HIT_PLAYER, playerObjP->nSegment, 0, vHitPt, 0, F1_0);
BumpTwoObjects (robot, playerObjP, 1, vHitPt);
return 1; 
}

//	-----------------------------------------------------------------------------
// Provide a way for network message to instantly destroy the control center
// without awarding points or anything.

//	if controlcen == NULL, that means don't do the explosion because the control center
//	was actually in another tObject.
void NetDestroyReactor (tObject *controlcen)
{
if (gameData.reactor.bDestroyed != 1) {
	DoReactorDestroyedStuff (controlcen);
	if ((controlcen != NULL) && !(controlcen->flags& (OF_EXPLODING|OF_DESTROYED))) {
		DigiLinkSoundToPos (SOUND_CONTROL_CENTER_DESTROYED, controlcen->nSegment, 0, &controlcen->position.vPos, 0, F1_0);
		ExplodeObject (controlcen, 0);
		}
	}
}

//	-----------------------------------------------------------------------------

void ApplyDamageToReactor (tObject *controlcen, fix damage, short who)
{
	int	whotype, i;

	//	Only allow a tPlayer to damage the control center.

if ((who < 0) || (who > gameData.objs.nLastObject))
	return;
whotype = gameData.objs.objects [who].nType;
if (whotype != OBJ_PLAYER) {
#if TRACE
	con_printf (CONDBG, "Damage to control center by tObject of nType %i prevented by MK! \n", whotype);
#endif
	return;
	}
if ((gameData.app.nGameMode & GM_MULTI) && !(gameData.app.nGameMode & GM_MULTI_COOP) && 
	 (LOCALPLAYER.timeLevel < netGame.control_invulTime)) {
	if (gameData.objs.objects [who].id == gameData.multiplayer.nLocalPlayer) {
		int t = netGame.control_invulTime - LOCALPLAYER.timeLevel;
		int secs = f2i (t) % 60;
		int mins = f2i (t) / 60;
		HUDInitMessage ("%s %d:%02d.", TXT_CNTRLCEN_INVUL, mins, secs);
		}
	return;
	}
if (gameData.objs.objects [who].id == gameData.multiplayer.nLocalPlayer) {
	if (0 >= (i = FindReactor (controlcen)))
		gameData.reactor.states [i].bHit = 1;
	AIDoCloakStuff ();
	}
if (controlcen->shields >= 0)
	controlcen->shields -= damage;
if ((controlcen->shields < 0) && !(controlcen->flags & (OF_EXPLODING | OF_DESTROYED))) {
	/*if (gameStates.app.bD2XLevel && gameStates.gameplay.bMultiBosses)*/
		extraGameInfo [0].nBossCount--;
	DoReactorDestroyedStuff (controlcen);
	if (gameData.app.nGameMode & GM_MULTI) {
		if (who == LOCALPLAYER.nObject)
			AddPointsToScore (CONTROL_CEN_SCORE);
		MultiSendDestroyReactor (OBJ_IDX (controlcen), gameData.objs.objects [who].id);
		}
	if (!(gameData.app.nGameMode & GM_MULTI))
		AddPointsToScore (CONTROL_CEN_SCORE);
	DigiLinkSoundToPos (SOUND_CONTROL_CENTER_DESTROYED, controlcen->nSegment, 0, &controlcen->position.vPos, 0, F1_0);
	ExplodeObject (controlcen, 0);
	}
}

//	-----------------------------------------------------------------------------

int CollidePlayerAndReactor (tObject * controlcen, tObject * playerObjP, vmsVector *vHitPt)
{ 
	int	i;

if (playerObjP->id == gameData.multiplayer.nLocalPlayer) {
	if (0 >= (i = FindReactor (controlcen)))
		gameData.reactor.states [i].bHit = 1;
	AIDoCloakStuff ();				//	In case tPlayer cloaked, make control center know where he is.
	}
if (BumpTwoObjects (controlcen, playerObjP, 1, vHitPt))
	DigiLinkSoundToPos (SOUND_ROBOT_HIT_PLAYER, playerObjP->nSegment, 0, vHitPt, 0, F1_0);
return 1; 
}

//	-----------------------------------------------------------------------------

int CollidePlayerAndMarker (tObject * marker, tObject * playerObjP, vmsVector *vHitPt)
{ 
#if TRACE
con_printf (CONDBG, "Collided with marker %d! \n", marker->id);
#endif
if (playerObjP->id==gameData.multiplayer.nLocalPlayer) {
	int drawn;

	if (gameData.app.nGameMode & GM_MULTI)
		drawn = HUDInitMessage (TXT_MARKER_PLRMSG, gameData.multiplayer.players [marker->id/2].callsign, gameData.marker.szMessage [marker->id]);
	else
		if (gameData.marker.szMessage [marker->id][0])
			drawn = HUDInitMessage (TXT_MARKER_IDMSG, marker->id+1, gameData.marker.szMessage [marker->id]);
		else
			drawn = HUDInitMessage (TXT_MARKER_ID, marker->id+1);
	if (drawn)
		DigiPlaySample (SOUND_MARKER_HIT, F1_0);
	DetectEscortGoalAccomplished (OBJ_IDX (marker));
   }
return 1;
}

//	-----------------------------------------------------------------------------
//	If a persistent weapon and other object is not a weapon, weaken it, else kill it.
//	If both gameData.objs.objects are weapons, weaken the weapon.
void MaybeKillWeapon (tObject *weaponP, tObject *otherObjP)
{
if ((weaponP->id == PROXMINE_ID) || (weaponP->id == SMARTMINE_ID) || (weaponP->id == SMALLMINE_ID)) {
	KillObject (weaponP);
	return;
	}
//	Changed, 10/12/95, MK: Make weaponP-weaponP collisions always kill both weapons if not persistent.
//	Reason: Otherwise you can't use proxbombs to detonate incoming homing missiles (or mega missiles).
if (weaponP->mType.physInfo.flags & PF_PERSISTENT) {
	//	Weapons do a lot of damage to weapons, other gameData.objs.objects do much less.
	if (!(otherObjP->mType.physInfo.flags & PF_PERSISTENT)) {
		weaponP->shields -= otherObjP->shields / ((otherObjP->nType == OBJ_WEAPON) ? 2 : 4);
		if (weaponP->shields <= 0) {
			weaponP->shields = 0;
			KillObject (weaponP);	// weaponP->lifeleft = 1;
			}
		}
	} 
else {
	KillObject (weaponP);	// weaponP->lifeleft = 1;
	}
}

//	-----------------------------------------------------------------------------

int CollideWeaponAndReactor (tObject * weaponP, tObject *controlcen, vmsVector *vHitPt)
{
	int	i;

if (weaponP->id == OMEGA_ID)
	if (!OkToDoOmegaDamage (weaponP))
		return 1;
if (weaponP->cType.laserInfo.parentType == OBJ_PLAYER) {
	fix	damage = weaponP->shields;
	if (gameData.objs.objects [weaponP->cType.laserInfo.nParentObj].id == gameData.multiplayer.nLocalPlayer)
		if (0 <= (i = FindReactor (controlcen)))
			gameData.reactor.states [i].bHit = 1;
	if (WI_damage_radius (weaponP->id))
		ExplodeBadassWeapon (weaponP, vHitPt);
	else
		ObjectCreateExplosion (controlcen->nSegment, vHitPt, controlcen->size*3/20, VCLIP_SMALL_EXPLOSION);
	DigiLinkSoundToPos (SOUND_CONTROL_CENTER_HIT, controlcen->nSegment, 0, vHitPt, 0, F1_0);
	damage = FixMul (damage, weaponP->cType.laserInfo.multiplier);
	ApplyDamageToReactor (controlcen, damage, weaponP->cType.laserInfo.nParentObj);
	MaybeKillWeapon (weaponP, controlcen);
	} 
else {	//	If robot weaponP hits control center, blow it up, make it go away, but do no damage to control center.
	ObjectCreateExplosion (controlcen->nSegment, vHitPt, controlcen->size*3/20, VCLIP_SMALL_EXPLOSION);
	MaybeKillWeapon (weaponP, controlcen);
	}
return 1;
}

//	-----------------------------------------------------------------------------

int CollideWeaponAndClutter (tObject * weaponP, tObject *clutter, vmsVector *vHitPt)	
{
ubyte exp_vclip = VCLIP_SMALL_EXPLOSION;
if (clutter->shields >= 0)
	clutter->shields -= weaponP->shields;
DigiLinkSoundToPos (SOUND_LASER_HIT_CLUTTER, (short) weaponP->nSegment, 0, vHitPt, 0, F1_0);
ObjectCreateExplosion ((short) clutter->nSegment, vHitPt, ((clutter->size/3)*3)/4, exp_vclip);
if ((clutter->shields < 0) && !(clutter->flags & (OF_EXPLODING|OF_DESTROYED)))
	ExplodeObject (clutter, STANDARD_EXPL_DELAY);
MaybeKillWeapon (weaponP, clutter);
return 1;
}

//--mk, 121094 -- extern void spinRobot (tObject *robot, vmsVector *vHitPt);

extern tObject *ExplodeBadassObject (tObject *objP, fix damage, fix distance, fix vForce);

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
GrPaletteFadeOut (NULL, 256, 0);
StartEndLevelSequence (0);		//pretend we hit the exit tTrigger
}

//	------------------------------------------------------------------------------------------------------
//	This is all the ugly stuff we do when you kill the final boss so that you don't die or something
//	which would ruin the logic of the cut sequence.
void DoFinalBossHacks (void)
{
if (gameStates.app.bPlayerIsDead) {
	Int3 ();		//	Uh-oh, tPlayer is dead.  Try to rescue him.
	gameStates.app.bPlayerIsDead = 0;
	}
if (LOCALPLAYER.shields <= 0)
	LOCALPLAYER.shields = 1;
//	If you're not invulnerable, get invulnerable!
if (!(LOCALPLAYER.flags & PLAYER_FLAGS_INVULNERABLE)) {
	LOCALPLAYER.invulnerableTime = gameData.time.xGame;
	LOCALPLAYER.flags |= PLAYER_FLAGS_INVULNERABLE;
	SetSpherePulse (gameData.multiplayer.spherePulse + gameData.multiplayer.nLocalPlayer, 0.02f, 0.5f);
	}
if (!(gameData.app.nGameMode & GM_MULTI))
	BuddyMessage ("Nice job, %s!", LOCALPLAYER.callsign);
gameStates.gameplay.bFinalBossIsDead = 1;
}

extern int MultiAllPlayersAlive ();
void MultiSendFinishGame ();

//	------------------------------------------------------------------------------------------------------
//	Return 1 if robot died, else return 0
int ApplyDamageToRobot (tObject *robotP, fix damage, int nKillerObj)
{
	char		bIsThief, bIsBoss;
	char		tempStolen [MAX_STOLEN_ITEMS];
	tObject	*killerObjP = (nKillerObj < 0) ? NULL : gameData.objs.objects + nKillerObj;

if (robotP->flags & OF_EXPLODING) 
	return 0;
if (robotP->shields < 0) 
	return 0;	//robotP already dead...
if (gameData.time.xGame - gameData.objs.xCreationTime [OBJ_IDX (robotP)] < F1_0)
	return 0;
if (!(gameStates.app.cheats.bRobotsKillRobots || EGI_FLAG (bRobotsHitRobots, 0, 0, 0))) {
	// guidebot may kill other bots
	if (killerObjP && (killerObjP->nType == OBJ_ROBOT) && !ROBOTINFO (killerObjP->id).companion)
		return 0;
	}
if ((bIsBoss = ROBOTINFO (robotP->id).bossFlag)) {
	int i = FindBoss (OBJ_IDX (robotP));
	if (i >= 0)
		gameData.boss [i].nHitTime = gameData.time.xGame;
	}

//	Buddy invulnerable on level 24 so he can give you his important messages.  Bah.
//	Also invulnerable if his cheat for firing weapons is in effect.
if (ROBOTINFO (robotP->id).companion) {
//		if ((gameData.missions.nCurrentMission == gameData.missions.nBuiltinMission && gameData.missions.nCurrentLevel == gameData.missions.nLastLevel) || gameStates.app.cheats.bMadBuddy)
	if ((gameData.missions.nCurrentMission == gameData.missions.nBuiltinMission && gameData.missions.nCurrentLevel == gameData.missions.nLastLevel))
		return 0;
	}
robotP->shields -= damage;
//	Do unspeakable hacks to make sure tPlayer doesn't die after killing boss.  Or before, sort of.
if (bIsBoss) {
	if ((gameData.missions.nCurrentMission == gameData.missions.nBuiltinMission) && 
		 (gameData.missions.nCurrentLevel == gameData.missions.nLastLevel)) {
		if ((robotP->shields < 0) && (extraGameInfo [0].nBossCount == 1)) {
			if (gameData.app.nGameMode & GM_MULTI) {
				if (!MultiAllPlayersAlive ()) // everyones gotta be alive
					robotP->shields = 1;
				else {
					MultiSendFinishGame ();
					DoFinalBossHacks ();
					}		
				}		
			else {	// NOTE LINK TO ABOVE!!!
				if ((LOCALPLAYER.shields < 0) || gameStates.app.bPlayerIsDead)
					robotP->shields = 1;		//	Sorry, we can't allow you to kill the final boss after you've died.  Rough luck.
				else
					DoFinalBossHacks ();
				}
			}
		}
	}

if (robotP->shields >= 0) {
	if (killerObjP == gameData.objs.console)
		ExecObjTriggers (OBJ_IDX (robotP), 1);
	return 0;
	}
if (IsMultiGame) {
	bIsThief = (ROBOTINFO (robotP->id).thief != 0);
	if (bIsThief)
		memcpy (tempStolen, gameData.thief.stolenItems, sizeof (*tempStolen) * MAX_STOLEN_ITEMS);
	if (MultiExplodeRobotSub (OBJ_IDX (robotP), nKillerObj, ROBOTINFO (robotP->id).thief)) {
		if (bIsThief)	
			memcpy (gameData.thief.stolenItems, tempStolen, sizeof (*tempStolen) * MAX_STOLEN_ITEMS);
		MultiSendRobotExplode (OBJ_IDX (robotP), nKillerObj, ROBOTINFO (robotP->id).thief);
		if (bIsThief)	
			memset (gameData.thief.stolenItems, 255, sizeof (gameData.thief.stolenItems));
		return 1;
		}
	else
		return 0;
	}

if (nKillerObj >= 0) {
	LOCALPLAYER.numKillsLevel++;
	LOCALPLAYER.numKillsTotal++;
	}

if (bIsBoss)
	StartBossDeathSequence (robotP);	//DoReactorDestroyedStuff (NULL);
else if (ROBOTINFO (robotP->id).bDeathRoll)
	StartRobotDeathSequence (robotP);	//DoReactorDestroyedStuff (NULL);
else {
	if (robotP->id == SPECIAL_REACTOR_ROBOT)
		SpecialReactorStuff ();
	ExplodeObject (robotP, ROBOTINFO (robotP->id).kamikaze ? 1 : STANDARD_EXPL_DELAY);		//	Kamikaze, explode right away, IN YOUR FACE!
	}
return 1;
}

//	------------------------------------------------------------------------------------------------------

extern int BossSpewRobot (tObject *objP, vmsVector *pos, short objType);

int	nBossInvulDot = 0;
int	nBuddyGaveHintCount = 5;
fix	xLastTimeBuddyGameHint = 0;

//	Return true if damage done to boss, else return false.
int DoBossWeaponCollision (tObject *robot, tObject *weapon, vmsVector *vHitPt)
{
	int	d2BossIndex;
	int	bDamage = 1;
	int	bKinetic = WI_matter (weapon->id);

d2BossIndex = ROBOTINFO (robot->id).bossFlag - BOSS_D2;
Assert ((d2BossIndex >= 0) && (d2BossIndex < NUM_D2_BOSSES));

//	See if should spew a bot.
if (weapon->cType.laserInfo.parentType == OBJ_PLAYER) {
	if ((bKinetic && bossProps [gameStates.app.bD1Mission][d2BossIndex].bSpewBotsKinetic) || 
		 (!bKinetic && bossProps [gameStates.app.bD1Mission][d2BossIndex].bSpewBotsEnergy)) {
		int i = FindBoss (OBJ_IDX (robot));
		if (i >= 0) {
			if (bossProps [gameStates.app.bD1Mission][d2BossIndex].bSpewMore && (d_rand () > 16384) &&
				 (BossSpewRobot (robot, vHitPt, -1) != -1))
				gameData.boss [i].nLastGateTime = gameData.time.xGame - gameData.boss [i].nGateInterval - 1;	//	Force allowing spew of another bot.
			BossSpewRobot (robot, vHitPt, -1);
			}
		}
	}

if (bossProps [gameStates.app.bD1Mission][d2BossIndex].bInvulSpot) {
	fix			dot;
	vmsVector	tvec1;

	//	Boss only vulnerable in back.  See if hit there.
	VmVecSub (&tvec1, vHitPt, &robot->position.vPos);
	VmVecNormalizeQuick (&tvec1);	//	Note, if BOSS_INVULNERABLE_DOT is close to F1_0 (in magnitude), then should probably use non-quick version.
	dot = VmVecDot (&tvec1, &robot->position.mOrient.fVec);
#if TRACE
	con_printf (CONDBG, "Boss hit vec dot = %7.3f \n", f2fl (dot));
#endif
	if (dot > nBossInvulDot) {
		short	nNewObj;
		short	nSegment;

		nSegment = FindSegByPoint (vHitPt, robot->nSegment);
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
		//	Make a copy of this weapon, because the physics wants to destroy it.
		if (!WI_matter (weapon->id)) {
			nNewObj = CreateObject (weapon->nType, weapon->id, -1, weapon->nSegment, &weapon->position.vPos, 
											&weapon->position.mOrient, weapon->size, weapon->controlType, weapon->movementType, weapon->renderType, 1);

			if (nNewObj != -1) {
				vmsVector	vec_to_point;
				vmsVector	weap_vec;
				fix			speed;
				tObject		*newObjP = gameData.objs.objects + nNewObj;

				if (weapon->renderType == RT_POLYOBJ) {
					newObjP->rType.polyObjInfo.nModel = gameData.weapons.info [newObjP->id].nModel;
					newObjP->size = FixDiv (gameData.models.polyModels [newObjP->rType.polyObjInfo.nModel].rad, gameData.weapons.info [newObjP->id].po_len_to_width_ratio);
				}

				newObjP->mType.physInfo.mass = WI_mass (weapon->nType);
				newObjP->mType.physInfo.drag = WI_drag (weapon->nType);
				VmVecZero (&newObjP->mType.physInfo.thrust);

				VmVecSub (&vec_to_point, vHitPt, &robot->position.vPos);
				VmVecNormalizeQuick (&vec_to_point);
				weap_vec = weapon->mType.physInfo.velocity;
				speed = VmVecNormalizeQuick (&weap_vec);
				VmVecScaleInc (&vec_to_point, &weap_vec, -F1_0*2);
				VmVecScale (&vec_to_point, speed/4);
				newObjP->mType.physInfo.velocity = vec_to_point;
				}
			}
		}
	} 
else if ((bKinetic && bossProps [gameStates.app.bD1Mission][d2BossIndex].bInvulKinetic) || 
		   (!bKinetic && bossProps [gameStates.app.bD1Mission][d2BossIndex].bInvulEnergy)) {
	short	nSegment;

	nSegment = FindSegByPoint (vHitPt, robot->nSegment);
	DigiLinkSoundToPos (SOUND_WEAPON_HIT_DOOR, nSegment, 0, vHitPt, 0, F1_0);
	bDamage = 0;
	}
return bDamage;
}

//	------------------------------------------------------------------------------------------------------

int FindHitObject (tObject *objP, short nObject)
{
	short	*p = gameData.objs.nHitObjects + OBJ_IDX (objP) * MAX_HIT_OBJECTS;
	int	i;
	
for (i = objP->cType.laserInfo.nLastHitObj; i; i--, p++)
	if (*p == nObject)
		return 1;
return 0;
}

//	------------------------------------------------------------------------------------------------------

int AddHitObject (tObject *objP, short nObject)
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

int CollideRobotAndWeapon (tObject *robotP, tObject *weaponP, vmsVector *vHitPt)
{ 
	int	bDamage = 1;
	int	bInvulBoss = 0;
	fix	nStrength = WI_strength (weaponP->id, gameStates.app.nDifficultyLevel);
	tRobotInfo *botInfoP = &ROBOTINFO (robotP->id);
	tWeaponInfo *wInfoP = gameData.weapons.info + weaponP->id;

if ((weaponP->id == PROXMINE_ID) && !COMPETITION && EGI_FLAG (bSmokeGrenades, 0, 0, 0))
	return 1;		
if (weaponP->id == OMEGA_ID)
	if (!OkToDoOmegaDamage (weaponP))
		return 1;
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
if ((botInfoP->companion) && 
	 (weaponP->cType.laserInfo.parentType == OBJ_ROBOT) && 
	  !gameStates.app.cheats.bRobotsKillRobots)
	return 1;
if (weaponP->id == EARTHSHAKER_ID)
	ShakerRockStuff ();
//	If a persistent weaponP hit robotP most recently, quick abort, else we cream the same robotP many times, 
//	depending on frame rate.
if (weaponP->mType.physInfo.flags & PF_PERSISTENT) {
	if (AddHitObject (weaponP, OBJ_IDX (robotP)) < 0)
		return 1;
	}
if (weaponP->cType.laserInfo.nParentSig == robotP->nSignature)
	return 1;
//	Changed, 10/04/95, put out blobs based on skill level and power of weaponP doing damage.
//	Also, only a weaponP hit from a tPlayer weaponP causes smart blobs.
if ((weaponP->cType.laserInfo.parentType == OBJ_PLAYER) && botInfoP->energyBlobs)
	if ((robotP->shields > 0) && bIsEnergyWeapon [weaponP->id]) {
		int	num_blobs;
		fix	probval = (gameStates.app.nDifficultyLevel+2) * min (weaponP->shields, robotP->shields);
		probval = botInfoP->energyBlobs * probval/ (NDL*32);
		num_blobs = probval >> 16;
		if (2*d_rand () < (probval & 0xffff))
			num_blobs++;
		if (num_blobs)
			CreateSmartChildren (robotP, num_blobs);
		}

	//	Note: If weaponP hits an invulnerable boss, it will still do badass damage, including to the boss, 
	//	unless this is trapped elsewhere.
	if (WI_damage_radius (weaponP->id)) {
		if (bInvulBoss) {			//don't make badass sound
			//this code copied from ExplodeBadassWeapon ()
			ObjectCreateBadassExplosion (weaponP, weaponP->nSegment, vHitPt, 
							wInfoP->impact_size, 
							wInfoP->robot_hit_vclip, 
							nStrength, 
							wInfoP->damage_radius, 
							nStrength,
							weaponP->cType.laserInfo.nParentObj);
		
			}
		else		//normal badass explosion
			ExplodeBadassWeapon (weaponP, vHitPt);
		}
	if (((weaponP->cType.laserInfo.parentType == OBJ_PLAYER) || 
		 ((weaponP->cType.laserInfo.parentType == OBJ_ROBOT) &&
		  (gameStates.app.cheats.bRobotsKillRobots || EGI_FLAG (bRobotsHitRobots, 0, 0, 0)))) && 
		 !(robotP->flags & OF_EXPLODING)) {	
		tObject *explObjP = NULL;
		if (weaponP->cType.laserInfo.nParentObj == LOCALPLAYER.nObject) {
			CreateAwarenessEvent (weaponP, PA_WEAPON_ROBOT_COLLISION);			// tObject "weaponP" can attract attention to tPlayer
			DoAiRobotHit (robotP, PA_WEAPON_ROBOT_COLLISION);
			}
	  	else
			MultiRobotRequestChange (robotP, gameData.objs.objects [weaponP->cType.laserInfo.nParentObj].id);
		if (botInfoP->nExp1VClip > -1)
			explObjP = ObjectCreateExplosion (weaponP->nSegment, vHitPt, (robotP->size/2*3)/4, (ubyte) botInfoP->nExp1VClip);
		else if (gameData.weapons.info [weaponP->id].robot_hit_vclip > -1)
			explObjP = ObjectCreateExplosion (weaponP->nSegment, vHitPt, wInfoP->impact_size, (ubyte) wInfoP->robot_hit_vclip);
		if (explObjP)
			AttachObject (robotP, explObjP);
		if (bDamage && (botInfoP->nExp1Sound > -1))
			DigiLinkSoundToPos (botInfoP->nExp1Sound, robotP->nSegment, 0, vHitPt, 0, F1_0);
		if (!(weaponP->flags & OF_HARMLESS)) {
			fix damage = weaponP->shields;
			if (bDamage)
				damage = FixMul (damage, weaponP->cType.laserInfo.multiplier);
			else
				damage = 0;
			//	Cut Gauss damage on bosses because it just breaks the game.  Bosses are so easy to
			//	hit, and missing a robotP is what prevents the Gauss from being game-breaking.
			if (weaponP->id == GAUSS_ID) {
				if (botInfoP->bossFlag)
					damage = damage * (2 * NDL - gameStates.app.nDifficultyLevel) / (2 * NDL);
				}
			else if (!COMPETITION && gameStates.app.bHaveExtraGameInfo [IsMultiGame] && (weaponP->id == FUSION_ID))
				damage *= extraGameInfo [IsMultiGame].nFusionPowerMod / 2;
			if (!ApplyDamageToRobot (robotP, damage, weaponP->cType.laserInfo.nParentObj))
				BumpTwoObjects (robotP, weaponP, 0, vHitPt);		//only bump if not dead. no damage from bump
			else if (weaponP->cType.laserInfo.nParentSig == gameData.objs.console->nSignature) {
				AddPointsToScore (botInfoP->scoreValue);
				DetectEscortGoalAccomplished (OBJ_IDX (robotP));
				}
			}
		//	If Gauss Cannon, spin robotP.
		if (robotP && !(botInfoP->companion || botInfoP->bossFlag) && (weaponP->id == GAUSS_ID)) {
			tAIStatic	*aip = &robotP->cType.aiInfo;

			if (aip->SKIP_AI_COUNT * gameData.time.xFrame < F1_0) {
				aip->SKIP_AI_COUNT++;
				robotP->mType.physInfo.rotThrust.p.x = FixMul ((d_rand () - 16384), gameData.time.xFrame * aip->SKIP_AI_COUNT);
				robotP->mType.physInfo.rotThrust.p.y = FixMul ((d_rand () - 16384), gameData.time.xFrame * aip->SKIP_AI_COUNT);
				robotP->mType.physInfo.rotThrust.p.z = FixMul ((d_rand () - 16384), gameData.time.xFrame * aip->SKIP_AI_COUNT);
				robotP->mType.physInfo.flags |= PF_USES_THRUST;
				}
			}
		}
MaybeKillWeapon (weaponP, robotP);
return 1; 
}

//##void CollideRobotAndCamera (tObject * robot, tObject * camera, vmsVector *vHitPt) { 
//##	return; 
//##}

//##void CollideRobotAndPowerup (tObject * robot, tObject * powerup, vmsVector *vHitPt) { 
//##	return; 
//##}

//##void CollideRobotAndDebris (tObject * robot, tObject * debris, vmsVector *vHitPt) { 
//##	return; 
//##}

//##void CollideHostageAndHostage (tObject * hostage1, tObject * hostage2, vmsVector *vHitPt) { 
//##	return; 
//##}

//	-----------------------------------------------------------------------------

int CollideHostageAndPlayer (tObject * hostage, tObject * tPlayer, vmsVector *vHitPt) 
{ 
	// Give tPlayer points, etc.
if (tPlayer == gameData.objs.console) {
	DetectEscortGoalAccomplished (OBJ_IDX (hostage));
	AddPointsToScore (HOSTAGE_SCORE);
	// Do effect
	hostage_rescue (hostage->id);
	// Remove the hostage tObject.
	KillObject (hostage);
	if (gameData.app.nGameMode & GM_MULTI)
		MultiSendRemObj (OBJ_IDX (hostage));
	}
return 1; 
}

//--unused-- void CollideHostageAndWeapon (tObject * hostage, tObject * weapon, vmsVector *vHitPt)
//--unused-- { 
//--unused-- 	//	Cannot kill hostages, as per Matt's edict!
//--unused-- 	//	 (A fine edict, but in contradiction to the milestone: "Robots attack hostages.")
//--unused-- 	hostage->shields -= weapon->shields/2;
//--unused-- 
//--unused-- 	CreateAwarenessEvent (weapon, PA_WEAPON_ROBOT_COLLISION);			// tObject "weapon" can attract attention to tPlayer
//--unused-- 
//--unused-- 	//PLAY_SOUND_3D (SOUND_HOSTAGE_KILLED, vHitPt, hostage->nSegment);
//--unused-- 	DigiLinkSoundToPos (SOUND_HOSTAGE_KILLED, hostage->nSegment , 0, vHitPt, 0, F1_0);
//--unused-- 
//--unused-- 
//--unused-- 	if (hostage->shields <= 0) {
//--unused-- 		ExplodeObject (hostage, 0);
//--unused-- 		KillObject (hostage);
//--unused-- 	}
//--unused-- 
//--unused-- 	if (WI_damage_radius (weapon->id))
//--unused-- 		ExplodeBadassWeapon (weapon);
//--unused-- 
//--unused-- 	MaybeKillWeapon (weapon, hostage);
//--unused-- 
//--unused-- }

//##void CollideHostageAndCamera (tObject * hostage, tObject * camera, vmsVector *vHitPt) { 
//##	return; 
//##}

//##void CollideHostageAndPowerup (tObject * hostage, tObject * powerup, vmsVector *vHitPt) { 
//##	return; 
//##}

//##void CollideHostageAndDebris (tObject * hostage, tObject * debris, vmsVector *vHitPt) { 
//##	return; 
//##}

//	-----------------------------------------------------------------------------

int CollidePlayerAndPlayer (tObject * player1, tObject * player2, vmsVector *vHitPt) 
{ 
if (gameStates.app.bD2XLevel && 
	 (gameData.segs.segment2s [player1->nSegment].special == SEGMENT_IS_NODAMAGE))
	return 1;
if (BumpTwoObjects (player1, player2, 1, vHitPt))
	DigiLinkSoundToPos (SOUND_ROBOT_HIT_PLAYER, player1->nSegment, 0, vHitPt, 0, F1_0);
return 1;
}

//	-----------------------------------------------------------------------------

int MaybeDropPrimaryWeaponEgg (tObject *playerObjP, int weapon_index)
{
	int weaponFlag = HAS_FLAG (weapon_index);
	int nPowerup = primaryWeaponToPowerup [weapon_index];

if (gameData.multiplayer.players [playerObjP->id].primaryWeaponFlags & weaponFlag)
	return CallObjectCreateEgg (playerObjP, 1, OBJ_POWERUP, nPowerup);
else
	return -1;
}

//	-----------------------------------------------------------------------------

void MaybeDropSecondaryWeaponEgg (tObject *playerObjP, int weapon_index, int count)
{
	int weaponFlag = HAS_FLAG (weapon_index);
	int nPowerup = secondaryWeaponToPowerup [weapon_index];

if (gameData.multiplayer.players [playerObjP->id].secondaryWeaponFlags & weaponFlag) {
	int	i, maxCount = (EGI_FLAG (bDropAllMissiles, 0, 0, 0)) ? count : min (count, 3);
	for (i=0; i<maxCount; i++)
		CallObjectCreateEgg (playerObjP, 1, OBJ_POWERUP, nPowerup);
	}
}

//	-----------------------------------------------------------------------------

void DropMissile1or4 (tObject *playerObjP, int nMissileIndex)
{
	int nMissiles, nPowerupId;

if ((nMissiles = gameData.multiplayer.players [playerObjP->id].secondaryAmmo [nMissileIndex])) {
	nPowerupId = secondaryWeaponToPowerup [nMissileIndex];
	if (!(IsMultiGame || EGI_FLAG (bDropAllMissiles, 0, 0, 0)) && (nMissiles > 10))
		nMissiles = 10;
	CallObjectCreateEgg (playerObjP, nMissiles/4, OBJ_POWERUP, nPowerupId+1);
	CallObjectCreateEgg (playerObjP, nMissiles%4, OBJ_POWERUP, nPowerupId);
	}
}

// -- int	Items_destroyed = 0;

//	-----------------------------------------------------------------------------

void DropPlayerEggs (tObject *playerObjP)
{
if ((playerObjP->nType == OBJ_PLAYER) || (playerObjP->nType == OBJ_GHOST)) {
	int			rthresh;
	int			nPlayerId = playerObjP->id;
	short			nObject, plrObjNum = OBJ_IDX (playerObjP);
	int			nVulcanAmmo=0;
	vmsVector	vRandom;
	tPlayer		*playerP = gameData.multiplayer.players + nPlayerId;

	// -- Items_destroyed = 0;

	// Seed the random number generator so in net play the eggs will always
	// drop the same way
	if (gameData.app.nGameMode & GM_MULTI) {
		gameData.multigame.create.nLoc = 0;
		d_srand (5483L);
	}

	//	If the tPlayer had smart mines, maybe arm one of them.
	rthresh = 30000;
	while ((playerP->secondaryAmmo [SMARTMINE_INDEX]%4==1) && (d_rand () < rthresh)) {
		short			newseg;
		vmsVector	tvec;

		MakeRandomVector (&vRandom);
		rthresh /= 2;
		VmVecAdd (&tvec, &playerObjP->position.vPos, &vRandom);
		newseg = FindSegByPoint (&tvec, playerObjP->nSegment);
		if (newseg != -1)
			CreateNewLaser (&vRandom, &tvec, newseg, plrObjNum, SMARTMINE_ID, 0);
	  	}

		//	If the tPlayer had proximity bombs, maybe arm one of them.
		if ((gameData.app.nGameMode & GM_MULTI) && !(gameData.app.nGameMode & (GM_HOARD | GM_ENTROPY))) {
			rthresh = 30000;
			while ((playerP->secondaryAmmo [PROXMINE_INDEX] % 4 == 1) && (d_rand () < rthresh)) {
				short			newseg;
				vmsVector	tvec;
	
				MakeRandomVector (&vRandom);
				rthresh /= 2;
				VmVecAdd (&tvec, &playerObjP->position.vPos, &vRandom);
				newseg = FindSegByPoint (&tvec, playerObjP->nSegment);
				if (newseg != -1)
					CreateNewLaser (&vRandom, &tvec, newseg, plrObjNum, PROXMINE_ID, 0);
			}
		}

		//	If the tPlayer dies and he has powerful lasers, create the powerups here.

		if (playerP->laserLevel > MAX_LASER_LEVEL)
			CallObjectCreateEgg (playerObjP, playerP->laserLevel-MAX_LASER_LEVEL, OBJ_POWERUP, POW_SUPERLASER);
		else if (playerP->laserLevel >= 1)
			CallObjectCreateEgg (playerObjP, playerP->laserLevel, OBJ_POWERUP, POW_LASER);	// Note: laserLevel = 0 for laser level 1.

		//	Drop quad laser if appropos
		if (playerP->flags & PLAYER_FLAGS_QUAD_LASERS)
			CallObjectCreateEgg (playerObjP, 1, OBJ_POWERUP, POW_QUADLASER);
		if (playerP->flags & PLAYER_FLAGS_CLOAKED)
			CallObjectCreateEgg (playerObjP, 1, OBJ_POWERUP, POW_CLOAK);
		while (playerP->nInvuls--)
			CallObjectCreateEgg (playerObjP, 1, OBJ_POWERUP, POW_INVUL);
		while (playerP->nCloaks--)
			CallObjectCreateEgg (playerObjP, 1, OBJ_POWERUP, POW_CLOAK);
		if (playerP->flags & PLAYER_FLAGS_MAP_ALL)
			CallObjectCreateEgg (playerObjP, 1, OBJ_POWERUP, POW_FULL_MAP);
		if (playerP->flags & PLAYER_FLAGS_AFTERBURNER)
			CallObjectCreateEgg (playerObjP, 1, OBJ_POWERUP, POW_AFTERBURNER);
		if (playerP->flags & PLAYER_FLAGS_AMMO_RACK)
			CallObjectCreateEgg (playerObjP, 1, OBJ_POWERUP, POW_AMMORACK);
		if (playerP->flags & PLAYER_FLAGS_CONVERTER)
			CallObjectCreateEgg (playerObjP, 1, OBJ_POWERUP, POW_CONVERTER);
		if ((playerP->flags & PLAYER_FLAGS_HEADLIGHT) && 
			 !(gameStates.app.bHaveExtraGameInfo [1] && IsMultiGame && extraGameInfo [1].bDarkness))
			CallObjectCreateEgg (playerObjP, 1, OBJ_POWERUP, POW_HEADLIGHT);
		// drop the other enemies flag if you have it

	playerP->nInvuls =
	playerP->nCloaks = 0;
	playerP->flags &= ~(PLAYER_FLAGS_INVULNERABLE | PLAYER_FLAGS_CLOAKED);
	if ((gameData.app.nGameMode & GM_CAPTURE) && (playerP->flags & PLAYER_FLAGS_FLAG)) {
		if ((GetTeam (nPlayerId)==TEAM_RED))
			CallObjectCreateEgg (playerObjP, 1, OBJ_POWERUP, POW_BLUEFLAG);
		else
			CallObjectCreateEgg (playerObjP, 1, OBJ_POWERUP, POW_REDFLAG);
		}

#ifndef _DEBUG			
		if (gameData.app.nGameMode & (GM_HOARD | GM_ENTROPY))
#endif
		if ((gameData.app.nGameMode & GM_HOARD) || 
		    ((gameData.app.nGameMode & GM_ENTROPY) && extraGameInfo [1].entropy.nVirusStability)) {
			// Drop hoard orbs
			
			int maxCount, i;
#if TRACE
			con_printf (CONDBG, "HOARD MODE: Dropping %d orbs \n", playerP->secondaryAmmo [PROXMINE_INDEX]);
#endif	
			maxCount = playerP->secondaryAmmo [PROXMINE_INDEX];
			if ((gameData.app.nGameMode & GM_HOARD) && (maxCount > 12))
				maxCount = 12;
			for (i = 0; i < maxCount; i++)
				CallObjectCreateEgg (playerObjP, 1, OBJ_POWERUP, POW_HOARD_ORB);
		}

		//Drop the vulcan, gauss, and ammo
		nVulcanAmmo = playerP->primaryAmmo [VULCAN_INDEX];
		if ((playerP->primaryWeaponFlags & HAS_FLAG (VULCAN_INDEX)) && 
			 (playerP->primaryWeaponFlags & HAS_FLAG (GAUSS_INDEX)))
			nVulcanAmmo /= 2;		//if both vulcan & gauss, each gets half
		if (nVulcanAmmo < VULCAN_AMMO_AMOUNT)
			nVulcanAmmo = VULCAN_AMMO_AMOUNT;	//make sure gun has at least as much as a powerup
		nObject = MaybeDropPrimaryWeaponEgg (playerObjP, VULCAN_INDEX);
		if (nObject!=-1)
			gameData.objs.objects [nObject].cType.powerupInfo.count = nVulcanAmmo;
		nObject = MaybeDropPrimaryWeaponEgg (playerObjP, GAUSS_INDEX);
		if (nObject!=-1)
			gameData.objs.objects [nObject].cType.powerupInfo.count = nVulcanAmmo;

		//	Drop the rest of the primary weapons
		MaybeDropPrimaryWeaponEgg (playerObjP, SPREADFIRE_INDEX);
		MaybeDropPrimaryWeaponEgg (playerObjP, PLASMA_INDEX);
		MaybeDropPrimaryWeaponEgg (playerObjP, FUSION_INDEX);
		MaybeDropPrimaryWeaponEgg (playerObjP, HELIX_INDEX);
		MaybeDropPrimaryWeaponEgg (playerObjP, PHOENIX_INDEX);
		nObject = MaybeDropPrimaryWeaponEgg (playerObjP, OMEGA_INDEX);
		if (nObject!=-1)
			gameData.objs.objects [nObject].cType.powerupInfo.count = (playerObjP->id==gameData.multiplayer.nLocalPlayer)?gameData.laser.xOmegaCharge:MAX_OMEGA_CHARGE;
		//	Drop the secondary weapons
		//	Note, proximity weapon only comes in packets of 4.  So drop n/2, but a max of 3 (handled inside maybe_drop..)  Make sense?
		if (!(gameData.app.nGameMode & (GM_HOARD | GM_ENTROPY)))
			MaybeDropSecondaryWeaponEgg (playerObjP, PROXMINE_INDEX, (playerP->secondaryAmmo [PROXMINE_INDEX])/4);
		MaybeDropSecondaryWeaponEgg (playerObjP, SMART_INDEX, playerP->secondaryAmmo [SMART_INDEX]);
		MaybeDropSecondaryWeaponEgg (playerObjP, MEGA_INDEX, playerP->secondaryAmmo [MEGA_INDEX]);
		if (!(gameData.app.nGameMode & GM_ENTROPY))
			MaybeDropSecondaryWeaponEgg (playerObjP, SMARTMINE_INDEX, (playerP->secondaryAmmo [SMARTMINE_INDEX])/4);
		MaybeDropSecondaryWeaponEgg (playerObjP, EARTHSHAKER_INDEX, playerP->secondaryAmmo [EARTHSHAKER_INDEX]);
		//	Drop the tPlayer's missiles in packs of 1 and/or 4
		DropMissile1or4 (playerObjP, HOMING_INDEX);
		DropMissile1or4 (playerObjP, GUIDED_INDEX);
		DropMissile1or4 (playerObjP, CONCUSSION_INDEX);
		DropMissile1or4 (playerObjP, FLASHMSL_INDEX);
		DropMissile1or4 (playerObjP, MERCURY_INDEX);
		//	If tPlayer has vulcan ammo, but no vulcan cannon, drop the ammo.
		if (!(playerP->primaryWeaponFlags & HAS_VULCAN_FLAG)) {
			int	amount = playerP->primaryAmmo [VULCAN_INDEX];
			if (amount > 200) {
#if TRACE
				con_printf (CONDBG, "Surprising amount of vulcan ammo: %i bullets. \n", amount);
#endif
				amount = 200;
			}
			while (amount > 0) {
				CallObjectCreateEgg (playerObjP, 1, OBJ_POWERUP, POW_VULCAN_AMMO);
				amount -= VULCAN_AMMO_AMOUNT;
			}
		}

		//	Always drop a shield and energy powerup.
		if (gameData.app.nGameMode & GM_MULTI) {
			CallObjectCreateEgg (playerObjP, 1, OBJ_POWERUP, POW_SHIELD_BOOST);
			CallObjectCreateEgg (playerObjP, 1, OBJ_POWERUP, POW_ENERGY);
		}

//--		//	Drop all the keys.
//--		if (LOCALPLAYER.flags & PLAYER_FLAGS_BLUE_KEY) {
//--			playerObjP->containsCount = 1;
//--			playerObjP->containsType = OBJ_POWERUP;
//--			playerObjP->containsId = POW_KEY_BLUE;
//--			ObjectCreateEgg (playerObjP);
//--		}
//--		if (LOCALPLAYER.flags & PLAYER_FLAGS_RED_KEY) {
//--			playerObjP->containsCount = 1;
//--			playerObjP->containsType = OBJ_POWERUP;
//--			playerObjP->containsId = POW_KEY_RED;
//--			ObjectCreateEgg (playerObjP);
//--		}
//--		if (LOCALPLAYER.flags & PLAYER_FLAGS_GOLD_KEY) {
//--			playerObjP->containsCount = 1;
//--			playerObjP->containsType = OBJ_POWERUP;
//--			playerObjP->containsId = POW_KEY_GOLD;
//--			ObjectCreateEgg (playerObjP);
//--		}

// -- 		if (Items_destroyed) {
// -- 			if (Items_destroyed == 1)
// -- 				HUDInitMessage ("%i item was destroyed.", Items_destroyed);
// -- 			else
// -- 				HUDInitMessage ("%i items were destroyed.", Items_destroyed);
// -- 			Items_destroyed = 0;
// -- 		}
	}

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

void ApplyDamageToPlayer (tObject *playerObjP, tObject *killerObjP, fix damage)
{
tPlayer *playerP = gameData.multiplayer.players + playerObjP->id;
tPlayer *killerP = (killerObjP && (killerObjP->nType == OBJ_PLAYER)) ? gameData.multiplayer.players + killerObjP->id : NULL;
if (gameStates.app.bPlayerIsDead)
	return;

if (gameStates.app.bD2XLevel && (gameData.segs.segment2s [playerObjP->nSegment].special == SEGMENT_IS_NODAMAGE))
	return;
if (LOCALPLAYER.flags & PLAYER_FLAGS_INVULNERABLE)
	return;
if (killerObjP && (killerObjP->nType == OBJ_ROBOT) && ROBOTINFO (killerObjP->id).companion)
	return;
if (killerObjP == playerObjP) {
	if (!COMPETITION && gameStates.app.bHaveExtraGameInfo [1] && extraGameInfo [1].bInhibitSuicide)
		return;
	}
else if (killerP && gameStates.app.bHaveExtraGameInfo [1] && 
			!(COMPETITION || extraGameInfo [1].bFriendlyFire)) {
	if (gameData.app.nGameMode & GM_TEAM) {
		if (GetTeam (playerObjP->id) == GetTeam (killerObjP->id))
			return;
		}
	else if (gameData.app.nGameMode & GM_MULTI_COOP)
		return;
	}
if (gameStates.app.bEndLevelSequence)
	return;

gameData.multiplayer.bWasHit [playerObjP->id] = -1;
//for the tPlayer, the 'real' shields are maintained in the gameData.multiplayer.players []
//array.  The shields value in the tPlayer's tObject are, I think, not
//used anywhere.  This routine, however, sets the gameData.objs.objects shields to
//be a mirror of the value in the Player structure. 

if (playerObjP->id == gameData.multiplayer.nLocalPlayer) {		//is this the local tPlayer?
	if ((gameData.app.nGameMode & GM_ENTROPY)&& extraGameInfo [1].entropy.bPlayerHandicap && killerP) {
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
	PALETTE_FLASH_ADD (f2i (damage)*4, -f2i (damage/2), -f2i (damage/2));	//flash red
	if (playerP->shields < 0)	{
  		playerP->nKillerObj = OBJ_IDX (killerObjP);
		KillObject (playerObjP);
		if (gameData.escort.nObjNum != -1)
			if (killerObjP && (killerObjP->nType == OBJ_ROBOT) && (ROBOTINFO (killerObjP->id).companion))
				gameData.escort.xSorryTime = gameData.time.xGame;
		}
	playerObjP->shields = playerP->shields;		//mirror
	}
}

//	-----------------------------------------------------------------------------

int CollidePlayerAndWeapon (tObject * playerObjP, tObject * weapon, vmsVector *vHitPt)
{
	fix		damage = weapon->shields;
	tObject * killer = NULL;

	//	In multiplayer games, only do damage to another tPlayer if in first frame.
	//	This is necessary because in multiplayer, due to varying framerates, omega blobs actually
	//	have a bit of a lifetime.  But they start out with a lifetime of ONE_FRAME_TIME, and this
	//	gets bashed to 1/4 second in laser_doWeapon_sequence.  This bashing occurs for visual purposes only.
if (gameStates.app.bD2XLevel && (gameData.segs.segment2s [playerObjP->nSegment].special == SEGMENT_IS_NODAMAGE))
	return 1;
if ((weapon->id == PROXMINE_ID) && !COMPETITION && EGI_FLAG (bSmokeGrenades, 0, 0, 0))
	return 1;
if (weapon->id == OMEGA_ID)
	if (!OkToDoOmegaDamage (weapon))
		return 1;
//	Don't Collide own smart mines unless direct hit.
if (weapon->id == SMARTMINE_ID)
	if (OBJ_IDX (playerObjP) == weapon->cType.laserInfo.nParentObj)
		if (VmVecDistQuick (vHitPt, &playerObjP->position.vPos) > playerObjP->size)
			return 1;
if (weapon->id == EARTHSHAKER_ID)
	ShakerRockStuff ();
damage = FixMul (damage, weapon->cType.laserInfo.multiplier);
if (!COMPETITION && gameStates.app.bHaveExtraGameInfo [IsMultiGame] && (weapon->id == FUSION_ID))
	damage *= extraGameInfo [IsMultiGame].nFusionPowerMod / 2;
if (IsMultiGame)
	damage = FixMul (damage, gameData.weapons.info [weapon->id].multi_damage_scale);
if (weapon->mType.physInfo.flags & PF_PERSISTENT) {
	if (AddHitObject (weapon, OBJ_IDX (playerObjP)) < 0)
		return 1;
}
if (playerObjP->id == gameData.multiplayer.nLocalPlayer) {
	if (!(LOCALPLAYER.flags & PLAYER_FLAGS_INVULNERABLE)) {
		DigiLinkSoundToPos (SOUND_PLAYER_GOT_HIT, playerObjP->nSegment, 0, vHitPt, 0, F1_0);
		if (gameData.app.nGameMode & GM_MULTI)
			MultiSendPlaySound (SOUND_PLAYER_GOT_HIT, F1_0);
		}
	else {
		DigiLinkSoundToPos (SOUND_WEAPON_HIT_DOOR, playerObjP->nSegment, 0, vHitPt, 0, F1_0);
		if (gameData.app.nGameMode & GM_MULTI)
			MultiSendPlaySound (SOUND_WEAPON_HIT_DOOR, F1_0);
		}
	}
ObjectCreateExplosion (playerObjP->nSegment, vHitPt, i2f (10)/2, VCLIP_PLAYER_HIT);
if (WI_damage_radius (weapon->id))
	ExplodeBadassWeapon (weapon, vHitPt);
MaybeKillWeapon (weapon, playerObjP);
BumpTwoObjects (playerObjP, weapon, 0, vHitPt);	//no damage from bump
if (!WI_damage_radius (weapon->id)) {
	if (weapon->cType.laserInfo.nParentObj > -1)
		killer = gameData.objs.objects + weapon->cType.laserInfo.nParentObj;

//		if (weapon->id == SMARTMSL_BLOB_ID)
//			damage /= 4;

	if (!(weapon->flags & OF_HARMLESS))
		ApplyDamageToPlayer (playerObjP, killer, damage);
}
//	Robots become aware of you if you get hit.
AIDoCloakStuff ();
return 1; 
}

//	-----------------------------------------------------------------------------
//	Nasty robots are the ones that attack you by running into you and doing lots of damage.
int CollidePlayerAndNastyRobot (tObject * playerObjP, tObject * robot, vmsVector *vHitPt)
{
//	if (!(ROBOTINFO (objP->id).energyDrain && gameData.multiplayer.players [playerObjP->id].energy))
ObjectCreateExplosion (playerObjP->nSegment, vHitPt, i2f (10)/2, VCLIP_PLAYER_HIT);
if (BumpTwoObjects (playerObjP, robot, 0, vHitPt))	{//no damage from bump
	DigiLinkSoundToPos (ROBOTINFO (robot->id).clawSound, playerObjP->nSegment, 0, vHitPt, 0, F1_0);
	ApplyDamageToPlayer (playerObjP, robot, F1_0* (gameStates.app.nDifficultyLevel+1));
	}
return 1;
}

//	-----------------------------------------------------------------------------

int CollidePlayerAndMatCen (tObject *objP)
{
	short	tSide;
	vmsVector	exit_dir;
	tSegment	*segp = gameData.segs.segments + objP->nSegment;

DigiLinkSoundToPos (SOUND_PLAYER_GOT_HIT, objP->nSegment, 0, &objP->position.vPos, 0, F1_0);
//	DigiPlaySample (SOUND_PLAYER_GOT_HIT, F1_0);
ObjectCreateExplosion (objP->nSegment, &objP->position.vPos, i2f (10)/2, VCLIP_PLAYER_HIT);
if (objP->id != gameData.multiplayer.nLocalPlayer)
	return 1;
for (tSide=0; tSide<MAX_SIDES_PER_SEGMENT; tSide++)
	if (WALL_IS_DOORWAY (segp, tSide, objP) & WID_FLY_FLAG) {
		vmsVector	exit_point, rand_vec;

		COMPUTE_SIDE_CENTER (&exit_point, segp, tSide);
		VmVecSub (&exit_dir, &exit_point, &objP->position.vPos);
		VmVecNormalizeQuick (&exit_dir);
		MakeRandomVector (&rand_vec);
		rand_vec.p.x /= 4;	
		rand_vec.p.y /= 4;	
		rand_vec.p.z /= 4;
		VmVecInc (&exit_dir, &rand_vec);
		VmVecNormalizeQuick (&exit_dir);
		}
BumpOneObject (objP, &exit_dir, 64*F1_0);
ApplyDamageToPlayer (objP, objP, 4*F1_0);	//	Changed, MK, 2/19/96, make killer the tPlayer, so if you die in matcen, will say you killed yourself
return 1; 
}

//	-----------------------------------------------------------------------------

int CollideRobotAndMatCen (tObject *objP)
{
	short	tSide;
	vmsVector	exit_dir;
	tSegment *segp=gameData.segs.segments + objP->nSegment;

DigiLinkSoundToPos (SOUND_ROBOT_HIT, objP->nSegment, 0, &objP->position.vPos, 0, F1_0);
//	DigiPlaySample (SOUND_ROBOT_HIT, F1_0);

if (ROBOTINFO (objP->id).nExp1VClip > -1)
	ObjectCreateExplosion ((short) objP->nSegment, &objP->position.vPos, (objP->size/2*3)/4, (ubyte) ROBOTINFO (objP->id).nExp1VClip);
for (tSide=0; tSide<MAX_SIDES_PER_SEGMENT; tSide++)
	if (WALL_IS_DOORWAY (segp, tSide, NULL) & WID_FLY_FLAG) {
		vmsVector	exit_point;

		COMPUTE_SIDE_CENTER (&exit_point, segp, tSide);
		VmVecSub (&exit_dir, &exit_point, &objP->position.vPos);
		VmVecNormalizeQuick (&exit_dir);
	}
BumpOneObject (objP, &exit_dir, 8*F1_0);
ApplyDamageToRobot (objP, F1_0, -1);
return 1; 
}

//##void CollidePlayerAndCamera (tObject * playerObjP, tObject * camera, vmsVector *vHitPt) { 
//##	return; 
//##}

//	-----------------------------------------------------------------------------

extern int Network_gotPowerup; // HACK!!!

int CollidePlayerAndPowerup (tObject * playerObjP, tObject * powerup, vmsVector *vHitPt) 
{ 
if (!gameStates.app.bEndLevelSequence && !gameStates.app.bPlayerIsDead && 
	(playerObjP->id == gameData.multiplayer.nLocalPlayer)) {
	int bPowerupUsed = DoPowerup (powerup, playerObjP->id);
	if (bPowerupUsed) {
		KillObject (powerup);
		if (gameData.app.nGameMode & GM_MULTI)
			MultiSendRemObj (OBJ_IDX (powerup));
		}
	}
else if (IsCoopGame && (playerObjP->id != gameData.multiplayer.nLocalPlayer)) {
	switch (powerup->id) {
		case POW_KEY_BLUE:	
			gameData.multiplayer.players [playerObjP->id].flags |= PLAYER_FLAGS_BLUE_KEY;
			break;
		case POW_KEY_RED:	
			gameData.multiplayer.players [playerObjP->id].flags |= PLAYER_FLAGS_RED_KEY;
			break;
		case POW_KEY_GOLD:	
			gameData.multiplayer.players [playerObjP->id].flags |= PLAYER_FLAGS_GOLD_KEY;
			break;
		default:
			break;
		}
	}
DetectEscortGoalAccomplished (OBJ_IDX (powerup));
return 1; 
}

//	-----------------------------------------------------------------------------

int CollidePlayerAndMonsterball (tObject * playerObjP, tObject * monsterball, vmsVector *vHitPt) 
{ 
if (!gameStates.app.bEndLevelSequence && !gameStates.app.bPlayerIsDead && 
	(playerObjP->id == gameData.multiplayer.nLocalPlayer)) {
	if (BumpTwoObjects (playerObjP, monsterball, 0, vHitPt))
		DigiLinkSoundToPos (SOUND_ROBOT_HIT_PLAYER, playerObjP->nSegment, 0, vHitPt, 0, F1_0);
	}
return 1;
}

//	-----------------------------------------------------------------------------
//##void CollidePlayerAndDebris (tObject * playerObjP, tObject * debris, vmsVector *vHitPt) { 
//##	return; 
//##}

int CollideActorAndClutter (tObject * actor, tObject * clutter, vmsVector *vHitPt) 
{ 
if (gameStates.app.bD2XLevel && 
	 (gameData.segs.segment2s [actor->nSegment].special == SEGMENT_IS_NODAMAGE))
	return 1;
if (!(actor->flags & OF_EXPLODING) && BumpTwoObjects (clutter, actor, 1, vHitPt))
	DigiLinkSoundToPos (SOUND_ROBOT_HIT_PLAYER, actor->nSegment, 0, vHitPt, 0, F1_0);
return 1;
}

//	-----------------------------------------------------------------------------
//	See if weapon1 creates a badass explosion.  If so, create the explosion
//	Return true if weapon does proximity (as opposed to only contact) damage when it explodes.
int MaybeDetonateWeapon (tObject *weapon1, tObject *weapon2, vmsVector *vHitPt)
{
	fix	dist;

if (!gameData.weapons.info [weapon1->id].damage_radius)
	return 0;

dist = VmVecDistQuick (&weapon1->position.vPos, &weapon2->position.vPos);
if (dist >= F1_0*5)
	weapon1->lifeleft = min (dist/64, F1_0);
else {
	MaybeKillWeapon (weapon1, weapon2);
	if (weapon1->flags & OF_SHOULD_BE_DEAD) {
		ExplodeBadassWeapon (weapon1, vHitPt);
		DigiLinkSoundToPos (gameData.weapons.info [weapon1->id].robot_hitSound, weapon1->nSegment , 0, vHitPt, 0, F1_0);
		}
	}
return 1;
}

//	-----------------------------------------------------------------------------

int CollideWeaponAndWeapon (tObject * weapon1, tObject * weapon2, vmsVector *vHitPt)
{ 
	int	id1 = weapon1->id;
	int	id2 = weapon2->id;
	int	bKill1, bKill2;
	// -- Does this look buggy??:  if (weapon1->id == SMALLMINE_ID && weapon1->id == SMALLMINE_ID)
if (id1 == SMALLMINE_ID && id2 == SMALLMINE_ID)
	return 1;		//these can't blow each other up  
if ((id1 == PROXMINE_ID || id2 == PROXMINE_ID) && !COMPETITION && EGI_FLAG (bSmokeGrenades, 0, 0, 0))
	return 1;
if (id1 == OMEGA_ID) {
	if (!OkToDoOmegaDamage (weapon1))
		return 1;
	}
else if (id2 == OMEGA_ID) {
	if (!OkToDoOmegaDamage (weapon2))
		return 1;
	}
bKill1 = WI_destructable (id1) || (!COMPETITION && EGI_FLAG (bShootMissiles, 0, 0, 0) && bIsMissile [id1]);
bKill2 = WI_destructable (id2) || (!COMPETITION && EGI_FLAG (bShootMissiles, 0, 0, 0) && bIsMissile [id2]);
if (bKill1 || bKill2) {
	//	Bug reported by Adam Q. Pletcher on September 9, 1994, smart bomb homing missiles were toasting each other.
	if ((id1 == id2) && (weapon1->cType.laserInfo.nParentObj == weapon2->cType.laserInfo.nParentObj))
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

int CollideWeaponAndMonsterball (tObject *weapon, tObject *powerup, vmsVector *vHitPt) 
{
if (weapon->cType.laserInfo.parentType == OBJ_PLAYER) {	
	DigiLinkSoundToPos (SOUND_ROBOT_HIT, weapon->nSegment , 0, vHitPt, 0, F1_0);
	if (weapon->id == EARTHSHAKER_ID)
		ShakerRockStuff ();
	if (weapon->mType.physInfo.flags & PF_PERSISTENT) {
		if (AddHitObject (weapon, OBJ_IDX (powerup)) < 0)
			return 1;
		}
	ObjectCreateExplosion (powerup->nSegment, vHitPt, i2f (10)/2, VCLIP_PLAYER_HIT);
	if (WI_damage_radius (weapon->id))
		ExplodeBadassWeapon (weapon, vHitPt);
	MaybeKillWeapon (weapon, powerup);
	BumpTwoObjects (weapon, powerup, 1, vHitPt);
	}
return 1; 
}

//	-----------------------------------------------------------------------------
//##void CollideWeaponAndCamera (tObject * weapon, tObject * camera, vmsVector *vHitPt) { 
//##	return; 
//##}

//	-----------------------------------------------------------------------------

int CollideWeaponAndDebris (tObject * weapon, tObject * debris, vmsVector *vHitPt) 
{ 
//	Hack! Prevent debris from causing bombs spewed at tPlayer death to detonate!
if ((weapon->id == PROXMINE_ID) || (weapon->id == SMARTMINE_ID)) {
	if (weapon->cType.laserInfo.creationTime + F1_0/2 > gameData.time.xGame)
		return 1;
	}
if ((weapon->cType.laserInfo.parentType==OBJ_PLAYER) && !(debris->flags & OF_EXPLODING))	{	
	DigiLinkSoundToPos (SOUND_ROBOT_HIT, weapon->nSegment , 0, vHitPt, 0, F1_0);
	ExplodeObject (debris, 0);
	if (WI_damage_radius (weapon->id))
		ExplodeBadassWeapon (weapon, vHitPt);
	MaybeKillWeapon (weapon, debris);
	KillObject (weapon);
	}
return 1; 
}

//##void CollideCameraAndCamera (tObject * camera1, tObject * camera2, vmsVector *vHitPt) { 
//##	return; 
//##}

//##void CollideCameraAndPowerup (tObject * camera, tObject * powerup, vmsVector *vHitPt) { 
//##	return; 
//##}

//##void CollideCameraAndDebris (tObject * camera, tObject * debris, vmsVector *vHitPt) { 
//##	return; 
//##}

//##void CollidePowerupAndPowerup (tObject * powerup1, tObject * powerup2, vmsVector *vHitPt) { 
//##	return; 
//##}

//##void CollidePowerupAndDebris (tObject * powerup, tObject * debris, vmsVector *vHitPt) { 
//##	return; 
//##}

//##void CollideDebrisAndDebris (tObject * debris1, tObject * debris2, vmsVector *vHitPt) { 
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

/* DPH: These ones are never used so I'm not going to bother */
#ifndef __GNUC__
#define IGNORE_COLLISION(type1, type2, collisionHandler) \
	case COLLISION_OF ((type1), (type2)): \
		break; \
	case COLLISION_OF ((type2), (type1)): \
		break;

#define ERROR_COLLISION(type1, type2, collisionHandler) \
	case COLLISION_OF ((type1), (type2)): \
		Error ("Error in collision nType!"); \
		break; \
	case COLLISION_OF ((type2), (type1)): \
		Error ("Error in collision nType!"); \
		break;
#endif

//	-----------------------------------------------------------------------------

int CollideTwoObjects (tObject * A, tObject * B, vmsVector *vHitPt)
{
	int collisionType = COLLISION_OF (A->nType, B->nType);

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
	DO_COLLISION		(OBJ_ROBOT, 	OBJ_CNTRLCEN, 	CollideRobotAndReactor)
	DO_COLLISION		(OBJ_HOSTAGE, 	OBJ_PLAYER, 	CollideHostageAndPlayer)
	NO_COLLISION		(OBJ_HOSTAGE, 	OBJ_WEAPON, 	CollideHostageAndWeapon)
	NO_COLLISION		(OBJ_HOSTAGE, 	OBJ_CAMERA, 	CollideHostageAndCamera)
	NO_COLLISION		(OBJ_HOSTAGE, 	OBJ_POWERUP, 	CollideHostageAndPowerup)
	NO_COLLISION		(OBJ_HOSTAGE, 	OBJ_DEBRIS, 	CollideHostageAndDebris)
	DO_COLLISION		(OBJ_PLAYER, 	OBJ_WEAPON, 	CollidePlayerAndWeapon)
	NO_COLLISION		(OBJ_PLAYER, 	OBJ_CAMERA, 	CollidePlayerAndCamera)
	DO_COLLISION		(OBJ_PLAYER, 	OBJ_POWERUP, 	CollidePlayerAndPowerup)
	DO_COLLISION		(OBJ_PLAYER, 	OBJ_DEBRIS, 	CollideActorAndClutter)
	DO_COLLISION		(OBJ_PLAYER, 	OBJ_CNTRLCEN, 	CollidePlayerAndReactor)
	DO_COLLISION		(OBJ_PLAYER, 	OBJ_CLUTTER, 	CollideActorAndClutter)
	DO_COLLISION		(OBJ_PLAYER, 	OBJ_MONSTERBALL, 	CollidePlayerAndMonsterball)
	NO_COLLISION		(OBJ_WEAPON, 	OBJ_CAMERA, 	CollideWeaponAndCamera)
	NO_COLLISION		(OBJ_WEAPON, 	OBJ_POWERUP, 	CollideWeaponAndPowerup)
	DO_COLLISION		(OBJ_WEAPON, 	OBJ_DEBRIS, 	CollideWeaponAndDebris)
	DO_COLLISION		(OBJ_WEAPON, 	OBJ_CNTRLCEN, 	CollideWeaponAndReactor)
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

	for (i=0; i < MAX_OBJECT_TYPES; i++)
		for (j=0; j < MAX_OBJECT_TYPES; j++)
			CollisionResult [i][j] = RESULT_NOTHING;

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
	ENABLE_COLLISION  (OBJ_PLAYER, OBJ_CNTRLCEN)
	ENABLE_COLLISION  (OBJ_PLAYER, OBJ_CLUTTER)
	ENABLE_COLLISION  (OBJ_PLAYER, OBJ_MARKER);
	ENABLE_COLLISION  (OBJ_PLAYER, OBJ_CAMBOT);
	DISABLE_COLLISION (OBJ_PLAYER, OBJ_SMOKE);
	ENABLE_COLLISION  (OBJ_PLAYER, OBJ_MONSTERBALL);
	DISABLE_COLLISION (OBJ_FIREBALL, OBJ_ROBOT);
	DISABLE_COLLISION (OBJ_FIREBALL, OBJ_HOSTAGE);
	DISABLE_COLLISION (OBJ_FIREBALL, OBJ_PLAYER);
	DISABLE_COLLISION (OBJ_FIREBALL, OBJ_WEAPON);
	DISABLE_COLLISION (OBJ_FIREBALL, OBJ_CAMERA);
	DISABLE_COLLISION (OBJ_FIREBALL, OBJ_POWERUP);
	DISABLE_COLLISION (OBJ_FIREBALL, OBJ_DEBRIS);
	ENABLE_COLLISION  (OBJ_FIREBALL, OBJ_CAMBOT);
	DISABLE_COLLISION  (OBJ_FIREBALL, OBJ_SMOKE);
	DISABLE_COLLISION (OBJ_ROBOT, OBJ_HOSTAGE);
	ENABLE_COLLISION  (OBJ_ROBOT, OBJ_PLAYER);
	ENABLE_COLLISION  (OBJ_ROBOT, OBJ_WEAPON);
	DISABLE_COLLISION (OBJ_ROBOT, OBJ_CAMERA);
	DISABLE_COLLISION (OBJ_ROBOT, OBJ_POWERUP);
	DISABLE_COLLISION (OBJ_ROBOT, OBJ_DEBRIS);
	ENABLE_COLLISION  (OBJ_ROBOT, OBJ_CNTRLCEN)
	ENABLE_COLLISION  (OBJ_HOSTAGE, OBJ_PLAYER);
	ENABLE_COLLISION  (OBJ_HOSTAGE, OBJ_WEAPON);
	DISABLE_COLLISION (OBJ_HOSTAGE, OBJ_CAMERA);
	DISABLE_COLLISION (OBJ_HOSTAGE, OBJ_POWERUP);
	DISABLE_COLLISION (OBJ_HOSTAGE, OBJ_DEBRIS);
	DISABLE_COLLISION (OBJ_WEAPON, OBJ_CAMERA);
	DISABLE_COLLISION (OBJ_WEAPON, OBJ_POWERUP);
	ENABLE_COLLISION  (OBJ_WEAPON, OBJ_DEBRIS);
	ENABLE_COLLISION  (OBJ_WEAPON, OBJ_WEAPON);
	ENABLE_COLLISION  (OBJ_WEAPON, OBJ_CNTRLCEN)
	ENABLE_COLLISION  (OBJ_WEAPON, OBJ_CLUTTER)
	ENABLE_COLLISION  (OBJ_WEAPON, OBJ_CAMBOT);
	DISABLE_COLLISION  (OBJ_WEAPON, OBJ_SMOKE);
	ENABLE_COLLISION  (OBJ_WEAPON, OBJ_MONSTERBALL);
	DISABLE_COLLISION (OBJ_CAMERA, OBJ_CAMERA);
	DISABLE_COLLISION (OBJ_CAMERA, OBJ_POWERUP);
	DISABLE_COLLISION (OBJ_CAMERA, OBJ_DEBRIS);
	DISABLE_COLLISION (OBJ_POWERUP, OBJ_POWERUP);
	DISABLE_COLLISION (OBJ_POWERUP, OBJ_DEBRIS);
	ENABLE_COLLISION  (OBJ_POWERUP, OBJ_WALL);
	DISABLE_COLLISION (OBJ_DEBRIS, OBJ_DEBRIS);
	ENABLE_COLLISION  (OBJ_ROBOT, OBJ_CAMBOT);
	DISABLE_COLLISION  (OBJ_ROBOT, OBJ_SMOKE);

}

//	-----------------------------------------------------------------------------

int CollideObjectWithWall (tObject * objP, fix hitspeed, short hitseg, short hitwall, vmsVector * vHitPt)
{
switch (objP->nType)	{
	case OBJ_NONE:
		Error ("An object of type NONE hit a wall! \n");
		break;
	case OBJ_PLAYER:		
		CollidePlayerAndWall (objP, hitspeed, hitseg, hitwall, vHitPt); 
		break;
	case OBJ_WEAPON:		
		CollideWeaponAndWall (objP, hitspeed, hitseg, hitwall, vHitPt); 
		break;
	case OBJ_DEBRIS:		
		CollideDebrisAndWall (objP, hitspeed, hitseg, hitwall, vHitPt); 
		break;
	case OBJ_FIREBALL:	
		break;	//CollideFireballAndWall (objP, hitspeed, hitseg, hitwall, vHitPt); 
	case OBJ_ROBOT:		
		CollideRobotAndWall (objP, hitspeed, hitseg, hitwall, vHitPt); 
		break;
	case OBJ_HOSTAGE:		
		break;	//CollideHostageAndWall (objP, hitspeed, hitseg, hitwall, vHitPt); 
	case OBJ_CAMERA:		
		break;	//CollideCameraAndWall (objP, hitspeed, hitseg, hitwall, vHitPt); 
	case OBJ_SMOKE:		
		break;	//CollideSmokeAndWall (objP, hitspeed, hitseg, hitwall, vHitPt); 
	case OBJ_POWERUP:		
		break;	//CollidePowerupAndWall (objP, hitspeed, hitseg, hitwall, vHitPt); 
	case OBJ_GHOST:		
		break;	//do nothing
	case OBJ_MONSTERBALL:		
#ifdef _DEBUG
		objP = objP;
#endif
		break;	//CollidePowerupAndWall (objP, hitspeed, hitseg, hitwall, vHitPt); 
	default:
		Error ("Unhandled tObject nType hit tWall in Collide.c \n");
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