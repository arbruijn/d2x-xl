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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "descent.h"
#include "error.h"
#include "segmath.h"
#include "fireball.h"
#include "network.h"
#include "multibot.h"
#include "collide.h"
#include "lightcluster.h"
#include "postprocessing.h"

#if DBG
#include "string.h"
#include <time.h>
#endif

#define	FIRE_AT_NEARBY_PLAYER_THRESHOLD	 I2X (40)

// ----------------------------------------------------------------------------
// Return firing status.
// If ready to fire a weapon, return true, else return false.
// Ready to fire a weapon if pNextrimaryFire <= 0 or nextSecondaryFire <= 0.
int32_t ReadyToFire (tRobotInfo *pRobotInfo, tAILocalInfo *pLocalInfo)
{
return (pLocalInfo->pNextrimaryFire <= 0) || ((pRobotInfo->nSecWeaponType != -1) && (pLocalInfo->nextSecondaryFire <= 0));
}

// ----------------------------------------------------------------------------------
void SetNextFireTime (CObject *pObj, tAILocalInfo *pLocalInfo, tRobotInfo *pRobotInfo, int32_t nGun)
{
	//	For guys in snipe mode, they have a 50% shot of getting this shot free.
if ((nGun != 0) || (pRobotInfo->nSecWeaponType == -1))
	if ((pObj->cType.aiInfo.behavior != AIB_SNIPE) || (RandShort () > 16384))
		pLocalInfo->nRapidFireCount++;
if (((nGun != 0) || (pRobotInfo->nSecWeaponType == -1)) && (pLocalInfo->nRapidFireCount < pRobotInfo->nRapidFireCount [gameStates.app.nDifficultyLevel])) {
	pLocalInfo->pNextrimaryFire = Min (I2X (1) / 8, pRobotInfo->primaryFiringWait [gameStates.app.nDifficultyLevel]/2);
	}
else {
	if ((pRobotInfo->nSecWeaponType == -1) || (nGun != 0)) {
		pLocalInfo->pNextrimaryFire = pRobotInfo->primaryFiringWait [gameStates.app.nDifficultyLevel];
		if (pLocalInfo->nRapidFireCount >= pRobotInfo->nRapidFireCount [gameStates.app.nDifficultyLevel])
			pLocalInfo->nRapidFireCount = 0;
		}
	else
		pLocalInfo->nextSecondaryFire = pRobotInfo->secondaryFiringWait [gameStates.app.nDifficultyLevel];
	}
}

// ----------------------------------------------------------------------------------
//	When some robots collide with the player, they attack.
//	If player is cloaked, then robot probably didn't actually collide, deal with that here.
void DoAIRobotHitAttack (CObject *pRobot, CObject *pTarget, CFixVector *vCollision)
{
	tAILocalInfo	*pLocalInfo = gameData.aiData.localInfo + OBJ_IDX (pRobot);
	tRobotInfo		*pRobotInfo = ROBOTINFO (pRobot->info.nId);

if (!pRobotInfo)
	return;
if (!pRobot->AttacksObject (pTarget))
	return;
if (pRobot->IsStatic ())
	return;
//	If player is dead, stop firing.
if (LOCALOBJECT->info.nType == OBJ_GHOST)
	return;
if (pRobotInfo->attackType != 1)
	return;
if (pLocalInfo->pNextrimaryFire > 0)
	return;
if (!TARGETOBJ->Cloaked ()) {
	if (CFixVector::Dist (OBJPOS (TARGETOBJ)->vPos, pRobot->info.position.vPos) < pRobot->info.xSize + TARGETOBJ->info.xSize + I2X (2)) {
		pTarget->CollidePlayerAndNastyRobot (pRobot, *vCollision);
		pRobot->DrainEnergy ();
		}
	}
pRobot->cType.aiInfo.GOAL_STATE = AIS_RECOVER;
SetNextFireTime (pRobot, pLocalInfo, pRobotInfo, 1);	//	1 = nGun: 0 is special (uses nextSecondaryFire)
}

#define	FIRE_K	8		//	Controls average accuracy of robot firing.  Smaller numbers make firing worse.  Being power of 2 doesn't matter.

// ====================================================================================================================

#define	MIN_LEAD_SPEED		 (I2X (4))
#define	MAX_LEAD_DISTANCE	 (I2X (200))
#define	LEAD_RANGE			 (I2X (1)/2)

// --------------------------------------------------------------------------------------------------------------------
//	Computes point at which projectile fired by robot can hit player given positions, player vel, elapsed time
inline fix ComputeLeadComponent (fix vTarget, fix vAttacker, fix player_vel, fix elapsedTime)
{
return FixDiv (vTarget - vAttacker, elapsedTime) + player_vel;
}

// --------------------------------------------------------------------------------------------------------------------
//	Lead the player, returning point to fire at in vFirePoint.
//	Rules:
//		Player not cloaked
//		Player must be moving at a speed >= MIN_LEAD_SPEED
//		Player not farther away than MAX_LEAD_DISTANCE
//		dot (vector_to_player, player_direction) must be in -LEAD_RANGE,LEAD_RANGE
//		if firing a matter weapon, less leading, based on skill level.
int32_t LeadTarget (CObject *pObj, CFixVector *vFirePoint, CFixVector *vBelievedTargetPos, int32_t nGuns, CFixVector *vFire)
{
	fix			dot, xTargetSpeed, xDistToTarget, xMaxWeaponSpeed, xProjectedTime;
	CFixVector	vTargetMovementDir, vVecToTarget;
	int32_t		nWeaponType;
	CWeaponInfo	*pWeaponInfo;
	tRobotInfo	*pRobotInfo = ROBOTINFO (pObj);

if (!pRobotInfo)
	return 0;
if (TARGETOBJ->Cloaked ())
	return 0;
vTargetMovementDir = TARGETOBJ->mType.physInfo.velocity;
xTargetSpeed = CFixVector::Normalize (vTargetMovementDir);
if (xTargetSpeed < MIN_LEAD_SPEED)
	return 0;
vVecToTarget = *vBelievedTargetPos - *vFirePoint;
xDistToTarget = CFixVector::Normalize (vVecToTarget);
if (xDistToTarget > MAX_LEAD_DISTANCE)
	return 0;
dot = CFixVector::Dot (vVecToTarget, vTargetMovementDir);
if ((dot < -LEAD_RANGE) || (dot > LEAD_RANGE))
	return 0;
//	Looks like it might be worth trying to lead the player.
nWeaponType = pRobotInfo->nWeaponType;
if ((nGuns == 0) && (pRobotInfo->nSecWeaponType != -1))
	nWeaponType = pRobotInfo->nSecWeaponType;
if (nWeaponType < 0)
	return 0;
pWeaponInfo = WEAPONINFO (nWeaponType);
if (!pWeaponInfo)
	return 0;
xMaxWeaponSpeed = pWeaponInfo->speed [gameStates.app.nDifficultyLevel];
if (xMaxWeaponSpeed < I2X (1))
	return 0;
//	Matter weapons:
//	At Rookie or Trainee, don't lead at all.
//	At higher skill levels, don't lead as well.  Accomplish this by screwing up xMaxWeaponSpeed.
if (pWeaponInfo->matter) {
	if (gameStates.app.nDifficultyLevel <= 1)
		return 0;
	else
		xMaxWeaponSpeed *= (DIFFICULTY_LEVEL_COUNT-gameStates.app.nDifficultyLevel);
   }
xProjectedTime = FixDiv (xDistToTarget, xMaxWeaponSpeed);
(*vFire).v.coord.x = ComputeLeadComponent ((*vBelievedTargetPos).v.coord.x, (*vFirePoint).v.coord.x, TARGETOBJ->mType.physInfo.velocity.v.coord.x, xProjectedTime);
(*vFire).v.coord.y = ComputeLeadComponent ((*vBelievedTargetPos).v.coord.y, (*vFirePoint).v.coord.y, TARGETOBJ->mType.physInfo.velocity.v.coord.y, xProjectedTime);
(*vFire).v.coord.z = ComputeLeadComponent ((*vBelievedTargetPos).v.coord.z, (*vFirePoint).v.coord.z, TARGETOBJ->mType.physInfo.velocity.v.coord.z, xProjectedTime);
CFixVector::Normalize (*vFire);
Assert (CFixVector::Dot (*vFire, pObj->info.position.mOrient.m.dir.f) < I2X (3) / 2);
//	Make sure not firing at especially strange angle.  If so, try to correct.  If still bad, give up after one try.
if (CFixVector::Dot (*vFire, pObj->info.position.mOrient.m.dir.f) < I2X (1) / 2) {
	*vFire += vVecToTarget;
	*vFire *= I2X (1) / 2;
	if (CFixVector::Dot (*vFire, pObj->info.position.mOrient.m.dir.f) < I2X (1) / 2) {
		return 0;
		}
	}
return 1;
}

// --------------------------------------------------------------------------------------------------------------------
//	Note: Parameter gameData.aiData.target.vDir is only passed now because guns which aren't on the forward vector from the
//	center of the robot will not fire right at the player.  We need to aim the guns at the player.  Barring that, we cheat.
//	When this routine is complete, the parameter gameData.aiData.target.vDir should not be necessary.
void AIFireLaserAtTarget (CObject *pObj, CFixVector *vFirePoint, int32_t nGun, CFixVector *vBelievedTargetPos)
{
	int16_t			nShot, nObject = pObj->Index ();
	tAILocalInfo	*pLocalInfo = gameData.aiData.localInfo + nObject;
	tRobotInfo		*pRobotInfo = ROBOTINFO (pObj);
	CFixVector		vFire;
	CFixVector		vRandTargetPos;
	int16_t			nWeaponType;
	fix				aim, dot;
	int32_t			count, i;

if (!pRobotInfo)
	return;
//	If this robot is only awake because a camera woke it up, don't fire.
if (pObj->cType.aiInfo.SUB_FLAGS & SUB_FLAGS_CAMERA_AWAKE)
	return;
if (TARGETOBJ->IsPlayer () && !pObj->AttacksPlayer ())
	return;
if (pObj->info.controlType == CT_MORPH)
	return;
//	If player is exploded, stop firing.
if (LOCALPLAYER.m_bExploded)
	return;
if (pObj->cType.aiInfo.xDyingStartTime)
	return;		//	No firing while in death roll.
//	Don't let the boss fire while in death roll.  Sorry, this is the easiest way to do this.
//	If you try to key the boss off pObj->cType.aiInfo.xDyingStartTime, it will hose the endlevel stuff.
if (pObj->IsBoss ()) {
	i = gameData.bossData.Find (nObject);
	if ((i < 0) || (gameData.bossData [i].m_nDyingStartTime))
		return;
	}
//	If player is cloaked, maybe don't fire based on how long cloaked and randomness.
if (TARGETOBJ->Cloaked ()) {
	fix	xCloakTime = gameData.aiData.cloakInfo [nObject % MAX_AI_CLOAK_INFO].lastTime;
	if ((gameData.timeData.xGame - xCloakTime > CLOAK_TIME_MAX / 4) &&
		 (RandShort () > FixDiv (gameData.timeData.xGame - xCloakTime, CLOAK_TIME_MAX) / 2)) {
		SetNextFireTime (pObj, pLocalInfo, pRobotInfo, nGun);
		return;
		}
	}
//	Handle problem of a robot firing through a CWall because its gun tip is on the other
//	CSide of the CWall than the robot's center.  For speed reasons, we normally only compute
//	the vector from the gun point to the player.  But we need to know whether the gun point
//	is separated from the robot's center by a CWall.  If so, don't fire!
if (pObj->cType.aiInfo.SUB_FLAGS & SUB_FLAGS_GUNSEG) {
	//	Well, the gun point is in a different CSegment than the robot's center.
	//	This is almost always ok, but it is not ok if something solid is in between.
	int32_t	nGunSeg = FindSegByPos (*vFirePoint, pObj->info.nSegment, 1, 0);
	//	See if these segments are connected, which should almost always be the case.
	int16_t nConnSide = (nGunSeg < 0) ? -1 : SEGMENT (nGunSeg)->ConnectedSide (SEGMENT (pObj->info.nSegment));
	if (nConnSide != -1) {
		//	They are connected via nConnSide in CSegment pObj->info.nSegment.
		//	See if they are unobstructed.
		if (!(SEGMENT (pObj->info.nSegment)->IsPassable (nConnSide, NULL) & WID_PASSABLE_FLAG)) {
			//	Can't fly through, so don't let this bot fire through!
			return;
			}
		}
	else {
		//	Well, they are not directly connected, so use FindHitpoint to see if they are unobstructed.
		CHitQuery	hitQuery (FQ_TRANSWALL, &pObj->info.position.vPos, vFirePoint, pObj->info.nSegment, pObj->Index ());
		CHitResult	hitResult;
		int32_t fate = FindHitpoint (hitQuery, hitResult, 0);
		if (fate != HIT_NONE) {
			Int3 ();		//	This bot's gun is poking through a CWall, so don't fire.
			MoveTowardsSegmentCenter (pObj);		//	And decrease chances it will happen again.
			return;
			}
		}
	}
//	Set position to fire at based on difficulty level and robot's aiming ability
aim = I2X (FIRE_K) - (FIRE_K - 1) * (pRobotInfo->aim << 8);	//	I2X (1) in bitmaps.tbl = same as used to be.  Worst is 50% more error.
//	Robots aim more poorly during seismic disturbance.
if (gameStates.gameplay.seismic.nMagnitude) {
	fix temp = I2X (1) - abs (gameStates.gameplay.seismic.nMagnitude);
	if (temp < I2X (1) / 2)
		temp = I2X (1) / 2;
	aim = FixMul (aim, temp);
	}
//	Lead the target half the time.
//	Note that when leading the player, aim is perfect.  This is probably acceptable since leading is so hacked in.
//	Problem is all robots will lead equally badly.
if (RandShort () < 16384) {
	if (LeadTarget (pObj, vFirePoint, vBelievedTargetPos, nGun, &vFire))		//	Stuff direction to fire at in vFirePoint.
		goto targetLed;
}

#if 0
if (gameStates.app.bNostalgia) {
#endif
	count = 4;			//	Don't want to sit in this loop foreverd:\temp\dm_test.
	i = (DIFFICULTY_LEVEL_COUNT - gameStates.app.nDifficultyLevel - 1) * 4 * aim;
	do {
		vRandTargetPos.v.coord.x = (*vBelievedTargetPos).v.coord.x + FixMul (SRandShort (), aim);
		vRandTargetPos.v.coord.y = (*vBelievedTargetPos).v.coord.y + FixMul (SRandShort (), aim);
		vRandTargetPos.v.coord.z = (*vBelievedTargetPos).v.coord.z + FixMul (SRandShort (), aim);
		CFixVector::NormalizedDir (vFire, vRandTargetPos, *vFirePoint);
		dot = CFixVector::Dot (pObj->info.position.mOrient.m.dir.f, vFire);
		} while (--count && (dot < I2X (1) / 4));
#if 0
	}
else {	// this way it should always work
	count = 10;
	CFixVector	vRand;
	vRand.dir.coord.x = FixMul (SRandShort (), aim);
	vRand.dir.coord.y = FixMul (SRandShort (), aim);
	vRand.dir.coord.z = FixMul (SRandShort (), aim);
	CFixVector vOffs = vRand * I2X (1) / 10;
	do {
		CFixVector::NormalizedDir (vFire, *vBelievedTargetPos + vRand, *vFirePoint);
		dot = CFixVector::Dot (pObj->info.position.mOrient.mat.dir.f, vFire);
		vRand -= vOffs;
		} while (--count && (dot < I2X (1) / 4));
	}
#endif

targetLed:

nWeaponType = pRobotInfo->nWeaponType;
if ((pRobotInfo->nSecWeaponType != -1) && ((nWeaponType < 0) || !nGun))
	nWeaponType = pRobotInfo->nSecWeaponType;
if (nWeaponType < 0)
	return;
if (0 > (nShot = CreateNewWeaponSimple (&vFire, vFirePoint, pObj->Index (), (uint8_t) nWeaponType, 1)))
	return;

if ((nWeaponType == FUSION_ID) && (gameStates.app.nSDLTicks [0] - pObj->TimeLastEffect () > 1000)) {
	pObj->SetTimeLastEffect (gameStates.app.nSDLTicks [0]);
	postProcessManager.Add (new CPostEffectShockwave (SDL_GetTicks (), I2X (1) / 3, pObj->info.xSize, 1, 
																	  OBJPOS (pObj)->vPos + OBJPOS (pObj)->mOrient.m.dir.f * pObj->info.xSize, pObj->Index ()));
	}

lightClusterManager.AddForAI (pObj, nObject, nShot);
pObj->Shots ().nObject = nShot;
pObj->Shots ().nSignature = OBJECT (nShot)->info.nSignature;

if (IsMultiGame) {
	AIMultiSendRobotPos (nObject, -1);
	MultiSendRobotFire (nObject, pObj->cType.aiInfo.CURRENT_GUN, &vFire);
	}
#if 1
if (++(pObj->cType.aiInfo.CURRENT_GUN) >= pRobotInfo->nGuns) {
	if ((pRobotInfo->nGuns == 1) || (pRobotInfo->nSecWeaponType == -1))
		pObj->cType.aiInfo.CURRENT_GUN = 0;
	else
		pObj->cType.aiInfo.CURRENT_GUN = 1;
	}
#endif
CreateAwarenessEvent (pObj, PA_NEARBY_ROBOT_FIRED);
SetNextFireTime (pObj, pLocalInfo, pRobotInfo, nGun);
}

//	-------------------------------------------------------------------------------------------------------------------

void DoFiringStuff (CObject *pObj, int32_t nTargetVisibility, CFixVector *vVecToTarget)
{
if ((gameData.aiData.target.nDistToLastPosFiredAt < FIRE_AT_NEARBY_PLAYER_THRESHOLD) ||
	 (gameData.aiData.nTargetVisibility >= 1)) {
	//	Now, if in robot's field of view, lock onto player
	fix dot = CFixVector::Dot (pObj->info.position.mOrient.m.dir.f, gameData.aiData.target.vDir);
	if ((dot >= I2X (7) / 8) || TARGETOBJ->Cloaked ()) {
		tAIStaticInfo*	pStaticInfo = &pObj->cType.aiInfo;
		tAILocalInfo*	pLocalInfo = gameData.aiData.localInfo + pObj->Index ();

		switch (pStaticInfo->GOAL_STATE) {
			case AIS_NONE:
			case AIS_REST:
			case AIS_SEARCH:
			case AIS_LOCK:
				pStaticInfo->GOAL_STATE = AIS_FIRE;
				if (pLocalInfo->targetAwarenessType <= PA_NEARBY_ROBOT_FIRED) {
					pLocalInfo->targetAwarenessType = PA_NEARBY_ROBOT_FIRED;
					pLocalInfo->targetAwarenessTime = PLAYER_AWARENESS_INITIAL_TIME;
					}
				break;
			}
		}
	else if (dot >= I2X (1)/2) {
		tAIStaticInfo	*pStaticInfo = &pObj->cType.aiInfo;
		switch (pStaticInfo->GOAL_STATE) {
			case AIS_NONE:
			case AIS_REST:
			case AIS_SEARCH:
				pStaticInfo->GOAL_STATE = AIS_LOCK;
				break;
			}
		}
	}
}

// --------------------------------------------------------------------------------------------------------------------
//	If a hiding robot gets bumped or hit, he decides to find another hiding place.
void DoAIRobotHit (CObject *pObj, int32_t nType)
{
if (pObj->info.controlType != CT_AI)
	return;
if ((nType != PA_WEAPON_ROBOT_COLLISION) && (nType != PA_PLAYER_COLLISION))
	return;
if (pObj->cType.aiInfo.behavior != AIB_STILL)
	return;
int32_t r = RandShort ();
//	Attack robots (eg, green guy) shouldn't have behavior = still.
//	1/8 time, charge player, 1/4 time create path, rest of time, do nothing
if (r < 4096) {
	CreatePathToTarget (pObj, 10, 1);
	pObj->cType.aiInfo.behavior = AIB_STATION;
	pObj->cType.aiInfo.nHideSegment = pObj->info.nSegment;
	gameData.aiData.localInfo [pObj->Index ()].mode = AIM_CHASE_OBJECT;
	}
else if (r < 4096 + 8192) {
	CreateNSegmentPath (pObj, RandShort () / 8192 + 2, -1);
	gameData.aiData.localInfo [pObj->Index ()].mode = AIM_FOLLOW_PATH;
	}
}

#if DBG
int32_t	bDoAIFlag=1;
int32_t	Cvv_test=0;
int32_t	Cvv_lastTime [MAX_OBJECTS_D2X];
int32_t	Gun_point_hack=0;
#endif

// --------------------------------------------------------------------------------------------------------------------
//	Returns true if this CObject should be allowed to fire at the player.
int32_t AIMaybeDoActualFiringStuff (CObject *pObj, tAIStaticInfo *pStaticInfo)
{
if (IsMultiGame &&
	 (pStaticInfo->GOAL_STATE != AIS_FLINCH) && (pObj->info.nId != ROBOT_BRAIN) &&
	 (pStaticInfo->CURRENT_STATE == AIS_FIRE))
	return 1;
return 0;
}

// --------------------------------------------------------------------------------------------------------------------
//	If fire_anyway, fire even if player is not visible.  We're firing near where we believe him to be.  Perhaps he's
//	lurking behind a corner.
void AIDoActualFiringStuff (CObject *pObj, tAIStaticInfo *pStaticInfo, tAILocalInfo *pLocalInfo, tRobotInfo *pRobotInfo, int32_t nGun)
{
if ((gameData.aiData.nTargetVisibility == 2) ||
	 (gameData.aiData.target.nDistToLastPosFiredAt < FIRE_AT_NEARBY_PLAYER_THRESHOLD)) {
	CFixVector vFirePos = gameData.aiData.target.vBelievedPos;

	//	Hack: If visibility not == 2, we're here because we're firing at a nearby player.
	//	So, fire at gameData.aiData.target.vLastPosFiredAt instead of the player position.
	if (!pRobotInfo->attackType && (gameData.aiData.nTargetVisibility != 2))
		vFirePos = gameData.aiData.target.vLastPosFiredAt;

	//	Changed by mk, 01/04/95, onearm would take about 9 seconds until he can fire at you.
	//	Above comment corrected.  Date changed from 1994, to 1995.  Should fix some very subtle bugs, 
	// as well as not cause me to wonder, in the future, why I was writing AI code for onearm ten months before he existed.
	if (!gameData.aiData.bObjAnimates || ReadyToFire (pRobotInfo, pLocalInfo)) {
		fix dot = CFixVector::Dot (pObj->info.position.mOrient.m.dir.f, gameData.aiData.target.vDir);
		if ((dot >= I2X (7) / 8) || ((dot > I2X (1) / 4) && pObj->IsBoss ())) {
			if (nGun < pRobotInfo->nGuns) {
				if (pRobotInfo->attackType == 1) {
					if ((TARGETOBJ->Type () == OBJ_PLAYER) && LOCALPLAYER.m_bExploded)
						return;
					if (gameData.aiData.target.xDist >= pObj->info.xSize + TARGETOBJ->info.xSize + I2X (2))
						return;
					if (!AILocalPlayerControlsRobot (pObj, ROBOT_FIRE_AGITATION - 2))
						return;
					DoAIRobotHitAttack (pObj, TARGETOBJ, &pObj->info.position.vPos);
					}
				else {
#if 1
					if (AICanFireAtTarget (pObj, &gameData.aiData.vGunPoint, &vFirePos)) {
#else
					if (gameData.aiData.vGunPoint.p.x || gameData.aiData.vGunPoint.p.y || gameData.aiData.vGunPoint.p.z) {
#endif
						if (!AILocalPlayerControlsRobot (pObj, ROBOT_FIRE_AGITATION))
							return;
						//	New, multi-weapon-nType system, 06/05/95 (life is slipping awayd:\temp\dm_test.)
						if (nGun != 0) {
							if (pLocalInfo->pNextrimaryFire <= 0) {
								AIFireLaserAtTarget (pObj, &gameData.aiData.vGunPoint, nGun, &vFirePos);
								gameData.aiData.target.vLastPosFiredAt = vFirePos;
								}
							if ((pLocalInfo->nextSecondaryFire <= 0) && (pRobotInfo->nSecWeaponType != -1)) {
								CalcGunPoint (&gameData.aiData.vGunPoint, pObj, 0);
								AIFireLaserAtTarget (pObj, &gameData.aiData.vGunPoint, 0, &vFirePos);
								gameData.aiData.target.vLastPosFiredAt = vFirePos;
								}
							}
						else if (pLocalInfo->pNextrimaryFire <= 0) {
							AIFireLaserAtTarget (pObj, &gameData.aiData.vGunPoint, nGun, &vFirePos);
							gameData.aiData.target.vLastPosFiredAt = vFirePos;
							}
						}
					}
				}

			//	Wants to fire, so should go into chase mode, probably.
			if ((pStaticInfo->behavior != AIB_RUN_FROM) &&
				 (pStaticInfo->behavior != AIB_STILL) &&
				 (pStaticInfo->behavior != AIB_SNIPE) &&
				 (pStaticInfo->behavior != AIB_FOLLOW) &&
				 !pRobotInfo->attackType &&
				 ((pLocalInfo->mode == AIM_FOLLOW_PATH) || (pLocalInfo->mode == AIM_IDLING)))
				pLocalInfo->mode = AIM_CHASE_OBJECT;
				pStaticInfo->GOAL_STATE = AIS_RECOVER;
				pLocalInfo->goalState [pStaticInfo->CURRENT_GUN] = AIS_RECOVER;
				// Switch to next gun for next fire.
#if 0
				if (++(pStaticInfo->CURRENT_GUN) >= pRobotInfo->nGuns) {
					if ((pRobotInfo->nGuns == 1) || (pRobotInfo->nSecWeaponType == -1))
						pStaticInfo->CURRENT_GUN = 0;
					else
						pStaticInfo->CURRENT_GUN = 1;
					}
#endif
				}
			}
		}
else if ((!pRobotInfo->attackType && (pRobotInfo->nWeaponType >= 0) && WI_homingFlag (pRobotInfo->nWeaponType)) ||
			(((pRobotInfo->nSecWeaponType != -1) && WI_homingFlag (pRobotInfo->nSecWeaponType)))) {
	fix dist;
	//	Robots which fire homing weapons might fire even if they don't have a bead on the player.
	if ((!gameData.aiData.bObjAnimates || (pLocalInfo->achievedState [pStaticInfo->CURRENT_GUN] == AIS_FIRE))
			&& (((pLocalInfo->pNextrimaryFire <= 0) && (pStaticInfo->CURRENT_GUN != 0)) ||
				((pLocalInfo->nextSecondaryFire <= 0) && (pStaticInfo->CURRENT_GUN == 0)))
			&& ((dist = CFixVector::Dist(gameData.aiData.vHitPos, pObj->info.position.vPos)) > I2X (40))) {
		if (!AILocalPlayerControlsRobot (pObj, ROBOT_FIRE_AGITATION))
			return;
		AIFireLaserAtTarget (pObj, &gameData.aiData.vGunPoint, nGun, &gameData.aiData.target.vBelievedPos);
		pStaticInfo->GOAL_STATE = AIS_RECOVER;
		pLocalInfo->goalState [pStaticInfo->CURRENT_GUN] = AIS_RECOVER;
		// Switch to next gun for next fire.
		if (++(pStaticInfo->CURRENT_GUN) >= pRobotInfo->nGuns)
			pStaticInfo->CURRENT_GUN = 0;
		}
	else {
		// Switch to next gun for next fire.
		if (++(pStaticInfo->CURRENT_GUN) >= pRobotInfo->nGuns)
			pStaticInfo->CURRENT_GUN = 0;
		}
	}
else {	//	---------------------------------------------------------------
	CFixVector	vLastPos;

	if (RandShort ()/2 < FixMul (gameData.timeData.xFrame, (gameStates.app.nDifficultyLevel << 12) + 0x4000)) {
		if ((!gameData.aiData.bObjAnimates || ReadyToFire (pRobotInfo, pLocalInfo)) &&
			 (gameData.aiData.target.nDistToLastPosFiredAt < FIRE_AT_NEARBY_PLAYER_THRESHOLD)) {
			CFixVector::NormalizedDir (vLastPos, gameData.aiData.target.vBelievedPos, pObj->info.position.vPos);
			fix dot = CFixVector::Dot (pObj->info.position.mOrient.m.dir.f, vLastPos);
			if (dot >= I2X (7) / 8) {
				if (pStaticInfo->CURRENT_GUN < pRobotInfo->nGuns) {
					if (pRobotInfo->attackType == 1) {
						if (!LOCALPLAYER.m_bExploded && (gameData.aiData.target.xDist < pObj->info.xSize + TARGETOBJ->info.xSize + I2X (2))) {	
							if (!AILocalPlayerControlsRobot (pObj, ROBOT_FIRE_AGITATION-2))
								return;
							DoAIRobotHitAttack (pObj, TARGETOBJ, &pObj->info.position.vPos);
							}
						else
							return;
						}
					else {
						if (gameData.aiData.vGunPoint.IsZero ())
							;
						else {
							if (!AILocalPlayerControlsRobot (pObj, ROBOT_FIRE_AGITATION))
								return;
							//	New, multi-weapon-nType system, 06/05/95 (life is slipping awayd:\temp\dm_test.)
							if (nGun != 0) {
								if (pLocalInfo->pNextrimaryFire <= 0)
									AIFireLaserAtTarget (pObj, &gameData.aiData.vGunPoint, nGun, &gameData.aiData.target.vLastPosFiredAt);

								if ((pLocalInfo->nextSecondaryFire <= 0) && (pRobotInfo->nSecWeaponType != -1)) {
									CalcGunPoint (&gameData.aiData.vGunPoint, pObj, 0);
									AIFireLaserAtTarget (pObj, &gameData.aiData.vGunPoint, 0, &gameData.aiData.target.vLastPosFiredAt);
									}
								}
							else if (pLocalInfo->pNextrimaryFire <= 0)
								AIFireLaserAtTarget (pObj, &gameData.aiData.vGunPoint, nGun, &gameData.aiData.target.vLastPosFiredAt);
							}
						}
					//	Wants to fire, so should go into chase mode, probably.
					if ((pStaticInfo->behavior != AIB_RUN_FROM) && (pStaticInfo->behavior != AIB_STILL) && (pStaticInfo->behavior != AIB_SNIPE) &&
						 (pStaticInfo->behavior != AIB_FOLLOW) && ((pLocalInfo->mode == AIM_FOLLOW_PATH) || (pLocalInfo->mode == AIM_IDLING)))
						pLocalInfo->mode = AIM_CHASE_OBJECT;
					}
				pStaticInfo->GOAL_STATE = AIS_RECOVER;
				pLocalInfo->goalState [pStaticInfo->CURRENT_GUN] = AIS_RECOVER;
				// Switch to next gun for next fire.
				if (++(pStaticInfo->CURRENT_GUN) >= pRobotInfo->nGuns) {
					if (pRobotInfo->nGuns == 1)
						pStaticInfo->CURRENT_GUN = 0;
					else
						pStaticInfo->CURRENT_GUN = 1;
					}
				}
			}
		}
	}
}

//	---------------------------------------------------------------
// eof

