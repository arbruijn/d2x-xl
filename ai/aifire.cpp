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
#include "gameseg.h"
#include "fireball.h"
#include "network.h"
#include "multibot.h"
#include "collide.h"
#include "lightcluster.h"

//#define _DEBUG
#if DBG
#include "string.h"
#include <time.h>
#endif

#define	FIRE_AT_NEARBY_PLAYER_THRESHOLD	 I2X (40)

// ----------------------------------------------------------------------------
// Return firing status.
// If ready to fire a weapon, return true, else return false.
// Ready to fire a weapon if nextPrimaryFire <= 0 or nextSecondaryFire <= 0.
int ReadyToFire (tRobotInfo *botInfoP, tAILocalInfo *ailP)
{
return (ailP->nextPrimaryFire <= 0) || ((botInfoP->nSecWeaponType != -1) && (ailP->nextSecondaryFire <= 0));
}

// ----------------------------------------------------------------------------------
void SetNextFireTime (CObject *objP, tAILocalInfo *ailP, tRobotInfo *botInfoP, int nGun)
{
	//	For guys in snipe mode, they have a 50% shot of getting this shot free.
if ((nGun != 0) || (botInfoP->nSecWeaponType == -1))
	if ((objP->cType.aiInfo.behavior != AIB_SNIPE) || (d_rand () > 16384))
		ailP->nRapidFireCount++;
if (((nGun != 0) || (botInfoP->nSecWeaponType == -1)) && (ailP->nRapidFireCount < botInfoP->nRapidFireCount [gameStates.app.nDifficultyLevel])) {
	ailP->nextPrimaryFire = min (I2X (1)/8, botInfoP->primaryFiringWait [gameStates.app.nDifficultyLevel]/2);
	}
else {
	if ((botInfoP->nSecWeaponType == -1) || (nGun != 0)) {
		ailP->nextPrimaryFire = botInfoP->primaryFiringWait [gameStates.app.nDifficultyLevel];
		if (ailP->nRapidFireCount >= botInfoP->nRapidFireCount [gameStates.app.nDifficultyLevel])
			ailP->nRapidFireCount = 0;
		}
	else
		ailP->nextSecondaryFire = botInfoP->secondaryFiringWait [gameStates.app.nDifficultyLevel];
	}
}

// ----------------------------------------------------------------------------------
//	When some robots collide with the CPlayerData, they attack.
//	If CPlayerData is cloaked, then robot probably didn't actually collide, deal with that here.
void DoAIRobotHitAttack (CObject *robotP, CObject *playerobjP, CFixVector *vCollision)
{
	tAILocalInfo	*ailP = gameData.ai.localInfo + OBJ_IDX (robotP);
	tRobotInfo		*botInfoP = &ROBOTINFO (robotP->info.nId);

if (!gameStates.app.cheats.bRobotsFiring)
	return;
//	If CPlayerData is dead, stop firing.
if (OBJECTS [LOCALPLAYER.nObject].info.nType == OBJ_GHOST)
	return;
if (botInfoP->attackType != 1)
	return;
if (ailP->nextPrimaryFire > 0)
	return;
if (!(LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED)) {
	if (CFixVector::Dist (OBJPOS (gameData.objs.consoleP)->vPos, robotP->info.position.vPos) <
		 robotP->info.xSize + gameData.objs.consoleP->info.xSize + I2X (2)) {
		playerobjP->CollidePlayerAndNastyRobot (robotP, *vCollision);
		if (botInfoP->energyDrain && LOCALPLAYER.energy) {
			LOCALPLAYER.energy -= I2X (botInfoP->energyDrain);
			if (LOCALPLAYER.energy < 0)
				LOCALPLAYER.energy = 0;
			}
		}
	}
robotP->cType.aiInfo.GOAL_STATE = AIS_RECOVER;
SetNextFireTime (robotP, ailP, botInfoP, 1);	//	1 = nGun: 0 is special (uses nextSecondaryFire)
}

#define	FIRE_K	8		//	Controls average accuracy of robot firing.  Smaller numbers make firing worse.  Being power of 2 doesn't matter.

// ====================================================================================================================

#define	MIN_LEAD_SPEED		 (I2X (4))
#define	MAX_LEAD_DISTANCE	 (I2X (200))
#define	LEAD_RANGE			 (I2X (1)/2)

// --------------------------------------------------------------------------------------------------------------------
//	Computes point at which projectile fired by robot can hit CPlayerData given positions, CPlayerData vel, elapsed time
inline fix ComputeLeadComponent (fix player_pos, fix robot_pos, fix player_vel, fix elapsedTime)
{
return FixDiv (player_pos - robot_pos, elapsedTime) + player_vel;
}

// --------------------------------------------------------------------------------------------------------------------
//	Lead the CPlayerData, returning point to fire at in vFirePoint.
//	Rules:
//		Player not cloaked
//		Player must be moving at a speed >= MIN_LEAD_SPEED
//		Player not farther away than MAX_LEAD_DISTANCE
//		dot (vector_to_player, player_direction) must be in -LEAD_RANGE,LEAD_RANGE
//		if firing a matter weapon, less leading, based on skill level.
int LeadPlayer (CObject *objP, CFixVector *vFirePoint, CFixVector *vBelievedPlayerPos, int nGuns, CFixVector *vFire)
{
	fix			dot, xPlayerSpeed, xDistToPlayer, xMaxWeaponSpeed, xProjectedTime;
	CFixVector	vPlayerMovementDir, vVecToPlayer;
	int			nWeaponType;
	CWeaponInfo	*wiP;
	tRobotInfo	*botInfoP;

if (LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED)
	return 0;
vPlayerMovementDir = gameData.objs.consoleP->mType.physInfo.velocity;
xPlayerSpeed = CFixVector::Normalize (vPlayerMovementDir);
if (xPlayerSpeed < MIN_LEAD_SPEED)
	return 0;
vVecToPlayer = *vBelievedPlayerPos - *vFirePoint;
xDistToPlayer = CFixVector::Normalize (vVecToPlayer);
if (xDistToPlayer > MAX_LEAD_DISTANCE)
	return 0;
dot = CFixVector::Dot (vVecToPlayer, vPlayerMovementDir);
if ((dot < -LEAD_RANGE) || (dot > LEAD_RANGE))
	return 0;
//	Looks like it might be worth trying to lead the player.
botInfoP = &ROBOTINFO (objP->info.nId);
nWeaponType = botInfoP->nWeaponType;
if ((nGuns == 0) && (botInfoP->nSecWeaponType != -1))
	nWeaponType = botInfoP->nSecWeaponType;
wiP = gameData.weapons.info + nWeaponType;
xMaxWeaponSpeed = wiP->speed [gameStates.app.nDifficultyLevel];
if (xMaxWeaponSpeed < I2X (1))
	return 0;
//	Matter weapons:
//	At Rookie or Trainee, don't lead at all.
//	At higher skill levels, don't lead as well.  Accomplish this by screwing up xMaxWeaponSpeed.
if (wiP->matter) {
	if (gameStates.app.nDifficultyLevel <= 1)
		return 0;
	else
		xMaxWeaponSpeed *= (NDL-gameStates.app.nDifficultyLevel);
   }
xProjectedTime = FixDiv (xDistToPlayer, xMaxWeaponSpeed);
(*vFire)[X] = ComputeLeadComponent ((*vBelievedPlayerPos)[X], (*vFirePoint)[X], gameData.objs.consoleP->mType.physInfo.velocity[X], xProjectedTime);
(*vFire)[Y] = ComputeLeadComponent ((*vBelievedPlayerPos)[Y], (*vFirePoint)[Y], gameData.objs.consoleP->mType.physInfo.velocity[Y], xProjectedTime);
(*vFire)[Z] = ComputeLeadComponent ((*vBelievedPlayerPos)[Z], (*vFirePoint)[Z], gameData.objs.consoleP->mType.physInfo.velocity[Z], xProjectedTime);
CFixVector::Normalize (*vFire);
Assert (CFixVector::Dot (*vFire, objP->info.position.mOrient.FVec ()) < I2X (3)/2);
//	Make sure not firing at especially strange angle.  If so, try to correct.  If still bad, give up after one try.
if (CFixVector::Dot (*vFire, objP->info.position.mOrient.FVec ()) < I2X (1)/2) {
	*vFire += vVecToPlayer;
	*vFire *= I2X (1)/2;
	if (CFixVector::Dot (*vFire, objP->info.position.mOrient.FVec ()) < I2X (1)/2) {
		return 0;
		}
	}
return 1;
}

// --------------------------------------------------------------------------------------------------------------------
//	Note: Parameter gameData.ai.vVecToPlayer is only passed now because guns which aren't on the forward vector from the
//	center of the robot will not fire right at the player.  We need to aim the guns at the player.  Barring that, we cheat.
//	When this routine is complete, the parameter gameData.ai.vVecToPlayer should not be necessary.
void AIFireLaserAtPlayer (CObject *objP, CFixVector *vFirePoint, int nGun, CFixVector *vBelievedPlayerPos)
{
	short				nShot, nObject = objP->Index ();
	tAILocalInfo	*ailP = gameData.ai.localInfo + nObject;
	tRobotInfo		*botInfoP = &ROBOTINFO (objP->info.nId);
	CFixVector		vFire;
	CFixVector		bpp_diff;
	short				nWeaponType;
	fix				aim, dot;
	int				count, i;

Assert (nObject >= 0);
//	If this robot is only awake because a camera woke it up, don't fire.
if (objP->cType.aiInfo.SUB_FLAGS & SUB_FLAGS_CAMERA_AWAKE)
	return;
if (!gameStates.app.cheats.bRobotsFiring)
	return;
if (objP->info.controlType == CT_MORPH)
	return;
//	If player is exploded, stop firing.
if (gameStates.app.bPlayerExploded)
	return;
if (objP->cType.aiInfo.xDyingStartTime)
	return;		//	No firing while in death roll.
//	Don't let the boss fire while in death roll.  Sorry, this is the easiest way to do this.
//	If you try to key the boss off objP->cType.aiInfo.xDyingStartTime, it will hose the endlevel stuff.
if (ROBOTINFO (objP->info.nId).bossFlag) {
	i = gameData.bosses.Find (nObject);
	if ((i < 0) || (gameData.bosses [i].m_nDyingStartTime))
		return;
	}
//	If CPlayerData is cloaked, maybe don't fire based on how long cloaked and randomness.
if (LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED) {
	fix	xCloakTime = gameData.ai.cloakInfo [nObject % MAX_AI_CLOAK_INFO].lastTime;
	if ((gameData.time.xGame - xCloakTime > CLOAK_TIME_MAX/4) &&
		 (d_rand () > FixDiv (gameData.time.xGame - xCloakTime, CLOAK_TIME_MAX)/2)) {
		SetNextFireTime (objP, ailP, botInfoP, nGun);
		return;
		}
	}
//	Handle problem of a robot firing through a CWall because its gun tip is on the other
//	CSide of the CWall than the robot's center.  For speed reasons, we normally only compute
//	the vector from the gun point to the player.  But we need to know whether the gun point
//	is separated from the robot's center by a CWall.  If so, don't fire!
if (objP->cType.aiInfo.SUB_FLAGS & SUB_FLAGS_GUNSEG) {
	//	Well, the gun point is in a different CSegment than the robot's center.
	//	This is almost always ok, but it is not ok if something solid is in between.
	int	nGunSeg = FindSegByPos (*vFirePoint, objP->info.nSegment, 1, 0);
	//	See if these segments are connected, which should almost always be the case.
	short nConnSide = (nGunSeg < 0) ? -1 : SEGMENTS [nGunSeg].ConnectedSide (&SEGMENTS [objP->info.nSegment]);
	if (nConnSide != -1) {
		//	They are connected via nConnSide in CSegment objP->info.nSegment.
		//	See if they are unobstructed.
		if (!(SEGMENTS [objP->info.nSegment].IsDoorWay (nConnSide, NULL) & WID_FLY_FLAG)) {
			//	Can't fly through, so don't let this bot fire through!
			return;
			}
		}
	else {
		//	Well, they are not directly connected, so use FindHitpoint to see if they are unobstructed.
		tCollisionQuery	fq;
		tCollisionData		hitData;
		int			fate;

		fq.startSeg			= objP->info.nSegment;
		fq.p0					= &objP->info.position.vPos;
		fq.p1					= vFirePoint;
		fq.radP0				=
		fq.radP1				= 0;
		fq.thisObjNum		= objP->Index ();
		fq.ignoreObjList	= NULL;
		fq.flags				= FQ_TRANSWALL;
		fq.bCheckVisibility = false;

		fate = FindHitpoint (&fq, &hitData);
		if (fate != HIT_NONE) {
			Int3 ();		//	This bot's gun is poking through a CWall, so don't fire.
			MoveTowardsSegmentCenter (objP);		//	And decrease chances it will happen again.
			return;
			}
		}
	}
//	Set position to fire at based on difficulty level and robot's aiming ability
aim = I2X (FIRE_K) - (FIRE_K-1)* (botInfoP->aim << 8);	//	I2X (1) in bitmaps.tbl = same as used to be.  Worst is 50% more error.
//	Robots aim more poorly during seismic disturbance.
if (gameStates.gameplay.seismic.nMagnitude) {
	fix temp = I2X (1) - abs (gameStates.gameplay.seismic.nMagnitude);
	if (temp < I2X (1)/2)
		temp = I2X (1)/2;
	aim = FixMul (aim, temp);
	}
//	Lead the CPlayerData half the time.
//	Note that when leading the CPlayerData, aim is perfect.  This is probably acceptable since leading is so hacked in.
//	Problem is all robots will lead equally badly.
if (d_rand () < 16384) {
	if (LeadPlayer (objP, vFirePoint, vBelievedPlayerPos, nGun, &vFire))		//	Stuff direction to fire at in vFirePoint.
		goto player_led;
}

dot = 0;
count = 0;			//	Don't want to sit in this loop foreverd:\temp\dm_test.
i = (NDL - gameStates.app.nDifficultyLevel - 1) * 4;
while ((count < 4) && (dot < I2X (1)/4)) {
	bpp_diff[X] = (*vBelievedPlayerPos)[X] + FixMul ((d_rand ()-16384) * i, aim);
	bpp_diff[Y] = (*vBelievedPlayerPos)[Y] + FixMul ((d_rand ()-16384) * i, aim);
	bpp_diff[Z] = (*vBelievedPlayerPos)[Z] + FixMul ((d_rand ()-16384) * i, aim);
	CFixVector::NormalizedDir(vFire, bpp_diff, *vFirePoint);
	dot = CFixVector::Dot (objP->info.position.mOrient.FVec (), vFire);
	count++;
	}

player_led:

nWeaponType = botInfoP->nWeaponType;
if ((botInfoP->nSecWeaponType != -1) && ((nWeaponType < 0) || !nGun))
	nWeaponType = botInfoP->nSecWeaponType;
if (nWeaponType < 0)
	return;
if (0 > (nShot = CreateNewLaserEasy (&vFire, vFirePoint, objP->Index (), (ubyte) nWeaponType, 1)))
	return;

lightClusterManager.AddForAI (objP, nObject, nShot);
objP->Shots ().nObject = nShot;
objP->Shots ().nSignature = OBJECTS [nShot].info.nSignature;

if (IsMultiGame) {
	AIMultiSendRobotPos (nObject, -1);
	MultiSendRobotFire (nObject, objP->cType.aiInfo.CURRENT_GUN, &vFire);
	}
#if 1
if (++(objP->cType.aiInfo.CURRENT_GUN) >= botInfoP->nGuns) {
	if ((botInfoP->nGuns == 1) || (botInfoP->nSecWeaponType == -1))
		objP->cType.aiInfo.CURRENT_GUN = 0;
	else
		objP->cType.aiInfo.CURRENT_GUN = 1;
	}
#endif
CreateAwarenessEvent (objP, PA_NEARBY_ROBOT_FIRED);
SetNextFireTime (objP, ailP, botInfoP, nGun);
}

//	-------------------------------------------------------------------------------------------------------------------

void DoFiringStuff (CObject *objP, int nPlayerVisibility, CFixVector *vVecToPlayer)
{
if ((gameData.ai.nDistToLastPlayerPosFiredAt < FIRE_AT_NEARBY_PLAYER_THRESHOLD) ||
	 (gameData.ai.nPlayerVisibility >= 1)) {
	//	Now, if in robot's field of view, lock onto CPlayerData
	fix dot = CFixVector::Dot (objP->info.position.mOrient.FVec (), gameData.ai.vVecToPlayer);
	if ((dot >= I2X (7) / 8) || (LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED)) {
		tAIStaticInfo	*aiP = &objP->cType.aiInfo;
		tAILocalInfo	*ailP = gameData.ai.localInfo + objP->Index ();

		switch (aiP->GOAL_STATE) {
			case AIS_NONE:
			case AIS_REST:
			case AIS_SEARCH:
			case AIS_LOCK:
				aiP->GOAL_STATE = AIS_FIRE;
				if (ailP->playerAwarenessType <= PA_NEARBY_ROBOT_FIRED) {
					ailP->playerAwarenessType = PA_NEARBY_ROBOT_FIRED;
					ailP->playerAwarenessTime = PLAYER_AWARENESS_INITIAL_TIME;
					}
				break;
			}
		}
	else if (dot >= I2X (1)/2) {
		tAIStaticInfo	*aiP = &objP->cType.aiInfo;
		switch (aiP->GOAL_STATE) {
			case AIS_NONE:
			case AIS_REST:
			case AIS_SEARCH:
				aiP->GOAL_STATE = AIS_LOCK;
				break;
			}
		}
	}
}

// --------------------------------------------------------------------------------------------------------------------
//	If a hiding robot gets bumped or hit, he decides to find another hiding place.
void DoAIRobotHit (CObject *objP, int nType)
{
	int	r;

if (objP->info.controlType != CT_AI)
	return;
if ((nType != PA_WEAPON_ROBOT_COLLISION) && (nType != PA_PLAYER_COLLISION))
	return;
if (objP->cType.aiInfo.behavior != AIB_STILL)
	return;
r = d_rand ();
//	Attack robots (eg, green guy) shouldn't have behavior = still.
//Assert (ROBOTINFO (objP->info.nId).attackType == 0);
//	1/8 time, charge CPlayerData, 1/4 time create path, rest of time, do nothing
if (r < 4096) {
	CreatePathToPlayer (objP, 10, 1);
	objP->cType.aiInfo.behavior = AIB_STATION;
	objP->cType.aiInfo.nHideSegment = objP->info.nSegment;
	gameData.ai.localInfo [objP->Index ()].mode = AIM_CHASE_OBJECT;
	}
else if (r < 4096 + 8192) {
	CreateNSegmentPath (objP, d_rand () / 8192 + 2, -1);
	gameData.ai.localInfo [objP->Index ()].mode = AIM_FOLLOW_PATH;
	}
}

#if DBG
int	bDoAIFlag=1;
int	Cvv_test=0;
int	Cvv_lastTime [MAX_OBJECTS_D2X];
int	Gun_point_hack=0;
#endif

// --------------------------------------------------------------------------------------------------------------------
//	Returns true if this CObject should be allowed to fire at the player.
int AIMaybeDoActualFiringStuff (CObject *objP, tAIStaticInfo *aiP)
{
if (IsMultiGame &&
	 (aiP->GOAL_STATE != AIS_FLINCH) && (objP->info.nId != ROBOT_BRAIN) &&
	 (aiP->CURRENT_STATE == AIS_FIRE))
	return 1;
return 0;
}

// --------------------------------------------------------------------------------------------------------------------
//	If fire_anyway, fire even if CPlayerData is not visible.  We're firing near where we believe him to be.  Perhaps he's
//	lurking behind a corner.
void AIDoActualFiringStuff (CObject *objP, tAIStaticInfo *aiP, tAILocalInfo *ailP, tRobotInfo *botInfoP, int nGun)
{
	fix	dot;

if ((gameData.ai.nPlayerVisibility == 2) ||
	 (gameData.ai.nDistToLastPlayerPosFiredAt < FIRE_AT_NEARBY_PLAYER_THRESHOLD)) {
	CFixVector vFirePos = gameData.ai.vBelievedPlayerPos;

	//	Hack: If visibility not == 2, we're here because we're firing at a nearby player.
	//	So, fire at gameData.ai.vLastPlayerPosFiredAt instead of the CPlayerData position.
	if (!botInfoP->attackType && (gameData.ai.nPlayerVisibility != 2))
		vFirePos = gameData.ai.vLastPlayerPosFiredAt;

	//	Changed by mk, 01/04/95, onearm would take about 9 seconds until he can fire at you.
	//	Above comment corrected.  Date changed from 1994, to 1995.  Should fix some very subtle bugs, as well as not cause me to wonder, in the future, why I was writing AI code for onearm ten months before he existed.
	if (!gameData.ai.bObjAnimates || ReadyToFire (botInfoP, ailP)) {
		dot = CFixVector::Dot (objP->info.position.mOrient.FVec (), gameData.ai.vVecToPlayer);
		if ((dot >= I2X (7) / 8) || ((dot > I2X (1) / 4) && botInfoP->bossFlag)) {
			if (nGun < botInfoP->nGuns) {
				if (botInfoP->attackType == 1) {
					if (gameStates.app.bPlayerExploded || (gameData.ai.xDistToPlayer >= objP->info.xSize + gameData.objs.consoleP->info.xSize + I2X (2)))	// botInfoP->circleDistance [gameStates.app.nDifficultyLevel] + gameData.objs.consoleP->info.xSize)
						return;
					if (!AIMultiplayerAwareness (objP, ROBOT_FIRE_AGITATION - 2))
						return;
					DoAIRobotHitAttack (objP, gameData.objs.consoleP, &objP->info.position.vPos);
					}
				else {
#if 1
					if (AICanFireAtPlayer (objP, &gameData.ai.vGunPoint, &vFirePos)) {
#else
					if (gameData.ai.vGunPoint.p.x || gameData.ai.vGunPoint.p.y || gameData.ai.vGunPoint.p.z) {
#endif
						if (!AIMultiplayerAwareness (objP, ROBOT_FIRE_AGITATION))
							return;
						//	New, multi-weapon-nType system, 06/05/95 (life is slipping awayd:\temp\dm_test.)
						if (nGun != 0) {
							if (ailP->nextPrimaryFire <= 0) {
								AIFireLaserAtPlayer (objP, &gameData.ai.vGunPoint, nGun, &vFirePos);
								gameData.ai.vLastPlayerPosFiredAt = vFirePos;
								}
							if ((ailP->nextSecondaryFire <= 0) && (botInfoP->nSecWeaponType != -1)) {
								CalcGunPoint (&gameData.ai.vGunPoint, objP, 0);
								AIFireLaserAtPlayer (objP, &gameData.ai.vGunPoint, 0, &vFirePos);
								gameData.ai.vLastPlayerPosFiredAt = vFirePos;
								}
							}
						else if (ailP->nextPrimaryFire <= 0) {
							AIFireLaserAtPlayer (objP, &gameData.ai.vGunPoint, nGun, &vFirePos);
							gameData.ai.vLastPlayerPosFiredAt = vFirePos;
							}
						}
					}
				}

			//	Wants to fire, so should go into chase mode, probably.
			if ((aiP->behavior != AIB_RUN_FROM) &&
				 (aiP->behavior != AIB_STILL) &&
				 (aiP->behavior != AIB_SNIPE) &&
				 (aiP->behavior != AIB_FOLLOW) &&
				 !botInfoP->attackType &&
				 ((ailP->mode == AIM_FOLLOW_PATH) || (ailP->mode == AIM_IDLING)))
				ailP->mode = AIM_CHASE_OBJECT;
				aiP->GOAL_STATE = AIS_RECOVER;
				ailP->goalState [aiP->CURRENT_GUN] = AIS_RECOVER;
				// Switch to next gun for next fire.
#if 0
				if (++(aiP->CURRENT_GUN) >= botInfoP->nGuns) {
					if ((botInfoP->nGuns == 1) || (botInfoP->nSecWeaponType == -1))
						aiP->CURRENT_GUN = 0;
					else
						aiP->CURRENT_GUN = 1;
					}
#endif
				}
			}
		}
else if ((!botInfoP->attackType && gameData.weapons.info [botInfoP->nWeaponType].homingFlag) ||
			(((botInfoP->nSecWeaponType != -1) && gameData.weapons.info [botInfoP->nSecWeaponType].homingFlag))) {
	fix dist;
	//	Robots which fire homing weapons might fire even if they don't have a bead on the player.
	if ((!gameData.ai.bObjAnimates || (ailP->achievedState [aiP->CURRENT_GUN] == AIS_FIRE))
			&& (((ailP->nextPrimaryFire <= 0) && (aiP->CURRENT_GUN != 0)) ||
				((ailP->nextSecondaryFire <= 0) && (aiP->CURRENT_GUN == 0)))
			&& ((dist = CFixVector::Dist(gameData.ai.vHitPos, objP->info.position.vPos)) > I2X (40))) {
		if (!AIMultiplayerAwareness (objP, ROBOT_FIRE_AGITATION))
			return;
		AIFireLaserAtPlayer (objP, &gameData.ai.vGunPoint, nGun, &gameData.ai.vBelievedPlayerPos);
		aiP->GOAL_STATE = AIS_RECOVER;
		ailP->goalState [aiP->CURRENT_GUN] = AIS_RECOVER;
		// Switch to next gun for next fire.
		if (++(aiP->CURRENT_GUN) >= botInfoP->nGuns)
			aiP->CURRENT_GUN = 0;
		}
	else {
		// Switch to next gun for next fire.
		if (++(aiP->CURRENT_GUN) >= botInfoP->nGuns)
			aiP->CURRENT_GUN = 0;
		}
	}
else {	//	---------------------------------------------------------------
	CFixVector	vLastPos;

	if (d_rand ()/2 < FixMul (gameData.time.xFrame, (gameStates.app.nDifficultyLevel << 12) + 0x4000)) {
		if ((!gameData.ai.bObjAnimates || ReadyToFire (botInfoP, ailP)) &&
			 (gameData.ai.nDistToLastPlayerPosFiredAt < FIRE_AT_NEARBY_PLAYER_THRESHOLD)) {
			CFixVector::NormalizedDir(vLastPos, gameData.ai.vBelievedPlayerPos, objP->info.position.vPos);
			dot = CFixVector::Dot (objP->info.position.mOrient.FVec (), vLastPos);
			if (dot >= I2X (7) / 8) {
				if (aiP->CURRENT_GUN < botInfoP->nGuns) {
					if (botInfoP->attackType == 1) {
						if (!gameStates.app.bPlayerExploded && (gameData.ai.xDistToPlayer < objP->info.xSize + gameData.objs.consoleP->info.xSize + I2X (2))) {		// botInfoP->circleDistance [gameStates.app.nDifficultyLevel] + gameData.objs.consoleP->info.xSize) {
							if (!AIMultiplayerAwareness (objP, ROBOT_FIRE_AGITATION-2))
								return;
							DoAIRobotHitAttack (objP, gameData.objs.consoleP, &objP->info.position.vPos);
							}
						else
							return;
						}
					else {
						if (gameData.ai.vGunPoint.IsZero ())
							;
						else {
							if (!AIMultiplayerAwareness (objP, ROBOT_FIRE_AGITATION))
								return;
							//	New, multi-weapon-nType system, 06/05/95 (life is slipping awayd:\temp\dm_test.)
							if (nGun != 0) {
								if (ailP->nextPrimaryFire <= 0)
									AIFireLaserAtPlayer (objP, &gameData.ai.vGunPoint, nGun, &gameData.ai.vLastPlayerPosFiredAt);

								if ((ailP->nextSecondaryFire <= 0) && (botInfoP->nSecWeaponType != -1)) {
									CalcGunPoint (&gameData.ai.vGunPoint, objP, 0);
									AIFireLaserAtPlayer (objP, &gameData.ai.vGunPoint, 0, &gameData.ai.vLastPlayerPosFiredAt);
									}
								}
							else if (ailP->nextPrimaryFire <= 0)
								AIFireLaserAtPlayer (objP, &gameData.ai.vGunPoint, nGun, &gameData.ai.vLastPlayerPosFiredAt);
							}
						}
					//	Wants to fire, so should go into chase mode, probably.
					if ((aiP->behavior != AIB_RUN_FROM) && (aiP->behavior != AIB_STILL) && (aiP->behavior != AIB_SNIPE) &&
						 (aiP->behavior != AIB_FOLLOW) && ((ailP->mode == AIM_FOLLOW_PATH) || (ailP->mode == AIM_IDLING)))
						ailP->mode = AIM_CHASE_OBJECT;
					}
				aiP->GOAL_STATE = AIS_RECOVER;
				ailP->goalState [aiP->CURRENT_GUN] = AIS_RECOVER;
				// Switch to next gun for next fire.
				if (++(aiP->CURRENT_GUN) >= botInfoP->nGuns) {
					if (botInfoP->nGuns == 1)
						aiP->CURRENT_GUN = 0;
					else
						aiP->CURRENT_GUN = 1;
					}
				}
			}
		}
	}
}

//	---------------------------------------------------------------
// eof

