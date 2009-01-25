#ifdef HAVE_CONFIG_H
#	include <conf.h>
#endif

#include "descent.h"
#include "error.h"
#include "network.h"
#include "render.h"
#include "omega.h"
#include "trackobject.h"

#define	OMEGA_MIN_TRACKABLE_DOT			 (I2X (15) / 16)		//	Larger values mean narrower cone.  I2X (1) means damn near impossible.  0 means 180 degree field of view.
#define	OMEGA_MAX_TRACKABLE_DIST		MAX_OMEGA_DIST	//	An CObject must be at least this close to be tracked.

fix	xMinTrackableDot = MIN_TRACKABLE_DOT;

//	-----------------------------------------------------------------------------------------------------------
//	Return true if weapon *trackerP is able to track CObject OBJECTS [nHomingTarget], else return false.
//	In order for the CObject to be trackable, it must be within a reasonable turning radius for the missile
//	and it must not be obstructed by a CWall.
int ObjectIsTrackeable (int nHomingTarget, CObject *trackerP, fix *xDot)
{
	CFixVector	vGoal;
	CObject		*objP;

if (nHomingTarget == -1)
	return 0;
if (IsCoopGame)
	return 0;
objP = OBJECTS + nHomingTarget;
//	Don't track CPlayerData if he's cloaked.
if ((nHomingTarget == LOCALPLAYER.nObject) && (LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED))
	return 0;
//	Can't track AI CObject if he's cloaked.
if (objP->info.nType == OBJ_ROBOT) {
	if (objP->cType.aiInfo.CLOAKED)
		return 0;
	//	Your missiles don't track your escort.
	if (ROBOTINFO (objP->info.nId).companion && 
		 (trackerP->cType.laserInfo.parent.nType == OBJ_PLAYER))
		return 0;
	}
vGoal = objP->info.position.vPos - trackerP->info.position.vPos;
CFixVector::Normalize(vGoal);
*xDot = CFixVector::Dot (vGoal, trackerP->info.position.mOrient.FVec ());
if ((*xDot < xMinTrackableDot) && (*xDot > I2X (9) / 10)) {
	CFixVector::Normalize(vGoal);
	*xDot = CFixVector::Dot (vGoal, trackerP->info.position.mOrient.FVec ());
	}

if ((*xDot >= xMinTrackableDot) || 
	 (EGI_FLAG (bEnhancedShakers, 0, 0, 0) && (trackerP->info.nType == OBJ_WEAPON) && (trackerP->info.nId == EARTHSHAKER_MEGA_ID) /*&& (*xDot >= 0)*/)) {
	//	xDot is in legal range, now see if CObject is visible
	return ObjectToObjectVisibility (trackerP, objP, FQ_TRANSWALL);
	}
return 0;
}

//	--------------------------------------------------------------------------------------------

int CallFindHomingObjectComplete (CObject *trackerP, CFixVector *vCurPos)
{
if (!IsMultiGame)
	return FindHomingObjectComplete (vCurPos, trackerP, OBJ_ROBOT, -1);
if ((trackerP->info.nType == OBJ_PLAYER) || (trackerP->cType.laserInfo.parent.nType == OBJ_PLAYER)) {
	//	It's fired by a CPlayerData, so if robots present, track robot, else track player.
	return IsCoopGame ? 
			 FindHomingObjectComplete (vCurPos, trackerP, OBJ_ROBOT, -1) :
			 FindHomingObjectComplete (vCurPos, trackerP, OBJ_PLAYER, OBJ_ROBOT);
		} 
#if DBG
if ((trackerP->cType.laserInfo.parent.nType != OBJ_ROBOT) && (trackerP->cType.laserInfo.parent.nType != OBJ_PLAYER))
	trackerP = trackerP;
#endif
return FindHomingObjectComplete (vCurPos, trackerP, OBJ_PLAYER, gameStates.app.cheats.bRobotsKillRobots ? OBJ_ROBOT : -1);
}

//	--------------------------------------------------------------------------------------------
//	Find CObject to home in on.
//	Scan list of OBJECTS rendered last frame, find one that satisfies function of nearness to center and distance.
int FindHomingObject (CFixVector *vTrackerPos, CObject *trackerP)
{
	int	i, bOmega, bGuidedMslView;
	fix	maxDot = -I2X (2);
	int	nBestObj = -1;
	int	curMinTrackableDot;
	int	bSpectate;

//	Find an CObject to track based on game mode (eg, whether in network play) and who fired it.
#if DBG
if ((trackerP->info.nType == OBJ_WEAPON) && (trackerP->info.nId == SMARTMINE_BLOB_ID))
	nDbgObj = nDbgObj;
#endif
if (IsMultiGame)
	return CallFindHomingObjectComplete (trackerP, vTrackerPos);
if (trackerP->info.nType == OBJ_WEAPON) {
	bOmega = (trackerP->info.nId == OMEGA_ID);
	bGuidedMslView = (trackerP == GuidedInMainView ());
	}
else
	bOmega =
	bGuidedMslView = 0;
bSpectate = SPECTATOR (trackerP);
curMinTrackableDot = MIN_TRACKABLE_DOT;
if (bOmega)
	curMinTrackableDot = OMEGA_MIN_TRACKABLE_DOT;

//	Not in network mode.  If not fired by CPlayerData, then track player.
if ((trackerP->info.nType != OBJ_PLAYER) && (trackerP->cType.laserInfo.parent.nObject != LOCALPLAYER.nObject)) {
	if (!(LOCALPLAYER.flags & PLAYER_FLAGS_CLOAKED))
		nBestObj = OBJ_IDX (gameData.objs.consoleP);
	} 
else {
		int	nWindow = -1;
		fix	dist, maxTrackableDist;

	//	Find the window which has the forward view.
	for (i = 0; i < MAX_RENDERED_WINDOWS; i++)
		if ((windowRenderedData [i].nFrame >= gameData.app.nFrameCount - 1) &&
			 ((windowRenderedData [i].viewerP == gameData.objs.consoleP) || bGuidedMslView) &&
			 !windowRenderedData [i].bRearView) {
			nWindow = i;
			break;
			}

	//	Couldn't find suitable view from this frame, so do complete search.
	if (nWindow == -1)
		return CallFindHomingObjectComplete (trackerP, vTrackerPos);

	maxTrackableDist = MAX_TRACKABLE_DIST;
	if (EGI_FLAG (bEnhancedShakers, 0, 0, 0) && (trackerP->info.nType == OBJ_WEAPON) && (trackerP->info.nId == EARTHSHAKER_MEGA_ID))
		maxTrackableDist *= 2;
	else if (bOmega)
		maxTrackableDist = OMEGA_MAX_TRACKABLE_DIST;

	//	Not in network mode and fired by player.
	for (i = windowRenderedData [nWindow].nObjects - 1; i >= 0; i--) {
		fix			dot; //, dist;
		CFixVector	vecToCurObj;
		int			nObject = windowRenderedData [nWindow].renderedObjects [i];
		CObject		*curObjP = OBJECTS + nObject;

		if (nObject == LOCALPLAYER.nObject)
			continue;

		//	Can't track AI CObject if he's cloaked.
		if (curObjP->info.nType == OBJ_ROBOT) {
			if (curObjP->cType.aiInfo.CLOAKED)
				continue;
			//	Your missiles don't track your escort.
			if (ROBOTINFO (curObjP->info.nId).companion && 
				 ((trackerP->info.nType == OBJ_PLAYER) || (trackerP->cType.laserInfo.parent.nType == OBJ_PLAYER)))
				continue;
			}
		else if (curObjP->info.nType == OBJ_WEAPON) {
			if (!EGI_FLAG (bKillMissiles, 0, 1, 0) || !bOmega || !(gameData.objs.bIsMissile [curObjP->info.nId] || WeaponIsMine (curObjP->info.nId)))
				continue;
			}
		else if ((curObjP->info.nType != OBJ_PLAYER) && (curObjP->info.nType != OBJ_REACTOR))
			continue;
		vecToCurObj = curObjP->info.position.vPos - *vTrackerPos;
		dist = CFixVector::Normalize(vecToCurObj);
		if (dist < maxTrackableDist) {
			dot = CFixVector::Dot (vecToCurObj, bSpectate ? gameStates.app.playerPos.mOrient.FVec () : trackerP->info.position.mOrient.FVec ());

			//	Note: This uses the constant, not-scaled-by-frametime value, because it is only used
			//	to determine if an CObject is initially trackable.  FindHomingObject is called on subsequent
			//	frames to determine if the CObject remains trackable.
			if (dot > curMinTrackableDot) {
				if (dot > maxDot) {
					if (ObjectToObjectVisibility (trackerP, OBJECTS + nObject, FQ_TRANSWALL)) {
						maxDot = dot;
						nBestObj = nObject;
						}
					}
				} 
			else if (dot > I2X (1) - (I2X (1) - curMinTrackableDot) * 2) {
				CFixVector::Normalize(vecToCurObj);
				dot = CFixVector::Dot (vecToCurObj, trackerP->info.position.mOrient.FVec ());
				if (dot > curMinTrackableDot) {
					if (dot > maxDot) {
						if (ObjectToObjectVisibility (trackerP, OBJECTS + nObject, FQ_TRANSWALL)) {
							maxDot = dot;
							nBestObj = nObject;
							}
						}
					}
				}
			}
		}
	}
return nBestObj;
}

//	--------------------------------------------------------------------------------------------
//	Find CObject to home in on.
//	Scan list of OBJECTS rendered last frame, find one that satisfies function of nearness to center and distance.
//	Can track two kinds of OBJECTS.  If you are only interested in one nType, set trackObjType2 to NULL
//	Always track proximity bombs.  --MK, 06/14/95
//	Make homing OBJECTS not track parent's prox bombs.
int FindHomingObjectComplete (CFixVector *vCurPos, CObject *trackerP, int trackObjType1, int trackObjType2)
{
	fix		maxDot = -I2X (2);
	int		nBestObj = -1;
	fix		maxTrackableDist;
	fix		minTrackableDot;
	CObject	*curObjP;

	//	Contact Mike: This is a bad and stupid thing.  Who called this routine with an illegal laser nType??
//Assert ((WI_homingFlag (trackerP->info.nId)) || (trackerP->info.nId == OMEGA_ID));

maxTrackableDist = MAX_TRACKABLE_DIST;
if (EGI_FLAG (bEnhancedShakers, 0, 0, 0) && (trackerP->info.nType == OBJ_WEAPON) && (trackerP->info.nId == EARTHSHAKER_MEGA_ID)) {
	maxTrackableDist *= 2;
	minTrackableDot = -I2X (1);
	}
else
#if 0//def _DEBUG
	minTrackableDot = -I2X (1);
#else
	minTrackableDot = MIN_TRACKABLE_DOT;
#endif

if ((trackerP->info.nType == OBJ_WEAPON) && (trackerP->info.nId == OMEGA_ID)) {
	maxTrackableDist = OMEGA_MAX_TRACKABLE_DIST;
	minTrackableDot = OMEGA_MIN_TRACKABLE_DOT;
	}

FORALL_ACTOR_OBJS (curObjP, nObject) {
	int			bIsProximity = 0;
	fix			dot, dist;
	CFixVector	vecToCurObj;

	if ((curObjP->info.nType != trackObjType1) && (curObjP->info.nType != trackObjType2)) {
		if (curObjP->info.nType != OBJ_WEAPON) 
			continue;
		if (!WeaponIsPlayerMine (curObjP->info.nId))
			continue;
		if (curObjP->cType.laserInfo.parent.nSignature == trackerP->cType.laserInfo.parent.nSignature)
			continue;
		bIsProximity = 1;
		}
	if (OBJ_IDX (curObjP) == trackerP->cType.laserInfo.parent.nObject) // Don't track shooter
		continue;

	//	Don't track cloaked players.
	if (curObjP->info.nType == OBJ_PLAYER) {
		if (gameData.multiplayer.players [curObjP->info.nId].flags & PLAYER_FLAGS_CLOAKED)
			continue;
		// Don't track teammates in team games
		if (IsTeamGame) {
			CObject *parentObjP = OBJECTS + trackerP->cType.laserInfo.parent.nObject;
			if ((parentObjP->info.nType == OBJ_PLAYER) && (GetTeam (curObjP->info.nId) == GetTeam (parentObjP->info.nId)))
				continue;
			}
		}

	//	Can't track AI CObject if he's cloaked.
	if ((curObjP->info.nType == OBJ_ROBOT) && (curObjP->cType.aiInfo.CLOAKED))
		continue;
	//	Your missiles don't track your escort.
	if (ROBOTINFO (curObjP->info.nId).companion && (trackerP->cType.laserInfo.parent.nType == OBJ_PLAYER))
		continue;

	vecToCurObj = curObjP->info.position.vPos - *vCurPos;
	dist = vecToCurObj.Mag();

	if (dist >= maxTrackableDist)
		continue;
	CFixVector::Normalize(vecToCurObj);
	dot = CFixVector::Dot (vecToCurObj, trackerP->info.position.mOrient.FVec ());
	if (bIsProximity)
		dot = ((dot << 3) + dot) >> 3;		//	I suspect Watcom would be too stupid to figure out the obvious...

	//	Note: This uses the constant, not-scaled-by-frametime value, because it is only used
	//	to determine if an CObject is initially trackable.  FindHomingObject is called on subsequent
	//	frames to determine if the CObject remains trackable.
	if ((dot > minTrackableDot) && (dot > maxDot) && (ObjectToObjectVisibility (trackerP, curObjP, FQ_TRANSWALL))) {
		maxDot = dot;
		nBestObj = OBJ_IDX (curObjP);
		}
	}
return nBestObj;
}

//	------------------------------------------------------------------------------------------------------------
//	See if legal to keep tracking currently tracked CObject.  If not, see if another CObject is trackable.  If not, return -1,
//	else return CObject number of tracking CObject.
//	Computes and returns a fairly precise dot product.
int TrackHomingTarget (int nHomingTarget, CObject *trackerP, fix *dot)
{
	int	rVal = -2;
	int	nFrame;
	int	goalType, goal2Type = -1;

//if (!gameOpts->legacy.bHomers && gameStates.limitFPS.bHomers && !gameStates.app.tick40fps.bTick)
	//	Every 8 frames for each CObject, scan all OBJECTS.
nFrame = OBJ_IDX (trackerP) ^ gameData.app.nFrameCount;
if (ObjectIsTrackeable (nHomingTarget, trackerP, dot)) {
	if (gameOpts->legacy.bHomers) {
		if (nFrame % 8)
			return nHomingTarget;
		}
	else {
		if (gameStates.limitFPS.bHomers && !gameStates.app.tick40fps.bTick)
			return nHomingTarget;
		}
	}

if (!gameOpts->legacy.bHomers || (nFrame % 4 == 0)) {
	//	If CPlayerData fired missile, then search for an CObject, if not, then give up.
	if (OBJECTS [trackerP->cType.laserInfo.parent.nObject].info.nType == OBJ_PLAYER) {
		if (nHomingTarget == -1) {
			if (IsMultiGame) {
				if (IsCoopGame)
					rVal = FindHomingObjectComplete (&trackerP->info.position.vPos, trackerP, OBJ_ROBOT, -1);
				else if (gameData.app.nGameMode & GM_MULTI_ROBOTS)		//	Not cooperative, if robots, track either robot or CPlayerData
					rVal = FindHomingObjectComplete (&trackerP->info.position.vPos, trackerP, OBJ_PLAYER, OBJ_ROBOT);
				else		//	Not cooperative and no robots, track only a CPlayerData
					rVal = FindHomingObjectComplete (&trackerP->info.position.vPos, trackerP, OBJ_PLAYER, -1);
				}
			else
				rVal = FindHomingObjectComplete (&trackerP->info.position.vPos, trackerP, OBJ_PLAYER, OBJ_ROBOT);
			} 
		else {
			goalType = OBJECTS [trackerP->cType.laserInfo.nHomingTarget].info.nType;
			if ((goalType == OBJ_PLAYER) || (goalType == OBJ_ROBOT))
				rVal = FindHomingObjectComplete (&trackerP->info.position.vPos, trackerP, goalType, -1);
			else
				rVal = -1;
			}
		} 
	else {
		if (gameStates.app.cheats.bRobotsKillRobots)
			goal2Type = OBJ_ROBOT;
		if (nHomingTarget == -1)
			rVal = FindHomingObjectComplete (&trackerP->info.position.vPos, trackerP, OBJ_PLAYER, goal2Type);
		else {
			goalType = OBJECTS [trackerP->cType.laserInfo.nHomingTarget].info.nType;
			rVal = FindHomingObjectComplete (&trackerP->info.position.vPos, trackerP, goalType, goal2Type);
			}
		}
	Assert (rVal != -2);		//	This means it never got set which is bad! Contact Mike.
	return rVal;
	}
return -1;
}

//	-----------------------------------------------------------------------------------------------------------
